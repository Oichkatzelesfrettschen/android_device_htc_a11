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
    device/htc/a11/keylayout/device-keypad.kl:$(TARGET_COPY_OUT_SYSTEM)/usr/keylayout/device-keypad.kl

PRODUCT_NAME := lineage_a11chl
PRODUCT_DEVICE := a11chl
PRODUCT_BRAND := htc
PRODUCT_MANUFACTURER := HTC
PRODUCT_MODEL := HTC Desire 510
PRODUCT_SHIPPING_API_LEVEL := 19
PRODUCT_TARGET_VNDK_VERSION := 29

# The a11chl product selects the Sprint CDMA/LTE blob family at build time.
# CM13's variant script selected the same family for 0PCV10000/0PCV20000.
PRODUCT_VENDOR_PROPERTIES += \
    ro.telephony.default_cdma_sub=1 \
    ro.telephony.default_network=8 \
    telephony.lteOnCdmaDevice=1 \
    ril.subscription.types=NV,RUIM \
    ro.ril.set.mtusize=1422
