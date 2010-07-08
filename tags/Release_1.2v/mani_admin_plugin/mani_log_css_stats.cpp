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
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_log_css_stats.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

// Define Weapons
static	char	*css_weapons[MANI_MAX_LOG_CSS_WEAPONS] = 
{
"ak47","m4a1","mp5navy","awp","usp","deagle","aug","hegrenade","xm1014",
"knife","g3sg1","sg550","galil","m3","scout","sg552","famas",
"glock","tmp","ump45","p90","m249","elite","mac10","fiveseven",
"p228","flashbang","smokegrenade"
};


ConVar mani_external_stats_css_include_bots ("mani_external_stats_css_include_bots", "0", 0, "0 = no bots kills are logged, 1 = bot kills are logged", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiLogCSSStats::ManiLogCSSStats()
{
	// Setup hash table for weapon search speed improvment
	for (int i = 0; i < 255; i ++)
	{
		hash_table[i] = -1;
	}

	for (int i = 0; i < MANI_MAX_LOG_CSS_WEAPONS; i++)
	{
		hash_table[this->GetHashIndex(css_weapons[i])] = i;
	}

	gpManiLogCSSStats = this;
}

ManiLogCSSStats::~ManiLogCSSStats()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::Load(void)
{
	this->ResetStats();
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		this->UpdatePlayerIDInfo(&player, false);
	}

	level_ended = false;
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::LevelInit(void)
{
	this->ResetStats();
	level_ended = false;
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::NetworkIDValidated(player_t *player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	this->UpdatePlayerIDInfo(player_ptr, true);
}


//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::ClientActive(player_t *player_ptr)
{
	// Only use for bots !!
	if (mani_external_stats_log.GetInt() == 0 || 
		mani_external_stats_css_include_bots.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	if (!player_ptr->is_bot) return;

	this->UpdatePlayerIDInfo(player_ptr, true);
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiLogCSSStats::ClientDisconnect(player_t	*player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Dump this players information to the log
	this->DumpPlayerStats(player_ptr->index - 1);
	this->ResetPlayerStats(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiLogCSSStats::PlayerDeath
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 bool attacker_exists,
 bool headshot, 
 char *weapon_name
 )
{
	int victim_index;
	int attacker_index;

	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	if (attacker_ptr->user_id <= 0 || !attacker_exists)
	{
		// World attacked player (i.e. fell too far)
		return;
	}

	if (mani_external_stats_css_include_bots.GetInt() == 0)
	{
		if (attacker_ptr->is_bot || victim_ptr->is_bot)
		{
			return;
		}
	}

	victim_index = victim_ptr->index - 1;
	attacker_index = attacker_ptr->index - 1;

	// Get weapon index
	int attacker_weapon = hash_table[this->GetHashIndex(weapon_name)];
	if (attacker_weapon == -1)
	{
		return;
	}

	// Update victim's stats with 'deaths'
	player_stats_list[victim_index].weapon_stats_list[attacker_weapon].dump = true;
	player_stats_list[victim_index].weapon_stats_list[attacker_weapon].total_deaths++;
	player_stats_list[victim_index].team = victim_ptr->team;

	// Update the attackers stats
	player_stats_list[attacker_index].team = attacker_ptr->team;
	player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].dump = true;
	player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_kills += 1;
	if (headshot)
	{
		player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_headshots += 1;
	}

	// Handle team kills
	if (attacker_ptr->team == victim_ptr->team &&
		attacker_ptr->index != victim_ptr->index)
	{
		player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_team_kills += 1;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiLogCSSStats::PlayerHurt
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 IGameEvent * event
)
{
	int victim_index;
	int attacker_index;
	int health_amount;
	int	hit_group;
	char	weapon_name[128];

	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;
	if (attacker_ptr->user_id <= 0) return;

	if (mani_external_stats_css_include_bots.GetInt() == 0)
	{
		if (attacker_ptr->is_bot || victim_ptr->is_bot)
		{
			return;
		}
	}

	victim_index = victim_ptr->index - 1;
	Q_strcpy(weapon_name, event->GetString("weapon", ""));

	// Get weapon index
	int attacker_weapon = hash_table[this->GetHashIndex(weapon_name)];
	if (attacker_weapon == -1)
	{
		return;
	}

	attacker_index = attacker_ptr->index - 1;

	health_amount = event->GetInt("dmg_health", 0);
	if (health_amount == 0)
	{
		// No damage inflicted
		return;
	}

	hit_group = event->GetInt("hitgroup", 0);

	attacker_index = attacker_ptr->index - 1;

	// Update the attackers stats
	if (player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].last_hit_time != gpGlobals->curtime)
	{
		player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_shots_hit += 1;
		player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].hit_groups[hit_group] += 1;
	}

	player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].last_hit_time = gpGlobals->curtime;
	player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].dump = true;
	player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_damage += health_amount;
	player_stats_list[attacker_index].team = attacker_ptr->team;
}

//---------------------------------------------------------------------------------
// Purpose: Dump stats on round end
//---------------------------------------------------------------------------------
void ManiLogCSSStats::RoundEnd(void)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Dump stats at the end of the round
	for (int i = 0; i < max_players; i++)
	{
		DumpPlayerStats(i);
		Q_strcpy(player_stats_list[i].name,"");
	}
}

