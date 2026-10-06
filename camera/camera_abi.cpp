/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "a11-camera-abi"

#include <errno.h>
#include <limits.h>
#include <limits>
#include <memory>
#include <new>
#include <stdlib.h>
#include <camera/CameraParameters.h>
#include <log/log.h>
#include <sensor/SensorEventQueue.h>
#include <sensor/SensorManager.h>
#include <ui/GraphicBuffer.h>
#include <utils/Mutex.h>
#include <utils/String8.h>

using android::CameraParameters;
using android::GraphicBuffer;
using android::Mutex;
using android::Sensor;
using android::SensorEventQueue;
using android::SensorManager;
using android::String8;
using android::sp;

/* The blobs allocate 36 bytes and inline the KitKat singleton access.
 * A pointer-sized adapter keeps the larger SensorManager in libsensor. */
struct LegacySensorManager {
    SensorManager *manager;
};
static_assert(sizeof(LegacySensorManager) <= 36, "KitKat manager allocation");

extern "C" {
Mutex a11_sensor_lock(Mutex::PRIVATE);
LegacySensorManager *a11_sensor_instance = nullptr;

/* The HAL runs in the binderized camera provider, whose vendor domain
 * cannot look up sensorservice, and SensorManager::getInstanceForPackage
 * retries that lookup without end, which blocks camera open. The adapter
 * holds no libsensor manager: a11_sensor_default answers null and
 * a11_sensor_queue_create an empty queue. libcameraface's SensorListener
 * returns from initialize() on an empty queue and skips enableSensor() and
 * disableSensor() on a null sensor, so face detection runs without
 * orientation input. */
void *a11_sensor_manager_construct(LegacySensorManager *self)
{
    new (self) LegacySensorManager{nullptr};
    ALOGW("SensorManager at %p: sensor events disabled", self);
    return self;
}

const Sensor *a11_sensor_default(LegacySensorManager *self, int type)
{
    return self->manager != nullptr ? self->manager->getDefaultSensor(type) : nullptr;
}

/* ARM's nontrivial sp return uses a hidden output pointer before this. */
void a11_sensor_queue_create(sp<SensorEventQueue> *result, LegacySensorManager *self)
{
    if (self->manager == nullptr) {
        new (result) sp<SensorEventQueue>();
        return;
    }
    new (result) sp<SensorEventQueue>(self->manager->createEventQueue(String8(""), 0));
}

ssize_t a11_sensor_queue_read(SensorEventQueue *self, ASensorEvent *events, size_t count)
{
    ssize_t received = self->read(events, count);
    if (received > 0)
        self->sendAck(events, static_cast<int>(received));
    return received;
}

int a11_sensor_queue_fd(const SensorEventQueue *self)
{
    return self->getFd();
}

int a11_sensor_enable(const SensorEventQueue *self, const Sensor *sensor)
{
    return self->enableSensor(sensor);
}

int a11_sensor_disable(const SensorEventQueue *self, const Sensor *sensor)
{
    return self->disableSensor(sensor);
}

int a11_sensor_rate(const SensorEventQueue *self, const Sensor *sensor, int64_t period)
{
    return self->setEventRate(sensor, period);
}
}

/* HTC allocates 120 bytes for GraphicBuffer. RefBase remains the primary
 * base at offset zero; all buffer operations use the separately allocated
 * Android 11 buffer. Returning its native buffer also preserves gralloc
 * callbacks and the stride/handle fields consumed by the effects libraries. */
class LegacyGraphicBuffer : public android::RefBase {
public:
    LegacyGraphicBuffer(uint32_t width, uint32_t height, int format, uint32_t usage)
        : buffer(new GraphicBuffer(width, height, format, usage, "a11-camera")) {}

    sp<GraphicBuffer> buffer;
};
static_assert(sizeof(LegacyGraphicBuffer) <= 120, "KitKat buffer allocation");
static_assert(sizeof(void *) == 4, "HTC camera ABI is ARM32");
static_assert(offsetof(ANativeWindowBuffer, stride) == 0x28, "HTC native buffer stride");
static_assert(offsetof(ANativeWindowBuffer, handle) == 0x3c, "HTC native buffer handle");

extern "C" void *a11_buffer_construct(void *self, uint32_t width, uint32_t height,
        int format, uint32_t usage)
{
    return new (self) LegacyGraphicBuffer(width, height, format, usage);
}

extern "C" int a11_buffer_lock(LegacyGraphicBuffer *self, uint32_t usage, void **address)
{
    return self->buffer->lock(usage, address);
}

extern "C" int a11_buffer_unlock(LegacyGraphicBuffer *self)
{
    return self->buffer->unlock();
}

extern "C" ANativeWindowBuffer *a11_buffer_native(LegacyGraphicBuffer *self)
{
    return self->buffer->getNativeBuffer();
}

/* HTC parameter keys add symbols without changing CameraParameters' map
 * layout. Keep the legacy methods functional over the framework parser. */
#define HTC_PARAMETER(symbol, value) \
    extern "C" const char symbol[] = value

HTC_PARAMETER(a11_capture_eis, "eis");
HTC_PARAMETER(a11_capture_hdr, "hdr");
HTC_PARAMETER(a11_capture_zoe, "zoe");
HTC_PARAMETER(a11_capture_key, "capture-mode");
HTC_PARAMETER(a11_capture_normal, "normal");
HTC_PARAMETER(a11_capture_panorama, "panorama");
HTC_PARAMETER(a11_burst_capturing, "contiburst-capturing");
HTC_PARAMETER(a11_burst_state, "contiburst-state");
HTC_PARAMETER(a11_capture_burst, "contiburst");
HTC_PARAMETER(a11_burst_done, "contiburst-done");
HTC_PARAMETER(a11_force_audio, "forceuseaudio");
HTC_PARAMETER(a11_capture_supported, "capture-mode-values");
HTC_PARAMETER(a11_capture_burst_single, "contiburst-one-shot");

static void parameter_pair(const CameraParameters *self, const char *key,
        char separator, int *first, int *second)
{
    *first = *second = -1;
    const char *value = self->get(key);
    if (!value)
        return;
    char *end;
    errno = 0;
    long first_value = strtol(value, &end, 10);
    if (end == value || *end != separator || errno || first_value < INT_MIN || first_value > INT_MAX)
        return;
    const char *second_text = end + 1;
    long second_value = strtol(second_text, &end, 10);
    if (end == second_text || *end || errno || second_value < INT_MIN || second_value > INT_MAX)
        return;
    *first = static_cast<int>(first_value);
    *second = static_cast<int>(second_value);
}

extern "C" void a11_parameter_raw(const CameraParameters *self, int *width, int *height)
{
    parameter_pair(self, "raw-size", 'x', width, height);
}

extern "C" void a11_parameter_luma(const CameraParameters *self, int *brightness, int *luma)
{
    parameter_pair(self, "brightness-luma-target-set", ',', brightness, luma);
}
