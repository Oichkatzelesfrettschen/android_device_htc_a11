$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, build/make/target/product/go_defaults.mk)
$(call inherit-product, vendor/lineage/config/common_mini_go_phone.mk)
$(call inherit-product, device/htc/msm8226-common/msm8226.mk)

# The 512 MiB product uses one in-process Tethering APEX.
PRODUCT_PACKAGES += com.android.tethering.a11chl

PRODUCT_COPY_FILES += \
    device/htc/a11/rootdir/etc/fstab.a11chl:$(TARGET_COPY_OUT_RAMDISK)/fstab.qcom \
    device/htc/a11/rootdir/etc/fstab.a11chl:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom \
    device/htc/a11/init/init.a11chl.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.a11chl.rc \
    device/htc/a11/keylayout/device-keypad.kl:$(TARGET_COPY_OUT_SYSTEM)/usr/keylayout/device-keypad.kl \
    device/htc/a11/idc/himax-touchscreen.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/himax-touchscreen.idc

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
