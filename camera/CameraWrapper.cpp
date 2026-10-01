/*
 * Copyright (C) 2014, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * camera.msm8226 for the a11 on Android 11: a HAL1 module that loads the
 * stock KitKat QCamera2 HAL (installed as camera.vendor.msm8226.so) and
 * presents it to the legacy camera provider.
 *
 * The vendor module declares CAMERA_MODULE_API_VERSION_1_0 and
 * CAMERA_DEVICE_API_VERSION_1_0 (its static hw_module_t template, copied into
 * HMI by a constructor). camera_device_ops_t and its callback typedefs are
 * unchanged between KitKat and Android 11. Allocation callbacks restore the
 * framework cookie because QCameraStreamMemory supplies a stream object. The
 * module-level structures differ: Android 11's camera_module_t carries open_legacy,
 * set_torch_mode and init where KitKat's reserved[] sat, and Android 11's camera_info
 * appends resource_cost and the conflicting-device list after the four fields
 * KitKat's HAL writes. This module presents the Android 11 layout, leaves the three
 * newer entry points NULL (module API 1.0, so the provider drives torch through
 * a HAL1 device and never calls them), and zero-fills camera_info before the
 * vendor HAL writes its KitKat prefix.
 *
 * get_parameters hands CameraService a string this module owns: the vendor
 * string goes back through the vendor's own put_parameters at once, because
 * only the vendor HAL knows how it allocated it.
 */

#define LOG_TAG "CameraWrapper"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <log/log.h>
#include <hardware/camera.h>
#include <hardware/hardware.h>

/* Written once under gVendorModuleOnce and read-only afterwards. */
static pthread_once_t gVendorModuleOnce = PTHREAD_ONCE_INIT;
static camera_module_t *gVendorModule;
static int gVendorModuleStatus;

/* QCameraStreamMemory supplies its own object as the allocation cookie.
 * Separate entry points recover the framework cookie for each camera. */
static pthread_mutex_t gMemoryCallbackLock = PTHREAD_MUTEX_INITIALIZER;
static struct {
    bool opened;
    camera_request_memory get_memory;
    void *user;
} gMemoryCallbacks[2];

template <unsigned int CameraId>
static camera_memory_t *camera_get_memory(int fd, size_t buf_size,
        unsigned int num_bufs, void *user)
{
    (void)user;
    pthread_mutex_lock(&gMemoryCallbackLock);
    camera_request_memory get_memory = gMemoryCallbacks[CameraId].get_memory;
    void *framework_user = gMemoryCallbacks[CameraId].user;
    pthread_mutex_unlock(&gMemoryCallbackLock);
    if (!get_memory)
        return NULL;
    return get_memory(fd, buf_size, num_bufs, framework_user);
}

static void clear_memory_callback(unsigned int camera_id)
{
    pthread_mutex_lock(&gMemoryCallbackLock);
    memset(&gMemoryCallbacks[camera_id], 0, sizeof(gMemoryCallbacks[camera_id]));
    pthread_mutex_unlock(&gMemoryCallbackLock);
}

static int camera_device_open(const hw_module_t *module, const char *name,
        hw_device_t **device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

static struct hw_module_methods_t camera_module_methods = {
    .open = camera_device_open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = CAMERA_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "a11 Camera Wrapper",
        .author = "The CyanogenMod Project",
        .methods = &camera_module_methods,
        .dso = NULL,
        .reserved = {0},
    },
    .get_number_of_cameras = camera_get_number_of_cameras,
    .get_camera_info = camera_get_camera_info,
    .set_callbacks = NULL,
    .get_vendor_tag_ops = NULL,
    .open_legacy = NULL,
    .set_torch_mode = NULL,
    .init = NULL,
    .get_physical_camera_info = NULL,
    .is_stream_combination_supported = NULL,
    .notify_device_state_change = NULL,
    .reserved = {0},
};

typedef struct wrapper_camera_device {
    camera_device_t base;
    int id;
    camera_device_t *vendor;
} wrapper_camera_device_t;

static camera_device_t *vendor_device(struct camera_device *device)
{
    return reinterpret_cast<wrapper_camera_device_t *>(device)->vendor;
}

#define VENDOR_CALL(device, func, ...) ({ \
    camera_device_t *__vendor = vendor_device(device); \
    __vendor->ops->func(__vendor, ##__VA_ARGS__); \
})

