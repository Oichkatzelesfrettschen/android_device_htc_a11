/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_TAG "vendor.lineage.livedisplay@2.0-service.a11"

#include "livedisplay/sysfs/AdaptiveBacklight.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

using ::android::base::ReadFileToString;
using ::android::base::WriteStringToFile;

namespace {
// htc_register_attrs() (mdss_htc_util.c) creates cabc_level_ctl on the
// lcd-backlight LED class device. The attribute holds a panel CABC mode: 0
// sends htc,cabc-off-cmds, 1 htc,cabc-ui-cmds and 2 htc,cabc-video-cmds.
// htc_set_cabc() sends the commands on the next frame commit and restarts from
// mode 1 on every panel power cycle, so a stored 0 is sent again after resume.
constexpr const char* kFileCabcLevelCtl = "/sys/class/leds/lcd-backlight/cabc_level_ctl";
constexpr const char* kCabcOff = "0";
constexpr const char* kCabcUi = "1";
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sysfs {

AdaptiveBacklight::AdaptiveBacklight() : file_(kFileCabcLevelCtl) {}

bool AdaptiveBacklight::isSupported() {
    return !access(file_, R_OK | W_OK);
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IAdaptiveBacklight follow.
Return<bool> AdaptiveBacklight::isEnabled() {
    std::string contents;
    if (!ReadFileToString(file_, &contents)) {
        PLOG(ERROR) << "Cannot read " << file_;
        return false;
    }

    // The attribute's show() method appends a NUL after the newline, so the
    // number is parsed from the front of the buffer.
    return std::strtol(contents.c_str(), nullptr, 10) > 0;
}

Return<bool> AdaptiveBacklight::setEnabled(bool enabled) {
    if (!WriteStringToFile(enabled ? kCabcUi : kCabcOff, file_, true)) {
        PLOG(ERROR) << "Cannot write " << file_;
        return false;
    }
    return true;
}

}  // namespace sysfs
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
