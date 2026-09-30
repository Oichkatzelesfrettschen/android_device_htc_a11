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
 * sensors.a11: wraps the stock a11chl sensors HAL (installed as
 * sensors.vendor.msm8226.so, hw_module_t.hal_api_version 0 --
 * SENSORS_DEVICE_API_VERSION_0_1, the oldest legacy value) as a sub-HAL of
 * the msm8226 multihal, which SensorService reaches through
 * android.hardware.sensors@1.0.
 *
 * The vendor HAL's static sensor_t table (read directly from its .data.rel.ro
 * section: name, vendor, version, handle, type, in that order) carries four
 * entries, all with a real, non-null name and vendor:
 *
 *   handle=0 type=1  (ACCELEROMETER)  "BOSCH BMA250 3-axis Accelerometer" / "BOSCH"
 *   handle=1 type=8  (PROXIMITY)      "CM36282 Proximity sensor" / "Capella Microsystems"
 *   handle=2 type=5  (LIGHT)          "CM36282 Light sensor" / "Capella Microsystems"
 *   handle=3 type=21 (HEART_RATE!)    "HTC Gesture sensor" / "HTC"
 *
 * The fourth entry's type, 21, is frameworks/native/libs/gui/Sensor.h's
 * SENSOR_TYPE_HEART_RATE -- a collision, not a capability: this HAL predates
 * that constant (stringType and requiredPermission are both null, unlike a
 * real M-era heart-rate sensor's HAL entry), and the Desire 510 has no heart
 * rate hardware. frameworks/native/libs/gui/Sensor.cpp's
 * SENSOR_TYPE_HEART_RATE case runs unconditionally on every HAL version --
 * unlike every other case, it is not guarded by halVersion -- and
 * unconditionally constructs an AppOpsManager and calls
 * permissionToOpCode(), which SIGSEGVs at fault address 0 in SensorService's
 * init thread on every boot: system_server's IPCThreadState/ProcessState
 * binder plumbing is not yet ready this early in bring-up, before
 * SystemServer's own binder thread pool starts.
 *
 * This module forwards the vendor HAL's device open()/close()/activate()/
 * setDelay()/poll() and set_operation_mode() untouched -- reporting the
 * vendor device's own version, not an invented one -- and patches
 * get_sensors_list()'s returned array in two ways: it remaps the "HTC
 * Gesture sensor" entry's type from the colliding SENSOR_TYPE_HEART_RATE to
 * SENSOR_TYPE_DEVICE_PRIVATE_BASE (sensors.h's own reserved range for a type
 * Android does not define), which routes it through Sensor.cpp's default:
 * case -- memory-safe for a null stringType/requiredPermission, and no
 * AppOpsManager call -- without claiming it is any specific gesture this
 * module cannot confirm; and it defends every entry's name/vendor against a
 * future vendor blob revision leaving either null, though neither is null on
 * the HAL this module wraps today. It adds no capability the vendor HAL does
 * not already report.
 */

#define LOG_TAG "SensorsWrapper"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/sensors.h>

#define MAX_SENSORS 16

static pthread_once_t gVendorModuleOnce = PTHREAD_ONCE_INIT;
static struct sensors_module_t *gVendorModule;
static int gVendorModuleStatus;

static struct sensor_t patched_list[MAX_SENSORS];

static void load_vendor_module(void)
{
    const hw_module_t *module = NULL;
    gVendorModuleStatus = hw_get_module_by_class("sensors", "vendor", &module);
    if (gVendorModuleStatus) {
        ALOGE("%s: failed to open vendor sensors module: %d", __func__,
                gVendorModuleStatus);
        return;
    }
    gVendorModule = (struct sensors_module_t *)(void *)(uintptr_t)module;
}

/* Loads sensors.vendor.<platform>.so on first use; 0 once it is loaded. */
static int check_vendor_module(void)
{
    pthread_once(&gVendorModuleOnce, load_vendor_module);
    return gVendorModule ? 0 : (gVendorModuleStatus ? gVendorModuleStatus : -ENODEV);
}

/*
 * The three sensor types this HAL reports as a standard Android type
 * (accelerometer, light, proximity) get their vendor from the well-known
 * part the "BOSCH"/"CM3"-prefixed name string identifies; the HTC gesture
 * sensor is HTC's own. A name this table does not recognize -- there are
 * none on this HAL, but a future vendor blob could add one -- falls back to
 * "unknown", never a fabricated part number.
 */
static const char *vendor_for_name(const char *name)
{
    if (name != NULL) {
        if (strstr(name, "BOSCH") != NULL || strstr(name, "BMA") != NULL)
            return "Bosch Sensortec";
        if (strstr(name, "CM3") != NULL)
            return "Capella Microsystems";
        if (strstr(name, "HTC") != NULL)
            return "HTC";
    }
    return "unknown";
}

static int sensors_get_sensors_list(struct sensors_module_t *module,
        struct sensor_t const **list)
{
    struct sensor_t const *real_list;
    int count, i;

    (void)module;
    if (check_vendor_module() || gVendorModule->get_sensors_list == NULL) {
        *list = NULL;
        return 0;
    }

    count = gVendorModule->get_sensors_list(gVendorModule, &real_list);
    if (count < 0)
        count = 0;
    if (count > MAX_SENSORS) {
        ALOGE("%s: vendor HAL reports %d sensors, capping at %d", __func__,
                count, MAX_SENSORS);
        count = MAX_SENSORS;
    }

    for (i = 0; i < count; i++) {
        patched_list[i] = real_list[i];
        if (patched_list[i].name == NULL || patched_list[i].name[0] == '\0')
            patched_list[i].name = "unknown sensor";
        if (patched_list[i].vendor == NULL || patched_list[i].vendor[0] == '\0')
            patched_list[i].vendor = vendor_for_name(patched_list[i].name);
        /* SENSOR_TYPE_HEART_RATE (21) is a numeric collision with this pre-M
         * HAL's own "gesture" type, not a real capability (see the file
         * header); Sensor.cpp's HEART_RATE case is the one case that is not
         * halVersion-guarded, and it crashes unconditionally. */
        if (patched_list[i].type == SENSOR_TYPE_HEART_RATE)
            patched_list[i].type = SENSOR_TYPE_DEVICE_PRIVATE_BASE;
    }

    *list = patched_list;
    return count;
}

static int sensors_set_operation_mode(unsigned int mode)
{
    if (check_vendor_module() || gVendorModule->set_operation_mode == NULL)
        return mode == 0 ? 0 : -EINVAL;
    return gVendorModule->set_operation_mode(mode);
}

static int sensors_device_open(const struct hw_module_t *module, const char *id,
        struct hw_device_t **device)
{
    (void)module;
    if (check_vendor_module() || gVendorModule->common.methods == NULL ||
            gVendorModule->common.methods->open == NULL) {
        return -ENODEV;
    }
    /* Forward to the vendor module's own open(); the returned device is the
     * vendor's, with its own version and activate/setDelay/poll --
     * unmodified, since the crash this module fixes is in list
     * construction, not device polling. */
    return gVendorModule->common.methods->open(&gVendorModule->common, id, device);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = sensors_device_open,
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "a11 Sensors Wrapper",
        .author = "The CyanogenMod Project",
        .methods = &sensors_module_methods,
        .dso = NULL,
        .reserved = {0},
    },
    .get_sensors_list = sensors_get_sensors_list,
    .set_operation_mode = sensors_set_operation_mode,
};
