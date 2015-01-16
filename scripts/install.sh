release=`lsb_release -r | awk '{ print $2 }'`

if [ "$release" = "14.04" ]; then
	./install_14.04.sh
elif [ "$release" = "12.04" ]; then
	./install_12.04.sh
else
	echo "Sorry, this install script only support Ubuntu 12.04 and 14.04"
fi