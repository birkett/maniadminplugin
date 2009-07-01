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
#include "mani_victimstats.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

struct	current_ip_t
{
	bool	in_use;
	char	ip_address[128];
};


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiVictimStats::ManiVictimStats()
{
	// Init
	gpManiVictimStats = this;
}

ManiVictimStats::~ManiVictimStats()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on connect
//---------------------------------------------------------------------------------
void ManiVictimStats::ClientActive(player_t	*player_ptr)
{
	if (war_mode) return;
	if (gpManiGameType->IsGameType(MANI_GAME_CSS)) return;

	PlayerSpawn(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiVictimStats::ClientDisconnect(player_t	*player_ptr)
{
	if (war_mode) return;
	if (gpManiGameType->IsGameType(MANI_GAME_CSS)) return;

	PlayerSpawn(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiVictimStats::PlayerSpawn
(
 player_t *player_ptr
 )
{
	int	index = player_ptr->index - 1;
	if (gpManiGameType->IsGameType(MANI_GAME_CSS)) return;

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		damage_list[index][i].health_inflicted = 0;
		damage_list[index][i].armor_inflicted = 0;
		damage_list[index][i].last_hit_time = 0;
		damage_list[index][i].health_taken = 0;
		damage_list[index][i].armor_taken = 0;
		damage_list[index][i].shots_taken = 0;
		damage_list[index][i].shots_inflicted = 0;
		damage_list[index][i].killed = false;
		damage_list[index][i].headshot = false;
		damage_list[index][i].shown_stats = false;
		Q_strcpy(damage_list[index][i].name,"");
		Q_strcpy(damage_list[index][i].weapon_name,"");

		for (int j = 0; j < MANI_MAX_HITGROUPS; j ++)
		{
			damage_list[index][i].hit_groups_taken[j] = 0;
			damage_list[index][i].hit_groups_inflicted[j] = 0;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiVictimStats::PlayerDeath
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 bool headshot, 
 char *weapon_name
 )
{
	int victim_index;
	int attacker_index;

	victim_index = victim_ptr->index - 1;

	if (attacker_ptr->user_id <= 0)
	{
		// World attacked player (i.e. fell too far)
		Q_strcpy(damage_list[victim_index][victim_index].name, victim_ptr->name);
	}
	else
	{
		if (!FindPlayerByUserID(attacker_ptr))
		{
			return;
		}

		Vector	v = attacker_ptr->player_info->GetAbsOrigin() - victim_ptr->player_info->GetAbsOrigin();

		// Update attackers matrix
		attacker_index = attacker_ptr->index - 1;
		damage_list[attacker_index][victim_index].killed = true;
		damage_list[attacker_index][victim_index].headshot = headshot;
		Q_strcpy(damage_list[attacker_index][victim_index].name, victim_ptr->name);
		Q_strcpy(damage_list[attacker_index][victim_index].weapon_name, weapon_name);
		damage_list[attacker_index][victim_index].distance = v.Length() * 0.025;
	}

	// Show dead players stats
	ShowStats(victim_ptr);

}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiVictimStats::PlayerHurt
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 IGameEvent * event
)
{
	int victim_index;
	int attacker_index;
	int health_amount;
	int armor_amount;
	int	total_damage;
	int	hit_group;

	if (mani_show_victim_stats.GetInt() == 0) return;

	victim_index = victim_ptr->index - 1;

	if (attacker_ptr->user_id == 0)
	{
		// World attacked player (i.e. fell too far)
		Q_strcpy(damage_list[victim_index][victim_index].name, victim_ptr->name);
		return;
	}

	health_amount = event->GetInt("dmg_health", 0);
	armor_amount = event->GetInt("dmg_armor", 0);
	hit_group = event->GetInt("hitgroup", 0);
	total_damage = health_amount + armor_amount;

	if (total_damage == 0)
	{
		// No damage inflicted
		return;
	}

	attacker_index = attacker_ptr->index - 1;

	// Update matrix
	damage_list[victim_index][attacker_index].armor_taken += armor_amount;
	damage_list[victim_index][attacker_index].health_taken += health_amount;

	if (damage_list[victim_index][attacker_index].last_hit_time != gpGlobals->curtime)
	{
		damage_list[victim_index][attacker_index].shots_taken += 1;
		damage_list[attacker_index][victim_index].shots_inflicted += 1;
		damage_list[victim_index][attacker_index].hit_groups_taken[hit_group] += 1;
		damage_list[attacker_index][victim_index].hit_groups_inflicted[hit_group] += 1;
	}


	damage_list[victim_index][attacker_index].last_hit_time = gpGlobals->curtime;
	damage_list[attacker_index][victim_index].health_inflicted += health_amount;
	damage_list[attacker_index][victim_index].armor_inflicted += armor_amount;
	damage_list[attacker_index][victim_index].last_hit_time = gpGlobals->curtime;
	Q_strcpy(damage_list[victim_index][attacker_index].name, attacker_ptr->name);
	Q_strcpy(damage_list[attacker_index][victim_index].name, victim_ptr->name);
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiVictimStats::ShowStats(player_t *victim_ptr)
{
	int victim_index;
	player_settings_t	*player_settings;

	if (victim_ptr->is_bot) return;
	victim_index = victim_ptr->index - 1;

	if (damage_list[victim_index][victim_index].shown_stats)
	{
		// Player already seen stats, don't show them again
		return;
	}

	damage_list[victim_index][victim_index].shown_stats = true;

	if (war_mode) return;
	if (!gpManiGameType->IsValidActiveTeam(victim_ptr->team)) return;

	player_settings = FindPlayerSettings(victim_ptr);
	if (!player_settings) return;
	if (!player_settings->damage_stats) return;

	// Normal player, dump stats out to them
	// Show attackers first
	if (mani_show_victim_stats_inflicted_only.GetInt() == 0)
	{
		for (int i = 0; i < max_players; i++)
		{
			if (damage_list[victim_index][i].shots_taken == 0)
			{
				continue;
			}

			char hit_groups[1024];

			Q_strcpy(hit_groups, " ");

			if (player_settings->damage_stats == 2)
			{
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_GENERIC], hit_groups, "Body");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_HEAD], hit_groups, "Head");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_CHEST], hit_groups, "Chest");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_STOMACH], hit_groups, "Stomach");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_LEFTARM], hit_groups, "Left Arm");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_RIGHTARM], hit_groups, "Right Arm");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_LEFTLEG], hit_groups, "Left Leg");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_RIGHTLEG], hit_groups, "Right Leg");
				AddHitGroup(damage_list[victim_index][i].hit_groups_taken[HITGROUP_GEAR], hit_groups, "Gear");
			}

			char attacker_string[256];
			Q_snprintf(attacker_string, sizeof(attacker_string), "ATTACKER %s %c%c %i Dmg, %i Hit(s)%s",
				damage_list[victim_index][i].name,
				0xC2, 0xBB,
				damage_list[victim_index][i].health_taken/* + damage_list[victim_index][i].armor_taken*/,
				damage_list[victim_index][i].shots_taken,
				hit_groups
				);

			SayToPlayer(victim_ptr, "%s", attacker_string);
		}
	}

	for (int i = 0; i < max_players; i++)
	{
		if (damage_list[victim_index][i].shots_inflicted == 0)
		{
			continue;
		}


		if (damage_list[victim_index][i].killed)
		{
			continue;
		}

		char hit_groups[1024];

		Q_strcpy(hit_groups, " ");

		if (player_settings->damage_stats == 2)
		{
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_GENERIC], hit_groups, "Body");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_HEAD], hit_groups, "Head");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_CHEST], hit_groups, "Chest");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_STOMACH], hit_groups, "Stomach");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_LEFTARM], hit_groups, "Left Arm");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_RIGHTARM], hit_groups, "Right Arm");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_LEFTLEG], hit_groups, "Left Leg");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_RIGHTLEG], hit_groups, "Right Leg");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_GEAR], hit_groups, "Gear");
		}

		char victim_string[256];

		Q_snprintf(victim_string, sizeof(victim_string), "VICTIM %s %c%c %i Dmg, %i Hit(s)%s",
			damage_list[victim_index][i].name,
			0xC2, 0xBB,
			damage_list[victim_index][i].health_inflicted/* + damage_list[victim_index][i].armor_inflicted*/,
			damage_list[victim_index][i].shots_inflicted,
			hit_groups);

		SayToPlayer(victim_ptr, "%s", victim_string);
	}

	for (int i = 0; i < max_players; i++)
	{
		if (damage_list[victim_index][i].shots_inflicted == 0)
		{
			continue;
		}


		if (!damage_list[victim_index][i].killed)
		{
			continue;
		}

		char hit_groups[1024];

		Q_strcpy(hit_groups, " ");

		if (player_settings->damage_stats == 2)
		{
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_GENERIC], hit_groups, "Body");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_HEAD], hit_groups, "Head");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_CHEST], hit_groups, "Chest");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_STOMACH], hit_groups, "Stomach");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_LEFTARM], hit_groups, "Left Arm");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_RIGHTARM], hit_groups, "Right Arm");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_LEFTLEG], hit_groups, "Left Leg");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_RIGHTLEG], hit_groups, "Right Leg");
			AddHitGroup(damage_list[victim_index][i].hit_groups_inflicted[HITGROUP_GEAR], hit_groups, "Gear");
		}

		char victim_string[256];

		Q_snprintf(victim_string, sizeof(victim_string), "KILLED %s%s %c%c %i Dmg, %i Hit(s) %s @ %.2fm (%.1fft)%s",
			(damage_list[victim_index][i].headshot) ? "(HS) ":"",
			damage_list[victim_index][i].name,
			0xC2, 0xBB,
			damage_list[victim_index][i].health_inflicted/* + damage_list[victim_index][i].armor_inflicted*/,
			damage_list[victim_index][i].shots_inflicted,
			damage_list[victim_index][i].weapon_name,
			damage_list[victim_index][i].distance,
			(damage_list[victim_index][i].distance * 3.28),
			hit_groups
			);

		SayToPlayer(victim_ptr, "%s", victim_string);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Add hit group text to string
