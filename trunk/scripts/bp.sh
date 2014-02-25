#!/bin/sh
export PATH="./:$PATH"
PROG_NAME=$(basename $0)
BASE_FOLDER=$(dirname $(readlink -f $0))/..
CLEAN=FALSE
DEBUG_ON=FALSE
OUTPUT_DIR="$BASE_FOLDER/plugin_output/orange_bin"

##########################################
dump_vars()
{
echo "CSGO_BUILD = $CSGO_BUILD"
echo "CSS_BUILD = $CSS_BUILD"
echo "DODS_BUILD = $DODS_BUILD"
echo "HL2DM_BUILD = $HL2DM_BUILD"
echo "TF2_BUILD = $TF2_BUILD"


echo "VSP_BUILD = $VSP_BUILD"
echo "DEBUG_ON = $DEBUG_ON"
echo ""
}

#########################################
create_folder()
{
if [ ! -d "$1" ]; then
	mkdir $1
fi
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
echo "CSGO SDK"
echo "0 - Build CSGO VSP"
echo "1 - Build CSGO SMM" 

echo "CSS SDK"
echo "2 - Build CSS VSP"
echo "3 - Build CSS SMM"

echo "DODS SDK"
echo "4 - Build DODS VSP"
echo "5 - Build DODS SMM"

echo "HL2DM SDK"
echo "6 - Build HL2DM VSP"
echo "7 - Build HL2DM SMM"

echo "TF2 SDK"
echo "8 - Build TF2 VSP"
echo "9 - Build TF2 SMM"   

echo ""
echo -e "> \c"
read REPLY

if [ "$REPLY" = "0" ]
then
#CSGO VSP
	export CSGO_BUILD="TRUE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "1" ]
then
#CSGO SMM
	export CSGO_BUILD="TRUE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="FALSE"
elif [ "$REPLY" = "2" ]
then
#CSS VSP
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="TRUE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "3" ]
then
#CSS SMM
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="TRUE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="FALSE"
elif [ "$REPLY" = "4" ]
then
#DODS VSP
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="TRUE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "5" ]
then
#DODS SMM
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="TRUE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="FALSE"
elif [ "$REPLY" = "6" ]
then
#HL2DM VSP
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="TRUE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "7" ]
then
#HL2DM SMM
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="TRUE"
	export TF2_BUILD="FALSE"
	export VSP_BUILD="FALSE"
elif [ "$REPLY" = "8" ]
then
#TF2 VSP
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="TRUE"
	export VSP_BUILD="TRUE"
elif [ "$REPLY" = "9" ]
then
#TF2 SMM
	export CSGO_BUILD="FALSE"
	export CSS_BUILD="FALSE"
	export DODS_BUILD="FALSE"
	export HL2DM_BUILD="FALSE"
	export TF2_BUILD="TRUE"
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
export DEMANGLE_OBJ_DIR="$BUILD_OBJ_DIR/demangle"
export DEMANGLE_OBJS="$DEMANGLE_OBJ_DIR/cp-demangle.o $DEMANGLE_OBJ_DIR/cp-demint.o $DEMANGLE_OBJ_DIR/cplus-dem.o $DEMANGLE_OBJ_DIR/safe-ctype.o $DEMANGLE_OBJ_DIR/xmalloc.o $DEMANGLE_OBJ_DIR/xstrdup.o"
export DEMANGLE_SRC_DIR="./demangle"

if [ "$CSGO_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# CSGO VSP Build
###############################
	echo "CSGO VSP BUILD"
	export HL2SDK_SRC_DIR="../sdks/csgo"
        export HL2BIN_DIR="../sdks/csgo/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"
	export PROTOBUF_SRC_DIR="$PUBLIC_SRC_DIR/game/shared/csgo/protobuf"
	export PROTOBUF_SRC_DIR2="$PUBLIC_SRC_DIR/engine/protobuf"
	export PROTOBUF_OBJ_DIR="$BUILD_OBJ_DIR/protobuf"

	export INCLUDEDIRS=" -I$HL2SDK_SRC_DIR/common/protobuf-2.3.0/src -I$PUBLIC_SRC_DIR/engine/protobuf -I$PUBLIC_SRC_DIR/game/shared/csgo/protobuf -I$HL2SDK_SRC_DIR/public/mathlib -I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier0 -I$PUBLIC_SRC_DIR/tier1 -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/game/server -I$PUBLIC_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/game/shared -I$HL2SDK_SRC_DIR/public/steam -I./mysql5.1/linux_32/include/ -I$SOURCEHOOK_SRC_DIR -I$SOURCEMM_ROOT -I./jit -I./Knight -I./asm"

	export NAME="mani_admin_plugin"
#	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o $SOURCEHOOK_OBJ_DIR/sourcehook_hookmangen.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookidman.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookmaninfo.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cproto.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cvfnptr.o"
	export PROTOBUF_OBJS="$PROTOBUF_OBJ_DIR/netmessages.pb.o $PROTOBUF_OBJ_DIR/cstrike15_usermessages.pb.o $PROTOBUF_OBJ_DIR/cstrike15_usermessage_helpers.o"
	export ORANGE="-DGAME_ORANGE -DGAME_CSGO -DCOMPILER_GCC -DPOSIX -DPLATFORM_POSIX"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a $HL2SDK_SRC_DIR/lib/linux/interfaces_i486.a $HL2SDK_SRC_DIR/lib/linux32/release/libprotobuf.a"
	export TIER0_SO="libtier0.so"
	export VSTDLIB_SO="libvstdlib.so"

elif [ "$CSGO_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# CSGO SMM Build
###############################
	echo "CSGO SMM BUILD"
	export HL2SDK_SRC_DIR="../sdks/csgo"
        export HL2BIN_DIR="../sdks/csgo/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"
	export PROTOBUF_SRC_DIR="$PUBLIC_SRC_DIR/game/shared/csgo/protobuf"
	export PROTOBUF_SRC_DIR2="$PUBLIC_SRC_DIR/engine/protobuf"
	export PROTOBUF_OBJ_DIR="$BUILD_OBJ_DIR/protobuf"

	export INCLUDEDIRS=" -I$HL2SDK_SRC_DIR/common/protobuf-2.3.0/src -I$PUBLIC_SRC_DIR/engine/protobuf -I$PUBLIC_SRC_DIR/game/shared/csgo/protobuf -I$HL2SDK_SRC_DIR/public/mathlib -I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier0 -I$PUBLIC_SRC_DIR/tier1 -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/game/server -I$PUBLIC_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/game/shared -I$HL2SDK_SRC_DIR/public/steam -I./mysql5.1/linux_32/include/ -I$SOURCEHOOK_SRC_DIR -I$SOURCEMM_ROOT -I./jit -I./Knight -I./asm"

	export NAME="mani_admin_plugin_mm"
#	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export PROTOBUF_OBJS="$PROTOBUF_OBJ_DIR/netmessages.pb.o $PROTOBUF_OBJ_DIR/cstrike15_usermessages.pb.o $PROTOBUF_OBJ_DIR/cstrike15_usermessage_helpers.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DGAME_ORANGE -DGAME_CSGO -DCOMPILER_GCC -DPOSIX -DPLATFORM_POSIX"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a $HL2SDK_SRC_DIR/lib/linux/interfaces_i486.a $HL2SDK_SRC_DIR/lib/linux32/release/libprotobuf.a"
	export TIER0_SO="libtier0.so"
	export VSTDLIB_SO="libvstdlib.so"

elif [ "$CSS_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# CSS VSP Build
###############################
	echo "CSS VSP BUILD"
	export HL2SDK_SRC_DIR="../sdks/css"
        export HL2BIN_DIR="../sdks/css/lib/linux" 
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o $SOURCEHOOK_OBJ_DIR/sourcehook_hookmangen.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookidman.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookmaninfo.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cproto.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cvfnptr.o"
	export ORANGE="-DGAME_ORANGE -DGAME_CSS"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$CSS_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# CSS SMM Build
###############################
	echo "CSS SMM BUILD"
	export HL2SDK_SRC_DIR="../sdks/css"
        export HL2BIN_DIR="../sdks/css/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DGAME_ORANGE -DGAME_CSS"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$DODS_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# DODS VSP Build
###############################
	echo "DODS VSP BUILD"
	export HL2SDK_SRC_DIR="../sdks/dods"
        export HL2BIN_DIR="../sdks/dods/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o $SOURCEHOOK_OBJ_DIR/sourcehook_hookmangen.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookidman.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookmaninfo.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cproto.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cvfnptr.o"
	export ORANGE="-DGAME_ORANGE -DGAME_DODS"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$DODS_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# DODS SMM Build
###############################
	echo "DODS SMM BUILD"
	export HL2SDK_SRC_DIR="../sdks/dods"
        export HL2BIN_DIR="../sdks/dods/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DGAME_ORANGE -DGAME_DODS"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"
	
elif [ "$HL2DM_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# HL2DM VSP Build
###############################
	echo "HL2DM VSP BUILD"
	export HL2SDK_SRC_DIR="../sdks/hl2dm"
        export HL2BIN_DIR="../sdks/hl2dm/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o $SOURCEHOOK_OBJ_DIR/sourcehook_hookmangen.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookidman.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookmaninfo.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cproto.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cvfnptr.o"
	export ORANGE="-DGAME_ORANGE -DGAME_HL2DM"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$HL2DM_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# HL2DM SMM Build
###############################
	echo "HL2DM SMM BUILD"
	export HL2SDK_SRC_DIR="../sdks/hl2dm"
        export HL2BIN_DIR="../sdks/hl2dm/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DGAME_ORANGE -DGAME_HL2DM"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$TF2_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "TRUE" ]
then
###############################
# TF2 VSP Build
###############################
	echo "TF2 VSP BUILD"
	export HL2SDK_SRC_DIR="../sdks/tf2"
        export HL2BIN_DIR="../sdks/tf2/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
#	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export SOURCEHOOK_OBJ_DIR="$BUILD_OBJ_DIR/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEHOOK_OBJS="$SOURCEHOOK_OBJ_DIR/sourcehook.o $SOURCEHOOK_OBJ_DIR/sourcehook_hookmangen.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookidman.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_chookmaninfo.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cproto.o $SOURCEHOOK_OBJ_DIR/sourcehook_impl_cvfnptr.o"
	export ORANGE="-DGAME_ORANGE -DGAME_TF2"
	export EXTRA_FILES_1="mani_callback_valve.cpp mani_sourcehook.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"

elif [ "$TF2_BUILD" = "TRUE" ] && [ "$VSP_BUILD" = "FALSE" ]
then
###############################
# TF2 SMM Build
###############################
	echo "TF2 SMM BUILD"
	export HL2SDK_SRC_DIR="../sdks/tf2"
        export HL2BIN_DIR="../sdks/tf2/lib/linux"
	export SOURCEMM_ROOT="../sourcemm_sdk/core"
	export PLUGIN_SRC_DIR="."
	export PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public"
	export TIER0_PUBLIC_SRC_DIR="$HL2SDK_SRC_DIR/public/tier0"
	export TIER1_SRC_DIR="$HL2SDK_SRC_DIR/tier1"
	export MATHLIB_SRC_DIR="$HL2SDK_SRC_DIR/mathlib"
	export SOURCEHOOK_SRC_DIR="$SOURCEMM_ROOT/sourcehook"
	export ASM_OBJ_DIR="$BUILD_OBJ_DIR/asm"
	export KNIGHT_OBJ_DIR="$BUILD_OBJ_DIR/Knight"

	export INCLUDEDIRS="-I$PUBLIC_SRC_DIR -I$PUBLIC_SRC_DIR/tier1 -I$PUBLIC_SRC_DIR/engine -I$PUBLIC_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/game_shared -I$HL2SDK_SRC_DIR/common -I$HL2SDK_SRC_DIR/dlls -I$HL2SDK_SRC_DIR/public/mathlib -I$HL2SDK_SRC_DIR/public/tier0 -I./mysql5.1/linux_32/include/ -I../sourcemm_sdk/core -I../sourcemm_sdk/core/sourcehook -I$HL2SDK_SRC_DIR/game/server -I$HL2SDK_SRC_DIR/public/game/server -I$HL2SDK_SRC_DIR/game/shared -I./jit -I./asm -I./Knight -I$HL2SDK_SRC_DIR/public/steam"

	export NAME="mani_admin_plugin_mm"
	export TIER1_OBJS="$TIER1_OBJ_DIR/bitbuf.o"
#	export MATHLIB_OBJS="$MATHLIB_OBJ_DIR/mathlib_base.o"
	export TIER0_OBJS="$TIER0_OBJ_DIR/memoverride.o"
	export SOURCEMM="-DSOURCEMM"
	export ORANGE="-DGAME_ORANGE -DGAME_TF2"
	export EXTRA_FILES_1="mani_callback_sourcemm.cpp asm/asm.c Knight/KeCodeAllocator.cpp"
	export EXTRA_LIBS="$HL2SDK_SRC_DIR/lib/linux/tier1_i486.a $HL2SDK_SRC_DIR/lib/linux/mathlib_i486.a"
	export TIER0_SO="libtier0_srv.so"
	export VSTDLIB_SO="libvstdlib_srv.so"	



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

if [ "$CSGO_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-g"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/csgo_bin"

elif [ "$CSS_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-c"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/css_bin"

elif [ "$DODS_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-d"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/dods_bin"

elif [ "$HL2DM_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-h"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/hl2dm_bin"

elif [ "$TF2_BUILD" = "TRUE" ]
then
	COPY_ORANGE="-t"
	OUTPUT_DIR="$BASE_FOLDER/plugin_output/tf2_bin"
fi

cd -

# Create output folders if they don't already exist
create_folder "$BASE_FOLDER/plugin_output"
create_folder "$BASE_FOLDER/plugin_output/csgo_bin"
create_folder "$BASE_FOLDER/plugin_output/csgo_bin/VSP"
create_folder "$BASE_FOLDER/plugin_output/csgo_bin/SourceMM"
create_folder "$BASE_FOLDER/plugin_output/css_bin"
create_folder "$BASE_FOLDER/plugin_output/css_bin/VSP"
create_folder "$BASE_FOLDER/plugin_output/css_bin/SourceMM"
create_folder "$BASE_FOLDER/plugin_output/dods_bin"
create_folder "$BASE_FOLDER/plugin_output/dods_bin/VSP"
create_folder "$BASE_FOLDER/plugin_output/dods_bin/SourceMM"
create_folder "$BASE_FOLDER/plugin_output/hl2dm_bin"
create_folder "$BASE_FOLDER/plugin_output/hl2dm_bin/VSP"
create_folder "$BASE_FOLDER/plugin_output/hl2dm_bin/SourceMM"
create_folder "$BASE_FOLDER/plugin_output/tf2_bin"
create_folder "$BASE_FOLDER/plugin_output/tf2_bin/VSP"
create_folder "$BASE_FOLDER/plugin_output/tf2_bin/SourceMM"



if [ "$VSP_BUILD" = "TRUE" ]
then
	if [ -f $EXE_DIR/mani_admin_plugin_i486.so ]
	then
		cp -f $EXE_DIR/mani_admin_plugin_i486.so $OUTPUT_DIR/VSP
		setbinaries.pl $COPY_SMM $COPY_ORANGE $RELEASE_MODE
	fi
else
	COPY_SMM="-s"	
	if [ -f $EXE_DIR/mani_admin_plugin_mm_i486.so ]
	then
		cp -f $EXE_DIR/mani_admin_plugin_mm_i486.so $OUTPUT_DIR/SourceMM
		setbinaries.pl $COPY_SMM $COPY_ORANGE $RELEASE_MODE
	fi
fi

show_build_mode
echo "Finished option $REPLY"
