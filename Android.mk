LOCAL_PATH := $(call my-dir)

# camera.$(TARGET_BOARD_PLATFORM) and the other modules below are defined only
# for this device: a tree that also holds device/htc/m8-common defines
# camera.msm8974, and Kati rejects a repeated module name.
ifeq ($(TARGET_DEVICE),a11chl)
include $(call all-subdir-makefiles,$(LOCAL_PATH))
endif