static void load_vendor_module(void)
{
    const hw_module_t *module = NULL;
    gVendorModuleStatus = hw_get_module_by_class("camera", "vendor", &module);
    if (gVendorModuleStatus) {
        ALOGE("%s: failed to open vendor camera module: %d", __func__,
                gVendorModuleStatus);
        return;
    }
    gVendorModule = reinterpret_cast<camera_module_t *>(
            const_cast<hw_module_t *>(module));
}

/* Loads camera.vendor.<platform>.so on first use; 0 once it is loaded. */
static int check_vendor_module(void)
{
    pthread_once(&gVendorModuleOnce, load_vendor_module);
    return gVendorModule ? 0 : (gVendorModuleStatus ? gVendorModuleStatus : -ENODEV);
}

static int camera_get_number_of_cameras(void)
{
    if (check_vendor_module())
        return 0;
    return gVendorModule->get_number_of_cameras();
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    if (!info)
        return -EINVAL;
    if (check_vendor_module())
        return -ENODEV;

    /* The KitKat HAL writes facing, orientation, device_version and
     * static_camera_characteristics; the extended tail stays zero. */
    memset(info, 0, sizeof(*info));
    return gVendorModule->get_camera_info(camera_id, info);
}

/*******************************************************************
 * camera_device_ops: pass-through to the vendor device
 *******************************************************************/

static int camera_set_preview_window(struct camera_device *device,
        struct preview_stream_ops *window)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, set_preview_window, window);
}

static void camera_set_callbacks(struct camera_device *device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    if (!device)
        return;
    wrapper_camera_device_t *wrapper_dev =
            reinterpret_cast<wrapper_camera_device_t *>(device);
    pthread_mutex_lock(&gMemoryCallbackLock);
    gMemoryCallbacks[wrapper_dev->id].get_memory = get_memory;
    gMemoryCallbacks[wrapper_dev->id].user = user;
    pthread_mutex_unlock(&gMemoryCallbackLock);
    camera_request_memory vendor_get_memory = NULL;
    if (get_memory)
        vendor_get_memory = wrapper_dev->id == 0 ? camera_get_memory<0> :
                camera_get_memory<1>;
    VENDOR_CALL(device, set_callbacks, notify_cb, data_cb, data_cb_timestamp,
            vendor_get_memory, user);
}

static void camera_enable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return;
    VENDOR_CALL(device, enable_msg_type, msg_type);
}

static void camera_disable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return;
    VENDOR_CALL(device, disable_msg_type, msg_type);
}

static int camera_msg_type_enabled(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return 0;
    return VENDOR_CALL(device, msg_type_enabled, msg_type);
}

static int camera_start_preview(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, start_preview);
}

static void camera_stop_preview(struct camera_device *device)
{
    if (!device)
        return;
    VENDOR_CALL(device, stop_preview);
}

static int camera_preview_enabled(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, preview_enabled);
}

static int camera_store_meta_data_in_buffers(struct camera_device *device,
        int enable)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, store_meta_data_in_buffers, enable);
}

static int camera_start_recording(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, start_recording);
}

static void camera_stop_recording(struct camera_device *device)
{
    if (!device)
        return;
    VENDOR_CALL(device, stop_recording);
}

static int camera_recording_enabled(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, recording_enabled);
}

static void camera_release_recording_frame(struct camera_device *device,
        const void *opaque)
{
    if (!device)
        return;
    VENDOR_CALL(device, release_recording_frame, opaque);
}

static int camera_auto_focus(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, auto_focus);
}

static int camera_cancel_auto_focus(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, cancel_auto_focus);
}

static int camera_take_picture(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, take_picture);
}

static int camera_cancel_picture(struct camera_device *device)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, cancel_picture);
}

static int camera_set_parameters(struct camera_device *device,
        const char *params)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, set_parameters, params);
}

/* Returns a malloc'd copy owned by this module; camera_put_parameters frees
 * it. The vendor string is released through the vendor's put_parameters. */
static char *camera_get_parameters(struct camera_device *device)
{
    if (!device)
        return NULL;

    char *vendor_params = VENDOR_CALL(device, get_parameters);
    if (!vendor_params)
        return NULL;

    char *params = strdup(vendor_params);
    VENDOR_CALL(device, put_parameters, vendor_params);
    if (!params)
        ALOGE("%s: out of memory copying parameters", __func__);
    return params;
}

static void camera_put_parameters(struct camera_device *device, char *params)
{
    (void)device;
    free(params);
}

static int camera_send_command(struct camera_device *device,
        int32_t cmd, int32_t arg1, int32_t arg2)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, send_command, cmd, arg1, arg2);
}

