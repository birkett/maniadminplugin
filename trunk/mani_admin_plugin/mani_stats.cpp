//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "Color.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "bitbuf.h"
#include "mani_main.h"
#include "mani_client_flags.h"
#include "mani_language.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_parser.h"
#include "mani_maps.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_maps.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_commands.h"
#include "mani_help.h"
#include "mani_warmuptimer.h"
#include "mani_keyvalues.h"
#include "mani_stats.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerPluginHelpers *helpers;
extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	IFileSystem	*filesystem;
extern	bool war_mode;
extern	int	max_players;
extern	bool just_loaded;
extern	CGlobalVars *gpGlobals;
extern	ConVar *vip_version;
extern	time_t	g_RealTime;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static int sort_by_steam_id ( const void *m1,  const void *m2); 
static int sort_by_name ( const void *m1,  const void *m2); 

static int sort_by_kills ( const void *m1,  const void *m2); 
static int sort_by_kd_ratio ( const void *m1,  const void *m2);
static int sort_by_kills_deaths ( const void *m1,  const void *m2);
static int sort_by_points ( const void *m1,  const void *m2);

static int sort_by_name_kills ( const void *m1,  const void *m2); 
static int sort_by_name_kd_ratio ( const void *m1,  const void *m2);
static int sort_by_name_kills_deaths ( const void *m1,  const void *m2);
static int sort_by_name_points ( const void *m1,  const void *m2);

static int sort_by_kills_weapon ( const void *m1,  const void *m2);

// Define Weapons
static	char	*hitboxes[MANI_MAX_STATS_HITGROUPS] = 
{
"hg","hh","hc","ht","la","ra","ll","rl"
};

static	char	*hitboxes_text[MANI_MAX_STATS_HITGROUPS] = 
{
"hit_generic","hit_head","hit_chest","hit_stomach","hit_leftarm","hit_righarm","hit_leftleg","hit_rightleg"
};

static	int	hitboxes_gui_text[MANI_MAX_STATS_HITGROUPS] = 
{
M_VSTATS_BODY,M_VSTATS_HEAD,M_VSTATS_CHEST,M_VSTATS_STOMACH,M_VSTATS_LEFT_ARM,M_VSTATS_RIGHT_ARM,M_VSTATS_LEFT_LEG,M_VSTATS_RIGHT_LEG
};

// Define Weapons
static	char	*css_weapons[MANI_MAX_STATS_CSS_WEAPONS] = 
{
"ak47","m4a1","mp5navy","awp","usp","deagle","aug","hegrenade","xm1014",
"knife","g3sg1","sg550","galil","m3","scout","sg552","famas",
"glock","tmp","ump45","p90","m249","elite","mac10","fiveseven",
"p228","flashbang","smokegrenade"
};

static	char	*css_weapons_nice[MANI_MAX_STATS_CSS_WEAPONS] = 
{
"AK-47","M4A1","MP5-Navy","AWP","USP","Deagle","Aug","HE Grenade","XM1014",
"Knife","G3-SG1","SG-550","Galil","M3","Scout","SG-552","Famas",
"Glock","Tmp","UMP-45","P90","M249","Elite","MAC-10","Five-Seven",
"P-228","Flashbang","Smoke Grenade"
};

static	char	*weapon_short_name[MANI_MAX_STATS_CSS_WEAPONS] =
{
"2a","2b","2c","2d","2e","2f","2g","2h","2i","2j","2k","2l","2m","2n","2o","2p","2q","2r","2s","2t","2u","2v","2w","2x","2y","2z","3a","3b"
};

