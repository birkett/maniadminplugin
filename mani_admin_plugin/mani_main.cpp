//
// Mani Admin Plugin
//
// Copyright (c) 2009 Giles Millward (Mani). All rights reserved.
//
// This file is part of ManiAdminPlugin.
//
// Mani Admin Plugin is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Mani Admin Plugin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Mani Admin Plugin.  If not, see <http://www.gnu.org/licenses/>.
//


//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifndef __linux__
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <winsock.h>

#endif
#include <mysql.h>

#ifdef __linux__
#include <dlfcn.h>
#else
typedef unsigned long DWORD;
#define PVFN2( classptr , offset ) ((*(DWORD*) classptr ) + offset)
#define VFN2( classptr , offset ) *(DWORD*)PVFN2( classptr , offset )
#endif
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "shake.h" 
#include "mrecipientfilter.h" 
//#include "enginecallback.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "bitbuf.h"
#include "icvar.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"

#include "mani_main.h"
#include "mani_timers.h"
#include "mani_player.h"
#include "mani_client_flags.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_memory.h"
#include "mani_parser.h"
#include "mani_convar.h"
#include "mani_client.h"
#include "mani_menu.h"
#include "mani_output.h"
#include "mani_panel.h"
#include "mani_quake.h"
#include "mani_crontab.h"
#include "mani_adverts.h"
#include "mani_maps.h"
#include "mani_stats.h"
#include "mani_replace.h"
#include "mani_effects.h"
#include "mani_sounds.h"
#include "mani_voice.h"
#include "mani_skins.h"
#include "mani_weapon.h"
#include "mani_team.h"
#include "mani_teamkill.h"
#include "mani_ghost.h"
#include "mani_customeffects.h"
#include "mani_mapadverts.h"
#include "mani_downloads.h"
#include "mani_victimstats.h"
#include "mani_database.h"
#include "mani_netidvalid.h"
#include "mani_autokickban.h"
#include "mani_reservedslot.h"
#include "mani_spawnpoints.h"
#include "mani_sprayremove.h"
#include "mani_chattrigger.h"
#include "mani_warmuptimer.h"
#include "mani_log_css_stats.h"
#include "mani_log_dods_stats.h"
#include "mani_mostdestructive.h"
#include "mani_css_objectives.h"
#include "mani_css_bounty.h"
#include "mani_css_betting.h"
#include "mani_trackuser.h"
#include "mani_anti_rejoin.h"
#include "mani_save_scores.h"
#include "mani_team_join.h"
#include "mani_mp_restartgame.h"
#include "mani_automap.h"
#include "mani_observer_track.h"
#include "mani_afk.h"
#include "mani_vote.h"
#include "mani_ping.h"
#include "mani_mysql.h"
#include "mani_vfuncs.h"
#include "mani_mainclass.h"
#include "mani_callback_sourcemm.h"
#include "mani_callback_valve.h"
#include "mani_sourcehook.h"
#include "mani_help.h"
#include "mani_commands.h"
#include "mani_vars.h"
#include "mani_sigscan.h"
#include "mani_client_sql.h"
#include "mani_globals.h"
#include "mani_util.h"

#include "shareddefs.h"
#include "cbaseentity.h"
#ifndef __linux__
#include "windows.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#include "beam_flags.h" 

int			menu_message_index = 10;
int			text_message_index = 5;
int			fade_message_index = 12;
int			vgui_message_index = 13;
int			saytext2_message_index = 4;
int			saytext_message_index = 3;
int			radiotext_message_index = 21;
int			hudMsg_message_index = 6;
int			hintMsg_message_index = 22;

int			tp_beam_index = 0;
int			plasmabeam_index = 0;
int			lgtning_index = 0;
int			explosion_index = 0;
int			orangelight_index = 0;
int			bluelight_index = 0;
int			purplelaser_index = 0;
int			spray_glow_index = 0; 

time_t	g_RealTime = 0;

// General sounds
char *menu_select_exit_sound="buttons/combine_button7.wav";
char *menu_select_sound="buttons/button14.wav";

CAdminPlugin g_ManiAdminPlugin;
CAdminPlugin *gpManiAdminPlugin;


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}


CONVAR_CALLBACK_PROTO (ManiAdminPluginVersion);
CONVAR_CALLBACK_PROTO (ManiTickrate);
CONVAR_CALLBACK_PROTO (WarModeChanged);
CONVAR_CALLBACK_PROTO (ManiStatsBySteamID);
CONVAR_CALLBACK_PROTO (ManiCSStackingNumLevels);
CONVAR_CALLBACK_PROTO (ManiUnlimitedGrenades);


ConVar mani_admin_plugin_version ("mani_admin_plugin_version", PLUGIN_CORE_VERSION, FCVAR_REPLICATED | FCVAR_NOTIFY, "This is the version of the plugin", CONVAR_CALLBACK_REF(ManiAdminPluginVersion)); 
ConVar mani_war_mode ("mani_war_mode", "0", 0, "This defines whether war mode is enabled or disabled (1 = enabled)", true, 0, true, 1, CONVAR_CALLBACK_REF(WarModeChanged)); 
ConVar mani_stats_by_steam_id ("mani_stats_by_steam_id", "1", 0, "This defines whether the steam id is used or name is used to organise the stats (1 = steam id)", true, 0, true, 1, CONVAR_CALLBACK_REF(ManiStatsBySteamID)); 
ConVar mani_tickrate ("mani_tickrate", "", FCVAR_REPLICATED | FCVAR_NOTIFY, "Server tickrate information", CONVAR_CALLBACK_REF(ManiTickrate));
ConVar mani_cs_stacking_num_levels ("mani_cs_stacking_num_levels", "1", 0, "Set number of players that can build a stack", true, 1, true, 50, CONVAR_CALLBACK_REF(ManiCSStackingNumLevels));
ConVar mani_unlimited_grenades ("mani_unlimited_grenades", "0", 0, "0 = normal CSS mode, 1 = Grenades replenished after throw (CSS Only)", true, 0, true, 1, CONVAR_CALLBACK_REF(ManiUnlimitedGrenades));
ConVar mani_show_events ("mani_show_events", "0", 0, "Shows events in server console, enabled or disabled (1 = enabled)", true, 0, true, 1); 

ConVar mani_exec_default_file1 ("mani_exec_default_file1", "mani_server.cfg", 0, "Run a default .cfg file on level change after server.cfg"); 
ConVar mani_exec_default_file2 ("mani_exec_default_file2", "./mani_admin_plugin/defaults.cfg", 0, "Run a default .cfg file on level change after server.cfg"); 
ConVar mani_exec_default_file3 ("mani_exec_default_file3", "", 0, "Run a default .cfg file on level change after server.cfg"); 
ConVar mani_exec_default_file4 ("mani_exec_default_file4", "", 0, "Run a default .cfg file on level change after server.cfg"); 
ConVar mani_exec_default_file5 ("mani_exec_default_file5", "", 0, "Run a default .cfg file on level change after server.cfg"); 

bool war_mode = false;
float	next_ping_check;
int	max_players = 0;
float			chat_flood[MANI_MAX_PLAYERS];

int				last_slapped_player;
float			last_slapped_time;
tw_spam_t		tw_spam_list[MANI_MAX_PLAYERS];

int				name_changes[MANI_MAX_PLAYERS];

name_change_t	user_name[MANI_MAX_PLAYERS];
team_scores_t	team_scores;
int				server_tickrate = 33;

bool		trigger_changemap;
float		trigger_changemap_time;
float		timeleft_offset;
bool		get_new_timeleft_offset;
bool		round_end_found;
bool		first_map_loaded = false;
char		custom_map_config[512]="";
char		map_type_config[512]="";
bool		just_loaded;
float		end_spawn_protection_time;
int			round_number;
float		round_start_time = 0;
rcon_t		*rcon_list;

swear_t		*swear_list;

cexec_t		*cexec_list;
cexec_t		*cexec_t_list;
cexec_t		*cexec_ct_list;
cexec_t		*cexec_spec_list;
cexec_t		*cexec_all_list;

int	rcon_list_size;
int	swear_list_size;

int	cexec_list_size;
int	cexec_t_list_size;
int	cexec_ct_list_size;
int	cexec_spec_list_size;
int	cexec_all_list_size;

float	last_cheat_check_time;

int	level_changed;
int	message_type;
float	test_val;


ConVar	*mp_friendlyfire = NULL; 
ConVar	*mp_freezetime = NULL; 
ConVar	*mp_winlimit = NULL; 
ConVar	*mp_maxrounds = NULL; 
ConVar	*mp_timelimit = NULL; 
ConVar	*mp_fraglimit = NULL; 
ConVar	*mp_limitteams = NULL; 
ConVar	*sv_lan = NULL; 
ConVar	*sv_cheats = NULL; 
ConVar	*sv_alltalk = NULL;  
ConVar	*mp_restartgame = NULL; 
ConVar	*sv_gravity = NULL; 
ConVar  *hostname = NULL; 
ConVar	*cs_stacking_num_levels = NULL; 
ConVar  *phy_pushscale = NULL; 
ConVar	*vip_version = NULL; 
ConVar	*mp_dynamicpricing = NULL;
ConVar	*tv_name = NULL;
ConVar	*mp_allowspectators = NULL;

//RenderMode_t mani_render_mode = kRenderNormal;

bf_write *msg_buffer;


// 
// The plugin is a static singleton that is exported as an interface
//
// Don't forget to make an instance


//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CAdminPlugin::CAdminPlugin()
{
	rcon_list = NULL;
	rcon_list_size = 0;

	round_number = 0;
	swear_list = NULL;
	swear_list_size = 0;
	target_player_list = NULL;
	target_player_list_size = 0;
	
	cexec_list = NULL;
	cexec_t_list = NULL;
	cexec_ct_list = NULL;
	cexec_spec_list = NULL;
	cexec_all_list = NULL;

	cexec_list_size = 0;
	cexec_t_list_size = 0;
	cexec_ct_list_size = 0;
	cexec_spec_list_size = 0;
	cexec_all_list_size = 0;

	war_mode = false;

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;

		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	next_ping_check = 0.0;
	message_type = 0;
	test_val = 0;
	just_loaded = true;
	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;

	timeleft_offset = 0;
	get_new_timeleft_offset = true;
	trigger_changemap = false;
	round_end_found = false;

	InitMaps();
	InitPlayerSettingsLists();
	gpManiIGELCallback = this;
	gpManiAdminPlugin = this;
	srand( (unsigned)time(NULL));

	event_duplicate = false;

	int ev_index = 0;

	// Create event hash table
	for (int i = 0; i < 256; i++)
	{
		event_table[i] = -1;
		event_fire[i].funcPtr = NULL;
		Q_strcpy(event_fire[i].event_name,"");
	}

	// Setup function pointers for event functions
	Q_strcpy(event_fire[ev_index].event_name, "player_hurt");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvPlayerHurt;

	Q_strcpy(event_fire[ev_index].event_name, "player_team");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvPlayerTeam;

	Q_strcpy(event_fire[ev_index].event_name, "player_death");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvPlayerDeath;

	Q_strcpy(event_fire[ev_index].event_name, "player_say");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvPlayerSay;

	Q_strcpy(event_fire[ev_index].event_name, "player_spawn");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvPlayerSpawn;

	Q_strcpy(event_fire[ev_index].event_name, "weapon_fire");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvWeaponFire;

	Q_strcpy(event_fire[ev_index].event_name, "bomb_planted");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombPlanted;

	Q_strcpy(event_fire[ev_index].event_name, "bomb_dropped");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombDropped;

	Q_strcpy(event_fire[ev_index].event_name, "bomb_exploded");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombExploded;

	Q_strcpy(event_fire[ev_index].event_name, "bomb_defused");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombDefused;
	
	Q_strcpy(event_fire[ev_index].event_name, "bomb_pickup");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombPickUp;

	Q_strcpy(event_fire[ev_index].event_name, "bomb_begindefuse");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvBombBeginDefuse;

	Q_strcpy(event_fire[ev_index].event_name, "hostage_stops_following");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvHostageStopsFollowing;

	Q_strcpy(event_fire[ev_index].event_name, "hostage_rescued");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvHostageRescued;

	Q_strcpy(event_fire[ev_index].event_name, "hostage_follows");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvHostageFollows;

	Q_strcpy(event_fire[ev_index].event_name, "hostage_killed");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvHostageKilled;

	Q_strcpy(event_fire[ev_index].event_name, "round_start");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvRoundStart;

	Q_strcpy(event_fire[ev_index].event_name, "round_end");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvRoundEnd;

	Q_strcpy(event_fire[ev_index].event_name, "round_freeze_end");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvRoundFreezeEnd;

	Q_strcpy(event_fire[ev_index].event_name, "vip_escaped");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvVIPEscaped;

	Q_strcpy(event_fire[ev_index].event_name, "vip_killed");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvVIPKilled;

	Q_strcpy(event_fire[ev_index].event_name, "dod_stats_weapon_attack");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodStatsWeaponAttack;

	Q_strcpy(event_fire[ev_index].event_name, "dod_point_captured");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodPointCaptured;

	Q_strcpy(event_fire[ev_index].event_name, "dod_capture_blocked");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodCaptureBlocked;

	Q_strcpy(event_fire[ev_index].event_name, "dod_round_win");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodRoundWin;

	Q_strcpy(event_fire[ev_index].event_name, "dod_stats_player_killed");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodStatsPlayerKilled;

	Q_strcpy(event_fire[ev_index].event_name, "dod_stats_player_damage");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodStatsPlayerDamage;

	Q_strcpy(event_fire[ev_index].event_name, "dod_game_over");
	event_fire[ev_index++].funcPtr = &CAdminPlugin::EvDodGameOver;

	max_events = ev_index;

	for (int i = 0; i < max_events; i++)
	{
		int index = this->GetEventIndex(event_fire[i].event_name, MANI_EVENT_HASH_SIZE);

		if (event_table[index] != -1)
		{
			event_duplicate = true;
		}

		event_table[index] = i;
	}

	// Setup randomish seed for rand() function
	srand( (unsigned)time( NULL ) );
}

