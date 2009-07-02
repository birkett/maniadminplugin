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
#include "dlls/iplayerinfo.h"
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
#include "mani_save_scores.h"
#include "mani_team_join.h"
#include "mani_afk.h"
#include "mani_vote.h"
#include "mani_ping.h"
#include "mani_mysql.h"
#include "mani_mysql_thread.h"
#include "mani_vfuncs.h"
#include "mani_mainclass.h"
#include "mani_callback_sourcemm.h"
#include "mani_callback_valve.h"
#include "mani_sourcehook.h"
#include "mani_help.h"
#include "mani_commands.h"
#include "mani_sigscan.h"
#include "mani_globals.h"

#include "shareddefs.h"
#include "cbaseentity.h"

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

cheat_cvar_t	*cheat_cvar_list = NULL;
cheat_cvar_t	*cheat_cvar_list2 = NULL;
cheat_pinger_t	cheat_ping_list[MANI_MAX_PLAYERS];
int				cheat_cvar_list_size = 0;
int				cheat_cvar_list_size2 = 0;

// General sounds
char *menu_select_exit_sound="buttons/combine_button7.wav";
char *menu_select_sound="buttons/button14.wav";



CAdminPlugin g_ManiAdminPlugin;
CAdminPlugin *gpManiAdminPlugin;


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

static void ManiAdminPluginVersion ( ConVar *var, char const *pOldString );
static void ManiTickrate ( ConVar *var, char const *pOldString );
static void WarModeChanged ( ConVar *var, char const *pOldString );
static void ManiStatsBySteamID ( ConVar *var, char const *pOldString );
static void ManiCSStackingNumLevels ( ConVar *var, char const *pOldString );
static void ManiUnlimitedGrenades ( ConVar *var, char const *pOldString );

ConVar mani_admin_plugin_version ("mani_admin_plugin_version", PLUGIN_CORE_VERSION, FCVAR_REPLICATED | FCVAR_NOTIFY, "This is the version of the plugin", ManiAdminPluginVersion); 
ConVar mani_war_mode ("mani_war_mode", "0", 0, "This defines whether war mode is enabled or disabled (1 = enabled)", true, 0, true, 1, WarModeChanged); 
ConVar mani_stats_by_steam_id ("mani_stats_by_steam_id", "1", 0, "This defines whether the steam id is used or name is used to organise the stats (1 = steam id)", true, 0, true, 1, ManiStatsBySteamID); 
ConVar mani_tickrate ("mani_tickrate", "", FCVAR_REPLICATED | FCVAR_NOTIFY, "Server tickrate information", ManiTickrate);
ConVar mani_cs_stacking_num_levels ("mani_cs_stacking_num_levels", "1", 0, "Set number of players that can build a stack", true, 1, true, 50, ManiCSStackingNumLevels);
ConVar mani_unlimited_grenades ("mani_unlimited_grenades", "0", 0, "0 = normal CSS mode, 1 = Grenades replenished after throw (CSS Only)", true, 0, true, 1, ManiUnlimitedGrenades);

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
bool		change_team;
float		change_team_time;
float		timeleft_offset;
bool		get_new_timeleft_offset;
bool		round_end_found;
bool		first_map_loaded = false;
char		custom_map_config[512]="";
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

lang_trans_t		*lang_trans_list;

gimp_t				*gimp_phrase_list;

int	rcon_list_size;
int	swear_list_size;

int	cexec_list_size;
int	cexec_t_list_size;
int	cexec_ct_list_size;
int	cexec_spec_list_size;
int	cexec_all_list_size;

int	lang_trans_list_size;

int gimp_phrase_list_size;

float	last_cheat_check_time;

int	level_changed;
int	message_type;
float	test_val;


ConVar	*mp_friendlyfire; 
ConVar	*mp_freezetime;
ConVar	*mp_winlimit; 
ConVar	*mp_maxrounds; 
ConVar	*mp_timelimit; 
ConVar	*mp_fraglimit; 
ConVar	*mp_limitteams; 
ConVar	*sv_lan; 
ConVar	*sv_cheats; 
ConVar	*sv_alltalk; 
ConVar	*mp_restartgame; 
ConVar	*sv_gravity; 
ConVar  *hostname;
ConVar	*cs_stacking_num_levels;
ConVar  *phy_pushscale;
ConVar	*vip_version;

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

	lang_trans_list = NULL;
	lang_trans_list_size = 0;

	gimp_phrase_list = NULL;
	gimp_phrase_list_size = 0;

	war_mode = false;
	InitCheatPingList();

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

	change_team = false;
	timeleft_offset = 0;
	get_new_timeleft_offset = true;
	trigger_changemap = false;
	round_end_found = false;

	InitMaps();
	InitPlayerSettingsLists();
	gpManiIGELCallback = this;
	gpManiAdminPlugin = this;

#ifndef SOURCEMM
	SourceHook_InitSourceHook();
