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
#include "inetchannelinfo.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_stats.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_output.h"
#include "mani_reservedslot.h"
#include "mani_autokickban.h"
#include "mani_log_css_stats.h"
#include "mani_save_scores.h"
#include "mani_gametype.h"
#include "mani_netidvalid.h"
#include "mani_util.h"
#include "mani_trackuser.h"
#include "mani_observer_track.h"
#include "mani_vote.h"
#include "mani_afk.h"
#include "mani_ping.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerGameDLL	*serverdll;
extern	IPlayerInfoManager *playerinfomanager;

extern	CGlobalVars *gpGlobals;
extern	int	max_players;

ConVar mani_steam_id_pending_timeout ("mani_steam_id_pending_timeout", "0", 0, "0 = disabled, > 0 = number of seconds before player is kicked for having STEAM_ID_PENDING steam id", true, 0, true, 90);
ConVar mani_steam_id_pending_show_admin ("mani_steam_id_pending_show_admin", "0", 0, "0 = disabled, 1 = show admins when a player was kicked", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiNetIDValid
//class ManiNetIDValid
//{

ManiNetIDValid::ManiNetIDValid()
{
	// Init
	timeout = -999.0;

	gpManiNetIDValid = this;
}

ManiNetIDValid::~ManiNetIDValid()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Clean up
//---------------------------------------------------------------------------------
void ManiNetIDValid::CleanUp(void)
{
	// Cleanup
	net_id_list.clear();
	timeout = -999.0;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin loaded
//---------------------------------------------------------------------------------
void ManiNetIDValid::Load(void)
{
	this->CleanUp();

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.player_info->IsHLTV()) continue;
		// Ignore bots
		if (FStrEq(player.steam_id, "BOT")) continue;
		if (FStrEq(player.steam_id, MANI_STEAM_PENDING))
		{
			net_id_t temp;
			// Add to list for pending search during game frame

			temp.player_index = i;
			time(&temp.timer);
			temp.timer += mani_steam_id_pending_timeout.GetInt();
			net_id_list.push_back(temp);
			continue;
		}

		// Call our own callback function instead of Valve's
		this->NetworkIDValidated(&player);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Map Loaded
//---------------------------------------------------------------------------------
void ManiNetIDValid::LevelInit(void)
{
	this->CleanUp();
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Add client to list of players to check for
//---------------------------------------------------------------------------------
void ManiNetIDValid::ClientActive(player_t *player_ptr)
{
	// Ignore bots
	if (FStrEq(player_ptr->steam_id, "BOT")) return;
	if (FStrEq(player_ptr->steam_id, MANI_STEAM_PENDING))
	{
		net_id_t temp;
		// Add to list for pending search during game frame

		temp.player_index = player_ptr->index;
		time(&temp.timer);
		temp.timer += mani_steam_id_pending_timeout.GetInt();
		net_id_list.push_back(temp);
		return;
	}

	this->NetworkIDValidated(player_ptr);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Look for pending clients with STEAM_ID of STEAM_ID_PENDING
//---------------------------------------------------------------------------------
void ManiNetIDValid::GameFrame(void)
{

	if (timeout < gpGlobals->curtime)
	{
		timeout = gpGlobals->curtime + 1.0;

		if (!net_id_list.empty())
		{
			// Got some players to parse
			for (std::vector<net_id_t>::iterator i = net_id_list.begin(); i != net_id_list.end(); ++i)
			{
				player_t player;
				player.index = i->player_index;
				if (!FindPlayerByIndex(&player))
				{
					net_id_list.erase(i);
					break;
				}

				if (player.is_bot)
				{
					net_id_list.erase(i);
					break;
				}


				if (FStrEq(player.steam_id, MANI_STEAM_PENDING))
				{
					if (this->TimeoutKick(&player, i->timer))
					{
						// Player was kicked
						net_id_list.erase(i);
						break;
					}
					else
					{
						// player still has time to connect
						continue;
					}
				}

				// Steam id is okay, just call NetworkIDValidated here
				// Call our own callback function instead of Valve's
				this->NetworkIDValidated(&player);
				net_id_list.erase(i);
				break;
			}
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Kick player if exceeded timeout and timeout enabled
//---------------------------------------------------------------------------------
bool ManiNetIDValid::TimeoutKick(player_t *player_ptr, time_t timeout)
{
	time_t current_time;
	time(&current_time);
					// Do we need to kick anyone ?
	if (!IsLAN() && 
		mani_steam_id_pending_timeout.GetInt() > 0)
	{
		if (timeout <= current_time)
		{
			// Kick player

			if (mani_steam_id_pending_show_admin.GetInt() != 0)
			{
				// Show to admins
				for (int j = 1; j <= max_players; j++)
				{
					player_t	admin;
					admin.index = j;
					if (!FindPlayerByIndex(&admin)) continue;
					if (admin.is_bot) continue;
					if (!gpManiClient->HasAccess(admin.index, ADMIN, ADMIN_BASIC_ADMIN)) continue;

					SayToPlayer(ORANGE_CHAT, &admin, "[MANI_ADMIN_PLUGIN] Warning !! Player %s kicked for invalid Steam ID", player_ptr->name);
				}
			}

			char kick_cmd[512];
			snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i Steam ID is invalid ! Try again\n", player_ptr->user_id);
			LogCommand (NULL, "Kick (STEAM_ID_PENDING) [%s] [%s] %s", player_ptr->name, player_ptr->steam_id, kick_cmd);
			engine->ServerCommand(kick_cmd);
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Client Disconnected so remove from net id list
//---------------------------------------------------------------------------------
void ManiNetIDValid::ClientDisconnect(player_t *player_ptr)
{
	for (std::vector<net_id_t>::iterator i = net_id_list.begin(); i != net_id_list.end();++i)
	{
		if (i->player_index == player_ptr->index)
		{
			net_id_list.erase(i);
			break;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Our replacement function for Valves
//---------------------------------------------------------------------------------
void ManiNetIDValid::NetworkIDValidated( player_t *player_ptr )
{
//	MMsg("Mani -> Network ID [%s] Validated\n", player_ptr->steam_id);

	if (ProcessPluginPaused()) return ;

	gpManiClient->NetworkIDValidated(player_ptr);

	gpManiVote->NetworkIDValidated(player_ptr);
	gpManiStats->NetworkIDValidated(player_ptr);
	gpManiPing->NetworkIDValidated(player_ptr);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->NetworkIDValidated(player_ptr);
	}

	gpManiSaveScores->NetworkIDValidated(player_ptr);
	gpManiAFK->NetworkIDValidated(player_ptr);
	gpManiObserverTrack->NetworkIDValidated(player_ptr);
	return ;
}

ManiNetIDValid	g_ManiNetIDValid;
ManiNetIDValid	*gpManiNetIDValid;
