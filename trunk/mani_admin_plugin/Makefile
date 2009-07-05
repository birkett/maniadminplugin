#############################################################################
# Developer configurable items
#############################################################################

# Variables to be setup before calling make
#
# ORANGE_BUILD
# VSP_BUILD
# DEBUG_ON


ifeq ($(DEBUG_ON), "TRUE")
	DEBUG_FLAGS="-g -ggdb"
else
	STRIP_SYM=-s
endif


# source files that should be compiled and linked into the binary
PLUGIN_FILES =  mani_adverts.cpp \
                mani_afk.cpp \
                mani_anti_rejoin.cpp \
                mani_autokickban.cpp \
                mani_automap.cpp \
                mani_chattrigger.cpp \
                mani_client.cpp \
                mani_client_interface.cpp \
                mani_client_sql.cpp \
                mani_client_util.cpp \
                mani_commands.cpp \
                mani_convar.cpp \
                mani_crontab.cpp \
                mani_css_betting.cpp \
                mani_css_bounty.cpp \
                mani_css_objectives.cpp \
                mani_customeffects.cpp \
                mani_database.cpp \
                mani_downloads.cpp \
                mani_effects.cpp \
                mani_file.cpp \
                mani_gametype.cpp \
                mani_globals.cpp \
                mani_ghost.cpp \
                mani_help.cpp \
                mani_hlxinterface.cpp \
                mani_keyvalues.cpp \
                mani_language.cpp \
		mani_log_css_stats.cpp \
		mani_log_dods_stats.cpp \
                mani_main.cpp \
                mani_mapadverts.cpp \
                mani_maps.cpp \
                mani_menu.cpp \
                mani_memory.cpp \
                mani_messagemode.cpp \
		mani_mostdestructive.cpp \
		mani_mp_restartgame.cpp \
		mani_mutex.cpp \
                mani_mysql.cpp \
		mani_netidvalid.cpp \
		mani_observer_track.cpp \
		mani_output.cpp \
		mani_panel.cpp \
		mani_parser.cpp \
		mani_ping.cpp \
		mani_player.cpp \
		mani_quake.cpp \
		mani_replace.cpp \
		mani_reservedslot.cpp \
		mani_save_scores.cpp \
		mani_sigscan.cpp \
		mani_skins.cpp \
		mani_sounds.cpp \
		mani_spawnpoints.cpp \
		mani_sprayremove.cpp \
		mani_stats.cpp \
		mani_team.cpp \
		mani_team_join.cpp \
		mani_teamkill.cpp \
		mani_timers.cpp \
		mani_trackuser.cpp \
		mani_util.cpp \
		mani_vars.cpp \
		mani_vfuncs.cpp \
		mani_victimstats.cpp \
		mani_voice.cpp \
		mani_vote.cpp \
		mani_warmuptimer.cpp \
		mani_weapon.cpp \
		mrecipientfilter.cpp

PLUGIN_FILES+= $(EXTRA_FILES_1)

# compiler options (gcc 3.4.1 or above is required)
# to find the location for the CPP_LIB files use "updatedb" then "locate libstdc++.a"
CPLUS=/usr/bin/g++-3.4
CLINK=/usr/bin/gcc-3.4
CPP_LIB=/usr/lib/gcc/i486-linux-gnu/3.4.6/libstdc++.a \
	/usr/lib/gcc/i486-linux-gnu/3.4.6/libgcc_eh.a \
	./mysql5.1/linux_32/lib/libmysqlclient.a \
	./mysql5.1/linux_32/lib/libz.a \
	$(EXTRA_LIBS)

# put any compiler flags you want passed here
USER_CFLAGS=

# link flags for your mod, make sure to include any special libraries here
LDFLAGS=-lm -ldl $(STRIP_SYM) tier0_i486.so vstdlib_i486.so

#############################################################################
# Things below here shouldn't need to be altered
#############################################################################

# the CPU target for the build, must be i486 for now
ARCH=i486
ARCH_CFLAGS=-mtune=i686 -march=pentium3 -mmmx -O3

# -fpermissive is so gcc 3.4.x doesn't complain about some template stuff
BASE_CFLAGS=-fpermissive -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp \
	-D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
	-D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp $(SOURCEMM) $(ORANGE)
SHLIBEXT=so
SHLIBLDFLAGS=-shared

CFLAGS=$(USER_CFLAGS) $(BASE_CFLAGS) $(ARCH_CFLAGS)
CFLAGS+= $(DEBUG_FLAGS)

DO_CC=$(CPLUS) $(INCLUDEDIRS) -w $(CFLAGS) -DARCH=$(ARCH) -o $@ -c $<

#####################################################################

PLUGIN_OBJS := $(PLUGIN_FILES:%.cpp=$(PLUGIN_OBJ_DIR)/%.o)

all: dirs $(NAME)_$(ARCH).$(SHLIBEXT)

