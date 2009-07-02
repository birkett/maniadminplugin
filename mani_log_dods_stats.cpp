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
#include "dlls/iplayerinfo.h"
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
#include "mani_log_dods_stats.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

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

static	char	*dod_weapons_log[MANI_MAX_LOG_DODS_WEAPONS] =
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

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiLogDODSStats::ManiLogDODSStats()
{
	gpManiLogDODSStats = this;
}

ManiLogDODSStats::~ManiLogDODSStats()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::Load(void)
{
	this->ResetStats();
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		this->UpdatePlayerIDInfo(&player, false);
	}

	level_ended = false;
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::LevelInit(void)
{
	this->ResetStats();
	level_ended = false;
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::NetworkIDValidated(player_t *player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	if (player_ptr->is_bot) return;
	this->UpdatePlayerIDInfo(player_ptr, true);
}


//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiLogDODSStats::ClientDisconnect(player_t	*player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Dump this players information to the log
	if (player_ptr->is_bot) return;
	this->DumpPlayerStats(player_ptr->index - 1);
	this->ResetPlayerStats(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiLogDODSStats::PlayerDeath
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 bool attacker_exists,
 int  weapon_index
 )
{
	int victim_index;
	int attacker_index;

	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	if (attacker_ptr->user_id <= 0 || !attacker_exists || weapon_index == -1)
	{
		// World attacked player (i.e. fell too far)
		return;
	}

	victim_index = victim_ptr->index - 1;
	attacker_index = attacker_ptr->index - 1;

	// Get weapon index
	int attacker_weapon = map_dod_weapons[weapon_index];
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

	// Handle team kills
	if (attacker_ptr->team == victim_ptr->team &&
		attacker_ptr->index != victim_ptr->index)
	{
		player_stats_list[attacker_index].weapon_stats_list[attacker_weapon].total_team_kills += 1;
	}

	this->DumpPlayerStats(victim_index);
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiLogDODSStats::PlayerHurt
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
	int	weapon_index;

	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;
	if (attacker_ptr->user_id <= 0) return;

	victim_index = victim_ptr->index - 1;
	weapon_index = event->GetInt("weapon", -1);

	// Get weapon index
	if (weapon_index == -1) return;

	int attacker_weapon = map_dod_weapons[weapon_index];
	if (attacker_weapon == -1)
	{
		return;
	}

	attacker_index = attacker_ptr->index - 1;

	health_amount = event->GetInt("damage", 0);
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
void ManiLogDODSStats::RoundEnd(void)
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
void ManiLogDODSStats::PlayerSpawn(player_t *player_ptr)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	// Reset player stats for spawned player.
    UpdatePlayerIDInfo(player_ptr, true);
}

//---------------------------------------------------------------------------------
// Purpose: PlayerFired their gun
//---------------------------------------------------------------------------------
void ManiLogDODSStats::PlayerFired(int index, int weapon_index)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	// Get weapon index
	if (weapon_index == -1) return;

	int attacker_weapon = map_dod_weapons[weapon_index];
	if (attacker_weapon == -1)
	{
		return;
	}

	player_stats_list[index].weapon_stats_list[attacker_weapon].total_shots_fired += 1;
	player_stats_list[index].weapon_stats_list[attacker_weapon].dump = true;
}

//---------------------------------------------------------------------------------
// Purpose: Flag was blocked from capture
//---------------------------------------------------------------------------------
void ManiLogDODSStats::CaptureBlocked(player_t *player_ptr, const char *cp_name)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	char *team_name = gpManiGameType->GetTeamLogName(player_ptr->team);

	if (player_ptr->team == 2)
	{
        UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"allies_blocked_capture\" - \"%s\"\n",
												player_ptr->name, 
												player_ptr->user_id, 
												player_ptr->steam_id, 
												team_name,
												cp_name);
	}
	else
	{
        UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"axis_blocked_capture\" - \"%s\"\n",
												player_ptr->name, 
												player_ptr->user_id, 
												player_ptr->steam_id, 
												team_name,
												cp_name);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Flag was blocked from capture
