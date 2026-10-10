/*
 * Copyright (C) 2019 The LineageOS Project
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

#include "TouchscreenGesture.h"

#include <android-base/file.h>
#include <android-base/logging.h>

namespace vendor {
namespace lineage {
namespace touch {
namespace V1_0 {
namespace implementation {

namespace {

// The Himax driver registers one switch per wake gesture under
// /sys/android_touch; a write of 1 enables it and 0 disables it. The keycode
// is a scan code no input device emits: the kernel wakes the screen itself by
// injecting KEY_POWER from its wake_pwrkey device.
struct GestureNode {
    int32_t id;
    const char* name;
    int32_t keycode;
    const char* node;
};

constexpr GestureNode kGestures[] = {
        {0, "Sweep to wake", 0x2f0, "/sys/android_touch/sweep2wake"},
        {1, "Double tap to wake", 0x2f1, "/sys/android_touch/doubletap2wake"},
};

}  // namespace

// Methods from ::vendor::lineage::touch::V1_0::ITouchscreenGesture follow.
Return<void> TouchscreenGesture::getSupportedGestures(getSupportedGestures_cb _hidl_cb) {
    hidl_vec<Gesture> gestures;
    gestures.resize(sizeof(kGestures) / sizeof(kGestures[0]));
    for (size_t i = 0; i < gestures.size(); i++) {
        gestures[i].id = kGestures[i].id;
        gestures[i].name = kGestures[i].name;
        gestures[i].keycode = kGestures[i].keycode;
    }
    _hidl_cb(gestures);
    return Void();
}

Return<bool> TouchscreenGesture::setGestureEnabled(
        const ::vendor::lineage::touch::V1_0::Gesture& gesture, bool enabled) {
    for (const GestureNode& g : kGestures) {
        if (g.id != gesture.id) continue;
        if (!android::base::WriteStringToFile(enabled ? "1" : "0", g.node)) {
            PLOG(ERROR) << "Failed to write " << g.node;
            return false;
        }
        return true;
    }
    LOG(ERROR) << "Unknown gesture id " << gesture.id;
    return false;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace touch
}  // namespace lineage
}  // namespace vendor
