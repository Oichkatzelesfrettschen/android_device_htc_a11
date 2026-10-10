/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_TAG "vendor.lineage.livedisplay@2.0-service.a11"

#include "livedisplay/sysfs/DisplayColorCalibration.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using ::android::base::ReadFileToString;
using ::android::base::StringPrintf;
using ::android::base::WriteStringToFile;
using ::android::hardware::Void;

namespace {
// kcal_ctrl_probe() (arch/arm/mach-msm/kcal_ctrl.c) creates these attributes
// on the kcal_ctrl platform device. kcal holds "R G B"; kcal_set_values()
// clamps each channel to 0..255 and raises it to kcal_min. A write to kcal
// updates the stored scale; writing 1 to kcal_ctrl rebuilds the MDP gamma LUT
// (update_preset_lcdc_lut), which the next frame commit loads.
constexpr const char* kFileKcal = "/sys/devices/platform/kcal_ctrl.0/kcal";
constexpr const char* kFileKcalCtrl = "/sys/devices/platform/kcal_ctrl.0/kcal_ctrl";
constexpr const char* kFileKcalMin = "/sys/devices/platform/kcal_ctrl.0/kcal_min";

// g_kcal_min starts at 35 in board_kcal.c; a channel below it reads back as
// kcal_min.
constexpr int32_t kChannelMax = 255;
constexpr int32_t kChannelFloorDefault = 35;

int32_t ReadChannelFloor() {
    std::string contents;
    if (!ReadFileToString(kFileKcalMin, &contents)) {
        return kChannelFloorDefault;
    }
    return std::clamp<int32_t>(std::strtol(contents.c_str(), nullptr, 10), 0, kChannelMax);
}
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sysfs {

bool DisplayColorCalibration::isSupported() {
    return !access(kFileKcal, R_OK | W_OK) && !access(kFileKcalCtrl, W_OK);
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IDisplayColorCalibration follow.
Return<int32_t> DisplayColorCalibration::getMaxValue() {
    return kChannelMax;
}

Return<int32_t> DisplayColorCalibration::getMinValue() {
    return ReadChannelFloor();
}

// LineageHardwareManager reads fewer than three values as no calibration, and
// DisplayHardwareController indexes the result when it animates a color change
// inside system_server. The reply therefore always carries three channels, and
// an unreadable kcal reports the unscaled white point.
Return<void> DisplayColorCalibration::getCalibration(getCalibration_cb _hidl_cb) {
    std::vector<int32_t> rgb(3, kChannelMax);
    std::string contents;
    int r = 0, g = 0, b = 0;

    if (ReadFileToString(kFileKcal, &contents) &&
        std::sscanf(contents.c_str(), "%d %d %d", &r, &g, &b) == 3) {
        rgb = {r, g, b};
    } else {
        PLOG(ERROR) << "Cannot read " << kFileKcal;
    }

    _hidl_cb(rgb);
    return Void();
}

Return<bool> DisplayColorCalibration::setCalibration(const hidl_vec<int32_t>& rgb) {
    if (rgb.size() != 3) {
        LOG(ERROR) << "Unrecognized RGB data!";
        return false;
    }

    const int32_t channelFloor = ReadChannelFloor();
    if (!WriteStringToFile(StringPrintf("%d %d %d", std::clamp(rgb[0], channelFloor, kChannelMax),
                                        std::clamp(rgb[1], channelFloor, kChannelMax),
                                        std::clamp(rgb[2], channelFloor, kChannelMax)),
                           kFileKcal, true)) {
        PLOG(ERROR) << "Cannot write " << kFileKcal;
        return false;
    }

    if (!WriteStringToFile("1", kFileKcalCtrl, true)) {
        PLOG(ERROR) << "Cannot write " << kFileKcalCtrl;
        return false;
    }
    return true;
}

}  // namespace sysfs
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
