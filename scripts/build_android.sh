set -e

cd ../android
source build/envsetup.sh
$LUNCHCHOICE
extract-bsp

make -j`nproc`

cd ../scripts