// This is a map for weapon bytes sent via events, the indexes translate into 
// our text name array.
static	int		map_dod_weapons[50] = 
{
//0  1  2  3  4  5  6  7  8  9
 -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 
  9, 10,11,12,13,14,15,16,17,18, 
  19,-1,-1,20,21,22,23,-1,-1,24,
  24, 5, 7, 8, 9,14,15,13,12,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

static	char	*dod_weapons_nice[MANI_MAX_STATS_DODS_WEAPONS] =
{
"Amerknife", "Spade", 
"Colt", "P-38", "C-96", 
"Garand", "M1-Carbine", "K-98", 
"Spring", "Scoped K-98", 
"Thompson", "MP-40", "MP-44", "Bar", 
"30-Cal", "MG-42", 
"Bazooka", "P-Schreck", 
"Frag HE-US", "Frag HE-Ger", 
"Smoke US", "Smoke Ger", 
"Rifle Grenade US", "Rifle Grenade Ger",
"Punch"
};

static	char	*dod_weapons_log[MANI_MAX_STATS_DODS_WEAPONS] =
{
"amerknife", "spade", 
"colt", "p38", "c96", 
"garand", "m1carbine", "k98", 
"spring", "k98_scoped", 
"thompson", "mp40", "mp44", "bar", 
"30cal", "mg42", 
"bazooka", "pschreck", 
"frag_us", "frag_ger", 
"smoke_us", "smoke_ger", 
"riflegren_us", "riflegren_ger",
"punch"
};

//static char *stats_user_def_name[MANI_MAX_USER_DEF] = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p"};
static char *stats_user_def_name[MANI_MAX_USER_DEF] = {"1a","1b","1c","1d","1e","1f","1g","1h","1i","1j","1k","1l","1m","1n","1o","1p"};

CONVAR_CALLBACK_PROTO(ManiStatsDecayStart);
CONVAR_CALLBACK_PROTO(ManiStatsDecayPeriod);
CONVAR_CALLBACK_PROTO(ManiStatsIgnoreRanks);

ConVar mani_stats_decay_start ("mani_stats_decay_start", "2", 0, "This defines the number of days before points decay starts, default is 2 days",true, 0, true, 365, CONVAR_CALLBACK_REF(ManiStatsDecayStart)); 
ConVar mani_stats_decay_period ("mani_stats_decay_period", "7", 0, "This defines the number of days that the decay period will last before points flat line at 500 points, default is 7 days",true, 0, true, 365, CONVAR_CALLBACK_REF(ManiStatsDecayPeriod)); 
ConVar mani_stats_decay_restore_points_on_connect ("mani_stats_decay_restore_points_on_connect", "1", 0, "0 = Full points not restored on player reconnect if points decayed, 1 = Full points restored on reconnect if points decayed",true, 0, true, 1 ); 
ConVar mani_stats_points_add_only ("mani_stats_points_add_only", "0", 0, "If set to 0 you lose points for being killed, if set to 1 you do not",true, 0, true, 1); 
ConVar mani_stats_ignore_ranks_after_x_days ("mani_stats_ignore_ranks_after_x_days", "21", 0, "After this many days, ranked players are ignored from the rank output (they are not deleted)",true, 0, true, 365, CONVAR_CALLBACK_REF(ManiStatsIgnoreRanks)); 

ConVar mani_stats_points_multiplier ("mani_stats_points_multiplier", "5.0", 0, "Multiplier used in a kill calculation", true, -100, true, 100); 
ConVar mani_stats_points_death_multiplier ("mani_stats_points_death_multiplier", "1.0", 0, "Multiplier used against the points removed from a player if killed", true, -100, true, 100); 
ConVar mani_stats_players_needed ("mani_stats_players_needed", "2", 0, "Players need per active team before stats can be calculated, if not team based then number of active players on server", true, 0, true, 10); 
ConVar mani_stats_kills_before_points_removed ("mani_stats_kills_before_points_removed", "0", 0, "Number of kills + deaths a new player needs before their own kills start affecting other players points", true, 0, true, 500); 

// CSS weapon weights
ConVar mani_stats_css_weapon_ak47 ("mani_stats_css_weapon_ak47", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_m4a1 ("mani_stats_css_weapon_m4a1", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_mp5navy ("mani_stats_css_weapon_mp5navy", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_awp ("mani_stats_css_weapon_awp", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_usp ("mani_stats_css_weapon_usp", "1.4", 0, "Weapon weight (1.4 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_deagle ("mani_stats_css_weapon_deagle", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_aug ("mani_stats_css_weapon_aug", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_hegrenade ("mani_stats_css_weapon_hegrenade", "1.8", 0, "Weapon weight (1.8 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_xm1014 ("mani_stats_css_weapon_xm1014", "1.1", 0, "Weapon weight (1.1 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_knife ("mani_stats_css_weapon_knife", "2", 0, "Weapon weight (2.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_g3sg1 ("mani_stats_css_weapon_g3sg1", "0.8", 0, "Weapon weight (0.8 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_sg550 ("mani_stats_css_weapon_sg550", "0.8", 0, "Weapon weight (0.8 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_galil ("mani_stats_css_weapon_galil", "1.1", 0, "Weapon weight (1.1 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_m3 ("mani_stats_css_weapon_m3", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_scout ("mani_stats_css_weapon_scout", "1.1", 0, "Weapon weight (1.1 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_sg552 ("mani_stats_css_weapon_sg552", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_famas ("mani_stats_css_weapon_famas", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_glock ("mani_stats_css_weapon_glock", "1.4", 0, "Weapon weight (1.4 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_tmp ("mani_stats_css_weapon_tmp", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_ump45 ("mani_stats_css_weapon_ump45", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_p90 ("mani_stats_css_weapon_p90", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_m249 ("mani_stats_css_weapon_m249", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_elite ("mani_stats_css_weapon_elite", "1.4", 0, "Weapon weight (1.4 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_mac10 ("mani_stats_css_weapon_mac10", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_fiveseven ("mani_stats_css_weapon_fiveseven", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_p228("mani_stats_css_weapon_p228", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_flashbang ("mani_stats_css_weapon_flashbang", "5", 0, "Weapon weight (5.0 default)", true, -100, true, 100); 
ConVar mani_stats_css_weapon_smokegrenade ("mani_stats_css_weapon_smokegrenade", "5.0", 0, "Weapon weight (5.0 default)", true, -100, true, 100); 

ConVar mani_stats_css_bomb_planted_bonus ("mani_stats_css_bomb_planted_bonus", "10", 0, "Bomb Planted bonus points", true, -100, true, 100); 
ConVar mani_stats_css_bomb_defused_bonus ("mani_stats_css_bomb_defused_bonus", "10", 0, "Bomb Defused bonus points", true, -100, true, 100); 
ConVar mani_stats_css_hostage_rescued_bonus ("mani_stats_css_hostage_rescued_bonus", "5", 0, "Hostage rescued bonus points", true, -100, true, 100); 
ConVar mani_stats_css_hostage_killed_bonus ("mani_stats_css_hostage_killed_bonus", "-15", 0, "Hostage killed bonus points", true, -100, true, 100); 
ConVar mani_stats_css_vip_escape_bonus ("mani_stats_css_vip_escape_bonus", "4", 0, "VIP escape bonus (requires LDuke VIP Plugin)", true, -100, true, 100); 
ConVar mani_stats_css_vip_killed_bonus ("mani_stats_css_vip_killed_bonus", "10", 0, "VIP killed bonus (requires LDuke VIP Plugin)", true, -100, true, 100); 

// Team bonuses
ConVar mani_stats_css_ct_eliminated_team_bonus ("mani_stats_css_ct_eliminated_team_bonus", "2", 0, "All CTs killed all T team bonus", true, -100, true, 100); 
ConVar mani_stats_css_t_eliminated_team_bonus ("mani_stats_css_t_eliminated_team_bonus", "2", 0, "All Ts killed all CT team bonus", true, -100, true, 100); 
ConVar mani_stats_css_ct_vip_escaped_team_bonus ("mani_stats_css_ct_vip_escaped_team_bonus", "10", 0, "VIP escaped team bonus (requires LDuke VIP Plugin)", true, -100, true, 100); 
ConVar mani_stats_css_t_vip_assassinated_team_bonus ("mani_stats_css_t_vip_assassinated_team_bonus", "6", 0, "VIP assasinated team bonus (requires LDuke VIP Plugin)", true, -100, true, 100); 
ConVar mani_stats_css_t_target_bombed_team_bonus ("mani_stats_css_t_target_bombed_team_bonus", "5", 0, "Bomb exploded team bonus", true, -100, true, 100); 
ConVar mani_stats_css_ct_all_hostages_rescued_team_bonus ("mani_stats_css_ct_all_hostages_rescued_team_bonus", "10", 0, "All hostages rescued bonus", true, -100, true, 100); 
ConVar mani_stats_css_ct_bomb_defused_team_bonus ("mani_stats_css_ct_bomb_defused_team_bonus", "5", 0, "Bomb defused team bonus", true, -100, true, 100); 
ConVar mani_stats_css_ct_hostage_killed_team_bonus ("mani_stats_css_ct_hostage_killed_team_bonus", "1", 0, "CT Team bonus for a hostage being killed", true, -100, true, 100); 
ConVar mani_stats_css_ct_hostage_rescued_team_bonus ("mani_stats_css_ct_hostage_rescued_team_bonus", "1", 0, "Per hostage rescue CT team bonus", true, -100, true, 100); 
ConVar mani_stats_css_t_bomb_planted_team_bonus ("mani_stats_css_t_bomb_planted_team_bonus", "2", 0, "Bomb planted team bonus", true, -100, true, 100); 

// Dods weightings
ConVar mani_stats_dods_weapon_amerknife ("mani_stats_dods_weapon_amerknife", "3.0", 0, "Weapon weight (3.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_spade ("mani_stats_dods_weapon_spade", "3.0", 0, "Weapon weight (3.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_colt ("mani_stats_dods_weapon_colt", "1.6", 0, "Weapon weight (1.6 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_p38 ("mani_stats_dods_weapon_p38", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_c96 ("mani_stats_dods_weapon_c96", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_garand ("mani_stats_dods_weapon_garand", "1.3", 0, "Weapon weight (1.3 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_m1carbine ("mani_stats_dods_weapon_m1carbine", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_k98 ("mani_stats_dods_weapon_k98", "1.3", 0, "Weapon weight (1.3 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_spring ("mani_stats_dods_weapon_spring", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_k98_scoped ("mani_stats_dods_weapon_k98_scoped", "1.5", 0, "Weapon weight (1.5 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_thompson ("mani_stats_dods_weapon_thompson", "1.25", 0, "Weapon weight (1.25 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_mp40 ("mani_stats_dods_weapon_mp40", "1.25", 0, "Weapon weight (1.25 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_mp44 ("mani_stats_dods_weapon_mp44", "1.35", 0, "Weapon weight (1.35 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_bar ("mani_stats_dods_weapon_bar", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_30cal ("mani_stats_dods_weapon_30cal", "1.25", 0, "Weapon weight (1.25 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_mg42 ("mani_stats_dods_weapon_mg42", "1.2", 0, "Weapon weight (1.2 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_bazooka ("mani_stats_dods_weapon_bazooka", "2.25", 0, "Weapon weight (2.25 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_pschreck ("mani_stats_dods_weapon_pschreck", "2.25", 0, "Weapon weight (2.25 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_frag_us ("mani_stats_dods_weapon_frag_us", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_frag_ger ("mani_stats_dods_weapon_frag_ger", "1", 0, "Weapon weight (1.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_smoke_us ("mani_stats_dods_weapon_smoke_us", "5", 0, "Weapon weight (5.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_smoke_ger ("mani_stats_dods_weapon_smoke_ger", "5", 0, "Weapon weight (5.0 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_riflegren_us ("mani_stats_dods_weapon_riflegren_us", "1.3", 0, "Weapon weight (1.3 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_riflegren_ger ("mani_stats_dods_weapon_riflegren_ger", "1.3", 0, "Weapon weight (1.3 default)", true, -100, true, 100); 
ConVar mani_stats_dods_weapon_punch ("mani_stats_dods_weapon_punch", "3.0", 0, "Weapon weight (3.0 default)", true, -100, true, 100); 

ConVar mani_stats_dods_capture_point ("mani_stats_dods_capture_point", "4", 0, "Bomb Planted bonus points", true, -100, true, 100); 
ConVar mani_stats_dods_block_capture ("mani_stats_dods_block_capture", "4", 0, "Bomb Defused bonus points", true, -100, true, 100); 
ConVar mani_stats_dods_round_win_bonus ("mani_stats_dods_round_win_bonus", "4", 0, "Points given to all players on winning team", true, -100, true, 100); 

ManiStats::ManiStats()
{
	// Init
	rank_player_list = NULL;
	rank_player_name_list = NULL;
	rank_player_waiting_list = NULL;
	rank_player_name_waiting_list = NULL;
	rank_list = NULL;
	rank_name_list = NULL;

	rank_list_size = 0;
	rank_name_list_size = 0;
	rank_player_list_size = 0;
	rank_player_name_list_size = 0;
	rank_player_waiting_list_size = 0;
	rank_player_name_waiting_list_size = 0;
	time(&last_stats_write_time);
	time(&last_stats_calculate_time);

	level_ended = false;

	// Setup hash table for user search speed improvment
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}

	// Setup hash table for weapon search speed improvment
	for (int i = 0; i < 255; i ++)
	{
		weapon_hash_table[i] = -1;
	}

	gpManiStats = this;
}

ManiStats::~ManiStats()
{
	// Cleanup
	this->FreeStats();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiStats::Load(void)
{
	level_ended = false;

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		for (int i = 0; i < MANI_MAX_STATS_CSS_WEAPONS; i++)
		{
			weapon_hash_table[this->GetCSSWeaponHashIndex(css_weapons[i])] = i;
			weapon_weights[i] = NULL;
		}

		// Set up weapon weighting pointers
		int index = 0;

		weapon_weights[index++] = &mani_stats_css_weapon_ak47;
		weapon_weights[index++] = &mani_stats_css_weapon_m4a1;
		weapon_weights[index++] = &mani_stats_css_weapon_mp5navy;
		weapon_weights[index++] = &mani_stats_css_weapon_awp;
		weapon_weights[index++] = &mani_stats_css_weapon_usp;
		weapon_weights[index++] = &mani_stats_css_weapon_deagle;
		weapon_weights[index++] = &mani_stats_css_weapon_aug;
		weapon_weights[index++] = &mani_stats_css_weapon_hegrenade;
		weapon_weights[index++] = &mani_stats_css_weapon_xm1014;
		weapon_weights[index++] = &mani_stats_css_weapon_knife;
		weapon_weights[index++] = &mani_stats_css_weapon_g3sg1;
		weapon_weights[index++] = &mani_stats_css_weapon_sg550;
		weapon_weights[index++] = &mani_stats_css_weapon_galil;
		weapon_weights[index++] = &mani_stats_css_weapon_m3;
		weapon_weights[index++] = &mani_stats_css_weapon_scout;
		weapon_weights[index++] = &mani_stats_css_weapon_sg552;
		weapon_weights[index++] = &mani_stats_css_weapon_famas;
		weapon_weights[index++] = &mani_stats_css_weapon_glock;
		weapon_weights[index++] = &mani_stats_css_weapon_tmp;
		weapon_weights[index++] = &mani_stats_css_weapon_ump45;
		weapon_weights[index++] = &mani_stats_css_weapon_p90;
		weapon_weights[index++] = &mani_stats_css_weapon_m249;
		weapon_weights[index++] = &mani_stats_css_weapon_elite;
		weapon_weights[index++] = &mani_stats_css_weapon_mac10;
		weapon_weights[index++] = &mani_stats_css_weapon_fiveseven;
		weapon_weights[index++] = &mani_stats_css_weapon_p228;
		weapon_weights[index++] = &mani_stats_css_weapon_flashbang;
		weapon_weights[index++] = &mani_stats_css_weapon_smokegrenade;
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		// Set up weapon weighting pointers
		int index = 0;

		weapon_weights[index++] = &mani_stats_dods_weapon_amerknife;
		weapon_weights[index++] = &mani_stats_dods_weapon_spade;
		weapon_weights[index++] = &mani_stats_dods_weapon_colt;
		weapon_weights[index++] = &mani_stats_dods_weapon_p38;
		weapon_weights[index++] = &mani_stats_dods_weapon_c96;
		weapon_weights[index++] = &mani_stats_dods_weapon_garand;
		weapon_weights[index++] = &mani_stats_dods_weapon_m1carbine;
		weapon_weights[index++] = &mani_stats_dods_weapon_k98;
		weapon_weights[index++] = &mani_stats_dods_weapon_spring;
		weapon_weights[index++] = &mani_stats_dods_weapon_k98_scoped;
		weapon_weights[index++] = &mani_stats_dods_weapon_thompson;
		weapon_weights[index++] = &mani_stats_dods_weapon_mp40;
		weapon_weights[index++] = &mani_stats_dods_weapon_mp44;
		weapon_weights[index++] = &mani_stats_dods_weapon_bar;
		weapon_weights[index++] = &mani_stats_dods_weapon_30cal;
		weapon_weights[index++] = &mani_stats_dods_weapon_mg42;
		weapon_weights[index++] = &mani_stats_dods_weapon_bazooka;
		weapon_weights[index++] = &mani_stats_dods_weapon_pschreck;
		weapon_weights[index++] = &mani_stats_dods_weapon_frag_us;
		weapon_weights[index++] = &mani_stats_dods_weapon_frag_ger;
		weapon_weights[index++] = &mani_stats_dods_weapon_smoke_us;
		weapon_weights[index++] = &mani_stats_dods_weapon_smoke_ger;
		weapon_weights[index++] = &mani_stats_dods_weapon_riflegren_us;
		weapon_weights[index++] = &mani_stats_dods_weapon_riflegren_ger;
		weapon_weights[index++] = &mani_stats_dods_weapon_punch;
	}

	this->LoadStats();

	// Setup hash table for user search speed improvment
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		Q_memset(&(session[i]), 0, sizeof(session_t));
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (FStrEq(player.steam_id,"STEAM_ID_PENDING")) continue;

		hash_table[player.user_id] = i;
	}

/*
	Some timing tests, strcmp vs hashlookup
	Winner = Hashlookup by a factor of 2

	int j = 0;
	float timer = gpGlobals->curtime;
	for (int i = 0; i < 20000000; i ++)
	{
		int x = this->GetCSSWeaponIndex(css_weapons[rand() % 28]);
		j += x;
	}

	MMsg("strcmp = %f\n", gpGlobals->curtime - timer);
	MMsg("j = %i\n", j);

	int k = 0; 
	float timer2 = gpGlobals->curtime;
	for (int i = 0; i < 20000000; i++)
	{
		int x = this->GetCSSWeaponHashIndex(css_weapons[rand() % 28]);
		k += x;
	}
	MMsg("hash = %f\n", gpGlobals->curtime - timer2);
	MMsg("k = %i\n", k);
	while(1);
*/

}

//---------------------------------------------------------------------------------
// Purpose: Plugin Un-Loaded
//---------------------------------------------------------------------------------
void	ManiStats::Unload(void)
{
	this->FreeStats();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiStats::LevelInit(const char *map_name)
{
	//WriteDebug("******\nLEVEL INIT MAP %s\n******\n" ,map_name);

	this->LoadStats();
	level_ended = false;
}

//---------------------------------------------------------------------------------
// Purpose: Load Stats into memory
//---------------------------------------------------------------------------------
void	ManiStats::LoadStats(void)
{
	time_t	current_time;

	for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
	{
		active_player_list[i].active = false;
		active_player_list[i].rank_ptr = NULL;
		active_player_list[i].last_hit_time = -999.0;
		active_player_list[i].last_user_id = -1;
	}

	if (rank_player_list_size == 0)
	{
		// Try and read the stats file
		this->ReadStats(true);
	}

	if (rank_player_name_list_size == 0)
	{
		// Try and read the stats by name file
		this->ReadStats(false);
	}

	ReBuildStatsList(true);
	ReBuildStatsList(false);
	CalculateStats(true, false);
	CalculateStats(false, false);

	WriteStats(true);
	WriteStats(false);

	time(&current_time);

	for (int i = 1; i <= max_players; i ++)
	{
		player_t	player;
		rank_t		*rank_ptr;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (FStrEq(player.steam_id,"STEAM_ID_PENDING")) continue;

		// Need to find player stats from core list
		rank_ptr = FindStoredRank(&player);
		if (rank_ptr == NULL)
		{
			// Should never happen !! Pointer will either come from the 
			// waiting list or the main core list
			//WriteDebug("ManiStats::LoadStats() Failed to get stored rank for player [%s] [%s]\n" , player.name, player.steam_id);
			continue;
		}

		// Assign to list containing active players on the server
		rank_ptr->points_decay = 0.0;
		rank_ptr->last_connected = current_time;
		active_player_list[player.index - 1].active = true;
		active_player_list[player.index - 1].rank_ptr = rank_ptr;	
		active_player_list[player.index - 1].last_hit_time = -999.0;
		active_player_list[player.index - 1].last_user_id = -1;
	}

	int	steam_size = (sizeof(rank_t) * rank_player_list_size) + (sizeof(rank_t *) * rank_player_list_size) +
					 (sizeof(rank_t *) * rank_list_size);
	int	name_size = (sizeof(rank_t) * rank_player_name_list_size) + (sizeof(rank_t *) * rank_player_name_list_size) +
					(sizeof(rank_t *) * rank_name_list_size);

	float steam_size_meg = 0;
	float name_size_meg = 0;

	if (steam_size != 0)
	{
		steam_size_meg = ((float) steam_size) / 1048576.0;
	}

	if (name_size != 0)
	{
		name_size_meg = ((float) name_size) / 1048576.0;
	}

	MMsg("Steam ID Player Stats memory usage %fMB with %i records\n", steam_size_meg, rank_player_list_size);
	MMsg("Name Player Stats memory usage %fMB with %i records\n", name_size_meg, rank_player_name_list_size);
}
//---------------------------------------------------------------------------------
// Purpose: Player has been validated and is ready for accepting stats
//---------------------------------------------------------------------------------
void	ManiStats::NetworkIDValidated(player_t *player_ptr)
{
	rank_t	*rank_ptr = NULL;
	time_t	current_time;

	//WriteDebug("ManiStats::NetworkIDValidated() Player [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);

	if (player_ptr->is_bot) return;
	if (mani_stats.GetInt() == 0) return;

	active_player_list[player_ptr->index - 1].active = false;
	active_player_list[player_ptr->index - 1].rank_ptr = NULL;

	// Need to find player stats from core list
	rank_ptr = FindStoredRank(player_ptr);
	if (rank_ptr == NULL)
	{
		// Should never happen !! Pointer will either come from the 
		// waiting list or the main core list
		//WriteDebug("ManiStats::NetworkIDValidated() Failed to get stored rank for player [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);
		return;
	}

	// Assign to list containing active players on the server
	time(&current_time);

	// Store ip address in compact form
	this->GetIPList(player_ptr->ip_address, rank_ptr->ip_address);

	if (mani_stats_decay_restore_points_on_connect.GetInt() == 0)
	{
		// Strip the decayed points from player
		rank_ptr->points = rank_ptr->points - rank_ptr->points_decay;
		if (rank_ptr->points < 500) 
		{
			rank_ptr->points = 500;
		}
	}

	rank_ptr->points_decay = 0.0;
	rank_ptr->last_connected = current_time;
	active_player_list[player_ptr->index - 1].rank_ptr = rank_ptr;
	active_player_list[player_ptr->index - 1].last_hit_time = -999.0;
	active_player_list[player_ptr->index - 1].last_user_id = -1;
	active_player_list[player_ptr->index - 1].active = true;

	if (hash_table[player_ptr->user_id] == -1)
	{
		hash_table[player_ptr->user_id] = player_ptr->index;
		Q_memset(&(session[player_ptr->index - 1]),0,sizeof(session_t));
		session[player_ptr->index - 1].start_points = rank_ptr->points;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect, remove them from active list
//---------------------------------------------------------------------------------
void ManiStats::ClientDisconnect(player_t	*player_ptr)
{
	rank_t	*player_found = NULL;
	time_t	current_time;
	//WriteDebug("ManiStats::ClientDisconnect() Player disconnected [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);

	if (!active_player_list[player_ptr->index - 1].active) return;

	time(&current_time);
	player_found = active_player_list[player_ptr->index - 1].rank_ptr;
	player_found->total_time_online += (current_time - player_found->last_connected);
	player_found->last_connected = current_time;

	// update total connected time
	active_player_list[player_ptr->index - 1].active = false;
	active_player_list[player_ptr->index - 1].rank_ptr = NULL;

	hash_table[player_ptr->user_id] = -1;
	Q_memset(&(session[player_ptr->index - 1]),0,sizeof(session_t));
}

//---------------------------------------------------------------------------------
// Purpose: Update total connection times if level ends
//---------------------------------------------------------------------------------
void ManiStats::LevelShutdown(void)
{
	rank_t	*player_found = NULL;
	time_t	current_time;

	//WriteDebug("ManiStats::ClientDisconnect() Player disconnected [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);

	// Check if level ended once already
	if (level_ended) return;
	level_ended = true;

	if (mani_stats.GetInt() == 0) return;

	time(&current_time);

	// update total connected time
	for (int i = 0; i < max_players; i++)
	{
		// Ignore non active players
		if (!active_player_list[i].active) continue;
		player_found = active_player_list[i].rank_ptr;
		if (!player_found) continue;

		// Update player connection time
		player_found->total_time_online += (current_time - player_found->last_connected);
		player_found->last_connected = current_time;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free stats lists
//---------------------------------------------------------------------------------
void	ManiStats::FreeStats(void)
{
	this->FreeStats(true);
	this->FreeStats(false);
	this->FreeActiveList();
}

//---------------------------------------------------------------------------------
// Purpose: Free stats lists
//---------------------------------------------------------------------------------
void	ManiStats::FreeActiveList(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		active_player_list[i].rank_ptr = NULL;
		active_player_list[i].last_hit_time = -999.0;
		active_player_list[i].last_user_id = -1;
		active_player_list[i].active = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free stats lists
//---------------------------------------------------------------------------------
void	ManiStats::FreeStats(bool use_steam_id)
{

	if (use_steam_id)
	{
		for (int i = 0; i < rank_player_list_size; i++)
		{
			free (rank_player_list[i]);
		}

		for (int i = 0; i < rank_player_waiting_list_size; i++) 
		{
			free (rank_player_waiting_list[i]);
		}

		FreeList((void **) &rank_player_list, &rank_player_list_size);
		FreeList((void **) &rank_player_waiting_list, &rank_player_waiting_list_size);
		FreeList((void **) &rank_list, &rank_list_size);
	}
	else
	{
		for (int i = 0; i < rank_player_name_list_size; i++) 
		{
			free (rank_player_name_list[i]);
		}

		for (int i = 0; i < rank_player_name_waiting_list_size; i++)
		{
			free (rank_player_name_waiting_list[i]);
		}

		FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);
		FreeList((void **) &rank_player_name_waiting_list, &rank_player_name_waiting_list_size);
		FreeList((void **) &rank_name_list, &rank_name_list_size);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Find rank from database
//---------------------------------------------------------------------------------
void ManiStats::GameFrame(void)
{
	if (war_mode) return;
	if (mani_stats.GetInt() == 0) return;
	if (mani_stats_calculate_frequency.GetInt() == 0 && mani_stats_write_frequency_to_disk.GetInt() == 0) return;

	if (mani_stats_calculate_frequency.GetInt() != 0 && 
		((last_stats_calculate_time + (mani_stats_calculate_frequency.GetInt() * 60)) < g_RealTime))
	{
		time(&last_stats_calculate_time);
		this->CalculateStats(mani_stats_by_steam_id.GetBool(), false);
	}

	if (mani_stats_write_frequency_to_disk.GetInt() != 0 &&
		((last_stats_write_time + (mani_stats_write_frequency_to_disk.GetInt() * 60)) < g_RealTime))
	{
		time(&last_stats_write_time);

		this->ReBuildStatsList(mani_stats_by_steam_id.GetBool());
		this->CalculateStats(mani_stats_by_steam_id.GetBool(), false);
		this->WriteStats(mani_stats_by_steam_id.GetBool());
	}
}

//---------------------------------------------------------------------------------
// Purpose: Find rank from database
//---------------------------------------------------------------------------------
rank_t *ManiStats::FindStoredRank (player_t *player_ptr)
{
	rank_t	player_key;
	rank_t	*player_key_address;
	rank_t	*player_found = NULL;
	rank_t	**player_found_address = NULL;
	rank_t	add_player;
	time_t	current_time;

	player_key_address = &player_key;

	// First we check the main large list in memory
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in player rank list
		Q_strcpy(player_key.steam_id, player_ptr->steam_id);
		player_found_address = (rank_t **) bsearch (&player_key_address, rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id);
	}
	else
	{
		// Do BSearch for name in player rank list
		Q_strcpy(player_key.name, player_ptr->name);
		player_found_address = (rank_t **) bsearch (&player_key_address, rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name);
	}

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		time(&current_time);
		Q_strcpy(player_found->name, player_ptr->name);
		Q_strcpy(player_found->steam_id, player_ptr->steam_id);
		//WriteDebug("ManiStats::FindStoredRank() Player found in core stats [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);
		return player_found;
	}

	// Do BSearch in waiting list
	// this stops duplicates entering the list
	player_key_address = &player_key;
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in player rank list
		Q_strcpy(player_key.steam_id, player_ptr->steam_id);
		player_found_address = (rank_t **) bsearch (&player_key_address, rank_player_waiting_list, rank_player_waiting_list_size, sizeof(rank_t *), sort_by_steam_id);
	}
	else
	{
		// Do BSearch for name in player rank list
		Q_strcpy(player_key.name, player_ptr->name);
		player_found_address = (rank_t **) bsearch (&player_key_address, rank_player_name_waiting_list, rank_player_name_waiting_list_size, sizeof(rank_t *), sort_by_name);
	}

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		time(&current_time);
		Q_strcpy(player_found->name, player_ptr->name);
		Q_strcpy(player_found->steam_id, player_ptr->steam_id);
		//WriteDebug("ManiStats::FindStoredRank() Player found in waiting list [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);
		return player_found;
	}

	// This player simply does not exist yet. We need to add them
	// to the waiting list as we don't want to add them to the main 
	// list and do a qsort until the server is not mid game

	//WriteDebug("ManiStats::FindStoredRank() Adding player to waiting list [%s] [%s]\n" , player_ptr->name, player_ptr->steam_id);

	time(&current_time);
	Q_memset((void *) &add_player, 0, sizeof(rank_t));

	Q_strcpy(add_player.steam_id, player_ptr->steam_id);
	Q_strcpy(add_player.name, player_ptr->name);
	add_player.last_connected = current_time;
	add_player.rank = -1;
	add_player.points = 1000.0;
	add_player.rank_points = 1000.0;
	

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Player not in list so Add
		AddToList((void **) &rank_player_waiting_list, sizeof(rank_t *), &rank_player_waiting_list_size);
		rank_player_waiting_list[rank_player_waiting_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));
		player_found = rank_player_waiting_list[rank_player_waiting_list_size - 1];
		*(rank_player_waiting_list[rank_player_waiting_list_size - 1]) = add_player;
		qsort(rank_player_waiting_list, rank_player_waiting_list_size, sizeof(rank_t *), sort_by_steam_id); 
	}
	else
	{
		// Player not in list so Add
		AddToList((void **) &rank_player_name_waiting_list, sizeof(rank_t *), &rank_player_name_waiting_list_size);
		rank_player_name_waiting_list[rank_player_name_waiting_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));
		player_found = rank_player_name_waiting_list[rank_player_name_waiting_list_size - 1];
		*(rank_player_name_waiting_list[rank_player_name_waiting_list_size - 1]) = add_player;
		qsort(rank_player_name_waiting_list, rank_player_name_waiting_list_size, sizeof(rank_t *), sort_by_name); 
	}

	return player_found;
}

//---------------------------------------------------------------------------------
// Purpose: Calculate Name stats
//---------------------------------------------------------------------------------
void ManiStats::CalculateStats(bool use_steam_id, bool round_end)
{
	// Do BSearch for Name
	time_t	current_time;
	time_t	decay_start;
	time_t	end_decay;

	//WriteDebug("ManiStats::CalculateStats() In function steam mode [%s] round end [%s]\n" , (use_steam_id ? "TRUE":"FALSE"), (round_end ? "TRUE":"FALSE"));
	float	timer;

	timer = gpGlobals->curtime;

	if (use_steam_id)
	{
		//WriteDebug("ManiStats::CalculateStats() %i players in waiting list\n", rank_player_waiting_list_size);
		for (int i = 0; i < rank_player_waiting_list_size; i++)
		{
			AddToList((void **) &rank_player_list, sizeof(rank_t *), &rank_player_list_size);
			rank_player_list[rank_player_list_size - 1] = rank_player_waiting_list[i];
		}

		if (rank_player_waiting_list_size != 0)
		{
			qsort(rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id); 
			FreeList((void **) &rank_player_waiting_list, &rank_player_waiting_list_size);
		}

	}
	else
	{
		//WriteDebug("ManiStats::CalculateStats() %i players in waiting list\n", rank_player_name_waiting_list_size);

		for (int i = 0; i < rank_player_name_waiting_list_size; i++)
		{
			AddToList((void **) &rank_player_name_list, sizeof(rank_t *), &rank_player_name_list_size);
			rank_player_name_list[rank_player_name_list_size - 1] = rank_player_name_waiting_list[i];
		}

		if (rank_player_name_waiting_list_size != 0)
		{
			qsort(rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name); 
			FreeList((void **) &rank_player_name_waiting_list, &rank_player_name_waiting_list_size);
		}
	}

	// If not in per round mode, return
	if (round_end)
	{
		if (mani_stats_mode.GetInt() == 0)
		{
			return;
		}
	}

	if ((use_steam_id && rank_player_list_size == 0) ||
		!use_steam_id && rank_player_name_list_size == 0)
	{
		return;
	}

	time(&current_time);

	// Calculate decay start time ( 3 days )
	decay_start = current_time - (mani_stats_decay_start.GetInt() * 24 * 60 * 60);
	end_decay = decay_start - (mani_stats_decay_period.GetInt() * 24 * 60 * 60);

	float decay_length = (float) (mani_stats_decay_period.GetInt() * 24 * 60 * 60);

	//WriteDebug("ManiStats::CalculateStats() Freeing old ranks\n");

	if (use_steam_id)
	{
		FreeList((void **) &rank_list, &rank_list_size);
	}
	else
	{
		FreeList((void **) &rank_name_list, &rank_name_list_size);
	}


	//WriteDebug("ManiStats::CalculateStats() Building new rank list\n");

	if (use_steam_id)
	{
		for (int i = 0; i < rank_player_list_size; i++)
		{
			rank_player_list[i]->rank = -1;

			if (mani_stats_kills_required.GetInt() > rank_player_list[i]->kills)
			{
				continue;
			}

			if (rank_player_list[i]->last_connected + (mani_stats_ignore_ranks_after_x_days.GetInt() * 86400) < current_time)
			{
				continue;
			}

			rank_player_list[i]->points_decay = 0.0;

			if (rank_player_list[i]->last_connected < decay_start)
			{
				float total_points = rank_player_list[i]->points;

				if (rank_player_list[i]->last_connected < end_decay)
				{
					// Reached end of decay time, set to 500 points by default
					rank_player_list[i]->points_decay = total_points - 500;
				}
				else
				{
					float new_decay_points;

					// Caclulate new decay points based on 10 day window
					new_decay_points = total_points - 500;
					float time_left = (float) (rank_player_list[i]->last_connected - end_decay);

					// Work out percentage to take of score delta
					float time_percent = 1.0 - (time_left / decay_length);
					new_decay_points *= time_percent;
					rank_player_list[i]->points_decay = new_decay_points;
				}
			}

			AddToList((void **) &rank_list, sizeof(rank_t *), &rank_list_size);
			rank_list[rank_list_size - 1] = rank_player_list[i];
		}
	}
	else
	{
		for (int i = 0; i < rank_player_name_list_size; i++)
		{
			rank_player_name_list[i]->rank = -1;

			if (mani_stats_kills_required.GetInt() > rank_player_name_list[i]->kills)
			{
				continue;
			}

			if (rank_player_name_list[i]->last_connected + (mani_stats_ignore_ranks_after_x_days.GetInt() * 86400) < current_time)
			{
				continue;
			}

			// Calculate decay
			rank_player_name_list[i]->points_decay = 0.0;

			if (rank_player_name_list[i]->last_connected < decay_start)
			{
				float total_points = rank_player_name_list[i]->points;

				if (rank_player_name_list[i]->last_connected <= end_decay)
				{
					// Reached end of decay time, set to 500 points by default
					rank_player_name_list[i]->points_decay = total_points - 500;
				}
				else
				{
					float new_decay_points;

					// Caclulate new decay points based on 10 day window
					new_decay_points = total_points - 500;
					float time_left = (float) (rank_player_name_list[i]->last_connected - end_decay);

					// Work out percentage to take of score delta
					float time_percent = 1.0 - (time_left / 864000.0);
					new_decay_points *= time_percent;
					rank_player_name_list[i]->points_decay = new_decay_points;
				}
			}

			AddToList((void **) &rank_name_list, sizeof(rank_t *), &rank_name_list_size);
			rank_name_list[rank_name_list_size - 1] = rank_player_name_list[i];
		}
	}

	//WriteDebug("ManiStats::CalculateStats() Sorting new rank list\n");

	if (use_steam_id)
	{
		if (mani_stats_calculate.GetInt() == 0)
		{
			// By kills
			qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kills); 
		}
		else if (mani_stats_calculate.GetInt() == 1)
		{
			// By kd ratio
			qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kd_ratio); 
		}
		else if (mani_stats_calculate.GetInt() == 2)
		{
			// By Kills - Deaths
			qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kills_deaths); 
		}
		else
		{
			// By Points
			qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_points); 
		}

		// Update player ranks
		for (int i = 0; i < rank_list_size; i++)
		{
			rank_list[i]->rank = i + 1;
			rank_list[i]->rank_points = rank_list[i]->points - rank_list[i]->points_decay;
		}
	}
	else
	{
		if (mani_stats_calculate.GetInt() == 0)
		{
			// By kills
			qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kills); 
		}
		else if (mani_stats_calculate.GetInt() == 1)
		{
			// By kd ratio
			qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kd_ratio); 
		}
		else if (mani_stats_calculate.GetInt() == 2)
		{
			// By Kills - Deaths
			qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kills_deaths); 
		}
		else
		{
			// By Points
			qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_points); 
		}

		// Update player ranks
		for (int i = 0; i < rank_name_list_size; i++)
		{
			rank_name_list[i]->rank = i + 1;
			rank_name_list[i]->rank_points = rank_name_list[i]->points - rank_name_list[i]->points_decay;
		}
	}

	MMsg("Calculate Stats total time [%f]\n", gpGlobals->curtime - timer);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Rebuild the player list
//---------------------------------------------------------------------------------
void ManiStats::ReBuildStatsList(bool use_steam_id)
{
	rank_t	**temp_player_list=NULL;
	int	temp_player_list_size=0;

	time_t	current_time;
	time(&current_time);

	//WriteDebug("ManiStats::ReBuildStatsList() In function steam mode [%s]\n" , (use_steam_id ? "TRUE":"FALSE"));

	// Rebuild list
	if (use_steam_id)
	{
		for (int i = 0; i < rank_player_list_size; i++)
		{
			if (!just_loaded && rank_player_list[i]->last_connected + (mani_stats_drop_player_days.GetInt() * 86400) < current_time)
			{
				// Player outside of time limit so drop them
				free (rank_player_list[i]);
				continue;
			}

			if (FStrEq(rank_player_list[i]->steam_id, "BOT"))
			{
				// Remove bots if they are in there
				free(rank_player_list[i]);
				continue;
			}

			AddToList((void **) &temp_player_list, sizeof(rank_t *), &temp_player_list_size);
			temp_player_list[temp_player_list_size - 1] = rank_player_list[i];
		}

		// Free the old list
		FreeList((void **) &rank_player_list, &rank_player_list_size);

		rank_player_list = temp_player_list;
		rank_player_list_size = temp_player_list_size;
		qsort(rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id);
	}
	else
	{
		for (int i = 0; i < rank_player_name_list_size; i++)
		{
			if (!just_loaded && rank_player_name_list[i]->last_connected + (mani_stats_drop_player_days.GetInt() * 86400) < current_time)
			{
				// Player outside of time limit so drop them
				free (rank_player_name_list[i]);
				continue;
			}

			if (FStrEq(rank_player_name_list[i]->steam_id, "BOT"))
			{
				// Remove bots if they are in there
				free(rank_player_name_list[i]);
				continue;
			}

			AddToList((void **) &temp_player_list, sizeof(rank_t *), &temp_player_list_size);
			temp_player_list[temp_player_list_size - 1] = rank_player_name_list[i];
		}

		// Free the old list
		FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);

		rank_player_name_list = temp_player_list;
		rank_player_name_list_size = temp_player_list_size;
		qsort(rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name);
	}

	//WriteDebug("ManiStats::ReBuildStatsList() End\n" , (use_steam_id ? "TRUE":"FALSE"));

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process a gun fired
//---------------------------------------------------------------------------------
void ManiStats::CSSPlayerFired(int index, bool is_bot)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (is_bot) return;
	if (!active_player_list[index].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	player_found = active_player_list[index].rank_ptr;

	// Increment shots fired
	player_found->user_def[CSS_SHOT_FIRED]++;
	session[index].user_def[CSS_SHOT_FIRED]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a gun fired
//---------------------------------------------------------------------------------
void ManiStats::DODSPlayerFired(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	// Increment shots fired
	player_found->user_def[DODS_SHOT_FIRED]++;
	session[player_ptr->index - 1].user_def[DODS_SHOT_FIRED]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a point captured
//---------------------------------------------------------------------------------
void ManiStats::DODSPointCaptured(const char *cappers, int cappers_length)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	for (int i = 0; i < cappers_length; i ++)
	{
		player_t player;

		player.index = cappers[i];
		if (!FindPlayerByIndex(&player)) continue;
		if (!MoreThanOnePlayer()) return;
		if (player.is_bot) continue;
		if (!active_player_list[player.index - 1].active) continue;

		player_found = active_player_list[player.index - 1].rank_ptr;

		// Increment shots fired
		player_found->user_def[DODS_POINT_CAPTURED]++;
		session[player.index - 1].user_def[DODS_POINT_CAPTURED]++;
		player_found->points += mani_stats_dods_capture_point.GetInt();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process a capture blocked
//---------------------------------------------------------------------------------
void ManiStats::DODSCaptureBlocked(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
//	if (!MoreThanOnePlayer()) return;


	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[DODS_CAPTURE_BLOCKED]++;
	session[player_ptr->index - 1].user_def[DODS_CAPTURE_BLOCKED]++;
	player_found->points += mani_stats_dods_block_capture.GetInt();
}

//---------------------------------------------------------------------------------
// Purpose: Process the round ending
//---------------------------------------------------------------------------------
void ManiStats::CSSRoundEnd(int winning_team, const char *message)
{
	rank_t	*player_found;
	if (mani_stats.GetInt() == 0) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (player.team != 2 && player.team != 3) continue;
		if (!active_player_list[player.index - 1].active) return;

		player_found = active_player_list[player.index - 1].rank_ptr;

		// Okay player is on a valid team for CSS
		if (player.team == 2)
		{
			// Terrorist team
			if (winning_team == 3)
			{
				// CT team won
				player_found->user_def[CSS_LOST_AS_T]++;
				session[player.index - 1].user_def[CSS_LOST_AS_T]++;
			}
			else if (winning_team == 2)
			{
				player_found->user_def[CSS_WON_AS_T]++;
				session[player.index - 1].user_def[CSS_WON_AS_T]++;
			}
		}
		else
		{
			// player is on ct team
			if (winning_team == 3)
			{
				// CT team won
				player_found->user_def[CSS_WON_AS_CT]++;
				session[player.index - 1].user_def[CSS_WON_AS_CT]++;
			}
			else if (winning_team == 2)
			{
				player_found->user_def[CSS_LOST_AS_CT]++;
				session[player.index - 1].user_def[CSS_LOST_AS_CT]++;
			}
		}
	}

	if (FStrEq(message, "#CTs_Win"))
	{
		this->AddTeamPoints(3, mani_stats_css_t_eliminated_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#Terrorists_Win"))
	{
		this->AddTeamPoints(2, mani_stats_css_ct_eliminated_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#VIP_Escaped"))
	{
		this->AddTeamPoints(3, mani_stats_css_ct_vip_escaped_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#VIP_Assassinated"))
	{
		this->AddTeamPoints(2, mani_stats_css_t_vip_assassinated_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#Target_Bombed"))
	{
		this->AddTeamPoints(2, mani_stats_css_t_target_bombed_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#All_Hostages_Rescued"))
	{
		this->AddTeamPoints(3, mani_stats_css_ct_all_hostages_rescued_team_bonus.GetInt());
	}
	else if (FStrEq(message, "#Bomb_Defused"))
	{
		this->AddTeamPoints(3, mani_stats_css_ct_bomb_defused_team_bonus.GetInt());
	}
}
//---------------------------------------------------------------------------------
// Purpose: Process a round end for Dods 
//---------------------------------------------------------------------------------
void ManiStats::AddTeamPoints
(
 int team,  
 int points
)
{
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		rank_t *player_found;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (player.team != team) continue;
		if (!active_player_list[player.index - 1].active) return;

		player_found = active_player_list[player.index - 1].rank_ptr;
		if (!player_found) continue;

		player_found->points += points;
	}
}
//---------------------------------------------------------------------------------
// Purpose: Process a round end for Dods 
//---------------------------------------------------------------------------------
void ManiStats::DODSRoundEnd(int winning_team)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (player.team != 2 && player.team != 3) continue;
		if (!active_player_list[player.index - 1].active) return;

		player_found = active_player_list[player.index - 1].rank_ptr;

		// Okay player is on a valid team for DoD
		if (player.team == 2)
		{
			// Player is on team Allies
			
			if (winning_team == 3)
			{
				// Axis won
				player_found->user_def[DODS_LOST_AS_AXIS]++;
				session[player.index - 1].user_def[DODS_LOST_AS_AXIS]++;
			}
			else if (winning_team == 2)
			{
				player_found->user_def[DODS_WON_AS_ALLIES]++;
				session[player.index - 1].user_def[DODS_WON_AS_ALLIES]++;
				player_found->points += mani_stats_dods_round_win_bonus.GetInt();
			}
		}
		else
		{
			// player is on Axis team
			if (winning_team == 3)
			{
				// AXIS team won
				player_found->user_def[DODS_WON_AS_AXIS]++;
				session[player.index - 1].user_def[DODS_WON_AS_AXIS]++;
				player_found->points += mani_stats_dods_round_win_bonus.GetInt();
			}
			else if (winning_team == 2)
			{
				player_found->user_def[DODS_LOST_AS_ALLIES]++;
				session[player.index - 1].user_def[DODS_LOST_AS_ALLIES]++;
			}
		}
	}

	this->CalculateStats(mani_stats_by_steam_id.GetBool(), true);
}
//---------------------------------------------------------------------------------
// Purpose: Process a bomb plant
//---------------------------------------------------------------------------------
void ManiStats::BombPlanted(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_BOMB_PLANTED]++;
	session[player_ptr->index - 1].user_def[CSS_BOMB_PLANTED]++;
	player_found->points += mani_stats_css_bomb_planted_bonus.GetInt();

	this->AddTeamPoints(2, mani_stats_css_t_bomb_planted_team_bonus.GetInt());
}


//---------------------------------------------------------------------------------
// Purpose: Process a bomb defused
//---------------------------------------------------------------------------------
void ManiStats::BombDefused(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_BOMB_DEFUSED]++;
	session[player_ptr->index - 1].user_def[CSS_BOMB_DEFUSED]++;
	player_found->points += mani_stats_css_bomb_defused_bonus.GetInt();

}

//---------------------------------------------------------------------------------
// Purpose: Process a bomb defusal attempt
//---------------------------------------------------------------------------------
void ManiStats::BombBeginDefuse(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_BOMB_DEFUSE_ATTEMPT]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a hostage being rescued
//---------------------------------------------------------------------------------
void ManiStats::HostageRescued(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_HOSTAGE_RESCUED]++;
	session[player_ptr->index - 1].user_def[CSS_HOSTAGE_RESCUED]++;
	player_found->points += mani_stats_css_hostage_rescued_bonus.GetInt();

	this->AddTeamPoints(3, mani_stats_css_ct_hostage_rescued_team_bonus.GetInt());
}

//---------------------------------------------------------------------------------
// Purpose: Process a hostage following a player
//---------------------------------------------------------------------------------
void ManiStats::HostageFollows(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_HOSTAGE_FOLLOW]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a hostage being killed
//---------------------------------------------------------------------------------
void ManiStats::HostageKilled(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_HOSTAGE_KILLED]++;
	session[player_ptr->index - 1].user_def[CSS_HOSTAGE_KILLED]++;
	player_found->points += mani_stats_css_hostage_killed_bonus.GetInt();

	this->AddTeamPoints(3, mani_stats_css_ct_hostage_killed_team_bonus.GetInt());
}

//---------------------------------------------------------------------------------
// Purpose: Process the bomb exploding
//---------------------------------------------------------------------------------
void ManiStats::BombExploded(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;
	if (player_found == NULL) 
	{
		MMsg("Warning, player [%s] not in rank list\n", player_ptr->steam_id);
		return;
	}

	player_found->user_def[CSS_BOMB_EXPLODED]++;
	session[player_ptr->index - 1].user_def[CSS_BOMB_EXPLODED]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a bomb being dropped
//---------------------------------------------------------------------------------
void ManiStats::BombDropped(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_BOMB_DROPPED]++;
}

//---------------------------------------------------------------------------------
// Purpose: Process a VIP escaping
//---------------------------------------------------------------------------------
void ManiStats::VIPEscaped(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_VIP_ESCAPED]++;
	session[player_ptr->index - 1].user_def[CSS_VIP_ESCAPED]++;
	player_found->points += mani_stats_css_vip_escape_bonus.GetInt();
}

//---------------------------------------------------------------------------------
// Purpose: Process a VIP being killed
//---------------------------------------------------------------------------------
void ManiStats::VIPKilled(player_t *player_ptr)
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!MoreThanOnePlayer()) return;

	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	player_found->user_def[CSS_VIP_KILLED]++;
	session[player_ptr->index - 1].user_def[CSS_VIP_KILLED]++;
	player_found->points += mani_stats_css_vip_killed_bonus.GetInt();
}

//---------------------------------------------------------------------------------
// Purpose: Process a player hurt (shot hit)
//---------------------------------------------------------------------------------
void ManiStats::PlayerHurt
(
 player_t *victim_ptr,  
 player_t *attacker_ptr,
 IGameEvent * event
 )
{
	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (attacker_ptr->user_id <= 0) return;
	if (attacker_ptr->is_bot) return;

	if (!active_player_list[attacker_ptr->index - 1].active) return;

	player_found = active_player_list[attacker_ptr->index - 1].rank_ptr;

	int health_amount;
	
	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		health_amount = event->GetInt("dmg_health", 0);
	}
	else
	{
		health_amount = event->GetInt("damage", 0);
	}

	int hit_group = event->GetInt("hitgroup", 0);

	if (active_player_list[attacker_ptr->index - 1].last_hit_time != gpGlobals->curtime
		|| active_player_list[attacker_ptr->index - 1].last_user_id != victim_ptr->user_id)
	{
		// Update shots fired
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			player_found->user_def[CSS_SHOT_HIT]++;
			session[attacker_ptr->index - 1].user_def[CSS_SHOT_HIT]++;
		}
		else
		{
			player_found->user_def[DODS_SHOT_HIT]++;
			session[attacker_ptr->index - 1].user_def[DODS_SHOT_HIT]++;
		}

		player_found->hit_groups[hit_group] += 1;
	}

	player_found->damage += health_amount;
	session[attacker_ptr->index - 1].damage += health_amount;
	active_player_list[attacker_ptr->index - 1].last_hit_time = gpGlobals->curtime;
}

//---------------------------------------------------------------------------------
// Purpose: Process a player death
//---------------------------------------------------------------------------------
void ManiStats::PlayerDeath
(
 player_t *victim_ptr,  
 player_t *attacker_ptr,  
 char *weapon_name,
 bool attacker_exists, 
 bool headshot
 )
{
	// Do BSearch for steam ID
	rank_t	*a_player_found = NULL;
	rank_t	*v_player_found = NULL;
	bool	team_kill = false;

	if (mani_stats.GetInt() == 0) return;

	if (!attacker_exists) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	if (attacker_ptr->user_id == 0)
	{
		// World attacked
		return;
	}

	// Ignore kills made by bots if set to do so
	if ((victim_ptr->is_bot || attacker_ptr->is_bot) && 
		mani_stats_include_bot_kills.GetInt() == 0)
	{
		return;
	}

	bool suicide = false;
	if (victim_ptr->user_id == attacker_ptr->user_id)
	{
		// Suicide, don't register
		suicide = true;
	}

	if (active_player_list[attacker_ptr->index - 1].active)
	{
		a_player_found = active_player_list[attacker_ptr->index - 1].rank_ptr;
		if (suicide == false)
		{
			if (attacker_ptr->team != victim_ptr->team || !gpManiGameType->IsTeamPlayAllowed())
			{
				// Not a team attack or not in team play mode
				if (headshot)
				{
					session[attacker_ptr->index - 1].headshots++;
					a_player_found->headshots ++;
				}

				a_player_found->kills++;
				session[attacker_ptr->index - 1].kills ++;
				if (a_player_found->deaths == 0)
				{
					// Prevent math error div 0
					a_player_found->kd_ratio = a_player_found->kills;
				}
				else
				{
					a_player_found->kd_ratio = (float) ((float) a_player_found->kills / (float) a_player_found->deaths);
				}
			}
			else
			{
				// This was a tk by the attacker, so bump up their deaths instead
				a_player_found->deaths ++;
				session[victim_ptr->index - 1].deaths ++;
				if (a_player_found->kills == 0)
				{
					// Prevent math error div 0
					a_player_found->kd_ratio = 0;
				}
				else
				{
					a_player_found->kd_ratio = (float) ((float) a_player_found->kills / (float) a_player_found->deaths);
				}

				team_kill = true;
				a_player_found->team_kills ++;
				session[attacker_ptr->index - 1].team_kills ++;
			}
		}

		// Update name
		Q_strcpy(a_player_found->name, attacker_ptr->name);
	}
	
	if (active_player_list[victim_ptr->index - 1].active)
	{
		v_player_found = active_player_list[victim_ptr->index - 1].rank_ptr;

		if (suicide)
		{
			v_player_found->suicides ++;
			session[victim_ptr->index - 1].suicides ++;
		}
		else
		{
			if (attacker_ptr->team != victim_ptr->team || !gpManiGameType->IsTeamPlayAllowed())
			{
				v_player_found->deaths ++;
				session[victim_ptr->index - 1].deaths ++;
				if (v_player_found->kills == 0)
				{
					// Prevent math error div 0
					v_player_found->kd_ratio = 0;
				}
				else
				{
					v_player_found->kd_ratio = (float) ((float) v_player_found->kills / (float) v_player_found->deaths);
				}
			}
		}

		// Update name
		Q_strcpy(v_player_found->name, victim_ptr->name);
	}

	rank_t a_bot;
	rank_t v_bot;

	if (attacker_ptr->is_bot)
	{
		a_bot.kills = 1;
		a_bot.points = 1000;
		a_player_found = &a_bot;
	}

	if (victim_ptr->is_bot)
	{
		v_bot.kills = 1;
		v_bot.points = 1000;
		v_player_found = &v_bot;
	}

	float	weapon_weight = 1.0;

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		int attacker_weapon = weapon_hash_table[this->GetCSSWeaponHashIndex(weapon_name)];
		if (attacker_weapon != -1)
		{
			a_player_found->weapon_kills[attacker_weapon]++;
			weapon_weight = weapon_weights[attacker_weapon]->GetFloat();
		}
	}

	if (v_player_found && a_player_found)
	{
		SetPointsDeltas(a_player_found, v_player_found, team_kill, attacker_ptr->is_bot, victim_ptr->is_bot, attacker_ptr->index - 1, victim_ptr->index - 1, weapon_weight, suicide);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Process a DODS player death
//---------------------------------------------------------------------------------
void ManiStats::DODSPlayerDeath
(
 player_t *victim_ptr,  
 player_t *attacker_ptr,  
 int  weapon_index,
 bool attacker_exists
 )
{
	// Do BSearch for steam ID
	rank_t	*a_player_found = NULL;
	rank_t	*v_player_found = NULL;
	bool	team_kill = false;

	if (mani_stats.GetInt() == 0) return;

	if (!attacker_exists) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	if (weapon_index == -1) return;

	if (attacker_ptr->user_id == 0)
	{
		// World attacked
		return;
	}

	// Ignore kills made by bots if set to do so
	if ((victim_ptr->is_bot || attacker_ptr->is_bot) && 
		mani_stats_include_bot_kills.GetInt() == 0)
	{
		return;
	}

	bool suicide = false;
	if (victim_ptr->user_id == attacker_ptr->user_id)
	{
		// Suicide, don't register
		suicide = true;
	}

	if (active_player_list[attacker_ptr->index - 1].active)
	{
		a_player_found = active_player_list[attacker_ptr->index - 1].rank_ptr;
		if (suicide == false)
		{
			if (attacker_ptr->team != victim_ptr->team || !gpManiGameType->IsTeamPlayAllowed())
			{
				// Not a team attack or not in team play mode
				a_player_found->kills++;
				session[attacker_ptr->index - 1].kills ++;
				if (a_player_found->deaths == 0)
				{
					// Prevent math error div 0
					a_player_found->kd_ratio = a_player_found->kills;
				}
				else
				{
					a_player_found->kd_ratio = (float) ((float) a_player_found->kills / (float) a_player_found->deaths);
				}
			}
			else
			{
				// This was a tk by the attacker, so bump up their deaths instead
				a_player_found->deaths ++;
				session[victim_ptr->index - 1].deaths ++;
				if (a_player_found->kills == 0)
				{
					// Prevent math error div 0
					a_player_found->kd_ratio = 0;
				}
				else
				{
					a_player_found->kd_ratio = (float) ((float) a_player_found->kills / (float) a_player_found->deaths);
				}

				team_kill = true;
				a_player_found->team_kills ++;
				session[attacker_ptr->index - 1].team_kills ++;
			}
		}

		// Update name
		Q_strcpy(a_player_found->name, attacker_ptr->name);
	}
	
	if (active_player_list[victim_ptr->index - 1].active)
	{
		v_player_found = active_player_list[victim_ptr->index - 1].rank_ptr;

		if (suicide)
		{
			v_player_found->suicides ++;
			session[victim_ptr->index - 1].suicides ++;
		}
		else
		{
			if (attacker_ptr->team != victim_ptr->team || !gpManiGameType->IsTeamPlayAllowed())
			{
				v_player_found->deaths ++;
				session[victim_ptr->index - 1].deaths ++;
				if (v_player_found->kills == 0)
				{
					// Prevent math error div 0
					v_player_found->kd_ratio = 0;
				}
				else
				{
					v_player_found->kd_ratio = (float) ((float) v_player_found->kills / (float) v_player_found->deaths);
				}
			}
		}

		// Update name
		Q_strcpy(v_player_found->name, victim_ptr->name);
	}

	rank_t a_bot;
	rank_t v_bot;

	if (attacker_ptr->is_bot)
	{
		a_bot.kills = 1;
		a_bot.points = 1000;
		a_player_found = &a_bot;
	}

	if (victim_ptr->is_bot)
	{
		v_bot.kills = 1;
		v_bot.points = 1000;
		v_player_found = &v_bot;
	}

	float	weapon_weight = 1.0;

	int attacker_weapon = map_dod_weapons[weapon_index];
	if (attacker_weapon != -1)
	{
		a_player_found->weapon_kills[attacker_weapon]++;
		weapon_weight = weapon_weights[attacker_weapon]->GetFloat();
	}

	if (v_player_found && a_player_found)
	{
		SetPointsDeltas(a_player_found, v_player_found, team_kill, attacker_ptr->is_bot, victim_ptr->is_bot, attacker_ptr->index - 1, victim_ptr->index - 1, weapon_weight, suicide);
	}

}
//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
/*
sub calcpoints_default {
  my $self = shift;
  my ($k,$v,$w) = @_;
  my $conf = $self->{ps}{conf};
  my $basepoints = $self->{ps}{conf}{basepoints};

  my $kpoints = (defined $k->{points}) ? $k->{points} : $basepoints;
  my $vpoints = (defined $v->{points}) ? $v->{points} : $basepoints;

  my $diff = $kpoints - $vpoints;							# difference in points
  my $prob = 1 / ( 1 + 10 ** ($diff / $basepoints) );				# find probability of kill
  my $kadj = (($k->{kills} || 1) > 100) ? 15 : 30;				# calculate adjustment
  my $vadj = (($v->{kills} || 1) > 100) ? 15 : 30;

  my $kbonus = $kadj * $prob;
  my $vbonus = $vadj * $prob;

  if ($conf->{use}{weaponweights}) {				# adjust points points based on weapon
#    eval {							# watch out for verbal perl warnings (incase someone fudges a weight value)
      $kbonus = $kbonus * $conf->{weapons}{$w}{weight} if $conf->{weapons}{$w}{weight};
#    };
  }

  return (
	$kpoints + $kbonus, 							# killers new points value
	$vpoints - $vbonus,							# victims ...
	$kbonus,								# total bonus points given to killer
	$vbonus									# ... victim
  );
*/


void ManiStats::SetPointsDeltas
(
 rank_t	*a_player_ptr,
 rank_t	*v_player_ptr,
 bool	team_kill,
 bool	a_is_bot,
 bool	v_is_bot,
 int	a_index,
 int	v_index,
 float	weapon_weight,
 bool	suicide
 )
{
	rank_t	*temp_player_ptr;
	bool	temp_bot;

	if (team_kill)
	{
		// Swap players if team kill
		temp_player_ptr = a_player_ptr;
		a_player_ptr = v_player_ptr;
		v_player_ptr = temp_player_ptr;
		temp_bot = a_is_bot;
		a_is_bot = v_is_bot;
		v_is_bot = temp_bot;
	}

	float a_bonus, v_bonus;

	
	a_bonus = v_bonus = (((float) v_player_ptr->points / (float) a_player_ptr->points) * mani_stats_points_multiplier.GetFloat()) * weapon_weight;

	v_bonus *= mani_stats_points_death_multiplier.GetFloat();

	//// Do points computation ELO
	//float	diff = a_player_ptr->points - v_player_ptr->points;
	//float	prob = 1.0 / (1.0 + pow((float) 10.0,(float) (diff / 1000.0)));
	//float	a_adj = a_player_ptr->kills > 100 ? 15:30;
	//float	v_adj = v_player_ptr->kills > 100 ? 15:30;
	//float	a_bonus = a_adj * prob;
	//float	v_bonus = v_adj * prob;

//	MMsg("diff [%f] prob [%f] a_adj [%f] v_adj [%f] a_bonus [%f] v_bonus [%f]\n", 
//			diff, prob, a_adj, v_adj, a_bonus, v_bonus); 

	// Update player delta tracking

	// Flatten points if too low
	if (!v_is_bot && v_player_ptr->points - v_bonus < 500.0)
	{
		v_bonus = v_player_ptr->points - 500.0;
	}

	// Update players pointss
	if ( !suicide )
		a_player_ptr->points += a_bonus;

	if (mani_stats_points_add_only.GetInt() == 0)
	{
		// Thiz is to stop new players on the stats leeching experienced player points until
		// the new player has a minimum amount of deaths + kills
		if (suicide || team_kill ||
			(!team_kill && ((a_player_ptr->kills + a_player_ptr->deaths) > mani_stats_kills_before_points_removed.GetInt())))
		{
			v_player_ptr->points -= v_bonus;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ManiStats::ShowRank(player_t *player_ptr)
{
	rank_t	*player_found;
	char	output_string[512];

	if (mani_stats.GetInt() == 0) return;
	if (!active_player_list[player_ptr->index - 1].active) return;
	player_found = active_player_list[player_ptr->index - 1].rank_ptr;

	// Found player so update their details
	int	number_of_ranks;
	if (mani_stats_by_steam_id.GetInt() == 1) 
	{
		Q_strcpy(player_found->name, player_ptr->name);
		number_of_ranks = rank_list_size;
	}
	else
	{
		number_of_ranks = rank_name_list_size;
	}

	if (player_found->rank == -1)
	{
		snprintf(output_string, sizeof(output_string), "%s", Translate(player_ptr, 1004, "%s%i%s%i%s%.2f",
															player_ptr->name,
															player_found->kills,
															(player_found->kills == 1) ? Translate(player_ptr, M_STATS_KILL_SINGLE):Translate(player_ptr, M_STATS_KILL_PLURAL),
															player_found->deaths,
															(player_found->deaths == 1) ? Translate(player_ptr, M_STATS_DEATH_SINGLE):Translate(player_ptr, M_STATS_DEATH_PLURAL),
															player_found->kd_ratio));

	}
	else
	{
		if (mani_stats_calculate.GetInt() != 3)
		{
			snprintf(output_string, sizeof(output_string), "%s", Translate(player_ptr, 1005, "%s%i%i%i%s%i%s%.2f",
															player_ptr->name,
															player_found->rank,
															number_of_ranks,
															player_found->kills,
															(player_found->kills == 1) ? Translate(player_ptr, M_STATS_KILL_SINGLE):Translate(player_ptr, M_STATS_KILL_PLURAL),
															player_found->deaths,
															(player_found->deaths == 1) ? Translate(player_ptr, M_STATS_DEATH_SINGLE):Translate(player_ptr, M_STATS_DEATH_PLURAL),
															player_found->kd_ratio));
		}
		else
		{
			snprintf(output_string, sizeof(output_string), "%s", Translate(player_ptr, 1006, "%s%i%i%.0f%i%s%i%s%.2f",
															player_ptr->name,
															player_found->rank,
															number_of_ranks,
															player_found->points,
															player_found->kills,
															(player_found->kills == 1) ? Translate(player_ptr, M_STATS_KILL_SINGLE):Translate(player_ptr, M_STATS_KILL_PLURAL),
															player_found->deaths,
															(player_found->deaths == 1) ? Translate(player_ptr, M_STATS_DEATH_SINGLE):Translate(player_ptr, M_STATS_DEATH_PLURAL),
															player_found->kd_ratio));
		}
	}

	// Joke for ShotKind, he uses 'rank' way too many times
	/*if (strcmp(player_ptr->steam_id, "STEAM_0:0:6004306") == 0)
	{
		snprintf(output_string, sizeof(output_string), "Excessive use of 'rank' use on STEAM_0:0:6004306 plz check website for stats");
	}*/

	if (mani_stats_show_rank_to_all.GetInt() == 1)
	{
		if (player_ptr->is_dead)
		{
			SayToDead(ORANGE_CHAT, "%s", output_string);
		}
		else
		{
			SayToAll(ORANGE_CHAT, false, "%s", output_string);
		}
	}
	else
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "%s", output_string);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ShowTopFreePage::SetBackMore(int list_size)
{
	more = true;
	back = true;

	if (current_rank >= list_size)
	{
		current_rank = list_size - 10;
	}

	if ((list_size - current_rank) <= 10)
	{
		more = false;
	}

	if (current_rank <= 0)
	{
		current_rank = 0;
		back = false;
	}
}

bool ShowTopFreePage::SetStartRank(int start_rank)
{
	int		rp_list_size;

	if (war_mode) return false;
	if (mani_stats.GetInt() == 0) return false;

	current_rank = start_rank - 10;
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		rp_list_size = gpManiStats->rank_list_size;
	}
	else
	{
		rp_list_size = gpManiStats->rank_name_list_size;
	}

	if (rp_list_size == 0) return false;
	this->SetBackMore(rp_list_size);
	return true;
}

bool ShowTopFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	int		rp_list_size;

	if (war_mode) return false;
	if (mani_stats.GetInt() == 0) return false;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		rp_list_size = gpManiStats->rank_list_size;
	}
	else
	{
		rp_list_size = gpManiStats->rank_name_list_size;
	}

	if (rp_list_size == 0) return false;
	if (option == 10) return false;
	if (option == 9 && more) current_rank += 10;
	if (option == 8 && back) current_rank -= 10;

	this->SetBackMore(rp_list_size);
	this->Render(player_ptr);
	return true;
}

bool ShowTopFreePage::Render(player_t *player_ptr)
{
	char	menu_string[2048];
	char	player_string[512];
	char	player_name[32];
	char	rank_output[2048];
	int		total_bytes = 0;

	rank_t	**rp_list;
	int		rp_list_size;

	if (war_mode) return false;
	if (mani_stats.GetInt() == 0) return false;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		if (gpManiStats->rank_list_size == 0) return false;
		rp_list_size = gpManiStats->rank_list_size;
		rp_list = gpManiStats->rank_list;
	}
	else
	{
		if (gpManiStats->rank_name_list_size == 0) return false;
		rp_list_size = gpManiStats->rank_name_list_size;
		rp_list = gpManiStats->rank_name_list;
	}

	int max_ranks = current_rank + 10;
	if (max_ranks >= rp_list_size) max_ranks = rp_list_size;

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		char	title_string[256];
		if (mani_stats_calculate.GetInt() == 0)	total_bytes += snprintf(title_string, sizeof(title_string), "%s", Translate(player_ptr, 1007, "%i%i",	current_rank + 1, max_ranks));
		else if (mani_stats_calculate.GetInt() == 1) total_bytes += snprintf(title_string, sizeof(title_string), "%s", Translate(player_ptr, 1008, "%i%i", current_rank + 1, max_ranks));
		else if (mani_stats_calculate.GetInt() == 2) total_bytes += snprintf(title_string, sizeof(title_string), "%s", Translate(player_ptr, 1009, "%i%i", current_rank + 1, max_ranks));
		else total_bytes += snprintf(title_string, sizeof(title_string), "%s", Translate(player_ptr, 1010, "%i%i", current_rank + 1, max_ranks));
		total_bytes += snprintf(menu_string, sizeof(menu_string), "->1. %s", title_string);
		DrawMenu (player_ptr->index, this->timeout, 0, false, false, true, menu_string, false);
	}
	else
	{
		// Escape style menu has more space so elaborate
		if (mani_stats_calculate.GetInt() == 0)	snprintf(rank_output, sizeof(rank_output), "%s", Translate(player_ptr, 1007, "%i%i", current_rank + 1, max_ranks));
		else if (mani_stats_calculate.GetInt() == 1) snprintf(rank_output, sizeof(rank_output), "%s", Translate(player_ptr, 1008, "%i%i", current_rank + 1, max_ranks));
		else if (mani_stats_calculate.GetInt() == 2) snprintf(rank_output, sizeof(rank_output), "%s", Translate(player_ptr, 1009, "%i%i", current_rank + 1, max_ranks));
		else snprintf(rank_output, sizeof(rank_output), "%s", Translate(player_ptr, 1010, "%i%i", current_rank + 1, max_ranks));
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		for (int i = current_rank; i < max_ranks; i++)
		{
			Q_strncpy(player_name, rp_list[i]->name, sizeof(player_name));

			if (mani_stats_calculate.GetInt() == 0)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), "%-2i %s=%i  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kills,
					rp_list[i]->kd_ratio
					);	
			}
			else if (mani_stats_calculate.GetInt() == 1)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), "%-2i %s=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kd_ratio
					);	
			}
			else if (mani_stats_calculate.GetInt() == 2)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), "%-2i %s=%i  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kills - rp_list[i]->deaths,
					rp_list[i]->kd_ratio
					);	
			}
			else
			{
				total_bytes += snprintf(player_string, sizeof(player_string), "%-2i %s=%.0f  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->rank_points,
					rp_list[i]->kd_ratio
					);	
			}

			// Stop if greater than 490 going to buffer
			if (total_bytes > 490)
			{
				break;
			}

			DrawMenu (player_ptr->index, this->timeout, 7, true, true, true, player_string, false);
		}

		// Show More and Exit
		char	extras[512];

		if (back)
		{
			snprintf(extras, sizeof(extras), Translate(player_ptr, 674));
			DrawMenu (player_ptr->index, this->timeout, 7, true, true, true, extras, false);
		}

		if (more) 
		{
			snprintf(extras, sizeof(extras), Translate(player_ptr, 675));
			DrawMenu (player_ptr->index, this->timeout, 7, true, true, true, extras, false);
		}

		snprintf(extras, sizeof(extras), Translate(player_ptr, 676));
		DrawMenu (player_ptr->index, this->timeout, 7, back, more, true, extras, true);
		return true;
	}
	else
	{
		for (int i = current_rank; i < max_ranks; i++)
		{
			Q_strncpy(player_name, rp_list[i]->name, sizeof(player_name));

			if (mani_stats_calculate.GetInt() == 0)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), " %-2i %s=%i  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kills,
					rp_list[i]->kd_ratio
					);	
			}
			else if (mani_stats_calculate.GetInt() == 1)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), " %-2i %s=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kd_ratio
					);	
			}
			else if (mani_stats_calculate.GetInt() == 2)
			{
				total_bytes += snprintf(player_string, sizeof(player_string), " %-2i %s=%i  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->kills - rp_list[i]->deaths,
					rp_list[i]->kd_ratio
					);	
			}
			else
			{
				total_bytes += snprintf(player_string, sizeof(player_string), " %-2i %s=%.0f  KDR=%.2f\n",
					i + 1,
					player_name,
					rp_list[i]->rank_points,
					rp_list[i]->kd_ratio
					);	
			}

			strcat(rank_output, player_string);
		}


		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", Translate(player_ptr, 1011));
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "Msg", rank_output);
		helpers->CreateMessage( player_ptr->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
		return false;
	}
}

void ShowTopFreePage::Redraw(player_t *player_ptr)
{
	this->Render(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_statsme command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiStats::ProcessMaStatsMe(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (mani_stats.GetInt() == 0) return PLUGIN_CONTINUE;

	// Find target players to lookup.
	if (!FindTargetPlayers(player_ptr, gpCmd->Cmd_Argv(1), NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", gpCmd->Cmd_Argv(1)));
		return PLUGIN_STOP;
	}

	StatsMeFreePage *ptr = new StatsMeFreePage;
	g_menu_mgr.AddFreePage(player_ptr, ptr, 5, MANI_STATS_ME_TIMEOUT);
	if (!ptr->Render(&(target_player_list[0]), player_ptr))
	{
		g_menu_mgr.Kill();
	}


	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Show player their stats
//---------------------------------------------------------------------------------
bool StatsMeFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	return false;
}

bool StatsMeFreePage::Render(player_t *player_ptr, player_t *output_player_ptr)
{
	char	menu_string[2048];
	char	extras[2048];
	char	time_online[128];
	int		total_ranks;
	int		total_bytes = 0;
	int		days;
	int		hours;
	int		minutes;
	int		seconds;
	int		menu_index = 1;
	rank_t	*player_found;
	char	points_string[64];

	if (mani_stats.GetInt() == 0) return false;

	if (!gpManiStats->active_player_list[player_ptr->index - 1].active) return false;
	player_found = gpManiStats->active_player_list[player_ptr->index - 1].rank_ptr;
	
	if ( output_player_ptr )
		this->user_id = output_player_ptr->user_id;
	else
		this->user_id = 0;

	// Found player so update their details
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		Q_strcpy(player_found->name, player_ptr->name);
		total_ranks = gpManiStats->rank_list_size;
	}
	else
	{
		total_ranks = gpManiStats->rank_name_list_size;
	}

	char	headshot_string[128]="";
	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		snprintf(headshot_string, sizeof(headshot_string), "%s", Translate(player_ptr, M_STATS_HEADSHOTS, "%i", player_found->headshots));
	}

	if (player_found->rank == -1)
	{
		total_bytes += snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1014, "%i%s%i%s%i%.2f%i", 
												menu_index ++,
												player_found->name,
												player_found->kills,
												headshot_string,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides));
	}
	else
	{
		snprintf(points_string, sizeof(points_string), "%s", Translate(player_ptr, 1019, "%.0f", player_found->points));

		total_bytes += snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1013, "%i%s%i%i%s%i%s%i%.2f%i", 
												menu_index ++,
												player_found->name,
												player_found->rank,
												total_ranks,
												((mani_stats_calculate.GetInt() == 3) ? points_string:""),
												player_found->kills,
												headshot_string,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides));
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, menu_string, false);
	}


	time_t current_time;

	time(&current_time);

	int	total_time = player_found->total_time_online + (current_time - player_found->last_connected);

	seconds = (int)(total_time % 60);
	minutes = (int)(total_time / 60 % 60);
	hours = (int)(total_time / 3600 % 24);
	days = (int)(total_time / 3600 / 24);

	if (days > 0)
	{
		total_bytes += snprintf (time_online, sizeof (time_online), 
			"%s", Translate(player_ptr, 1015, "%i%i%i%i", 
			days, 
			hours,
			minutes,
			seconds));
	}
	else if (hours > 0)
	{
		total_bytes += snprintf (time_online, sizeof (time_online), 
			"%s", Translate(player_ptr, 1016, "%i%i%i",  
			hours,
			minutes,
			seconds));
	}
	else if (minutes > 0)
	{
		total_bytes += snprintf (time_online, sizeof (time_online), 
			"%s", Translate(player_ptr, 1017, "%i%i",  
			minutes,
			seconds));
	}
	else
	{
		total_bytes += snprintf (time_online, sizeof (time_online), 
			"%s", Translate(player_ptr, 1018, "%i", 
			seconds));
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, time_online, false);
	}

	strcat(menu_string, time_online);

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		float accuracy = 0.0;

		if (player_found->user_def[CSS_SHOT_FIRED] == 0)
		{
			accuracy = 0.0;
		}
		else if (player_found->user_def[CSS_SHOT_HIT] == 0)
		{
			accuracy = 0.0;
		}
		else
		{
			accuracy = ((float) player_found->user_def[CSS_SHOT_HIT] / (float) player_found->user_def[CSS_SHOT_FIRED]) * 100.0;
		}

		int	total_t = player_found->user_def[CSS_WON_AS_T] + player_found->user_def[CSS_LOST_AS_T];
		int total_ct = player_found->user_def[CSS_WON_AS_CT] + player_found->user_def[CSS_LOST_AS_CT];
		float total_t_per = 0;
		float total_ct_per = 0;

		if (player_found->user_def[CSS_WON_AS_T] > 0)
		{
			total_t_per = (float) ((float) player_found->user_def[CSS_WON_AS_T] / (float)total_t) * 100.0;
		}

		if (player_found->user_def[CSS_WON_AS_CT] > 0)
		{
			total_ct_per = (float) ((float) player_found->user_def[CSS_WON_AS_CT] / (float)total_ct) * 100.0;
		}

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1020, "%i%.2f%i%i%i%i%i%i%i%i%i",
										    menu_index,
											accuracy,
											player_found->damage,
											player_found->team_kills,
											menu_index + 1,
											player_found->user_def[CSS_BOMB_PLANTED],
											player_found->user_def[CSS_BOMB_EXPLODED],
											player_found->user_def[CSS_BOMB_DEFUSED],
											menu_index + 2,
											player_found->user_def[CSS_HOSTAGE_RESCUED],
											player_found->user_def[CSS_HOSTAGE_KILLED]));

		menu_index += 3;
		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);

		if (vip_version)
		{
			total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1021, "%i%i%i",
										    menu_index++,
											player_found->user_def[CSS_VIP_ESCAPED],
											player_found->user_def[CSS_VIP_KILLED]));

			if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
			{
				DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
			}

			strcat(menu_string, extras);
		}


		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1022, "%i%i%i%.2f%i%i%.2f%i",
											menu_index ++,
											player_found->user_def[CSS_WON_AS_T], total_t, total_t_per,
											player_found->user_def[CSS_WON_AS_CT], total_ct, total_ct_per,
											total_t + total_ct
											));

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		float accuracy = 0.0;

		if (player_found->user_def[DODS_SHOT_FIRED] == 0)
		{
			accuracy = 0.0;
		}
		else if (player_found->user_def[DODS_SHOT_HIT] == 0)
		{
			accuracy = 0.0;
		}
		else
		{
			accuracy = ((float) player_found->user_def[DODS_SHOT_HIT] / (float) player_found->user_def[DODS_SHOT_FIRED]) * 100.0;
		}

		int	total_ax = player_found->user_def[DODS_WON_AS_AXIS] + player_found->user_def[DODS_LOST_AS_AXIS];
		int total_al = player_found->user_def[DODS_WON_AS_ALLIES] + player_found->user_def[DODS_LOST_AS_ALLIES];
		float total_ax_per = 0;
		float total_al_per = 0;

		if (player_found->user_def[CSS_WON_AS_T] > 0)
		{
			total_ax_per = (float) ((float) player_found->user_def[DODS_WON_AS_AXIS] / (float)total_ax) * 100.0;
		}

		if (player_found->user_def[DODS_WON_AS_ALLIES] > 0)
		{
			total_al_per = (float) ((float) player_found->user_def[DODS_WON_AS_ALLIES] / (float)total_al) * 100.0;
		}

		total_bytes += snprintf(extras, sizeof(extras),"%s",  Translate(player_ptr, 1023, "%i%.2f%i%i%i%i%i",
										    menu_index,
											accuracy,
											player_found->damage,
											player_found->team_kills,
											menu_index + 1,
											player_found->user_def[DODS_POINT_CAPTURED],
											player_found->user_def[DODS_CAPTURE_BLOCKED]));

		menu_index += 2;
		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1024, "%i%i%i%.2f%i%i%.2f%i",
											menu_index ++,
											player_found->user_def[DODS_WON_AS_AXIS], total_ax, total_ax_per,
											player_found->user_def[DODS_WON_AS_ALLIES], total_al, total_al_per,
											total_ax + total_al
											));

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, "", true);
		return true;
	}
	else
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", Translate(player_ptr, 1025));
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "Msg", menu_string);
		helpers->CreateMessage( output_player_ptr->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
		return false;
	}
}

