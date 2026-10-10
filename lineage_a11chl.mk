$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, build/make/target/product/go_defaults.mk)
$(call inherit-product, vendor/lineage/config/common_mini_go_phone.mk)
$(call inherit-product, device/htc/msm8226-common/msm8226.mk)

# The multihal sensors service (android.hardware.sensors@1.0-service.htc8226)
# reads /vendor/etc/sensors/_hals.conf and dlopens each sub-HAL listed there.
# sensors.a11 is the wrapper that loads the HTC vendor sub-module
# (sensors.vendor.msm8226.so, installed by a11chl-vendor.mk) and remaps its
# type=21 gesture entry off the SENSOR_TYPE_HEART_RATE collision that otherwise
# SIGSEGVs SensorService. Without both the wrapper and the conf, sensorservice
# reports "No Sensors on the device".
PRODUCT_PACKAGES += sensors.a11
PRODUCT_PACKAGES += \
    camera.msm8226 \
    liba11-camera-abi

PRODUCT_COPY_FILES += \
    device/htc/a11/configs/media_profiles.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml
PRODUCT_COPY_FILES += \
    device/htc/a11/sensors/_hals.conf:$(TARGET_COPY_OUT_VENDOR)/etc/sensors/_hals.conf

PRODUCT_COPY_FILES += \
    device/htc/a11/rootdir/etc/fstab.a11chl:$(TARGET_COPY_OUT_RAMDISK)/fstab.qcom \
    device/htc/a11/rootdir/etc/fstab.a11chl:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom \
    device/htc/a11/init/init.a11chl.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.a11chl.rc \
    device/htc/a11/keylayout/device-keypad.kl:$(TARGET_COPY_OUT_SYSTEM)/usr/keylayout/device-keypad.kl \
    device/htc/a11/idc/himax-touchscreen.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/himax-touchscreen.idc

# The msm8226 audio HAL opens /vendor/etc/mixer_paths.xml and
# /vendor/etc/audio_platform_info.xml by absolute path (msm8974/platform.c
# MIXER_XML_PATH, platform_info.c PLATFORM_INFO_XML_PATH); the HIDL
# audio policy service reads /vendor/etc/audio_policy_configuration.xml and
# resolves its xi:include hrefs in the same directory; the effects factory
# reads /vendor/etc/audio_effects.xml. The sound card is msm8226-tapan-snd-card.
PRODUCT_COPY_FILES += \
    device/htc/a11/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    device/htc/a11/audio/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml \
    device/htc/a11/audio/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml \
    device/htc/a11/audio/audio_platform_info.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info.xml \
    frameworks/av/services/audiopolicy/config/bluetooth_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml

# A2DP audio: the Android 11 Bluetooth stack serves the source stream through
# IBluetoothAudioProvidersFactory 2.1, which the audio HAL service registers
# when the passthrough implementation is installed; audio.bluetooth.default is
# the audio policy's "bluetooth" module on that session. The legacy
# audio.a2dp.default module waits on /data/misc/bluedroid/.a2dp_ctrl, a
# socket this stack never creates.
PRODUCT_PACKAGES += \
    android.hardware.bluetooth.audio@2.1-impl \
    audio.bluetooth.default

# The built-in Pronto WLAN driver (prima 3.2.3.172) validates the configuration
# download against its own cfg table, so the configuration files come from the
# kernel tree that builds the driver.
PRODUCT_COPY_FILES += \
    kernel/htc/a11/drivers/staging/prima/firmware_bin/WCNSS_cfg.dat:$(TARGET_COPY_OUT_SYSTEM)/etc/firmware/wlan/prima/WCNSS_cfg.dat \
    kernel/htc/a11/drivers/staging/prima/firmware_bin/WCNSS_qcom_cfg.ini:$(TARGET_COPY_OUT_SYSTEM)/etc/firmware/wlan/prima/WCNSS_qcom_cfg.ini

# microG Services 0.3.17.252432 and Companion 0.3.17.40226 (github.com/microg/GmsCore
# release v0.3.17.252432) install byte-for-byte from prebuilt/microg/Android.mk
# as privileged apps. The release signature (SHA-256 9bd06727...4165) is the
# certificate PackageManagerService.isMicrogSigned matches before it spoofs the
# Google signature, and a raw copy keeps the APK v2 signature block. Enforce
# mode requires the privapp allowlist entries for their signature|privileged
# permissions.
PRODUCT_PACKAGES += \
    GmsCore \
    FakeStore \
    privapp-permissions-microg.xml \
    default-permissions-microg.xml

# Vendor HAL services built from this tree: LiveDisplay, thermal and vibrator.
$(call inherit-product, device/htc/a11/hal-services.mk)