//---------------------------------------------------------------------------------
// Purpose: PlayerSpawned, update their steam id and name in the stats
//---------------------------------------------------------------------------------
void ManiLogCSSStats::PlayerSpawn(player_t *player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Reset player stats for spawned player.
    UpdatePlayerIDInfo(player_ptr, true);
}

//---------------------------------------------------------------------------------
// Purpose: PlayerFired their gun
//---------------------------------------------------------------------------------
void ManiLogCSSStats::PlayerFired(int index, char *weapon_name, bool is_bot)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (is_bot && mani_external_stats_css_include_bots.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Get weapon index
	int attacker_weapon = hash_table[this->GetHashIndex(weapon_name)];
	if (attacker_weapon == -1)
	{
		return;
	}

	player_stats_list[index].weapon_stats_list[attacker_weapon].total_shots_fired += 1;
	player_stats_list[index].weapon_stats_list[attacker_weapon].dump = true;
}

//---------------------------------------------------------------------------------
// Purpose: Reset all stats for all players
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::ResetStats(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayerStats(i);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::ResetPlayerStats(int index)
{
	Q_strcpy(player_stats_list[index].name, "");
	Q_strcpy(player_stats_list[index].steam_id, "");

	for (int j = 0; j < MANI_MAX_LOG_CSS_WEAPONS; j ++)
	{
		Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, css_weapons[j]);
		if (css_weapons[j][0] == 's' && css_weapons[j][1] == 'm')
		{
			Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, "smokegrenade_projectile");
		}

		player_stats_list[index].weapon_stats_list[j].dump = false;
		player_stats_list[index].weapon_stats_list[j].total_shots_fired = 0;
		player_stats_list[index].weapon_stats_list[j].total_shots_hit = 0;
		player_stats_list[index].weapon_stats_list[j].total_kills = 0;
		player_stats_list[index].weapon_stats_list[j].total_headshots = 0;
		player_stats_list[index].weapon_stats_list[j].total_team_kills = 0;
		player_stats_list[index].weapon_stats_list[j].total_deaths = 0;
		player_stats_list[index].weapon_stats_list[j].total_damage = 0;
		player_stats_list[index].weapon_stats_list[j].last_hit_time = 0.0;
		for (int k = 0; k < MANI_MAX_LOG_CSS_HITGROUPS; k++)
		{
			player_stats_list[index].weapon_stats_list[j].hit_groups[k] = 0;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update player details with the option to reset their stats
//---------------------------------------------------------------------------------
void	ManiLogCSSStats::UpdatePlayerIDInfo(player_t *player_ptr, bool reset_stats)
{
	int index = player_ptr->index - 1;

	Q_strcpy(player_stats_list[index].name, player_ptr->name);
	Q_strcpy(player_stats_list[index].steam_id, player_ptr->steam_id);
	player_stats_list[index].user_id = player_ptr->user_id;

	if (reset_stats)
	{
		for (int j = 0; j < MANI_MAX_LOG_CSS_WEAPONS; j ++)
		{
			Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, css_weapons[j]);
			if (css_weapons[j][0] == 's' && css_weapons[j][1] == 'm')
			{
				Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, "smokegrenade_projectile");
			}

			player_stats_list[index].weapon_stats_list[j].dump = false;
			player_stats_list[index].weapon_stats_list[j].total_shots_fired = 0;
			player_stats_list[index].weapon_stats_list[j].total_shots_hit = 0;
			player_stats_list[index].weapon_stats_list[j].total_kills = 0;
			player_stats_list[index].weapon_stats_list[j].total_headshots = 0;
			player_stats_list[index].weapon_stats_list[j].total_team_kills = 0;
			player_stats_list[index].weapon_stats_list[j].total_deaths = 0;
			player_stats_list[index].weapon_stats_list[j].total_damage = 0;
			player_stats_list[index].weapon_stats_list[j].last_hit_time = 0.0;
			for (int k = 0; k < MANI_MAX_LOG_CSS_HITGROUPS; k++)
			{
				player_stats_list[index].weapon_stats_list[j].hit_groups[k] = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiLogCSSStats::DumpPlayerStats(int	index)
{
	if (!gpManiGameType->IsValidActiveTeam(player_stats_list[index].team)) return;
	if (FStrEq(player_stats_list[index].name,"")) return;

	char *team_name = gpManiGameType->GetTeamLogName(player_stats_list[index].team);
	char *name = player_stats_list[index].name;
	char *steam_id = player_stats_list[index].steam_id;
	int  user_id = player_stats_list[index].user_id;

	for (int i = 0; i < MANI_MAX_LOG_CSS_WEAPONS; i++)
	{
		if (!player_stats_list[index].weapon_stats_list[i].dump) continue;

		// Dump stats for this weapon to the log

		weapon_stats_t *weapon_stats_ptr = &(player_stats_list[index].weapon_stats_list[i]);

		UTILLogPrintf( "\"%s<%i><%s><%s>\" triggered \"weaponstats\" (weapon \"%s\") (shots \"%i\") (hits \"%i\") (kills \"%i\") (headshots \"%i\") (tks \"%i\") (damage \"%i\") (deaths \"%i\")\n",
												name, user_id, steam_id, team_name,
												weapon_stats_ptr->weapon_name,
												weapon_stats_ptr->total_shots_fired,
												weapon_stats_ptr->total_shots_hit,
												weapon_stats_ptr->total_kills,
												weapon_stats_ptr->total_headshots,
												weapon_stats_ptr->total_team_kills,
												weapon_stats_ptr->total_damage,
												weapon_stats_ptr->total_deaths);

		UTILLogPrintf( "\"%s<%i><%s><%s>\" triggered \"weaponstats2\" (weapon \"%s\") (head \"%i\") (chest \"%i\") (stomach \"%i\") (leftarm \"%i\") (rightarm \"%i\") (leftleg \"%i\") (rightleg \"%i\")\n",
												name, user_id, steam_id, team_name,
												weapon_stats_ptr->weapon_name,
												weapon_stats_ptr->hit_groups[HITGROUP_HEAD],
												weapon_stats_ptr->hit_groups[HITGROUP_CHEST],
												weapon_stats_ptr->hit_groups[HITGROUP_STOMACH],
												weapon_stats_ptr->hit_groups[HITGROUP_LEFTARM],
												weapon_stats_ptr->hit_groups[HITGROUP_RIGHTARM],
												weapon_stats_ptr->hit_groups[HITGROUP_LEFTLEG],
												weapon_stats_ptr->hit_groups[HITGROUP_RIGHTLEG]);

		weapon_stats_ptr->dump = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Find an index for our weapon data (should this be a bsearch ?)
//---------------------------------------------------------------------------------
int ManiLogCSSStats::FindWeapon(char *weapon_string)
{
	for (int i = 0; i < MANI_MAX_LOG_CSS_WEAPONS; i++)
	{
		if (FStrEq(css_weapons[i], weapon_string))
		{
			// Found match
			return i;
		}
	}

	// No match :(
	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Find the hash index based on first 5 characters of weapon name plus 
//          a modifier for the 'm' character. This is for speed optimisation.
//---------------------------------------------------------------------------------
int ManiLogCSSStats::GetHashIndex(char *weapon_string)
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

ManiLogCSSStats	g_ManiLogCSSStats;
ManiLogCSSStats	*gpManiLogCSSStats;
