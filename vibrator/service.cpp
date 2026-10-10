/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_TAG "android.hardware.vibrator-service.a11"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "Vibrator.h"

using ::aidl::android::hardware::vibrator::Vibrator;

int main() {
    // One binder thread serializes the sysfs writes behind setAmplitude() and on().
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<Vibrator> vibrator = ndk::SharedRefBase::make<Vibrator>();
    const std::string instance = std::string() + Vibrator::descriptor + "/default";
    const binder_status_t status =
            AServiceManager_addService(vibrator->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK) << "Cannot register " << instance;

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // joinThreadPool() does not return in normal operation
}
