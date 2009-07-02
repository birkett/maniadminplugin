#!/bin/sh

echo ""
echo "Select build option:"
echo ""
echo "1 - Build normal"
echo "2 - Build normal clean"
echo "--"
echo "3 - Build normal debug"
echo "4 - Build normal debug clean"
echo "---------"
echo "5 - Build smm"
echo "6 - Build smm clean"
echo "--"
echo "7 - Build smm debug"
echo "8 - Build smm debug clean"

echo ""
echo -e "> \c"
read REPLY

export STRIP_SYM=""
export PROJ_NAME=""
export DEBUG_FLAGS=""
export SOURCE_MM=""
export SOURCEMM_DIR=""

if [ "$REPLY" = "1" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin
	export STRIP_SYM="-s"
	make
	copynormal.sh
	cd -
elif [ "$REPLY" = "2" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin
	export STRIP_SYM="-s"
	make clean
	make
	copynormal.sh
	cd -
elif [ "$REPLY" = "3" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin
	export DEBUG_FLAGS="-g -ggdb"
	make
	copynormal.sh
	cd -
elif [ "$REPLY" = "4" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin
	export DEBUG_FLAGS="-g -ggdb"
	make clean
	make
	copynormal.sh
	cd -
elif [ "$REPLY" = "5" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin_mm
	export STRIP_SYM="-s"
	export SOURCE_MM="-DSOURCEMM"
	export SOURCEMM_DIR="-I../sourcemm/ -I../sourcemm/sourcemm/"
	make
	copysmm.sh
	cd -
elif [ "$REPLY" = "6" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin_mm
	export STRIP_SYM="-s"
	export SOURCE_MM="-DSOURCEMM"
	export SOURCEMM_DIR="-I../sourcemm/ -I../sourcemm/sourcemm/"
	make clean
	make
	copysmm.sh
	cd -
elif [ "$REPLY" = "7" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin_mm
	export DEBUG_FLAGS="-g -ggdb"
	export SOURCE_MM="-DSOURCEMM"
	export SOURCEMM_DIR="-I../sourcemm/ -I../sourcemm/sourcemm/"
	make
	copysmm.sh
elif [ "$REPLY" = "8" ]
then
	cd $HOME/MyDev/mani_admin_plugin/
	export PROJ_NAME=mani_admin_plugin_mm
	export DEBUG_FLAGS="-g -ggdb"
	export SOURCE_MM="-DSOURCEMM"
	export SOURCEMM_DIR="-I../sourcemm/ -I../sourcemm/sourcemm/"
	make clean
	make
	copysmm.sh
	cd -
else
	echo "Unknown selection !!"
	exit
fi

