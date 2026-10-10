/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_TAG "android.hardware.vibrator-service.a11"

#include "Vibrator.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include <algorithm>
#include <cmath>

using ::android::base::StringPrintf;
using ::android::base::WriteStringToFile;

namespace {
// The timed_output class device of qpnp-vibrator.c. enable_store() takes a
// duration in milliseconds, clamps it to qcom,qpnp-vib-timeout-ms, restarts
// the hrtimer and drives the motor at the programmed voltage; 0 stops it.
constexpr const char* kFileEnable = "/sys/class/timed_output/vibrator/enable";
// voltage_level_store() takes millivolts and keeps voltage_input / 100 as the
// 5-bit VTG_CTL level, rejecting anything outside QPNP_VIB_MIN_LEVEL (12) to
// QPNP_VIB_MAX_LEVEL (31) with EINVAL. The level applies at the next
// qpnp_vib_set(on).
constexpr const char* kFileVoltageLevel = "/sys/class/timed_output/vibrator/voltage_level";
constexpr int kMinLevel = 12;
constexpr int kMaxLevel = 31;
constexpr int kMvPerLevel = 100;
// Default of qcom,qpnp-vib-timeout-ms in the device tree and in the driver.
constexpr int kMaxOnMs = 15000;

ndk::ScopedAStatus unsupported() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

bool writeNode(const char* path, const std::string& value) {
    if (!WriteStringToFile(value, path, true)) {
        PLOG(ERROR) << "Cannot write " << value << " to " << path;
        return false;
    }
    return true;
}

bool writeLevel(int level) {
    return writeNode(kFileVoltageLevel, StringPrintf("%d", level * kMvPerLevel));
}
}  // anonymous namespace

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

Vibrator::Vibrator() : mActiveUntil(std::chrono::steady_clock::now()) {
    // The kernel keeps the last programmed voltage across a service restart;
    // on() without a setAmplitude() runs at full drive.
    writeLevel(kMaxLevel);
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    *_aidl_return = IVibrator::CAP_AMPLITUDE_CONTROL;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    std::lock_guard<std::mutex> lock(mLock);
    mActiveUntil = std::chrono::steady_clock::now();
    if (!writeNode(kFileEnable, "0")) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }
    return ndk::ScopedAStatus::ok();
}

// The service reports no CAP_ON_CALLBACK, and the interface requires such a
// service to reject an on() that carries a callback.
ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    if (callback != nullptr) {
        return unsupported();
    }
    if (timeoutMs <= 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    std::lock_guard<std::mutex> lock(mLock);
    mActiveUntil = std::chrono::steady_clock::now() +
                   std::chrono::milliseconds(std::min(timeoutMs, kMaxOnMs));
    if (!writeNode(kFileEnable, StringPrintf("%d", timeoutMs))) {
        mActiveUntil = std::chrono::steady_clock::now();
        return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect /*effect*/, EffectStrength /*strength*/,
                                     const std::shared_ptr<IVibratorCallback>& /*callback*/,
                                     int32_t* /*_aidl_return*/) {
    return unsupported();
}

// The empty list is the complete answer: perform() rejects every effect.
ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    _aidl_return->clear();
    return ndk::ScopedAStatus::ok();
}

// The amplitude spans the driver's 20 voltage levels. The framework changes the
// amplitude right after on() for the first segment of a waveform, and the
// driver reads the level only when the motor starts, so a vibration that is
// still running restarts for its remaining time at the new level.
ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    if (!(amplitude > 0.0f) || amplitude > 1.0f) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    const int level =
            kMinLevel + static_cast<int>(std::lround(amplitude * (kMaxLevel - kMinLevel)));

    std::lock_guard<std::mutex> lock(mLock);
    if (!writeLevel(level)) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }

    const long long remainingMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    mActiveUntil - std::chrono::steady_clock::now())
                    .count();
    if (remainingMs > 0 && !writeNode(kFileEnable, StringPrintf("%lld", remainingMs))) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool /*enabled*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive /*primitive*/,
                                                  int32_t* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& /*composite*/,
                                     const std::shared_ptr<IVibratorCallback>& /*callback*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t /*id*/, Effect /*effect*/,
                                            EffectStrength /*strength*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t /*id*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getQFactor(float* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& /*composite*/,
                                         const std::shared_ptr<IVibratorCallback>& /*callback*/) {
    return unsupported();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