void StatsMeFreePage::Redraw(player_t *player_ptr)
{
	player_t target;
	target.user_id = this->user_id;
	if ( !FindPlayerByUserID(&target)) return;
	this->Render(player_ptr,&target);
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_session command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiStats::ProcessMaSession(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (mani_stats.GetInt() == 0) return PLUGIN_CONTINUE;

	// Find target players to lookup.
	if (!FindTargetPlayers(player_ptr, gpCmd->Cmd_Argv(1), NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", gpCmd->Cmd_Argv(1)));
		return PLUGIN_STOP;
	}

	SessionFreePage *ptr = new SessionFreePage;
	g_menu_mgr.AddFreePage(player_ptr, ptr, 5, MANI_STATS_ME_TIMEOUT);
	if (!ptr->Render(&(target_player_list[0]), player_ptr))
	{
		g_menu_mgr.Kill();
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Show player their session information
//---------------------------------------------------------------------------------
bool SessionFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	return false;
}

bool SessionFreePage::Render(player_t *player_ptr, player_t *output_player_ptr)
{
	char	menu_string[2048];
	char	extras[2048];
	char	time_online[128];
	int		total_bytes = 0;
	int		days;
	int		hours;
	int		minutes;
	int		seconds;
	int		menu_index = 1;
	char	points_string[128];

	int	index = player_ptr->index - 1;

	session_t	*session_ptr = &(gpManiStats->session[index]);

	if (mani_stats.GetInt() == 0) return false;

	if ( output_player_ptr )
		this->user_id = output_player_ptr->user_id;
	else
		this->user_id = 0;

	float	kd_ratio = 0.0;

	if (session_ptr->deaths == 0)
	{
		// Prevent math error div 0
		kd_ratio = session_ptr->kills;
	}
	else
	{
		kd_ratio = (float) ((float) session_ptr->kills / (float) session_ptr->deaths);
	}

	int points_acquired = (int) (gpManiStats->active_player_list[player_ptr->index - 1].rank_ptr->points - session_ptr->start_points);
	char sign;

	if (points_acquired > 0)
	{
		sign = '+';
	}
	else if (points_acquired < 0)
	{
		sign = '-';
	}
	else
	{
		sign = ' ';
	}

	if (points_acquired >= 0)
	{
        snprintf(points_string, sizeof(points_string), "%s", Translate(player_ptr, 1026, "%c%i", sign, points_acquired ));
	}
	else
	{
        snprintf(points_string, sizeof(points_string), "%s", Translate(player_ptr, 1027, "%c%i", sign, points_acquired * -1));
	}

	char	headshot_string[128]="";
	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		snprintf(headshot_string, sizeof(headshot_string), "%s", Translate(player_ptr, M_STATS_HEADSHOTS, "%i", session_ptr->headshots));
	}

	total_bytes += snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1028, "%i%s%s%i%s%i%.2f%i", 
											menu_index ++,
											player_ptr->name,
											((mani_stats_calculate.GetInt() == 3) ? points_string:""),
											session_ptr->kills,
											headshot_string,
											session_ptr->deaths,
											kd_ratio,
											session_ptr->suicides));

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, menu_string, false);
	}


	INetChannelInfo *nci = engine->GetPlayerNetInfo(player_ptr->index);
	if (nci)
	{
		int total_time = nci->GetTimeConnected();

		seconds = (int)(total_time % 60);
		minutes = (int)(total_time / 60 % 60);
		hours = (int)(total_time / 3600 % 24);
		days = (int)(total_time / 3600 / 24);

		if (days > 0)
		{
			total_bytes += snprintf (time_online, sizeof (time_online), "%s", Translate(player_ptr, 1043, "%i%i%i%i",
				days, 
				hours,
				minutes,
				seconds));
		}
		else if (hours > 0)
		{
			total_bytes += snprintf (time_online, sizeof (time_online), "%s", Translate(player_ptr, 1044, "%i%i%i", 
				hours,
				minutes,
				seconds));
		}
		else if (minutes > 0)
		{
			total_bytes += snprintf (time_online, sizeof (time_online), "%s", Translate(player_ptr, 1045, "%i%i", 
				minutes,
				seconds));
		}
		else
		{
			total_bytes += snprintf (time_online, sizeof (time_online), "%s", Translate(player_ptr, 1046, "%i", 
				seconds));
		}

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, time_online, false);
		}

		strcat(menu_string, time_online);
	}

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		float accuracy = 0.0;

		if (session_ptr->user_def[CSS_SHOT_FIRED] == 0)
		{
			accuracy = 0.0;
		}
		else if (session_ptr->user_def[CSS_SHOT_HIT] == 0)
		{
			accuracy = 0.0;
		}
		else
		{
			accuracy = ((float) session_ptr->user_def[CSS_SHOT_HIT] / (float) session_ptr->user_def[CSS_SHOT_FIRED]) * 100.0;
		}

		int	total_t = session_ptr->user_def[CSS_WON_AS_T] + session_ptr->user_def[CSS_LOST_AS_T];
		int total_ct = session_ptr->user_def[CSS_WON_AS_CT] + session_ptr->user_def[CSS_LOST_AS_CT];
		float total_t_per = 0;
		float total_ct_per = 0;

		if (session_ptr->user_def[CSS_WON_AS_T] > 0)
		{
			total_t_per = (float) ((float) session_ptr->user_def[CSS_WON_AS_T] / (float)total_t) * 100.0;
		}

		if (session_ptr->user_def[CSS_WON_AS_CT] > 0)
		{
			total_ct_per = (float) ((float) session_ptr->user_def[CSS_WON_AS_CT] / (float)total_ct) * 100.0;
		}

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1029, "%i%.2f%i%i%i%i%i%i%i%i%i",
										    menu_index,
											accuracy,
											session_ptr->damage,
											session_ptr->team_kills,
											menu_index + 1,
											session_ptr->user_def[CSS_BOMB_PLANTED],
											session_ptr->user_def[CSS_BOMB_EXPLODED],
											session_ptr->user_def[CSS_BOMB_DEFUSED],
											menu_index + 2,
											session_ptr->user_def[CSS_HOSTAGE_RESCUED],
											session_ptr->user_def[CSS_HOSTAGE_KILLED]));

		menu_index += 3;
		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);

		if (vip_version)
		{
			total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1030, "%i%i%i",
										    menu_index++,
											session_ptr->user_def[CSS_VIP_ESCAPED],
											session_ptr->user_def[CSS_VIP_KILLED]));

			if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
			{
				DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
			}

			strcat(menu_string, extras);
		}


		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1031, "%i%i%i%.2f%i%i%.2f%i",
											menu_index ++,
											session_ptr->user_def[CSS_WON_AS_T], total_t, total_t_per,
											session_ptr->user_def[CSS_WON_AS_CT], total_ct, total_ct_per,
											total_t + total_ct
											));

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		float accuracy = 0.0;

		if (session_ptr->user_def[DODS_SHOT_FIRED] == 0)
		{
			accuracy = 0.0;
		}
		else if (session_ptr->user_def[DODS_SHOT_HIT] == 0)
		{
			accuracy = 0.0;
		}
		else
		{
			accuracy = ((float) session_ptr->user_def[DODS_SHOT_HIT] / (float) session_ptr->user_def[DODS_SHOT_FIRED]) * 100.0;
		}

		int	total_ax = session_ptr->user_def[DODS_WON_AS_AXIS] + session_ptr->user_def[DODS_LOST_AS_AXIS];
		int total_al = session_ptr->user_def[DODS_WON_AS_ALLIES] + session_ptr->user_def[DODS_LOST_AS_ALLIES];
		float total_ax_per = 0;
		float total_al_per = 0;

		if (session_ptr->user_def[CSS_WON_AS_T] > 0)
		{
			total_ax_per = (float) ((float) session_ptr->user_def[DODS_WON_AS_AXIS] / (float)total_ax) * 100.0;
		}

		if (session_ptr->user_def[DODS_WON_AS_ALLIES] > 0)
		{
			total_al_per = (float) ((float) session_ptr->user_def[DODS_WON_AS_ALLIES] / (float)total_al) * 100.0;
		}

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1032, "%i%.2f%i%i%i%i%i",
										    menu_index,
											accuracy,
											session_ptr->damage,
											session_ptr->team_kills,
											menu_index + 1,
											session_ptr->user_def[DODS_POINT_CAPTURED],
											session_ptr->user_def[DODS_CAPTURE_BLOCKED]));

		menu_index += 2;
		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1033, "%i%i%i%.2f%i%i%.2f%i",
											menu_index ++,
											session_ptr->user_def[DODS_WON_AS_AXIS], total_ax, total_ax_per,
											session_ptr->user_def[DODS_WON_AS_ALLIES], total_al, total_al_per,
											total_ax + total_al
											));

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (output_player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, "", true);
		return true;
	}
	else
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", Translate(player_ptr, 1034));
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "Msg", menu_string);
		helpers->CreateMessage( output_player_ptr->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
		return false;
	}
}