//---------------------------------------------------------------------------------
void ManiLogDODSStats::PointCaptured(const char *cappers, int cappers_length, const char *cp_name)
{
	if (mani_external_stats_log.GetInt() == 0) return;
	if (war_mode && mani_external_stats_log_allow_war_logs.GetInt() == 0) return;

	for (int i = 0; i < cappers_length; i ++)
	{
		player_t player;

		player.index = cappers[i];
		if (!FindPlayerByIndex(&player)) continue;

		char *team_name = gpManiGameType->GetTeamLogName(player.team);

		if (player.team == 2)
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"allies_capture_flag\" - \"%s\"\n",
				player.name, 
				player.user_id, 
				player.steam_id, 
				team_name,
				cp_name);
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"axis_capture_flag\" - \"%s\"\n",
				player.name, 
				player.user_id, 
				player.steam_id, 
				team_name,
				cp_name);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset all stats for all players
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::ResetStats(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayerStats(i);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::ResetPlayerStats(int index)
{
	Q_strcpy(player_stats_list[index].name, "");
	Q_strcpy(player_stats_list[index].steam_id, "");

	for (int j = 0; j < MANI_MAX_LOG_DODS_WEAPONS; j ++)
	{
		Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, dod_weapons_log[j]);
		player_stats_list[index].weapon_stats_list[j].dump = false;
		player_stats_list[index].weapon_stats_list[j].total_shots_fired = 0;
		player_stats_list[index].weapon_stats_list[j].total_shots_hit = 0;
		player_stats_list[index].weapon_stats_list[j].total_kills = 0;
		player_stats_list[index].weapon_stats_list[j].total_headshots = 0;
		player_stats_list[index].weapon_stats_list[j].total_team_kills = 0;
		player_stats_list[index].weapon_stats_list[j].total_deaths = 0;
		player_stats_list[index].weapon_stats_list[j].total_damage = 0;
		player_stats_list[index].weapon_stats_list[j].last_hit_time = 0.0;
		for (int k = 0; k < MANI_MAX_LOG_DODS_HITGROUPS; k++)
		{
			player_stats_list[index].weapon_stats_list[j].hit_groups[k] = 0;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update player details with the option to reset their stats
//---------------------------------------------------------------------------------
void	ManiLogDODSStats::UpdatePlayerIDInfo(player_t *player_ptr, bool reset_stats)
{
	int index = player_ptr->index - 1;

	Q_strcpy(player_stats_list[index].name, player_ptr->name);
	Q_strcpy(player_stats_list[index].steam_id, player_ptr->steam_id);
	player_stats_list[index].user_id = player_ptr->user_id;

	if (reset_stats)
	{
		for (int j = 0; j < MANI_MAX_LOG_DODS_WEAPONS; j ++)
		{
			Q_strcpy(player_stats_list[index].weapon_stats_list[j].weapon_name, dod_weapons_log[j]);
			player_stats_list[index].weapon_stats_list[j].dump = false;
			player_stats_list[index].weapon_stats_list[j].total_shots_fired = 0;
			player_stats_list[index].weapon_stats_list[j].total_shots_hit = 0;
			player_stats_list[index].weapon_stats_list[j].total_kills = 0;
			player_stats_list[index].weapon_stats_list[j].total_headshots = 0;
			player_stats_list[index].weapon_stats_list[j].total_team_kills = 0;
			player_stats_list[index].weapon_stats_list[j].total_deaths = 0;
			player_stats_list[index].weapon_stats_list[j].total_damage = 0;
			player_stats_list[index].weapon_stats_list[j].last_hit_time = 0.0;
			for (int k = 0; k < MANI_MAX_LOG_DODS_HITGROUPS; k++)
			{
				player_stats_list[index].weapon_stats_list[j].hit_groups[k] = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiLogDODSStats::DumpPlayerStats(int	index)
{
	if (!gpManiGameType->IsValidActiveTeam(player_stats_list[index].team)) return;
	if (FStrEq(player_stats_list[index].name,"")) return;

	char *team_name = gpManiGameType->GetTeamLogName(player_stats_list[index].team);
	char *name = player_stats_list[index].name;
	char *steam_id = player_stats_list[index].steam_id;
	int  user_id = player_stats_list[index].user_id;

	for (int i = 0; i < MANI_MAX_LOG_DODS_WEAPONS; i++)
	{
		if (!player_stats_list[index].weapon_stats_list[i].dump) continue;

		// Dump stats for this weapon to the log

		weapon_stats_t *weapon_stats_ptr = &(player_stats_list[index].weapon_stats_list[i]);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"weaponstats\" (weapon \"%s\") (shots \"%i\") (hits \"%i\") (kills \"%i\") (headshots \"%i\") (tks \"%i\") (damage \"%i\") (deaths \"%i\")\n",
												name, user_id, steam_id, team_name,
												weapon_stats_ptr->weapon_name,
												weapon_stats_ptr->total_shots_fired,
												weapon_stats_ptr->total_shots_hit,
												weapon_stats_ptr->total_kills,
												weapon_stats_ptr->total_headshots,
												weapon_stats_ptr->total_team_kills,
												weapon_stats_ptr->total_damage,
												weapon_stats_ptr->total_deaths);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"weaponstats2\" (weapon \"%s\") (head \"%i\") (chest \"%i\") (stomach \"%i\") (leftarm \"%i\") (rightarm \"%i\") (leftleg \"%i\") (rightleg \"%i\")\n",
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

ManiLogDODSStats	g_ManiLogDODSStats;
ManiLogDODSStats	*gpManiLogDODSStats;