CAdminPlugin::~CAdminPlugin()
{
	if (client_sql_manager)
	{
		client_sql_manager->Unload();
		delete client_sql_manager;
		client_sql_manager = NULL;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Get linear index of event name
//---------------------------------------------------------------------------------
int CAdminPlugin::GetEventIndex(const char *event_string, const int loop_length)
{
	int total = 0;
	int i = 0;

	for (i = 0; i < loop_length; i++)
	{
		if (event_string[i] == '\0')
		{
			break;
		}

//		if (event_string[i] == 'l')
//		{
//			total += 25;
//		}

		total += event_string[i];
	}

	return total & 0xff;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CAdminPlugin::Load(void)
{
	gpManiTrackUser->Load();
	if (!LoadLanguage())
	{
		return false;
	}

//	int ver = UTIL_GetWebVersion("75.126.15.16", 80, "/mani_admin_plugin/gametypes/gversion.dat");
//MMsg("Gametypes version [%i]\n", ver);

	gpManiGameType->Init();
	gpCmd->Load();
	g_menu_mgr.Load();

	// Load up game specific settings */
	/*
	gameclient = (IServerGameClients*) gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, NULL);
	if (!gameclient)
	{
		MMsg("Failed to load game client dll\n");
	}*/

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{

#ifdef __linux__
		void	*handle;
		void	*var_address;

		handle = dlopen(gpManiGameType->GetLinuxBin(), RTLD_NOW);
	
		if (handle == NULL)
		{
			MMsg("Failed to open server image, error [%s]\n", dlerror());
			gpManiGameType->SetAdvancedEffectsAllowed(false);
		}
		else
		{ 
			MMsg("Program Start at [%p]\n", handle);

			var_address = dlsym(handle, "te");
			if (var_address == NULL)
			{
				MMsg("dlsym failure : Error [%s]\n", dlerror());
				gpManiGameType->SetAdvancedEffectsAllowed(false);
			}
			else
			{
				MMsg("var_address = %p\n", var_address);
				temp_ents = *(ITempEntsSystem **) var_address;
			}

			dlclose(handle);
		}
#else
		temp_ents = **(ITempEntsSystem***)(VFN2(effects, gpManiGameType->GetAdvancedEffectsVFuncOffset()) + (gpManiGameType->GetAdvancedEffectsCodeOffset()));
#endif
	}

	const char *game_type = serverdll->GetGameDescription();

	MMsg("Game Type [%s]\n", game_type);

	gpManiVictimStats->RoundStart();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;
		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	next_ping_check = 0.0;
	mp_friendlyfire = g_pCVar->FindVar( "mp_friendlyfire");
	mp_freezetime = g_pCVar->FindVar( "mp_freezetime");
	mp_winlimit = g_pCVar->FindVar( "mp_winlimit");
	mp_maxrounds = g_pCVar->FindVar( "mp_maxrounds");
	mp_timelimit = g_pCVar->FindVar( "mp_timelimit");
	mp_fraglimit = g_pCVar->FindVar( "mp_fraglimit");
	mp_limitteams = g_pCVar->FindVar( "mp_limitteams");
	mp_restartgame = g_pCVar->FindVar( "mp_restartgame");
	mp_dynamicpricing = g_pCVar->FindVar( "mp_dynamicpricing");
	
	if (mp_dynamicpricing)
	{
		mp_dynamicpricing->AddFlags(FCVAR_REPLICATED | FCVAR_NOTIFY);
	}

	sv_lan = g_pCVar->FindVar( "sv_lan");
	sv_gravity = g_pCVar->FindVar( "sv_gravity");
	cs_stacking_num_levels = g_pCVar->FindVar( "cs_stacking_num_levels");

	sv_cheats = g_pCVar->FindVar( "sv_cheats");
	sv_alltalk = g_pCVar->FindVar( "sv_alltalk");
	hostname = g_pCVar->FindVar( "hostname");
	phy_pushscale = g_pCVar->FindVar( "phys_pushscale");
	vip_version = g_pCVar->FindVar("vip_version");
	tv_name = g_pCVar->FindVar("tv_name");
	mp_allowspectators = g_pCVar->FindVar("mp_allowspectators");

	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;
	trigger_changemap = false;
	Q_strcpy(custom_map_config,"");
	Q_strcpy(map_type_config,"");

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	char	message_name[1024];
	int	Msg_size;


	message_type = 0;
/*	while (serverdll->GetUserMessageInfo(message_type, message_name, sizeof(message_name), (int &) Msg_size))
	{
		MMsg("Msg [%i] Name [%s] size[%i]\n", message_type, message_name, Msg_size);
		message_type ++;
		if (message_type > 20)
		{
			while(1);
		}
	}
*/

	for (int i = 0; i < gpManiGameType->GetMaxMessages(); i++)
	{
		serverdll->GetUserMessageInfo(i, message_name, sizeof(message_name), (int &) Msg_size);
		MMsg("Message name %s index %i\n", message_name, i);
		if (FStrEq(message_name, "ShowMenu")) 
		{
			menu_message_index = i;
//			MMsg("Hooked ShowMenu [%i]\n", i);
		}
		else if (FStrEq(message_name, "TextMsg"))
		{
			text_message_index = i;
//			MMsg("Hooked TextMsg [%i]\n", i);
		}
		else if (FStrEq(message_name, "Fade"))
		{
			fade_message_index = i;
//			MMsg("Hooked Fade [%i]\n", i);
		}
		else if (FStrEq(message_name, "VGUIMenu"))
		{
			vgui_message_index = i;
//			MMsg("Hooked VGUIMenu [%i]\n", i);
		}
		else if (FStrEq(message_name, "SayText2"))
		{
			saytext2_message_index = i;
	//		MMsg("Hooked SayText2 [%i]\n", i);
		}
		else if (FStrEq(message_name, "SayText"))
		{
			saytext_message_index = i;
//			MMsg("Hooked SayText [%i]\n", i);
		}
		else if (FStrEq(message_name, "RadioText"))
		{
			radiotext_message_index = i;
//			MMsg("Hooked RadioText [%i]\n", i);
		}
		else if (FStrEq(message_name, "HudMsg"))
		{
			hudMsg_message_index = i;
//			MMsg("Hooked HudMsg [%i]\n", i);
		}
		else if (FStrEq(message_name, "HintText"))
		{
			hintMsg_message_index = i;
//			MMsg("Hooked HintMsg [%i]\n", i);
		}
	}

	timeleft_offset = 0;
	get_new_timeleft_offset = false;
	round_end_found = false;

	LoadSigScans();

	SetPluginPausedStatus(false);
	gpManiDatabase->Init();

	// Fire up the database
	client_sql_manager = new SQLManager(1, 100);
	client_sql_manager->Load();

	InitEffects();
	InitTKPunishments();
	gpManiDownloads->Init();
	gpManiReservedSlot->Load();
	gpManiAutoKickBan->Load();
	gpManiChatTriggers->Load();
	gpManiPing->Load();
	gpManiVote->Load();
	gpManiCSSBounty->Load();
	gpManiCSSBetting->Load();
	gpManiObserverTrack->Load();

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiWeaponMgr->Load();
		gpManiLogCSSStats->Load();
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		gpManiLogDODSStats->Load();
	}

	gpManiSaveScores->Load();
	gpManiTeamJoin->Load();
	gpManiStats->Load();
	gpManiAFK->Load();
	gpManiNetIDValid->Load();
	gpManiClient->Init();
	gpManiAntiRejoin->Load();

	if (first_map_loaded)
	{
		InitPanels();	// Fails if plugin loaded on server startup but placed here in case user runs plugin_load
		ResetActivePlayers();
		LoadQuakeSounds();
		LoadCronTabs();
		LoadAdverts();
		LoadMaps("Unknown"); //host_maps cvar is borked :/
		LoadWebShortcuts();
		ResetLogCount();
		LoadCommandList();
		LoadSounds();
		LoadSkins();
		SkinResetTeamID();
		FreeTKPunishments();
		gpManiGhost->Init();
		gpManiCustomEffects->Init();
		gpManiMapAdverts->Init();
		gpManiReservedSlot->LevelInit();
		gpManiAutoKickBan->LevelInit();
		gpManiSpawnPoints->Load(current_map);
		gpManiSprayRemove->Load();
	}

	gpManiAutoMap->Load();

	filesystem->CreateDirHierarchy( "./cfg/mani_admin_plugin/data/");

	// Work out server tickrate
#ifdef __linux__
	server_tickrate = (int) ((1.0 / serverdll->GetTickInterval()) + 0.5);
#else
	server_tickrate = (int) (1.0 / serverdll->GetTickInterval());
#endif
MMsg("Here");
	mani_tickrate.SetValue(server_tickrate);

MMsg("Here2");
	// Hook our changelevel here
	//HOOKVFUNC(engine, 0, OrgEngineChangeLevel, ManiChangeLevelHook);
	//HOOKVFUNC(engine, 43, OrgEngineEntityMessageBegin, ManiEntityMessageBeginHook);
	//HOOKVFUNC(engine, 44, OrgEngineMessageEnd, ManiMessageEndHook);
	g_ManiSMMHooks.HookVFuncs();

	g_PluginLoaded = true;
	g_PluginLoadedOnce = true;

	this->InitEvents();
	gpManiMPRestartGame->Load();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CAdminPlugin::InitEvents( void )
{
	gameeventmanager->AddListener( gpManiIGELCallback, "player_hurt", true );
	gameeventmanager->AddListener( gpManiIGELCallback, "player_team", true );
	gameeventmanager->AddListener( gpManiIGELCallback, "player_death", true );
	gameeventmanager->AddListener( gpManiIGELCallback, "player_say", true );
	gameeventmanager->AddListener( gpManiIGELCallback, "player_spawn", true );

	if (gpManiGameType->IsGameType(MANI_GAME_CSS)) 
	{
		gameeventmanager->AddListener( gpManiIGELCallback, "weapon_fire", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "hostage_stops_following", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_planted", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_dropped", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_exploded", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_defused", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_begindefuse", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "bomb_pickup", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "hostage_rescued", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "hostage_follows", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "hostage_killed", true);
		gameeventmanager->AddListener( gpManiIGELCallback, "round_start", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "round_end", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "round_freeze_end", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "vip_escaped", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "vip_killed", true );
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_stats_weapon_attack", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_point_captured", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_capture_blocked", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_round_win", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_stats_player_killed", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_stats_player_damage", true );
		gameeventmanager->AddListener( gpManiIGELCallback, "dod_game_over", true );
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CAdminPlugin::Unload( void )
{
	// Free up any lists being used
	gpManiTrackUser->Unload();

	g_menu_mgr.Unload();

	FreeCronTabs();
	FreeAdverts();
	FreeWebShortcuts();
	FreeMaps();
	FreePlayerSettings();
	FreePlayerNameSettings();
	FreeCommandList();
	FreeSounds();
	FreeSkins();
	gpManiTeam->UnLoad();
	FreeTKPunishments();

	if (client_sql_manager)
	{
		client_sql_manager->Unload();
		delete client_sql_manager;
		client_sql_manager = NULL;
	}

	FreeList((void **) &rcon_list, &rcon_list_size);

	FreeList((void **) &tk_player_list, &tk_player_list_size);
	FreeList((void **) &swear_list, &swear_list_size);

	FreeList((void **) &cexec_list, &cexec_list_size);
	FreeList((void **) &cexec_t_list, &cexec_t_list_size);
	FreeList((void **) &cexec_ct_list, &cexec_ct_list_size);
	FreeList((void **) &cexec_spec_list, &cexec_spec_list_size);
	FreeList((void **) &cexec_all_list, &cexec_all_list_size);
	FreeList((void **) &target_player_list, &target_player_list_size);

	trigger_changemap = false;
	FreeLanguage();
	g_PluginLoaded = false;
	gpManiStats->Unload();
	gpManiTeamJoin->Unload();
	gpManiAFK->Unload();
	gpManiSaveScores->Unload();
	gpManiVote->Unload();
	gameeventmanager->RemoveListener(gpManiIGELCallback);
	gpManiAutoMap->Unload();
	gpManiAntiRejoin->Unload();
	gpManiMPRestartGame->Unload();
	gpManiObserverTrack->Unload();

}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CAdminPlugin::Pause( void )
{
	SayToAll(GREEN_CHAT, true, "Mani Admin Plugin is paused");
	DirectLogCommand("[MANI_ADMIN_PLUGIN] Mani Admin Plugin is paused\n");
	SetPluginPausedStatus(true);
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CAdminPlugin::UnPause( void )
{
	SayToAll(GREEN_CHAT, true, "Mani Admin Plugin is un-paused");
	DirectLogCommand("[MANI_ADMIN_PLUGIN] Mani Admin Plugin is un-paused\n");
	SetPluginPausedStatus(false);
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CAdminPlugin::GetPluginDescription( void )
{
	return mani_version;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::LevelInit( char const *pMapName )
{
	FileHandle_t file_handle;
	char	swear_word[128];
	char	rcon_command[512];
	int		i;
	char	map_config_filename[256];
	char	base_filename[256];
	char	alias_command[512];
	int		total_load_index;

	MMsg("********************************************************\n");
	MMsg("************* Mani Admin Plugin Level Init *************\n");
	MMsg("********************************************************\n");

	gpManiTrackUser->LevelInit();
	g_menu_mgr.LevelInit();

	// Reset game type info (mp_teamplay may have changed)
	gpManiGameType->Init();
	total_load_index = ManiGetTimer();

//	filesystem->PrintSearchPaths();

//	MMsg("mani_path = [%s]\n", mani_path.GetString());

	InitPanels();

	FreeList((void **) &rcon_list, &rcon_list_size);

	FreeList((void **) &tk_player_list, &tk_player_list_size);
	FreeList((void **) &swear_list, &swear_list_size);

	FreeList((void **) &cexec_list, &cexec_list_size);
	FreeList((void **) &cexec_t_list, &cexec_t_list_size);
	FreeList((void **) &cexec_ct_list, &cexec_ct_list_size);
	FreeList((void **) &cexec_spec_list, &cexec_spec_list_size);
	FreeList((void **) &cexec_all_list, &cexec_all_list_size);
	FreeList((void **) &target_player_list, &target_player_list_size);

	mp_friendlyfire = g_pCVar->FindVar( "mp_friendlyfire");
	mp_freezetime = g_pCVar->FindVar( "mp_freezetime");
	mp_winlimit = g_pCVar->FindVar( "mp_winlimit");
	mp_maxrounds = g_pCVar->FindVar( "mp_maxrounds");
	mp_timelimit = g_pCVar->FindVar( "mp_timelimit");
	mp_fraglimit = g_pCVar->FindVar( "mp_fraglimit");
	mp_limitteams = g_pCVar->FindVar( "mp_limitteams");
	mp_restartgame = g_pCVar->FindVar( "mp_restartgame");
	mp_dynamicpricing = g_pCVar->FindVar( "mp_dynamicpricing");
	if (mp_dynamicpricing)
	{
		mp_dynamicpricing->AddFlags(FCVAR_REPLICATED | FCVAR_NOTIFY);
	}

	sv_lan = g_pCVar->FindVar( "sv_lan");
	sv_gravity = g_pCVar->FindVar( "sv_gravity");
	cs_stacking_num_levels = g_pCVar->FindVar( "cs_stacking_num_levels");

	sv_cheats = g_pCVar->FindVar( "sv_cheats");
	sv_alltalk = g_pCVar->FindVar( "sv_alltalk");
	hostname = g_pCVar->FindVar( "hostname");
	phy_pushscale = g_pCVar->FindVar( "phys_pushscale");
	vip_version = g_pCVar->FindVar("vip_version");
	tv_name = g_pCVar->FindVar("tv_name");
	mp_allowspectators = g_pCVar->FindVar("mp_allowspectators");

	next_ping_check = 0.0;
	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;
	timeleft_offset = 0;
	get_new_timeleft_offset = true;
	trigger_changemap = false;
	round_end_found = false;

	// Set plugin defaults in case they have been changed
	round_number = 0;

	// Used for map config
	level_changed = true;

	for (i = 0; i < MANI_MAX_TEAMS; i++)
	{
		team_scores.team_score[i] = 0;
	}

	gpManiNetIDValid->LevelInit();

	InitEffects();
	SkinResetTeamID();
	gpManiDownloads->Init();
	gpManiGhost->Init();
	gpManiCustomEffects->Init();
	gpManiVictimStats->RoundStart();
	gpManiChatTriggers->LevelInit();
	gpManiWarmupTimer->LevelInit();
	gpManiSaveScores->LevelInit();
	gpManiTeamJoin->LevelInit();
	gpManiAFK->LevelInit();
	gpManiPing->LevelInit();
	gpManiCSSBounty->LevelInit();
	gpManiCSSBetting->LevelInit();
	gpManiObserverTrack->LevelInit();

	InitTKPunishments();

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->LevelInit();
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		gpManiLogDODSStats->LevelInit();
	}

	// Init votes and menu system
	for (i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;
		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		esounds->PrecacheSound(slay_sound_name, true);
		esounds->PrecacheSound(slap_sound_name[0].sound_name, true);
		esounds->PrecacheSound(slap_sound_name[1].sound_name, true);
		esounds->PrecacheSound(slap_sound_name[2].sound_name, true);
	}
	else
	{
		esounds->PrecacheSound(hl2mp_slay_sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[0].sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[1].sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[2].sound_name, true);
	}

	// Stuff for time bomb
	esounds->PrecacheSound(countdown_beep, true);
	esounds->PrecacheSound(final_beep, true);

	// Beacon sound
	esounds->PrecacheSound(beacon_sound, true);

	// Menu select noises
	esounds->PrecacheSound(menu_select_sound, true);
	esounds->PrecacheSound(menu_select_exit_sound, true);

	tp_beam_index = engine->PrecacheModel( "sprites/tp_beam001.vmt", true );
	plasmabeam_index = engine->PrecacheModel( "sprites/plasmabeam.vmt", true );
	lgtning_index = engine->PrecacheModel( "sprites/lgtning.vmt", true );
	explosion_index = engine->PrecacheModel( "sprites/sprite_fire01.vmt", true );
	orangelight_index = engine->PrecacheModel( "sprites/orangelight1.vmt", true );
	bluelight_index = engine->PrecacheModel( "sprites/bluelight1.vmt", true );
	purplelaser_index = engine->PrecacheModel( "sprites/purplelaser1.vmt", true );
	spray_glow_index = engine->PrecacheModel( "sprites/blueglow2.vmt", true );

	if (!first_map_loaded)
	{
		LoadLanguage();
		first_map_loaded = true;
		ResetLogCount();
	}


	gpManiClient->Init();
	LoadQuakeSounds();
	LoadCronTabs();
	LoadAdverts();
	LoadMaps(pMapName);
	LoadWebShortcuts();
	LoadCommandList();
	LoadSounds();
	LoadSkins();
	FreeTKPunishments();
	gpManiMapAdverts->Init();
	gpManiAutoKickBan->LevelInit();
	gpManiReservedSlot->LevelInit();
	gpManiSpawnPoints->LevelInit(current_map);
	gpManiSprayRemove->LevelInit();
	gpManiAntiRejoin->LevelInit();

	//Get swear word list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/wordfilter.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load wordfilter.txt\n");
	}
	else
	{
//		MMsg("Swearword list\n");
		while (filesystem->ReadLine (swear_word, sizeof(swear_word), file_handle) != NULL)
		{
			if (!ParseLine(swear_word, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &swear_list, sizeof(swear_t), &swear_list_size);
			// Convert to upper case
			Q_strupr(swear_word);
			Q_strcpy(swear_list[swear_list_size - 1].swear_word, swear_word);
			swear_list[swear_list_size - 1].length = Q_strlen(swear_word);
//			MMsg("[%s] ", swear_word);
		}

//		MMsg("\n");
		filesystem->Close(file_handle);
	}

	//Get Map Config for this map
	snprintf( map_config_filename, sizeof(map_config_filename), "./cfg/%s/map_config/%s.cfg", mani_path.GetString(), pMapName);
	if (filesystem->FileExists(map_config_filename))
	{
		snprintf(custom_map_config, sizeof(custom_map_config), "exec ./%s/map_config/%s.cfg\n", mani_path.GetString(), pMapName);
//		MMsg("Custom map config [%s] found for this map\n", custom_map_config);
	}
	else
	{
//		MMsg("No custom map config found for this map\n");
		Q_strcpy(custom_map_config,"");
	}

	char map_type[512] = "";
	int  char_index = 0;

	while (pMapName[char_index] != '\0')
	{
		if (pMapName[char_index] == '_')
		{
			map_type[char_index] = '\0';
			break;
		}

		char_index ++;
	}


	if (pMapName[char_index] == '\0')
	{
		Q_strcpy(map_type_config,"");
	}
	else
	{
		snprintf( map_config_filename, sizeof(map_config_filename), "./cfg/%s/map_config/%s_.cfg", mani_path.GetString(), map_type);
		if (filesystem->FileExists(map_config_filename))
		{
			snprintf(map_type_config, sizeof(map_type_config), "exec ./%s/map_config/%s_.cfg\n", mani_path.GetString(), map_type);
		}
		else
		{
			Q_strcpy(map_type_config,"");
		}
	}
	
	//Get rcon list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/rconlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load rconlist.txt\n");
	}
	else
	{
//		MMsg("rcon list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &rcon_list, sizeof(rcon_t), &rcon_list_size);
			Q_strcpy(rcon_list[rcon_list_size - 1].rcon_command, rcon_command);
			Q_strcpy(rcon_list[rcon_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get single player cexec list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_player.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load cexeclist_player.txt\n");
	}
	else
	{
//		MMsg("cexeclist_player list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_list, sizeof(cexec_t), &cexec_list_size);
			Q_strcpy(cexec_list[cexec_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_list[cexec_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get all cexec list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_all.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load cexeclist_all.txt\n");
	}
	else
	{
//		MMsg("cexeclist_all list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_all_list, sizeof(cexec_t), &cexec_all_list_size);
			Q_strcpy(cexec_all_list[cexec_all_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_all_list[cexec_all_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get t cexec list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_t.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load cexeclist_t.txt\n");
	}
	else
	{
//		MMsg("cexeclist_t list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_t_list, sizeof(cexec_t), &cexec_t_list_size);
			Q_strcpy(cexec_t_list[cexec_t_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_t_list[cexec_t_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get ct cexec list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_ct.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load cexeclist_ct.txt\n");
	}
	else
	{
//		MMsg("cexeclist_ct list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_ct_list, sizeof(cexec_t), &cexec_ct_list_size);
			Q_strcpy(cexec_ct_list[cexec_ct_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_ct_list[cexec_ct_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get spec cexec list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_spec.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load cexeclist_spec.txt\n");
	}
	else
	{
//		MMsg("cexeclist_spec list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_spec_list, sizeof(cexec_t), &cexec_spec_list_size);
			Q_strcpy(cexec_spec_list[cexec_spec_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_spec_list[cexec_spec_list_size - 1].alias, alias_command);
//			MMsg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	gpManiVote->LevelInit();
	int timer_index = ManiGetTimer();
	gpManiStats->LevelInit(pMapName);
	MMsg("Stats Loaded in %.4f seconds\n", ManiGetTimerDuration(timer_index));

	just_loaded = false;
	gpManiVote->ProcessBuildUserVoteMaps();

	timer_index = ManiGetTimer();
	ProcessPlayerSettings();
	MMsg("Player Lists Loaded in %.4f seconds\n", ManiGetTimerDuration(timer_index));
	gpManiAutoMap->LevelInit();
	gpManiMPRestartGame->LevelInit();
	this->InitEvents();
	time_t current_time;

	time(&current_time);

//	int day = current_time / (60 * 60 * 24);

//	MMsg("time = [%i] days = [%i]\n", current_time, day);

	MMsg("********************************************************\n");
	MMsg(" Mani Admin Plugin Level Init Time = %.3f seconds\n", ManiGetTimerDuration(total_load_index));
	MMsg("********************************************************\n");
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CAdminPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	max_players = clientMax; 

	// Get team manager pointers
	gpManiTeam->Init(edictCount);
	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiWeaponMgr->LevelInit();
	}

	char	file_execute[512]="";
	snprintf(file_execute, sizeof(file_execute), "exec \"%s\"\n", mani_exec_default_file1.GetString());
	engine->ServerCommand(file_execute);
	snprintf(file_execute, sizeof(file_execute), "exec \"%s\"\n", mani_exec_default_file2.GetString());
	engine->ServerCommand(file_execute);
	snprintf(file_execute, sizeof(file_execute), "exec \"%s\"\n", mani_exec_default_file3.GetString());
	engine->ServerCommand(file_execute);
	snprintf(file_execute, sizeof(file_execute), "exec \"%s\"\n", mani_exec_default_file4.GetString());
	engine->ServerCommand(file_execute);
	snprintf(file_execute, sizeof(file_execute), "exec \"%s\"\n", mani_exec_default_file5.GetString());
	engine->ServerCommand(file_execute);
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CAdminPlugin::GameFrame( bool simulating )
{
	if (ProcessPluginPaused()) return;
 
	if (event_duplicate)
	{
		// Event hash table is broken !!!
		MMsg("MANI ADMIN PLUGIN - Event Hash table has duplicates!\n");
	}

	time(&g_RealTime);

	g_menu_mgr.GameFrame();
	gpManiGameType->GameFrame();
	LanguageGameFrame();

	// Simulate NetworkIDValidate
	gpManiNetIDValid->GameFrame();

	if (client_sql_manager)
	{
		client_sql_manager->GameFrame();
	}

	gpManiSprayRemove->GameFrame();
	gpManiWarmupTimer->GameFrame();
	gpManiAFK->GameFrame();
	gpManiPing->GameFrame();

	if (war_mode && mani_war_mode_force_overview_zero.GetInt() == 1)
	{
		TurnOffOverviewMap();
	}

	gpManiStats->GameFrame();

	if (war_mode) return;

	gpManiTeam->GameFrame();

	ProcessAdverts();
	ProcessInGamePunishments();

	if (trigger_changemap && gpGlobals->curtime >= trigger_changemap_time)
	{
		char	server_cmd[512];
		
		trigger_changemap = false;
//		engine->ChangeLevel(next_map, NULL);
		snprintf(server_cmd, sizeof(server_cmd), "changelevel %s\n", next_map);
		engine->ServerCommand(server_cmd);
	}

	gpManiVote->GameFrame();
	gpManiAutoMap->GameFrame();

}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CAdminPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gpManiStats->LevelShutdown();
	gpManiSpawnPoints->LevelShutdown();
	gpManiAFK->LevelShutdown();
	gpManiWeaponMgr->LevelShutdown();
	gameeventmanager->RemoveListener(gpManiIGELCallback);
	gpManiMPRestartGame->LevelShutdown();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientActive( edict_t *pEntity )
{
 // MMsg("Mani -> Client Active\n");

	gpManiTrackUser->ClientActive(pEntity);

	if (ProcessPluginPaused())
	{
		return;
	}

	player_t player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return;
	if (!player.player_info->IsHLTV())
	{
		g_menu_mgr.ClientActive(&player);
	}

	if (player.player_info->IsHLTV()) return;
	gpManiNetIDValid->ClientActive(&player);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->ClientActive(&player);
	}

	if (FStrEq(player.player_info->GetNetworkIDString(),"BOT")) return;

	gpManiGhost->ClientActive(&player);
	gpManiVictimStats->ClientActive(&player);
	gpManiMapAdverts->ClientActive(&player);
	gpManiCSSBounty->ClientActive(&player);
	gpManiWeaponMgr->ClientActive(&player);

	if (!player.is_bot)
	{
		user_name[player.index - 1].in_use = true;
		Q_strcpy(user_name[player.index - 1].name, engine->GetClientConVarValue(player.index, "name"));

		// Player settings
		PlayerJoinedInitSettings(&player);
	}


	if (!player.is_bot)
	{
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_JOINSERVER);
	}
}



//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientDisconnect( edict_t *pEntity )
{
	player_t player;

	player.entity = pEntity;

	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	g_menu_mgr.ClientDisconnect(&player);
	gpManiNetIDValid->ClientDisconnect(&player);
	gpManiSprayRemove->ClientDisconnect(&player);
	gpManiSaveScores->ClientDisconnect(&player);
	gpManiAFK->ClientDisconnect(&player);
	gpManiPing->ClientDisconnect(&player);
	gpManiCSSBounty->ClientDisconnect(&player);
	gpManiCSSBetting->ClientDisconnect(&player);
	gpManiAntiRejoin->ClientDisconnect(&player);
	gpManiObserverTrack->ClientDisconnect(&player);


	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->ClientDisconnect(&player);
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		gpManiLogDODSStats->ClientDisconnect(&player);
	}

	// Handle player settings
	PlayerSettingsDisconnect(&player);
	SkinPlayerDisconnect(&player);
	gpManiGhost->ClientDisconnect(&player);
	gpManiVictimStats->ClientDisconnect(&player);
	gpManiClient->ClientDisconnect(&player);
	gpManiStats->ClientDisconnect(&player);
	gpManiWeaponMgr->ClientDisconnect(&player);

	user_name[player.index - 1].in_use = false;
	Q_strcpy(user_name[player.index - 1].name,"");
	name_changes[player.index - 1] = 0;

	if (player.is_bot)
	{
		gpManiTrackUser->ClientDisconnect(&player);
		return;
	}

	EffectsClientDisconnect(player.index - 1, false);

	if (mani_tk_protection.GetInt() == 1)
	{
		if (IsLAN())
		{
			// Reset user id if in list 
			for (int i = 0; i < tk_player_list_size; i++)
			{
				if (tk_player_list[i].user_id == player.user_id)
				{
					tk_player_t temp_tk_player;
					tk_player_t *temp_tk_player_ptr;

					if (i != tk_player_list_size - 1)
					{
						temp_tk_player = tk_player_list[tk_player_list_size - 1];
						tk_player_list[tk_player_list_size - 1] = tk_player_list[i];
						tk_player_list[i] = temp_tk_player;
					}

					if (tk_player_list_size != 1)
					{
						// Shrink array by 1
						temp_tk_player_ptr = (tk_player_t *) realloc(tk_player_list, (tk_player_list_size - 1) * sizeof(tk_player_t));
						tk_player_list = temp_tk_player_ptr;
						tk_player_list_size --;
					}
					else
					{
						free(tk_player_list);
						tk_player_list = NULL;
						tk_player_list_size = 0;
					}

					break;
				}
			}
		}
	}

	gpManiVote->ClientDisconnect(&player);
	gpManiTrackUser->ClientDisconnect(&player);
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
//  MMsg("Client [%s] Put in Server\n",playername );

	if (level_changed)
	{
		level_changed = false;
		
// SetupTeamList(engine->GetEntityCount());

		if (ProcessPluginPaused())	return;

		if (mani_tk_protection.GetInt() == 1 && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			// Force tk punish off (required for plugin tk punishment)
			engine->ServerCommand("mp_tkpunish 0\n");
			if (mani_tk_spawn_time.GetInt() != 0)
			{
				engine->ServerCommand("mp_spawnprotectiontime 0\n");
			}
		}

		if (mani_afk_kicker.GetInt() != 0)
		{
			// Force mp_autokick 0
			engine->ServerCommand("mp_autokick 0\n");
		}

		// Execute any pre-map configs
		ExecuteCronTabs(false);

		if (map_type_config)
		{
			if (!FStrEq(map_type_config,""))
			{
				MMsg("Executing %s\n", map_type_config);
				engine->ServerCommand(map_type_config);
			}
		}

		if (custom_map_config)
		{
			if (!FStrEq(custom_map_config,""))
			{
				MMsg("Executing %s\n", custom_map_config);
				engine->ServerCommand(custom_map_config);
			}
		}
 
		// Execute any post-map configs
		ExecuteCronTabs(true);

		if (mp_dynamicpricing)
		{
			mp_dynamicpricing->AddFlags(FCVAR_REPLICATED | FCVAR_NOTIFY);
		}
	}	
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	if ( playerinfomanager ) 
	{
		int	player_index = engine->IndexOfEdict(pEdict);

		if (user_name[player_index - 1].in_use)
		{
			const char * name = engine->GetClientConVarValue(player_index, "name" );
			if (strcmp(user_name[player_index - 1].name, name) != 0)
			{
				player_t player;
				player.index = player_index;
				if (FindPlayerByIndex(&player))
				{
					if (!player.is_bot)
					{
						// Handle name change
						PlayerJoinedInitSettings(&player);
						ProcessChangeName(&player, name, user_name[player_index - 1].name);
						Q_strcpy(user_name[player_index - 1].name, name);
					}
				}
			}
		}

	}
	return;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
//	if (!ProcessPluginPaused())
//	{
//		MMsg("Mani -> Client Connected [%s] [%s]\n", pszName, engine->GetPlayerNetworkIDString( pEntity ));
//	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
#ifdef ORANGE
PLUGIN_RESULT	CAdminPlugin::ClientCommand( edict_t *pEntity,  const CCommand &args)
#else
PLUGIN_RESULT	CAdminPlugin::ClientCommand( edict_t *pEntity )
#endif
{
#ifndef ORANGE 
	CCommand args;
#endif

	if (ProcessPluginPaused())
	{
		return PLUGIN_CONTINUE;
	}

	player_t	player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
	gpCmd->ExtractClientAndServerCommand(args);

	int	pargc = gpCmd->Cmd_Argc();
	const char *pcmd = gpCmd->Cmd_Argv(0);
	const char *pcmd2 = gpCmd->Cmd_Argv(1);

	if (pargc == 2 && 
		(strcmp(pcmd, "menuselect") == 0 || 
		strcmp(pcmd, "ma_escselect") == 0))
	{
		if (pargc != 2)	return PLUGIN_STOP;

		bool esc_style = false;
		int selected_index = atoi(pcmd2);
		if (strcmp(pcmd, "ma_escselect") == 0)
		{
			esc_style = true;
		}

		g_menu_mgr.OptionSelected(&player, selected_index);
		if (esc_style)
		{
			// Escape style commands are custom so stop them
			return PLUGIN_STOP;
		}

		// Let standard menu command through for other plugins
		return PLUGIN_CONTINUE;
	}

	if (strcmp(pcmd, "ma_escinput") == 0)
	{
		if (pargc == 1) return PLUGIN_STOP;

		g_menu_mgr.OptionSelected(&player, 1337);
		return PLUGIN_STOP;
	}

	if (strcmp(pcmd, "ma_menurepop") == 0)
	{
		g_menu_mgr.RepopulatePage(&player);
		return PLUGIN_STOP;
	}

	if (gpCmd->HandleCommand(&player, M_CCONSOLE, args) == PLUGIN_STOP) return PLUGIN_STOP;
	else if ( FStrEq( pcmd, "jointeam")) return (gpManiTeamJoin->PlayerJoin(pEntity, (char *) gpCmd->Cmd_Argv(1)));
	else if ( FStrEq( pcmd, "joinclass")) return (gpManiWarmupTimer->JoinClass(pEntity));
	else if ( FStrEq( pcmd, "admin" )) 
	{
		MENUPAGE_CREATE_FIRST(PrimaryMenuPage, &player, 0, -1);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "settings"))
	{
		MENUPAGE_CREATE_FIRST(PlayerSettingsPage, &player, 0, -1);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "votemap" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_map.GetInt() == 0) return PLUGIN_CONTINUE;
		MENUPAGE_CREATE_FIRST(UserVoteMapPage, &player, 0, -1);
		return PLUGIN_STOP;
	}	
	else if ( FStrEq( pcmd, "votekick" ) && !war_mode && !IsLAN())
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_kick.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_kick_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaUserVoteKick(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "voteban" ) && !war_mode && !IsLAN())
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_ban.GetInt() == 0) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_ban_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaUserVoteBan(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaRockTheVoteNominateMap(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "favourites" ) && !war_mode)
	{
		MENUPAGE_CREATE_FIRST(FavouritesPage, &player, 0, -1);
		return PLUGIN_STOP;
	}
	else if ( FStrEq(pcmd, "buy"))	return (gpManiWeaponMgr->CanBuy(&player, pcmd2));
	else if ( FStrEq( pcmd, "nextmap" )) return (ProcessMaNextMap(&player, "nextmap", 0, M_CCONSOLE));
	else if ( FStrEq( pcmd, "timeleft" )) return (gpManiAdminPlugin->ProcessMaTimeLeft(&player, "timeleft", 0, M_CCONSOLE));
	else if ( FStrEq( pcmd, "listmaps" )) return (ProcessMaListMaps(&player, "listmaps", 0, M_CCONSOLE));
	else if ( FStrEq( pcmd, "damage" )) return (ProcessMaDamage(player.index));
	else if ( FStrEq( pcmd, "deathbeam" )) return (ProcessMaDeathBeam(player.index));
	else if ( FStrEq( pcmd, "sounds" )) return (ProcessMaSounds(player.index));
	else if ( FStrEq( pcmd, "quake" )) return (ProcessMaQuake(player.index));	
	else if ( FStrEq( pcmd, "ma_version" ))
	{
		PrintToClientConsole(pEntity, "%s\n", mani_version);
		PrintToClientConsole(pEntity, "Server Tickrate %i\n", server_tickrate);

		if (mani_dead_alltalk.GetInt() == 1)
		{
			PrintToClientConsole(pEntity, "Dead AllTalk is On\n");
		}

		if (war_mode)
		{
			PrintToClientConsole(pEntity, "War mode enabled\n");
		}

#ifdef WIN32
		PrintToClientConsole(pEntity, "Windows server\n");
#else
		PrintToClientConsole(pEntity, "Linux server\n");
#endif
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_votemenu" ))
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_STOP;
		if (war_mode) return PLUGIN_STOP;
		MENUPAGE_CREATE_FIRST(SystemVotemapPage, &player, 0, -1);
		return PLUGIN_STOP;
	}
	else if (FStrEq( pcmd, "ma_explode" ))
	{
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		if (!gpManiClient->HasAccess(player.index, ADMIN, ADMIN_EXPLODE, war_mode)) return PLUGIN_BAD_ADMIN;

		gpManiAdminPlugin->ProcessExplodeAtCurrentPosition (&player);
		return PLUGIN_STOP;
	} 

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
int PrimaryMenuItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("player_options", sub_option) == 0)
	{
		MENUPAGE_CREATE(PlayerManagementPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("punish", sub_option) == 0)
	{
		MENUPAGE_CREATE(PunishTypePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("mapoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE(MapManagementPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("voteoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE(VoteTypePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("restrict_weapon", sub_option) == 0)
	{
		MENUPAGE_CREATE(RestrictWeaponPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("play_sound", sub_option) == 0)
	{
		MENUPAGE_CREATE(PlaySoundPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("rcon", sub_option) == 0)
	{
		MENUPAGE_CREATE(RConPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("config", sub_option) == 0)
	{
		MENUPAGE_CREATE(ConfigOptionsPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("changemap", sub_option) == 0)
	{
		MENUPAGE_CREATE(ChangeMapPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("warmode", sub_option) == 0)
	{
		mani_war_mode.SetValue(0);
		AdminSayToAll(GREEN_CHAT, player_ptr, 1, "Disabled War Mode"); 
		LogCommand (player_ptr, "Disable war mode\n");
		return RePopOption(REPOP_MENU);
	}
	else if (strcmp("client", sub_option) == 0)
	{
		MENUPAGE_CREATE(ClientOrGroupPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool PrimaryMenuPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 100));
	this->SetTitle("%s", Translate(player_ptr, 101));

	bool allowed = false;
	if (!war_mode)
	{
		if ((gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_KICK) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC_MENU) ||
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MUTE) ||
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SPRAY_TAG)) 
			 && !war_mode)
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 102, player_options);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAY) || 
			 (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAP) && gpManiGameType->IsSlapAllowed()) || 
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BLIND) ||
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FREEZE) ||
			 (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT) && gpManiGameType->IsTeleportAllowed()) ||
			 (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_DRUG) && gpManiGameType->IsDrugAllowed()) ||
			 (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BURN) && gpManiGameType->IsFireAllowed()) ||
			 gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_NO_CLIP))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 103, punish);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CHANGEMAP))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 104, mapoptions);
		}

		if ((gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RANDOM_MAP_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MAP_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MENU_RCON_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MENU_QUESTION_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CANCEL_VOTE) && gpManiVote->SysVoteInProgress()))
			
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 105, voteoptions);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON) && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 106, restrict_weapon);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PLAYSOUND))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 107, play_sound);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON_MENU1))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 108, rcon);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CONFIG))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 109, config);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CLIENT_ADMIN))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 2600, client);
			allowed = true;
		}
	}
	else
	{
		// Draw war options
		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CHANGEMAP))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 110, changemap);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON_MENU1))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 108, rcon);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 111, warmode);
			allowed = true;
		}

		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CLIENT_ADMIN))
		{
			MENUOPTION_CREATE(PrimaryMenuItem, 2600, client);
			allowed = true;
		}
	}

	if (!allowed)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 2580, "%s", "admin"));
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------

int ChangeMapItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *map;
	if (this->params.GetParam("map", &map))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_map");
		gpCmd->AddParam("%s", map);
		ProcessMaMap(player_ptr, "ma_map", 0, M_MENU);
	}

	return CLOSE_MENU;
}

bool ChangeMapPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 130));
	this->SetTitle("%s", Translate(player_ptr, 131));
	for (int i = 0; i != map_list_size; i++)
	{
		MenuItem *ptr = new ChangeMapItem();
		ptr->params.AddParam("map", map_list[i].map_name);
		ptr->SetDisplayText(map_list[i].map_name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------
int SetNextMapItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *map;
	if (this->params.GetParam("map", &map))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_setnextmap");
		gpCmd->AddParam("%s", map);
		ProcessMaSetNextMap(player_ptr, "ma_setnextmap", 0, M_MENU);
	}

	return CLOSE_MENU;
}

bool SetNextMapPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 140));
	this->SetTitle("%s", Translate(player_ptr, 141));
	for (int i = 0; i != map_list_size; i++)
	{
		MenuItem *ptr = new SetNextMapItem();
		ptr->params.AddParam("map", map_list[i].map_name);
		ptr->SetDisplayText(map_list[i].map_name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
int KickPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_kick");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaKick(player_ptr, "ma_kick", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU_WAIT3);
}

bool KickPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 150));
	this->SetTitle("%s", Translate(player_ptr, 151));
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (!player.is_bot)
		{
			if (player_ptr->index != player.index &&
				gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_KICK))
			{
				continue;
			}
		}

		MenuItem *ptr = new KickPlayerItem();
		if (player.is_bot)
		{
			ptr->SetDisplayText("BOT [%s]", player.name);
		}
		else
		{
			ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		}
		
		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
int SlapMoreItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	int health;
	if (m_page_ptr->params.GetParam("user_id", &user_id) && m_page_ptr->params.GetParam("health", &health))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_slap");
		gpCmd->AddParam("%i", user_id);
		gpCmd->AddParam("%i", health);
		g_ManiAdminPlugin.ProcessMaSlap(player_ptr, "ma_slap", 0, M_MENU);
	}

	return REPOP_MENU;
}

bool SlapMorePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 163));
	this->SetTitle("%s", Translate(player_ptr, 164));

	int user_id;
	this->params.GetParam("user_id", &user_id);
	int health;
	this->params.GetParam("health", &health);

	player_t player;
	player.user_id = user_id;
	if (!FindPlayerByUserID(&player)) return false;
	if (player.is_dead) return false;

	MenuItem *ptr = new SlapMoreItem;
	ptr->params.AddParam("user_id", user_id);
	ptr->params.AddParam("health", health);
	ptr->SetDisplayText("%s", Translate(player_ptr, 162));
	this->AddItem(ptr);
	return true;
}

int SlapPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	int health;
	if (this->params.GetParam("user_id", &user_id) && m_page_ptr->params.GetParam("health", &health))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_slap");
		gpCmd->AddParam("%i", user_id);
		gpCmd->AddParam("%i", health);
		g_ManiAdminPlugin.ProcessMaSlap(player_ptr, "ma_slap", 0, M_MENU);

		// Move on to repeat slap menu
		MENUPAGE_CREATE_PARAM2(SlapMorePage, player_ptr, AddParam("user_id", user_id), AddParam("health", health), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool SlapPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 160));
	this->SetTitle("%s", Translate(player_ptr, 161));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;

		if (!player.is_bot)
		{
			if (player_ptr->index != player.index &&
				gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_SLAP))
			{
				continue;
			}
		}

		MenuItem *ptr = new SlapPlayerItem();
		if (player.is_bot)
		{
			ptr->SetDisplayText("BOT [%s]", player.name);
		}
		else
		{
			ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		}
		
		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Blind Player draw and request
//---------------------------------------------------------------------------------
int BlindPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	int blind;
	if (this->params.GetParam("user_id", &user_id) && m_page_ptr->params.GetParam("blind", &blind))
	{
		if (blind > 255) blind = 255;
		if (blind < 0) blind = 0;
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_blind");
		gpCmd->AddParam("%i", user_id);
		gpCmd->AddParam("%i", blind);
		g_ManiAdminPlugin.ProcessMaBlind(player_ptr, "ma_blind", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool BlindPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 170));
	this->SetTitle("%s", Translate(player_ptr, 171));

	int blind;
	this->params.GetParam("blind", &blind);

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index && gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BLIND))
		{
			continue;
		}

		MenuItem *ptr = new BlindPlayerItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Player draw and request
//---------------------------------------------------------------------------------
int FreezePlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_freeze");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaFreeze(player_ptr, "ma_freeze", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool FreezePlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 190));
	this->SetTitle("%s", Translate(player_ptr, 191));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_FREEZE)) continue;

		MenuItem *ptr = new FreezePlayerItem();
		ptr->SetDisplayText("%s[%s] %i", (punish_mode_list[player.index - 1].frozen) ? Translate(player_ptr, 192):"", player.name, player.user_id);		
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Burn Player draw and request
//---------------------------------------------------------------------------------
int BurnPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_burn");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaBurn(player_ptr, "ma_burn", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool BurnPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 740));
	this->SetTitle("%s", Translate(player_ptr, 741));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_BURN)) continue;

		MenuItem *ptr = new BurnPlayerItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);		
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Drug Player draw and request
//---------------------------------------------------------------------------------
int DrugPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_drug");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaDrug(player_ptr, "ma_drug", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool DrugPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 200));
	this->SetTitle("%s", Translate(player_ptr, 201));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_DRUG)) continue;

		MenuItem *ptr = new DrugPlayerItem();
		ptr->SetDisplayText("%s[%s] %i", (punish_mode_list[player.index - 1].drugged) ? Translate(player_ptr, 202):"", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle No Clip Player draw and request
//---------------------------------------------------------------------------------
int NoClipPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_noclip");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaNoClip(player_ptr, "ma_noclip", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool NoClipPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 750));
	this->SetTitle("%s", Translate(player_ptr, 751));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;
		if (player.is_bot) continue;

		MenuItem *ptr = new NoClipPlayerItem();
		ptr->SetDisplayText("%s[%s] %i", (punish_mode_list[player.index - 1].no_clip) ? Translate(player_ptr, 752):"", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle TimeBomb Player draw and request
//---------------------------------------------------------------------------------
int TimeBombPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_timebomb");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaTimeBomb(player_ptr, "ma_timebomb", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool TimeBombPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 850));
	this->SetTitle("%s", Translate(player_ptr, 851));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_TIMEBOMB)) continue;

		MenuItem *ptr = new TimeBombPlayerItem();
		ptr->SetDisplayText("%s%s %i", (punish_mode_list[player.index - 1].time_bomb != 0) ? "-> ":"", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Fire Bomb Player draw and request
//---------------------------------------------------------------------------------
int FireBombPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_firebomb");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaFireBomb(player_ptr, "ma_firebomb", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool FireBombPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 852));
	this->SetTitle("%s", Translate(player_ptr, 853));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_FIREBOMB)) continue;

		MenuItem *ptr = new FireBombPlayerItem();
		ptr->SetDisplayText("%s%s %i", (punish_mode_list[player.index - 1].fire_bomb != 0) ? "-> ":"",	player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Bomb Player draw and request
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Handle Fire Bomb Player draw and request
//---------------------------------------------------------------------------------
int FreezeBombPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_freezebomb");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaFreezeBomb(player_ptr, "ma_freezebomb", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool FreezeBombPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 854));
	this->SetTitle("%s", Translate(player_ptr, 855));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_FREEZEBOMB)) continue;

		MenuItem *ptr = new FreezeBombPlayerItem();
		ptr->SetDisplayText("%s%s %i", (punish_mode_list[player.index - 1].freeze_bomb != 0) ? "-> ":"",	player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Beacon Player draw and request
//---------------------------------------------------------------------------------
int BeaconPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_beacon");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaBeacon(player_ptr, "ma_beacon", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool BeaconPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 856));
	this->SetTitle("%s", Translate(player_ptr, 857));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_BEACON)) continue;

		MenuItem *ptr = new BeaconPlayerItem();
		ptr->SetDisplayText("%s%s %i", (punish_mode_list[player.index - 1].beacon != 0) ? "-> ":"",	player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Mute Player draw and request
//---------------------------------------------------------------------------------
int MutePlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_mute");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaMute(player_ptr, "ma_mute", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool MutePlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 760));
	this->SetTitle("%s", Translate(player_ptr, 761));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index,IMMUNITY, IMMUNITY_MUTE)) continue;

		MenuItem *ptr = new MutePlayerItem();
		ptr->SetDisplayText("%s[%s] %i", (punish_mode_list[player.index - 1].muted) ? Translate(player_ptr, 762):"", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Test if player has saved this location
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanTeleport(player_t *player)
{
	player_settings_t *player_settings = FindPlayerSettings(player);

	for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
	{
		if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Teleport Player draw and request
//---------------------------------------------------------------------------------
int TeleportPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_teleport");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaTeleport(player_ptr, "ma_teleport", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool TeleportPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	if (!g_ManiAdminPlugin.CanTeleport(player_ptr))
	{
		PrintToClientConsole(player_ptr->entity, Translate(player_ptr, 222));
		return false;
	}

	this->SetEscLink("%s", Translate(player_ptr, 220));
	this->SetTitle("%s", Translate(player_ptr, 221));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;

		if (!player.is_bot &&
			player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_TELEPORT)) continue;

		MenuItem *ptr = new TeleportPlayerItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int SlayPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	if (this->params.GetParam("user_id", &user_id))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_slay");
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaSlay(player_ptr, "ma_slay", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU_WAIT1);
}

bool SlayPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 230));
	this->SetTitle("%s", Translate(player_ptr, 231));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;

		if (!player.is_bot &&
			player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_SLAY)) continue;

		MenuItem *ptr = new SlayPlayerItem();
		if (player.is_bot)
		{
			ptr->SetDisplayText("BOT [%s]",  player.name);							
		}
		else
		{
			ptr->SetDisplayText("[%s] %i",  player.name, player.user_id);							
		}

		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------

void CAdminPlugin::ProcessExplodeAtCurrentPosition( player_t *player)
{
	Vector pos = player->entity->GetCollideable()->GetCollisionOrigin();

	if (esounds)
	{
		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();

		mrf.AddAllPlayers(max_players);

		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, slay_sound_name, 0.5,  ATTN_NORM, 0, 100, &pos);
		}
		else
		{
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, hl2mp_slay_sound_name, 0.6,  ATTN_NORM, 0, 100, &pos);
		}
	}

	if (effects)
	{
		// Generate some FXs
		int mag = 60;
		int trailLen = 4;
		pos.z+=12; // rise the effect for better results :)
		effects->Sparks(pos, mag, trailLen, NULL);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Allows admin to run rcon commands from a list
//---------------------------------------------------------------------------------
int RConItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int param_index;
	if (this->params.GetParam("param_index", &param_index))
	{
		char	rcon_cmd[512];

		if (param_index < 0 || param_index >= rcon_list_size)
		{
			return CLOSE_MENU;
		}

		snprintf( rcon_cmd, sizeof(rcon_cmd),	"%s\n", rcon_list[param_index].rcon_command);
		LogCommand (player_ptr, "rcon command [%s]\n", rcon_list[param_index].rcon_command);
		engine->ServerCommand(rcon_cmd);
	}

	return REPOP_MENU;
}

bool RConPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 260));
	this->SetTitle("%s", Translate(player_ptr, 261));

	for( int i = 0; i < rcon_list_size; i++ )
	{
		MenuItem *ptr = new RConItem();
		ptr->SetDisplayText("%s", rcon_list[i].alias);
		ptr->params.AddParam("param_index", i);
		this->AddItem(ptr);
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose:  Show the Map Management menu
//---------------------------------------------------------------------------------
int MapManagementItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("changemap", sub_option) ==0)
	{
		MENUPAGE_CREATE(ChangeMapPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("setnextmap", sub_option) ==0)
	{
		MENUPAGE_CREATE(SetNextMapPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool MapManagementPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 590));
	this->SetTitle("%s", Translate(player_ptr, 591));

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CHANGEMAP))
	{
		MENUOPTION_CREATE(MapManagementItem, 592, changemap);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CHANGEMAP) && !war_mode)
	{
		MENUOPTION_CREATE(MapManagementItem, 593, setnextmap);
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose:  Show the player Management screens
//---------------------------------------------------------------------------------
int PlayerManagementItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("slay", sub_option) ==0)
	{
		MENUPAGE_CREATE(SlayPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("kicktype", sub_option) ==0)
	{
		MENUPAGE_CREATE(KickTypePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("bantype", sub_option) ==0)
	{
		MENUPAGE_CREATE(BanTypePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("swapteam", sub_option) ==0)
	{
		MENUPAGE_CREATE(SwapPlayerPage, player_ptr, 0 , -1);
		return NEW_MENU;
	}
	else if (strcmp("swapteamd", sub_option) ==0)
	{
		MENUPAGE_CREATE(SwapPlayerDPage, player_ptr, 0 , -1);
		return NEW_MENU;
	}
	else if (strcmp("specplay", sub_option) ==0)
	{
		MENUPAGE_CREATE(SpecPlayerPage, player_ptr, 0 , -1);
		return NEW_MENU;
	}
	else if (strcmp("balanceteam", sub_option) ==0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_balance");
		gpManiTeam->ProcessMaBalance (player_ptr, "ma_balance", 0, M_MENU);
		return CLOSE_MENU;
	}
	else if (strcmp("cexecoptions", sub_option) ==0)
	{
		MENUPAGE_CREATE(CExecOptionsPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("mute", sub_option) ==0)
	{
		MENUPAGE_CREATE(MutePlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("spray", sub_option) ==0)
	{
		MENUPAGE_CREATE(SprayPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("observe", sub_option) ==0)
	{
		MENUPAGE_CREATE(ObservePlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool PlayerManagementPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 610));
	this->SetTitle("%s", Translate(player_ptr, 611));

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAY) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 612, slay);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_KICK) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 613, kicktype);
	}

	if ((gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN) ||
		gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN)) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 614, bantype);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP) && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		MENUOPTION_CREATE(PlayerManagementItem, 615, swapteam);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP) && !war_mode && gpManiGameType->IsTeamPlayAllowed() && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		MENUOPTION_CREATE(PlayerManagementItem, 184, swapteamd);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP) && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		MENUOPTION_CREATE(PlayerManagementItem, 619, specplay);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP) && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		MENUOPTION_CREATE(PlayerManagementItem, 616, balanceteam);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC_MENU) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 617, cexecoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MUTE) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 618, mute);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SPRAY_TAG) && !war_mode)
	{
		MENUOPTION_CREATE(PlayerManagementItem, 620, spray);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN) && !war_mode &&
		gpManiGameType->IsSpectatorAllowed())
	{
		MENUOPTION_CREATE(PlayerManagementItem, 3113, observe);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Allows admin to run client commands from a list
//---------------------------------------------------------------------------------
int CExecOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	this->params.GetParam("sub_option", &sub_option);
	MENUPAGE_CREATE_PARAM(CExecPage, player_ptr, AddParam("sub_option", sub_option), 0, -1);
	return NEW_MENU;
}

