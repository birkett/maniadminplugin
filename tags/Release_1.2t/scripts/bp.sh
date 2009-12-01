#!/bin/sh

PROG_NAME=$(basename $0)
BASE_FOLDER=$(dirname $(readlink -f $0))/..
CLEAN=FALSE
DEBUG_ON=FALSE
OUTPUT_DIR="$BASE_FOLDER/plugin_output/legacy_bin"

##########################################
dump_vars()
{
echo "ORANGE_BUILD = $ORANGE_BUILD"
echo "VSP_BUILD = $VSP_BUILD"
echo "DEBUG_ON = $DEBUG_ON"
echo ""
}

##########################################
show_build_mode()
{
echo "**********************************"
if [ "$CLEAN" = "TRUE" ]
then
	if [ "$DEBUG_ON" = "TRUE" ]
	then
		echo "CLEAN BUILD      DEBUG"
	else
		echo "CLEAN BUILD           "
	fi
else
	if [ "$DEBUG_ON" = "TRUE" ]
	then
		echo "DEBUG"
	else
		echo "NORMAL"
	fi
fi
echo "**********************************"
}

##########################################
select_build_type()
{
echo ""
echo "Select build option:"
echo ""
echo "Old SDK"
echo "1 - Build VSP"
echo "2 - Build SMM"
echo ""

echo "Orange SDK"
echo "3 - Build Orange VSP"
echo "4 - Build Orange SMM" 

echo ""
echo -e "> \c"
read REPLY

if [ "$REPLY" = "1" ]
then
#Old VSP
	export ORANGE_BUILD="FALSE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "2" ]
then
#Old SMM
	export ORANGE_BUILD="FALSE"
	export VSP_BUILD="FALSE"
elif [ "$REPLY" = "3" ]
then
#Orange VSP
	export ORANGE_BUILD="TRUE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "4" ]
then
#Orange SMM
	export ORANGE_BUILD="TRUE"
	export VSP_BUILD="FALSE"
else
	echo "Unknown selection !!"
	exit
fi
}

###############################
#Main Processing
###############################

while getopts cd OPTION
do
	case ${OPTION} in
	c) CLEAN=TRUE;;
	d) export DEBUG_ON=TRUE;;
	\?) print -u2 "Usage: ${PROG_NAME} [ -c -d ]"
	exit 2;;
	esac
done

show_build_mode
select_build_type

cd $BASE_FOLDER/mani_admin_plugin/
export DEBUG_ON
dump_vars


EXE_DIR=$BASE_FOLDER/mani_admin_plugin

if [ -f $EXE_DIR/mani_admin_plugin_mm_i486.so ] && [ "$VSP_BUILD" = "TRUE" ]
then
	rm $EXE_DIR/mani_admin_plugin_mm_i486.so
fi

if [ -f $EXE_DIR/mani_admin_plugin_i486.so ] && [ "$VSP_BUILD" = "FALSE" ]
then
	rm $EXE_DIR/mani_admin_plugin_i486.so
fi

#Setup some environment variables for the make file

# the dir we want to put binaries we build into
export BUILD_DIR=.

# the place to put object files
export BUILD_OBJ_DIR="$BUILD_DIR/obj"
export PLUGIN_OBJ_DIR="$BUILD_OBJ_DIR"
export PUBLIC_OBJ_DIR="$BUILD_OBJ_DIR/public"
export TIER0_OBJ_DIR="$BUILD_OBJ_DIR/tier0"
export TIER1_OBJ_DIR="$BUILD_OBJ_DIR/tier1"
export MATHLIB_OBJ_DIR="$BUILD_OBJ_DIR/mathlib"