void SessionFreePage::Redraw(player_t *player_ptr)
{
	player_t target;
	target.user_id = this->user_id;
	if ( !FindPlayerByUserID(&target)) return;
	this->Render(player_ptr, &target);
}

//---------------------------------------------------------------------------------
// Purpose: Show hit box information
//---------------------------------------------------------------------------------
bool HitBoxMeFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	return false;
}

bool HitBoxMeFreePage::Render(player_t *player_ptr)
{
	char	menu_string[2048];
	char	extras[2048];
	int		total_bytes = 0;
	int		total_hits = 0;
	int		menu_index = 1;

	float	percent_array[MANI_MAX_STATS_HITGROUPS];

	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return false;
	if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
		gpManiGameType->IsGameType(MANI_GAME_DOD) ||
		gpManiGameType->IsGameType(MANI_GAME_CSGO))) return false;

	if (!gpManiStats->active_player_list[player_ptr->index - 1].active) return false;
	player_found = gpManiStats->active_player_list[player_ptr->index - 1].rank_ptr;

	// Found player so update their details
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		Q_strcpy(player_found->name, player_ptr->name);
	}

	total_bytes += snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1035, "%i%s",
								menu_index++,
								player_found->name));

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, menu_string, false);
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS) ||
		gpManiGameType->IsGameType(MANI_GAME_DOD) ||
		gpManiGameType->IsGameType(MANI_GAME_CSGO))
	{
		float accuracy = 0.0;

		if (player_found->user_def[CSS_SHOT_FIRED] == 0)
		{
			accuracy = 0.0;
		}
		else if (player_found->user_def[CSS_SHOT_HIT] == 0)
		{
			accuracy = 0.0;
		}
		else
		{
			accuracy = ((float) player_found->user_def[CSS_SHOT_HIT] / (float) player_found->user_def[CSS_SHOT_FIRED]) * 100.0;
		}

		total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1036, "%.2f%i",
											accuracy,
											player_found->damage
											));

		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}


	total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1037, "%i",
											menu_index++
											));

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
	}

	strcat(menu_string, extras);

	for (int i = 0; i < MANI_MAX_STATS_HITGROUPS; i++)
	{
		total_hits += player_found->hit_groups[i];
		percent_array[i] = 0;
	}

	// Calculate percentages
	for (int i = 0; i < MANI_MAX_STATS_HITGROUPS; i++)
	{
		if (total_hits == 0)
		{
			percent_array[i] = 0;
			continue;
		}

		if (player_found->hit_groups[i] == 0)
		{
			continue;
		}

		percent_array[i] = (float) ((float) player_found->hit_groups[i] / (float) total_hits) * 100.0;
	}

	for (int i = 0; i < MANI_MAX_STATS_HITGROUPS; i ++)
	{
		total_bytes += snprintf(extras, sizeof(extras), 
							"  %s : %.2f%%\n"
							"  %s\n",
							Translate(player_ptr, hitboxes_gui_text[i]),
							percent_array[i],
							gpManiStats->GetBar(percent_array[i]));
										
		if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
		{
			DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
		}

		strcat(menu_string, extras);
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, "", true);
		return true;
	}
	else
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", Translate(player_ptr, 1038));
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "Msg", menu_string);
		helpers->CreateMessage( player_ptr->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
		return false;
	}
}