bool CExecOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 440));
	this->SetTitle("%s", Translate(player_ptr, 441));

	MenuItem *ptr;

	ptr = new CExecOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 442));
	ptr->params.AddParam("sub_option", "cexec");
	this->AddItem(ptr);

	ptr = new CExecOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 443));
	ptr->params.AddParam("sub_option", "cexec_all");
	this->AddItem(ptr);

	if (gpManiGameType->IsTeamPlayAllowed())
	{
		ptr = new CExecOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 444));
		ptr->params.AddParam("sub_option", "cexec_t");
		this->AddItem(ptr);

		ptr = new CExecOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 445));
		ptr->params.AddParam("sub_option", "cexec_ct");
		this->AddItem(ptr);
	}

	if (gpManiGameType->IsSpectatorAllowed())
	{
		ptr = new CExecOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 446));
		ptr->params.AddParam("sub_option", "cexec_spec");
		this->AddItem(ptr);
	}

	return true;
}

int CExecItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int		index;

	char *sub_option;
	if (!m_page_ptr->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	this->params.GetParam("index", &index);
	if (strcmp("cexec_t", sub_option) ==0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_cexec_t");
		gpCmd->AddParam("%s", cexec_t_list[index].cexec_command);
		g_ManiAdminPlugin.ProcessMaCExecT(player_ptr, "ma_cexec_t", 0, M_MENU);
	}
	else if (strcmp("cexec_ct", sub_option) ==0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_cexec_ct");
		gpCmd->AddParam("%s", cexec_ct_list[index].cexec_command);
		g_ManiAdminPlugin.ProcessMaCExecCT(player_ptr, "ma_cexec_ct", 0, M_MENU);
	}
	else if (strcmp("cexec_spec", sub_option) ==0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_cexec_spec");
		gpCmd->AddParam("%s", cexec_spec_list[index].cexec_command);
		g_ManiAdminPlugin.ProcessMaCExecSpec(player_ptr, "ma_cexec_spec", 0, M_MENU);
	}
	else if (strcmp("cexec_all", sub_option) ==0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_cexec_all");
		gpCmd->AddParam("%s", cexec_all_list[index].cexec_command);
		g_ManiAdminPlugin.ProcessMaCExecAll(player_ptr, "ma_cexec_all", 0, M_MENU);
	}
	else
	{
		// Single Player needs player sub menu
		MENUPAGE_CREATE_PARAM(CExecPlayerPage, player_ptr, AddParam("index", index), 0, -1);
		return NEW_MENU;
	}

	return RePopOption(REPOP_MENU);
}

bool CExecPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 270));
	this->SetTitle("%s", Translate(player_ptr, 271));

	MenuItem *ptr;
	char *sub_option;
	this->params.GetParam("sub_option", &sub_option);

	if (strcmp("cexec_t", sub_option) == 0)
	{
		for( int i = 0; i < cexec_t_list_size; i++ )
		{
			ptr = new CExecItem();
			ptr->params.AddParam("index", i);
			ptr->SetDisplayText("%s", cexec_t_list[i].alias);
			this->AddItem(ptr);
		}
	}
	else if (strcmp("cexec_ct", sub_option) == 0)
	{
		for( int i = 0; i < cexec_ct_list_size; i++ )
		{
			ptr = new CExecItem();
			ptr->params.AddParam("index", i);
			ptr->SetDisplayText("%s", cexec_ct_list[i].alias);
			this->AddItem(ptr);
		}
	}
	else if (strcmp("cexec_spec", sub_option) == 0)
	{
		for( int i = 0; i < cexec_spec_list_size; i++ )
		{
			ptr = new CExecItem();
			ptr->params.AddParam("index", i);
			ptr->SetDisplayText("%s", cexec_spec_list[i].alias);
			this->AddItem(ptr);
		}
	}
	else if (strcmp("cexec_all", sub_option) == 0)
	{
		for( int i = 0; i < cexec_spec_list_size; i++ )
		{
			ptr = new CExecItem();
			ptr->params.AddParam("index", i);
			ptr->SetDisplayText("%s", cexec_all_list[i].alias);
			this->AddItem(ptr);
		}
	}
	else if (strcmp("cexec", sub_option) == 0)
	{
		for( int i = 0; i < cexec_list_size; i++ )
		{
			ptr = new CExecItem();
			ptr->params.AddParam("index", i);
			ptr->SetDisplayText("%s", cexec_list[i].alias);
			this->AddItem(ptr);
		}
	}
	return true;
}

int CExecPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int user_id;
	int	index;

	if (this->params.GetParam("user_id", &user_id) &&
		this->params.GetParam("index", &index))
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_cexec");
		gpCmd->AddParam("%i", user_id);
		gpCmd->AddParam("%s", cexec_list[index].cexec_command);
		g_ManiAdminPlugin.ProcessMaCExec(player_ptr, "ma_cexec_all", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU_WAIT1);
}

bool CExecPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 280));
	this->SetTitle("%s", Translate(player_ptr, 281));

	int index;

	this->params.GetParam("index", &index);

	MenuItem *ptr;

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index && 
			gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_CEXEC))
		{
			continue;
		}

		ptr = new CExecPlayerItem();
		ptr->params.AddParam("user_id", player.user_id);
		ptr->params.AddParam("index", index);
		ptr->SetDisplayText("[%s] %i",  player.name, player.user_id);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}
//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
int PunishTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("slapoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE(SlapOptionsPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("blindoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE(BlindOptionsPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("freeze", sub_option) == 0)
	{
		MENUPAGE_CREATE(FreezePlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("drug", sub_option) == 0)
	{
		MENUPAGE_CREATE(DrugPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("teleport", sub_option) == 0)
	{
		MENUPAGE_CREATE(TeleportPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("savelocation", sub_option) == 0)
	{
		g_ManiAdminPlugin.ProcessMaSaveLoc(player_ptr, "ma_saveloc", 0, M_MENU); 
		return RePopOption(REPOP_MENU);
	}
	else if (strcmp("burn", sub_option) == 0)
	{
		MENUPAGE_CREATE(BurnPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("noclip", sub_option) == 0)
	{
		MENUPAGE_CREATE(NoClipPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("skinoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE(SkinOptionsPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("timebomb", sub_option) == 0)
	{
		MENUPAGE_CREATE(TimeBombPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("freezebomb", sub_option) == 0)
	{
		MENUPAGE_CREATE(FreezeBombPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("firebomb", sub_option) == 0)
	{
		MENUPAGE_CREATE(FireBombPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("beacon", sub_option) == 0)
	{
		MENUPAGE_CREATE(BeaconPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool PunishTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 290));
	this->SetTitle("%s", Translate(player_ptr, 291));

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAP) && !war_mode && gpManiGameType->IsSlapAllowed())
	{
		MENUOPTION_CREATE(PunishTypeItem, 292, slapoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BLIND) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 293, blindoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FREEZE) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 294, freeze);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_DRUG) && !war_mode && gpManiGameType->IsDrugAllowed())
	{
		MENUOPTION_CREATE(PunishTypeItem, 295, drug);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT) && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		player_settings_t *player_settings;
		player_settings = FindPlayerSettings(player_ptr);
		if (player_settings)
		{
			for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
			{
				if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
				{
					MENUOPTION_CREATE(PunishTypeItem, 297, teleport);
					break;
				}
			}
		}
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT) && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		MENUOPTION_CREATE(PunishTypeItem, 298, savelocation);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BURN) && !war_mode && gpManiGameType->IsFireAllowed())
	{
		MENUOPTION_CREATE(PunishTypeItem, 299, burn);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_NO_CLIP) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 300, noclip);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SETSKINS) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 301, skinoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TIMEBOMB) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 302, timebomb);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FIREBOMB) && !war_mode && gpManiGameType->IsFireAllowed())
	{
		MENUOPTION_CREATE(PunishTypeItem, 303, firebomb);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FREEZEBOMB) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 304, freezebomb);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BEACON) && !war_mode)
	{
		MENUOPTION_CREATE(PunishTypeItem, 305, beacon);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
int VoteTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("votercon", sub_option) == 0)
	{
		MENUPAGE_CREATE(RConVotePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("votequestion", sub_option) == 0)
	{
		MENUPAGE_CREATE(QuestionVotePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("voteextend", sub_option) == 0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_voteextend");
		gpManiVote->ProcessMaVoteExtend(player_ptr, "ma_voteextend", 0, M_MENU);
		return CLOSE_MENU;
	}
	else if (strcmp("randomvoteoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(VoteDelayTypePage, player_ptr, AddParam("vote_type", sub_option), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("mapvoteoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(VoteDelayTypePage, player_ptr, AddParam("vote_type", sub_option), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("buildmapvote", sub_option) == 0)
	{
		MENUPAGE_CREATE(SystemVoteBuildMapPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("multimapvoteoptions", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(VoteDelayTypePage, player_ptr, AddParam("vote_type", sub_option), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("cancelvote", sub_option) == 0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_votecancel");
		gpManiVote->ProcessMaVoteCancel(player_ptr, "ma_votecancel", 0, M_MENU);
		return CLOSE_MENU;
	}
	return CLOSE_MENU;
}

bool VoteTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 310));
	this->SetTitle("%s", Translate(player_ptr, 311));

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MENU_RCON_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 312, votercon);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MENU_QUESTION_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 313, votequestion);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 314, voteextend);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RANDOM_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 315, randomvoteoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 316, mapvoteoptions);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 317, buildmapvote);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		int		m_list_size = 0;
		map_t	*m_list = NULL;

		// Set pointer to correct list
		switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
		{
			case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
			case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
			case 2: m_list = map_list; m_list_size = map_list_size;break;
			default : break;
		}

		int selected_map = 0;

		for (int i = 0; i < m_list_size; i++)
		{
			if (m_list[i].selected_for_vote)
			{
				selected_map ++;
				break;
			}
		}

		if (selected_map != 0)
		{
			MENUOPTION_CREATE(VoteTypeItem, 318, multimapvoteoptions);
		}
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CANCEL_VOTE) && gpManiVote->SysVoteInProgress())
	{
		MENUOPTION_CREATE(VoteTypeItem, 319, cancelvote);
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Show Delay Type Options
//---------------------------------------------------------------------------------
int VoteDelayTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *vote_type;
	char *delay_type;

	m_page_ptr->params.GetParam("vote_type", &vote_type);
	this->params.GetParam("delay_type", &delay_type);

	if (strcmp(vote_type, "randomvoteoptions") == 0)
	{
		MENUPAGE_CREATE_PARAM(SystemVoteRandomMapPage, player_ptr, AddParam("delay_type", delay_type), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(vote_type, "mapvoteoptions") == 0)
	{
		MENUPAGE_CREATE_PARAM(SystemVoteSingleMapPage, player_ptr, AddParam("delay_type", delay_type), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(vote_type, "multimapvoteoptions") == 0)
	{
		gpManiVote->ProcessMenuSystemVoteMultiMap(player_ptr, delay_type);
		return CLOSE_MENU;
	}

	return CLOSE_MENU;
}

bool VoteDelayTypePage::PopulateMenuPage(player_t *player_ptr)
{
	if (gpManiVote->SysVoteInProgress()) return false;

	this->SetEscLink("%s", Translate(player_ptr, 310));
	this->SetTitle("%s", Translate(player_ptr, 311));

	MenuItem *ptr;

	ptr = new VoteDelayTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 332));
	ptr->params.AddParam("delay_type", "now");
	this->AddItem(ptr);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		ptr = new VoteDelayTypeItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 333));
		ptr->params.AddParam("delay_type", "round");
		this->AddItem(ptr);
	}

	ptr = new VoteDelayTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 334));
	ptr->params.AddParam("delay_type", "end");
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int SlapOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int	health;

	this->params.GetParam("health", &health);
	MENUPAGE_CREATE_PARAM(SlapPlayerPage, player_ptr, AddParam("health", health), 0, -1);

	return NEW_MENU;
}

bool SlapOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 370));
	this->SetTitle("%s", Translate(player_ptr, 371));

	MenuItem *ptr;

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 372));
	ptr->params.AddParam("health", 0);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 373));
	ptr->params.AddParam("health", 1);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 374));
	ptr->params.AddParam("health", 5);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 375));
	ptr->params.AddParam("health", 10);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 376));
	ptr->params.AddParam("health", 20);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 377));
	ptr->params.AddParam("health", 50);
	this->AddItem(ptr);

	ptr = new SlapOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 378));
	ptr->params.AddParam("health", 99);
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Show user the blindness amounts
//---------------------------------------------------------------------------------
int BlindOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int	blind;

	this->params.GetParam("blind", &blind);
	MENUPAGE_CREATE_PARAM(BlindPlayerPage, player_ptr, AddParam("blind", blind), 0, -1);
	return NEW_MENU;
}

bool BlindOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 390));
	this->SetTitle("%s", Translate(player_ptr, 391));

	MenuItem *ptr;

	ptr = new BlindOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 392));
	ptr->params.AddParam("blind", 0);
	this->AddItem(ptr);

	ptr = new BlindOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 393));
	ptr->params.AddParam("blind", 245);
	this->AddItem(ptr);

	ptr = new BlindOptionsItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 394));
	ptr->params.AddParam("blind", 255);
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int BanOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *ban_type;
	int	time;

	m_page_ptr->params.GetParam("ban_type", &ban_type);
	this->params.GetParam("time", &time);

	if (strcmp(ban_type, "steam_id") == 0)
	{
		MENUPAGE_CREATE_PARAM2(BanPlayerPage, player_ptr, AddParam("ban_type", ban_type), AddParam("time", time), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(ban_type, "ip_address") == 0)
	{
		MENUPAGE_CREATE_PARAM2(BanPlayerPage, player_ptr, AddParam("ban_type", ban_type), AddParam("time", time), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(ban_type, "name") == 0)
	{
		MENUPAGE_CREATE_PARAM(AutoBanPage, player_ptr, AddParam("time", time), 0, -1);
		return NEW_MENU;
	}
	return CLOSE_MENU;
}

bool BanOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 350));
	this->SetTitle("%s", Translate(player_ptr, 351));
	char *sub_option;

	this->params.GetParam("sub_option", &sub_option);
	MenuItem *ptr;

	bool perm_ban = true;
	bool temp_ban = true;

	// Check if player is admin
	perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
	temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);
	int max_time = mani_admin_temp_ban_time_limit.GetInt();

	if (perm_ban)
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 358));
		ptr->params.AddParam("time", 0);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 5))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 352));
		ptr->params.AddParam("time", 5);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 30))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 353));
		ptr->params.AddParam("time", 30);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 60))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 354));
		ptr->params.AddParam("time", 60);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 120))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 355));
		ptr->params.AddParam("time", 120);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 1440))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 356));
		ptr->params.AddParam("time", 1440);
		this->AddItem(ptr);
	}

	if (perm_ban || (temp_ban && max_time >= 10080))
	{
		ptr = new BanOptionsItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 357));
		ptr->params.AddParam("time", 10080);
		this->AddItem(ptr);
	}

	return true;
}

int BanTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *ban_type;

	this->params.GetParam("ban_type", &ban_type);

	if (strcmp(ban_type, "steam_id") == 0)
	{
		MENUPAGE_CREATE_PARAM(BanOptionsPage, player_ptr, AddParam("ban_type", ban_type), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(ban_type, "ip_address") == 0)
	{
		MENUPAGE_CREATE_PARAM(BanOptionsPage, player_ptr, AddParam("ban_type", ban_type), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(ban_type, "name") == 0)
	{
		MENUPAGE_CREATE_PARAM(BanOptionsPage, player_ptr, AddParam("ban_type", ban_type), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool BanTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 400));
	this->SetTitle("%s", Translate(player_ptr, 401));

	MenuItem *ptr;

	ptr = new BanTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 402));
	ptr->params.AddParam("ban_type", "steam_id");
	this->AddItem(ptr);

	ptr = new BanTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 403));
	ptr->params.AddParam("ban_type", "ip_address");
	this->AddItem(ptr);

	ptr = new BanTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 404));
	ptr->params.AddParam("ban_type", "name");
	this->AddItem(ptr);
	return true;
}

int BanPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *ban_type;
	int	user_id;
	int	time;

	m_page_ptr->params.GetParam("ban_type", &ban_type);
	m_page_ptr->params.GetParam("time", &time);
	this->params.GetParam("user_id", &user_id);

	if (strcmp(ban_type, "steam_id") == 0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_ban");
		gpCmd->AddParam("%i", time);
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaBan(player_ptr, "ma_ban", 0, M_MENU);
		return RePopOption(REPOP_MENU_WAIT3);
	}
	else if (strcmp(ban_type, "ip_address") == 0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_banip");
		gpCmd->AddParam("%i", time);
		gpCmd->AddParam("%i", user_id);
		g_ManiAdminPlugin.ProcessMaBanIP(player_ptr, "ma_banip", 0, M_MENU);
		return RePopOption(REPOP_MENU_WAIT3);
	}

	return CLOSE_MENU;
}

bool BanPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	char *ban_type;
	this->params.GetParam("ban_type", &ban_type);
	
	this->SetEscLink("%s", Translate(player_ptr, 500));

	if (strcmp(ban_type, "steam_id") == 0)
	{
		// ban by user id
		this->SetTitle("%s", Translate(player_ptr, 501));
	}
	else
	{
		// ban by ip
		this->SetTitle("%s", Translate(player_ptr, 502));
	}

	MenuItem *ptr;

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		if (player_ptr->index != player.index && 
			gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
		{
			continue;
		}

		ptr = new BanPlayerItem;
		ptr->SetDisplayText("[%s] %i",  player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int KickTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	this->params.GetParam("sub_option", &sub_option);

	if (strcmp(sub_option, "kick") == 0)
	{
		MENUPAGE_CREATE(KickPlayerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(sub_option, "autokickname") == 0 ||
		strcmp(sub_option, "autokicksteam") == 0 ||
		strcmp(sub_option, "autokickip") == 0)
	{
		MENUPAGE_CREATE_PARAM(AutoKickPage, player_ptr, AddParam("ban_type", sub_option), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool KickTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 420));
	this->SetTitle("%s", Translate(player_ptr, 421));

	MenuItem *ptr;

	ptr = new KickTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 422));
	ptr->params.AddParam("sub_option", "kick");
	this->AddItem(ptr);

	ptr = new KickTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 423));
	ptr->params.AddParam("sub_option", "autokickname");
	this->AddItem(ptr);

	ptr = new KickTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 424));
	ptr->params.AddParam("sub_option", "autokicksteam");
	this->AddItem(ptr);

	ptr = new KickTypeItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 425));
	ptr->params.AddParam("sub_option", "autokickip");
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin config options
//---------------------------------------------------------------------------------
int ConfigOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	this->params.GetParam("sub_option", &sub_option);
	if (strcmp(sub_option, "adverts") == 0)
	{
		ToggleAdverts(player_ptr);
		return REPOP_MENU;
	}
	else if (strcmp(sub_option, "tk_protection") == 0)
	{
		if (mani_tk_protection.GetInt() == 1)
		{
			mani_tk_protection.SetValue(0);
			FreeList((void **) &tk_player_list, &tk_player_list_size);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled tk protection", player_ptr->name);
			LogCommand (player_ptr, "Disable tk protection\n");
		}
		else
		{
			mani_tk_protection.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled tk protection", player_ptr->name);
			LogCommand (player_ptr, "Enable tk protection\n");
			// Need to turn off tk punish
			engine->ServerCommand("mp_tkpunish 0\n");
		}
		return REPOP_MENU;
	}
	else if (strcmp(sub_option, "tk_forgive") == 0)
	{
		if (mani_tk_forgive.GetInt() == 1)
		{
			mani_tk_forgive.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled tk forgive options", player_ptr->name);
			LogCommand (player_ptr, "Disable tk forgive\n");
		}
		else
		{
			mani_tk_forgive.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled tk forgive options", player_ptr->name);
			LogCommand (player_ptr, "Enable tk forgive\n");
		}
		return REPOP_MENU;
	}
	else if (strcmp(sub_option, "warmode") == 0)
	{
		if (mani_war_mode.GetInt() == 1)
		{
			mani_war_mode.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled War Mode", player_ptr->name);
			LogCommand (player_ptr, "Disable war mode\n");
		}
		else
		{
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled War Mode", player_ptr->name);
			LogCommand (player_ptr, "Enable war mode\n");
			mani_war_mode.SetValue(1);
		}
		return CLOSE_MENU;
	}
	else if (strcmp(sub_option, "stats") == 0)
	{
		if (mani_stats.GetInt() == 1)
		{
			mani_stats.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled stats", player_ptr->name);
			LogCommand (player_ptr, "Disable stats\n");
		}
		else
		{
			mani_stats.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled stats", player_ptr->name);
			LogCommand (player_ptr, "Enable stats\n");
		}
		return REPOP_MENU;
	}
	else if (strcmp(sub_option, "resetstats") == 0)
	{
		gpManiStats->ResetStats ();
		SayToAll (GREEN_CHAT, true, "ADMIN %s reset the stats", player_ptr->name);
		LogCommand (player_ptr, "Reset stats\n");
		return RePopOption(REPOP_MENU);
	}

	return CLOSE_MENU;
}

