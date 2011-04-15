//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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
#include "mani_mostdestructive.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

struct	current_ip_t
{
	bool	in_use;
	char	ip_address[128];
};

ConVar mani_stats_most_destructive_mode ("mani_stats_most_destructive_mode", "0", 0, "0 = By Kills then Damage, 1 = By Damage alone", true, 0, true, 1); 

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiMostDestructive::ManiMostDestructive()
{
	// Init
	Q_memset(destruction_list, 0, sizeof(destructive_t) * MANI_MAX_PLAYERS);
	gpManiMostDestructive = this;
}

ManiMostDestructive::~ManiMostDestructive()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiMostDestructive::PlayerDeath
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 bool attacker_exists
 )
{
	if (war_mode) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (mani_stats_most_destructive.GetInt() == 0) return;

	if (attacker_ptr->user_id <= 0)
	{
		// World attacked player (i.e. fell too far)
		return;
	}

	if (!attacker_exists) return;
	if (attacker_ptr->index == victim_ptr->index) return;
	if (attacker_ptr->team == victim_ptr->team) return;
 
	destruction_list[attacker_ptr->index - 1].kills ++;
	Q_strcpy(destruction_list[attacker_ptr->index - 1].name, attacker_ptr->name);
}

//---------------------------------------------------------------------------------
// Purpose: Update victim/attacker stats matrix
//---------------------------------------------------------------------------------
void ManiMostDestructive::PlayerHurt
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 IGameEvent * event
)
{
	int health_amount;

	if (war_mode) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (mani_stats_most_destructive.GetInt() == 0) return;

	if (attacker_ptr == NULL) return;
	if (attacker_ptr->user_id == 0)
	{
		// World attacked player (i.e. fell too far)
		return;
	}

	if (attacker_ptr->index == victim_ptr->index) return;
	if (attacker_ptr->team == victim_ptr->team) return;

	health_amount = event->GetInt("dmg_health", 0);
	destruction_list[attacker_ptr->index - 1].damage_done += health_amount;
	Q_strcpy(destruction_list[attacker_ptr->index - 1].name, attacker_ptr->name);
}

//---------------------------------------------------------------------------------
// Purpose: Check all players on round start
//---------------------------------------------------------------------------------
void ManiMostDestructive::RoundStart(void)
{
	Q_memset(destruction_list, 0, sizeof(destructive_t) * MANI_MAX_PLAYERS);
}

//---------------------------------------------------------------------------------
// Purpose: Check all players on round start
//---------------------------------------------------------------------------------
void ManiMostDestructive::RoundEnd(void)
{
	player_settings_t	*player_settings;
	int		destruction = 0;
	int		kills = -999;
	int		found_index = -1;
	char	output_message[192];

	if (war_mode) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (mani_stats_most_destructive.GetInt() == 0) return;

	// Find player with most destruction
	for (int i = 0; i < max_players; i ++)
	{
		if (mani_stats_most_destructive_mode.GetInt() != 0)
		{
			if (destruction_list[i].damage_done > destruction)
			{
				found_index = i;
				destruction = destruction_list[i].damage_done;
			}
		}
		else
		{
			if (destruction_list[i].kills > kills)
			{
				destruction = destruction_list[i].damage_done;
				kills = destruction_list[i].kills;
				found_index = i;
			}
			else if (destruction_list[i].kills == kills)
			{
				if (destruction_list[i].damage_done > destruction)
				{
					found_index = i;
					destruction = destruction_list[i].damage_done;
				}
			}
		}
	}

	for (int i = 1; i <= max_players; i ++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (player.is_bot) continue;

		player_settings = FindPlayerSettings(&player);
		if (!player_settings) continue;
		if (player_settings->show_destruction == 0) continue;

		char	player_plural[30];
		if (destruction_list[found_index].kills == 1)
		{
			snprintf(player_plural, sizeof(player_plural), Translate(NULL, 1200));
		}
		else
		{
			snprintf(player_plural, sizeof(player_plural), Translate(NULL, 1201));
		}

		if (destruction_list[found_index].kills == 0) 
		{
			snprintf (output_message, sizeof(output_message), "%s",
							Translate(&player, 1202, "%s%i",
							destruction_list[found_index].name,
							destruction_list[found_index].damage_done));
		}
		else
		{
			snprintf (output_message, sizeof(output_message), "%s", 
							Translate(&player, 1203, "%s%i%i%s",
							destruction_list[found_index].name,
							destruction_list[found_index].damage_done,
							destruction_list[found_index].kills,
							player_plural));
		}

		SplitHintString(output_message, 35);
		MRecipientFilter mrf;
		mrf.RemoveAllRecipients();
		mrf.MakeReliable();
		mrf.AddPlayer(i);
		UTIL_SayHint(&mrf, output_message);
	}
}

ManiMostDestructive	g_ManiMostDestructive;
ManiMostDestructive	*gpManiMostDestructive;
