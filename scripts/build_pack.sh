set -e
cd ../scripts
./user.sh


cd ../android
source build/envsetup.sh
$LUNCHCHOICE

pack

cd ..
path="`pwd`/."
mv lichee/tools/pack/*.img $path

echo ""
echo "---------------------- Build Complete ----------------------"
echo "You will find the new image here: $path"
echo "Burn this image to an SD card using PhoenixCard"
echo "------------------------------------------------------------"
echo ""

cd scripts