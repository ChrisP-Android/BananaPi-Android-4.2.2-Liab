set -e

cd ../lichee/linux-3.4
make clean
cd ..
./build.sh -p sun7i_android
cd ../scripts
