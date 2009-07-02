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
#include "mani_output.h"
#include "mani_reservedslot.h"
#include "mani_autokickban.h"
#include "mani_log_css_stats.h"
#include "mani_save_scores.h"
#include "mani_gametype.h"
#include "mani_netidvalid.h"
#include "mani_trackuser.h"
#include "mani_afk.h"
#include "mani_ping.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerGameDLL	*serverdll;
extern	IPlayerInfoManager *playerinfomanager;

extern	CGlobalVars *gpGlobals;
extern	int	max_players;
extern	ConVar	*sv_lan;

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
	net_id_list = NULL;
	net_id_list_size = 0;
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
	if (net_id_list_size != 0)
	{
		free(net_id_list);
		net_id_list_size = 0;
	}

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
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			player_t	player;

			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				// Ignore source TV
				if (playerinfo->IsHLTV()) continue;

				Q_strcpy(player.steam_id, playerinfo->GetNetworkIDString());

				// Ignore bots
				if (FStrEq(player.steam_id, "BOT")) continue;
				if (FStrEq(player.steam_id, MANI_STEAM_PENDING))
				{
					// Add to list for pending search during game frame
					AddToList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size);
					net_id_list[net_id_list_size - 1].player_index = i;
					time(&net_id_list[net_id_list_size - 1].timer);
					net_id_list[net_id_list_size - 1].timer += mani_steam_id_pending_timeout.GetInt();
					continue;
				}

				// Steam id is okay, just call NetworkIDValidated here
				player.index = i;
				player.player_info = playerinfo;
				player.team = playerinfo->GetTeamIndex();
				player.user_id = playerinfo->GetUserID();
				Q_strcpy(player.name, playerinfo->GetName());
				player.health = playerinfo->GetHealth();
				player.is_dead = playerinfo->IsDead();
				player.entity = pEntity;
				player.is_bot = false;
				GetIPAddressFromPlayer(&player);

				// Call our own callback function instead of Valve's
				this->NetworkIDValidated(&player);
			}
		}
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
void ManiNetIDValid::ClientActive(edict_t *pEntity)
{
	player_t	player;

	player.entity = pEntity;

	if(player.entity && !player.entity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( player.entity );
		if (playerinfo && playerinfo->IsConnected())
		{
			// Ignore source TV
			if (playerinfo->IsHLTV()) return;

			Q_strcpy(player.steam_id, playerinfo->GetNetworkIDString());

			// Ignore bots
			if (FStrEq(player.steam_id, "BOT")) return;

			player.index = engine->IndexOfEdict(player.entity);

			if (FStrEq(player.steam_id, MANI_STEAM_PENDING))
			{
				// Add to list for pending search during game frame
				AddToList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size);
				net_id_list[net_id_list_size - 1].player_index = player.index;
				time(&net_id_list[net_id_list_size - 1].timer);
				net_id_list[net_id_list_size - 1].timer += mani_steam_id_pending_timeout.GetInt();
				return;
			}

			player.user_id = playerinfo->GetUserID();
			player.team = playerinfo->GetTeamIndex();
			player.health = playerinfo->GetHealth();
			player.is_dead = playerinfo->IsDead();
			Q_strcpy(player.name, playerinfo->GetName());
			player.player_info = playerinfo;
			player.is_bot = false;
			GetIPAddressFromPlayer(&player);
			this->NetworkIDValidated(&player);
		}
	}

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

		if (net_id_list)
		{
			// Got some players to parse
			for (int i = 0; i < net_id_list_size; i++)
			{
				edict_t *pEntity = engine->PEntityOfEntIndex(net_id_list[i].player_index);
				if(pEntity && !pEntity->IsFree() )
				{
					player_t	player;

					IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
					if (playerinfo && playerinfo->IsConnected())
					{
						// Ignore source TV
						if (playerinfo->IsHLTV()) continue;

						Q_strcpy(player.steam_id, playerinfo->GetNetworkIDString());

						// Ignore bots
						if (FStrEq(player.steam_id, "BOT")) continue;
						if (FStrEq(player.steam_id, MANI_STEAM_PENDING))
						{
							// Do we need to kick anyone ?
							if (sv_lan->GetInt() == 0 && 
								mani_steam_id_pending_timeout.GetInt() > 0)
							{
								time_t current_time;
								time(&current_time);
								if (net_id_list[i].timer <= current_time)
								{
									// Kick player

									if (mani_steam_id_pending_show_admin.GetInt() != 0)
									{
										// Show to admins
										for (int j = 1; j <= max_players; j++)
										{
											player_t	admin;
											int			admin_index;

											admin.index = j;
											if (!FindPlayerByIndex(&admin)) continue;
											if (admin.is_bot) continue;
											if (!gpManiClient->IsAdmin(&admin, &admin_index)) continue;

											SayToPlayer(&admin, "[MANI_ADMIN_PLUGIN] Warning !! Player %s kicked for invalid Steam ID", player.name);
										}
									}

									char kick_cmd[512];
									Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i Steam ID is invalid ! Try again\n", player.user_id);
									LogCommand (NULL, "Kick (STEAM_ID_PENDING) [%s] [%s] %s", player.name, player.steam_id, kick_cmd);
									engine->ServerCommand(kick_cmd);							
									RemoveIndexFromList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size, i, (void *) &(net_id_list[i]), (void *) &(net_id_list[net_id_list_size - 1]));
									i--;
								}
							}

							continue;
						}

						// Steam id is okay, just call NetworkIDValidated here
						player.index = net_id_list[i].player_index;
						player.player_info = playerinfo;
						player.team = playerinfo->GetTeamIndex();
						player.user_id = playerinfo->GetUserID();
						Q_strcpy(player.name, playerinfo->GetName());
						player.health = playerinfo->GetHealth();
						player.is_dead = playerinfo->IsDead();
						player.entity = pEntity;
						player.is_bot = false;
						GetIPAddressFromPlayer(&player);

						// Call our own callback function instead of Valve's
						this->NetworkIDValidated(&player);
						RemoveIndexFromList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size, i, (void *) &(net_id_list[i]), (void *) &(net_id_list[net_id_list_size - 1]));
						i--;
					}
					else
					{
						RemoveIndexFromList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size, i, (void *) &(net_id_list[i]), (void *) &(net_id_list[net_id_list_size - 1]));
						i--;
					}
				}
				else
				{
					RemoveIndexFromList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size, i, (void *) &(net_id_list[i]), (void *) &(net_id_list[net_id_list_size - 1]));
					i--;
				}
			}
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Map Loaded
//---------------------------------------------------------------------------------
void ManiNetIDValid::ClientDisconnect(player_t *player_ptr)
{
	for (int i = 0; i < net_id_list_size; i++)
	{
		if (net_id_list[i].player_index == player_ptr->index)
		{
			RemoveIndexFromList((void **) &net_id_list, sizeof(net_id_t), &net_id_list_size, i, (void *) &(net_id_list[i]), (void *) &(net_id_list[net_id_list_size - 1]));
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


	if (gpManiAutoKickBan->NetworkIDValidated(player_ptr))
	{
		// Player was let through
		if (mani_reserve_slots.GetInt() == 1)
		{
			if (!gpManiReservedSlot->NetworkIDValidated(player_ptr))
			{
				// Joining player was kicked so return immediately
				return ;
			}
		}
	}

	gpManiClient->NetworkIDValidated(player_ptr);
	gpManiStats->NetworkIDValidated(player_ptr);
	gpManiPing->NetworkIDValidated(player_ptr);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		gpManiLogCSSStats->NetworkIDValidated(player_ptr);
	}

	gpManiSaveScores->NetworkIDValidated(player_ptr);
	gpManiAFK->NetworkIDValidated(player_ptr);

	return ;
}

ManiNetIDValid	g_ManiNetIDValid;
ManiNetIDValid	*gpManiNetIDValid;
