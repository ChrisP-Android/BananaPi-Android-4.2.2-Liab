#./user.sh
#startet from mkuserimg.sh
#insert user wishes like commands, apps etc...
cd ../user
cp -r bin/busybox ../android/out/target/product/sugar-bpi/system/bin
cp -r bin/busybox ../android/out/target/product/sugar-lemaker/system/bin
rm ../android/out/target/product/sugar-bpi/system/xbin/su
rm ../android/out/target/product/sugar-lemaker/system/xbin/su
cp -r xbin/* ../android/out/target/product/sugar-bpi/system/xbin
cp -r xbin/* ../android/out/target/product/sugar-lemaker/system/xbin
cp build-bpi.prop ../android/out/target/product/sugar-bpi/system/build.prop
#touch ../android/out/target/product/sugar-bpi/system/audio_hdmi 
cp build-lemaker.prop ../android/out/target/product/sugar-lemaker/system/build.prop
#touch ../android/out/target/product/sugar-lemaker/system/audio_hdmi 
cp -r app/* ../android/out/target/product/sugar-bpi/system/app
cp -r app/* ../android/out/target/product/sugar-lemaker/system/app
cp -r etc/* ../android/out/target/product/sugar-bpi/system/etc
cp -r etc/* ../android/out/target/product/sugar-lemaker/system/etc
cp -r lib/* ../android/out/target/product/sugar-bpi/system/lib
cp -r lib/* ../android/out/target/product/sugar-lemaker/system/lib
exit