#endif

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
	if (mysql_thread)
	{
		mysql_thread->Unload();
		delete mysql_thread;
		mysql_thread = NULL;
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
	if (IsTampered())
	{
		ShowTampered();
		return false;
	}
	
	gpManiTrackUser->Load();
	gpManiGameType->Init();
	gpCmd->Load();

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

	if (gpManiGameType->GetAdvancedEffectsAllowed())
	{
		temp_ents_cc = SH_GET_CALLCLASS(temp_ents);
	}

	const char *game_type = serverdll->GetGameDescription();

	MMsg("Game Type [%s]\n", game_type);

	InitCheatPingList();
	UpdateCurrentPlayerList();
	gpManiVictimStats->RoundStart();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		menu_confirm[i].in_use = false;
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;
		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	next_ping_check = 0.0;
	mp_friendlyfire = cvar->FindVar( "mp_friendlyfire");
	mp_freezetime = cvar->FindVar( "mp_freezetime");
	mp_winlimit = cvar->FindVar( "mp_winlimit");
	mp_maxrounds = cvar->FindVar( "mp_maxrounds");
	mp_timelimit = cvar->FindVar( "mp_timelimit");
	mp_fraglimit = cvar->FindVar( "mp_fraglimit");
	mp_limitteams = cvar->FindVar( "mp_limitteams");
	mp_restartgame = cvar->FindVar( "mp_restartgame");
	sv_lan = cvar->FindVar( "sv_lan");
	sv_gravity = cvar->FindVar( "sv_gravity");
	cs_stacking_num_levels = cvar->FindVar( "cs_stacking_num_levels");

	sv_cheats = cvar->FindVar( "sv_cheats");
	sv_alltalk = cvar->FindVar( "sv_alltalk");
	hostname = cvar->FindVar( "hostname");
	phy_pushscale = cvar->FindVar( "phys_pushscale");
	vip_version = cvar->FindVar("vip_version");

	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;
	change_team = false;
	trigger_changemap = false;
	Q_strcpy(custom_map_config,"");

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

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
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


	if (first_map_loaded)
	{
		if (!LoadLanguage())
		{
			return false;
		}

		gpManiNetIDValid->Load();
		gpManiClient->Init();
		InitPanels();	// Fails if plugin loaded on server startup but placed here in case user runs plugin_load
		ResetActivePlayers();
		LoadQuakeSounds();
		LoadCronTabs();
		LoadAdverts();
		LoadMaps("Unknown"); //host_maps cvar is borked :/
		LoadWebShortcuts();
		ResetLogCount();
		LoadCheatList();
		LoadCommandList();
		LoadSounds();
		LoadSkins();
		SkinResetTeamID();
		LoadWeapons("Unknown");
		FreeTKPunishments();
		gpManiGhost->Init();
		gpManiCustomEffects->Init();
		gpManiMapAdverts->Init();
		gpManiReservedSlot->LevelInit();
		gpManiAutoKickBan->LevelInit();
		gpManiSpawnPoints->Load(current_map);
		gpManiSprayRemove->Load();
	}

	filesystem->CreateDirHierarchy( "./cfg/mani_admin_plugin/data/");

	// Work out server tickrate
#ifdef __linux__
	server_tickrate = (int) ((1.0 / serverdll->GetTickInterval()) + 0.5);
#else
	server_tickrate = (int) (1.0 / serverdll->GetTickInterval());
#endif
	mani_tickrate.SetValue(server_tickrate);

	// Hook our changelevel here
	//HOOKVFUNC(engine, 0, OrgEngineChangeLevel, ManiChangeLevelHook);
	//HOOKVFUNC(engine, 43, OrgEngineEntityMessageBegin, ManiEntityMessageBeginHook);
	//HOOKVFUNC(engine, 44, OrgEngineMessageEnd, ManiMessageEndHook);
	g_ManiSMMHooks.HookVFuncs();

	mysql_thread = new ManiMySQLThread();

	// Fire up the database
	mysql_thread->Load();

	g_PluginLoaded = true;
	g_PluginLoadedOnce = true;

	if (gpManiAdminPlugin->IsTampered())
	{
		gpManiAdminPlugin->ShowTampered();
		return true;
	}
	else
	{
		this->InitEvents();
	}

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

	FreeMenu();
	FreeCronTabs();
	FreeAdverts();
	FreeWebShortcuts();
	FreeMaps();
	FreePlayerSettings();
	FreePlayerNameSettings();
	FreeCommandList();
	FreeSounds();
	FreeSkins();
	FreeTeamList();
	FreeTKPunishments();

	if (mysql_thread)
	{
		mysql_thread->Unload();
		delete mysql_thread;
		mysql_thread = NULL;
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

	FreeList ((void **) &gimp_phrase_list, &gimp_phrase_list_size);
	FreeList ((void **) &cheat_cvar_list, &cheat_cvar_list_size);
	FreeList ((void **) &cheat_cvar_list2, &cheat_cvar_list_size2);

	trigger_changemap = false;
	FreeLanguage();
	g_PluginLoaded = false;
	gpManiStats->Unload();
	gpManiTeamJoin->Unload();
	gpManiAFK->Unload();
	gpManiSaveScores->Unload();
	gpManiVote->Unload();
	gameeventmanager->RemoveListener(gpManiIGELCallback);

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
	char	gimp_phrase[256];
	int		total_load_index;

	MMsg("********************************************************\n");
	MMsg("************* Mani Admin Plugin Level Init *************\n");
	MMsg("********************************************************\n");

	gpManiTrackUser->LevelInit();

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

	FreeList ((void **) &gimp_phrase_list, &gimp_phrase_list_size);

	mp_friendlyfire = cvar->FindVar( "mp_friendlyfire");
	mp_freezetime = cvar->FindVar( "mp_freezetime");
	mp_winlimit = cvar->FindVar( "mp_winlimit");
	mp_maxrounds = cvar->FindVar( "mp_maxrounds");
	mp_timelimit = cvar->FindVar( "mp_timelimit");
	mp_fraglimit = cvar->FindVar( "mp_fraglimit");
	mp_limitteams = cvar->FindVar( "mp_limitteams");
	mp_restartgame = cvar->FindVar( "mp_restartgame");
	sv_lan = cvar->FindVar( "sv_lan");
	sv_gravity = cvar->FindVar( "sv_gravity");
	cs_stacking_num_levels = cvar->FindVar( "cs_stacking_num_levels");

	sv_cheats = cvar->FindVar( "sv_cheats");
	sv_alltalk = cvar->FindVar( "sv_alltalk");
	hostname = cvar->FindVar( "hostname");
	phy_pushscale = cvar->FindVar( "phys_pushscale");
	vip_version = cvar->FindVar("vip_version");

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
	InitCheatPingList();
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
		menu_confirm[i].in_use = false;
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;
		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	change_team = false;

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


	LoadWeapons(pMapName);
	gpManiClient->Init();
	LoadQuakeSounds();
	LoadCronTabs();
	LoadAdverts();
	LoadMaps(pMapName);
	LoadWebShortcuts();
	LoadCheatList();
	LoadCommandList();
	LoadSounds();
	LoadSkins();
	FreeTKPunishments();
	gpManiMapAdverts->Init();
	gpManiAutoKickBan->LevelInit();
	gpManiReservedSlot->LevelInit();
	gpManiSpawnPoints->LevelInit(current_map);
	gpManiSprayRemove->LevelInit();

	//Get gimp phrase list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/gimpphrase.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load gimpphrase.txt\n");
	}
	else
	{
//		MMsg("Gimp phrase list\n");
		while (filesystem->ReadLine (gimp_phrase, sizeof(gimp_phrase), file_handle) != NULL)
		{
			if (!ParseLine(gimp_phrase, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &gimp_phrase_list, sizeof(gimp_t), &gimp_phrase_list_size);
			Q_strcpy(gimp_phrase_list[gimp_phrase_list_size - 1].phrase, gimp_phrase);
//			MMsg("[%s]\n", gimp_phrase);
		}

		filesystem->Close(file_handle);
	}


	//Get swear word list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/wordfilter.txt", mani_path.GetString());
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
	Q_snprintf( map_config_filename, sizeof(map_config_filename), "./cfg/%s/map_config/%s.cfg", mani_path.GetString(), pMapName);
	if (filesystem->FileExists(map_config_filename))
	{
		Q_snprintf(custom_map_config, sizeof(custom_map_config), "exec ./%s/map_config/%s.cfg\n", mani_path.GetString(), pMapName);
//		MMsg("Custom map config [%s] found for this map\n", custom_map_config);
	}
	else
	{
//		MMsg("No custom map config found for this map\n");
		Q_strcpy(custom_map_config,"");
	}

	//Get rcon list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/rconlist.txt", mani_path.GetString());
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
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_player.txt", mani_path.GetString());
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
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_all.txt", mani_path.GetString());
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
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_t.txt", mani_path.GetString());
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
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_ct.txt", mani_path.GetString());
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
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_spec.txt", mani_path.GetString());
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

	if (this->IsTampered())
	{
		this->ShowTampered();
		return;
	}
	else
	{
		this->InitEvents();
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		ResetWeaponCount();
	}

	time_t current_time;

	time(&current_time);

	int day = current_time / (60 * 60 * 24);

//	MMsg("time = [%i] days = [%i]\n", current_time, day);

	MMsg("********************************************************\n");
	MMsg(" Mani Admin Plugin Level Init Time = %.3f seconds\n", ManiGetTimerDuration(total_load_index));
	MMsg("********************************************************\n");

//	if (query->ExecuteQuery("SELECT username FROM phpbb_users", &count, res_ptr))
/*	if (query->ExecuteQuery("SHOW tables", &count, res_ptr))
	{
		MMsg("Row count = [%i]\n", count);
/*		for ( x = 0 ; fd = mysql_fetch_field( res_ptr ) ; x++ )
				strcpy( aszFlds[ x ], fd->name ) ;
*/
 
/*		l = 1;
		while (row = query->FetchRow())
		{
			if (row[0])
			MMsg("%i : %s\n", l++, row[0]);
		}
	}
*/

//	while(1);
/*
#define		DEFALT_SQL_STMT	"SELECT username FROM phpbb_users"
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


	/********************************************************
	**
	**		main  :-
	**
	********************************************************/


/*	char		szSQL[ 200 ], aszFlds[ 25 ][ 25 ], szDB[ 50 ] ;
	const  char   *pszT;
	int			j, k, l, x ;
	MYSQL		* myData ;
	MYSQL_RES	* res ;
	MYSQL_FIELD	* fd ;
	MYSQL_ROW	row ;

	//....just curious....
	printf( "sizeof( MYSQL ) == %d\n", (int) sizeof( MYSQL ) ) ;
	strcpy( szDB, "suck" ) ;
	strcpy( szSQL, "SELECT username FROM phpbb_users" ) ;

	if ( (myData = mysql_init((MYSQL*) 0)) && 
		mysql_real_connect( myData, "www.cs-suck.com", "Mani", "suckmapass", "suck", MYSQL_PORT,	NULL, 0 ) )
	{
		if ( mysql_select_db( myData, szDB ) < 0 ) {
			printf( "Can't select the %s database !\n", szDB ) ;
			mysql_close( myData ) ;
		}
	}
	else {
		printf( "Can't connect to the mysql server on port %d !\n",
			MYSQL_PORT ) ;
		mysql_close( myData ) ;
	}
	//....
	if ( ! mysql_query( myData, szSQL ) ) {
		res = mysql_store_result( myData ) ;
		i = (int) mysql_num_rows( res ) ; l = 1 ;
		printf( "Query:  %s\nNumber of records found:  %ld\n", szSQL, i ) ;
		//....we can get the field-specific characteristics here....
		for ( x = 0 ; fd = mysql_fetch_field( res ) ; x++ )
			strcpy( aszFlds[ x ], fd->name ) ;
		//....
		while ( row = mysql_fetch_row( res ) ) {
			j = mysql_num_fields( res ) ;
			printf( "Record #%ld:-\n", l++ ) ;
			for ( k = 0 ; k < j ; k++ )
				printf( "  Fld #%d (%s): %s\n", k + 1, aszFlds[ k ],
				(((row[k]==NULL)||(!strlen(row[k])))?"NULL":row[k])) ;
			puts( "==============================\n" ) ;
		}
		mysql_free_result( res ) ;
	}
	else printf( "Couldn't execute %s on the server !\n", szSQL ) ;
	//....
/*	puts( "====  Diagnostic info  ====" ) ;
	pszT = mysql_get_client_info() ;
	printf( "Client info: %s\n", pszT ) ;
	//....
	pszT = mysql_get_host_info( myData ) ;
	printf( "Host info: %s\n", pszT ) ;
	//....
	pszT = mysql_get_server_info( myData ) ;
	printf( "Server info: %s\n", pszT ) ;
	//....
	res = mysql_list_processes( myData ) ; l = 1 ;
	if (res)
	{
		for ( x = 0 ; fd = mysql_fetch_field( res ) ; x++ )
			strcpy( aszFlds[ x ], fd->name ) ;
		while ( row = mysql_fetch_row( res ) ) {
			j = mysql_num_fields( res ) ;
			printf( "Process #%ld:-\n", l++ ) ;
			for ( k = 0 ; k < j ; k++ )
				printf( "  Fld #%d (%s): %s\n", k + 1, aszFlds[ k ],
				(((row[k]==NULL)||(!strlen(row[k])))?"NULL":row[k])) ;
			puts( "==============================\n" ) ;
		}
	}
	else
	{
		printf("Got error %s when retreiving processlist\n",mysql_error(myData));
	}
	//....
	res = mysql_list_tables( myData, "%" ) ; l = 1 ;
	for ( x = 0 ; fd = mysql_fetch_field( res ) ; x++ )
		strcpy( aszFlds[ x ], fd->name ) ;
	while ( row = mysql_fetch_row( res ) ) {
		j = mysql_num_fields( res ) ;
		printf( "Table #%ld:-\n", l++ ) ;
		for ( k = 0 ; k < j ; k++ )
			printf( "  Fld #%d (%s): %s\n", k + 1, aszFlds[ k ],
			(((row[k]==NULL)||(!strlen(row[k])))?"NULL":row[k])) ;
		puts( "==============================\n" ) ;
	}
	//....
	pszT = mysql_stat( myData ) ;
	puts( pszT ) ;
	//....*/
//	mysql_close( myData ) ;
//	while(1);

}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CAdminPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	max_players = clientMax; 

	// Get team manager pointers
	SetupTeamList(edictCount);
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CAdminPlugin::GameFrame( bool simulating )
{
/*static float start_time = 0.0;

	if (gpGlobals->curtime >= start_time)
	{
		MMsg("curtime = [%f]\n", gpGlobals->curtime);
		start_time += 1.0;
	}
*/
	if (ProcessPluginPaused())
	{
		return;
	}

	if (event_duplicate)
	{
		// Event hash table is broken !!!
		MMsg("MANI ADMIN PLUGIN - Event Hash table has duplicates!\n");
	}

	LanguageGameFrame();

	// Simulate NetworkIDValidate
	gpManiNetIDValid->GameFrame();

	if (mysql_thread)
	{
		mysql_thread->GameFrame();
	}

	gpManiSprayRemove->GameFrame();
	gpManiWarmupTimer->GameFrame();
	gpManiAFK->GameFrame();
	gpManiPing->GameFrame();

	if (mani_protect_against_cheat_cvars.GetInt() == 1)
	{
		ProcessCheatCVars();
	}

	if (war_mode && mani_war_mode_force_overview_zero.GetInt() == 1)
	{
		TurnOffOverviewMap();
	}

	gpManiStats->GameFrame();

	if (war_mode) return;

	if (change_team && change_team_time < gpGlobals->curtime && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		change_team = false;
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_balance");
		gpCmd->AddParam("m"); // mute the function
		this->ProcessMaBalance(NULL, "ma_balance", 0, M_SCONSOLE);
	}

	ProcessAdverts();
	ProcessInGamePunishments();

	if (trigger_changemap && gpGlobals->curtime >= trigger_changemap_time)
	{
		char	server_cmd[512];
		
		trigger_changemap = false;
//		engine->ChangeLevel(next_map, NULL);
		Q_snprintf(server_cmd, sizeof(server_cmd), "changelevel %s\n", next_map);
		engine->ServerCommand(server_cmd);
	}

	gpManiVote->GameFrame();
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CAdminPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gpManiStats->LevelShutdown();
	gpManiSpawnPoints->LevelShutdown();
	gpManiAFK->LevelShutdown();
	gameeventmanager->RemoveListener(gpManiIGELCallback);
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientActive( edict_t *pEntity )
{
//  MMsg("Mani -> Client Active\n");

	gpManiTrackUser->ClientActive(pEntity);

	if (ProcessPluginPaused())
	{
		return;
	}

	gpManiNetIDValid->ClientActive(pEntity);

	player_t player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return;
	if (player.player_info->IsHLTV()) return;

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->ClientActive(&player);
	}

	if (FStrEq(player.player_info->GetNetworkIDString(),"BOT")) return;

	gpManiGhost->ClientActive(&player);
	gpManiVictimStats->ClientActive(&player);
	gpManiMapAdverts->ClientActive(&player);
	gpManiCSSBounty->ClientActive(&player);

	if (!player.is_bot)
	{
		user_name[player.index - 1].in_use = true;
		Q_strcpy(user_name[player.index - 1].name, engine->GetClientConVarValue(player.index, "name"));

		// Player settings
		PlayerJoinedInitSettings(&player);
		cheat_ping_list[player.index - 1].connected = true;
		cheat_ping_list[player.index - 1].waiting_for_control = false;
		cheat_ping_list[player.index - 1].waiting_for_cvar = false;
		cheat_ping_list[player.index - 1].waiting_to_send = true;
		cheat_ping_list[player.index - 1].count = true;
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

	gpManiNetIDValid->ClientDisconnect(&player);
	gpManiReservedSlot->ClientDisconnect(&player);
	gpManiSprayRemove->ClientDisconnect(&player);
	gpManiSaveScores->ClientDisconnect(&player);
	gpManiAFK->ClientDisconnect(&player);
	gpManiPing->ClientDisconnect(&player);
	gpManiCSSBounty->ClientDisconnect(&player);
	gpManiCSSBetting->ClientDisconnect(&player);

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
	ProcessTKDisconnected (&player);
	gpManiGhost->ClientDisconnect(&player);
	gpManiVictimStats->ClientDisconnect(&player);
	gpManiClient->ClientDisconnect(&player);
	gpManiStats->ClientDisconnect(&player);

	user_name[player.index - 1].in_use = false;
	Q_strcpy(user_name[player.index - 1].name,"");
	name_changes[player.index - 1] = 0;

	cheat_ping_list[player.index - 1].connected = false;

	if (player.is_bot)
	{
		gpManiTrackUser->ClientDisconnect(&player);
		return;
	}

	EffectsClientDisconnect(player.index - 1, false);

	if (mani_tk_protection.GetInt() == 1)
	{
		if (sv_lan && sv_lan->GetInt() == 1)
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
	menu_confirm[player.index - 1].in_use = false;
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
	}	
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	if ( playerinfomanager )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEdict );
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
PLUGIN_RESULT CAdminPlugin::ClientCommand( edict_t *pEntity )
{
	if (ProcessPluginPaused())
	{
		return PLUGIN_CONTINUE;
	}

	player_t	player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
	gpCmd->ExtractClientAndServerCommand();

	int	pargc = gpCmd->Cmd_Argc();
	const char *pcmd = gpCmd->Cmd_Argv(0);
	const char *pcmd2 = gpCmd->Cmd_Argv(1);

	if (FStrEq(pcmd, "menuselect"))
	{
		if (pargc != 2)	return PLUGIN_STOP;

		if (menu_confirm[player.index - 1].in_use)
		{
			int command_index = Q_atoi(pcmd2);
			menu_confirm[player.index - 1].in_use = false;
			char client_cmd[256];

			// Exit pressed
			if (command_index == 10)
			{
				ProcessPlayMenuSound(&player, menu_select_exit_sound);
				return PLUGIN_STOP;
			}

			if (command_index == 0)	return PLUGIN_STOP;
			command_index --;
			// Found menu command we want to use, cheat by running the command from the client console 
			if (!menu_confirm[player.index - 1].menu_select[command_index].in_use)	return PLUGIN_STOP;

			Q_snprintf(client_cmd, sizeof(client_cmd), "%s\n", menu_confirm[player.index - 1].menu_select[command_index].command);
//			helpers->ClientCommand(pEntity, client_cmd);
			ProcessPlayMenuSound(&player, menu_select_sound);

			gpCmd->ParseEventSayCommand(menu_confirm[player.index - 1].menu_select[command_index].command);
//			engine->ClientCommand(pEntity, "cmd %s\n", menu_confirm[player.index - 1].menu_select[command_index].command);
//			return PLUGIN_STOP;
		}
		else
		{
			return PLUGIN_CONTINUE;
		}
	}

	if (gpCmd->HandleCommand(&player, M_CCONSOLE) == PLUGIN_STOP) return PLUGIN_STOP;
	else if ( FStrEq( pcmd, "jointeam")) return (gpManiTeamJoin->PlayerJoin(pEntity, (char *) gpCmd->Cmd_Argv(1)));
	else if ( FStrEq( pcmd, "joinclass")) return (gpManiWarmupTimer->JoinClass(pEntity));
	else if ( FStrEq( pcmd, "admin" )) return (ProcessAdminMenu(pEntity));
	else if ( FStrEq( pcmd, "manisettings")) return (ProcessSettingsMenu(pEntity));
	else if ( FStrEq( pcmd, "settings")) return (ProcessSettingsMenu(pEntity));
	else if ( FStrEq( pcmd, "votemap" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_map.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaUserVoteMap(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}	
	else if ( FStrEq( pcmd, "votekick" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_kick.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_kick_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaUserVoteKick(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "voteban" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_ban.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_ban_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaUserVoteBan(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMaRockTheVoteNominateMap(&player, pargc, gpCmd->Cmd_Args(1));
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_rtvnominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		gpManiVote->ProcessMenuRockTheVoteNominateMap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotemapmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_map.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMenuUserVoteMap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotekickmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_kick.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_kick_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMenuUserVoteKick (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotebanmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_ban.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_ban_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		gpManiVote->ProcessMenuUserVoteBan(&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "favourites" ) && !war_mode)
	{
		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		ProcessMenuMaFavourites (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_weaponme" ) && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return PLUGIN_CONTINUE;

		int	page = 1;

		if (gpCmd->Cmd_Argc() == 2)
		{
			page = Q_atoi(gpCmd->Cmd_Argv(1));
		}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
		gpManiStats->ShowWeaponMe(&player, page);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_showtop" ) && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return PLUGIN_CONTINUE;

		int	rank_start = 1;

		if (gpCmd->Cmd_Argc() == 2)
		{
			rank_start = Q_atoi(gpCmd->Cmd_Argv(1));
		}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
		gpManiStats->ShowTop(&player, rank_start);
		return PLUGIN_STOP;
	}
	else if ( FStrEq(pcmd, "buyammo1") || FStrEq(pcmd, "buyammo2") || FStrEq(pcmd, "buy"))
	{
		return (ProcessClientBuy(pcmd, pcmd2, pargc, &player));
	}
	else if ( FStrEq( pcmd, "nextmap" ))
	{
		return (ProcessMaNextMap(&player, "nextmap", 0, M_CCONSOLE));
	}
	else if ( FStrEq( pcmd, "timeleft" )) return (ProcessMaTimeLeft(&player, "timeleft", 0, M_CCONSOLE));
	else if ( FStrEq( pcmd, "listmaps" )) return (ProcessMaListMaps(&player, "timeleft", 0, M_CCONSOLE));
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
		int	next_index = 0;
		int argv_offset = 0;

		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;

		if (FStrEq (gpCmd->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
			}

		gpManiVote->ProcessMenuSystemVotemap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_vote_accept" ))
	{
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		gpManiVote->ProcessVoteConfirmation (&player, true);
		return PLUGIN_STOP;
	}
	else if (FStrEq( pcmd, "ma_explode" ))
	{
		int admin_index;
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, "ma_explode", ALLOW_EXPLODE, war_mode, &admin_index)) return PLUGIN_STOP;

		ProcessExplodeAtCurrentPosition (&player);
		return PLUGIN_STOP;
	} 
	else if ( FStrEq( pcmd, "mani_vote_refuse" ))
	{
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		gpManiVote->ProcessVoteConfirmation (&player, false);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "tkbot" ) || FStrEq( pcmd, "tkhuman"))
	{
		if (ProcessTKCmd(&player)) return PLUGIN_STOP;
		return PLUGIN_STOP;
	}
	else
	{
		if (ProcessCheatCVarPing(&player, pcmd))
		{
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Init cheat ping list
//---------------------------------------------------------------------------------
bool	CAdminPlugin::ProcessCheatCVarPing(player_t *player, const char *pcmd)
{
	if (!FindPlayerByEntity(player)) return false;
	if (player->is_bot) return false;
	if (player->player_info->IsHLTV()) return false;
	if (FStrEq(player->player_info->GetNetworkIDString(),"BOT")) return false;

	if (cheat_ping_list[player->index - 1].waiting_for_control)
	{
		if (FStrEq(cheat_ping_list[player->index - 1].control_string, pcmd))
		{
			cheat_ping_list[player->index - 1].waiting_for_control = false;
			if (!cheat_ping_list[player->index - 1].waiting_for_cvar)
			{
				cheat_ping_list[player->index - 1].waiting_to_send = true;
			}

			return true;
		}
	}

	if (cheat_ping_list[player->index - 1].waiting_for_cvar)
	{
		if (FStrEq(cheat_ping_list[player->index - 1].cvar_string, pcmd))
		{
			cheat_ping_list[player->index - 1].waiting_for_cvar = false;
			if (!cheat_ping_list[player->index - 1].waiting_for_control)
			{
				cheat_ping_list[player->index - 1].waiting_to_send = true;
			}

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Init cheat ping list
//---------------------------------------------------------------------------------
void	CAdminPlugin::InitCheatPingList(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		cheat_ping_list[i].connected = false;
		cheat_ping_list[i].count = 0;
		Q_strcpy(cheat_ping_list[i].cvar_string,"");
		Q_strcpy(cheat_ping_list[i].control_string, "");
		cheat_ping_list[i].waiting_for_control = false;
		cheat_ping_list[i].waiting_for_cvar = false;
		cheat_ping_list[i].waiting_to_send = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update with player information
//---------------------------------------------------------------------------------
void	CAdminPlugin::UpdateCurrentPlayerList(void)
{
	for (int i = 1; i <= max_players; i++)
	{
		player_t	player;

		cheat_ping_list[i - 1].connected = false;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (player.player_info->IsHLTV()) continue;
		if (FStrEq(player.player_info->GetNetworkIDString(),"BOT")) continue;

		// Must be a player connected
		cheat_ping_list[i - 1].connected = true;
	}
}
//---------------------------------------------------------------------------------
// Purpose: Load cheat list
//---------------------------------------------------------------------------------
void	CAdminPlugin::LoadCheatList(void)
{
	// Free list first
	FreeList((void **) &cheat_cvar_list, &cheat_cvar_list_size);
	FreeList((void **) &cheat_cvar_list2, &cheat_cvar_list_size2);

	// hlh_norecoil 2
	AddToList((void **) &cheat_cvar_list, sizeof(cheat_cvar_t), &cheat_cvar_list_size);
	Q_strcpy(cheat_cvar_list[cheat_cvar_list_size - 1].cheat_cvar, "rvrixy|omysv*<");

	AddToList((void **) &cheat_cvar_list2, sizeof(cheat_cvar_t), &cheat_cvar_list_size2);
	Q_strcpy(cheat_cvar_list2[cheat_cvar_list_size2 - 1].cheat_cvar, "rvrixy|omysv");

	// omfg_norecoil 2
	AddToList((void **) &cheat_cvar_list, sizeof(cheat_cvar_t), &cheat_cvar_list_size);
	Q_strcpy(cheat_cvar_list[cheat_cvar_list_size - 1].cheat_cvar, "ywpqixy|omysv*<");
	
	AddToList((void **) &cheat_cvar_list2, sizeof(cheat_cvar_t), &cheat_cvar_list_size2);
	Q_strcpy(cheat_cvar_list2[cheat_cvar_list_size2 - 1].cheat_cvar, "ywpqixy|omysv");


	for (int i = 0; i < cheat_cvar_list_size; i++)
	{
		for (int j = 0; j < Q_strlen(cheat_cvar_list[i].cheat_cvar); j ++)
		{
			cheat_cvar_list[i].cheat_cvar[j] -= 10;
		}
	}

	for (int i = 0; i < cheat_cvar_list_size2; i++)
	{
		for (int j = 0; j < Q_strlen(cheat_cvar_list2[i].cheat_cvar); j ++)
		{
			cheat_cvar_list2[i].cheat_cvar[j] -= 10;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check for cheat cvar list
//---------------------------------------------------------------------------------
char	*CAdminPlugin::GenerateControlString(void)
{
	static	char	control_string[10];

	for (int i = 0; i < 9; i ++)
	{
		control_string[i] = (char) ((rand() % 26) + 97);
	}

	control_string[9] = '\0';

	return (char *) control_string;
}

//---------------------------------------------------------------------------------
// Purpose: Check for cheat cvar list
//---------------------------------------------------------------------------------
void	CAdminPlugin::ProcessCheatCVarCommands(void)
{
	// Game frame has called this
	player_t	player;
	char	ban_cmd[128];

	for (int i = 0; i < max_players; i++)
	{
		// Player not connected
		if (!cheat_ping_list[i].connected) continue;

		player.index = i + 1;

		if (cheat_ping_list[i].waiting_for_control &&
			cheat_ping_list[i].waiting_for_cvar)
		{
			cheat_ping_list[i].waiting_to_send = true;
			cheat_ping_list[i].waiting_for_cvar = false;
			cheat_ping_list[i].waiting_for_control = false;
		}

		if (!cheat_ping_list[i].waiting_for_control &&
			cheat_ping_list[i].waiting_for_cvar &&
			!cheat_ping_list[i].waiting_to_send)
		{
			// Hacker alert, we got the control cvar back
			// but not the cheat cvar

			if (!FindPlayerByIndex(&player))
			{
				cheat_ping_list[i].connected = false;
				continue;
			}

			// Process how we handle cheats

			cheat_ping_list[i].count ++;
			if (cheat_ping_list[i].count < mani_protect_against_cheat_cvars_threshold.GetInt())
			{
				int tries_left = mani_protect_against_cheat_cvars_threshold.GetInt() - cheat_ping_list[i].count;

				for (int j = 1; j <= max_players; j++)
				{
					player_t	admin;
					int			admin_index;

					admin.index = j;
					if (!FindPlayerByIndex(&admin)) continue;
					if (admin.is_bot) continue;
					if (!gpManiClient->IsAdmin(&admin, &admin_index)) continue;

					SayToPlayer(LIGHT_GREEN_CHAT, &admin, "[MANI_ADMIN_PLUGIN] Warning !! Player %s may have a cheat installed", player.name);
					SayToPlayer(LIGHT_GREEN_CHAT, &admin, "%i more detection attempt%s until automatic action is taken", tries_left, (tries_left == 1) ? "":"s");
				}

				continue;
			}

			if (mani_protect_against_cheat_cvars_mode.GetInt() == 0 && sv_lan->GetInt() != 1)
			{
				// Ban by user id
				SayToAll(ORANGE_CHAT, false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_protect_against_cheat_cvars_ban_time.GetInt(), 
											player.user_id);
				LogCommand(NULL, "Banned (Hack detected) [%s] [%s] %s", player.name, player.steam_id, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
				cheat_ping_list[i].connected = false;
				continue;
			}
			else if (mani_protect_against_cheat_cvars_mode.GetInt() == 1)
			{
				// Ban by user ip address
				SayToAll(ORANGE_CHAT, false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
						mani_protect_against_cheat_cvars_ban_time.GetInt(), 
						player.ip_address);
				LogCommand(NULL, "Banned IP (Hack detected) [%s] [%s] %s", player.name, player.ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				cheat_ping_list[i].connected = false;
				continue;
			}
			else if (mani_protect_against_cheat_cvars_mode.GetInt() == 2)
			{
				// Ban by user id and ip address
				SayToAll(ORANGE_CHAT, false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				if (sv_lan->GetInt() != 1)
				{
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_protect_against_cheat_cvars_ban_time.GetInt(), 
											player.user_id);
					LogCommand(NULL, "Banned (Hack detected) [%s] [%s] %s", player.name, player.steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);	
					engine->ServerCommand("writeid\n");
				}

				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_protect_against_cheat_cvars_ban_time.GetInt(), 
										player.ip_address);
				LogCommand(NULL, "Banned IP (Hack detected) [%s] [%s] %s", player.name, player.ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");

				cheat_ping_list[i].connected = false;
				continue;
			}
		}

		if (cheat_ping_list[i].waiting_to_send ||
			(!cheat_ping_list[i].waiting_for_cvar &&
			  cheat_ping_list[i].waiting_for_control))
		{
			int cvar_to_use = rand () % cheat_cvar_list_size;

			if (!FindPlayerByIndex(&player))
			{
				cheat_ping_list[i].connected = false;
				continue;
			}

			// Ready to send a control string and cheat cvar
			Q_strcpy(cheat_ping_list[i].control_string, GenerateControlString());
			Q_strcpy(cheat_ping_list[i].cvar_string, cheat_cvar_list2[cvar_to_use].cheat_cvar);

			// throw it out to client
			engine->ClientCommand(player.entity, "%s\n", cheat_ping_list[i].control_string);
			engine->ClientCommand(player.entity, "%s\n", cheat_cvar_list[cvar_to_use].cheat_cvar);

			cheat_ping_list[i].waiting_to_send = false;
			cheat_ping_list[i].waiting_for_cvar = true;
			cheat_ping_list[i].waiting_for_control = true;
			continue;
		}
	}
}
//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ProcessAdminMenu( edict_t *pEntity)
{
	int	admin_index;
	player_t player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return PLUGIN_STOP;
	}

	if (!gpManiClient->IsAdminAllowed(&player, "admin", ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;

	if (1 == gpCmd->Cmd_Argc())
	{
		// User typed admin at console
		ShowPrimaryMenu(pEntity, admin_index);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() > 1) 
	{
		const char *temp_command = gpCmd->Cmd_Argv(1);
		int next_index = 0;
		int argv_offset = 0;

		if (FStrEq (temp_command, "more"))
		{
			// Get next index for menu
			next_index = Q_atoi(gpCmd->Cmd_Argv(2));
			argv_offset = 2;
		}

		const char *menu_command = gpCmd->Cmd_Argv(1 + argv_offset);

		if (FStrEq (menu_command, "changemap"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change map\n");
				return PLUGIN_STOP;
			}

			// Change map command selected via menu or console
			ProcessChangeMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "setnextmap"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to set the next map\n");
				return PLUGIN_STOP;
			}

			// Set Next Map command selected via menu or console
			ProcessSetNextMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "kicktype"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to kick players\n");
				return PLUGIN_STOP;
			}

			// kick command selected via menu or console
			ProcessKickType (&player);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "kick"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to kick players\n");
				return PLUGIN_STOP;
			}

			// kick command selected via menu or console
			ProcessKickPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "punish"))
		{
			if (( !gpManiClient->IsAdminAllowed(admin_index, ALLOW_BLIND) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAP) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZE) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_GIMP) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_DRUG) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BURN) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_NO_CLIP) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SETSKINS) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_TIMEBOMB) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FIREBOMB) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZEBOMB) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BEACON))
				|| war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to punish players\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessPunishType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "playeroptions"))
		{
			if ((!gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAY) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MUTE) &&
				!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SPRAY_TAG))
				|| war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to manage players\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessPlayerManagementType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "mapoptions"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change maps\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessMapManagementType (&player, admin_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "voteoptions"))
		{
			if ((!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RANDOM_MAP_VOTE)) &&
				(!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE)) &&
				(!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_RCON_VOTE)) &&
				(!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_QUESTION_VOTE)) &&
				(!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CANCEL_VOTE)) ||
				 war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to manage votes\n");
				return PLUGIN_STOP;
			}

			// Vote management options
			ProcessVoteType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slay"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAY) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slay players\n");
				return PLUGIN_STOP;
			}

			// Slay command selected via menu or console
			ProcessSlayPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "spray") ||
			FStrEq (menu_command, "spraywarn") ||
			FStrEq (menu_command, "sprayslap") ||
			FStrEq (menu_command, "spraykick") ||
			FStrEq (menu_command, "sprayban") ||
			FStrEq (menu_command, "spraypermban"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SPRAY_TAG) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to check spray tags\n");
				return PLUGIN_STOP;
			}

			// Slay command selected via menu or console
			gpManiSprayRemove->ProcessMaSprayMenu (&player, admin_index, next_index, argv_offset, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "votercon"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_RCON_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run a rcon vote\n");
				return PLUGIN_STOP;
			}

			// RCon vote command selected via menu or console
			gpManiVote->ProcessRConVote (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "votequestion"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_QUESTION_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run a question vote\n");
				return PLUGIN_STOP;
			}

			// Question vote command selected via menu or console
			gpManiVote->ProcessQuestionVote (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "voteextend"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run an extension vote\n");
				return PLUGIN_STOP;
			}

			// Extend vote command selected via menu or console
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_voteextend");
			gpManiVote->ProcessMaVoteExtend(&player, "ma_voteextend", 0, M_MENU);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "ban") || FStrEq (menu_command, "banip"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessBanPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "randommapvote"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RANDOM_MAP_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run random map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			gpManiVote->ProcessMenuSystemVoteRandomMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "buildmapvote"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run random map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			gpManiVote->ProcessMenuSystemVoteBuildMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "singlemapvote"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			gpManiVote->ProcessMenuSystemVoteSingleMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "multimapvote"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			gpManiVote->ProcessMenuSystemVoteMultiMap (&player, admin_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "autobanname"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// AutoBan command selected via menu or console
			gpManiAutoKickBan->ProcessAutoBanPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "autokicksteam") || FStrEq (menu_command, "autokickip") || FStrEq (menu_command, "autokickname"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to autokick players\n");
				return PLUGIN_STOP;
			}

			// Autokick command selected via menu or console
			gpManiAutoKickBan->ProcessAutoKickPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slap"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slap players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSlapPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "blind"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BLIND) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to blind players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuBlindPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "freeze"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to freeze players\n");
				return PLUGIN_STOP;
			}

			// Freeze command selected via menu or console
			ProcessMenuFreezePlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "burn"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BURN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to burn players\n");
				return PLUGIN_STOP;
			}

			// Burn command selected via menu or console
			ProcessMenuBurnPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "swapteam"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to swap players to different teams\n");
				return PLUGIN_STOP;
			}

			// SwapTeam command selected via menu or console
			ProcessMenuSwapPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "specplayer"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to move players to be spectator\n");
				return PLUGIN_STOP;
			}

			// Spec command selected via menu or console
			ProcessMenuSpecPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "balanceteam"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) || war_mode || !gpManiGameType->IsTeamPlayAllowed())
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to balance teams\n");
				return PLUGIN_STOP;
			}

			// Balance players on the server
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_balance");
			this->ProcessMaBalance (&player, "ma_balance", 0, M_MENU);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cancelvote"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CANCEL_VOTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to cancel votes\n");
				return PLUGIN_STOP;
			}

			// Cancel vote on server
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_votecancel");
			gpManiVote->ProcessMaVoteCancel (&player, "ma_votecancel", 0, M_MENU);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "drug"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_DRUG) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to drug players\n");
				return PLUGIN_STOP;
			}

			// Drug command selected via menu or console
			ProcessMenuDrugPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "noclip"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_NO_CLIP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to no clip players\n");
				return PLUGIN_STOP;
			}

			// No Clip command selected via menu or console
			ProcessMenuNoClipPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "skinoptions"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SETSKINS) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change skins of players\n");
				return PLUGIN_STOP;
			}

			// Skin Options command selected via menu or console
			ProcessMenuSkinOptions (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "setskin"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SETSKINS) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change skins of players\n");
				return PLUGIN_STOP;
			}

			// Set Skin command selected via menu or console
			ProcessMenuSetSkin (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "timebomb"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_TIMEBOMB) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into time bombs\n");
				return PLUGIN_STOP;
			}

			// Set Time Bomb command selected via menu or console
			ProcessMenuTimeBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "firebomb"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FIREBOMB) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into fire bombs\n");
				return PLUGIN_STOP;
			}

			// Set Fire Bomb command selected via menu or console
			ProcessMenuFireBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "freezebomb"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZEBOMB) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into freeze bombs\n");
				return PLUGIN_STOP;
			}

			// Set Freeze Bomb command selected via menu or console
			ProcessMenuFreezeBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "beacon"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BEACON) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into beacons\n");
				return PLUGIN_STOP;
			}

			// Set Freeze Bomb command selected via menu or console
			ProcessMenuBeaconPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "gimp"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_GIMP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to gimp players\n");
				return PLUGIN_STOP;
			}

			// Gimp command selected via menu or console
			ProcessMenuGimpPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "mute"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_MUTE) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to mute players\n");
				return PLUGIN_STOP;
			}

			// Gimp command selected via menu or console
			ProcessMenuMutePlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "teleport"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to teleport players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuTeleportPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "savelocation"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to teleport players\n");
				return PLUGIN_STOP;
			}

			// Save loc command selected via menu or console
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_saveloc");
			ProcessMaSaveLoc (&player, "ma_saveloc", 0, M_MENU);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "banoptions") || FStrEq (menu_command, "banoptionsip") ||
			FStrEq (menu_command, "autobanoptionsname"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessBanOptions (pEntity, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "randomvoteoptions") || FStrEq (menu_command, "mapvoteoptions") ||
			FStrEq (menu_command, "multimapvoteoptions"))
		{
			if ((!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RANDOM_MAP_VOTE) &&
				 !gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE)) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run votes\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessDelayTypeOptions (&player, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slapoptions"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAP) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slap players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (amount of damage)
			ProcessSlapOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "blindoptions"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BLIND) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to blind players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (amount of blindness)
			ProcessBlindOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "bantype"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessBanType (&player);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "config"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CONFIG))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change the plugin config\n");
				return PLUGIN_STOP;
			}

			// Config options 
			ProcessConfigOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "toggle"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CONFIG))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change the plugin config\n");
				return PLUGIN_STOP;
			}

			// Process toggle of options
			ProcessConfigToggle (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "restrict_weapon"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RESTRICTWEAPON) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to restrict weapons\n");
				return PLUGIN_STOP;
			}

			// restrict weapon command selected via menu or console
			ProcessRestrictWeapon (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "play_sound"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_PLAYSOUND) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to play sounds\n");
				return PLUGIN_STOP;
			}

			// play sound command selected via menu or console
			ProcessPlaySound (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
			
		if (FStrEq (menu_command, "rcon"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RCON_MENU))
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run rcon command
			ProcessRconCommand (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cexecoptions"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run rcon command
			ProcessCExecOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cexec") ||
			FStrEq (menu_command, "cexec_t") ||
			FStrEq (menu_command, "cexec_ct") ||
			FStrEq (menu_command, "cexec_spec") ||
			FStrEq (menu_command, "cexec_all"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run exec command
			ProcessCExecCommand (&player, (char *) menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}

		if (FStrEq (menu_command, "cexec_player"))
		{
			if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			ProcessCExecPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowPrimaryMenu( edict_t *pEntity, int admin_index )
{
	int	 range = 0;

	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	if (!war_mode)
	{
		if ((gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK) || 
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) || 
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) ||
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_MUTE) ||
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) || 
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_SPRAY_TAG)) 
			 && !war_mode)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(102));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAY) || 
			 (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAP) && gpManiGameType->IsSlapAllowed()) || 
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_BLIND) ||
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZE) ||
			 (gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) && gpManiGameType->IsTeleportAllowed()) ||
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_GIMP) ||
			 (gpManiClient->IsAdminAllowed(admin_index, ALLOW_DRUG) && gpManiGameType->IsDrugAllowed()) ||
			 (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BURN) && gpManiGameType->IsFireAllowed()) ||
			 gpManiClient->IsAdminAllowed(admin_index, ALLOW_NO_CLIP))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(103));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(104));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mapoptions");
		}

		if ((gpManiClient->IsAdminAllowed(admin_index, ALLOW_RANDOM_MAP_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_RCON_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_QUESTION_VOTE) && !gpManiVote->SysVoteInProgress()) ||
			(gpManiClient->IsAdminAllowed(admin_index, ALLOW_CANCEL_VOTE) && gpManiVote->SysVoteInProgress()))
			
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(105));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteoptions");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_RESTRICTWEAPON) && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(106));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin restrict_weapon");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_PLAYSOUND))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(107));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin play_sound");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_RCON_MENU))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(108));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin rcon");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CONFIG))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(109));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin config");
		}
	}
	else
	{
		// Draw war options
		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(110));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin changemap");
		}

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_RCON_MENU))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(108));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin rcon");
		}

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(111));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle warmode");

	}

	if (menu_list_size == 0) return;
	DrawStandardMenu(&player, Translate(100), Translate(101), true);
	
	return;
}


