BOARD_VENDOR := htc

include device/htc/msm8226-common/BoardConfigCommon.mk

DEVICE_MANIFEST_FILE += device/htc/a11/bluetooth_audio_manifest.xml

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a7
TARGET_CPU_VARIANT_RUNTIME := cortex-a7

# The HTC camera load group contains text relocations. The non-Treble
# passthrough provider loads HAL1 inside cameraserver, rather than mediaserver.
TARGET_PROCESS_SDK_VERSION_OVERRIDE += \
    /system/bin/cameraserver=22 \
    /system/vendor/bin/mm-qcamera-daemon=22

# dex_preopt_config.mk and dexpreopt.go omit compressed OAT debug metadata
# when false, reducing installed boot and app artifacts at the cost of native
# symbolization detail for compiled Java code on the low-RAM product.
WITH_DEXPREOPT_DEBUG_INFO := false

TARGET_BOOTLOADER_BOARD_NAME := MSM8226
TARGET_NO_BOOTLOADER := true
TARGET_OTA_ASSERT_DEVICE := a11chl

TARGET_KERNEL_SOURCE := kernel/htc/a11
TARGET_KERNEL_CONFIG := cyanogenmod_a11ul_defconfig
BOARD_KERNEL_IMAGE_NAME := zImage
# HBOOT 3.19 appends about 840 bytes of its own arguments and refuses to boot
# when the result exceeds 1024 bytes ("bootargs out of size"). This line plus
# the build's " buildvariant=<variant>" must stay within 170 bytes; the
# bring-up job checks the packed boot.img cmdline against that budget.
BOARD_KERNEL_CMDLINE := androidboot.hardware=qcom androidboot.bootdevice=msm_sdcc.1 androidboot.init_fatal_panic=true
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_SEPARATED_DT := true
# HBOOT 3.19 selects a DTB by htc,project-id and hw-id; the kernel DTS
# describes only project 325, so the table ships as the booting image.
BOARD_KERNEL_PREBUILT_DT := device/htc/a11/prebuilt/dt.img
BOARD_MKBOOTIMG_ARGS := --kernel_offset 0x00008000 --ramdisk_offset 0x02008000 --tags_offset 0x01e00000

BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_CACHEIMAGE_PARTITION_SIZE := 113246208
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1207959552
BOARD_FLASH_BLOCK_SIZE := 512
TARGET_RECOVERY_FSTAB := device/htc/a11/rootdir/etc/fstab.a11chl

TARGET_SCREEN_HEIGHT := 854
TARGET_SCREEN_WIDTH := 480

# HTC vendor libraries import __htclog_init_mask from HTC's liblog, which the
# Android 11 liblog does not define; liba11-legacy-radio.so does. The linker
# loads the shim beside each listed library or executable, matched by resolved
# path, in every process that loads it; an executable's entry applies when the
# linker starts it. hci_qcomm_init runs from the hciattach script, where the
# service's LD_PRELOAD does not reach it.
A11_HTCLOG_SHIM := /system/vendor/lib/liba11-legacy-radio.so
A11_HTCLOG_LIBS := \
    /system/vendor/bin/hci_qcomm_init \
    /system/vendor/lib/hw/sensors.vendor.msm8226.so \
    /system/vendor/lib/libacdbloader.so \
    /system/vendor/lib/libacdbrtac.so \
    /system/vendor/lib/libadiertac.so \
    /system/vendor/lib/libaudcal.so \
    /system/vendor/lib/libbt-vendor.so \
    /system/vendor/lib/libdiag.so \
    /system/vendor/lib/libdsi_netctrl.so \
    /system/vendor/lib/libdsutils.so \
    /system/vendor/lib/libnetmgr.so \
    /system/vendor/lib/libOmxAmrwbplusDec.so \
    /system/vendor/lib/libOmxWmaDec.so \
    /system/vendor/lib/libqcci_legacy.so \
    /system/vendor/lib/libqdi.so \
    /system/vendor/lib/libqdp.so \
    /system/vendor/lib/libqmi_client_qmux.so \
    /system/vendor/lib/libqmi_csi.so \
    /system/vendor/lib/libqmi.so \
    /system/vendor/lib/libril-qc-qmi-1.so
TARGET_LD_SHIM_LIBS += $(foreach lib,$(A11_HTCLOG_LIBS),$(lib)|$(A11_HTCLOG_SHIM))

# Unique legacy imports bind to the camera ABI adapter without interposing
# Android 11's SensorManager or GraphicBuffer implementations.
TARGET_LD_SHIM_LIBS += \
    /system/vendor/lib/hw/camera.vendor.msm8226.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/libBeautyChat.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/libcamera_af.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/libcameraface.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/libmmqjpeg_codec.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/libposteffect.so|/system/lib/liba11-camera-abi.so \
    /system/vendor/lib/hw/camera.vendor.msm8226.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmm-qcamera.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmjpeg_interface.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_hdr.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_hdr_16_9.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_video.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_videoHFR.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_video_16_9.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_video_60fps.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_zsl.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k5e2_twolane_zsl_16_9.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k6a1gx_hdr.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k6a1gx_video.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec_s5k6a1gx_zsl.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_awb_s5k5e2_twolane.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_awb_s5k6a1gx.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libBeautyChat.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libawb_calibration.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libcamera_aec.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libcamera_af.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libcamera_awb.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libcameraface.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libcamerapp.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k5e2_twolane_default_video.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k5e2_twolane_hdr.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k5e2_twolane_hfr_common.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k5e2_twolane_preview.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k5e2_twolane_video_16_9.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k6a1gx_common.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k6a1gx_default_video.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k6a1gx_hdr.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libchromatix_s5k6a1gx_preview.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libjpegdhw.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libjpegehw.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_c2d_module.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_cpp_module.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_iface_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_imglib_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_isp_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_pproc_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_sensor_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_stats_algorithm.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_stats_modules.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera2_vpe_module.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_imglib.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_interface.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_ofilm_oty5f03_eeprom.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_s5k6a1gx.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_sunny_p12v01m_eeprom.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_sunny_p5v23c_eeprom.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_sunny_q8v18a_eeprom.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_truly_cm7700_eeprom.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmcamera_tuning.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmjpeg.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libmmqjpeg_codec.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/liboemcamera.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libposteffect.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libqomx_core.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libqomx_jpegdec.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libqomx_jpegenc.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_aec.so|/system/vendor/lib/liba11-legacy-radio.so \
    /system/vendor/lib/libtuning_af.so|/system/vendor/lib/liba11-legacy-radio.so
TARGET_LD_SHIM_LIBS += \
    /system/vendor/lib/libthermalclient.so|$(A11_HTCLOG_SHIM)

# The linker reads its shim list and per-process SDK overrides from the
# bionic_linker soong config namespace (bionic/linker/Android.bp). The list
# separator is ':' and each override is a space-separated path=sdk pair.
$(call soong_config_set,bionic_linker,ld_shim_libs,$(subst $(space),:,$(strip $(TARGET_LD_SHIM_LIBS))))
$(call soong_config_set,bionic_linker,process_sdk_version_overrides,$(strip $(TARGET_PROCESS_SDK_VERSION_OVERRIDE)))

# The a11chl modem loads a generation 2 XTRA file's 168-hour window one
# 1024-week GPS era early; libloc_eng counts that window as current.
TARGET_XTRA_ACCEPT_WEEK_ERA_ALIAS := true
