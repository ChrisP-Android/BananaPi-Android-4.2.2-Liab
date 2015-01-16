#!/bin/bash
function cdevice()
{	
	cd $DEVICE
}

function cout()
{
	cd $OUT	
}

function extract-bsp()
{
	LICHEE_DIR=$ANDROID_BUILD_TOP/../lichee
	LINUXOUT_DIR=$LICHEE_DIR/out/android/common
	LINUXOUT_MODULE_DIR=$LICHEE_DIR/out/android/common/lib/modules/*/*
	CURDIR=$PWD

	cd $DEVICE

	#extract kernel
	if [ -f kernel ]; then
		rm kernel
	fi
	cp $LINUXOUT_DIR/bImage kernel
	echo "$DEVICE/bImage copied!"

	#extract linux modules
	if [ -d modules ]; then
		rm -rf modules
	fi
	mkdir -p modules/modules
	cp -rf $LINUXOUT_MODULE_DIR modules/modules
	echo "$DEVICE/modules copied!"
	chmod 0755 modules/modules/*

# create modules.mk
(cat << EOF) > ./modules/modules.mk 
# modules.mk generate by extract-files.sh , do not edit it !!!!
PRODUCT_COPY_FILES += \\
	\$(call find-copy-subdir-files,*,\$(LOCAL_PATH)/modules,system/vendor/modules)

EOF

	cd $CURDIR
}

function make-all()
{
	# check lichee dir
	LICHEE_DIR=$ANDROID_BUILD_TOP/../lichee
	if [ ! -d $LICHEE_DIR ] ; then
		echo "$LICHEE_DIR not exists!"
		return
	fi

	extract-bsp
	m -j8
} 


function pack()
{
	T=$(gettop)
	export ANDROID_IMAGE_OUT=$OUT
	export PACKAGE=$T/../lichee/tools/pack

	sh $DEVICE/package.sh $1
}

function exdroid_diff()
{
	echo "please check v1, v2 in build/tools/exdroid_diff.sh (^C to break)"
	read
	repo forall -c '$ANDROID_BUILD_TOP/build/tools/exdroid_diff.sh'	
}

function exdroid_patch()
{
	echo "please confirm this is v1 (^C to break)"
	read
	repo forall -c '$ANDROID_BUILD_TOP/build/tools/exdroid_patch.sh'	
}

function get_uboot()
{
  pack $1
  echo "-------------------------------------"
  if [ ! -e $OUT/bootloader ];then
      mkdir $OUT/bootloader
  fi
  rm -rf $OUT/bootloader/*
  cp $PACKAGE/out/bootloader.fex $OUT/bootloader.fex
  cp -r $PACKAGE/out/boot-resource/* $OUT/bootloader/
  echo "cp $PACKAGE/out/bootloader.fex to $OUT/bootloader.fex"
  echo "cp -r $PACKAGE/out/bootloader/* $OUT/bootloader/"
  cp $PACKAGE/out/env.fex $OUT
  echo "cp $PACKAGE/out/env.fex $OUT/"
  cp $PACKAGE/out/boot0_nand.fex $OUT
  cp $PACKAGE/out/boot0_sdcard.fex $OUT
  cp $PACKAGE/out/u-boot.fex $OUT/uboot.fex
  echo "cp $PACKAGE/out/boot0_nand.fex $OUT/boot0_nand.fex"
  echo "cp $PACKAGE/out/boot0_sdcard.fex $OUT/boot0_sdcard.fex"
  echo "cp $PACKAGE/out/u-boot.fex $OUT/uboot.fex"
}

function make_ota_target_file()
{
    get_uboot
    echo "rm $OUT/obj/PACKAGING/target_files_intermediates/"
    rm -rf $OUT/obj/PACKAGING/target_files_intermediates/
    echo "---make target-files-package---"
    make target-files-package

}

function make_ota_package()
{
    get_uboot
    echo "rm $OUT/obj/PACKAGING/target_files_intermediates/"
    rm -rf $OUT/obj/PACKAGING/target_files_intermediates/
    echo "----make otapackage ----"
    make otapackage
}

function make_ota_package_inc()
{
    mv *.zip old_target_files.zip
    get_uboot
    echo "rm $OUT/obj/PACKAGING/target_files_intermediates/"
    rm -rf $OUT/obj/PACKAGING/target_files_intermediates/
    echo "----make otapackage_inc----"
    make otapackage_inc
}
