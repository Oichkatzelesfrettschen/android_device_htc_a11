# Vendor HAL services built from device/htc/a11 sources.

# LiveDisplay: adaptive backlight on the panel CABC attribute and color
# calibration on the KCAL platform device (livedisplay/).
PRODUCT_PACKAGES += \
    vendor.lineage.livedisplay@2.0-service.a11

# Thermal: the AIDL thermal HAL of hardware/google/pixel reads the zones that
# thermal_info_config.json names. Its rc keeps vendor.thermal-hal disabled until
# vendor.thermal.link_ready is 1, which init.a11chl.thermal.rc sets at boot.
PRODUCT_PACKAGES += \
    android.hardware.thermal-service.pixel

PRODUCT_COPY_FILES += \
    device/htc/a11/thermal/thermal_info_config.json:$(TARGET_COPY_OUT_VENDOR)/etc/thermal_info_config.json \
    device/htc/a11/thermal/init.a11chl.thermal.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.a11chl.thermal.rc
