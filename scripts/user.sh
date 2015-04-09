#./user.sh
#startet from mkuserimg.sh
#insert user wishes like better busybox app etc...
cd ../user
cp -r bin/busybox ../android/out/target/product/sugar-bpi/system/bin
cp -r bin/busybox ../android/out/target/product/sugar-lemaker/system/bin
rm ../android/out/target/product/sugar-bpi/system/xbin/su
rm ../android/out/target/product/sugar-lemaker/system/xbin/su
cp -r xbin/* ../android/out/target/product/sugar-bpi/system/xbin
cp -r xbin/* ../android/out/target/product/sugar-lemaker/system/xbin
cp build-bpi.prop ../android/out/target/product/sugar-bpi/system/build.prop
cp build-lemaker.prop ../android/out/target/product/sugar-lemaker/system/build.prop
cp -r app/* ../android/out/target/product/sugar-bpi/system/app
cp -r app/* ../android/out/target/product/sugar-lemaker/system/app
cp -r preinstall/* ../android/out/target/product/sugar-bpi/system/preinstall
cp -r preinstall/* ../android/out/target/product/sugar-lemaker/system/preinstall
cp -r etc/* ../android/out/target/product/sugar-bpi/system/etc
cp -r etc/* ../android/out/target/product/sugar-lemaker/system/etc
cp -r lib/* ../android/out/target/product/sugar-bpi/system/lib
cp -r lib/* ../android/out/target/product/sugar-lemaker/system/lib
cp -r config/* ../lichee/tools/pack/chips/sun7i/configs/android

exit