# apexd mounts a decompressed .capex from /data through a dm-verity device,
# and the 3.4 kernel builds no DM_VERITY, so every compressed APEX fails to
# activate and bpfloader, finding no netbpfload in com.android.tethering,
# reboots the device. Uncompressed APEXes mount from /system without verity.
# updatable_apex.mk, inherited through full_base_telephony.mk above, sets
# the variable true first, and the first inherited assignment of a
# single-value product variable wins over msm8226.mk's false; this file's
# own assignment wins over both.
PRODUCT_COMPRESSED_APEX := false

PRODUCT_NAME := lineage_a11chl
PRODUCT_DEVICE := a11chl
PRODUCT_BRAND := htc
PRODUCT_MANUFACTURER := HTC
PRODUCT_MODEL := HTC Desire 510
PRODUCT_SHIPPING_API_LEVEL := 19

# The 480x854 panel reports 213 dpi; aapt2 keeps only hdpi bitmaps (the
# closest bucket at or above it) in source-built APKs and framework-res, and
# the framework scales them down at load.
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# PhoneGlobals initializes telephony, and TeleService registers the phone
# and isub services, only when PackageManager reports FEATURE_TELEPHONY; the
# CDMA/LTE modem with a UICC slot declares both the CDMA and GSM feature sets.
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.cdma.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.cdma.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.gsm.xml

# The a11chl product selects the Sprint CDMA/LTE blob family at build time.
# CM13's variant script selected the same family for 0PCV10000/0PCV20000.
# PRODUCT_PROPERTY_OVERRIDES is the Android 11 variable that writes vendor/build.prop.
# Network mode 9 is LTE/GSM/WCDMA, matching the modem's NV_PREF_MODE 0x25 for
# a SIM on a GSM-family LTE network. The 0PCV1 front end carries LTE
# B25/B26/B41 and CDMA with no GSM or WCDMA path, so the modem scans LTE only;
# B25 is a superset of B2, and the Sprint CDMA network is decommissioned.
# HTC rmt_storage serves the modem EFS only when ro.baseband.arch names an MSM
# target; qcril brings up data calls through netmgrd. The GNSS engine runs in
# the modem, so persist.vendor.radio.start=1 starts rmt_storage, qmuxd, and
# netmgrd on every boot; setting it to 0 keeps the modem down.
PRODUCT_PROPERTY_OVERRIDES += \
    persist.vendor.radio.start=1 \
    ro.baseband.arch=msm \
    persist.data.netmgrd.qos.enable=true \
    ro.telephony.default_cdma_sub=1 \
    ro.telephony.default_network=9 \
    telephony.lteOnCdmaDevice=1 \
    ril.subscription.types=NV,RUIM \
    ro.ril.set.mtusize=1422

# Dual-mic noise suppression. The HAL (msm8974/platform.c) reads
# ro.vendor.audio.sdk.fluencetype once at init; "fluence" selects the
# ADC1 + ADC3 endfire dual-mic paths (voice-dmic-ef, voice-rec-dmic-ef-fluence,
# dmic-endfire) and ACDB device 41. voicecall and voicerec switch those paths
# on; audiorec and speaker stay off because their devices need ACDB ids the
# stock databases lack.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.audio.sdk.fluencetype=fluence \
    persist.vendor.audio.fluence.voicecall=true \
    persist.vendor.audio.fluence.voicerec=true \
    persist.vendor.audio.fluence.audiorec=false \
    persist.vendor.audio.fluence.speaker=false

# The HTC bootloader passes the Bluetooth address to the htc_bdaddress kernel
# module, which exports it as a 17-character colon-separated string; the
# Bluetooth HAL reads its address from the file ro.bt.bdaddr_path names and
# aborts with "No Bluetooth Address!" without one. libbt-vendor selects the
# WCNSS SMD transport (/dev/smd3 commands, /dev/smd2 ACL) from
# ro.qualcomm.bt.hci_transport.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.bt.bdaddr_path=/sys/module/htc_bdaddress/parameters/bdaddress \
    ro.qualcomm.bt.hci_transport=smd

# post_process_props.py writes persist.sys.usb.config=adb only into the
# build.prop whose ro.adb.secure is 0 (system) and none into vendor,
# system_ext and product; init loads product last, so a debuggable build
# names adb there as well and enumerates adb without a stored setting.
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_PRODUCT_PROPERTIES += persist.sys.usb.config=adb
endif

# build/make lists ro.product.first_api_level as a vendor property, and
# without split overrides it lands in system/build.prop, whose blacklist
# drops it. ueventd parses /vendor/ueventd.rc, and with it the firmware
# directories PIL loads mba, modem and adsp from, only when the launch API
# level is 31 or lower, so product/etc/build.prop carries it.
PRODUCT_PRODUCT_PROPERTIES += ro.product.first_api_level=$(PRODUCT_SHIPPING_API_LEVEL)

# Resource overlays
DEVICE_PACKAGE_OVERLAYS += device/htc/a11/overlay
