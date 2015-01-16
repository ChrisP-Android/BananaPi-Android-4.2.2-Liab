#!/bin/bash
cd $PACKAGE
SIGMODE="none"
if [ "$1" = "-s" -o "$2" = "-s" ]; then
	echo "-------------------sig version-------------------"
	SIGMODE="sig";
fi
  ./pack -c sun7i -p android -b wing-evb-v10  -d uart0 -s ${SIGMODE}
cd -