//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessChangeMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char new_map[128];

		Q_strcpy(new_map, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_map");
		gpCmd->AddParam("%s", new_map);
		ProcessMaMap(player, "ma_map", 0, M_MENU);
		return;
	}
	else
	{
		int	i;

		// Setup menu list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), map_list_size, &menu_list_size);

		for (i = 0; i < map_list_size; i++)
		{
			Q_strcpy(menu_list[i].menu_text, map_list[i].map_name);
			Q_snprintf(menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin changemap %s", map_list[i].map_name);
			Q_strcpy(menu_list[i].sort_name, map_list[i].map_name);
		}

		SortMenu();

		// List size may have changed
		if (next_index > map_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player, Translate(130), Translate(131), next_index, "admin", "changemap", true, -1);
	}
		
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSetNextMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char new_map[128];

		Q_strcpy(new_map, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_setnextmap");
		gpCmd->AddParam("%s", new_map);
		ProcessMaSetNextMap(player, "ma_setnextmap", 0, M_MENU);
		return;
	}
	else
	{
		int	i;

		// Setup menu list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), map_list_size, &menu_list_size);

		for (i = 0; i < map_list_size; i++)
		{
			Q_strcpy(menu_list[i].menu_text, map_list[i].map_name);
			Q_snprintf(menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin setnextmap %s", map_list[i].map_name);
		}

		// List size may have changed
		if (next_index > map_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player, Translate(140), Translate(141), next_index, "admin", "setnextmap", true, -1);
	}
		
	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessKickPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		char string[128];

		Q_strcpy(string, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_kick");
		gpCmd->AddParam("%s", string);
		ProcessMaKick(admin, "ma_kick", 0, M_MENU);

		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index,IMMUNITY_ALLOW_KICK))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]", player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kick %i", player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();
		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, Translate(150), Translate(151), next_index,"admin","kick",	true, -1);
		// Draw menu list
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSlapPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsSlapAllowed()) return;

	if (argc - argv_offset == 4)
	{
		// Slap the player
		char string1[128];
		char string2[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(3 + argv_offset));
		Q_strcpy(string2, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_slap");
		gpCmd->AddParam("%s", string1);
		gpCmd->AddParam("%s", string2);
		ProcessMaSlap(admin, "ma_slap", 0, M_MENU);

		FreeMenu();

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(162));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap %s %s", string2, string1);
		if (menu_list_size == 0) return;
		DrawStandardMenu(admin, Translate(163), Translate(164), true);

		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_SLAP))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]", player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap %s %i", gpCmd->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char more_cmd[128];
		Q_snprintf( more_cmd, sizeof(more_cmd), "slap %s", gpCmd->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(160), Translate(161), next_index, "admin", more_cmd, true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Blind Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBlindPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		int blind_amount = atoi(gpCmd->Cmd_Argv(2 + argv_offset));
		if (blind_amount < 0)
		{
			blind_amount = 0;
		}

		if (blind_amount > 255)
		{
			blind_amount = 255;
		}

		char blind_amount_str[128];
		Q_snprintf(blind_amount_str, sizeof(blind_amount_str), "%i", blind_amount);

		// Blind the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(3 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_blind");
		gpCmd->AddParam("%s", string1);
		gpCmd->AddParam("%s", blind_amount_str);
		ProcessMaBlind(admin, "ma_blind", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_BLIND))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind %s %i", gpCmd->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char more_cmd[128];
		Q_snprintf( more_cmd, sizeof(more_cmd), "blind %s", gpCmd->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(170), Translate(171), next_index, "admin", more_cmd, true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Swap Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSwapPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Swap the player to opposite team
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_swapteam");
		gpCmd->AddParam("%s", string1);
		ProcessMaSwapTeam(admin, "ma_swapteam", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;


			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_SWAP))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] [%s] %i", 
											Translate(gpManiGameType->GetTeamShortTranslation(player.team)),
											player.name, 
											player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin swapteam %i", player.user_id);
			Q_strcpy( menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();
		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(180), Translate(181), next_index, "admin", "swapteam", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Move of player to spectator side
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSpecPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Swap the player to opposite team
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_spec");
		gpCmd->AddParam("%s", string1);
		ProcessMaSpec(admin, "ma_spec", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;

			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_SWAP))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] [%s] %i", 
											Translate(gpManiGameType->GetTeamShortTranslation(player.team)),
											player.name, 
											player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin specplayer %i", player.user_id);
			Q_strcpy( menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(790), Translate(791), next_index, "admin", "specplayer", true, -1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFreezePlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Freeze the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_freeze");
		gpCmd->AddParam("%s", string1);
		ProcessMaFreeze(admin, "ma_freeze", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_FREEZE))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].frozen) ? Translate(192):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freeze %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(190), Translate(191), next_index, "admin", "freeze", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Burn Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBurnPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsFireAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Burn the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_burn");
		gpCmd->AddParam("%s", string1);
		ProcessMaBurn(admin, "ma_burn", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_BURN))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin burn %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(740), Translate(741), next_index, "admin", "burn", true,	-1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Drug Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuDrugPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsDrugAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Drug the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_drug");
		gpCmd->AddParam("%s", string1);
		ProcessMaDrug(admin, "ma_drug", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_DRUG))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].drugged) ? Translate(202):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin drug %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(200), Translate(201), next_index, "admin", "drug", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle No Clip Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuNoClipPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// No clip the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_noclip");
		gpCmd->AddParam("%s", string1);
		ProcessMaNoClip(admin, "ma_noclip", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)	continue;
			if (player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].no_clip) ? Translate(752):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin noclip %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(750), Translate(751), next_index, "admin", "noclip", true,	-1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Gimp Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuGimpPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Gimp the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_gimp");
		gpCmd->AddParam("%s", string1);
		ProcessMaGimp(admin, "ma_gimp", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_GIMP))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].gimped) ? Translate(212):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin gimp %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(210), Translate(211), next_index, "admin", "gimp", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Gimp Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuTimeBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Time Bomb
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_timebomb");
		gpCmd->AddParam("%s", string1);
		ProcessMaTimeBomb(admin, "ma_timebomb", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_TIMEBOMB))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
								(punish_mode_list[player.index - 1].time_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin timebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(850), Translate(851), next_index, "admin", "timebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Fire Bomb Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFireBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Fire Bomb
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_firebomb");
		gpCmd->AddParam("%s", string1);
		ProcessMaFireBomb(admin, "ma_firebomb", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_FIREBOMB))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
				(punish_mode_list[player.index - 1].fire_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin firebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(852), Translate(853), next_index, "admin", "firebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Bomb Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFreezeBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Freeze Bomb
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_freezebomb");
		gpCmd->AddParam("%s", string1);
		ProcessMaFreezeBomb(admin, "ma_freezebomb", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_FREEZEBOMB))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
				(punish_mode_list[player.index - 1].freeze_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freezebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(854), Translate(855), next_index, "admin", "freezebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Beacon Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBeaconPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Beacon
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_beacon");
		gpCmd->AddParam("%s", string1);
		ProcessMaBeacon(admin, "ma_beacon", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_BEACON))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
								(punish_mode_list[player.index - 1].beacon != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin beacon %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(856), Translate(857), next_index, "admin", "beacon", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Mute Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuMutePlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Mute/Un mute the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_mute");
		gpCmd->AddParam("%s", string1);
		ProcessMaMute(admin, "ma_mute", 0, M_MENU);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_MUTE))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].muted) ? Translate(762):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mute %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(760), Translate(761), next_index, "admin", "mute", true, -1);
	}

	return;
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
void CAdminPlugin::ProcessMenuTeleportPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsTeleportAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Teleport the player
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_teleport");
		gpCmd->AddParam("%s", string1);
		ProcessMaTeleport(admin, "ma_teleport", 0, M_MENU);
		return;
	}
	else
	{
		if (!CanTeleport(admin))
		{
			PrintToClientConsole(admin->entity, Translate(222));
			return;
		}

		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_TELEPORT))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin teleport %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(220), Translate(221), next_index, "admin", "teleport", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSlayPlayer( player_t *admin, int next_index, int argv_offset )
{
	player_t	player;
	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char string1[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_slay");
		gpCmd->AddParam("%s", string1);
		ProcessMaSlay(admin, "ma_slay", 0, M_MENU);
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_SLAY))
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]",  player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slay %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(230), Translate(231), next_index,"admin","slay",true,-1);
	}


	return;
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
void CAdminPlugin::ProcessRconCommand( player_t *admin, int next_index, int argv_offset )
{

	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	rcon_cmd[512];
		int		rcon_index;

		rcon_index = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset));
		if (rcon_index < 0 || rcon_index >= rcon_list_size)
		{
			return;
		}

		Q_snprintf( rcon_cmd, sizeof(rcon_cmd),	"%s\n", rcon_list[rcon_index].rcon_command);

		LogCommand (admin, "rcon command [%s]\n", rcon_list[rcon_index].rcon_command);
		engine->ServerCommand(rcon_cmd);
	}
	else
	{
		// Setup rcon list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), rcon_list_size, &menu_list_size);

		for( int i = 0; i < rcon_list_size; i++ )
		{
			Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", rcon_list[i].alias);							
			Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin rcon %i", i);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(260), Translate(261), next_index, "admin", "rcon", true, -1);
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Allows admin to run client commands from a list
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecCommand( player_t *admin, char *command, int next_index, int argv_offset )
{

	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	cexec_cmd[512];
		char	cexec_log_cmd[512];
		int		cexec_index;
		int		length;
		player_t	player;

		cexec_index = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset));

		if (cexec_index < 0) return;

	    if (FStrEq(command,"cexec_t"))
		{
			// All T's
			if (cexec_index >= cexec_t_list_size) return;

			Q_snprintf( cexec_cmd, sizeof(cexec_cmd), "%s\n", cexec_t_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != TEAM_A)
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin, "[%s] on player [%s]\n", cexec_t_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_ct"))
		{
			// All CT's
			if (cexec_index >= cexec_ct_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_ct_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != TEAM_B)
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin, "[%s] on player [%s]\n", cexec_ct_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_spec"))
		{
			if (!gpManiGameType->IsSpectatorAllowed()) return;

			// All spectators
			if (cexec_index >= cexec_spec_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_spec_list[cexec_index].cexec_command);

			// All CT's
			if (cexec_index >= cexec_ct_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_ct_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != gpManiGameType->GetSpectatorIndex())
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin, "[%s] on player [%s]\n", cexec_spec_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_all"))
		{
			// All players
			if (cexec_index >= cexec_all_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_all_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin, "[%s] on player [%s]\n", cexec_all_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else
		{
			return;
		}

		Q_strcpy(cexec_log_cmd, cexec_cmd);
		// Chop carriage return
		if (cexec_log_cmd)
		{
			length = Q_strlen(cexec_log_cmd);
			if (length != 0)
			{
				cexec_log_cmd[Q_strlen(cexec_log_cmd) - 1] = '\0';
			}
		}

		LogCommand (admin, "%s command [%s]\n", command, cexec_log_cmd);
	}
	else
	{
		FreeMenu();

		// Setup cexec lists
		if (FStrEq(command,"cexec"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_list_size, &menu_list_size);
			for( int i = 0; i < cexec_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin cexec_player %i", i);
			}
		}
		else if (FStrEq(command,"cexec_t"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_t_list_size, &menu_list_size);
			for( int i = 0; i < cexec_t_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_t_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_ct"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_ct_list_size, &menu_list_size);
			for( int i = 0; i < cexec_ct_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_ct_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_spec"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_spec_list_size, &menu_list_size);
			for( int i = 0; i < cexec_spec_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_spec_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_all"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_all_list_size, &menu_list_size);
			for( int i = 0; i < cexec_all_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_all_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(270), Translate(271), next_index, "admin", command, true, -1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		char	client_cmd[512];
		const int	command_index = atoi(gpCmd->Cmd_Argv(2 + argv_offset));

		if (command_index < 0 || command_index >= cexec_list_size)
		{
			return;
		}

		player.user_id = Q_atoi(gpCmd->Cmd_Argv(3 + argv_offset));
		if (!FindPlayerByUserID(&player))
		{
			return;
		}

		if (player.is_bot)
		{
			return;
		}

		int immunity_index = -1;
		if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
		{
			if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
			{
				return;
			}
		}

		Q_snprintf( client_cmd, sizeof(client_cmd), 
					"%s\n", 
					cexec_list[command_index].cexec_command);
		
		LogCommand (admin, "[%s] on player [%s]\n", cexec_list[command_index].cexec_command, player.steam_id);
		engine->ClientCommand(player.entity, client_cmd);
		return;
	}
	else
	{
		char	more_player_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				continue;
			}

			if (player.is_bot) continue;

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_CEXEC))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_player %s %i", gpCmd->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_player_cmd, sizeof(more_player_cmd), "cexec_player %s", gpCmd->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(280), Translate(281), next_index, "admin", more_player_cmd, true,	-1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose:  Show the Map Management menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMapManagementType(player_t *player, int admin_index )
{

	FreeMenu();


	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(592));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin changemap");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHANGEMAP) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(593));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin setnextmap");
	}


	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(672));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin");

	DrawStandardMenu(player, Translate(590), Translate(591), true);

}

//---------------------------------------------------------------------------------
// Purpose:  Show the player Management screens
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerManagementType(player_t *player, int admin_index, int next_index)
{

	FreeMenu();

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAY) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(612));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slay");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(613));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kicktype");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(614));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin bantype");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(615));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin swapteam");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(619));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin specplayer");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SWAP) && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(616));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin balanceteam");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CEXEC_MENU) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(617));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexecoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MUTE) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(618));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mute");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SPRAY_TAG) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(620));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin spray");
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, Translate(610), Translate(611), next_index, "admin", "playeroptions", true, -1);

}
//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPunishType(player_t *player, int admin_index, int next_index)
{

	FreeMenu();


	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SLAP) && !war_mode && gpManiGameType->IsSlapAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(292));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slapoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BLIND) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(293));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blindoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZE) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(294));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freeze");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_DRUG) && !war_mode && gpManiGameType->IsDrugAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(295));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin drug");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_GIMP) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(296));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin gimp");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		player_settings_t *player_settings;

		player_settings = FindPlayerSettings(player);
		if (player_settings)
		{
			for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
			{
				if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
				{
					AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
					Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(297));
					Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin teleport");
					break;
				}
			}
		}
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_TELEPORT) && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(298));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin savelocation");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BURN) && !war_mode && gpManiGameType->IsFireAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(299));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin burn");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_NO_CLIP) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(300));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin noclip");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SETSKINS) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(301));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin skinoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_TIMEBOMB) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(302));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin timebomb");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_FIREBOMB) && !war_mode && gpManiGameType->IsFireAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(303));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin firebomb");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_FREEZEBOMB) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(304));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freezebomb");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BEACON) && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(305));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin beacon");
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, Translate(290), Translate(291), next_index, "admin", "punish", true, -1);

}

