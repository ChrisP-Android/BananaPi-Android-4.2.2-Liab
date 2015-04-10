set -e

cd ../android

source build/envsetup.sh
lunch
extract-bsp
pack

cd ..
path="`pwd`/bpi_android.img"
#mv lichee/tools/pack/sun7i_android_sugar-lemaker.img $path

echo ""
echo "---------------------- Build Complete ----------------------"
#echo "You will find the new image here: $path"
echo "Burn this image to an SD card using PhoenixCard"
echo "------------------------------------------------------------"
echo ""

cd scripts