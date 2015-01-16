adb devices
adb remount
::adb shell mount -o remount,rw /dev/block/system /system
adb push disp/disp.ko /system/vendor/modules/disp.ko
adb shell chmod 644 /system/vendor/modules/disp.ko
adb push lcd/lcd.ko /system/vendor/modules/lcd.ko
adb shell chmod 644 /system/vendor/modules/lcd.ko
adb push hdmi/hdmi.ko /system/vendor/modules/hdmi.ko
adb shell chmod 644 /system/vendor/modules/hdmi.ko
adb push hdmi/hdcp.ko /system/vendor/modules/hdcp.ko
adb shell chmod 644 /system/vendor/modules/hdcp.ko
adb shell sync
echo press any key to reboot
pause
adb shell reboot