if [ "$ORANGE_BUILD" = "FALSE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# Old VSP Build
###############################
	echo "OLD VSP BUILD"
	export HL2SDK_SRC_DIR="../sdk"
        export HL2BIN_DIR="../srcds_1/bin"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export SOURCEHOOK_SRC_DIR="../sourcemm_1_7/core-legacy/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I$HL2SDK_SRC_DIR/../mani_admin_plugin/mysql5.1/linux_32/include -I../sourcemm_1_7/core-legacy -I../sourcemm_1_7/core-legacy/sourcehook -I$HL2SDK_SRC_DIR/public/vstdlib -I./jit -I./asm -I./Knight -I../sdk/public/steam"

	export NAME="mani_admin_plugin"

	export TIER1_OBJS="$TIER1_OBJ_DIR/convar.o $TIER1_OBJ_DIR/interface.o $TIER1_OBJ_DIR/KeyValues.o $TIER1_OBJ_DIR/bitbuf.o $TIER1_OBJ_DIR/utlbuffer.o"

	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride-vc7.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o"
	export EXTRA_FILES_1="serverplugin_convar.cpp mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="../sdk/linux_sdk/tier1_i486.a ../sdk/linux_sdk/mathlib_i486.a"



elif [ "$ORANGE_BUILD" = "FALSE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# Old SMM Build
###############################
	echo "OLD SMM BUILD"
	export HL2SDK_SRC_DIR="../sdk"
        export HL2BIN_DIR="../srcds_1/bin"

	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export SOURCEHOOK_SRC_DIR="../sourcemm_1_7/core-legacy/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls  -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I$HL2SDK_SRC_DIR/../mani_admin_plugin/mysql5.1/linux_32/include/ -I../sourcemm_1_7/core-legacy -I../sourcemm_1_7/core-legacy/sourcehook -I$HL2SDK_SRC_DIR/public/vstdlib -I./jit -I./asm -I./Knight -I../sdk/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/convar.o $TIER1_OBJ_DIR/KeyValues.o $TIER1_OBJ_DIR/bitbuf.o $TIER1_OBJ_DIR/utlbuffer.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride-vc7.o"
	export SOURCEMM="-DSOURCEMM"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="../sdk/linux_sdk/tier1_i486.a ../sdk/linux_sdk/mathlib_i486.a"


elif [ "$ORANGE_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# Orange VSP Build
###############################
	echo "ORANGE VSP BUILD"
	export HL2SDK_SRC_DIR="../sdk_orange"
        export HL2BIN_DIR="../srcds_1/orangebox/bin"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="../sourcemm_1_7/core/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I$HL2SDK_SRC_DIR/../mani_admin_plugin/mysql5.1/linux_32/include/ -I../sourcemm_1_7/core -I../sourcemm_1_7/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I../sdk/public/steam"

	export NAME="mani_admin_plugin"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o"
	export ORANGE="-DORANGE"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="../sdk_orange/lib/linux/tier1_i486.a ../sdk_orange/lib/linux/mathlib_i486.a"


else
###############################
# Orange SMM Build
###############################
	echo "ORANGE SMM BUILD"
	export HL2SDK_SRC_DIR="../sdk_orange"
        export HL2BIN_DIR="../srcds_1/orangebox/bin"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I$HL2SDK_SRC_DIR/../mani_admin_plugin/mysql5.1/linux_32/include/ -I../sourcemm_1_7/core -I../sourcemm_1_7/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I../sdk/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DORANGE"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="../sdk_orange/lib/linux/tier1_i486.a ../sdk_orange/lib/linux/mathlib_i486.a"
fi

if [ "$CLEAN" = "TRUE" ]
then
	make clean
fi

make -j 2

COPY_SMM=""
COPY_ORANGE=""

if [ "$DEBUG_ON" = "TRUE" ]
then
	RELEASE_MODE=""
else
	RELEASE_MODE="-r"
fi

if [ "$ORANGE_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-o"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/orange_bin"
fi

cd -

if [ "$VSP_BUILD" = "TRUE" ]
then
	if [ -f $EXE_DIR/mani_admin_plugin_i486.so ]
	then
		cp -f $EXE_DIR/mani_admin_plugin_i486.so $OUTPUT_DIR/VSP
		./setbinaries.pl $COPY_SMM $COPY_ORANGE $RELEASE_MODE
	fi
else
	COPY_SMM="-s"	
	if [ -f $EXE_DIR/mani_admin_plugin_mm_i486.so ]
	then
		cp -f $EXE_DIR/mani_admin_plugin_mm_i486.so $OUTPUT_DIR/SourceMM
		./setbinaries.pl $COPY_SMM $COPY_ORANGE $RELEASE_MODE
	fi
fi

show_build_mode
echo "Finished option $REPLY"