//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessVoteType(player_t *player, int admin_index, int next_index)
{

	if (war_mode) return;

	FreeMenu();

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_RCON_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(312));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votercon");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_QUESTION_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(313));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votequestion");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(314));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteextend");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_RANDOM_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(315));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin randomvoteoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(316));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mapvoteoptions");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(317));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin buildmapvote");
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_MAP_VOTE) && !gpManiVote->SysVoteInProgress())
	{
		int		m_list_size;
		map_t	*m_list;

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
			}
		}

		if (selected_map != 0)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(318));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin multimapvoteoptions");
		}
	}

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_CANCEL_VOTE) && gpManiVote->SysVoteInProgress())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(319));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cancelvote");
	}

	if (menu_list_size == 0) return;


	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, Translate(310), Translate(311), next_index, "admin", "voteoptions",	true, -1);

}

//---------------------------------------------------------------------------------
// Purpose: Show Delay Type Options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessDelayTypeOptions( player_t *player, const char *menu_command)
{

	char	next_menu[128];

	if (gpManiVote->SysVoteInProgress()) return;

	if (FStrEq(menu_command,"randomvoteoptions"))
	{
		Q_strcpy(next_menu, "randommapvote");
	}
	else if(FStrEq(menu_command,"mapvoteoptions"))
	{
		Q_strcpy(next_menu, "singlemapvote");
	}
	else 
	{
		// Start multimap vote
		Q_strcpy(next_menu, "multimapvote");
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(332));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_NO_DELAY);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(333));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_END_OF_ROUND_DELAY);
	}

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(334));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_END_OF_MAP_DELAY);

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteoptions");

	DrawStandardMenu(player, Translate(330), Translate(331), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanOptions( edict_t *pEntity, const char *ban_command)
{
	player_t	player;
	char	ban_type[128];

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	if (FStrEq(ban_command,"banoptions"))
	{
		Q_strcpy(ban_type, "ban");
	}
	else if(FStrEq(ban_command,"banoptionsip"))
	{
		Q_strcpy(ban_type, "banip");
	}
	else 
	{
		Q_strcpy(ban_type, "autobanname");
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(352));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 5", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(353));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 30", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(354));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 60", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(355));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 120", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(356));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 1440", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(357));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 10080", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(358));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 0", ban_type);

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(&player, Translate(350), Translate(351), true);

	return;
}


