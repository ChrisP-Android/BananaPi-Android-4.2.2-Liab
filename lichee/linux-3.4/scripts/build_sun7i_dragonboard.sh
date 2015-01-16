#!/bin/bash
#
# scripts/build_sun7i.h
#
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

set -e

cpu_cores=`cat /proc/cpuinfo | grep "processor" | wc -l`
if [ ${cpu_cores} -le 8 ] ; then
    jobs=${cpu_cores}
else
    jobs=`expr ${cpu_cores} / 2`
fi

# Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump

KERNEL_VERSION="3.4"
LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}

update_kernel_ver()
{
    if [ -r include/generated/utsrelease.h ] ; then
        KERNEL_VERSION=`cat include/generated/utsrelease.h | awk -F\" '{print $2}'`
    fi
    LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
}

build_standby()
{
    echo "build standby"

    # If .config is newer than include/config/auto.conf, someone tinkered
    # with it and forgot to run make oldconfig.
    # if auto.conf.cmd is missing then we are probably in a cleaned tree so
    # we execute the config step to be sure to catch updated Kconfig files
    if [ .config -nt include/config/auto.conf -o \
        ! -f include/config/auto.conf.cmd ] ; then
        echo "Generating autoconf.h for standby"
        make -f Makefile ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} \
            silentoldconfig
    fi
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} KDIR=${LICHEE_KDIR} \
        -C ${LICHEE_KDIR}/arch/arm/mach-sun7i/pm/standby all
}

NAND_ROOT=${LICHEE_KDIR}/modules/nand

build_nand_lib()
{
    echo "build nand library ${NAND_ROOT}/lib"
    if [ -d ${NAND_ROOT}/lib ]; then
        echo "build nand library now"
        make -C modules/nand/lib clean 2> /dev/null 
        make -C modules/nand/lib lib install
    else
        echo "build nand with existing library"
    fi
}

copy_nand_mod()
{
    cd ${LICHEE_KDIR}
    if [ -x "./scripts/build_rootfs.sh" ]; then
        ./scripts/build_rootfs.sh e rootfs/rootfs.cpio.gz > /dev/null
    else
        echo "No such file: scripts/build_rootfs.sh"
        exit 1
    fi

    mkdir -p ./skel/lib/modules/${KERNEL_VERSION}
    cp ${LICHEE_MOD_DIR}/nand.ko ./skel/lib/modules/${KERNEL_VERSION}
    if [ $? -ne 0 ]; then
        echo "copy nand module error: $?"
        exit 1
    fi

    rm -f rootfs.cpio.gz
    ./scripts/build_rootfs.sh c rootfs.cpio.gz > /dev/null
    rm -rf skel
}

build_kernel()
{
    echo "Building kernel"

    if [ ! -f .config ] ; then
        printf "\n\033[0;31;1mUsing default config ...\033[0m\n\n"
        cp arch/arm/configs/${LICHEE_KERN_DEFCONF} .config
    fi

    build_standby
    make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j${jobs} uImage modules

    ${OBJCOPY} -R .note.gnu.build-id -S -O binary vmlinux bImage

    update_kernel_ver
    rm -rf output
    mkdir -p ${LICHEE_MOD_DIR}
    cp bImage output/
    cp -f arch/arm/boot/[zu]Image output/
    cp .config output/

    for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
        cp $file ${LICHEE_MOD_DIR}
    done
    cp -f Module.symvers ${LICHEE_MOD_DIR}

    #cp drivers/net/wireless/bcm4330/firmware/bcm4330.bin ${LICHEE_MOD_DIR}
    #cp drivers/net/wireless/bcm4330/firmware/bcm4330.hcd ${LICHEE_MOD_DIR}
    #cp drivers/net/wireless/bcm4330/firmware/nvram.txt ${LICHEE_MOD_DIR}/bcm4330_nvram.txt
}

build_modules()
{
    echo "Building modules"

    if [ ! -f include/generated/utsrelease.h ]; then
        printf "Please build kernel first\n"
        exit 1
    fi
    update_kernel_ver

    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
        install

    build_nand_lib
    make -C modules/nand LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
        install
    copy_nand_mod
        
    (
    export LANG=en_US.UTF-8
    unset LANGUAGE
    make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
        install
    )

    ##build ar6302 sdio wifi module
    #make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
    #    ARCH=arm KERNEL_DIR=${LICHEE_KDIR} INSTALL_DIR=${LICHEE_MOD_DIR} \
    #    all install
    ##build ar6003 sdio wifi module
    #make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
    #    ARCH=arm KERNEL_DIR=${LICHEE_KDIR} INSTALL_DIR=${LICHEE_MOD_DIR} \
    #    all
    ##build usi-bmc4329 sdio wifi module
    #make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} dhd-cdc-sdmmc-gpl
    ##build bcm40181 sdio wifi module 5.90.125.69.2
    #make -C modules/wifi/bcm40181/5.90.125.69.2/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 dhd-cdc-sdmmc-gpl
    ##build bcm40183 sdio wifi module
    #make -C modules/wifi/bcm40183/5.90.125.95.3/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 dhd-cdc-sdmmc-gpl
}

build_bootimg()
{
    echo "Building boot image"
    ${LICHEE_TOOLS_DIR}/pack/pctools/linux/android/mkbootimg \
        --kernel output/bImage \
        --ramdisk rootfs.cpio.gz \
        --board 'sun7i' \
        --base 0x40000000 \
        -o output/boot.img
    echo "Copy boot.img to output directory ..."
    cp output/boot.img ${LICHEE_PLAT_OUT}
}

gen_output()
{
    if [ -d ${LICHEE_BR_OUT}/target ] ; then
        echo "Copy modules to target ..."
        local module_dir="${LICHEE_BR_OUT}/target/lib/modules"
        rm -rf ${module_dir}
        mkdir -p ${module_dir}
        cp -rf ${LICHEE_MOD_DIR} ${module_dir}
    fi
}

clean_kernel()
{
    echo "Cleaning kernel"
    make distclean
    rm -rf output/*
}

clean_modules()
{
    echo "Cleaning modules"
    make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    
    (
    export LANG=en_US.UTF-8
    unset LANGUAGE
    make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    )

    ##build ar6302 sdio wifi module
    #make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
    #    ARCH=arm KERNEL_DIR=${LICHEE_KDIR} INSTALL_DIR=${LICHEE_MOD_DIR} \
    #    clean
    ##build ar6003 sdio wifi module
    #make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
    #    ARCH=arm KERNEL_DIR=${LICHEE_KDIR} INSTALL_DIR=${LICHEE_MOD_DIR} \
    #    clean
    ##build usi-bmc4329 sdio wifi module
    #make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} clean
    ##build bcm40181 sdio wifi module 5.90.125.69.2
    #make -C modules/wifi/bcm40181/5.90.125.69.2/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} clean
    ##build bcm40183 sdio wifi module
    #make -C modules/wifi/bcm40183/5.90.125.95.3/open-src/src/dhd/linux \
    #    CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
    #    LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} \
    #    INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 clean
}

case "$1" in
    kernel)
        build_kernel
        ;;
    modules)
        build_modules
        ;;
    clean)
        clean_modules
        clean_kernel
        ;;
    *)
        build_kernel
        build_modules
        build_bootimg
        gen_output
        ;;
esac

