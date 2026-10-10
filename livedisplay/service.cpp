/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_TAG "vendor.lineage.livedisplay@2.0-service.a11"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <livedisplay/sysfs/AdaptiveBacklight.h>
#include <livedisplay/sysfs/DisplayColorCalibration.h>

using ::android::OK;
using ::android::sp;
using ::android::status_t;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;

using ::vendor::lineage::livedisplay::V2_0::sysfs::AdaptiveBacklight;
using ::vendor::lineage::livedisplay::V2_0::sysfs::DisplayColorCalibration;

// The device manifest declares both instances, and LineageHardwareManager
// reaches a declared HIDL instance with getService(true), which waits until
// the instance registers. Both instances register unconditionally: a missing
// sysfs node fails the call and logs here, while a skipped registration leaves
// system_server waiting.
int main() {
    configureRpcThreadpool(1, true /*callerWillJoin*/);

    sp<AdaptiveBacklight> ab = new AdaptiveBacklight();
    if (!ab->isSupported()) {
        LOG(ERROR) << "lcd-backlight cabc_level_ctl is not accessible";
    }
    status_t status = ab->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register LiveDisplay IAdaptiveBacklight (" << status << ")";
        return 1;
    }

    sp<DisplayColorCalibration> dcc = new DisplayColorCalibration();
    if (!DisplayColorCalibration::isSupported()) {
        LOG(ERROR) << "kcal_ctrl.0 kcal and kcal_ctrl are not accessible";
    }
    status = dcc->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register LiveDisplay IDisplayColorCalibration (" << status << ")";
        return 1;
    }

    LOG(DEBUG) << "LiveDisplay HAL service is ready.";
    joinRpcThreadpool();

    // The thread pool does not return in normal operation.
    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return 1;
}
