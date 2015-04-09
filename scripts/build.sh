#LUNCHCHOICE='lunch sugar_lemaker-eng'
LUNCHCHOICE='lunch sugar_lemaker-userdebug'
#LUNCHCHOICE='lunch sugar_bpi-eng'
#LUNCHCHOICE='lunch sugar_bpi-userdebug'
export LUNCHCHOICE
./build_kernel.sh
./build_android.sh
./build_pack.sh