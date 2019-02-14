#!/bin/sh
# ---------------------------------------------------------
#  Set the core module defines according to Core Module
# ---------------------------------------------------------
config="realview_eb_config"
# ---------------------------------------------------------
# Set up the Realview type define
# ---------------------------------------------------------
# Use this until later versions provide distinct core tiles
# ---------------------------------------------------------
cpu=arm_rvebct
if [ "$1" == "" ]
then
	echo "$0:: No parameters - using arm-linux-gcc realview_eb_config"

else
	case "$config" in
	realview_eb_config)
	echo "#define CONFIG_REALVIEW_EB" > ./include/config.h
	;;

	*)
	echo "$0:: Unrecognised config - using realview_eb_config"
	echo "#define CONFIG_REALVIEW_EB" > ./include/config.h
	;;

	esac

fi
# ---------------------------------------------------------
#	Ensure correct core object loaded first in U-Boot image
# ---------------------------------------------------------
sed -r 's/CPU_FILE/cpu\/'$cpu'\/start.o/; s/#.*//' board/realview_eb/u-boot.lds.template > board/realview_eb/u-boot.lds
# ---------------------------------------------------------
# Complete the configuration
# ---------------------------------------------------------
./mkconfig -a realview_eb arm $cpu realview_eb