//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSlapOptions( edict_t *pEntity)
{
	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	if (!gpManiGameType->IsSlapAllowed()) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(372));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 0");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(373));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 1");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(374));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 5");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(375));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 10");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(376));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 20");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(377));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 50");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(378));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 99");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");

	DrawStandardMenu(&player, Translate(370), Translate(371), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Show user the blindness amounts
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBlindOptions( edict_t *pEntity)
{
	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(392));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 0");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(393));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 245");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(394));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 255");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");

	DrawStandardMenu(&player, Translate(390), Translate(391), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanType( player_t *player )
{
	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(402));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin banoptions");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(403));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin banoptionsip");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(404));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autobanoptionsname");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(player, Translate(400), Translate(401), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessKickType(player_t *player )
{
	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(422));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kick");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(423));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokickname");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(424));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokicksteam");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(425));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokickip");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(player, Translate(420), Translate(421), true);

	return;
}


//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecOptions( edict_t *pEntity )
{
	player_t	player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}


	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(442));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(443));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_all");

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(444));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_t");

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(445));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_ct");
	}

	if (gpManiGameType->IsSpectatorAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(446));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_spec");
	}

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(&player, Translate(440), Translate(441), true);

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Plugin config options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessConfigOptions( edict_t *pEntity )
{

	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_adverts.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(462));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(463));
	}

	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle adverts");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_tk_protection.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(464));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(465));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle tk_protection");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_tk_forgive.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(466));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(467));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle tk_forgive");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_war_mode.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(468));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(469));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle warmode");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_stats.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(470));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(471));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle stats");


	if (mani_stats.GetInt() == 1)
	{
		int admin_index = -1;
		if (gpManiClient->IsAdmin(&player, &admin_index))
		{
			if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_RESET_ALL_RANKS))
			{
				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(472));
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle resetstats");
			}
		}
	}


	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin");

	DrawStandardMenu(&player, Translate(460), Translate(461), true);

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Plugin config options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessConfigToggle( edict_t *pEntity )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	// Must have 3 arguements
	if (argc != 3)
	{
		return;
	}

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	if (FStrEq(gpCmd->Cmd_Argv(2), "adverts"))
	{
		ToggleAdverts(&player);
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "tk_protection"))
	{
		if (mani_tk_protection.GetInt() == 1)
		{
			mani_tk_protection.SetValue(0);
			FreeList((void **) &tk_player_list, &tk_player_list_size);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled tk protection", player.name);
			LogCommand (&player, "Disable tk protection\n");
		}
		else
		{
			mani_tk_protection.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled tk protection", player.name);
			LogCommand (&player, "Enable tk protection\n");
			// Need to turn off tk punish
			engine->ServerCommand("mp_tkpunish 0\n");
		}
		return;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "tk_forgive"))
	{
		if (mani_tk_forgive.GetInt() == 1)
		{
			mani_tk_forgive.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled tk forgive options", player.name);
			LogCommand (&player, "Disable tk forgive\n");
		}
		else
		{
			mani_tk_forgive.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled tk forgive options", player.name);
			LogCommand (&player, "Enable tk forgive\n");
		}
		return;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "warmode"))
	{
		if (mani_war_mode.GetInt() == 1)
		{
			mani_war_mode.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled War Mode", player.name);
			LogCommand (&player, "Disable war mode\n");
		}
		else
		{
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled War Mode", player.name);
			LogCommand (&player, "Enable war mode\n");
			mani_war_mode.SetValue(1);
		}
		return;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "stats"))
	{
		if (mani_stats.GetInt() == 1)
		{
			mani_stats.SetValue(0);
			SayToAll (GREEN_CHAT, true, "ADMIN %s disabled stats", player.name);
			LogCommand (&player, "Disable stats\n");
		}
		else
		{
			mani_stats.SetValue(1);
			SayToAll (GREEN_CHAT, true, "ADMIN %s enabled stats", player.name);
			LogCommand (&player, "Enable stats\n");
		}
		return;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "resetstats"))
	{
		int admin_index = -1;
		if (!gpManiClient->IsAdmin(&player, &admin_index)) return;
		if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_RESET_ALL_RANKS)) return;

		gpManiStats->ResetStats ();
		SayToAll (GREEN_CHAT, true, "ADMIN %s reset the stats", player.name);
		LogCommand (&player, "Reset stats\n");
		return;
	}
	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		char string1[128];
		char string2[128];

		Q_strcpy(string1, gpCmd->Cmd_Argv(2 + argv_offset));
		Q_strcpy(string2, gpCmd->Cmd_Argv(3 + argv_offset));
		gpCmd->NewCmd();


		if (FStrEq("ban", ban_command))
		{
			gpCmd->AddParam("ma_ban");
			gpCmd->AddParam("%s", string1);
			gpCmd->AddParam("%s", string2);
			ProcessMaBan(admin, "ma_ban", 0, M_MENU);
		}
		else
		{
			gpCmd->AddParam("ma_banip");
			gpCmd->AddParam("%s", string1);
			gpCmd->AddParam("%s", string2);
			ProcessMaBanIP(admin, "ma_banip", 0, M_MENU);
		}

		return;
	}
	else
	{
		char	more_ban_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_BAN))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %s %i", ban_command, gpCmd->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_ban_cmd, sizeof(more_ban_cmd), "%s %s", ban_command, gpCmd->Cmd_Argv(2 + argv_offset));

		if (FStrEq("ban", ban_command))
		{
			DrawSubMenu (admin, Translate(500), Translate(501), next_index, "admin", more_ban_cmd,	true, -1);
		}
		else
		{
			DrawSubMenu (admin, Translate(500), Translate(502), next_index, "admin", more_ban_cmd, true, -1);
		}
	}
	
	return;
}



