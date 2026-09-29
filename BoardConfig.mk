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
BOARD_KERNEL_CMDLINE := androidboot.hardware=qcom androidboot.bootdevice=msm_sdcc.1 androidboot.selinux=permissive androidboot.init_fatal_panic=true
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
