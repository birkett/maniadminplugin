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
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_gametype.h"
#include "mani_effects.h"
#include "mani_ghost.h"

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


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiGhost::ManiGhost()
{
	// Init
	gpManiGhost = this;
}

ManiGhost::~ManiGhost()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Setup ghosting for server
//---------------------------------------------------------------------------------
void ManiGhost::Init(void)
{
	// Init to defaults
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		current_ip_list[i].in_use = false;
		current_ip_list[i].is_ghost = false;
		Q_strcpy(current_ip_list[i].ip_address, "");
	}

	// Update for existing players on server
	bool	found_ip = false;
	for (int i = 1; i <= max_players; i++)
	{
		player_t	player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.player_info->IsHLTV()) continue;
		if (player.is_bot) continue;
		if (gpManiClient->HasAccess(player.index, ADMIN, ADMIN_BASIC_ADMIN)) continue;
		if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_GHOST))
		{
			continue;
		}

		current_ip_list[i - 1].in_use = true;
		Q_strcpy(current_ip_list[i - 1].ip_address, player.ip_address);
		current_ip_list[i - 1].is_ghost = false;
		found_ip = true;
	}

	// If any players were found check if any are ghosting
	if (found_ip)
	{
		for (int i = 0; i < max_players; i++)
		{
			if (!current_ip_list[i].in_use) continue;

			for (int j = 0; j < max_players; i ++)
			{
				if (j == i) continue;
				if (!current_ip_list[j].in_use) continue;

				if (FStruEq(current_ip_list[i].ip_address, current_ip_list[j].ip_address))
				{
					current_ip_list[i].is_ghost = true;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on connect
//---------------------------------------------------------------------------------
void ManiGhost::ClientActive(player_t	*player_ptr)
{
	if (player_ptr->is_bot) return;
	if (player_ptr->player_info->IsHLTV()) return;
	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN)) return;
	if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_GHOST))
	{
		return;
	}

	current_ip_list[player_ptr->index - 1].in_use = true;

	Q_strcpy(current_ip_list[player_ptr->index - 1].ip_address, player_ptr->ip_address);

	for (int i = 0; i < max_players; i++)
	{
		if (!current_ip_list[i].in_use) continue;
		if (i == (player_ptr->index - 1)) continue;

		if (FStruEq(current_ip_list[i].ip_address, player_ptr->ip_address))
		{
			current_ip_list[i].is_ghost = true;
			current_ip_list[player_ptr->index - 1].is_ghost = true;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiGhost::ClientDisconnect(player_t	*player_ptr)
{
	if (player_ptr->is_bot) return;
	if (player_ptr->player_info->IsHLTV()) return;

	current_ip_list[player_ptr->index - 1].in_use = false;
	current_ip_list[player_ptr->index - 1].is_ghost = false;

	int	player_count = 0;
	int	ghost_index = 0;

	// Check for existing ghosted players on same ip
	for (int i = 0; i < max_players; i++)
	{
		if (!current_ip_list[i].in_use) continue;
		if (i == (player_ptr->index - 1)) continue;

		if (FStruEq(current_ip_list[i].ip_address, player_ptr->ip_address))
		{
			if (player_count == 0)
			{
				ghost_index = i;
			}

			player_count ++;
		}
	}

	if (player_count == 1)
	{
		// One player left so unghost them
		current_ip_list[ghost_index].is_ghost = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on death
//---------------------------------------------------------------------------------
void ManiGhost::PlayerDeath(player_t *player_ptr)
{
	if (war_mode) return;
	if (mani_blind_ghosters.GetInt() == 0) return;
	if (!current_ip_list[player_ptr->index - 1].in_use) return;
	if (!current_ip_list[player_ptr->index - 1].is_ghost) return;

	bool	player_alive = false;

	// This player is ghosting so they need to be blinded (unless companions are all dead)
	for (int i = 0; i < max_players; i++)
	{
		player_t	player;

		if (!current_ip_list[i].is_ghost) continue;
		if (!current_ip_list[i].in_use) continue;
		if (i == player_ptr->index - 1) continue;

		if (!FStrEq(current_ip_list[player_ptr->index - 1].ip_address, current_ip_list[i].ip_address)) continue;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			current_ip_list[player_ptr->index - 1].in_use = false;
			current_ip_list[player_ptr->index - 1].is_ghost = false;
			continue;
		}

		// Found player

		if (!player.is_dead && player.team != gpManiGameType->GetSpectatorIndex())
		{
			player_alive = true;
		}
	}
	
	if (!player_alive)
	{
		// Unghost spectating players with same ip
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;
			if (i == player_ptr->index - 1) continue;
			if (!current_ip_list[i].in_use) continue;
			if (!current_ip_list[i].is_ghost) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player)) continue;

			// Found player, unblind them
			BlindPlayer(&player, 0);
		}
	}
	else
	{
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			BlindPlayer(player_ptr, 255);
			SayToPlayer(ORANGE_CHAT, player_ptr, "You have been temporarily blinded for ghosting on an IP address with another player");
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check all players on round start
//---------------------------------------------------------------------------------
void ManiGhost::RoundStart(void)
{
	if (war_mode) return;
	if (mani_blind_ghosters.GetInt() == 0) return;
	if (!gpManiGameType->IsSpectatorAllowed()) return;

	for (int i = 0; i < max_players; i++)
	{
		if (!current_ip_list[i].in_use) continue;
		if (!current_ip_list[i].is_ghost) continue;

		player_t	player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;

		if (player.team == gpManiGameType->GetSpectatorIndex())
		{
			BlindPlayer(&player, 0);
			SayToPlayer(ORANGE_CHAT, &player, "You have been temporarily blinded for ghosting on an IP address with another player");
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Provides true if the player has same IP as someone else
//---------------------------------------------------------------------------------
bool ManiGhost::IsGhosting(player_t *player_ptr)
{
	if (war_mode) return false;
	if (!current_ip_list[player_ptr->index - 1].in_use) return false;

	if (current_ip_list[player_ptr->index - 1].is_ghost)
	{
		return true;
	}

	return false;
}

ManiGhost	g_ManiGhost;
ManiGhost	*gpManiGhost;