void HitBoxMeFreePage::Redraw(player_t *player_ptr)
{
	this->Render(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Weapon usage info
//---------------------------------------------------------------------------------
bool WeaponMeFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	if (option == 9) return this->Render(player_ptr);
	return false;
}

bool WeaponMeFreePage::Render(player_t *player_ptr)
{
	char	menu_string[2048];
	char	extras[2048];
	int		total_bytes = 0;
	int		total_kills = 0;
	int		menu_index = 1;

	weaponme_t *weapon_list = NULL;
	int		   weapon_list_size = 0;

	rank_t	*player_found;

	if (mani_stats.GetInt() == 0) return false;
	if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) || gpManiGameType->IsGameType(MANI_GAME_DOD) || gpManiGameType->IsGameType(MANI_GAME_CSGO))) return false;
	if (!gpManiGameType->IsAMXMenuAllowed()) return false;

	if (!gpManiStats->active_player_list[player_ptr->index - 1].active) return false;
	player_found = gpManiStats->active_player_list[player_ptr->index - 1].rank_ptr;

	int max_weapons;

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		max_weapons = MANI_MAX_STATS_CSS_WEAPONS;
	}
	else
	{
		max_weapons = MANI_MAX_STATS_DODS_WEAPONS;
	}

	// Make a big list for our weapons
	CreateList ((void **) &weapon_list, sizeof(weaponme_t), max_weapons, &weapon_list_size);

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		for (int i = 0; i < max_weapons; i ++)
		{
			// Copy nice weapon names for display
			Q_strcpy(weapon_list[i].weapon_name, css_weapons_nice[i]);
			weapon_list[i].kills = player_found->weapon_kills[i];
			total_kills += weapon_list[i].kills;
			weapon_list[i].percent = 0.0;
		}
	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		for (int i = 0; i < max_weapons; i ++)
		{
			Q_strcpy(weapon_list[i].weapon_name, dod_weapons_nice[i]);
			weapon_list[i].kills = player_found->weapon_kills[i];
			total_kills += weapon_list[i].kills;
			weapon_list[i].percent = 0.0;
		}
	}

	// Sort list into weapon usage order
	qsort(weapon_list, weapon_list_size, sizeof(weaponme_t), sort_by_kills_weapon);

	if (total_kills != 0)
	{
		for (int i = 0; i < max_weapons; i++)
		{
			if (weapon_list[i].kills != 0)
			{
				weapon_list[i].percent = (float) ((float) weapon_list[i].kills / (float) total_kills) * 100.0;
			}
		}
	}


	// Found player so update their details
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		Q_strcpy(player_found->name, player_ptr->name);
	}

	total_bytes += snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1039, "%i%s%i",
								menu_index++,
								player_found->name,
								page));

	DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, menu_string, false);

	float accuracy = 0.0;

	if (player_found->user_def[CSS_SHOT_FIRED] == 0)
	{
		accuracy = 0.0;
	}
	else if (player_found->user_def[CSS_SHOT_HIT] == 0)
	{
		accuracy = 0.0;
	}
	else
	{
		accuracy = ((float) player_found->user_def[CSS_SHOT_HIT] / (float) player_found->user_def[CSS_SHOT_FIRED]) * 100.0;
	}

	total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1040, "%.2f%i",
										accuracy,
										player_found->damage
										));

	DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);

	total_bytes += snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1041, "%i", 
											menu_index++
											));

	DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);

	strcat(menu_string, extras);

	if (page < 1) page = 1;
	if (page > 3) page = 3;

	int start_index = (page - 1) * 10;
	int end_index = page * 10;

	if (end_index > max_weapons) 
	{
		end_index = max_weapons;
	}

	for (int i = start_index; i < end_index; i ++)
	{
		total_bytes += snprintf(extras, sizeof(extras), 
							"  %s : %i (%.2f%%)\n  %s\n",
							weapon_list[i].weapon_name,
							weapon_list[i].kills,
							weapon_list[i].percent,
							gpManiStats->GetBar(weapon_list[i].percent));
										
		DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);
	}

	snprintf(extras, sizeof(extras), "%s", Translate(player_ptr, 1042, "%i", (page == 3) ? 1:page + 1));
	DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, extras, false);

	DrawMenu (player_ptr->index, MANI_STATS_ME_TIMEOUT, 7, true, true, true, "", true);

	page ++;
	if (page == 4) page = 1;
	return true;
}