bool ConfigOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 460));
	this->SetTitle("%s", Translate(player_ptr, 461));

	MenuItem *ptr;

	ptr = new ConfigOptionsItem;
	ptr->params.AddParam("sub_option", "adverts");
	if (mani_adverts.GetInt() == 1)
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 462));
	}
	else
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 463));
	}
	this->AddItem(ptr);

	ptr = new ConfigOptionsItem;
	ptr->params.AddParam("sub_option", "tk_protection");
	if (mani_tk_protection.GetInt() == 1)
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 464));
	}
	else
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 465));
	}
	this->AddItem(ptr);

	ptr = new ConfigOptionsItem;
	ptr->params.AddParam("sub_option", "tk_forgive");
	if (mani_tk_forgive.GetInt() == 1)
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 466));
	}
	else
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 467));
	}
	this->AddItem(ptr);

	ptr = new ConfigOptionsItem;
	ptr->params.AddParam("sub_option", "warmode");
	if (mani_war_mode.GetInt() == 1)
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 468));
	}
	else
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 469));
	}
	this->AddItem(ptr);

	ptr = new ConfigOptionsItem;
	ptr->params.AddParam("sub_option", "stats");
	if (mani_stats.GetInt() == 1)
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 470));
	}
	else
	{
		ptr->SetDisplayText("%s", Translate(player_ptr, 471));
	}
	this->AddItem(ptr);

	if (mani_stats.GetInt() == 1)
	{
		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESET_ALL_RANKS))
		{
			ptr = new ConfigOptionsItem;
			ptr->params.AddParam("sub_option", "resetstats");
			ptr->SetDisplayText("%s", Translate(player_ptr, 472));
			this->AddItem(ptr);
		}
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CAdminPlugin::FireGameEvent( IGameEvent * event )
{
	if (ProcessPluginPaused()) return;
	if (!war_mode && 
		mani_show_events.GetInt() != 0)
	{
		MMsg("Event Name [%s]\n", event->GetName());
	}

	// Get numeric index for this event
	int hash_index = this->GetEventIndex(event->GetName(), MANI_EVENT_HASH_SIZE);
	if (hash_index == -1) return;
	int	true_index = event_table[hash_index];
	if (event_fire[true_index].funcPtr == NULL) return;

	// Call event handler callback function
	(g_ManiAdminPlugin.*event_fire[true_index].funcPtr) (event);

}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvPlayerHurt(IGameEvent *event)
{
	player_t	victim;
	player_t	attacker;
	int tk_player_index = -1;
	char		weapon_name[128];

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);

	if (attacker.user_id > 0)
	{
		if (!FindPlayerByUserID(&attacker)) return;
	}

	if (!FindPlayerByUserID(&victim)) return;

	gpManiVictimStats->PlayerHurt(&victim, &attacker, event);
	gpManiStats->PlayerHurt(&victim, &attacker, event);
	gpManiMostDestructive->PlayerHurt(&victim, &attacker, event);

	// Log stats
	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->PlayerHurt(&victim, &attacker, event);
	}

	if (!gpManiGameType->IsTeamPlayAllowed()) return;

	// World attacked
	if (attacker.user_id <= 0) return;

	if (!mp_friendlyfire) return;
	if (mp_friendlyfire->GetInt() == 0)	return;

	// Did attack happen to fellow team mate ?
	if (!gpManiTeam->IsOnSameTeam (&victim, &attacker))	return;

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		Q_strcpy(weapon_name, event->GetString("weapon","NULL"));
		if (FStrEq("smokegrenade", weapon_name)) return;
		if (FStrEq("flashbang", weapon_name)) return;
	}

	// Is option to show opposite team wound on ?
	if (mani_tk_show_opposite_team_wound.GetInt() == 1)
	{
		bool show_attack = true;
		int attacker_index = attacker.index - 1;

		// Test how soon this happened since the last one
		if (tw_spam_list[attacker_index].last_time + 0.40 > gpGlobals->curtime
			&& tw_spam_list[attacker_index].index == victim.index)
		{
			// This is spam, so set flag.
			show_attack = false;
		}
		else
		{	
			// Show the attack
			tw_spam_list[attacker_index].last_time = gpGlobals->curtime;
			tw_spam_list[attacker_index].index = victim.index;
		}

		// Not spam but update time anyway

		if (show_attack)
		{
			// Show player attack to opposite side 
			bool	ct = false;
			bool	t = false;
			if (attacker.team == TEAM_A)
			{
				ct = true;
			}
			else if (attacker.team == TEAM_B)
			{
				t = true;
			}

			// Show to all spectators
			SayToTeam (ORANGE_CHAT, ct, t, true, "(%s) %s attacked a teammate", 
				Translate(NULL, gpManiGameType->GetTeamShortTranslation(attacker.team)), attacker.name);
		}
	}

	if (gpManiWarmupTimer->IgnoreTK()) return;

	// Do out of spawn protection time processing
	if (gpGlobals->curtime > end_spawn_protection_time)
	{
		if (mani_tk_slap_on_team_wound.GetInt() == 1)
		{
			// If set then slap them
			ProcessSlapPlayer(&attacker, mani_tk_slap_on_team_wound_damage.GetInt(), true);
		}

		if (mani_tk_team_wound_reflect.GetInt() == 1)
		{
			ProcessReflectDamagePlayer(&victim, &attacker, event);
		}

		return;
	}

	// Exit if tk protection not enabled
	if (mani_tk_protection.GetInt() == 0) return;

	// TK/TW occured (both on same team)
	// Slay them first before anything 

	// Handle tracking of this player
	// Find tkplayer list

	if (attacker.is_bot)
	{
		// Bots don't normally spawn attack
		SlayPlayer(&attacker, false, true, true);
		DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
		SayToAll (ORANGE_CHAT, false, "Player %s has been slayed for spawn attacking", attacker.name);
		return;
	}

	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), &attacker))
		{
			tk_player_index = i;
			break;
		}
	}

	if (tk_player_index != -1)
	{
		// Player is already on list !!
		if (tk_player_list[tk_player_index].last_round_violation != round_number)
		{
			// If a new round then bump the violations
			if (victim.is_bot)
			{
				if (mani_tk_allow_bots_to_add_violations.GetInt() == 1)
				{
					tk_player_list[tk_player_index].violations_committed++;
				}
			}
			else
			{
				tk_player_list[tk_player_index].violations_committed++;
			}

			Q_strcpy(tk_player_list[tk_player_index].name, attacker.name);
			tk_player_list[tk_player_index].last_round_violation = round_number;
			tk_player_list[tk_player_index].rounds_to_miss += tk_player_list[tk_player_index].spawn_violations_committed;
			tk_player_list[tk_player_index].spawn_violations_committed ++;
			SlayPlayer(&attacker, true, true, true);
			DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
			SayToAll (ORANGE_CHAT, false, "Player %s has been slayed for spawn attacking", attacker.name);
			// Check if ban required
			TKBanPlayer (&attacker, tk_player_index);
		}
	}
	else
	{
		// Add this player to list
		CreateNewTKPlayer(attacker.name, attacker.steam_id, attacker.user_id, 0, 0);

		// If a new round then bump the violations
		if (victim.is_bot)
		{
			if (mani_tk_allow_bots_to_add_violations.GetInt() == 1)
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 1;
			}
			else
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 0;
			}
		}
		else
		{
			tk_player_list[tk_player_list_size - 1].violations_committed = 1;
		}

		tk_player_list[tk_player_list_size - 1].spawn_violations_committed = 1;
		SlayPlayer(&attacker, true, true, true);
		DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
		SayToAll (ORANGE_CHAT, false, "Player %s has been slayed for spawn attacking", attacker.name);
		// Check if ban required
		TKBanPlayer (&attacker, tk_player_list_size - 1);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvPlayerTeam(IGameEvent *event)
{
	if (!gpManiGameType->IsTeamPlayAllowed()) return;

	player_t join_player;

	join_player.user_id = event->GetInt("userid", -1);
	if (join_player.user_id == -1) return;

	if (!FindPlayerByUserID(&join_player)) return;
	join_player.team = event->GetInt("team", 1);
	gpManiTeamJoin->PlayerTeamEvent(&join_player);
	SkinTeamJoin(&join_player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvPlayerDeath(IGameEvent *event)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		ProcessPlayerDeath(event);
	}

	ProcessQuakeDeath(event);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvPlayerSay(IGameEvent *event)
{
	player_t player;
	int user_id = event->GetInt("userid", -1);
	const char *say_string = event->GetString("text", "");

	if (user_id == -1) return;

	player.user_id = user_id;
	if (!FindPlayerByUserID(&player)) return;

	if (!gpManiChatTriggers->PlayerSay(&player, say_string, false, true))
	{
		return;
	}

	// Check for web shortcuts that have been said in game
	if (ProcessWebShortcuts(player.entity, say_string))
	{
		return;
	}

	// Create gpCmd argc structure
	gpCmd->ParseEventSayCommand(say_string);
	const char *cmd = gpCmd->Cmd_Argv(0);
	int	cmd_len = Q_strlen(cmd);
	int	argc = gpCmd->Cmd_Argc();

	gpCmd->DumpCommands();
//	MMsg("cmd [%s]  cmd_len %i  argc %i\n", cmd, cmd_len, argc);

	if (FStrEq(say_string, "nextmap") && !war_mode) {ProcessMaNextMap(&player, "nextmap", 0, M_CCONSOLE); return;}
	else if (FStrEq(say_string, "damage") && !war_mode) {ProcessMaDamage(player.index); return;}
	else if (FStrEq(say_string, "destructive") && !war_mode) {ProcessMaDestruction(player.index); return;}
	else if (FStrEq(say_string, "deathbeam") && !war_mode) {ProcessMaDeathBeam(player.index); return;}
	else if (FStrEq(say_string, "sounds") && !war_mode) {ProcessMaSounds(player.index); return;}
	else if (FStrEq(say_string, "quake") && !war_mode) {ProcessMaQuake(player.index); return;}
	else if (FStrEq(say_string, "settings") && !war_mode) {MENUPAGE_CREATE_FIRST(PlayerSettingsPage, &player, 0, -1); return;}
	else if (FStrEq(say_string, "timeleft") && !war_mode) {ProcessMaTimeLeft(&player, "timeleft", 0, M_CCONSOLE); return;}
	else if (FStrEq(say_string, "listmaps") && !war_mode) 
	{
		ProcessMaListMaps(&player, "listmaps", 0, M_CCONSOLE);
		SayToPlayer(ORANGE_CHAT, &player, "Check your console for the list of maps !!");
	}
	else if ((FStrEq(say_string, "motd") || FStrEq(say_string, "rules")) && !war_mode)
	{
		MRecipientFilter mrf;
		mrf.AddPlayer(player.index);
		DrawMOTD(&mrf);
		return;
	}
	else if (FStrEq(say_string, "votemap") && !war_mode && 
		mani_voting.GetInt() == 1 && 
		mani_vote_allow_user_vote_map.GetInt() == 1) 
	{
		MENUPAGE_CREATE_FIRST(UserVoteMapPage, &player, 0, -1);
		return;
	}
	else if (FStrEq(say_string, "votekick") && !war_mode &&
		!IsLAN() &&
		mani_voting.GetInt() == 1 && 
		mani_vote_allow_user_vote_kick.GetInt() == 1) 
	{
		MENUPAGE_CREATE_FIRST(UserVoteKickPage, &player, 0, -1);
		return;
	}
	else if (FStrEq(say_string, "voteban") && !war_mode &&
		!IsLAN() &&
		mani_voting.GetInt() == 1 && 
		mani_vote_allow_user_vote_ban.GetInt() == 1) 
	{
		MENUPAGE_CREATE_FIRST(UserVoteBanPage, &player, 0, -1);
		return;
	}
	else if (FStrEq(say_string, "thetime") && !war_mode)
	{
		char	time_text[128];
		char	tmp_buf[128];
		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		current_time += (time_t) (mani_adjust_time.GetInt() * 60);

		time_now = localtime(&current_time);
		if (mani_military_time.GetInt() == 1)
		{
			// Miltary 24 hour
			strftime(tmp_buf, sizeof(tmp_buf), "%H:%M:%S",time_now);
		}
		else
		{
			// Standard 12 hour clock
			strftime(tmp_buf, sizeof(tmp_buf), "%I:%M:%S %p",time_now);
		}

		snprintf( time_text, sizeof(time_text), "The time is : %s %s\n", tmp_buf, mani_thetime_timezone.GetString());
		MMsg("The local time [%s]\n", time_text);
		PrintToClientConsole(player.entity, "%s", time_text);
		if (mani_thetime_player_only.GetInt() == 1)
		{
			ClientMsgSinglePlayer(player.entity, 10, 4, "The time is : %s %s", tmp_buf, mani_thetime_timezone.GetString());
		}
		else
		{
			Color white(255,255,255,255);
			ClientMsg(&white, 15, false, 4, "The time is : %s %s", tmp_buf, mani_thetime_timezone.GetString());
		}
	}
	else if (FStrEq(say_string, "ff") && !war_mode)
	{
		char	ff_message[128];

		if (mp_friendlyfire)
		{
			if (mp_friendlyfire->GetInt() == 1 )
			{
				snprintf(ff_message, sizeof(ff_message), "Friendly fire is on");
				PrintToClientConsole(player.entity, "Friendly fire is on\n");
			}
			else
			{
				snprintf(ff_message, sizeof(ff_message), "Friendly fire is off");
				PrintToClientConsole(player.entity, "Friendly fire is off\n");
			}

			if (mani_ff_player_only.GetInt() == 1)
			{
				ClientMsgSinglePlayer(player.entity, 15, 4, "%s", ff_message);
			}
			else
			{
				Color	white(255,255,255,255);
				ClientMsg(&white, 15, false, 4, "%s", ff_message);
			}
		}
	}
	else if (cmd_len > 2 &&
		toupper(say_string[0]) == 'T' &&
		toupper(say_string[1]) == 'O' &&
		toupper(say_string[2]) == 'P')
	{
		if (mani_stats.GetInt() == 0 || war_mode)
		{
			return;
		}

		if (FStrEq(say_string,"TOP")) 
		{
			ShowTopFreePage *ptr = new ShowTopFreePage;
			if (ptr->SetStartRank(10))
			{
				g_menu_mgr.AddFreePage(&player, ptr, 0, mani_stats_top_display_time.GetInt());
				if (!ptr->Render(&player))
				{
					g_menu_mgr.Kill();
				}
			}
			else
			{
				delete ptr;
			}
			return;
		}
		else
		{
			ShowTopFreePage *ptr = new ShowTopFreePage;
			if (ptr->SetStartRank(atoi(&(say_string[3]))))
			{
				g_menu_mgr.AddFreePage(&player, ptr, 0, mani_stats_top_display_time.GetInt());
				if (!ptr->Render(&player))
				{
					g_menu_mgr.Kill();
				}
			}
			else
			{
				delete ptr;
			}
			return;
		}
	}
	else if (argc <= 3 && FStrEq(cmd, "bet"))
	{
		gpManiCSSBetting->PlayerBet(&player);
		return;
	}
	else if (FStrEq(say_string,"rank") && !war_mode)
	{
		if (mani_stats.GetInt() == 0)
		{
			if (!FStrEq(mani_stats_alternative_rank_message.GetString(),""))
			{
				SayToPlayer(ORANGE_CHAT, &player, "%s", mani_stats_alternative_rank_message.GetString());
			}

			return;
		}

		gpManiStats->ShowRank(&player);
	}
	else if (FStrEq(say_string,"bounty") && !war_mode)
	{
		BountyFreePage *ptr = new BountyFreePage;
		g_menu_mgr.AddFreePage(&player, ptr, 0, mani_stats_top_display_time.GetInt());
		if (!ptr->Render(&player))
		{
			g_menu_mgr.Kill();
		}
	}
	else if (FStrEq(say_string,"statsme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		StatsMeFreePage *ptr = new StatsMeFreePage;
		g_menu_mgr.AddFreePage(&player, ptr, 0, MANI_STATS_ME_TIMEOUT);
		if (!ptr->Render(&player, &player))
		{
			g_menu_mgr.Kill();
		}
	}
	else if (FStrEq(say_string,"session") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		SessionFreePage *ptr = new SessionFreePage;
		g_menu_mgr.AddFreePage(&player, ptr, 0, MANI_STATS_ME_TIMEOUT);
		if (!ptr->Render(&player, &player))
		{
			g_menu_mgr.Kill();
		}	
	}
	else if (FStrEq(say_string,"hitboxme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		HitBoxMeFreePage *ptr = new HitBoxMeFreePage;
		g_menu_mgr.AddFreePage(&player, ptr, 0, MANI_STATS_ME_TIMEOUT);
		if (!ptr->Render(&player))
		{
			g_menu_mgr.Kill();
		}	
	}
	else if (FStrEq(say_string,"weaponme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		WeaponMeFreePage *ptr = new WeaponMeFreePage;
		ptr->page = 1;
		g_menu_mgr.AddFreePage(&player, ptr, 0, MANI_STATS_ME_TIMEOUT);
		if (!ptr->Render(&player))
		{
			g_menu_mgr.Kill();
		}
	}
	else if ( FStrEq( say_string, "vote" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (war_mode) return;
		MENUPAGE_CREATE_FIRST(SystemVotemapPage, &player, 0, -1);
		return;
	}

	else if ( FStrEq( say_string, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		MENUPAGE_CREATE_FIRST(RockTheVoteNominateMapPage, &player, 0, -1);
		return;
	}
	else if ( (FStrEq( say_string, "rockthevote" ) || FStrEq( say_string, "rtv" )) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		gpManiVote->ProcessMaRockTheVote(&player);
	}
	else if ( FStrEq( say_string, "favourites" ) && !war_mode)
	{
		// Show web favourites menu
		MENUPAGE_CREATE_FIRST(FavouritesPage, &player, 5, -1);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvPlayerSpawn(IGameEvent *event)
{
	player_t spawn_player;

	spawn_player.user_id = event->GetInt("userid", -1);
	if (spawn_player.user_id == -1) return;
	if (!FindPlayerByUserID(&spawn_player)) return;
	//CBaseEntity *pPlayer = spawn_player.entity->GetUnknown()->GetBaseEntity();
	ProcessSetColour(spawn_player.entity, 255, 255, 255, 255 );

	ForceSkinType(&spawn_player);
	gpManiSpawnPoints->Spawn(&spawn_player);

	gpManiSaveScores->PlayerSpawn(&spawn_player);
	gpManiObserverTrack->PlayerSpawn(&spawn_player);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->PlayerSpawn(&spawn_player);
		gpManiCSSBounty->PlayerSpawn(&spawn_player);
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		gpManiLogDODSStats->PlayerSpawn(&spawn_player);
	}

	// Reset any effects flags
	EffectsClientDisconnect(spawn_player.index - 1, true);
	gpManiVictimStats->PlayerSpawn(&spawn_player); 
	gpManiAntiRejoin->PlayerSpawn(&spawn_player);

	if (mani_tk_protection.GetInt() == 1 && 
		(mp_friendlyfire && mp_friendlyfire->GetInt() == 1) && 
		!war_mode)
	{
		// Handle delayed punishments
		ProcessTKSpawnPunishment(&spawn_player);
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		SkinTeamJoin(&spawn_player);
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiWeaponMgr->PlayerSpawn(&spawn_player);
	}

	// Give grenade to player if unlimited grenades
	if (!war_mode &&
		mani_unlimited_grenades.GetInt() != 0 && 
		sv_cheats && 
		gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(spawn_player.entity), "weapon_hegrenade");
	}

	gpManiWarmupTimer->PlayerSpawn(&spawn_player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvWeaponFire(IGameEvent *event)
{
	char weapon_name[128];
	bool hegrenade = false;

	int user_id = event->GetInt("userid", -1);
	Q_strcpy(weapon_name, event->GetString("weapon", "NULL"));

	if (FStrEq(weapon_name,"hegrenade"))
	{
		hegrenade = true;
	}

	int	i = gpManiTrackUser->GetIndex(user_id);
	if (i != -1)
	{
		edict_t *pPlayerEdict = engine->PEntityOfEntIndex(i);
		if(pPlayerEdict && !pPlayerEdict->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pPlayerEdict );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (!playerinfo->IsHLTV())
				{
					bool is_bot = false;

					if (FStrEq(playerinfo->GetNetworkIDString(),"BOT"))
					{
						is_bot = true;
					}

					// Update stats
					if (gpManiGameType->IsGameType(MANI_GAME_CSS))
					{
						gpManiLogCSSStats->PlayerFired(i - 1, weapon_name, is_bot);
					}

					gpManiStats->CSSPlayerFired(i - 1, is_bot);

					if (punish_mode_list[i - 1].frozen && !is_bot)
					{
						engine->ClientCommand(pPlayerEdict,"drop\n");
					}
					else
					{
						if (!war_mode &&
							hegrenade && 
							(mani_unlimited_grenades.GetInt() != 0 || gpManiWarmupTimer->UnlimitedHE()) &&
							sv_cheats && 
							gpManiGameType->IsGameType(MANI_GAME_CSS) &&
							(playerinfo->GetTeamIndex() == 2 || playerinfo->GetTeamIndex() == 3))
						{
							CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(pPlayerEdict), "weapon_hegrenade");
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvHostageStopsFollowing(IGameEvent *event)
{
	if (war_mode) return;
	// Warn player if hostage has stopped 

	player_t player;

	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;
	if (player.is_bot) return;

	SayToPlayer(LIGHT_GREEN_CHAT, &player, "A hostage has stopped following you!");
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombPlanted(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->BombPlanted(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombDropped(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->BombDropped(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombExploded(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->BombExploded(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombDefused(IGameEvent *event)
{
	player_t player;
	if (war_mode) return;

	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->BombDefused(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombPickUp(IGameEvent *event)
{
	return;
	player_t player;
	if (war_mode) return;

	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvBombBeginDefuse(IGameEvent *event)
{
	player_t player;
	if (war_mode) return;

	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->BombBeginDefuse(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvHostageRescued(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->HostageRescued(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvHostageFollows(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->HostageFollows(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvHostageKilled(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->HostageKilled(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvRoundStart(IGameEvent *event)
{
	EffectsRoundStart();
	ProcessQuakeRoundStart();
	ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_ROUNDSTART);

	if (get_new_timeleft_offset || 	!round_end_found)
	{
		get_new_timeleft_offset = false;
		timeleft_offset = gpGlobals->curtime;
		round_number = 0;
	}

	round_end_found = false;
	gpManiVictimStats->RoundStart();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		sounds_played[i] = 0;
		tw_spam_list[i].index = -99;
		tw_spam_list[i].last_time = -99.0;
		if (mani_player_name_change_reset.GetInt() == 0)
		{
			name_changes[i] = 0;
		}
	}

	round_number ++;

	gpManiGhost->RoundStart();
	gpManiMostDestructive->RoundStart();
	gpManiWarmupTimer->RoundStart();
	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiWeaponMgr->RoundStart();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvRoundEnd(IGameEvent *event)
{
	const char *message = event->GetString("message", "NULL");
	int winning_team = event->GetInt("winner", -1);
//Msg("Round end [%s] Win Team [%i]\n", message, winning_team);
	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->RoundEnd();
	}

	gpManiMostDestructive->RoundEnd();

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		if (winning_team > 1 && winning_team < MANI_MAX_TEAMS)
		{
			gpManiStats->CSSRoundEnd(winning_team, message);
			gpManiCSSObjectives->CSSRoundEnd(winning_team, message);
			team_scores.team_score[winning_team] ++;
		}
	}

	gpManiCSSBounty->CSSRoundEnd(message);
	gpManiCSSBetting->CSSRoundEnd(winning_team);

	ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_ROUNDSTART);

	if(FStrEq("#Game_Commencing", message))
	{
		gpManiSaveScores->GameCommencing();
		gpManiAFK->GameCommencing();
		gpManiAntiRejoin->LevelInit();
		get_new_timeleft_offset = true;
		timeleft_offset = gpGlobals->curtime;
		for (int i = 0; i < MANI_MAX_TEAMS; i++)
		{
			team_scores.team_score[i] = 0;
		}
	}

	round_end_found = true;

	gpManiTeam->RoundEnd();
	gpManiVictimStats->RoundEnd();

	if (mani_stats.GetInt() == 1 && !war_mode)
	{
		gpManiStats->CalculateStats(mani_stats_by_steam_id.GetBool(), true);
	}

	// Reset drug mode
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		punish_mode_list[i].drugged = 0;
	}

	gpManiAFK->RoundEnd();

}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvRoundFreezeEnd(IGameEvent *event)
{
	if (mani_tk_protection.GetInt() == 1 && !war_mode)
	{
		// Start spawn protection timer
		float spawn_protection_start_time;
		spawn_protection_start_time = gpGlobals->curtime;
		// Pre calculate time
		end_spawn_protection_time = spawn_protection_start_time + mani_tk_spawn_time.GetFloat();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvVIPKilled(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("attacker", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->VIPKilled(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvVIPEscaped(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("userid", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;

	gpManiStats->VIPEscaped(&player);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodStatsWeaponAttack(IGameEvent *event)
{
	player_t player;

	if (war_mode) return;
	player.user_id = event->GetInt("attacker", -1);
	if (player.user_id == -1) return;
	if (!FindPlayerByUserID(&player)) return;
	
	int	weapon_id = event->GetInt("weapon", -1);
	gpManiStats->DODSPlayerFired(&player);
	gpManiLogDODSStats->PlayerFired(player.index - 1, weapon_id);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodPointCaptured(IGameEvent *event)
{
	if (war_mode) return;

	const char	*cappers = event->GetString("cappers", "");
//	const char  *point_name = event->GetString("cpname", "");
	const char	*cp_name = event->GetString("cpname", "NULL");

	int		cappers_length = Q_strlen(cappers);

	gpManiStats->DODSPointCaptured(cappers, cappers_length);
	gpManiLogDODSStats->PointCaptured(cappers, cappers_length, cp_name);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodCaptureBlocked(IGameEvent *event)
{
	if (war_mode) return;
	player_t player;

	if (war_mode) return;
	player.index = event->GetInt("blocker", -1);
	if (player.index == -1) return;
	if (!FindPlayerByIndex(&player)) return;

	const	char *cp_name = event->GetString("cpname", "NULL");

	gpManiStats->DODSCaptureBlocked(&player);
	gpManiLogDODSStats->CaptureBlocked(&player, cp_name);

}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodRoundWin(IGameEvent *event)
{
	if (war_mode) return;

	int	winning_team = event->GetInt("team", -1);

	gpManiStats->DODSRoundEnd(winning_team);
	gpManiVictimStats->RoundEnd();
	gpManiLogDODSStats->RoundEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodStatsPlayerDamage(IGameEvent *event)
{
	if (war_mode) return;

	player_t	victim;
	player_t	attacker;

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);

	if (attacker.user_id > 0)
	{
		if (!FindPlayerByUserID(&attacker)) return;
	}

	if (!FindPlayerByUserID(&victim)) return;

	// Log stats
	gpManiLogDODSStats->PlayerHurt(&victim, &attacker, event);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodStatsPlayerKilled(IGameEvent *event)
{
	if (war_mode) return;

	ProcessDODSPlayerDeath(event);
}

//---------------------------------------------------------------------------------
// Purpose: Process this event
//---------------------------------------------------------------------------------
void CAdminPlugin::EvDodGameOver(IGameEvent *event)
{
	if (war_mode) return;

	gpManiVictimStats->RoundEnd();
	gpManiLogDODSStats->RoundEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Handle a player changing name
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessChangeName(player_t *player, const char *new_name, char *old_name)
{
//	MMsg("ProcessChangeName : Name changed to \"%s\" (from \"%s\"\n", new_name, old_name);

	if (war_mode) return;

	char	kick_cmd[512];
	char	ban_cmd[512];

	name_changes[player->index - 1] ++;

	if (mani_player_name_change_threshold.GetInt() != 0)
	{
		if (mani_player_name_change_threshold.GetInt() < name_changes[player->index - 1])
		{
			if (mani_player_name_change_punishment.GetInt() == 0)
			{
				SayToAll(ORANGE_CHAT, false,"Player was kicked for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto kicked for name hacking\n");
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
				LogCommand (NULL, "Kick (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
				engine->ServerCommand(kick_cmd);				
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 1 && !IsLAN())
			{
				// Ban by user id
				SayToAll(ORANGE_CHAT, false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");
				snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->user_id);
				LogCommand(NULL, "Banned (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 2)
			{
				// Ban by user ip address
				SayToAll(ORANGE_CHAT, false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");
				snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->ip_address);
				LogCommand(NULL, "Banned IP (Name change threshold) [%s] [%s] %s", player->name, player->ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 3)
			{
				// Ban by user id and ip address
				SayToAll(ORANGE_CHAT, false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");

				if (!IsLAN())
				{
					snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_player_name_change_ban_time.GetInt(), 
											player->user_id);
					LogCommand(NULL, "Banned (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);	
					engine->ServerCommand("writeid\n");
				}

				snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->ip_address);
				LogCommand(NULL, "Banned IP (Name change threshold) [%s] [%s] %s", player->name, player->ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				name_changes[player->index - 1] = 0;
				return;
			}
		}
	}

	// Update stats data
	gpManiStats->NetworkIDValidated(player);
	gpManiAutoKickBan->ProcessChangeName(player, new_name, old_name);
}

//---------------------------------------------------------------------------------
// Purpose: Handle Reflection of for attacker
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessReflectDamagePlayer
(
 player_t *victim, 
 player_t *attacker,
 IGameEvent *event
 )
{
	char	weapon_name[128];
	int		tk_player_index;

	if (!gpManiGameType->IsTeamPlayAllowed()) return;

	Q_strcpy(weapon_name, event->GetString("weapon","NULL"));

	if (FStrEq("smokegrenade", weapon_name)) return;
	if (FStrEq("flashbang", weapon_name)) return;

	// Get player details
	if (!victim->entity)
	{
		if (!FindPlayerByUserID(victim)) return;
	}

	if (!attacker->entity)
	{
		if (!FindPlayerByUserID(attacker)) return;
	}

	// World hurt victim
	if (attacker->user_id == 0) return;

	// Bot attacked so not deliberate
	if (attacker->is_bot) return;

	int damage_done = event->GetInt("dmg_health",0) + event->GetInt("dmg_armour");

	// Find attacker database
	tk_player_index = -1;
	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), attacker))
		{
			tk_player_index = i;
			break;
		}
	}

	if (tk_player_index == -1)
	{
		// Add this player to list
		CreateNewTKPlayer(attacker->name, attacker->steam_id, attacker->user_id, 0, 0);
		tk_player_index = tk_player_list_size - 1;
	}

	tk_player_list[tk_player_index].team_wounds ++;

	if (tk_player_list[tk_player_index].team_wounds <= mani_tk_team_wound_reflect_threshold.GetInt())
	{
		// No need to punish yet, threshold not broken
		return;
	}

	int	damage_to_inflict = (int) ((float) damage_done * tk_player_list[tk_player_index].team_wound_reflect_ratio);
	tk_player_list[tk_player_index].team_wound_reflect_ratio += mani_tk_team_wound_reflect_ratio_increase.GetFloat();

	// Hurt the player
	int sound_index = rand() % 3;

//	CBaseEntity *m_pCBaseEntity = attacker->entity->GetUnknown()->GetBaseEntity(); 

	int health = 0;
//	health = m_pCBaseEntity->GetHealth();
	health = Prop_GetVal(attacker->entity, MANI_PROP_HEALTH, 0);
	if (health <= 0) return;
		
	health -= ((damage_to_inflict >= 0) ? damage_to_inflict : damage_to_inflict * -1);
	if (health <= 0)
	{
		health = 0;
	}

	Prop_SetVal(attacker->entity, MANI_PROP_HEALTH, health);
//	m_pCBaseEntity->SetHealth(health);

	if (health <= 0)
	{
		// Player dead so slay them
		SlayPlayer(attacker, true, true, true);
	}

	if (esounds)
	{
		Vector pos = attacker->entity->GetCollideable()->GetCollisionOrigin();

		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();
		mrf.AddAllPlayers(max_players);
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			esounds->EmitSound((IRecipientFilter &)mrf, attacker->index - 1, CHAN_AUTO, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, &pos);
		}
		else
		{
			esounds->EmitSound((IRecipientFilter &)mrf, attacker->index - 1, CHAN_AUTO, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, &pos);
		}

	}
}

//---------------------------------------------------------------------------------
// Purpose: Slay any players that are up for being slayed due to tk/tw
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerDeath(IGameEvent * event)
{
	player_t	victim;
	player_t	attacker;
	bool	headshot;
	char	weapon_name[128];
	bool	attacker_exists = true;

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	// Were they on the same team ?
	// Find user id and attacker id
	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);
	headshot = event->GetBool("headshot", false);
	Q_strcpy(weapon_name, event->GetString("weapon", ""));

	if (!FindPlayerByUserID(&victim)) return;
	if (attacker.user_id <= 0 || !FindPlayerByUserID(&attacker))
	{
		attacker_exists = false;
	}

	punish_mode_list[victim.index - 1].no_clip = false;

	EffectsPlayerDeath(&victim);
	gpManiGhost->PlayerDeath(&victim);
	gpManiStats->PlayerDeath(&victim, &attacker, weapon_name, attacker_exists, headshot);
	gpManiCSSBounty->PlayerDeath(&victim, &attacker, attacker_exists);
	gpManiObserverTrack->PlayerDeath(&victim);

	if (mani_show_death_beams.GetInt() != 0)
	{
		ProcessDeathBeam(&attacker, &victim);
	}

	if (!gpManiWarmupTimer->IgnoreTK())
	{
		ProcessTKDeath(&attacker, &victim);
	}

	// Must go after Process TK Death or menu check will not work !!!!
	gpManiVictimStats->PlayerDeath(&victim, &attacker, attacker_exists, headshot, weapon_name);

	// Log stats
	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->PlayerDeath(&victim, &attacker, attacker_exists, headshot, weapon_name);
	}

	gpManiMostDestructive->PlayerDeath(&victim, &attacker, attacker_exists);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiWarmupTimer->PlayerDeath(&victim);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle Dods specific killed event
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessDODSPlayerDeath(IGameEvent * event)
{
	player_t	victim;
	player_t	attacker;
	int		weapon_index;
	bool	attacker_exists = true;

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	// Were they on the same team ?
	// Find user id and attacker id

	victim.user_id = event->GetInt("victim", -1);
	attacker.user_id = event->GetInt("attacker", -1);
	weapon_index = event->GetInt("weapon", -1);

	if (!FindPlayerByUserID(&victim)) return;
	if (attacker.user_id <= 0 || !FindPlayerByUserID(&attacker))
	{
		attacker_exists = false;
	}

	punish_mode_list[victim.index - 1].no_clip = false;

	EffectsPlayerDeath(&victim);
	gpManiGhost->PlayerDeath(&victim);
	gpManiStats->DODSPlayerDeath(&victim, &attacker, weapon_index, attacker_exists);

	if (mani_show_death_beams.GetInt() != 0)
	{
		ProcessDeathBeam(&attacker, &victim);
	}

	if (!gpManiWarmupTimer->IgnoreTK())
	{
		ProcessTKDeath(&attacker, &victim);
	}

	// Must go after Process TK Death or menu check will not work !!!!
	gpManiVictimStats->DODSPlayerDeath(&victim, &attacker, attacker_exists, weapon_index);
	gpManiMostDestructive->PlayerDeath(&victim, &attacker, attacker_exists);
	gpManiLogDODSStats->PlayerDeath(&victim, &attacker, attacker_exists, weapon_index);

}

//---------------------------------------------------------------------------------
// Purpose: Read the stats
//---------------------------------------------------------------------------------
bool CAdminPlugin::HookSayCommand(bool team_say, const CCommand &args)
{

	player_t player;

	if (engine->IsDedicatedServer() && con_command_index == -1) return true;
	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player))
	{
		return true;
	}

	if (player.is_bot) return true;

	gpCmd->ExtractSayCommand(team_say, args);
	if (gpCmd->Cmd_Argc() == 0) return true;

	if (g_menu_mgr.ChatHooked(&player))
	{
		// Block chat
		return false;
	}

	gpManiAFK->NotAFK(player.index - 1);

	if (!gpManiChatTriggers->PlayerSay(&player, gpCmd->Cmd_Args(0), team_say, false))
	{
		return false;
	}

	// Mute player output
	if (punish_mode_list[player.index - 1].muted && !war_mode)
	{
		return false;
	}

/*	MMsg("Trimmed Say [%s]\n", trimmed_say);
	MMsg("No of args [%i]\n", say_argc);
	for (int i = 0; i < say_argc; i++)
	{
		MMsg("Index [%i] [%s]\n", say_argv[i].index, say_argv[i].argv_string);
	}

	MMsg("\n\n");
*/
	bool found_swear_word = false;

	char	*trimmed_string = (char *) gpCmd->Cmd_Args(0);

	if (!war_mode)
	{
		if (mani_filter_words_mode.GetInt() != 0)
		{
			// Process string for swear words
			char	upper_say[2048];

			// Copy in uppercase
			Q_strncpy(upper_say, gpCmd->Cmd_Args(0), sizeof(upper_say));
			Q_strupr(upper_say);
	

			for (int i = 0; i < swear_list_size; i ++)
			{
				char	*str_index;
				swear_list[i].found = false;

				Q_strcpy (swear_list[i].filtered, upper_say);
				while (NULL != (str_index = Q_stristr(swear_list[i].filtered, swear_list[i].swear_word)))
				{
					for (int j = 0; j < swear_list[i].length; j++)
					{
						str_index[j] = '*';
					}

					swear_list[i].found = true;
				}
			}

			for (int i = 0; i < swear_list_size; i ++)
			{
				if (swear_list[i].found)
				{
					int str_length = Q_strlen(gpCmd->Cmd_Args(0));
					for (int j = 0; j < str_length; j ++)
					{
						if (swear_list[i].filtered[j] == '*')
						{
							trimmed_string[j] = '*';
						}
					}

					found_swear_word = true;
				}
			}
		}
	}

	char replacement_string[2048]="";
	if (!CheckForReplacement(&player, gpCmd->Cmd_Argv(0), replacement_string))
	{
		// RCon or client command command executed
		return false;
	}

	if (strcmp(replacement_string, "") != 0)
	{
		gpCmd->SetParam(0, "%s", replacement_string);
	}

	// All say commands begin with an @ symbol
	if (strcmp(gpCmd->Cmd_SayArg0(),"") != 0)
	{
		if (team_say)
		{
			if (gpCmd->HandleCommand(&player, M_TSAY, args) == PLUGIN_STOP) return false;
		}
		else
		{
			if (gpCmd->HandleCommand(&player, M_SAY, args) == PLUGIN_STOP) return false;
		}
	}

	// Normal say command
	// Is swear word in there ?
	if (found_swear_word)
	{
		SayToPlayer(ORANGE_CHAT, &player, "%s", mani_filter_words_warning.GetString());
		return false;
	}

	// Check anti spam
	if (!war_mode && mani_chat_flood_time.GetFloat() > 0.1)
	{
		if (chat_flood[con_command_index] > gpGlobals->curtime)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", mani_chat_flood_message.GetString());
			chat_flood[con_command_index] = gpGlobals->curtime + mani_chat_flood_time.GetFloat() + 3.0;
			return false;
		}

		chat_flood[con_command_index] = gpGlobals->curtime + mani_chat_flood_time.GetFloat();
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: When level changed, check if we want to override it
//---------------------------------------------------------------------------------
bool CAdminPlugin::HookChangeLevelCommand(void)
{
	if (!IsCommandIssuedByServerAdmin()) return false;

	if (override_changelevel > 0)
	{
		char	changelevel_command[128];

		snprintf(changelevel_command, sizeof(changelevel_command), "changelevel %s\n", forced_nextmap);
		engine->ServerCommand(changelevel_command);
		override_changelevel = 0;
		override_setnextmap = false;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_kick command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaKick(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	char kick_cmd[256];

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_KICK, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_KICK))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to kick
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			int j = Q_strlen(target_player_list[i].name) - 1;

			while (j != -1)
			{
				if (target_player_list[i].name[j] == '\0') break;
				if (target_player_list[i].name[j] == ' ') break;
				j--;
			}

			j++;

			snprintf( kick_cmd, sizeof(kick_cmd), "bot_kick \"%s\"\n", &(target_player_list[i].name[j]));
			LogCommand (player_ptr, "bot_kick [%s]\n", target_player_list[i].name);
			engine->ServerCommand(kick_cmd);
			continue;
		}

		PrintToClientConsole(target_player_list[i].entity, "You have been kicked by Admin\n");
		snprintf( kick_cmd, sizeof(kick_cmd), 
					"kickid %i You were kicked by Admin\n", 
					target_player_list[i].user_id);

		LogCommand (player_ptr, "Kick (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].steam_id, kick_cmd);
		engine->ServerCommand(kick_cmd);
		AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminkick_anonymous.GetInt(), "kicked player %s", target_player_list[i].name ); 
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unban command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnBan(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	bool perm_ban = true;
	bool temp_ban = true;

	if (player_ptr)
	{
		// Check if player is admin
		perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
		temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);

		if (!(temp_ban || perm_ban)) return PLUGIN_BAD_ADMIN;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Try steam id next
	bool ban_by_ip = true;

	char	target_steam_id[2048];
	const char *target_string = gpCmd->Cmd_Argv(1);
	Q_strcpy(target_steam_id, target_string);
	if (Q_strlen(target_steam_id) > 6)
	{
		target_steam_id[6] = '\0';
		if (FStruEq(target_steam_id, "STEAM_"))
		{
			ban_by_ip = false;		
		}
	}

	char unban_cmd[128];
	if (!ban_by_ip)
	{
		snprintf( unban_cmd, sizeof(unban_cmd), "removeid %s\n", target_string);
	}
	else
	{
		snprintf( unban_cmd, sizeof(unban_cmd), "removeip %s\n", target_string);
	}

	LogCommand (player_ptr, "%s", unban_cmd);

	engine->ServerCommand(unban_cmd);
	if (!ban_by_ip)
	{
		engine->ServerCommand("writeid\n");
	}
	else
	{
		engine->ServerCommand("writeip\n");
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Unbanned [%s], no confirmation possible", target_string);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ban command (does both IP and User ID
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBan(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	char ban_cmd[256];
	bool perm_ban = true;
	bool temp_ban = true;

	if (player_ptr)
	{
		// Check if player is admin
		perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
		temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);

		if (!(temp_ban || perm_ban)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int time_to_ban = atoi(gpCmd->Cmd_Argv(1));
	if (time_to_ban < 0) time_to_ban = 0;

	// If if only temp ban if it's within limits
	if (!perm_ban)
	{
		if (time_to_ban == 0 || time_to_ban > mani_admin_temp_ban_time_limit.GetInt())
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 2581, "%i", mani_admin_temp_ban_time_limit.GetInt()));
			return PLUGIN_STOP;
		}
	}

	if (IsLAN())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot ban by ID when on LAN or everyone gets banned !!\n");
		return PLUGIN_STOP;
	}

	const char *target_string = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_BAN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to ban
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (time_to_ban == 0)
		{
			// Permanent ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by admin permanently !!\n");
		}
		else
		{
			// X minute ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by Admin for %i minutes\n", time_to_ban);
		}

		snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", time_to_ban, target_player_list[i].user_id);

		LogCommand (player_ptr, "Banned (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].steam_id, ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeid\n");
		AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminban_anonymous.GetInt(), "banned player %s", target_player_list[i].name ); 
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ban command (does both IP and User ID
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBanIP(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	char ban_cmd[256];
	bool perm_ban = true;
	bool temp_ban = true;

	if (player_ptr)
	{
		// Check if player is admin
		perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
		temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);

		if (!(temp_ban || perm_ban)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int time_to_ban = atoi(gpCmd->Cmd_Argv(1));
	if (time_to_ban < 0) time_to_ban = 0;

	// If if only temp ban if it's within limits
	if (!perm_ban)
	{
		if (time_to_ban == 0 || time_to_ban > mani_admin_temp_ban_time_limit.GetInt())
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 2581, "%i", mani_admin_temp_ban_time_limit.GetInt()));
			return PLUGIN_STOP;
		}
	}

	const char *target_string = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_BAN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to ban
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (time_to_ban == 0)
		{
			// Permanent ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by admin permanently !!\n");
		}
		else
		{
			// X minute ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by Admin for %i minutes\n", time_to_ban);
		}

		snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", time_to_ban, target_player_list[i].ip_address);

		LogCommand (player_ptr, "Banned IP (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].ip_address, ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeip\n");
		AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminban_anonymous.GetInt(), "banned player %s", target_player_list[i].name ); 
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_slay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSlay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAY, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	const char *target_string = gpCmd->Cmd_Argv(1);
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SLAY))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to slay
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		SlayPlayer(&(target_player_list[i]), false, true, true);
		LogCommand (player_ptr, "slayed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminslay_anonymous.GetInt(), "slayed player %s", target_player_list[i].name ); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_offset (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaOffset(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, war_mode)) return PLUGIN_BAD_ADMIN;

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	int offset_number = atoi(target_string);

	// Careful :)
	if (offset_number < 0) offset_number = 0;
	if (offset_number > 2000) offset_number = 2000;

	int *offset_ptr;
	offset_ptr = ((int *)player_ptr->entity->GetUnknown() + offset_number);

	LogCommand (player_ptr, "Checked offset [%i] which is set to [%i]\n", offset_number, *offset_ptr);
	SayToPlayer(ORANGE_CHAT, player_ptr, "Offset [%i] = [%i]", offset_number, *offset_ptr);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_teamindex (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTeamIndex(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, war_mode)) return PLUGIN_BAD_ADMIN;

#ifdef __linux__
	SayToPlayer(ORANGE_CHAT, player_ptr, "Linux Server");
#else
	SayToPlayer(ORANGE_CHAT, player_ptr, "Windows Server");
#endif

	LogCommand (player_ptr, "Current index is [%i]\n", player_ptr->team);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Current index is [%i]", player_ptr->team);

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_offsetscan (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaOffsetScan(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, war_mode)) return PLUGIN_BAD_ADMIN;

	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *start_range = gpCmd->Cmd_Argv(2);
	const char *end_range = gpCmd->Cmd_Argv(3);

	int start_offset = atoi(start_range);
	int end_offset = atoi(end_range);

	if (start_offset > end_offset)
	{
		end_offset = atoi(start_range);
		start_offset = atoi(end_range);
	}

#ifdef __linux__
	OutputHelpText(ORANGE_CHAT, player_ptr, "Linux Server");
#else
	OutputHelpText(ORANGE_CHAT, player_ptr, "Windows Server");
#endif

	LogCommand (player_ptr, "Checking offsets %i to %i\n", start_offset, end_offset);
	SayToPlayer(ORANGE_CHAT, player_ptr, "Checking offsets %i to %i", start_offset, end_offset);

	// Careful :)
	if (end_offset > 5000) end_offset = 5000;
	if (start_offset < 0) end_offset = 0;

	int	target_value = atoi(target_string);

	bool found_match = false;
	for (int i = start_offset; i <= end_offset; i++)
	{
		int *offset_ptr;
		offset_ptr = ((int *)player_ptr->entity->GetUnknown() + i);

		if (*offset_ptr == target_value)
		{
			LogCommand (player_ptr, "Offset [%i] = [%i]\n", i, *offset_ptr);
			OutputHelpText(ORANGE_CHAT, player_ptr, "Offset [%i] = [%i]", i, *offset_ptr);
			found_match = true;
		}
	}

	if (!found_match)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Did not find any matches");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_offsetscanf (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaOffsetScanF(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, war_mode)) return PLUGIN_BAD_ADMIN;

	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *start_range = gpCmd->Cmd_Argv(2);
	const char *end_range = gpCmd->Cmd_Argv(3);

	int start_offset = atoi(start_range);
	int end_offset = atoi(end_range);

	if (start_offset > end_offset)
	{
		end_offset = atoi(start_range);
		start_offset = atoi(end_range);
	}

#ifdef __linux__
	OutputHelpText(ORANGE_CHAT, player_ptr, "Linux Server");
#else
	OutputHelpText(ORANGE_CHAT, player_ptr, "Windows Server");
#endif

	LogCommand (player_ptr, "Checking offsets %i to %i\n", start_offset, end_offset);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Checking offsets %i to %i", start_offset, end_offset);

	// Careful :)
	if (end_offset > 5000) end_offset = 5000;
	if (start_offset < 0) end_offset = 0;

	float	target_value = atof(target_string);

	bool found_match = false;
	for (int i = start_offset; i <= end_offset; i++)
	{
		float *offset_ptr;
		offset_ptr = ((float *)player_ptr->entity->GetUnknown() + i);

		if (*offset_ptr == target_value)
		{
			LogCommand (player_ptr, "Offset [%i] = [%f]\n", i, *offset_ptr);
			OutputHelpText(ORANGE_CHAT, player_ptr, "Offset [%i] = [%f]", i, *offset_ptr);
			found_match = true;
		}
	}

	if (!found_match)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Did not find any matches");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_slay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSlap (player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *damage_string = gpCmd->Cmd_Argv(2);

	if (!gpManiGameType->IsSlapAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SLAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int damage = 0;

	if (gpCmd->Cmd_Argc() == 3)
	{
		damage = atoi(damage_string);
		if (damage >= 100)
		{
			damage = 100;
		}
		else if (damage < 0)
		{
			damage = 0;
		}
	}
	else
	{
		damage = 0;
	}

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		ProcessSlapPlayer(&(target_player_list[i]), damage, false);

		if (last_slapped_player != target_player_list[i].index ||
		    last_slapped_time < gpGlobals->curtime - 3)
		{
			LogCommand (player_ptr, "slapped user [%s] [%s] with %i damage\n", target_player_list[i].name, target_player_list[i].steam_id, damage);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminslap_anonymous.GetInt(), "slapped player %s with %i damage", target_player_list[i].name, damage ); 
			}
			last_slapped_player = target_player_list[i].index;
			last_slapped_time = gpGlobals->curtime;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setcash command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSetCash(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaCash(player_ptr, command_name, help_id, command_type, MANI_SET_CASH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_givecash command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveCash(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaCash(player_ptr, command_name, help_id, command_type, MANI_GIVE_CASH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_givecashp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveCashP(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaCash(player_ptr, command_name, help_id, command_type, MANI_GIVE_CASH_PERCENT);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_takecash command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTakeCash(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaCash(player_ptr, command_name, help_id, command_type, MANI_TAKE_CASH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_takecashp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTakeCashP(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaCash(player_ptr, command_name, help_id, command_type, MANI_TAKE_CASH_PERCENT);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_xxxcash derivatives command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCash(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type, const int mode)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CASH, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *amount = gpCmd->Cmd_Argv(2);
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Convert to float and int
	float fAmount = atof(amount);
	int iAmount = atoi(amount);

	if (fAmount < 0.0) fAmount = 0.0;
	if (iAmount < 0) iAmount = 0;

	fAmount *= 0.01;

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on an active team", target_player_list[i].name);
			continue;
		}

		int target_cash = Prop_GetVal(target_player_list[i].entity, MANI_PROP_ACCOUNT, 0);

		int new_cash = 0;

		switch (mode)
		{
			case (MANI_SET_CASH) :	new_cash = iAmount; break;
			case (MANI_GIVE_CASH) :	new_cash = iAmount + target_cash; break;
			case (MANI_GIVE_CASH_PERCENT) :	new_cash = (int) (((float) target_cash) * (fAmount + 1.0)); break;
			case (MANI_TAKE_CASH) :	new_cash = target_cash - iAmount; break;
			case (MANI_TAKE_CASH_PERCENT) :	new_cash = (int) (((float) target_cash) * fAmount); break;
			default : new_cash = target_cash;
		}
			
		if (new_cash < 0) new_cash = 0;
		else if (new_cash > 16000) new_cash = 16000;

		LogCommand (player_ptr, "%s : Player [%s] [%s] had [%i] cash, now has [%i] cash\n", command_name, target_player_list[i].name, target_player_list[i].steam_id, target_cash, new_cash);

		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincash_anonymous.GetInt(), "changed player %s cash reserves", target_player_list[i].name); 
		}

		Prop_SetVal(target_player_list[i].entity, MANI_PROP_ACCOUNT, new_cash);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_sethealth command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSetHealth(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaHealth(player_ptr, command_name, help_id, command_type, MANI_SET_HEALTH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_givehealth command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveHealth(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaHealth(player_ptr, command_name, help_id, command_type, MANI_GIVE_HEALTH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_givehealthp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveHealthP(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaHealth(player_ptr, command_name, help_id, command_type, MANI_GIVE_HEALTH_PERCENT);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_takehealth command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTakeHealth(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaHealth(player_ptr, command_name, help_id, command_type, MANI_TAKE_HEALTH);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_takehealthp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTakeHealthP(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	return this->ProcessMaHealth(player_ptr, command_name, help_id, command_type, MANI_TAKE_HEALTH_PERCENT);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_sethealth command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaHealth(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type, const int mode)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_HEALTH, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *amount = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Convert to float and int
	float fAmount = atof(amount);
	int iAmount = atoi(amount);

	if (fAmount < 0.0) fAmount = 0.0;
	if (iAmount < 0) iAmount = 0;

	fAmount *= 0.01;

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on an active team", target_player_list[i].name);
			continue;
		}

		int target_health;
		target_health = Prop_GetVal(target_player_list[i].entity, MANI_PROP_HEALTH, 0);
//		CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
//		target_health = m_pCBaseEntity->GetHealth();

		int new_health = 0;

		switch (mode)
		{
			case (MANI_SET_HEALTH) :	new_health = iAmount; break;
			case (MANI_GIVE_HEALTH) :	new_health = iAmount + target_health; break;
			case (MANI_GIVE_HEALTH_PERCENT) :	new_health = (int) (((float) target_health) * (fAmount + 1.0)); break;
			case (MANI_TAKE_HEALTH) :	new_health = target_health - iAmount; break;
			case (MANI_TAKE_HEALTH_PERCENT) :	new_health = (int) (((float) target_health) * fAmount); break;
			default : new_health = target_health;
		}
			
		if (new_health < 0) new_health = 0;
		else if (new_health > 999999) new_health = 999999;

		LogCommand (player_ptr, "%s : Player [%s] [%s] had [%i] health, now has [%i] health\n", command_name, target_player_list[i].name, target_player_list[i].steam_id, target_health, new_health);

		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminhealth_anonymous.GetInt(), "changed player %s health to %i", target_player_list[i].name, new_health); 
		}

		if (new_health <= 0)
		{
			SlayPlayer(&(target_player_list[i]), false, false, false);
		}
		else
		{
			Prop_SetVal(target_player_list[i].entity, MANI_PROP_HEALTH, new_health);
//			m_pCBaseEntity->SetHealth(new_health);		
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_blind command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBlind(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *blind_amount_string = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BLIND, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_BLIND))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int blind_amount = 0;

	if (gpCmd->Cmd_Argc() == 3)
	{
		blind_amount = atoi(blind_amount_string);
		if (blind_amount > 255)
		{
			blind_amount = 255;
		}
		else if (blind_amount < 0)
		{
			blind_amount = 255;
		}
	}
	else
	{
		blind_amount = 255;
	}

	// Found some players to blind
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		BlindPlayer(&(target_player_list[i]), blind_amount);
		LogCommand (player_ptr, "%s user [%s] [%s]\n", (blind_amount == 0) ? "unblinded":"blinded", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminblind_anonymous.GetInt(), "%s player %s", (blind_amount == 0) ? "unblinded":"blinded", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_freeze command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFreeze(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FREEZE, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_FREEZE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].frozen == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].frozen)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{
			ProcessFreezePlayer(&(target_player_list[i]), true);
			LogCommand (player_ptr, "froze user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfreeze_anonymous.GetInt(), "froze player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnFreezePlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "defrosted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfreeze_anonymous.GetInt(), "defrosted player %s", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_noclip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaNoClip(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_NO_CLIP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		ProcessNoClipPlayer(&(target_player_list[i]));

		if (punish_mode_list[target_player_list[i].index - 1].no_clip)
		{
			LogCommand (player_ptr, "noclip user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminnoclip_anonymous.GetInt(), "player %s is in no clip mode", target_player_list[i].name); 
		}
		else
		{
			LogCommand (player_ptr, "un-noclip user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminnoclip_anonymous.GetInt(), "player %s is mortal again", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_burn command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBurn(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (!gpManiGameType->IsFireAllowed()) return PLUGIN_STOP;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BURN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_BURN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to burn
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		ProcessBurnPlayer(&(target_player_list[i]), mani_admin_burn_time.GetInt());
		LogCommand (player_ptr, "burned user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminburn_anonymous.GetInt(), "burned player %s", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_drug command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDrug(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);
	if (!gpManiGameType->IsDrugAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_DRUG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DRUG))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to drug
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].drugged == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].drugged)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessDrugPlayer(&(target_player_list[i]), true);
			LogCommand (player_ptr, "drugged user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admindrug_anonymous.GetInt(), "drugged player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnDrugPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-drugged user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admindrug_anonymous.GetInt(), "un-drugged player %s", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_decal command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDecal(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!gpManiGameType->GetAdvancedEffectsAllowed()) return PLUGIN_STOP;

	if (!player_ptr) return PLUGIN_CONTINUE;
	const char *decal_name = gpCmd->Cmd_Argv(1);

	// Check if player is admin
	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, war_mode)) return PLUGIN_BAD_ADMIN;

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	CBaseEntity *pPlayer = player_ptr->entity->GetUnknown()->GetBaseEntity();

	Vector eyepos = CBaseEntity_EyePosition(pPlayer);
	QAngle angles = CBaseEntity_EyeAngles(pPlayer);

	// Get position of entity in eye sight
	Vector vecEnd;
	Vector forward;
	Vector pos;

	AngleVectors( angles, &forward);

	trace_t tr;

	vecEnd = eyepos + (forward * 1024.0f);
	MANI_TraceLineWorldProps  ( eyepos, vecEnd, MASK_SOLID_BRUSHONLY, COLLISION_GROUP_NONE, &tr );
	pos = tr.endpos;

	if (tr.fraction != 1.0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Target entity Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "No target entity");
		return PLUGIN_STOP;
	}

	int	   decal_index = gpManiCustomEffects->GetDecal((char *) decal_name);

	MRecipientFilter filter;
	Vector position;

	if (decal_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Invalid Decal Index for [%s]", decal_name);
		return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "\"x\" \"%.5f\"\n", pos.x);
	OutputToConsole(player_ptr, "\"y\" \"%.5f\"\n", pos.y);
	OutputToConsole(player_ptr, "\"z\" \"%.5f\"\n", pos.z);

	filter.AddAllPlayers(max_players);
	temp_ents->BSPDecal((IRecipientFilter &) filter, 0, &pos, 0, decal_index);	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_give command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGive(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *item_name = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_GIVE, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_GIVE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(target_player_list[i].entity), item_name);

		LogCommand (player_ptr, "gave user [%s] [%s] item [%s]\n", target_player_list[i].name, target_player_list[i].steam_id, item_name);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingive_anonymous.GetInt(), "gave player %s item %s", target_player_list[i].name, item_name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_giveammo command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveAmmo(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *weapon_slot_str = gpCmd->Cmd_Argv(2);
	const char *primary_fire_str = gpCmd->Cmd_Argv(3);
	const char *amount_str = gpCmd->Cmd_Argv(4);
	const char *noise_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_GIVE, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 5) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_GIVE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}


	bool	suppress_noise = false;

	if (gpCmd->Cmd_Argc() == 6)
	{
		if (FStrEq(noise_str,"1"))
		{
			suppress_noise = true;
		}
	}

	int	weapon_slot = atoi(weapon_slot_str);
	int	amount = atoi(amount_str);
	bool primary_fire = false;

	if (FStrEq(primary_fire_str,"1")) primary_fire = true;
	if (amount < 0) amount = 0; else if (amount > 1000) amount = 1000;
	if (weapon_slot < 0) weapon_slot = 0; else if (weapon_slot > 20) weapon_slot = 20;


	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		CBaseEntity *pPlayer = target_player_list[i].entity->GetUnknown()->GetBaseEntity();
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, weapon_slot);
		if (!pWeapon) continue;

		int	ammo_index;

		if (primary_fire)
		{
			ammo_index = CBaseCombatWeapon_GetPrimaryAmmoType(pWeapon);
		}
		else
		{
			ammo_index = CBaseCombatWeapon_GetSecondaryAmmoType(pWeapon);
		}

		CBaseCombatCharacter_GiveAmmo(pCombat, amount, ammo_index, suppress_noise);

		LogCommand (player_ptr, "gave user [%s] [%s] ammo\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingive_anonymous.GetInt(), "gave player %s ammo", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_colour and ma_color command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaColour(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *red_str = gpCmd->Cmd_Argv(2);
	const char *green_str = gpCmd->Cmd_Argv(3);
	const char *blue_str = gpCmd->Cmd_Argv(4);
	const char *alpha_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_COLOUR, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 6) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int red = atoi(red_str);
	int green = atoi(green_str);
	int blue = atoi(blue_str);
	int alpha = atoi(alpha_str);

	if (red > 255) red = 255; else if (red < 0) red = 0;
	if (green > 255) green = 255; else if (green < 0) green = 0;
	if (blue > 255) blue = 255; else if (blue < 0) blue = 0;
	if (alpha > 255) alpha = 255; else if (alpha < 0) red = 0;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		ProcessSetColour(target_player_list[i].entity, red, green, blue, alpha);
		
		LogCommand (player_ptr, "set user color [%s] [%s] to [%i] [%i] [%i] [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, red, blue, green, alpha);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincolor_anonymous.GetInt(), "set player %s color", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_colourweapon and ma_colorweapon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaColourWeapon(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *red_str = gpCmd->Cmd_Argv(2);
	const char *green_str = gpCmd->Cmd_Argv(3);
	const char *blue_str = gpCmd->Cmd_Argv(4);
	const char *alpha_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_COLOUR, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 6) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int red = atoi(red_str);
	int green = atoi(green_str);
	int blue = atoi(blue_str);
	int alpha = atoi(alpha_str);

	if (red > 255) red = 255; else if (red < 0) red = 0;
	if (green > 255) green = 255; else if (green < 0) green = 0;
	if (blue > 255) blue = 255; else if (blue < 0) blue = 0;
	if (alpha > 255) alpha = 255; else if (alpha < 0) red = 0;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		ProcessSetWeaponColour(m_pCBaseEntity, red, green, blue, alpha);
		
		LogCommand (player_ptr, "set user weapon color [%s] [%s] to [%i] [%i] [%i] [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, red, blue, green, alpha);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincolor_anonymous.GetInt(), "set player %s weapon color", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_gravity and ma_gravity command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGravity(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *gravity_string = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_GRAVITY, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_GRAVITY))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int gravity = atof(gravity_string);

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		CBaseEntity *pCBE = EdictToCBE(target_player_list[i].entity);

		// Need to set gravity
		if (Map_CanUseMap(pCBE, MANI_VAR_GRAVITY))
		{
			Map_SetVal(pCBE, MANI_VAR_GRAVITY, (float) (gravity * 0.01));
			LogCommand (player_ptr, "set user gravity [%s] [%s] to [%f]\n", target_player_list[i].name, target_player_list[i].steam_id, gravity);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingravity_anonymous.GetInt(), "set player %s gravity", target_player_list[i].name); 
			}
		}

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rendermode command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRenderMode(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *render_mode_str = gpCmd->Cmd_Argv(2);
	int	render_mode;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_COLOUR, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	render_mode = atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->SetRenderMode((RenderMode_t) render_mode);
		Prop_SetVal(target_player_list[i].entity, MANI_PROP_RENDER_MODE, render_mode);
		
		LogCommand (player_ptr, "set user rendermode [%s] [%s] to [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, render_mode);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincolor_anonymous.GetInt(), "set player %s to render mode %i", target_player_list[i].name,  render_mode); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_renderfx command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRenderFX(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	render_mode;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *render_mode_str = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_COLOUR, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	render_mode = atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->m_nRenderFX = render_mode;
		
		Prop_SetVal(target_player_list[i].entity, MANI_PROP_RENDER_FX, render_mode);
		
		LogCommand (player_ptr, "set user renderfx [%s] [%s] to [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, render_mode);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincolor_anonymous.GetInt(), "set player %s to renderfx %i", target_player_list[i].name,  render_mode); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_timebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTimeBomb(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TIMEBOMB, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_TIMEBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].time_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].time_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnTimeBombPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-timebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admintimebomb_anonymous.GetInt(), "player %s is no longer a time bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessTimeBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player_ptr, "timebomb user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admintimebomb_anonymous.GetInt(), "player %s is now a time bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_firebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFireBomb(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FIREBOMB, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_FIREBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to fire bomb
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].fire_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].fire_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnFireBombPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-firebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfirebomb_anonymous.GetInt(), "player %s is no longer a fire bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessFireBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player_ptr, "timebomb user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfirebomb_anonymous.GetInt(), "player %s is now a fire bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_freezebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFreezeBomb(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_FREEZEBOMB, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_FREEZEBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to freeze bomb
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].freeze_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].freeze_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnFreezeBombPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-freezebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfreezebomb_anonymous.GetInt(), "player %s is no longer a freeze bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessFreezeBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player_ptr, "un-freezebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminfreezebomb_anonymous.GetInt(), "player %s is now a freeze bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_beacon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBeacon(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BEACON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_BEACON))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to turn into beacons
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].beacon == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].beacon)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnBeaconPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-beaconed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminbeacon_anonymous.GetInt(), "player %s is no longer a beacon", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessBeaconPlayer(&(target_player_list[i]), true);
			LogCommand (player_ptr, "beaconed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminbeacon_anonymous.GetInt(), "player %s is now a beacon", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_mute command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaMute(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MUTE, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_MUTE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].muted)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessMutePlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "muted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminmute_anonymous.GetInt(), "muted player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnMutePlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-muted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminmute_anonymous.GetInt(), "un-muted player %s", target_player_list[i].name);
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_teleport command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTeleport(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *x_coords = gpCmd->Cmd_Argv(2);
	const char *y_coords = gpCmd->Cmd_Argv(3);
	const char *z_coords = gpCmd->Cmd_Argv(4);
	int argc = gpCmd->Cmd_Argc();

	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_CONTINUE;


	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (argc != 2 && argc != 5 || 
		(!player_ptr && argc == 2) ||
		(argc == 2 && !CanTeleport(player_ptr))) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	Vector origin;
	Vector *origin2 = NULL;

	if (argc == 5)
	{
		origin.x = atof(x_coords);
		origin.y = atof(y_coords);
		origin.z = atof(z_coords);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_TELEPORT))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	if (argc == 2)
	{
		player_settings_t *player_settings;
	
		player_settings = FindPlayerSettings(player_ptr);
		if (!player_settings) return PLUGIN_STOP;
		if (player_settings->teleport_coords_list_size == 0) return PLUGIN_STOP;

		for (int i = 0; i < player_settings->teleport_coords_list_size; i ++)
		{
			if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
			{
				origin2 = &(player_settings->teleport_coords_list[i].coords);
				break;
			}
		}

		if (!origin2) return PLUGIN_STOP;
		origin = *origin2;
	}

	// Found some players to teleport
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player_ptr;
		
		target_player_ptr = (player_t *) &(target_player_list[i]);

		if (target_player_ptr->is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_DEAD,"%s", target_player_ptr->name));
			continue;
		}
	
		ProcessTeleportPlayer(target_player_ptr, &origin);
		// Stack them
		origin.z += 70;

		LogCommand (player_ptr, "teleported user [%s] [%s]\n", target_player_ptr->name, target_player_ptr->steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminteleport_anonymous.GetInt(), "teleported player %s", target_player_ptr->name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_position command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaPosition(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	// Server can't run this 
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin
	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT, war_mode)) return PLUGIN_BAD_ADMIN;

	CBaseEntity *pPlayer = player_ptr->entity->GetUnknown()->GetBaseEntity();

	Vector pos = player_ptr->player_info->GetAbsOrigin();
	Vector eyepos = CBaseEntity_EyePosition(pPlayer);
	QAngle angles = CBaseEntity_EyeAngles(pPlayer);

	SayToPlayer(ORANGE_CHAT, player_ptr, "Absolute Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	SayToPlayer(ORANGE_CHAT, player_ptr, "Eye Position XYZ = %.5f %.5f %.5f", eyepos.x, eyepos.y, eyepos.z);
	SayToPlayer(ORANGE_CHAT, player_ptr, "Eye Angles XYZ = %.5f %.5f %.5f", angles.x, angles.y, angles.z);
	OutputToConsole(player_ptr, "\"x\" \"%.5f\"\n", pos.x);
	OutputToConsole(player_ptr, "\"y\" \"%.5f\"\n", pos.y);
	OutputToConsole(player_ptr, "\"z\" \"%.5f\"\n", pos.z);

	// Get position of entity in eye sight
	Vector vecEnd;
	Vector forward;

//	pos.z += 24;

	AngleVectors( angles, &forward);

//	forward[2] = 0.0;
//	VectorNormalize( forward );

//	VectorMA( pos, 50.0, forward, pos );
//	VectorMA( pos, 1024.0, forward, vecEnd );

	trace_t tr;

	vecEnd = eyepos + (forward * 1024.0f);
	MANI_TraceLineWorldProps  ( eyepos, vecEnd, MASK_SOLID_BRUSHONLY, COLLISION_GROUP_NONE, &tr );
	pos = tr.endpos;
	if (tr.fraction != 1.0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Target entity Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "No target entity");
	}

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_dropc4 command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDropC4(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_DROPC4, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: This only works on CS Source");
		return PLUGIN_STOP;
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t	bomb_player;

		bomb_player.index = i;
		if (!FindPlayerByIndex(&bomb_player)) continue;
		if (bomb_player.player_info->IsHLTV()) continue;

		if (UTIL_DropC4(bomb_player.entity))
		{
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(GREEN_CHAT, player_ptr, mani_admindropc4_anonymous.GetInt(), "forced player %s to drop the C4", bomb_player.name); 
			}

			LogCommand (player_ptr, "forced c4 drop on player [%s] [%s]\n", bomb_player.name, bomb_player.steam_id);
			break;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rcon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRCon(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	char	rcon_cmd[2048];

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RCON, false)) return PLUGIN_BAD_ADMIN;

	LogCommand (player_ptr, "%s %s\n", command_name, gpCmd->Cmd_Args(1)); 
	snprintf( rcon_cmd, sizeof(rcon_cmd), "%s\n", gpCmd->Cmd_Args(1));
	OutputHelpText(ORANGE_CHAT, player_ptr, "Executed RCON %s", gpCmd->Cmd_Args(1));
	engine->ServerCommand(rcon_cmd);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_browse command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBrowse(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	MRecipientFilter mrf;
	mrf.AddPlayer(player_ptr->index);
	DrawURL(&mrf, "Browser", gpCmd->Cmd_Args(1));
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExec(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC, war_mode) && command_type != M_MENU) return PLUGIN_BAD_ADMIN;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_CEXEC))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	char	client_cmd[2048];

	snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s \"%s\" %s\n", command_name, target_string, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", say_string);
			
	// Found some players to run the command on
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);
		if (target_player->is_bot) continue;
		engine->ClientCommand(target_player->entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_t command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecT(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC, war_mode) && command_type != M_MENU) return PLUGIN_BAD_ADMIN;
	}
	
	char	client_cmd[2048];

	snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", say_string);
			
	// Found some players to run the command on
	for (int i = 1; i <= max_players; i++)
	{
		player_t client_player;
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		if (client_player.team != 2) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_ct command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecCT(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC, war_mode) && command_type != M_MENU) return PLUGIN_BAD_ADMIN;
	}
	
	char	client_cmd[2048];

	snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", say_string);
			
	// Found some players to run the command on
	for (int i = 1; i <= max_players; i++)
	{
		player_t client_player;
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		if (client_player.team != 3) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_all command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecAll(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC, war_mode) && command_type != M_MENU) return PLUGIN_BAD_ADMIN;
	}
	
	char	client_cmd[2048];

	snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", say_string);
			
	// Found some players to run the command on
	for (int i = 1; i <= max_players; i++)
	{
		player_t client_player;
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_spec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecSpec(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (gpCmd->Cmd_Argc() < 2 || !gpManiGameType->IsSpectatorAllowed()) 
	{
		return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}

	if (!gpManiGameType->IsSpectatorAllowed()) return PLUGIN_STOP;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CEXEC, war_mode) && command_type != M_MENU) return PLUGIN_BAD_ADMIN;
	}
	
	char	client_cmd[2048];

	snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", say_string);
			
	int spec_index = gpManiGameType->GetSpectatorIndex();

	// Found some players to run the command on
	for (int i = 1; i <= max_players; i++)
	{
		player_t client_player;
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		if (client_player.team != spec_index) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_users command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUsers(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_players = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, false)) return PLUGIN_BAD_ADMIN;
	}

	char	target_string[512];

	if (gpCmd->Cmd_Argc() < 2) 
	{
		Q_strcpy(target_string, "#ALL");
	}
	else
	{
		Q_strcpy(target_string, target_players);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current User List\n\n");
	OutputToConsole(player_ptr, "A Ghost Name                Steam ID             IP Address       UserID\n");
	OutputToConsole(player_ptr, "------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		bool	is_admin;

		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			continue;
		}

		if (gpManiClient->HasAccess(server_player->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			is_admin = true;
		}
		else
		{
			is_admin = false;
		}

		OutputToConsole(player_ptr, "%s %s %-19s %-20s %-16s %-7i\n",
						(is_admin) ? "*":" ",
						(gpManiGhost->IsGhosting(server_player)) ? " YES ":"     ",
						server_player->name,
						server_player->steam_id,
						server_player->ip_address,
						server_player->user_id);

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_admins command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAdmins(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_players = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, false)) return PLUGIN_BAD_ADMIN;
	}

	char	target_string[512];

	if (gpCmd->Cmd_Argc() < 2) 
	{
		Q_strcpy(target_string, "#ALL");
	}
	else
	{
		Q_strcpy(target_string, target_players);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current Admins with 'admin' flag List\n\n");
	OutputToConsole(player_ptr, "Name                           Admin Name\n");
	OutputToConsole(player_ptr, "------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			continue;
		}

		if (gpManiClient->HasAccess(server_player->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			const char *name = gpManiClient->FindClientName(server_player);
			OutputToConsole(player_ptr, "%-30s %-30s\n",
				server_player->name,
				(name == NULL) ? "No Name":name);
		}
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_rates command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRates(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_players = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (mani_all_see_ma_rates.GetInt() == 0)
		{
			if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_MA_RATES, false)) return PLUGIN_BAD_ADMIN;
		}
	}

	char	target_string[512];

	if (gpCmd->Cmd_Argc() < 2) 
	{
		Q_strcpy(target_string, "#ALL");
	}
	else
	{
		Q_strcpy(target_string, target_players);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current User List with rates\n\n");
	OutputToConsole(player_ptr, "  Name              Steam ID             UserID  rate    cmd    update interp\n");
	OutputToConsole(player_ptr, "-----------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			continue;
		}

		const char * szCmdRate = engine->GetClientConVarValue( server_player->index, "cl_cmdrate" );
		const char * szUpdateRate = engine->GetClientConVarValue( server_player->index, "cl_updaterate" );
		const char * szRate = engine->GetClientConVarValue( server_player->index, "rate" );
		const char * szInterp = engine->GetClientConVarValue( server_player->index, "cl_interp" );
		int nCmdRate = atoi( szCmdRate );
		int nUpdateRate = atoi( szUpdateRate );
		int nRate = atoi( szRate );
		float nInterp = atof( szInterp );

		
		OutputToConsole(player_ptr, "%s %-17s %-20s %-7i %-7i %-6i %-6i %-.2f\n",
					(nCmdRate < 20) ? "*":" ",
					server_player->name,
					server_player->steam_id,
					server_player->user_id,
					nRate,
					nCmdRate,
					nUpdateRate,
					nInterp);

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_config command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaConfig(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *filter = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, false)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current Plugin server var settings\n\n");

    ConCommandBase *pPtr = g_pCVar->GetCommands();
    while (pPtr)
	{
		if (!pPtr->IsCommand())
		{
			const char *name = pPtr->GetName();

			if (NULL != Q_stristr(name, "mani_"))
			{
				if (gpCmd->Cmd_Argc() == 2)
				{
					if (NULL != Q_stristr(name, filter))
					{
						// Found mani cvar filtered
						ConVar *mani_var = g_pCVar->FindVar(name);
						OutputToConsole(player_ptr, "%s %s\n", name, mani_var->GetString());
					}
				}
				else
				{
					// Found mani cvar
					ConVar *mani_var = g_pCVar->FindVar(name);
					OutputToConsole(player_ptr, "%s %s\n", name, mani_var->GetString());
				}
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_saveloc
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSaveLoc(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_STOP;

	if (!player_ptr) return PLUGIN_STOP;

	// Check if player is admin
	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_TELEPORT, war_mode)) return PLUGIN_BAD_ADMIN;

	ProcessSaveLocation(player_ptr);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Current location saved, any players will be teleported here");

	return PLUGIN_STOP;
}



//---------------------------------------------------------------------------------
// Purpose: Process the ma_war
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaWar(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *option = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_WAR, false)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() == 1)
	{
		if (mani_war_mode.GetInt() == 1)
		{
			mani_war_mode.SetValue(0);
			AdminSayToAll(GREEN_CHAT, player_ptr, 1, "Disabled War Mode"); 
			LogCommand (player_ptr, "Disable war mode\n");
		}
		else
		{
			AdminSayToAll(GREEN_CHAT, player_ptr, 1, "Enabled War Mode"); 
			LogCommand (player_ptr, "Enable war mode\n");
			mani_war_mode.SetValue(1);
		}	

		return PLUGIN_STOP;
	}

	int	option_val = atoi(option);

	if (option_val == 0)
	{
		// War mode off please
		mani_war_mode.SetValue(0);
		AdminSayToAll(GREEN_CHAT, player_ptr, 1, "Disabled War Mode"); 
		LogCommand (player_ptr, "Disable war mode\n");
	}
	else if (option_val == 1)
	{
		// War mode on please
		AdminSayToAll(GREEN_CHAT, player_ptr, 1, "Enabled War Mode"); 
		LogCommand (player_ptr, "Enable war mode\n");
		mani_war_mode.SetValue(1);
	}
	
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the timeleft command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTimeLeft(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	char	time_string[256]="";
	char	fraglimit_string[256]="";
	char	winlimit_string[256]="";
	char	maxrounds_string[256]="";
	char	final_string[1024];
	int	minutes_portion = 0;
	int 	seconds_portion = 0;
	int	time_left;
	bool	no_timelimit = true;
	bool	follow_string = false;
	bool	last_round = false;

	// DEBUG to trace timeleft error
	if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		LogCommand(player_ptr, "timeleft triggered\n");
	}

	if (mp_timelimit)
	{
		if (mp_timelimit->GetInt() != 0)
		{
			time_left = (int) ((mp_timelimit->GetFloat() * 60) - (gpGlobals->curtime - timeleft_offset));
			if (time_left < 0)
			{
				time_left = 0;
			}

			if (time_left != 0)
			{
				minutes_portion = time_left / 60;
				seconds_portion = time_left - (minutes_portion * 60);
			}

			if (gpManiGameType->IsGameType(MANI_GAME_CSS) && time_left == 0)
			{
				last_round = true;
			}
			else
			{
				snprintf(time_string, sizeof(time_string), "Timeleft %i:%02i", minutes_portion, seconds_portion, gpGlobals->curtime);	
			}

			follow_string = true;
			no_timelimit = false;
		}
		else 
		{
			snprintf(time_string, sizeof(time_string), "No timelimit for map");
		}
	}

	if (mp_fraglimit && mp_fraglimit->GetInt() != 0)
	{
		if (follow_string)
		{
			snprintf(fraglimit_string, sizeof(fraglimit_string), ", or change map after player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
		}
		else
		{
			snprintf(fraglimit_string, sizeof(fraglimit_string), "Map will change after a player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
			follow_string = true;
		}

	}

	if (mp_winlimit && mp_winlimit->GetInt() != 0)
	{
		if (follow_string)
		{
			snprintf(winlimit_string, sizeof(winlimit_string), ", or change map after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
		}
		else
		{
			snprintf(winlimit_string, sizeof(winlimit_string), "Map will change after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (mp_maxrounds && mp_maxrounds->GetInt() != 0)
	{
		if (follow_string)
		{
			snprintf(maxrounds_string, sizeof(maxrounds_string), ", or change map after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
		}
		else
		{
			snprintf(maxrounds_string, sizeof(maxrounds_string), "Map will change after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (!last_round)
	{
		if (no_timelimit && follow_string)
		{
			snprintf(final_string, sizeof(final_string), "%s%s%s", fraglimit_string, winlimit_string, maxrounds_string);
		}
		else
		{
			snprintf(final_string, sizeof(final_string), "%s%s%s%s", time_string, fraglimit_string, winlimit_string, maxrounds_string);
		}
	}
	else
	{
		snprintf(final_string, sizeof(final_string), "This is the last round !!");
	}

	if (player_ptr)
	{		
		if (mani_timeleft_player_only.GetInt() == 1)
		{
			SayToPlayer(GREEN_CHAT, player_ptr,"%s", final_string);
		}
		else
		{
			SayToAll(ORANGE_CHAT, false,"%s", final_string);
		}	
	}
	else
	{
		OutputToConsole(player_ptr, "%s\n", final_string);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_settings command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSettings(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	player_settings_t *player_settings;

	player_settings = FindPlayerSettings(player_ptr);
	if (!player_settings) return PLUGIN_STOP;

	OutputToConsole(player_ptr, "Your current settings are\n\n");
	OutputToConsole(player_ptr,		"Display Damage Stats    (%s)\n", (player_settings->damage_stats) ? "On":"Off");
	if (mani_quake_sounds.GetInt() == 1)
	{
		OutputToConsole(player_ptr,	"Quake Style Sounds      (%s)\n", (player_settings->quake_sounds) ? "On":"Off");
		OutputToConsole(player_ptr,	"Server Sounds           (%s)\n", (player_settings->server_sounds) ? "On":"Off");
	}

	if (player_settings->teleport_coords_list_size != 0)
	{
		// Dump maps stored for teleport
		OutputToConsole(player_ptr, "Current maps you have teleport locations saved on :-\n");
		for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
		{
			OutputToConsole(player_ptr, "[%s] ", player_settings->teleport_coords_list[i].map_name);
		}

		OutputToConsole(player_ptr, "\n");
	}

	return PLUGIN_STOP;
}







//---------------------------------------------------------------------------------
// Purpose: Turn off the overview map function
//---------------------------------------------------------------------------------
void	CAdminPlugin::TurnOffOverviewMap(void)
{

	bool	player_found = false;
	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->IsHLTV())
				{
					continue;
				}

				if (playerinfo->IsDead())
				{
					// Force overview mode to 0 for dead clients
					player_found = true;
					mrf.AddPlayer(i);
				}
			}
		}
	}

	if (player_found)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, vgui_message_index );
		msg_buffer->WriteString("overview");
		msg_buffer->WriteByte(0); // Turn off 'overview' map
		msg_buffer->WriteByte(0); // Count of 0
		engine->MessageEnd();

		msg_buffer = engine->UserMessageBegin( &mrf, vgui_message_index );
		msg_buffer->WriteString("specmenu");
		msg_buffer->WriteByte(0); // Turn off 'overview' map
		msg_buffer->WriteByte(0); // Count of 0
		engine->MessageEnd();
	}

	return;
}

//*******************************************************************************
//
// New Console Commands
//
//*******************************************************************************
SCON_COMMAND(ma_kick, 2157, MaKick, true);
SCON_COMMAND(ma_ban, 2169, MaBan, true);
SCON_COMMAND(ma_banip, 2171, MaBanIP, true);
SCON_COMMAND(ma_unban, 2173, MaUnBan, true);
SCON_COMMAND(ma_slay, 2163, MaSlay, false);
SCON_COMMAND(ma_slap, 2025, MaSlap, false);
SCON_COMMAND(ma_setcash, 2029, MaSetCash, false);
SCON_COMMAND(ma_givecash, 2031, MaGiveCash, false);
SCON_COMMAND(ma_givecashp, 2033, MaGiveCashP, false);
SCON_COMMAND(ma_takecash, 2035, MaTakeCash, false);
SCON_COMMAND(ma_takecashp, 2037, MaTakeCashP, false);
SCON_COMMAND(ma_sethealth, 2039, MaSetHealth, false);
SCON_COMMAND(ma_givehealth, 2041, MaGiveHealth, false);
SCON_COMMAND(ma_givehealthp, 2043, MaGiveHealthP, false);
SCON_COMMAND(ma_takehealth, 2045, MaTakeHealth, false);
SCON_COMMAND(ma_takehealthp, 2047, MaTakeHealthP, false);
SCON_COMMAND(ma_blind, 2049, MaBlind, false);
SCON_COMMAND(ma_freeze, 2051, MaFreeze, false);
SCON_COMMAND(ma_noclip, 2053, MaNoClip, false);
SCON_COMMAND(ma_burn, 2055, MaBurn, false);
SCON_COMMAND(ma_drug, 2075, MaDrug, false);
SCON_COMMAND(ma_give, 2071, MaGive, false);
SCON_COMMAND(ma_giveammo, 2073, MaGiveAmmo, false);
SCON_COMMAND(ma_give_ammo, 2073, MaGiveAmmo, false);
SCON_COMMAND(ma_gravity, 2057, MaGravity, false);
SCON_COMMAND(ma_color, 2059, MaColour, false);
SCON_COMMAND(ma_colour, 2061, MaColour, false);
SCON_COMMAND(ma_colorweapon, 2063, MaColourWeapon, false);
SCON_COMMAND(ma_colourweapon, 2065, MaColourWeapon, false);
SCON_COMMAND(ma_render_mode, 2067, MaRenderMode, false);
SCON_COMMAND(ma_render_fx, 2069, MaRenderFX, false);
SCON_COMMAND(ma_timebomb, 2081, MaTimeBomb, false);
SCON_COMMAND(ma_firebomb, 2083, MaFireBomb, false);
SCON_COMMAND(ma_freezebomb, 2083, MaFreezeBomb, false);
SCON_COMMAND(ma_beacon, 2087, MaBeacon, false);
SCON_COMMAND(ma_mute, 2089, MaMute, false);
SCON_COMMAND(ma_teleport, 2091, MaTeleport, false);
SCON_COMMAND(ma_cexec, 2023, MaCExec, false);
SCON_COMMAND(ma_cexec_t, 0, MaCExecT, false);
SCON_COMMAND(ma_cexec_ct, 0, MaCExecCT, false);
SCON_COMMAND(ma_cexec_all, 0, MaCExecAll, false);
SCON_COMMAND(ma_cexec_spec, 0, MaCExecSpec, false);
SCON_COMMAND(ma_users, 2179, MaUsers, true);
SCON_COMMAND(ma_admins, 2231, MaAdmins, true);
SCON_COMMAND(ma_rates, 2177, MaRates, true);
SCON_COMMAND(ma_showsounds, 2181, MaShowSounds, false);
SCON_COMMAND(ma_play, 2137, MaPlaySound, false);

SCON_COMMAND(ma_dropc4, 2101, MaDropC4, false);
SCON_COMMAND(ma_war, 2121, MaWar, true);
SCON_COMMAND(ma_config, 2183, MaConfig, true);
SCON_COMMAND(ma_timeleft, 2185, MaTimeLeft, true);
SCON_COMMAND(ma_help, 2219, MaHelp, true);

CON_COMMAND(ma_version, "Prints the version of the plugin")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	MMsg( "%s\n", mani_version );
	MMsg( "Server Tickrate %i\n", server_tickrate);
#ifdef WIN32
		MMsg("Windows server\n");
#else
		MMsg("Linux server\n");
#endif
	return;
}

CON_COMMAND(ma_game, "Prints the game type in use")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	const char *game_type = serverdll->GetGameDescription();

	MMsg( "Game Type = [%s]\n", game_type );

	return;
}

CONVAR_CALLBACK_FN(WarModeChanged)
{
	if (!FStrEq(pOldString, mani_war_mode.GetString()))
	{
		if (atoi(mani_war_mode.GetString()) == 0)
		{
			war_mode = false;
		}
		else
		{
			war_mode = true;
		}

		gpManiAFK->Load();
	}
}

CONVAR_CALLBACK_FN(ManiAdminPluginVersion)
{
	if (!FStrEq(pOldString, mani_admin_plugin_version.GetString()))
	{
		mani_admin_plugin_version.SetValue(PLUGIN_CORE_VERSION);
	}
}

CONVAR_CALLBACK_FN(ManiTickrate)
{
	if (!FStrEq(pOldString, mani_tickrate.GetString()))
	{
		mani_tickrate.SetValue(server_tickrate);
	}
}

CONVAR_CALLBACK_FN(ManiCSStackingNumLevels)
{
	if (!FStrEq(pOldString, mani_tickrate.GetString()))
	{
		if (cs_stacking_num_levels)
		{
			cs_stacking_num_levels->m_fMaxVal = mani_tickrate.GetFloat();
			cs_stacking_num_levels->SetValue(mani_tickrate.GetInt());
		}
	}
}

CONVAR_CALLBACK_FN(ManiUnlimitedGrenades)
{
	if (!FStrEq(pOldString, mani_unlimited_grenades.GetString()) && 
		gpManiGameType->IsGameType(MANI_GAME_CSS) && 
		sv_cheats &&
		!war_mode)
	{
		if (mani_unlimited_grenades.GetInt() == 1)
		{
			SayToAll(GREEN_CHAT, false, "Unlimited grenades enabled !!");
			for (int i = 1; i <= max_players; i++)
			{
				player_t player;

				player.index = i;
				if (!FindPlayerByIndex(&player)) continue;
				if (player.is_dead) continue;
				if (player.team == gpManiGameType->GetSpectatorIndex()) continue;

				CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(player.entity), "weapon_hegrenade");
			}
		}
		else
		{
			SayToAll(GREEN_CHAT, false, "Unlimited grenades disabled");
		}
	}
}

CONVAR_CALLBACK_FN(ManiStatsBySteamID)
{
	player_t	player;

	// Reset player settings
	ResetActivePlayers();
	// Reload players that are on server for change of mode
	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		// Don't add player if steam id is not confirmed
		if (mani_stats_by_steam_id.GetInt() == 1 && FStrEq(player.steam_id, "STEAM_ID_PENDING")) continue;
		
		gpManiStats->NetworkIDValidated(&player);
	}
}
