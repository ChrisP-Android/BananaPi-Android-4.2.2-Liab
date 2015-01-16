#!/bin/bash
cd ../lichee
#./build.sh -p sun7i_android

cd ../android
read -p 'make clean? ENTER=no ' y
if [ "$y" != "" ]; then
make clean
fi
source build/envsetup.sh
lunch
#lunch sugar_bpi-eng
#lunch sugar_lemaker-eng
#!/bin/bash
for (( i=30; i>0; i--)); do
printf "\rWaiting for $i seconds. Control the options! Hit any key to continue."
read -s -n 1 -t 1 key
if [ $? -eq 0 ]; then
break
fi
done
echo

extract-bsp
make -j`nproc`
if [ $? != 0 ] ; then
echo
echo "Error make!"
exit
fi
pack

cd ..
#mv lichee/tools/pack/sun7i_android_sugar-bpi.img ./bpi_android.img

echo ""
echo "---------------------- Build Complete ----------------------" 
echo "You will find the new 'android.img' in /lichee/tools/pack"
echo "Burn this image to an SD card using PhoenixCard"
echo "------------------------------------------------------------"
echo ""

echo "You will find the new 'android.img' in /lichee/tools/pack" >$HOME/build-android.log
for (( i=60; i>0; i--)); do
paplay /usr/share/sounds/pop.wav
printf "\rWaiting for $i seconds. Hit any key to continue."
read -s -n 1 -t 1 key
if [ $? -eq 0 ]
then
echo
exit
fi
done

for (( i=600; i>0; i--)); do
printf "\rWaiting for $i seconds before shutdown. Hit any key to continue."
read -s -n 1 -t 1 key
if [ $? -eq 0 ]
then
echo
exit
fi
done
qdbus org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout 1 2 2
exit
