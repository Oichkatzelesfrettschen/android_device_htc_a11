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

# The service, libpixelstats and pixelatoms-cpp sit in the hardware/google/pixel
# Soong namespace and pixel-power-ext-V1-ndk in hardware/google/interfaces; Make
# resolves the package only when both namespaces are exported to the product.
PRODUCT_SOONG_NAMESPACES += \
    hardware/google/interfaces \
    hardware/google/pixel

PRODUCT_COPY_FILES += \
    device/htc/a11/thermal/thermal_info_config.json:$(TARGET_COPY_OUT_VENDOR)/etc/thermal_info_config.json \
    device/htc/a11/thermal/init.a11chl.thermal.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.a11chl.thermal.rc

# Vibrator: IVibrator over the qpnp-vibrator timed_output node, with amplitude
# control on voltage_level (vibrator/). device/htc/msm8226-common installs no
# vibrator service.
PRODUCT_PACKAGES += \
    android.hardware.vibrator-service.a11
