
BUILD_NUMBER := $(shell date +%Y%m%d)

PRODUCT_COPY_FILES += \
	device/softwinner/common/bin/fsck.exfat:system/bin/fsck.exfat \
	device/softwinner/common/bin/mkfs.exfat:system/bin/mkfs.exfat \
	device/softwinner/common/bin/mount.exfat:system/bin/mount.exfat \
	device/softwinner/common/bin/ntfs-3g:system/bin/ntfs-3g \
	device/softwinner/common/bin/mke2fs.ext4:system/bin/mke2fs.ext4 \
	device/softwinner/common/bin/ntfs-3g.probe:system/bin/ntfs-3g.probe \
	device/softwinner/common/bin/mkntfs:system/bin/mkntfs \
	device/softwinner/common/bin/busybox:system/bin/busybox	\

PRODUCT_COPY_FILES += \
	device/softwinner/common/hardware/audio/audio_policy.conf:system/etc/audio_policy.conf

# 3G Data Card Configuration Flie
PRODUCT_COPY_FILES += \
	device/softwinner/common/rild/ip-down:system/etc/ppp/ip-down \
	device/softwinner/common/rild/ip-up:system/etc/ppp/ip-up \
	device/softwinner/common/rild/call-pppd:system/etc/ppp/call-pppd \
	device/softwinner/common/rild/3g_dongle.cfg:system/etc/3g_dongle.cfg \
	device/softwinner/common/rild/usb_modeswitch:system/bin/usb_modeswitch \
	device/softwinner/common/rild/usb_modeswitch.sh:system/bin/usb_modeswitch.sh \
	device/softwinner/common/rild/apns-conf_sdk.xml:system/etc/apns-conf.xml \
	device/softwinner/common/rild/libsoftwinner-ril.so:system/lib/libsoftwinner-ril.so

# usb modeswitch File
PRODUCT_COPY_FILES += \
	$(call find-copy-subdir-files,*,device/softwinner/common/rild/usb_modeswitch.d,system/etc/usb_modeswitch.d)

# NFS Support File
PRODUCT_COPY_FILES += \
	device/softwinner/common/bin/nfsprobe:system/bin/nfsprobe
