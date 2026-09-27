# Sensors HAL wrapper for a11chl; see sensors_wrapper.c for the fault this
# fixes. Builds as sensors.msm8226.so, the name hw_get_module("sensors")
# resolves for ro.board.platform=msm8226; the real vendor blob installs
# separately as sensors.vendor.msm8226.so (manifests/
# cm13-a11chl-proprietary-files.txt, matching camera.vendor.msm8226.so's
# same convention).
ifeq ($(TARGET_DEVICE),a11)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := sensors_wrapper.c
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_SHARED_LIBRARIES := libhardware liblog
include $(BUILD_SHARED_LIBRARY)
endif