dirs:
	@echo "Orange build $(ORANGE_BUILD)"
	@echo "VSP Build $(VSP_BUILD)"
	@echo "Debug On $(DEBUG_ON)"
	@echo $(HL2BIN_DIR)

	@echo "HL2SDK_SRC_DIR = $(HL2SDK_SRC_DIR)"
	@echo "HL2BIN_DIR = $(HL2BIN_DIR)"
	@echo "EXTRA_LIBS = $(EXTRA_LIBS)"
	@echo "EXTRA_FILES_1 = $(EXTRA_FILES_1)"
	@echo "SOURCEMM_ROOT = $(SOURCEMM_ROOT)"
	@echo "ORANGE = $(ORANGE)"
	@echo "PLUGIN_SRC_DIR = $(PLUGIN_SRC_DIR)"
	@echo "PUBLIC_SRC_DIR = $(PUBLIC_SRC_DIR)"
	@echo "TIER0_PUBLIC_SRC_DIR = $(TIER0_PUBLIC_SRC_DIR)"
	@echo "TIER1_SRC_DIR = $(TIER1_SRC_DIR)"
	@echo "SOURCEHOOK_SRC_DIR = $(SOURCEHOOK_SRC_DIR)"
	@echo "INCLUDEDIRS = $(INCLUDEDIRS)"
	@echo "NAME = $(NAME)"
	@echo "TIER1_OBJS = $(TIER1_OBJS)"
	@echo "TIER0_OBJS = $(TIER0_OBJS)"
	@echo "SOURCEHOOK_OBJS = $(SOURCEHOOK_OBJS)"
	@echo "SOURCEHOOK_OBJ_DIR = $(SOURCEHOOK_OBJ_DIR)"
	@echo "SOURCEMM = $(SOURCEMM)"
	@echo "ORANGE = $(ORANGE)"
	@echo "PLUGIN_FILES = $(PLUGIN_FILES)"
	@echo "PLUGIN_OBJS = $(PLUGIN_OBJS)"
	@echo ""

	@if [ ! -f "tier0_i486.so" ]; then ln -s $(HL2BIN_DIR)/tier0_i486.so .; fi
	@if [ ! -f "vstdlib_i486.so" ]; then ln -s $(HL2BIN_DIR)/vstdlib_i486.so .; fi
	@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	@if [ ! -d $(BUILD_OBJ_DIR) ]; then mkdir $(BUILD_OBJ_DIR); fi
	@if [ ! -d $(PLUGIN_OBJ_DIR) ]; then mkdir $(PLUGIN_OBJ_DIR); fi
	@if [ ! -d $(PUBLIC_OBJ_DIR) ]; then mkdir $(PUBLIC_OBJ_DIR); fi
	@if [ ! -d $(TIER0_OBJ_DIR) ]; then mkdir $(TIER0_OBJ_DIR); fi
	@if [ ! -d $(TIER1_OBJ_DIR) ]; then mkdir $(TIER1_OBJ_DIR); fi
	@if [ ! -d $(MATHLIB_OBJ_DIR) ]; then mkdir $(MATHLIB_OBJ_DIR); fi
	@if [ ! -d $(SOURCEHOOK_OBJ_DIR) ]; then mkdir $(SOURCEHOOK_OBJ_DIR); fi


$(NAME)_$(ARCH).$(SHLIBEXT): $(PLUGIN_OBJS) $(PUBLIC_OBJS) $(TIER0_OBJS) $(TIER1_OBJS) $(MATHLIB_OBJS) $(SOURCEHOOK_OBJS)
	$(CLINK) $(DEBUG_FLAGS) -o $(BUILD_DIR)/$@ $(SHLIBLDFLAGS) $(PLUGIN_OBJS) $(PUBLIC_OBJS) $(TIER0_OBJS) $(TIER1_OBJS) $(MATHLIB_OBJS) \
	$(SOURCEHOOK_OBJS) $(CPP_LIB) $(EXTRA_LIBS) $(LDFLAGS)

$(PLUGIN_OBJ_DIR)/%.o: $(PLUGIN_SRC_DIR)/%.cpp
	$(DO_CC)

$(PUBLIC_OBJ_DIR)/%.o: $(PUBLIC_SRC_DIR)/%.cpp
	$(DO_CC)

$(TIER0_OBJ_DIR)/%.o: $(TIER0_PUBLIC_SRC_DIR)/%.cpp
	$(DO_CC)

$(TIER1_OBJ_DIR)/%.o: $(TIER1_SRC_DIR)/%.cpp
	$(DO_CC)

$(MATHLIB_OBJ_DIR)/%.o: $(MATHLIB_SRC_DIR)/%.cpp
	$(DO_CC)

$(SOURCEHOOK_OBJ_DIR)/%.o: $(SOURCEHOOK_SRC_DIR)/%.cpp
	$(DO_CC)

clean:
	-rm -Rf $(PLUGIN_OBJ_DIR)
	-rm -f $(NAME)_$(ARCH).$(SHLIBEXT) tier0_i486.so vstdlib_i486.so