//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CAdminPlugin::FireGameEvent( IGameEvent * event )
{
	if (ProcessPluginPaused()) return;

//	const char *event_name = event->GetName();
//  MMsg("Event Name [%s]\n", event->GetName());

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
	if (!IsOnSameTeam (&victim, &attacker))	return;

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
			char	string_to_show[1024];
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

			Q_snprintf (string_to_show, sizeof(string_to_show), "(%s) %s attacked a teammate", 
				Translate(gpManiGameType->GetTeamShortTranslation(attacker.team)), attacker.name);

			// Show to all spectators
			SayToTeam (ORANGE_CHAT, ct, t, true, "%s", string_to_show);
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
	else if (FStrEq(say_string, "settings") && !war_mode) {ShowSettingsPrimaryMenu(&player, 0); return;}
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
		engine->ClientCommand(player.entity, "mani_uservotemapmenu\n"); return;
	}
	else if (FStrEq(say_string, "votekick") && !war_mode && 
		mani_voting.GetInt() == 1 && 
		mani_vote_allow_user_vote_kick.GetInt() == 1) 
	{
		engine->ClientCommand(player.entity, "mani_uservotekickmenu\n"); return;
	}
	else if (FStrEq(say_string, "voteban") && !war_mode && 
		mani_voting.GetInt() == 1 && 
		mani_vote_allow_user_vote_ban.GetInt() == 1) 
	{
		engine->ClientCommand(player.entity, "mani_uservotebanmenu\n"); return;
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

		Q_snprintf( time_text, sizeof(time_text), "The time is : %s %s\n", tmp_buf, mani_thetime_timezone.GetString());
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
				Q_snprintf(ff_message, sizeof(ff_message), "Friendly fire is on");
				PrintToClientConsole(player.entity, "Friendly fire is on\n");
			}
			else
			{
				Q_snprintf(ff_message, sizeof(ff_message), "Friendly fire is off");
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
			gpManiStats->ShowTop(&player, 10);
		}
		else
		{
			gpManiStats->ShowTop(&player, Q_atoi(&(say_string[3])));
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
		gpManiCSSBounty->ShowTop(&player);
	}
	else if (FStrEq(say_string,"statsme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		gpManiStats->ShowStatsMe(&player, &player);
	}
	else if (FStrEq(say_string,"session") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		gpManiStats->ShowSession(&player, &player);
	}
	else if (FStrEq(say_string,"hitboxme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		gpManiStats->ShowHitBoxMe(&player);
	}
	else if (FStrEq(say_string,"weaponme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		engine->ClientCommand(player.entity, "mani_weaponme 1\n");
	}
	else if ( FStrEq( say_string, "vote" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		engine->ClientCommand(player.entity, "mani_votemenu\n");
		return;
	}

	else if ( FStrEq( say_string, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		gpManiVote->ProcessRockTheVoteNominateMap(&player);
	}
	else if ( FStrEq( say_string, "rockthevote" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		gpManiVote->ProcessMaRockTheVote(&player);
	}
	else if ( FStrEq( say_string, "favourites" ) && !war_mode)
	{
		// Show web favourites menu
		engine->ClientCommand(player.entity, "favourites\n");
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
		RemoveRestrictedWeapons(&spawn_player);
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
		ResetWeaponCount();
	}

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
		get_new_timeleft_offset = true;
		timeleft_offset = gpGlobals->curtime;
		for (int i = 0; i < MANI_MAX_TEAMS; i++)
		{
			team_scores.team_score[i] = 0;
		}
	}

	round_end_found = true;

	if (mani_autobalance_teams.GetInt() == 1 && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		change_team = true;
		change_team_time = gpGlobals->curtime + 2.4;
	}

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
	const char  *point_name = event->GetString("cpname", "");
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
				Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
				LogCommand (NULL, "Kick (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
				engine->ServerCommand(kick_cmd);				
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 1 && sv_lan->GetInt() != 1)
			{
				// Ban by user id
				SayToAll(ORANGE_CHAT, false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
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
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
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

				if (sv_lan->GetInt() != 1)
				{
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_player_name_change_ban_time.GetInt(), 
											player->user_id);
					LogCommand(NULL, "Banned (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);	
					engine->ServerCommand("writeid\n");
				}

				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
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
// Purpose: Routine for showing KeyValue contents
//---------------------------------------------------------------------------------

void  CAdminPlugin::PrettyPrinter(KeyValues *keyValue, int indent) {

   // Sanity check!
   if (!keyValue)
      return;

   // This code is just for creating blank strings of size <indent>.
   // It maks our output look nested like the example file.
   // There are probably many better ways of doing this!
   char buff[100];
   for (int i = 0; i < indent; i++)
      buff[i] = ' ';
   buff[indent] = '\0';

   // Remember, not all nodes have a value, so we check that here!
   // Also, see notes below!
   if (Q_strlen(keyValue->GetString()) > 0)
      DirectLogCommand("%skey: %s, value: %s\n", buff,
         keyValue->GetName(),
         keyValue->GetString());
   else
      DirectLogCommand("%skey: %s\n", buff,
         keyValue->GetName(),
         keyValue->GetString());

   // This is the guts of the function.  See below for an explanation!
   KeyValues *kv;
   kv = keyValue->GetFirstSubKey();
   while (kv) {

      // Recursive call!
      PrettyPrinter(kv, indent + 4);

      // Get the next sibling.
      kv = kv->GetNextKey();
   }
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
	health = Prop_GetHealth(attacker->entity);
	if (health <= 0) return;
		
	health -= ((damage_to_inflict >= 0) ? damage_to_inflict : damage_to_inflict * -1);
	if (health <= 0)
	{
		health = 0;
	}

	Prop_SetHealth(attacker->entity, health);
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
	bool	spawn_protection = false;
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

	if (mani_show_death_beams.GetInt() != 0)
	{
		ProcessDeathBeam(&attacker, &victim);
	}

	bool	menu_displayed = false;

	if (!gpManiWarmupTimer->IgnoreTK())
	{
		menu_displayed = ProcessTKDeath(&attacker, &victim);
	}

	// Must go after Process TK Death or menu check will not work !!!!
	gpManiVictimStats->PlayerDeath(&victim, &attacker, attacker_exists, headshot, weapon_name, menu_displayed);

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
	bool	spawn_protection = false;
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

	bool	menu_displayed = false;

	if (!gpManiWarmupTimer->IgnoreTK())
	{
		menu_displayed = ProcessTKDeath(&attacker, &victim);
	}

	// Must go after Process TK Death or menu check will not work !!!!
	gpManiVictimStats->DODSPlayerDeath(&victim, &attacker, attacker_exists, weapon_index, menu_displayed);
	gpManiMostDestructive->PlayerDeath(&victim, &attacker, attacker_exists);
	gpManiLogDODSStats->PlayerDeath(&victim, &attacker, attacker_exists, weapon_index);

}

//---------------------------------------------------------------------------------
// Purpose: Show Tampered message
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowTampered(void)
{
	MMsg("****************************************************************************************\n");
	MMsg("****************************************************************************************\n");
	MMsg("*             Mani Admin Plugin has been altered by an unauthorised person             *\n");
	MMsg("* Please go to www.mani-admin-plugin.com to download the unaltered and correct version *\n");
	MMsg("****************************************************************************************\n");
	MMsg("****************************************************************************************\n");

	engine->LogPrint("************************************************************************************************************\n");
	engine->LogPrint("************************************************************************************************************\n");
	engine->LogPrint("* [MANI ADMIN PLUGIN] Mani Admin Plugin has been altered by an unauthorised person                         *\n");
	engine->LogPrint("* [MANI ADMIN PLUGIN] Please go to www.mani-admin-plugin.com to download the unaltered and correct version *\n");
	engine->LogPrint("************************************************************************************************************\n");
	engine->LogPrint("************************************************************************************************************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Forces certain cheat vars to default values every 1.5 seconds
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCheatCVars(void)
{
	if (gpGlobals->curtime >= last_cheat_check_time + 10.0)
	{
		last_cheat_check_time = gpGlobals->curtime;
		ProcessCheatCVarCommands();
	}

	return;
}


//---------------------------------------------------------------------------------
// Purpose: Read the stats
//---------------------------------------------------------------------------------
bool CAdminPlugin::HookSayCommand(bool team_say)
{

	player_t player;

	if (engine->IsDedicatedServer() && con_command_index == -1) return true;
	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player))
	{
		return true;
	}

	if (player.is_bot) return true;

	gpCmd->ExtractSayCommand(team_say);
	if (gpCmd->Cmd_Argc() == 0) return true;

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

	if (!war_mode && !punish_mode_list[player.index - 1].gimped)
	{
		if (mani_filter_words_mode.GetInt() != 0)
		{
			// Process string for swear words if player not gimped
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

/*	int argc_index = 1;
	char *pcmd = (char *) gpCmd->Cmd_SayArg0();
	char *pcmd1 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd2 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd3 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd4 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd5 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd6 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd7 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd8 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd9 = (char *) gpCmd->Cmd_Argv(argc_index++);
	char *pcmd10 =(char *)  gpCmd->Cmd_Argv(argc_index++);
	char *pcmd11 = (char *) gpCmd->Cmd_Argv(argc_index++);
*/
	if (!CheckForReplacement(&player, (char *) gpCmd->Cmd_Argv(0)))
	{
		// RCon or client command command executed
		return false;
	}

//	gpCmd->HandleCommand(NULL, 

	// All say commands begin with an @ symbol
	int say_argc = gpCmd->Cmd_Argc();

	if (strcmp(gpCmd->Cmd_SayArg0(),"") != 0)
	{
		if (team_say)
		{
			if (gpCmd->HandleCommand(&player, M_TSAY) == PLUGIN_STOP) return false;
		}
		else
		{
			if (gpCmd->HandleCommand(&player, M_SAY) == PLUGIN_STOP) return false;
		}
	}

	// Normal say command
	// Is swear word in there ?
	if (found_swear_word)
	{
		if (mani_filter_words_mode.GetInt() == 1)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", mani_filter_words_warning.GetString());
			return false;
		}

		// Mode 2 show filtered string
		char	client_cmd[2048];

		if (!team_say)
		{
			Q_snprintf(client_cmd, sizeof (client_cmd), "say %s\n", gpCmd->Cmd_Args(0));
		}
		else
		{
			Q_snprintf(client_cmd, sizeof (client_cmd), "say_team %s\n", gpCmd->Cmd_Args(0));
		}

		engine->ClientCommand(player.entity, client_cmd);
		return false;
	}

	if (!war_mode && punish_mode_list[player.index - 1].gimped)
	{
		if (gimp_phrase_list_size == 0) return true;

		// player is in gimped mode
		// Check if say string is in gimp word list
		for (int i = 0; i < gimp_phrase_list_size; i++)
		{
			if (FStrEq(gimp_phrase_list[i].phrase, gpCmd->Cmd_Args(0))) return true;
		}

		// If here then player say hasn't been altered yet
		int gimp_phrase = rand() % gimp_phrase_list_size;
		engine->ClientCommand(player.entity, "say %s\n", gimp_phrase_list[gimp_phrase].phrase);
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

		Q_snprintf(changelevel_command, sizeof(changelevel_command), "changelevel %s\n", forced_nextmap);
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
	int	admin_index;
	char kick_cmd[256];

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_KICK))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
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

			Q_snprintf( kick_cmd, sizeof(kick_cmd), "bot_kick \"%s\"\n", &(target_player_list[i].name[j]));
			LogCommand (player_ptr, "bot_kick [%s]\n", target_player_list[i].name);
			engine->ServerCommand(kick_cmd);
			continue;
		}

		PrintToClientConsole(target_player_list[i].entity, "You have been kicked by Admin\n");
		Q_snprintf( kick_cmd, sizeof(kick_cmd), 
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
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
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
		Q_snprintf( unban_cmd, sizeof(unban_cmd), "removeid %s\n", target_string);
	}
	else
	{
		Q_snprintf( unban_cmd, sizeof(unban_cmd), "removeip %s\n", target_string);
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
	int	admin_index;
	char ban_cmd[256];

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int time_to_ban = Q_atoi(gpCmd->Cmd_Argv(1));

	if (sv_lan && sv_lan->GetInt() == 1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot ban by ID when on LAN or everyone gets banned !!\n");
		return PLUGIN_STOP;
	}

	const char *target_string = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_BAN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to ban
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
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

		Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", time_to_ban, target_player_list[i].user_id);

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
	int	admin_index;
	char ban_cmd[256];

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int time_to_ban = Q_atoi(gpCmd->Cmd_Argv(1));
	const char *target_string = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_BAN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to ban
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
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

		Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", time_to_ban, target_player_list[i].ip_address);

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
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SLAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	const char *target_string = gpCmd->Cmd_Argv(1);
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_SLAY))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to slay
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;

	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	int offset_number = Q_atoi(target_string);

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
	int	admin_index;

	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

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
	int	admin_index;

	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *start_range = gpCmd->Cmd_Argv(2);
	const char *end_range = gpCmd->Cmd_Argv(3);

	int start_offset = Q_atoi(start_range);
	int end_offset = Q_atoi(end_range);

	if (start_offset > end_offset)
	{
		end_offset = Q_atoi(start_range);
		start_offset = Q_atoi(end_range);
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

	int	target_value = Q_atoi(target_string);

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
	int	admin_index;

	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin

	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *start_range = gpCmd->Cmd_Argv(2);
	const char *end_range = gpCmd->Cmd_Argv(3);

	int start_offset = Q_atoi(start_range);
	int end_offset = Q_atoi(end_range);

	if (start_offset > end_offset)
	{
		end_offset = Q_atoi(start_range);
		start_offset = Q_atoi(end_range);
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

	float	target_value = Q_atof(target_string);

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
	int	admin_index;

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *damage_string = gpCmd->Cmd_Argv(2);

	if (!gpManiGameType->IsSlapAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SLAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_SLAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int damage = 0;

	if (gpCmd->Cmd_Argc() == 3)
	{
		damage = Q_atoi(damage_string);
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
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_CASH, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *amount = gpCmd->Cmd_Argv(2);
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Convert to float and int
	float fAmount = Q_atof(amount);
	int iAmount = Q_atoi(amount);

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

		int target_cash = Prop_GetAccount(target_player_list[i].entity);

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

		Prop_SetAccount(target_player_list[i].entity, new_cash);
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
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_HEALTH, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *amount = gpCmd->Cmd_Argv(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Convert to float and int
	float fAmount = Q_atof(amount);
	int iAmount = Q_atoi(amount);

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
		target_health = Prop_GetHealth(target_player_list[i].entity);
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
			Prop_SetHealth(target_player_list[i].entity, new_health);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *blind_amount_string = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BLIND, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_BLIND))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int blind_amount = 0;

	if (gpCmd->Cmd_Argc() == 3)
	{
		blind_amount = Q_atoi(blind_amount_string);
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
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_FREEZE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_FREEZE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].frozen == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_NO_CLIP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (!gpManiGameType->IsFireAllowed()) return PLUGIN_STOP;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BURN, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_BURN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to burn
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);
	if (!gpManiGameType->IsDrugAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_DRUG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_DRUG))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to drug
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].drugged == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	if (!gpManiGameType->GetAdvancedEffectsAllowed()) return PLUGIN_STOP;

	if (!player_ptr) return PLUGIN_CONTINUE;
	const char *decal_name = gpCmd->Cmd_Argv(1);

	// Check if player is admin
	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *item_name = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_GIVE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_GIVE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *weapon_slot_str = gpCmd->Cmd_Argv(2);
	const char *primary_fire_str = gpCmd->Cmd_Argv(3);
	const char *amount_str = gpCmd->Cmd_Argv(4);
	const char *noise_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_GIVE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 5) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_GIVE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
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

	int	weapon_slot = Q_atoi(weapon_slot_str);
	int	primary_fire_index = Q_atoi(primary_fire_str);
	int	amount = Q_atoi(amount_str);
	bool primary_fire = false;

	if (FStrEq(primary_fire_str,"1")) primary_fire = true;
	if (amount < 0) amount = 0; else if (amount > 1000) amount = 1000;
	if (weapon_slot < 0) weapon_slot = 0; else if (weapon_slot > 20) weapon_slot = 20;


	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *red_str = gpCmd->Cmd_Argv(2);
	const char *green_str = gpCmd->Cmd_Argv(3);
	const char *blue_str = gpCmd->Cmd_Argv(4);
	const char *alpha_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 6) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int red = Q_atoi(red_str);
	int green = Q_atoi(green_str);
	int blue = Q_atoi(blue_str);
	int alpha = Q_atoi(alpha_str);

	if (red > 255) red = 255; else if (red < 0) red = 0;
	if (green > 255) green = 255; else if (green < 0) green = 0;
	if (blue > 255) blue = 255; else if (blue < 0) blue = 0;
	if (alpha > 255) alpha = 255; else if (alpha < 0) red = 0;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *red_str = gpCmd->Cmd_Argv(2);
	const char *green_str = gpCmd->Cmd_Argv(3);
	const char *blue_str = gpCmd->Cmd_Argv(4);
	const char *alpha_str = gpCmd->Cmd_Argv(5);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 6) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int red = Q_atoi(red_str);
	int green = Q_atoi(green_str);
	int blue = Q_atoi(blue_str);
	int alpha = Q_atoi(alpha_str);

	if (red > 255) red = 255; else if (red < 0) red = 0;
	if (green > 255) green = 255; else if (green < 0) green = 0;
	if (blue > 255) blue = 255; else if (blue < 0) blue = 0;
	if (alpha > 255) alpha = 255; else if (alpha < 0) red = 0;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *gravity_string = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_GRAVITY, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_GRAVITY))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	int gravity = Q_atof(gravity_string);

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		CBaseEntity *pCBE = EdictToCBE(target_player_list[i].entity);

		int index;

		// Need to set gravity
		index = gpManiGameType->GetPtrIndex(pCBE, MANI_VAR_GRAVITY);
		if (index != -2)
		{
			float *gravity_ptr;
			gravity_ptr = ((float *)pCBE + index);
			*gravity_ptr = (gravity * 0.01);
		}

		LogCommand (player_ptr, "set user gravity [%s] [%s] to [%f]\n", target_player_list[i].name, target_player_list[i].steam_id, gravity);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingravity_anonymous.GetInt(), "set player %s gravity", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rendermode command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRenderMode(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *render_mode_str = gpCmd->Cmd_Argv(2);
	int	render_mode;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	render_mode = Q_atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->SetRenderMode((RenderMode_t) render_mode);
		Prop_SetRenderMode(target_player_list[i].entity, render_mode);
		
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
	int	admin_index;
	int	render_mode;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *render_mode_str = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	render_mode = Q_atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_list[i].name));
			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->m_nRenderFX = render_mode;
		
		Prop_SetRenderFX(target_player_list[i].entity, render_mode);
		
		LogCommand (player_ptr, "set user renderfx [%s] [%s] to [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, render_mode);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admincolor_anonymous.GetInt(), "set player %s to renderfx %i", target_player_list[i].name,  render_mode); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_gimp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGimp(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_GIMP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_GIMP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr,"%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].gimped)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessGimpPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "gimped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingimp_anonymous.GetInt(), "gimped player %s", target_player_list[i].name); 
				SayToAll(ORANGE_CHAT, false, "%s", mani_gimp_transform_message.GetString());
			}
		}
		else
		{
			ProcessUnGimpPlayer(&(target_player_list[i]));
			LogCommand (player_ptr, "un-gimped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_admingimp_anonymous.GetInt(), "un-gimped player %s", target_player_list[i].name); 
				SayToAll(ORANGE_CHAT, false, "%s", mani_gimp_untransform_message.GetString());
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_timebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTimeBomb(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_TIMEBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_TIMEBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].time_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_FIREBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_FIREBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to fire bomb
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].fire_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_FREEZEBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_FREEZEBOMB))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].freeze_bomb == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_BEACON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_BEACON))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to turn into beacons
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].beacon == MANI_TK_ENFORCED)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_UNDER_TK,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *toggle = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_MUTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_MUTE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		int	do_action = 0;

		if (gpCmd->Cmd_Argc() == 3)
		{
			do_action = Q_atoi(toggle);
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *x_coords = gpCmd->Cmd_Argv(2);
	const char *y_coords = gpCmd->Cmd_Argv(3);
	const char *z_coords = gpCmd->Cmd_Argv(4);
	int argc = gpCmd->Cmd_Argc();

	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_CONTINUE;


	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc != 2 && argc != 5 || 
		(!player_ptr && argc == 2) ||
		(argc == 2 && !CanTeleport(player_ptr))) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	Vector origin;
	Vector *origin2 = NULL;

	if (argc == 5)
	{
		origin.x = Q_atof(x_coords);
		origin.y = Q_atof(y_coords);
		origin.z = Q_atof(z_coords);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_TELEPORT))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
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
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_DEAD,"%s", target_player_ptr->name));
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
	int	admin_index;

	// Server can't run this 
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin
	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;

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
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSwapTeam(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: %s This only works on team play games", command_name);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_SWAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on a team yet", target_player_list[i].name);
			continue;
		}

		// Swap player over
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			if (!CCSPlayer_SwitchTeam(EdictToCBE(target_player_list[i].entity),gpManiGameType->GetOpposingTeam(target_player_list[i].team)))
			{
				target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
			}
			else
			{
				// If not dead then force model change
				if (!target_player_list[i].player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(target_player_list[i].entity));
				}
			}
		}
		else
		{
			target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
		}

		LogCommand (player_ptr, "team swapped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								target_player_list[i].name, 
								Translate(gpManiGameType->GetTeamShortTranslation(gpManiGameType->GetOpposingTeam(target_player_list[i].team)))); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_spec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSpec(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsSpectatorAllowed())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: %s This only works on games with spectator capability", command_name);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_SWAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on a team yet", target_player_list[i].name);
			continue;
		}

		target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());

		LogCommand (player_ptr, "moved the following player to spectator [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "moved %s to be a spectator", target_player_list[i].name);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_balance command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBalance(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		if (gpCmd->Cmd_Argc() == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: This only works on team play games");
		}
		return PLUGIN_STOP;
	}

	bool mute_action = false;

	if (!player_ptr && gpCmd->Cmd_Argc() == 2)
	{
		mute_action = true;
	}

	if (mani_autobalance_mode.GetInt() == 0)
	{
		// Swap regardless if player is dead or alive
		ProcessMaBalancePlayerType(player_ptr, mute_action, true, true);
	}
	else if (mani_autobalance_mode.GetInt() == 1)
	{
		// Swap dead first, followed by Alive players if needed
		if (!ProcessMaBalancePlayerType(player_ptr, mute_action, true, false))
		{
			// Requirea check of alive people too
			ProcessMaBalancePlayerType(player_ptr, mute_action, false, false);
		}
	}
	else
	{
		// Dead only
		ProcessMaBalancePlayerType(player_ptr, mute_action, true, false);
	}	
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_dropc4 command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDropC4(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_DROPC4, war_mode, &admin_index)) return PLUGIN_STOP;
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

		CBaseEntity *pPlayer = bomb_player.entity->GetUnknown()->GetBaseEntity();

		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 4);

		if (pWeapon)
		{
			if (FStrEq("weapon_c4", CBaseCombatWeapon_GetName(pWeapon)))
			{
				CBasePlayer *pBase = (CBasePlayer *) pPlayer;
				CBasePlayer_WeaponDrop(pBase, pWeapon);

				if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
				{
					AdminSayToAll(GREEN_CHAT, player_ptr, mani_admindropc4_anonymous.GetInt(), "forced player %s to drop the C4", bomb_player.name); 
				}

				LogCommand (player_ptr, "forced c4 drop on player [%s] [%s]\n", bomb_player.name, bomb_player.steam_id);

				break;
			}
		}
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
bool	CAdminPlugin::ProcessMaBalancePlayerType
(
 player_t	*player_ptr,
 bool mute_action,
 bool dead_only,
 bool dont_care
)
{
	bool return_status = true;
	player_t target_player;
	int	t_count = 0;
	int	ct_count = 0;

	for (int i = 1; i <= max_players; i ++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team == TEAM_B) ct_count ++;
		else if (target_player.team == TEAM_A) t_count ++;
	}

	int team_difference = abs(ct_count - t_count);

	if (team_difference <= mp_limitteams->GetInt())
	{
		// No point balancing
		if (!mute_action)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
		}

		return true;
	}

	int team_to_swap = (ct_count > t_count) ? TEAM_B:TEAM_A;
	int number_to_swap = (team_difference / 2);
	if (number_to_swap == 0)
	{
		// No point balancing
		if (!mute_action)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
		}

		return true;
	}

	player_t *temp_player_list = NULL;
	int	temp_player_list_size = 0;

	for (int i = 1; i <= max_players; i++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team != team_to_swap) continue;
		if (!dont_care)
		{
			if (target_player.is_dead != dead_only) continue;
		}

		int immunity_index = -1;
		if (gpManiClient->IsImmune(&target_player, &immunity_index))
		{
			if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_BALANCE))
			{
				continue;
			}
		}

		// Player is a candidate
		AddToList((void **) &temp_player_list, sizeof(player_t), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = target_player;
	}

	if (temp_player_list_size == 0) return false;
	if (temp_player_list_size < number_to_swap)
	{
		// Not enough players for this type
		number_to_swap = temp_player_list_size;
		return_status = false;
	}

	int player_to_swap;

	while (number_to_swap != 0)
	{
		for (;;)
		{
			// Get player to swap
			player_to_swap = rand() % temp_player_list_size;
			if (player_to_swap >= temp_player_list_size) continue; // Safety
			if (temp_player_list[player_to_swap].team == team_to_swap) break;
		}

		// Swap player over
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			if (!CCSPlayer_SwitchTeam(EdictToCBE(temp_player_list[player_to_swap].entity),gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team)))
			{
				temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team));
			}
			else
			{
				// If not dead then force model change
				if (!temp_player_list[player_to_swap].player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(temp_player_list[player_to_swap].entity));
				}
			}
		}
		else
		{
			temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team));
		}

		temp_player_list[player_to_swap].team = gpManiGameType->GetOpposingTeam(team_to_swap);

		number_to_swap --;

		LogCommand (player_ptr, "team balanced user [%s] [%s]\n", temp_player_list[player_to_swap].name, temp_player_list[player_to_swap].steam_id);

		if (!mute_action)
		{
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								temp_player_list[player_to_swap].name, 
								Translate(gpManiGameType->GetTeamShortTranslation(temp_player_list[player_to_swap].team)));
			}
		}
	}

	FreeList((void **) &temp_player_list, &temp_player_list_size);

	return return_status;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_psay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaPSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_PSAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (player_ptr)
		{
			int client_admin = -1;
			bool target_is_admin;

			target_is_admin = false;

			if (gpManiClient->IsAdmin(target_player, &client_admin))
			{
				target_is_admin = true;
			}

			if (mani_adminsay_anonymous.GetInt() == 1 && !target_is_admin)
			{
				SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) to (%s) : %s", target_player->name, say_string);
				SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) to (%s) : %s", target_player->name, say_string);
			}
			else
			{
				SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, say_string);
				SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, say_string);
			}	

			LogCommand(player_ptr, "%s %s (ADMIN) %s to (%s) : %s\n", command_name, target_string, player_ptr->name, target_player->name, say_string);
		}
		else
		{
			SayToPlayer(GREEN_CHAT, target_player, "%s", say_string);
			LogCommand(player_ptr, "%s %s (CONSOLE) to (%s) : %s\n", command_name, target_string, target_player->name, say_string);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_msay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaMSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	int	time_to_display;
	msay_t	*lines_list = NULL;
	int		lines_list_size = 0;
	char	temp_line[2048];
	const char *time_display = gpCmd->Cmd_Argv(1);
	const char *target_string = gpCmd->Cmd_Argv(2);
	const char *say_string = gpCmd->Cmd_Args(3);

	if (mani_use_amx_style_menu.GetInt() == 0 || !gpManiGameType->IsAMXMenuAllowed()) return PLUGIN_STOP;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_PSAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	time_to_display = Q_atoi(time_display);
	if (time_to_display < 0) time_to_display = 0;
	else if (time_to_display > 100) time_to_display = 100;

	if (time_to_display == 0) time_to_display = -1;

	// Build up lines to display
	int	message_length = Q_strlen (say_string);

	Q_strcpy(temp_line,"");
	int	j = 0;
	int	i = 0;

	while (say_string[i] != '\0')
	{
		if (say_string[i] == '\\' && say_string[i + 1] != '\0')
		{
			switch (say_string[i + 1])
			{
			case 'n':
				{
					AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
					temp_line[j] = '\0';
					Q_strcat(temp_line,"\n");
					Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
					Q_strcpy(temp_line,"");
					j = -1;
					i++;
					break;
				}
			case '\\': temp_line[j] = '\\'; i++; break;
			default : temp_line[j] = say_string[i];break;
			}
		}
		else
		{
			temp_line[j] = say_string[i];
		}
		
		i ++;
		j ++;
	}

	temp_line[j] = '\0';
	if (temp_line[0] != '\0')
	{
		AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
		Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
	}

	// Found some players to talk to
	for (i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;
		menu_confirm[target_player->index - 1].in_use = false;

		for (j = 0; j < lines_list_size; j ++)
		{
			if (j == lines_list_size - 1)
			{
				DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, true);
			}
			else
			{
				DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, false);
			}
		}
	}

	FreeList((void **) &lines_list, &lines_list_size);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_say command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		if (!gpManiClient->IsAdmin(player_ptr, &admin_index))
		{
			if (!war_mode)
			{
				if (mani_allow_chat_to_admin.GetInt() == 1)
				{
					SayToAdmin (ORANGE_CHAT, player_ptr, "%s", say_string);
				}
				else
				{
					SayToPlayer (ORANGE_CHAT, player_ptr, "You are not allowed to chat directly to admin !!");
				}
			}
			return PLUGIN_STOP;
		}

		// Player is Admin
		if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHAT) || war_mode)
		{
			if (!war_mode)
			{
				SayToAdmin (GREEN_CHAT, player_ptr, "%s", say_string);
			}
			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(gpCmd->Cmd_Args(1), substitute_text, &col);

	LogCommand (player_ptr, "(ALL) %s %s\n", gpCmd->Cmd_Args(1), substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1 && !war_mode)
	{
		ClientMsg(&col, 10, false, 2, "%s", substitute_text);
	}

	if (mani_adminsay_chat_area.GetInt() == 1 || war_mode)
	{
		AdminSayToAll(LIGHT_GREEN_CHAT, player_ptr, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
	}

	if (mani_adminsay_bottom_area.GetInt() == 1 && !war_mode)
	{
		AdminHSayToAll(player_ptr, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_csay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_SAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 

	if (player_ptr)
	{
		AdminCSayToAll(player_ptr, mani_adminsay_anonymous.GetInt(), "%s", say_string);
	}
	else
	{
		CSayToAll("%s", say_string);
	}
			
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_chat command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaChat(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		if (!gpManiClient->IsAdmin(player_ptr, &admin_index))
		{
			if (!war_mode)
			{
				if (mani_allow_chat_to_admin.GetInt() == 1)
				{
					SayToAdmin (ORANGE_CHAT, player_ptr, "%s", say_string);
				}
				else
				{
					SayToPlayer (ORANGE_CHAT, player_ptr, "You are not allowed to chat directly to admin !!");
				}
			}
			return PLUGIN_STOP;
		}

		// Player is Admin
		if (!gpManiClient->IsAdminAllowed(admin_index, ALLOW_CHAT) || war_mode)
		{
			if (!war_mode)
			{
				SayToAdmin (GREEN_CHAT, player_ptr, "%s", say_string);
			}
			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(gpCmd->Cmd_Args(1), substitute_text, &col);

	LogCommand (player_ptr, "(CHAT) %s %s\n", command_name, substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1)
	{
		ClientMsg(&col, 15, true, 2, "%s", substitute_text);
	}
 
	if (mani_adminsay_chat_area.GetInt() == 1)
	{
		AdminSayToAdmin(GREEN_CHAT, player_ptr, "%s", substitute_text);
	}
					
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rcon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRCon(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;

	char	rcon_cmd[2048];

	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RCON, false, &admin_index)) return PLUGIN_STOP;

	LogCommand (player_ptr, "%s %s\n", command_name, gpCmd->Cmd_Args(1)); 
	Q_snprintf( rcon_cmd, sizeof(rcon_cmd), "%s\n", gpCmd->Cmd_Args(1));
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
	int	admin_index;
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_CEXEC, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_ALLOW_CEXEC))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	char	client_cmd[2048];

	Q_snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player_ptr, "%s \"%s\" %s\n", command_name, target_string, say_string); 
	OutputHelpText(ORANGE_CHAT, player_ptr, "Ran %s", client_cmd);
			
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
// Purpose: Process the ma_users command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUsers(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_players = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
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
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Did not find any players");
		return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current User List\n\n");
	OutputToConsole(player_ptr, "Names = number of times a player has changed name\n");
	OutputToConsole(player_ptr, "A Ghost Name                Steam ID             IP Address       UserID\n");
	OutputToConsole(player_ptr, "------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		bool	is_admin;
		int		admin_index;

		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			continue;
		}

		if (gpManiClient->IsAdmin(server_player, &admin_index))
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
// Purpose: Process the ma_rates command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRates(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *target_players = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_MA_RATES, false, &admin_index)) return PLUGIN_STOP;
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
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_DONT_CARE))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Did not find any players");
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
		int nCmdRate = Q_atoi( szCmdRate );
		int nUpdateRate = Q_atoi( szUpdateRate );
		int nRate = Q_atoi( szRate );
		float nInterp = Q_atof( szInterp );

		
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
	int	admin_index;
	const char *filter = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current Plugin server var settings\n\n");

    ConCommandBase *pPtr = cvar->GetCommands();
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
						ConVar *mani_var = cvar->FindVar(name);
						OutputToConsole(player_ptr, "%s %s\n", name, mani_var->GetString());
					}
				}
				else
				{
					// Found mani cvar
					ConVar *mani_var = cvar->FindVar(name);
					OutputToConsole(player_ptr, "%s %s\n", name, mani_var->GetString());
				}
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_help command
//---------------------------------------------------------------------------------
/*PLUGIN_RESULT	CAdminPlugin::ProcessMaHelp(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *filter = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player_ptr, "Current Plugin server console commands\n\n");

    ConCommandBase *pPtr = cvar->GetCommands();
    while (pPtr)
	{
		if (pPtr->IsCommand())
		{
			const char *name = pPtr->GetName();

			if (NULL != Q_stristr(name, "ma_"))
			{
				if (argc == 2)
				{
					if (NULL != Q_stristr(name, filter))
					{
						OutputToConsole(player_ptr, "%s\n", name);
					}
				}
				else
				{
					OutputToConsole(player_ptr, "%s\n", name);
				}
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}

	OutputToConsole(player_ptr, "nextmap (chat and console)\n");
	OutputToConsole(player_ptr, "damage (console and chat)\n");
	OutputToConsole(player_ptr, "deathbeam (chat only)\n");
	OutputToConsole(player_ptr, "quake (chat only)\n");
	OutputToConsole(player_ptr, "listmaps (console only))\n");
	OutputToConsole(player_ptr, "votemap (chat and console)\n");
	OutputToConsole(player_ptr, "votekick (chat and console)\n");
	OutputToConsole(player_ptr, "voteban (chat and console)\n");
	OutputToConsole(player_ptr, "nominate (chat and console)\n");
	OutputToConsole(player_ptr, "rockthevote (chat and console)\n");
	OutputToConsole(player_ptr, "timeleft (chat only)\n");
	OutputToConsole(player_ptr, "thetime (chat only)\n");
	OutputToConsole(player_ptr, "ff (chat only)\n");
	OutputToConsole(player_ptr, "rank (chat only)\n");
	OutputToConsole(player_ptr, "statsme (chat only)\n");
	OutputToConsole(player_ptr, "hitboxme (chat only)\n");
	OutputToConsole(player_ptr, "session (chat only)\n");
	OutputToConsole(player_ptr, "top (chat only)\n");

	return PLUGIN_STOP;
}
*/
//---------------------------------------------------------------------------------
// Purpose: Process the ma_saveloc
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSaveLoc(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_STOP;

	if (!player_ptr) return PLUGIN_STOP;

	// Check if player is admin
	if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;

	ProcessSaveLocation(player_ptr);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Current location saved, any players will be teleported here");

	return PLUGIN_STOP;
}