static void camera_release(struct camera_device *device)
{
    if (!device)
        return;
    VENDOR_CALL(device, release);
}

static int camera_dump(struct camera_device *device, int fd)
{
    if (!device)
        return -EINVAL;
    return VENDOR_CALL(device, dump, fd);
}

static int camera_device_close(hw_device_t *device)
{
    if (!device)
        return -EINVAL;

    wrapper_camera_device_t *wrapper_dev =
            reinterpret_cast<wrapper_camera_device_t *>(device);
    /* Keep the allocation callback available through vendor shutdown. */
    int rv = wrapper_dev->vendor->common.close(&wrapper_dev->vendor->common);
    clear_memory_callback(wrapper_dev->id);
    free(wrapper_dev->base.ops);
    free(wrapper_dev);
    return rv;
}

/*******************************************************************
 * camera_module functions
 *******************************************************************/

static int camera_device_open(const hw_module_t *module, const char *name,
        hw_device_t **device)
{
    if (!name || !device)
        return -EINVAL;
    if (check_vendor_module())
        return -ENODEV;

    char *end = NULL;
    long camera_id = strtol(name, &end, 10);
    int num_cameras = gVendorModule->get_number_of_cameras();
    if (end == name || *end != '\0' || camera_id < 0 || camera_id >= num_cameras) {
        ALOGE("%s: camera id \"%s\" out of range (%d cameras)", __func__,
                name, num_cameras);
        return -EINVAL;
    }
    if (camera_id >= static_cast<long>(sizeof(gMemoryCallbacks) /
            sizeof(gMemoryCallbacks[0])))
        return -EINVAL;

    wrapper_camera_device_t *camera_device = static_cast<wrapper_camera_device_t *>(
            calloc(1, sizeof(*camera_device)));
    camera_device_ops_t *camera_ops = static_cast<camera_device_ops_t *>(
            calloc(1, sizeof(*camera_ops)));
    if (!camera_device || !camera_ops) {
        ALOGE("%s: out of memory", __func__);
        free(camera_device);
        free(camera_ops);
        return -ENOMEM;
    }

    camera_device->id = static_cast<int>(camera_id);
    pthread_mutex_lock(&gMemoryCallbackLock);
    if (gMemoryCallbacks[camera_id].opened) {
        pthread_mutex_unlock(&gMemoryCallbackLock);
        free(camera_device);
        free(camera_ops);
        return -EBUSY;
    }
    gMemoryCallbacks[camera_id].opened = true;
    pthread_mutex_unlock(&gMemoryCallbackLock);
    int rv = gVendorModule->common.methods->open(
            reinterpret_cast<const hw_module_t *>(gVendorModule), name,
            reinterpret_cast<hw_device_t **>(&camera_device->vendor));
    if (rv) {
        clear_memory_callback(camera_id);
        ALOGE("%s: vendor open of camera %ld failed: %d", __func__, camera_id, rv);
        free(camera_device);
        free(camera_ops);
        return rv;
    }

    camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
    camera_device->base.common.version = camera_device->vendor->common.version;
    camera_device->base.common.module = const_cast<hw_module_t *>(module);
    camera_device->base.common.close = camera_device_close;
    camera_device->base.ops = camera_ops;

    camera_ops->set_preview_window = camera_set_preview_window;
    camera_ops->set_callbacks = camera_set_callbacks;
    camera_ops->enable_msg_type = camera_enable_msg_type;
    camera_ops->disable_msg_type = camera_disable_msg_type;
    camera_ops->msg_type_enabled = camera_msg_type_enabled;
    camera_ops->start_preview = camera_start_preview;
    camera_ops->stop_preview = camera_stop_preview;
    camera_ops->preview_enabled = camera_preview_enabled;
    camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
    camera_ops->start_recording = camera_start_recording;
    camera_ops->stop_recording = camera_stop_recording;
    camera_ops->recording_enabled = camera_recording_enabled;
    camera_ops->release_recording_frame = camera_release_recording_frame;
    camera_ops->auto_focus = camera_auto_focus;
    camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
    camera_ops->take_picture = camera_take_picture;
    camera_ops->cancel_picture = camera_cancel_picture;
    camera_ops->set_parameters = camera_set_parameters;
    camera_ops->get_parameters = camera_get_parameters;
    camera_ops->put_parameters = camera_put_parameters;
    camera_ops->send_command = camera_send_command;
    camera_ops->release = camera_release;
    camera_ops->dump = camera_dump;

    *device = &camera_device->base.common;
    return 0;
}
