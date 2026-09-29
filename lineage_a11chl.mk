$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, build/make/target/product/go_defaults.mk)
$(call inherit-product, vendor/lineage/config/common_mini_go_phone.mk)
$(call inherit-product, device/htc/msm8226-common/msm8226.mk)

# The 512 MiB product uses one in-process Tethering APEX.
PRODUCT_PACKAGES += com.android.tethering.a11chl

# The multihal sensors service (android.hardware.sensors@1.0-service.htc8226)
# reads /vendor/etc/sensors/_hals.conf and dlopens each sub-HAL listed there.
# sensors.a11 is the wrapper that loads the HTC vendor sub-module
# (sensors.vendor.msm8226.so, installed by a11chl-vendor.mk) and remaps its
# type=21 gesture entry off the SENSOR_TYPE_HEART_RATE collision that otherwise
# SIGSEGVs SensorService. Without both the wrapper and the conf, sensorservice
# reports "No Sensors on the device".
PRODUCT_PACKAGES += sensors.a11
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
    frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml

# The built-in Pronto WLAN driver (prima 3.2.3.172) validates the configuration
# download against its own cfg table, so the configuration files come from the
# kernel tree that builds the driver.
PRODUCT_COPY_FILES += \
    kernel/htc/a11/drivers/staging/prima/firmware_bin/WCNSS_cfg.dat:$(TARGET_COPY_OUT_SYSTEM)/etc/firmware/wlan/prima/WCNSS_cfg.dat \
    kernel/htc/a11/drivers/staging/prima/firmware_bin/WCNSS_qcom_cfg.ini:$(TARGET_COPY_OUT_SYSTEM)/etc/firmware/wlan/prima/WCNSS_qcom_cfg.ini

PRODUCT_NAME := lineage_a11chl
PRODUCT_DEVICE := a11chl
PRODUCT_BRAND := htc
PRODUCT_MANUFACTURER := HTC
PRODUCT_MODEL := HTC Desire 510
PRODUCT_SHIPPING_API_LEVEL := 19
PRODUCT_TARGET_VNDK_VERSION := 29

# The a11chl product selects the Sprint CDMA/LTE blob family at build time.
# CM13's variant script selected the same family for 0PCV10000/0PCV20000.
# PRODUCT_PROPERTY_OVERRIDES is the Android 11 variable that writes vendor/build.prop.
# Network mode 11 is LTE only: the 0PCV1 front end carries LTE B25/B26/B41 and
# CDMA, has no GSM or WCDMA path, and the Sprint CDMA network is decommissioned.
# HTC rmt_storage serves the modem EFS only when ro.baseband.arch names an MSM
# target; qcril brings up data calls through netmgrd.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.baseband.arch=msm \
    ro.use_data_netmgrd=true \
    persist.data.netmgrd.qos.enable=true \
    ro.telephony.default_cdma_sub=1 \
    ro.telephony.default_network=11 \
    telephony.lteOnCdmaDevice=1 \
    ril.subscription.types=NV,RUIM \
    ro.ril.set.mtusize=1422