//---------------------------------------------------------------------------------
// Purpose: Process the ma_war
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaWar(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	admin_index;
	const char *option = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_WAR, false, &admin_index)) return PLUGIN_STOP;
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

	int	option_val = Q_atoi(option);

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
				Q_snprintf(time_string, sizeof(time_string), "Timeleft %i:%02i", minutes_portion, seconds_portion, gpGlobals->curtime);	
			}

			follow_string = true;
			no_timelimit = false;
		}
		else 
		{
			Q_snprintf(time_string, sizeof(time_string), "No timelimit for map");
		}
	}

	if (mp_fraglimit && mp_fraglimit->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(fraglimit_string, sizeof(fraglimit_string), ", or change map after player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
		}
		else
		{
			Q_snprintf(fraglimit_string, sizeof(fraglimit_string), "Map will change after a player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
			follow_string = true;
		}

	}

	if (mp_winlimit && mp_winlimit->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(winlimit_string, sizeof(winlimit_string), ", or change map after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
		}
		else
		{
			Q_snprintf(winlimit_string, sizeof(winlimit_string), "Map will change after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (mp_maxrounds && mp_maxrounds->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(maxrounds_string, sizeof(maxrounds_string), ", or change map after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
		}
		else
		{
			Q_snprintf(maxrounds_string, sizeof(maxrounds_string), "Map will change after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (!last_round)
	{
		if (no_timelimit && follow_string)
		{
			Q_snprintf(final_string, sizeof(final_string), "%s%s%s", fraglimit_string, winlimit_string, maxrounds_string);
		}
		else
		{
			Q_snprintf(final_string, sizeof(final_string), "%s%s%s%s", time_string, fraglimit_string, winlimit_string, maxrounds_string);
		}
	}
	else
	{
		Q_snprintf(final_string, sizeof(final_string), "This is the last round !!");
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
SCON_COMMAND(ma_gravity, 2057, MaGravity, false);
SCON_COMMAND(ma_color, 2059, MaColour, false);
SCON_COMMAND(ma_colour, 2061, MaColour, false);
SCON_COMMAND(ma_colorweapon, 2063, MaColourWeapon, false);
SCON_COMMAND(ma_colourweapon, 2065, MaColourWeapon, false);
SCON_COMMAND(ma_render_mode, 2067, MaRenderMode, false);
SCON_COMMAND(ma_render_fx, 2069, MaRenderFX, false);
SCON_COMMAND(ma_gimp, 2079, MaGimp, false);
SCON_COMMAND(ma_timebomb, 2081, MaTimeBomb, false);
SCON_COMMAND(ma_firebomb, 2083, MaFireBomb, false);
SCON_COMMAND(ma_freezebomb, 2083, MaFreezeBomb, false);
SCON_COMMAND(ma_beacon, 2087, MaBeacon, false);
SCON_COMMAND(ma_mute, 2089, MaMute, false);
SCON_COMMAND(ma_teleport, 2091, MaTeleport, false);
SCON_COMMAND(ma_psay, 2009, MaPSay, false);
SCON_COMMAND(ma_msay, 2007, MaMSay, true);
SCON_COMMAND(ma_say, 2005, MaSay, true);
SCON_COMMAND(ma_csay, 2013, MaCSay, true);
SCON_COMMAND(ma_chat, 2011, MaChat, false);
SCON_COMMAND(ma_cexec, 2023, MaCExec, false);
SCON_COMMAND(ma_users, 2179, MaUsers, true);
SCON_COMMAND(ma_rates, 2177, MaRates, true);
SCON_COMMAND(ma_showsounds, 2181, MaShowSounds, false);
SCON_COMMAND(ma_play, 2137, MaPlaySound, false);
SCON_COMMAND(ma_swapteam, 2095, MaSwapTeam, true);
SCON_COMMAND(ma_spec, 2097, MaSpec, false);
SCON_COMMAND(ma_balance, 2099, MaBalance, false);
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

CON_COMMAND(ma_sql, "ma_sql <sql to run>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	request_list_t *request_list_ptr = NULL;
	MMsg("1\n");
	mysql_thread->AddRequest(0, &request_list_ptr);
	MMsg("2\n");
	mysql_thread->AddSQL(request_list_ptr, 0, "%s", engine->Cmd_Args());
	MMsg("3\n");
	mysql_thread->PostRequest(request_list_ptr);
	MMsg("4\n");
}

static void WarModeChanged ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (atoi(var->GetString()) == 0)
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

static void ManiAdminPluginVersion ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		mani_admin_plugin_version.SetValue(PLUGIN_CORE_VERSION);
	}
}

static void ManiTickrate ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		mani_tickrate.SetValue(server_tickrate);
	}
}

static void ManiCSStackingNumLevels ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (cs_stacking_num_levels)
		{
			cs_stacking_num_levels->m_fMaxVal = var->GetFloat();
			cs_stacking_num_levels->SetValue(var->GetInt());
		}
	}
}

