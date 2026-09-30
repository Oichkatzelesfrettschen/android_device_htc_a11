BOARD_VENDOR := htc

include device/htc/msm8226-common/BoardConfigCommon.mk

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a7
TARGET_CPU_VARIANT_RUNTIME := cortex-a7

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
# loads the shim beside each listed library, by resolved path, in every process
# that opens it. Executables that import the symbol directly keep their init
# LD_PRELOAD.
A11_HTCLOG_SHIM := /system/vendor/lib/liba11-legacy-radio.so
A11_HTCLOG_LIBS := \
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
    /system/vendor/lib/libqcci_legacy.so \
    /system/vendor/lib/libqdi.so \
    /system/vendor/lib/libqdp.so \
    /system/vendor/lib/libqmi_client_qmux.so \
    /system/vendor/lib/libqmi_csi.so \
    /system/vendor/lib/libqmi.so \
    /system/vendor/lib/libril-qc-qmi-1.so
TARGET_LD_SHIM_LIBS += $(foreach lib,$(A11_HTCLOG_LIBS),$(lib)|$(A11_HTCLOG_SHIM))
