# The microG APKs install as raw ETC-class files. android_app_import and the
# make APPS class zipalign a presigned APK, which drops its v2 signature block
# while the v1 signature file still declares X-Android-APK-Signed: 2;
# StrictJarVerifier then rejects the package as stripped.
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),a11chl)

# The APKs are not in git: fetch.sh stages them from this repository's
# microg release asset and verifies them against microg.sha256.
ifneq ($(words $(wildcard $(LOCAL_PATH)/*.apk)),2)
$(error microG APKs missing from $(LOCAL_PATH); run sh $(LOCAL_PATH)/fetch.sh)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := GmsCore
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com.google.android.gms-252432034.apk
LOCAL_MODULE_STEM := GmsCore.apk
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $(TARGET_OUT)/priv-app/GmsCore
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := FakeStore
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com.android.vending-84022634.apk
LOCAL_MODULE_STEM := FakeStore.apk
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $(TARGET_OUT)/priv-app/FakeStore
include $(BUILD_PREBUILT)

endif