void WeaponMeFreePage::Redraw(player_t *player_ptr)
{
	this->Render(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Draw a bar type display
//---------------------------------------------------------------------------------
char	*ManiStats::GetBar(float percent)
{
	static char bar_string[100];
	int i_percent;

	// 0xC2,0xA6
	char bar_char[3];

	snprintf (bar_char, sizeof(bar_char), "%c%c", 0xd7,0x80);

	Q_strcpy(bar_string,"");
	i_percent = (int) (percent / 2);
	
	for (int i = 0; i < i_percent; i++)
	{
		strcat(bar_string, bar_char);
	}

	return bar_string;
}

//---------------------------------------------------------------------------------
// Purpose: Reset the stats
//---------------------------------------------------------------------------------
void ManiStats::ResetStats(void)
{

	//WriteDebug("ManiStats::ResetStats()\n");

	char	base_filename[512];
	if (mani_stats.GetInt() == 0) return;

	// Delete the stats
	this->FreeStats();

	//Delete stats file
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/data/mani_stats.txt", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		MMsg("File mani_stats.dat exists, preparing to delete stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			MMsg("Failed to delete mani_stats.dat\n");
		}
	}

	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/data/mani_name_stats.txt", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		MMsg("File mani_name_stats.dat exists, preparing to delete stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			MMsg("Failed to delete mani_name_stats.dat\n");
		}
	}

	player_t	player;

	// Reload players that are on server
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

		// Don't add player if steam id is not confirmed
		if (mani_stats_by_steam_id.GetInt() == 1 && 
			FStrEq(player.steam_id, "STEAM_ID_PENDING"))
		{
			continue;
		}

		this->NetworkIDValidated(&player);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write the stats
