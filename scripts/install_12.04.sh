DEBIAN_FRONTEND=noninteractive
apt-get install -y python-software-properties
add-apt-repository -y ppa:webupd8team/java
apt-get update -y
apt-get install -y git-core gnupg flex bison gperf build-essential zip curl \
	zlib1g-dev libc6-dev lib32ncurses5-dev python-markdown \
	ia32-libs x11proto-core-dev libx11-dev \
	lib32z1-dev libgl1-mesa-dev g++-multilib mingw32 tofrodos \
	libxml2-utils libglapi-mesa:i386 uboot-mkimage oracle-java6-installer xsltproc