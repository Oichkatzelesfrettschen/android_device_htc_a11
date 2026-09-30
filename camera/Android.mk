LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    CameraWrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    libhardware liblog

LOCAL_C_INCLUDES := \
    system/media/camera/include

LOCAL_CFLAGS := -Wall -Wextra -Werror

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liba11-camera-abi
LOCAL_SRC_FILES := camera_abi.cpp jpeg_abi.c
LOCAL_SHARED_LIBRARIES := libcamera_client libsensor libui libutils liblog libcrypto
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)
