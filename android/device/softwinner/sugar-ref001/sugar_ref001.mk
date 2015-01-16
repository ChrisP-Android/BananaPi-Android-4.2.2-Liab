# wing fpga product config

$(call inherit-product, device/softwinner/wing-common/ProductCommon.mk)

DEVICE_PACKAGE_OVERLAYS := device/softwinner/sugar-ref001/overlay

PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/modules/modules/nand.ko:root/nand.ko \
	device/softwinner/sugar-ref001/modules/modules/sun7i-ir.ko:root/sun7i-ir.ko \
	#device/softwinner/sugar-ref001/modules/modules/disp.ko:root/disp.ko \
	#device/softwinner/sugar-ref001/modules/modules/lcd.ko:root/lcd.ko \
	#device/softwinner/sugar-ref001/modules/modules/hdmi.ko:root/hdmi.ko \

PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/kernel:kernel \
	device/softwinner/sugar-ref001/recovery.fstab:recovery.fstab \
	frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \

PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/ueventd.sun7i.rc:root/ueventd.sun7i.rc \
	device/softwinner/sugar-ref001/init.sun7i.rc:root/init.sun7i.rc \
	device/softwinner/sugar-ref001/init.sun7i.usb.rc:root/init.sun7i.usb.rc \
	device/softwinner/sugar-ref001/init.recovery.sun7i.rc:root/init.recovery.sun7i.rc \
	device/softwinner/sugar-ref001/needfix.rle:root/needfix.rle \
	device/softwinner/sugar-ref001/camera.cfg:system/etc/camera.cfg \
	device/softwinner/sugar-ref001/media_profiles.xml:system/etc/media_profiles.xml \
	frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml

#input device config
PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/sw-keyboard.kl:system/usr/keylayout/sw-keyboard.kl \
	device/softwinner/sugar-ref001/sun7i-ir.kl:system/usr/keylayout/sun7i-ir.kl \
	device/softwinner/sugar-ref001/tp.idc:system/usr/idc/tp.idc \
	device/softwinner/sugar-ref001/gsensor.cfg:system/usr/gsensor.cfg

PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/initlogo.rle:root/initlogo.rle

PRODUCT_COPY_FILES += \
	device/softwinner/sugar-ref001/vold.fstab:system/etc/vold.fstab
	
PRODUCT_PACKAGES += \
	TvdLauncher \
#	TSCalibration2

# wifi & bt config file
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
#    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
#    system/bluetooth/data/main.nonsmartphone.conf:system/etc/bluetooth/main.conf

# rtl8723as bt fw and config
#PRODUCT_COPY_FILES += \
#	device/softwinner/wing-common/hardware/realtek/bluetooth/rtl8723as/rlt8723a_chip_b_cut_bt40_fw.bin:system/etc/rlt8723a_chip_b_cut_bt40_fw.bin \
#	device/softwinner/wing-common/hardware/realtek/bluetooth/rtl8723as/rtk8723_bt_config:system/etc/rtk8723_bt_config

# bcm40181 sdio wifi fw and nvram
#PRODUCT_COPY_FILES += \
#	hardware/broadcom/wlan/firmware/bcm40181/fw_bcm40181a2_p2p.bin:system/vendor/modules/fw_bcm40181a2_p2p.bin \
#	hardware/broadcom/wlan/firmware/bcm40181/fw_bcm40181a2_apsta.bin:system/vendor/modules/fw_bcm40181a2_apsta.bin \
#	hardware/broadcom/wlan/firmware/bcm40181/fw_bcm40181a2.bin:system/vendor/modules/fw_bcm40181a2.bin \
#	hardware/broadcom/wlan/firmware/bcm40181/bcm40181_nvram.txt:system/vendor/modules/bcm40181_nvram.txt

# bcm40183 sdio wifi fw and nvram
#PRODUCT_COPY_FILES += \
#	hardware/broadcom/wlan/firmware/bcm40183/fw_bcm40183b2_p2p.bin:system/vendor/modules/fw_bcm40183b2_p2p.bin \
#	hardware/broadcom/wlan/firmware/bcm40183/fw_bcm40183b2_apsta.bin:system/vendor/modules/fw_bcm40183b2_apsta.bin \
#	hardware/broadcom/wlan/firmware/bcm40183/fw_bcm40183b2.bin:system/vendor/modules/fw_bcm40183b2.bin \
#	hardware/broadcom/wlan/firmware/bcm40183/bcm40183_nvram.txt:system/vendor/modules/bcm40183_nvram.txt

# ap6210 sdio wifi fw and nvram
PRODUCT_COPY_FILES += \
	hardware/broadcom/wlan/firmware/ap6210/fw_bcm40181a2.bin:system/vendor/modules/fw_bcm40181a2.bin \
	hardware/broadcom/wlan/firmware/ap6210/fw_bcm40181a2_apsta.bin:system/vendor/modules/fw_bcm40181a2_apsta.bin \
	hardware/broadcom/wlan/firmware/ap6210/fw_bcm40181a2_p2p.bin:system/vendor/modules/fw_bcm40181a2_p2p.bin \
	hardware/broadcom/wlan/firmware/ap6210/nvram_ap6210.txt:system/vendor/modules/nvram_ap6210.txt \
	hardware/broadcom/wlan/firmware/ap6210/bcm20710a1.hcd:system/vendor/modules/bcm20710a1.hcd
	
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.heapsize=256m \
	dalvik.vm.heapstartsize=8m \
	dalvik.vm.heapgrowthlimit=96m \
	dalvik.vm.heaptargetutilization=0.75 \
	dalvik.vm.heapminfree=2m \
	dalvik.vm.heapmaxfree=8m \
	persist.sys.usb.config=mass_storage,adb \
	ro.property.tabletUI=true \
	ro.udisk.lable=sugar \
	ro.product.firmware=v2.0 \
	ro.sw.defaultlauncherpackage=com.softwinner.launcher \
	ro.sw.defaultlauncherclass=com.softwinner.launcher.Launcher \
	audio.output.active=AUDIO_CODEC \
	audio.input.active=AUDIO_CODEC \
	ro.audio.multi.output=true \
	ro.sw.directlypoweroff=true \
	ro.softmouse.left.code=6 \
    ro.softmouse.right.code=14 \
    ro.softmouse.top.code=67 \
    ro.softmouse.bottom.code=10 \
    ro.softmouse.leftbtn.code=2 \
    ro.softmouse.midbtn.code=-1 \
    ro.softmouse.rightbtn.code=-1 \
    ro.sw.shortpressleadshut=false \
    ro.sw.videotrimming=1 \
    persist.sys.device_name = MiniMax 

$(call inherit-product-if-exists, device/softwinner/sugar-ref001/modules/modules.mk)

PRODUCT_CHARACTERISTICS := tablet

# Overrides
PRODUCT_BRAND  := softwinners
PRODUCT_NAME   := sugar_ref001
PRODUCT_DEVICE := sugar-ref001
PRODUCT_MODEL  := SoftwinerEvb

