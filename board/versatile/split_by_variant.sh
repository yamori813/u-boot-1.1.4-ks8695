#!/bin/sh
# ---------------------------------------------------------
#  Set the core module defines according to Core Module
# ---------------------------------------------------------
# ---------------------------------------------------------
# Set up the Versatile type define
# ---------------------------------------------------------
variant=versatilepb
if [ "$1" == "" ]
then
	echo "$0:: No parameters - using versatilepb_config"
	echo "#define CONFIG_ARCH_VERSATILE_PB" > ./include/config.h
	variant=versatilepb
else
	case "$1" in
	versatile_config)
	echo "#define CONFIG_ARCH_VERSATILE_PB" > ./include/config.h
	;;

	versatilepb_config)
	echo "#define CONFIG_ARCH_VERSATILE_PB" > ./include/config.h
	;;

	versatileab_config)
	echo "#define CONFIG_ARCH_VERSATILE_AB" > ./include/config.h
	variant=versatileab
	;;


	*)
	echo "$0:: Unrecognised config - using versatilepb_config"
	echo "#define CONFIG_ARCH_VERSATILE_PB" > ./include/config.h
	variant=versatilepb
	;;

	esac

fi


# ---------------------------------------------------------
# Link variant header to versatile.h
# ---------------------------------------------------------
if [ "$variant" != "versatile" ] 
then
	ln -s ./versatile.h ./include/configs/$variant.h
fi

# ---------------------------------------------------------
# Complete the configuration
# WAS ./mkconfig -a $variant arm arm926ejs versatile
# ---------------------------------------------------------
./mkconfig -a versatile arm arm926ejs versatile NULL versatile
echo "Variant:: $variant"
