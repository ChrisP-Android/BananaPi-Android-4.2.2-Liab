#!/bin/bash
cd ../busybox-1.23.1
export PATH=$PATH:../lichee/out/android/common/buildroot/external-toolchain/bin/
make
cp busybox ../android/device/softwinner/common/bin
cp busybox ../android/device/softwinner/wing-common