//---------------------------------------------------------------------------------
void	ManiVictimStats::AddHitGroup(int hits, char *final_string, char *bodypart)
{
	if (hits)
	{
		char	hit_text[32];

		Q_snprintf(hit_text, sizeof(hit_text), "%s: %i ", bodypart, hits);
		Q_strcat(final_string, hit_text);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check all players on round start
//---------------------------------------------------------------------------------
void ManiVictimStats::RoundStart(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		for (int j = 0; j < MANI_MAX_PLAYERS; j++)
		{
			damage_list[i][j].health_inflicted = 0;
			damage_list[i][j].armor_inflicted = 0;
			damage_list[i][j].last_hit_time = 0;
			damage_list[i][j].health_taken = 0;
			damage_list[i][j].armor_taken = 0;
			damage_list[i][j].shots_taken = 0;
			damage_list[i][j].shots_inflicted = 0;
			damage_list[i][j].killed = false;
			damage_list[i][j].headshot = false;
			damage_list[i][j].shown_stats = false;
			Q_strcpy(damage_list[i][j].name,"");
			Q_strcpy(damage_list[i][j].weapon_name,"");

			for (int k = 0; k < MANI_MAX_HITGROUPS; k ++)
			{
				damage_list[i][j].hit_groups_taken[k] = 0;
				damage_list[i][j].hit_groups_inflicted[k] = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check all players on round start
//---------------------------------------------------------------------------------
void ManiVictimStats::RoundEnd(void)
{
	if (mani_show_victim_stats.GetInt() == 1 && !war_mode)
	{
		player_t victim;

		for (int i = 1; i <= max_players; i++)
		{
			victim.index = i;
			if (damage_list[i - 1][i - 1].shown_stats)
			{
				continue;
			}

			if (!FindPlayerByIndex(&victim))
			{
				continue;
			}

			ShowStats(&victim);
		}
	}
}

ManiVictimStats	g_ManiVictimStats;
ManiVictimStats	*gpManiVictimStats;