//---------------------------------------------------------------------------------
void ManiStats::WriteStats(bool use_steam_id)
{
	char	core_filename[256];
	int		f_index = 0;
	ManiKeyValues *kv;

	rank_t	**rp_list;
	int		rp_list_size;

	//WriteDebug("ManiStats::WriteStats() In function steam mode [%s]\n" , (use_steam_id ? "TRUE":"FALSE"));

	if (use_steam_id)
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_stats.txt", mani_path.GetString());
		kv = new ManiKeyValues( "mani_stats.txt" );
		rp_list = rank_player_list;
		rp_list_size = rank_player_list_size;
	}
	else
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_name_stats.txt", mani_path.GetString());
		kv = new ManiKeyValues( "mani_name_stats.txt" );
		rp_list = rank_player_name_list;
		rp_list_size = rank_player_name_list_size;
	}

	kv->SetIndent(0);
	if (!kv->WriteStart(core_filename))
	{
		Msg("Failed to write to %s\n", core_filename);
		kv->DeleteThis();
		return;
	}

	kv->WriteKey("version", PLUGIN_VERSION_ID2);

	// Loop through all clients
	for (int i = 0; i < rp_list_size; i++)
	{
		kv->WriteNewSubKey(i + 1);
		kv->WriteKey("na", rp_list[i]->name);
		kv->WriteKey("st", rp_list[i]->steam_id);
		kv->WriteKey("ip1", rp_list[i]->ip_address[0]);
		kv->WriteKey("ip2", rp_list[i]->ip_address[1]);
		kv->WriteKey("ip3", rp_list[i]->ip_address[2]);
		kv->WriteKey("ip4", rp_list[i]->ip_address[3]);
		kv->WriteKey("lc", (int) rp_list[i]->last_connected);
		kv->WriteKey("rk", rp_list[i]->rank);
		kv->WriteKey("de", rp_list[i]->deaths);
		kv->WriteKey("hs", rp_list[i]->headshots);
		kv->WriteKey("kd", rp_list[i]->kd_ratio);
		kv->WriteKey("ki", rp_list[i]->kills);
		kv->WriteKey("su", rp_list[i]->suicides); 
		kv->WriteKey("po", rp_list[i]->points);
		kv->WriteKey("pd", rp_list[i]->points_decay);
		kv->WriteKey("tk", rp_list[i]->team_kills);
		kv->WriteKey("to", rp_list[i]->total_time_online);
		kv->WriteKey("da", rp_list[i]->damage);

		if (gpManiGameType->IsGameType(MANI_GAME_CSS) ||
		   gpManiGameType->IsGameType(MANI_GAME_DOD) ||
			gpManiGameType->IsGameType(MANI_GAME_CSGO))
		{
			for (int j = 0; j < MANI_MAX_STATS_HITGROUPS; j++)
			{
				if (rp_list[i]->hit_groups[j] != 0) kv->WriteKey(hitboxes[j], rp_list[i]->hit_groups[j]);
			}
		}

		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			for (int j = 0; j < MANI_MAX_STATS_CSS_WEAPONS; j++)
			{
				if (rp_list[i]->weapon_kills[j] != 0) kv->WriteKey(weapon_short_name[j], rp_list[i]->weapon_kills[j]);
			}
		}
		else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
		{
			for (int j = 0; j < MANI_MAX_STATS_DODS_WEAPONS; j++)
			{
				if (rp_list[i]->weapon_kills[j] != 0) kv->WriteKey(weapon_short_name[j], rp_list[i]->weapon_kills[j]);
			}
		}

		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			// CSS specific stuff
			/*
			SetInt("a", 0); // shots fired 0
			SetInt("b", 0); // shots hit 1
			SetInt("c", 0); // bombs planted 2
			SetInt("d", 0); // bombs defused 3
			SetInt("e", 0); // hostages rescued 4
			SetInt("f", 0); // hostages touched 5
			SetInt("g", 0); // hostages killed 6
			SetInt("h", 0); // bombs exploded 7
			SetInt("i", 0); // bombs dropped 8
			SetInt("j", 0); // bomb defusal attempts 9
			SetInt("k", 0); // VIP escaped
			SetInt("l", 0); // VIP killed
			*/

			for (int j = 0; j < MANI_MAX_USER_DEF; j++)
			{
				if (rp_list[i]->user_def[j] != 0) 
				{
					kv->WriteKey(stats_user_def_name[j], rp_list[i]->user_def[j]);
				}
			}
		}
		else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
		{
			// Dod specific stuff
			/* a = shots fired
			   b = shots hit
			   c = point captured
			   d = points blocked
			   e = won as axis
			   f = lost as axis
			   g = won as axis
			   h = lost as axis
			   */
			for (int j = 0; j < MANI_MAX_DODS_USER_DEF; j++)
			{
				if (rp_list[i]->user_def[j] != 0) 
				{
					kv->WriteKey(stats_user_def_name[j], rp_list[i]->user_def[j]);
				}
			}
		}

		kv->WriteEndSubKey();
	}

	kv->WriteEnd();
	kv->DeleteThis();

	if (mani_stats_write_text_file.GetInt() == 0)
	{
		return;
	}

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_ranks_readme.txt", mani_path.GetString());

	char	temp_string[2048];
	char	temp_string2[2048];
	int		temp_length;

	FileHandle_t file_handle;

	if (filesystem->FileExists( core_filename))
	{
		filesystem->RemoveFile(core_filename);
	}

	file_handle = filesystem->Open(core_filename,"wb",NULL);
	if (file_handle == NULL)
	{
		return;
	}

	// Write CSV format specifier documentation file
	temp_length = snprintf(temp_string, sizeof(temp_string), "Mani Admin Plugin 2006 www.mani-admin-plugin.com\n"
															   "%s\n", PLUGIN_VERSION_ID);
	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		filesystem->Close(file_handle);
		return;
	}

	temp_length = snprintf(temp_string, sizeof(temp_string), "%s\n", gpManiGameType->GetGameType());
	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		filesystem->Close(file_handle);
		return;
	}

	temp_length = snprintf(temp_string, sizeof(temp_string), "CSV seperated rank list format description\n\n"
																"%02i - steam_id,\n"
																"%02i - ip_address,\n"
																"%02i - last_connected (in seconds since Jan 1st 1970),\n"
																"%02i - rank,\n"
																"%02i - points,\n"
																"%02i - deaths,\n"
																"%02i - headshots,\n"
																"%02i - kills,\n"
																"%02i - suicides,\n"
																"%02i - team_kills,\n"
																"%02i - total_time_online (in seconds),\n"
																"%02i - damage,\n",
																f_index,
																f_index + 1,
																f_index + 2,
																f_index + 3,
																f_index + 4,
																f_index + 5,
																f_index + 6,
																f_index + 7,
																f_index + 8,
																f_index + 9,
																f_index + 10,
																f_index + 11
																);

	f_index += 12;

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		MMsg("Failed to write to %s\n", core_filename);
		filesystem->Close(file_handle);
		return;
	} 

	if (gpManiGameType->IsGameType(MANI_GAME_CSS) ||
		gpManiGameType->IsGameType(MANI_GAME_DOD) ||
		gpManiGameType->IsGameType(MANI_GAME_CSGO))
	{
		for (int i = 0; i < MANI_MAX_STATS_HITGROUPS; i++)
		{
			temp_length = snprintf(temp_string, sizeof(temp_string), "%02i - %s,\n", f_index++, hitboxes_text[i]);
			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				MMsg("Failed to write core stats to %s\n", core_filename);
				filesystem->Close(file_handle);
				return;
			} 
		}
	}

	bool	extra_fields = false;
	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		extra_fields = true;
		temp_length = 0;

		Q_strcpy(temp_string,"");
		for (int i = 0; i < MANI_MAX_STATS_CSS_WEAPONS; i++)
		{
			temp_length += snprintf(temp_string2, sizeof(temp_string2), "%02i - %s kills,\n", f_index++, css_weapons[i]);
			strcat(temp_string, temp_string2);
		}

		temp_length += snprintf(temp_string2, sizeof(temp_string2),
												"%02i - shots_fired,\n"
												"%02i - shots_hit,\n"
												"%02i - bombs_planted,\n"
												"%02i - bombs_defused,\n"
												"%02i - hostages_rescued,\n"
												"%02i - hostages_touched,\n"
												"%02i - hostages_killed,\n"
												"%02i - bombs_exploded,\n"
												"%02i - bombs_dropped,\n"
												"%02i - bomb_defusals_attempted,\n"
												"%02i - vip_escaped,\n"
												"%02i - vip_killed,\n"
												"%02i - won_as_ct,\n"
												"%02i - lost_as_ct,\n"
												"%02i - won_as_t,\n"
												"%02i - lost_as_t,\n",
												f_index,
												f_index + 1,
												f_index + 2,
												f_index + 3,
												f_index + 4,
												f_index + 5,
												f_index + 6,
												f_index + 7,
												f_index + 8,
												f_index + 9,
												f_index + 10,
												f_index + 11,
												f_index + 12,
												f_index + 13,
												f_index + 14,
												f_index + 15
												);
		f_index += 16;
		strcat(temp_string, temp_string2);

	}
	else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
	{
		extra_fields = true;
		temp_length = 0;

		Q_strcpy(temp_string,"");
		for (int i = 0; i < MANI_MAX_STATS_DODS_WEAPONS; i++)
		{
			temp_length += snprintf(temp_string2, sizeof(temp_string2), "%02i - %s kills,\n", f_index++, dod_weapons_log[i]);
			strcat(temp_string, temp_string2);
		}

		temp_length += snprintf(temp_string2, sizeof(temp_string2),
												"%02i - shots_fired,\n"
												"%02i - shots_hit,\n"
												"%02i - points_captured,\n"
												"%02i - blocked_points,\n"
												"%02i - won_as_axis,\n"
												"%02i - lost_as_axis,\n"
												"%02i - won_as_allies,\n"
												"%02i - lost_as_allies,\n",
												f_index,
												f_index + 1,
												f_index + 2,
												f_index + 3,
												f_index + 4,
												f_index + 5,
												f_index + 6,
												f_index + 7
												);
		f_index += 8;
		strcat(temp_string, temp_string2);
	}

	if (extra_fields)
	{
		if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
		{
			MMsg("Failed to write core stats to %s\n", core_filename);
			filesystem->Close(file_handle);
			return;
		} 
	}

	temp_length = snprintf(temp_string, sizeof(temp_string), "%02i - player name (always last)\n", f_index++);

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		MMsg("Failed to write core stats to %s\n", core_filename);
		filesystem->Close(file_handle);
		return;
	} 

	temp_length = snprintf(temp_string, sizeof(temp_string), "Total of %i fields\n", f_index);

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		MMsg("Failed to write core stats to %s\n", core_filename);
		filesystem->Close(file_handle);
		return;
	} 

	filesystem->Close(file_handle);


	if (use_steam_id)
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_ranks.txt", mani_path.GetString());
		rp_list = rank_list;
		rp_list_size = rank_list_size;
	}
	else
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_name_ranks.txt", mani_path.GetString());
		rp_list = rank_name_list;
		rp_list_size = rank_name_list_size;
	}

	if (filesystem->FileExists( core_filename))
	{
		filesystem->RemoveFile(core_filename);
	}

	file_handle = filesystem->Open(core_filename,"wb",NULL);
	if (file_handle == NULL)
	{
		return;
	}

	// Loop through all ranks and dump them in CSV format
	for (int i = 0; i < rp_list_size; i ++)
	{
		int	total_length;

		total_length = snprintf(temp_string, sizeof(temp_string), "%s,%i.%i.%i.%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",
			rp_list[i]->steam_id,
			(int) rp_list[i]->ip_address[0],
			(int) rp_list[i]->ip_address[1],
			(int) rp_list[i]->ip_address[2],
			(int) rp_list[i]->ip_address[3],
			(int) rp_list[i]->last_connected,
			rp_list[i]->rank,
			(int) (rp_list[i]->points - rp_list[i]->points_decay),
			rp_list[i]->deaths,
			rp_list[i]->headshots,
			rp_list[i]->kills,
			rp_list[i]->suicides,
			rp_list[i]->team_kills,
			rp_list[i]->total_time_online,
			rp_list[i]->damage);

		if (gpManiGameType->IsGameType(MANI_GAME_CSS) || 
			gpManiGameType->IsGameType(MANI_GAME_DOD) ||
		gpManiGameType->IsGameType(MANI_GAME_CSGO))
		{
			// Do hit groups
			for (int j = 0; j < MANI_MAX_STATS_HITGROUPS; j++)
			{
				total_length += snprintf(temp_string2, sizeof(temp_string2), ",%i", rp_list[i]->hit_groups[j]);
				strcat(temp_string, temp_string2);
			}
		}

		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			// Handle weapon stats
			for (int j = 0; j < MANI_MAX_STATS_CSS_WEAPONS; j++)
			{
				total_length += snprintf(temp_string2, sizeof(temp_string2), ",%i", rp_list[i]->weapon_kills[j]);
				strcat(temp_string, temp_string2);
			}

			// Handle User Def stuff
			for (int j = 0; j < MANI_MAX_USER_DEF; j++)
			{
				total_length += snprintf(temp_string2, sizeof(temp_string2), ",%i", rp_list[i]->user_def[j]);
				strcat(temp_string, temp_string2);
			}
		}
		else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
		{
			// Handle weapon stats
			for (int j = 0; j < MANI_MAX_STATS_DODS_WEAPONS; j++)
			{
				total_length += snprintf(temp_string2, sizeof(temp_string2), ",%i", rp_list[i]->weapon_kills[j]);
				strcat(temp_string, temp_string2);
			}

			// Handle User Def stuff
			for (int j = 0; j < MANI_MAX_DODS_USER_DEF; j++)
			{
				total_length += snprintf(temp_string2, sizeof(temp_string2), ",%i", rp_list[i]->user_def[j]);
				strcat(temp_string, temp_string2);
			}
		}

		total_length += snprintf(temp_string2, sizeof(temp_string2), ",%s\n", rp_list[i]->name);
		strcat(temp_string, temp_string2);

		if (filesystem->Write((void *) temp_string, total_length, file_handle) == 0)
		{
			MMsg("Failed to write core stats to %s\n", core_filename);
			filesystem->Close(file_handle);
			return;
		} 
	}

	filesystem->Close(file_handle);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Simple fast check to see if more than one player on server
