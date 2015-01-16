set -e

cd ../android
source build/envsetup.sh
lunch sugar_lemaker-eng
extract-bsp

make -j`nproc`

cd ../scripts