static void ManiUnlimitedGrenades ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()) && 
		gpManiGameType->IsGameType(MANI_GAME_CSS) && 
		sv_cheats &&
		!war_mode)
	{
		if (var->GetInt() == 1)
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

static void ManiStatsBySteamID ( ConVar *var, char const *pOldString )
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
		if (var->GetInt() == 1 && FStrEq(player.steam_id, "STEAM_ID_PENDING")) continue;
		
		gpManiStats->NetworkIDValidated(&player);
	}
}


//---------------------------------------------------------------------------------
// Purpose: Checks if version string has been altered
//---------------------------------------------------------------------------------
bool	CAdminPlugin::IsTampered(void)
{
	return false;
	int checksum = 0;
	int plus1 = 10000;
	int str_length = Q_strlen(mani_version);

	for (int i = 0; i < str_length; i ++)
	{
		checksum += (int) mani_version[i];
	}

	checksum += 0x342F;

// MMsg("Checksum string %i\n", checksum);
//MMsg("Offset required %i\n", checksum - plus1);
//while(1);

	if (checksum != (plus1 + 8407))
	{
		return true;
	}

	plus1 += 453;
	return false;
}

#ifndef SOURCEMM
//**************************************************************************************************
// Special hook for say commands
//**************************************************************************************************

class CSayHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLSayCommand;
public:
   CSayHook() : ConCommand("say", NULL, "say messages", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         MMsg("Didn't find say command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLSayCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say
      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookSayCommand(false)) return;
      // Forward to gamedll
      m_pGameDLLSayCommand->Dispatch();
   }
};

CSayHook g_SayHook;

//**************************************************************************************************
// Special hook for say commands
//**************************************************************************************************

class CSayTeamHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLSayCommand;
public:
   CSayTeamHook() : ConCommand("say_team", NULL, "say messages to teammates", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say_team") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         MMsg("Didn't find say_team command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLSayCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookSayCommand(true)) return;
      // Forward to gamedll
      m_pGameDLLSayCommand->Dispatch();
   }
};

CSayTeamHook g_SayTeamHook;

//**************************************************************************************************
// Special hook for autobuy commands
//**************************************************************************************************

class CAutobuyHook : public ConCommand
{
   // This will hold the pointer original gamedll autobuy command
   ConCommand *m_pGameDLLAutobuyCommand;
public:
   CAutobuyHook() : ConCommand("autobuy", NULL, "autobuy", FCVAR_GAMEDLL), m_pGameDLLAutobuyCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll autobuy command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "autobuy") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }

      if (!pPtr)
	  {
         MMsg("Didn't find autobuy command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLAutobuyCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the autobuy

      if(!ProcessPluginPaused() && !HookAutobuyCommand()) return;
      // Forward to gamedll
      m_pGameDLLAutobuyCommand->Dispatch();
   }
};

CAutobuyHook g_AutobuyHook;

//**************************************************************************************************
// Special hook for rebuy commands
//**************************************************************************************************
class CRebuyHook : public ConCommand
{
   // This will hold the pointer original gamedll rebuy command
   ConCommand *m_pGameDLLRebuyCommand;
public:
   CRebuyHook() : ConCommand("rebuy", NULL, "rebuy", FCVAR_GAMEDLL), m_pGameDLLRebuyCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll rebuy command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "rebuy") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }

      if (!pPtr)
	  {
         MMsg("Didn't find rebuy command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLRebuyCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !HookRebuyCommand()) return;
      // Forward to gamedll
      m_pGameDLLRebuyCommand->Dispatch();

	  if(!ProcessPluginPaused()) PostProcessRebuyCommand();
   }
};

CRebuyHook g_RebuyHook;



class CChangeLevelHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLChangeLevelCommand;
public:
   CChangeLevelHook() : ConCommand("changelevel", NULL, "changelevel", FCVAR_GAMEDLL), m_pGameDLLChangeLevelCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "changelevel") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         MMsg("Didn't find changelevel command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLChangeLevelCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookChangeLevelCommand()) return;
      // Forward to gamedll
      m_pGameDLLChangeLevelCommand->Dispatch();
   }
};

CChangeLevelHook g_ChangeLevelHook;
#endif