//---------------------------------------------------------------------------------
bool ManiStats::MoreThanOnePlayer(void)
{
	player_t player;
	static	int players_needed;

	players_needed = mani_stats_players_needed.GetInt();

	if (gpManiGameType->IsTeamPlayAllowed())
	{
		// Need one player on either team
		int	a_count = 0;
		int b_count = 0;

		for (int i = 1; i <= max_players; i ++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;

			if (player.team == 2)
			{
				a_count ++;
			}
			else if (player.team == 3)
			{
				b_count ++;
			}
			
			if (a_count >= players_needed && b_count >= players_needed) return true;
		}
	}
	else
	{
		// Need one player on either team
		int	count = 0;

		for (int i = 1; i <= max_players; i ++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

			count ++;
	
			if (count >= players_needed ) return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Read the stats
//---------------------------------------------------------------------------------
void ManiStats::ReadStats(bool use_steam_id)
{
	char	core_filename[256];
	ManiKeyValues *kv_ptr;

	if (use_steam_id && rank_player_list_size != 0)
	{
		return;
	}
	else if (!use_steam_id && rank_player_name_list_size != 0)
	{
		return;
	}

	//WriteDebug("ManiStats::ReadStats() In function steam mode [%s]\n" , (use_steam_id ? "TRUE":"FALSE"));
	if (use_steam_id)
	{
		kv_ptr = new ManiKeyValues("mani_stats.txt");
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_stats.txt", mani_path.GetString());
	}
	else
	{
		kv_ptr = new ManiKeyValues("mani_name_stats.txt");
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/mani_name_stats.txt", mani_path.GetString());
	}

	float timer1 = gpGlobals->curtime;

	kv_ptr->SetKeyPairSize(3,30);
	kv_ptr->SetKeySize(4, 100);
	if (!kv_ptr->ReadFile(core_filename))
	{
		MMsg("Failed to load %s\n", core_filename);
		kv_ptr->DeleteThis();
		return;
	}

	MMsg("Time for read = [%f]\n", gpGlobals->curtime - timer1);

	timer1 = gpGlobals->curtime;

	//////////////////////////////////////////////
	read_t *rd_ptr = kv_ptr->GetPrimaryKey();
	if (!rd_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	read_t *stats_ptr = kv_ptr->GetNextKey(rd_ptr);
	if (!stats_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	time_t	current_time;
	time(&current_time);

	for (;;)
	{
		int index;
		rank_t *rank_ptr;

		if (use_steam_id)
		{
			AddToList((void **) &rank_player_list, sizeof(rank_t *), &rank_player_list_size);
			rank_player_list[rank_player_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));
			Q_memset(rank_player_list[rank_player_list_size - 1], 0, sizeof(rank_t));
			index = rank_player_list_size - 1;
			rank_ptr = rank_player_list[index];
		}
		else
		{
			AddToList((void **) &rank_player_name_list, sizeof(rank_t *), &rank_player_name_list_size);
			rank_player_name_list[rank_player_name_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));
			Q_memset(rank_player_name_list[rank_player_name_list_size - 1], 0, sizeof(rank_t));
			index = rank_player_name_list_size - 1;
			rank_ptr = rank_player_name_list[index];
		}

		Q_strcpy(rank_ptr->name, kv_ptr->GetString("na","NULL"));
		Q_strcpy(rank_ptr->steam_id, kv_ptr->GetString("st","NULL"));
		rank_ptr->ip_address[0] = kv_ptr->GetInt("ip1", 0);
		rank_ptr->ip_address[1] = kv_ptr->GetInt("ip2", 0);
		rank_ptr->ip_address[2] = kv_ptr->GetInt("ip3", 0);
		rank_ptr->ip_address[3] = kv_ptr->GetInt("ip4", 0);
		rank_ptr->last_connected = kv_ptr->GetInt("lc", (int) current_time);
		rank_ptr->rank = kv_ptr->GetInt("rk", -1);
		rank_ptr->deaths = kv_ptr->GetInt("de", 0);
		rank_ptr->headshots = kv_ptr->GetInt("hs", 0);
		rank_ptr->kd_ratio = kv_ptr->GetFloat("kd", 0.0);
		rank_ptr->kills = kv_ptr->GetInt("ki", 0);
		rank_ptr->suicides = kv_ptr->GetInt("su", 0);
		rank_ptr->points = kv_ptr->GetFloat("po", 1000.0);
		rank_ptr->points_decay = kv_ptr->GetFloat("pd", 0.0);
		rank_ptr->team_kills = kv_ptr->GetFloat("tk", 0);
		rank_ptr->total_time_online = kv_ptr->GetFloat("to", 0);
		rank_ptr->damage = kv_ptr->GetInt("da", 0);

		// Get hit group information if available
		for (int i = 0; i < MANI_MAX_STATS_HITGROUPS; i ++)
		{
			rank_ptr->hit_groups[i] = kv_ptr->GetInt(hitboxes[i],0);
		}

		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			// CSS specific stuff
			for (int i = 0; i < MANI_MAX_USER_DEF; i++)
			{
				rank_ptr->user_def[i] = kv_ptr->GetInt(stats_user_def_name[i], 0);
			}

			for (int i = 0; i < MANI_MAX_STATS_CSS_WEAPONS; i ++)
			{
				rank_ptr->weapon_kills[i] = kv_ptr->GetInt(weapon_short_name[i], 0);
			}
		}	
		else if (gpManiGameType->IsGameType(MANI_GAME_DOD))
		{
			// DODS specific stuff
			for (int i = 0; i < MANI_MAX_DODS_USER_DEF; i++)
			{
				rank_ptr->user_def[i] = kv_ptr->GetInt(stats_user_def_name[i], 0);
			}

			for (int i = 0; i < MANI_MAX_STATS_DODS_WEAPONS; i ++)
			{
				rank_ptr->weapon_kills[i] = kv_ptr->GetInt(weapon_short_name[i], 0);
			}
		}

		stats_ptr = kv_ptr->GetNextKey(rd_ptr);
		if (!stats_ptr)
		{
			break;
		}
	}

    kv_ptr->DeleteThis();
	MMsg("Time for load into structure = [%f]\n", gpGlobals->curtime - timer1);

	if (use_steam_id)
	{
		qsort(rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id);
	}
	else
	{
		qsort(rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name); 
	}

	//WriteDebug("ManiStats::ReadStats() End\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ranks command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiStats::ProcessMaRanks(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	start_index;
	int	end_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index,ADMIN, ADMIN_CONFIG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	int players_ranked = 0;
	time_t current_time;
	time(&current_time);

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		for (int i = 0; i < rank_player_list_size; i++)
		{
			if (rank_player_list[i]->rank != -1) players_ranked ++;
		}

		float percent_ranked = 0;

		if (rank_player_list_size != 0 && players_ranked != 0)
		{
			percent_ranked = 100.0 *  ((float) players_ranked / (float) rank_player_list_size);
		}

		if (gpCmd->Cmd_Argc() == 1)
		{
			start_index = 0;
			end_index = rank_player_list_size;
		}
		else if (gpCmd->Cmd_Argc() == 2)
		{
			start_index = 0;
			end_index = atoi(gpCmd->Cmd_Argv(1));
		}
		else
		{
			start_index = atoi(gpCmd->Cmd_Argv(1)) - 1;
			end_index = atoi(gpCmd->Cmd_Argv(2));
		}

		if (start_index < 0) start_index = 0;
		if (end_index > rank_player_list_size) end_index = rank_player_list_size;

		OutputToConsole(player_ptr, "Currently %i Players in rank list (Steam Mode)\n", rank_player_list_size);
		OutputToConsole(player_ptr, "Out of those players %.2f percent are ranked\n\n", percent_ranked);
		OutputToConsole(player_ptr, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = start_index; i < end_index; i++)
		{
			OutputToConsole(player_ptr, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_player_list[i]->name,
						rank_player_list[i]->steam_id,
						rank_player_list[i]->rank,
						rank_player_list[i]->kills,
						rank_player_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_player_list[i]->last_connected)
						);
		}
	}
	else
	{
		for (int i = 0; i < rank_player_name_list_size; i++)
		{
			if (rank_player_name_list[i]->rank != -1) players_ranked ++;
		}

		float percent_ranked = 0;

		if (rank_player_name_list_size != 0 && players_ranked != 0)
		{
			percent_ranked = 100.0 * ((float) players_ranked / (float) rank_player_name_list_size);
		}

		if (gpCmd->Cmd_Argc() == 1)
		{
			start_index = 0;
			end_index = rank_player_name_list_size;
		}
		else if (gpCmd->Cmd_Argc() == 2)
		{
			start_index = 0;
			end_index = atoi(gpCmd->Cmd_Argv(1));
		}
		else
		{
			start_index = atoi(gpCmd->Cmd_Argv(1)) - 1;
			end_index = atoi(gpCmd->Cmd_Argv(2));
		}

		if (start_index < 0) start_index = 0;
		if (end_index > rank_player_name_list_size) end_index = rank_player_name_list_size;

		OutputToConsole(player_ptr, "Currently %i Players in rank list (Name mode)\n\n", rank_player_name_list_size);
		OutputToConsole(player_ptr, "Out of those players %.2f percent are ranked\n\n", percent_ranked);
		OutputToConsole(player_ptr, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = start_index; i < end_index; i++)
		{
			OutputToConsole(player_ptr, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_player_name_list[i]->name,
						"N/A",
						rank_player_name_list[i]->rank,
						rank_player_name_list[i]->kills,
						rank_player_name_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_player_name_list[i]->last_connected)
						);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ranks command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiStats::ProcessMaPLRanks(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int start_index;
	int end_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CONFIG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	time_t	current_time;
	time(&current_time);

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		if (gpCmd->Cmd_Argc() == 1)
		{
			start_index = 0;
			end_index = rank_list_size;
		}
		else if (gpCmd->Cmd_Argc() == 2)
		{
			start_index = 0;
			end_index = atoi(gpCmd->Cmd_Argv(1));
		}
		else
		{
			start_index = atoi(gpCmd->Cmd_Argv(1)) - 1;
			end_index = atoi(gpCmd->Cmd_Argv(2));
		}

		if (start_index < 0) start_index = 0;
		if (end_index > rank_list_size) end_index = rank_list_size;

		OutputToConsole(player_ptr, "Currently %i Ranked Players list (Steam Mode)\n\n", rank_list_size);
		OutputToConsole(player_ptr, "Name                      Steam ID             Rank   Kills  Deaths  Days\n");

		for (int i = 0; i < end_index; i++)
		{
			OutputToConsole(player_ptr, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_list[i]->name,
						rank_list[i]->steam_id,
						rank_list[i]->rank,
						rank_list[i]->kills,
						rank_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_list[i]->last_connected)
						);
		}
	}
	else
	{
		if (gpCmd->Cmd_Argc() == 1)
		{
			start_index = 0;
			end_index = rank_name_list_size;
		}
		else if (gpCmd->Cmd_Argc() == 2)
		{
			start_index = 0;
			end_index = atoi(gpCmd->Cmd_Argv(1));
		}
		else
		{
			start_index = atoi(gpCmd->Cmd_Argv(1)) - 1;
			end_index = atoi(gpCmd->Cmd_Argv(2));
		}

		if (start_index < 0) start_index = 0;
		if (end_index > rank_name_list_size) end_index = rank_name_list_size;

		OutputToConsole(player_ptr, "Currently %i Ranked Players list (Steam Mode)\n\n", rank_name_list_size);
		OutputToConsole(player_ptr, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = 0; i < end_index; i++)
		{
			OutputToConsole(player_ptr, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_name_list[i]->name,
						"N/A",
						rank_name_list[i]->rank,
						rank_name_list[i]->kills,
						rank_name_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_name_list[i]->last_connected)
						);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_resetrank command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiStats::ProcessMaResetRank(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found = NULL;
	rank_t	**player_found_address = NULL;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CONFIG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	rank_key_address = &rank_key;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in ranked player list
		Q_strcpy(rank_key.steam_id, gpCmd->Cmd_Argv(1));

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_list, 
							rank_player_list_size, 
							sizeof(rank_t *), 
							sort_by_steam_id
							);
	}
	else
	{
		// Do BSearch for name in ranked player name list
		Q_strcpy(rank_key.name, gpCmd->Cmd_Argv(1));

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_name_list, 
							rank_player_name_list_size, 
							sizeof(rank_t *), 
							sort_by_name
							);
	}

	if (player_found_address == NULL)
	{
		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			// Did not find player
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", gpCmd->Cmd_Argv(1))); 
		}
		else
		{
			// Did not find player
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", gpCmd->Cmd_Argv(1))); 
		}

		return PLUGIN_STOP;
	}

	time_t current_time;
	time(&current_time);

	// Player in list so reset rank
	player_found = *player_found_address;
	player_found->kills = 0;
	player_found->deaths = 0;
	player_found->suicides = 0;
	player_found->headshots = 0;
	player_found->kd_ratio = 0.0;
	player_found->last_connected = current_time;
	player_found->rank = -1;
	player_found->points = 1000.0;
	player_found->rank_points = 1000.0;
	player_found->points_decay = 0;
	player_found->total_time_online = 0;
	player_found->damage = 0;
	player_found->team_kills = 0;

	for (int i = 0; i < MANI_MAX_USER_DEF; i ++)
	{
		player_found->user_def[i] = 0;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Reset rank of player [%s] steam id [%s]", player_found->name, gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Reset rank of player [%s] steam id [%s]\n", player_found->name, gpCmd->Cmd_Argv(1));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Find the hash index based on first 5 characters of weapon name plus 
//          a modifier for the 'm' character. This is for speed optimisation.
//---------------------------------------------------------------------------------
int ManiStats::GetCSSWeaponHashIndex(char *weapon_string)
{
	int total = 0;

	for (int i = 0; i < 5; i++)
	{
		if (weapon_string[i] == '\0')
		{
			break;
		}

		if (weapon_string[i] == 'm')
		{
			total += 25;
		}

		total += weapon_string[i];
	}

	return total & 0xff;
}

//---------------------------------------------------------------------------------
// Purpose: Find the hash index based on first 5 characters of weapon name plus 
//          a modifier for the 'm' character. This is for speed optimisation.
//---------------------------------------------------------------------------------
int ManiStats::GetCSSWeaponIndex(char *weapon_string)
{
	for (int i = 0; i < MANI_MAX_STATS_CSS_WEAPONS; i++)
	{
		if (strcmp(weapon_string, css_weapons[i]) == 0)
		{
			return i;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Split an IP address up into unsigned char bytes
//---------------------------------------------------------------------------------
void ManiStats::GetIPList(char *ip_address, unsigned char *ip_list)
{
        char    temp_string[10];
        int     i = 0;
        int     j = 0;
        int     k = 0;

        ip_list[0] = 0;
        ip_list[1] = 0;
        ip_list[2] = 0;
        ip_list[3] = 0;

        while (ip_address[i] != '\0')
        {
                if (ip_address[i] == '.')
                {
                        temp_string[j] = '\0';
                        j = 0;
                        ip_list[k++] = atoi(temp_string);
                        if (k == 4) return;
                        i++;
                        continue;
                }

                temp_string[j++] = ip_address[i++];
        }

        temp_string[j] = '\0';
        ip_list[k++] = atoi(temp_string);

        return;
}

static int sort_by_steam_id ( const void *m1,  const void *m2) 
{
	return strcmp((*(rank_t **) m1)->steam_id, (*(rank_t **) m2)->steam_id);
}

static int sort_by_name ( const void *m1,  const void *m2) 
{
	return strcmp((*(rank_t **) m1)->name, (*(rank_t **) m2)->name);
}

static int sort_by_kills ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills > mi2->kills)
	{
		return -1;
	}
	else if (mi1->kills < mi2->kills)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_kd_ratio ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kd_ratio > mi2->kd_ratio)
	{
		return -1;
	}
	else if (mi1->kd_ratio < mi2->kd_ratio)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_kills_deaths ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills - mi1->deaths > mi2->kills - mi2->deaths)
	{
		return -1;
	}
	else if (mi1->kills - mi1->deaths < mi2->kills - mi2->deaths)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_points ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	float	m1_total, m2_total;

	m1_total = mi1->points - mi1->points_decay;
	m2_total = mi2->points - mi2->points_decay;

	if (m1_total > m2_total)
	{
		return -1;
	}
	else if (m1_total < m2_total)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_name_kills ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills > mi2->kills)
	{
		return -1;
	}
	else if (mi1->kills < mi2->kills)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_name_kd_ratio ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kd_ratio > mi2->kd_ratio)
	{
		return -1;
	}
	else if (mi1->kd_ratio < mi2->kd_ratio)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_name_kills_deaths ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills - mi1->deaths > mi2->kills - mi2->deaths)
	{
		return -1;
	}
	else if (mi1->kills - mi1->deaths < mi2->kills - mi2->deaths)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_name_points ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	float	m1_total, m2_total;

	m1_total = mi1->points - mi1->points_decay;
	m2_total = mi2->points - mi2->points_decay;

	if (m1_total > m2_total)
	{
		return -1;
	}
	else if (m1_total < m2_total)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_kills_weapon ( const void *m1,  const void *m2) 
{
	struct weaponme_t *mi1 = (struct weaponme_t *) m1;
	struct weaponme_t *mi2 = (struct weaponme_t *) m2;

	if (mi1->kills > mi2->kills)
	{
		return -1;
	}
	else if (mi1->kills < mi2->kills)
	{
		return 1;
	}

	return strcmp(mi1->weapon_name, mi2->weapon_name);
}

SCON_COMMAND(ma_ranks, 2215, MaRanks, false);
SCON_COMMAND(ma_plranks, 2217, MaPLRanks, false);
SCON_COMMAND(ma_resetrank, 2105, MaResetRank, false);

//---------------------------------------------------------------------------------
// Purpose: Number of days before decay period kicks in
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(ManiStatsDecayStart)
{
	if (!FStrEq(pOldString, mani_stats_decay_start.GetString()))
	{
		// cvar changed
		gpManiStats->LoadStats();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Duration in days of decay period for points
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(ManiStatsDecayPeriod)
{
	if (!FStrEq(pOldString, mani_stats_decay_period.GetString()))
	{
		// cvar changed
		gpManiStats->LoadStats();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Rebuild stats based on ignore days changing
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(ManiStatsIgnoreRanks)
{
	if (!FStrEq(pOldString, mani_stats_ignore_ranks_after_x_days.GetString()))
	{
		// cvar changed
		gpManiStats->LoadStats();
	}
}

ManiStats	g_ManiStats;
ManiStats	*gpManiStats;
