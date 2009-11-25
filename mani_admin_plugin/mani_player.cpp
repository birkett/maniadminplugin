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
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_language.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_menu.h"
#include "mani_quake.h"
#include "mani_output.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_gametype.h"
#include "mani_reservedslot.h"
#include "mani_skins.h"
#include "mani_vfuncs.h"
#include "mani_keyvalues.h"
#include "mani_commands.h"
#include "mani_trackuser.h"
#include "cbaseentity.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	IEngineSound *esounds;
extern	int	max_players;
extern	bool	war_mode;
extern	ConVar *tv_name;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

/*inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}*/

player_t	*target_player_list;
int			target_player_list_size;

player_settings_t	**player_settings_list = NULL;
player_settings_t	**player_settings_name_list = NULL;
player_settings_t	**player_settings_waiting_list = NULL;
player_settings_t	**player_settings_name_waiting_list = NULL;

active_settings_t	active_player_settings[MANI_MAX_PLAYERS];

int	player_settings_list_size = 0;
int	player_settings_name_list_size = 0;
int	player_settings_waiting_list_size = 0;
int	player_settings_name_waiting_list_size = 0;

static	void	WritePlayerSettings(player_settings_t **ps_list, int ps_list_size, char *filename, bool use_steam_id);
static	void	ReadPlayerSettings(void);
static	void	DeleteOldPlayerSettings(void);
static	int		DeriveVersion( const char	*version_string );
static	bool	ProcessSkinChoiceMenu( player_t *player_ptr, int next_index, int argv_offset, int skin_type, char *menu_command);

static int sort_settings_by_steam_id ( const void *m1,  const void *m2); 
static int sort_settings_by_name ( const void *m1,  const void *m2); 

static int sort_settings_by_steam_id ( const void *m1,  const void *m2) 
{
	return strcmp((*(player_settings_t **) m1)->steam_id, (*(player_settings_t **) m2)->steam_id);
}

static int sort_settings_by_name ( const void *m1,  const void *m2) 
{
	return strcmp((*(player_settings_t **) m1)->name, (*(player_settings_t **) m2)->name);
}

static	bool	mani_plugin_paused;

//---------------------------------------------------------------------------------
// Purpose: Set the plugin pause status
//---------------------------------------------------------------------------------
void SetPluginPausedStatus(bool status)
{
	mani_plugin_paused = status;
}

//---------------------------------------------------------------------------------
// Purpose: Check if plugin is paused
//---------------------------------------------------------------------------------
bool ProcessPluginPaused(void)
{
	return mani_plugin_paused;	
}

//---------------------------------------------------------------------------------
// Purpose: FindTargetPlayers using a search string
//---------------------------------------------------------------------------------
bool FindTargetPlayers(player_t *requesting_player, const char *target_string, char *immunity_flag)
{
	player_t player;
	player_t *temp_player_list = NULL;
	int temp_player_list_size = 0;
	
	int	target_user_id = atoi(target_string);
	char target_steam_id[MAX_NETWORKID_LENGTH];

	FreeList((void **) &target_player_list, &target_player_list_size);

	if (target_string == NULL) return false;

	if (FStrEq(target_string, "")) return false;

	if (gpManiGameType->IsTeamPlayAllowed())
	{
		int team = gpManiGameType->GetIndexFromGroup(target_string);

		if (team != -1)
		{
			// Looks like we want many targetted players
			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player)) continue;
				if (player.team != team) continue;
				if (player.player_info->IsHLTV()) continue;

				if (!player.is_bot &&
					immunity_flag && 
					gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
				{
					continue;
				}

				AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
				target_player_list[target_player_list_size - 1] = player;
			}

			if (target_player_list_size != 0)
			{
				return true;
			}

			return false;
		}
	}

	if (FStrEq(target_string,"#ALL"))
	{
		// Looks like we want many targetted players
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.player_info->IsHLTV()) continue;

			if (!player.is_bot &&
				immunity_flag && 
				gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
			{
				continue;
			}

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = player;
		}	

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	// STINGY BASTARD!!! Only doing things for yourself!
	if (FStrEq(target_string,"#SELF"))
	{
		if ( !requesting_player )
			return false;

		AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
		target_player_list[target_player_list_size - 1] = *requesting_player;

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	if (FStrEq(target_string,"#BOT"))
	{
		// Looks like we want many targetted players
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (!player.is_bot) continue;
			if (player.player_info->IsHLTV()) continue;

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = player;
		}	

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	if (FStrEq(target_string,"#ALIVE"))
	{
		// Looks like we want many targetted players
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_dead) continue;
			if (player.player_info->IsHLTV()) continue;

			if (!player.is_bot &&
				immunity_flag && 
				gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
			{
				continue;
			}

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = player;
		}	

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	if (FStrEq(target_string,"#DEAD"))
	{
		// Looks like we want many targetted players
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (!player.is_dead) continue;
			if (player.player_info->IsHLTV()) continue;

			if (!player.is_bot &&
				immunity_flag && 
				gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
			{
				continue;
			}

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = player;
		}	

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	if (FStrEq(target_string,"#HUMAN"))
	{
		// Looks like we want many targetted players
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_dead) continue;
			if (player.player_info->IsHLTV()) continue;
			if (player.is_bot) continue;

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = player;
		}	

		if (target_player_list_size != 0)
		{
			return true;
		}

		return false;
	}

	// Try looking for user id first
	if (target_user_id != 0)
	{
		player.user_id = target_user_id;
		if (FindPlayerByUserID(&player))
		{
			if (player.is_bot || 
				(requesting_player && 
				requesting_player->entity && 
				requesting_player->index == player.index))
			{
				AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
				target_player_list[0] = player;
				return true;
			}

			if (immunity_flag && gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
			{
				return false;
			}

			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[0] = player;
			return true;
		}
	}

	// Try steam id next
	Q_strcpy(target_steam_id, target_string);
	if (Q_strlen(target_steam_id) > 6)
	{
		target_steam_id[6] = '\0';
		if (FStruEq(target_steam_id, "STEAM_"))
		{
			Q_strcpy(player.steam_id, target_string);
			if (FindPlayerBySteamID(&player))
			{
				if (player.is_bot ||
					(requesting_player && 
					requesting_player->entity && 
					requesting_player->index == player.index))	
				{
					AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
					target_player_list[0] = player;
					return true;
				}
	
				if (immunity_flag && gpManiClient->HasAccess(player.index, IMMUNITY, immunity_flag))
				{
					return false;
				}

				AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
				target_player_list[0] = player;
				return true;
			}
		}
	}

	// Get temporary list of current players
	// Shouldn't we really be tracking these via client join
	// and disconnects or is finding the information fast via
	// searches by index ?. My guess is there isn't much difference 

	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player))
		{
			continue;
		}

		if (player.player_info->IsHLTV()) continue;

		AddToList((void **) &temp_player_list, sizeof(player_t), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = player;
	}

	// Search using exact name
	for (int i = 0; i < temp_player_list_size; i++)
	{
		player_t *target_ptr = &(temp_player_list[i]);
		// Is an exact name found ?
		if (!FStruEq(target_ptr->name, target_string))
		{
			continue;
		}

		if (target_ptr->is_bot || 
			(requesting_player && 
			requesting_player->entity && 
			requesting_player->index == target_ptr->index))
		{
			// Player is bot or is player requesting the the target function
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
			FreeList ((void **) &temp_player_list, &temp_player_list_size);
			return true;
		}

		if (immunity_flag == NULL || !gpManiClient->HasAccess(target_ptr->index, IMMUNITY, immunity_flag))
		{
			// Client doesn't have immunity
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
			FreeList ((void **) &temp_player_list, &temp_player_list_size);
			return true;
		}

		return false;
	}

	// Search using exact name (case insensitive)
	for (int i = 0; i < temp_player_list_size; i++)
	{
		player_t *target_ptr = &(temp_player_list[i]);

		// Is an exact name found (case insensitive) ?
		if (!FStrEq(target_ptr->name, target_string))
		{
			continue;
		}

		if (target_ptr->is_bot || 
			(requesting_player && 
			requesting_player->entity && 
			requesting_player->index == target_ptr->index))
		{
			// Player is bot or is player requesting the the target function
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
			FreeList ((void **) &temp_player_list, &temp_player_list_size);
			return true;
		}

		if (immunity_flag == NULL || !gpManiClient->HasAccess(target_ptr->index, IMMUNITY, immunity_flag))
		{
			// Client doesn't have immunity
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
			FreeList ((void **) &temp_player_list, &temp_player_list_size);
			return true;
		}

		return false;
	}

	// Search using Partial name match
	for (int i = 0; i < temp_player_list_size; i++)
	{
		player_t *target_ptr = &(temp_player_list[i]);

		// Is a partial name found ?
		if (Q_stristr(target_ptr->name, target_string) == NULL)
		{
			continue;
		}

		if (target_ptr->is_bot || 
			(requesting_player && 
			requesting_player->entity && 
			requesting_player->index == target_ptr->index))
		{
			// Player is bot or is player requesting the the target function
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
			continue;
		}

		if (immunity_flag == NULL || !gpManiClient->HasAccess(target_ptr->index, IMMUNITY, immunity_flag))
		{
			// Client doesn't have immunity
			AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
			target_player_list[target_player_list_size - 1] = temp_player_list[i];
		}
	}

	FreeList ((void **) &temp_player_list, &temp_player_list_size);

	if (target_player_list_size != 0)
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: FindPlayerBySteamID
//---------------------------------------------------------------------------------
bool FindPlayerBySteamID(player_t *player_ptr)
{
	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (FStrEq(playerinfo->GetNetworkIDString(), player_ptr->steam_id))
				{
					if (playerinfo->IsHLTV()) return false;
					player_ptr->player_info = playerinfo;
					player_ptr->index = i;
					player_ptr->team = playerinfo->GetTeamIndex();
					Q_strcpy(player_ptr->name, playerinfo->GetName());
					player_ptr->entity = pEntity;
					player_ptr->user_id = playerinfo->GetUserID();
					player_ptr->health = playerinfo->GetHealth();
					player_ptr->is_dead = playerinfo->IsObserver() | playerinfo->IsDead();

					if (FStrEq(player_ptr->steam_id,"BOT"))
					{
						if (tv_name && strcmp(player_ptr->name, tv_name->GetString()) == 0)
						{
							return false;
						}
						player_ptr->is_bot = true;
						Q_strcpy(player_ptr->ip_address,"");
					}
					else
					{
						player_ptr->is_bot = false;
						GetIPAddressFromPlayer(player_ptr);
					}

					return true;
				}
			}
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: FindPlayerByIPAddress
//---------------------------------------------------------------------------------
bool FindPlayerByIPAddress(player_t *player_ptr)
{
	player_t player;

	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				player.index = i;
				GetIPAddressFromPlayer(&player);

				if ((player.ip_address[0] != 0) && (FStrEq(player_ptr->ip_address, player.ip_address))) {

					if (playerinfo->IsHLTV()) return false;
					player_ptr->player_info = playerinfo;
					player_ptr->index = i;
					player_ptr->team = playerinfo->GetTeamIndex();
					Q_strcpy(player_ptr->name, playerinfo->GetName());
					Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());
					player_ptr->entity = pEntity;
					player_ptr->user_id = playerinfo->GetUserID();
					player_ptr->health = playerinfo->GetHealth();
					player_ptr->is_dead = playerinfo->IsObserver() | playerinfo->IsDead();

					if (FStrEq(player_ptr->steam_id,"BOT"))
					{
						if (tv_name && strcmp(player_ptr->name, tv_name->GetString()) == 0)
						{
							return false;
						}
						player_ptr->is_bot = true;
						Q_strcpy(player_ptr->ip_address,"");
					}
					else
						player_ptr->is_bot = false;

					return true;
				}
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------------------
// Purpose: FindPlayerByUserID (Using hash table)
//---------------------------------------------------------------------------------
bool FindPlayerByUserID(player_t *player_ptr)
{
	int	org_user_id = player_ptr->user_id;
	player_ptr->index = gpManiTrackUser->GetIndex(org_user_id);
	if (player_ptr->index == -1) return false;

	if (!FindPlayerByIndex(player_ptr))
	{
		return false;
	}

	// Last check if returned user_id matches original
	if (player_ptr->user_id != org_user_id)
	{
		// This should not happen but deal with it anyway
		MMsg("User ID Error in FindPlayerByUserID()\n");
		return false;
	}

	return true;

	/* Removed to use a 128k hash table for much faster
	   lookup of entity index
	
	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->GetUserID() == player_ptr->user_id)
				{
					if (playerinfo->IsHLTV()) return false;
					player_ptr->player_info = playerinfo;
					player_ptr->index = i;
					player_ptr->team = playerinfo->GetTeamIndex();
					Q_strcpy(player_ptr->name, playerinfo->GetName());
					Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());
					player_ptr->health = playerinfo->GetHealth();
					player_ptr->is_dead = playerinfo->IsDead();
					player_ptr->entity = pEntity;
					if (FStrEq(player_ptr->steam_id,"BOT"))
					{
						player_ptr->is_bot = true;
						Q_strcpy(player_ptr->ip_address,"");
					}
					else
					{
						player_ptr->is_bot = false;
						GetIPAddressFromPlayer(player_ptr);
					}
					return true;
				}
			}
		}
	}

	return false;*/
}

//---------------------------------------------------------------------------------
// Purpose: FindPlayerByEntity
//---------------------------------------------------------------------------------
bool FindPlayerByEntity(player_t *player_ptr)
{

	if(player_ptr->entity && !player_ptr->entity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( player_ptr->entity );
		if (playerinfo && playerinfo->IsConnected())
		{
			if (playerinfo->IsHLTV()) return false;
			player_ptr->player_info = playerinfo;
			player_ptr->index = engine->IndexOfEdict(player_ptr->entity);
			player_ptr->user_id = playerinfo->GetUserID();
			player_ptr->team = playerinfo->GetTeamIndex();
			player_ptr->health = playerinfo->GetHealth();
			player_ptr->is_dead = playerinfo->IsObserver() | playerinfo->IsDead();
			Q_strcpy(player_ptr->name, playerinfo->GetName());
			Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());

			if (FStrEq(player_ptr->steam_id,"BOT"))
			{
				if (tv_name && strcmp(player_ptr->name, tv_name->GetString()) == 0)
				{
					return false;
				}

				player_ptr->is_bot = true;
				Q_strcpy(player_ptr->ip_address,"");
			}
			else
			{
				player_ptr->is_bot = false;
				GetIPAddressFromPlayer(player_ptr);
			}

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: FindPlayerByIndex
//---------------------------------------------------------------------------------
bool FindPlayerByIndex(player_t *player_ptr)
{

	if (player_ptr->index < 1 || player_ptr->index > max_players)
	{
		return false;
	}

	edict_t *pEntity = engine->PEntityOfEntIndex(player_ptr->index);
	if(pEntity && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			if (playerinfo->IsHLTV()) return false;
			player_ptr->player_info = playerinfo;
			player_ptr->team = playerinfo->GetTeamIndex();
			player_ptr->user_id = playerinfo->GetUserID();
			Q_strcpy(player_ptr->name, playerinfo->GetName());
			Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());
			player_ptr->health = playerinfo->GetHealth();
			player_ptr->is_dead = playerinfo->IsObserver() | playerinfo->IsDead();
			player_ptr->entity = pEntity;

			if (FStrEq(player_ptr->steam_id,"BOT"))
			{
				if (tv_name && strcmp(player_ptr->name, tv_name->GetString()) == 0)
				{
					return false;
				}

				Q_strcpy(player_ptr->ip_address,"");
				player_ptr->is_bot = true;
			}
			else
			{
				player_ptr->is_bot = false;
				GetIPAddressFromPlayer(player_ptr);
			}
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------------
// Purpose: GetIPAddressFromPlayer
//---------------------------------------------------------------------------------
void GetIPAddressFromPlayer(player_t *player)
{
	INetChannelInfo *nci = engine->GetPlayerNetInfo(player->index);

	if (!nci)
	{
		Q_strcpy(player->ip_address,"");
		return;
	}

	const char *ip_address = nci->GetAddress();

	if (ip_address)
	{
		int str_length = Q_strlen(ip_address);
		for (int i = 0; i <= str_length; i++)
		{
			// Strip port id
			if (ip_address[i] == ':')
			{
				player->ip_address[i] = '\0';
				return;
			}

		player->ip_address[i] = ip_address[i];
		}
	}
	else
	{
		Q_strcpy(player->ip_address,"");
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Init lists to null
//---------------------------------------------------------------------------------
void	InitPlayerSettingsLists (void)
{
	player_settings_list = NULL;
	player_settings_list_size = 0;
	player_settings_name_list = NULL;
	player_settings_name_list_size = 0;
	player_settings_waiting_list = NULL;
	player_settings_waiting_list_size = 0;
	player_settings_name_waiting_list = NULL;
	player_settings_name_waiting_list_size = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Player has joined server, find their settings
//---------------------------------------------------------------------------------
void	PlayerJoinedInitSettings(player_t *player)
{
	player_settings_t *player_settings;

	// Find/Add player to stored list
	player_settings = FindStoredPlayerSettings (player);
	if (!player_settings) return;

	// update active list
	time_t	current_time;

	time(&current_time);
	player_settings->last_connected = current_time;
	active_player_settings[player->index - 1].ptr = player_settings;
	active_player_settings[player->index - 1].active = true;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Find player settings from our database of players
//---------------------------------------------------------------------------------
player_settings_t *FindStoredPlayerSettings (player_t *player)
{
	player_settings_t	player_key;
	player_settings_t	*player_key_address;
	player_settings_t	*player_found = NULL;
	player_settings_t	**player_found_address = NULL;
	player_settings_t	add_player;
	time_t current_time;

	if (FStrEq(player->steam_id,"BOT"))
	{
		// Is Bot or not connected yet
		return NULL;
	}

	if (FStrEq(player->steam_id, "STEAM_ID_PENDING") || FStrEq(player->steam_id, "BOT"))
	{
		// Still connecting
		return NULL;
	}

	player_key_address = &player_key;
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in player settings list
		Q_strcpy(player_key.steam_id, player->steam_id);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_list, player_settings_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id);
	}
	else
	{
		// Do BSearch for name in player settings list
		Q_strcpy(player_key.name, player->name);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_name_list, player_settings_name_list_size, sizeof(player_settings_t *), sort_settings_by_name);
	}

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		time(&current_time);
		Q_strcpy(player_found->name, player->name);
		Q_strcpy(player_found->steam_id, player->steam_id);
		player_found->last_connected = current_time;
		return player_found;
	}

	// Do BSearch in waiting list
	// this stops duplicates entering the list
	player_key_address = &player_key;
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in player settings list
		Q_strcpy(player_key.steam_id, player->steam_id);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_waiting_list, player_settings_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id);
	}
	else
	{
		// Do BSearch for name in player settings list
		Q_strcpy(player_key.name, player->name);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_name_waiting_list, player_settings_name_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_name);
	}

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		time(&current_time);
		Q_strcpy(player_found->name, player->name);
		Q_strcpy(player_found->steam_id, player->steam_id);
		player_found->last_connected = current_time;
		return player_found;
	}


	Q_memset(&add_player, 0, sizeof(player_settings_t));
	time(&current_time);
	Q_strcpy(add_player.steam_id, player->steam_id);
	Q_strcpy(add_player.name, player->name);
	add_player.damage_stats = mani_player_settings_damage.GetInt();
	add_player.show_destruction = mani_player_settings_destructive.GetInt();
	add_player.damage_stats_timeout = 15;
	add_player.quake_sounds = mani_player_settings_quake.GetInt();
	add_player.server_sounds = mani_player_settings_sounds.GetInt();
	add_player.show_death_beam = mani_player_settings_death_beam.GetInt();
	add_player.show_vote_results_progress = mani_player_settings_vote_progress.GetInt();
	add_player.teleport_coords_list = NULL;
	add_player.teleport_coords_list_size = 0;
	add_player.last_connected = current_time;
	for (int i = 0; i < 8; i ++)
	{
		add_player.bit_array[i] = 0;
	}

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Player not in list so Add
		AddToList((void **) &player_settings_waiting_list, sizeof(player_settings_t *), &player_settings_waiting_list_size);
		player_settings_waiting_list[player_settings_waiting_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
		*(player_settings_waiting_list[player_settings_waiting_list_size - 1]) = add_player;
		qsort(player_settings_waiting_list, player_settings_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id); 
	}
	else
	{
		// Player not in list so Add
		AddToList((void **) &player_settings_name_waiting_list, sizeof(player_settings_t *), &player_settings_name_waiting_list_size);
		player_settings_name_waiting_list[player_settings_name_waiting_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
		*(player_settings_name_waiting_list[player_settings_name_waiting_list_size - 1]) = add_player;
		qsort(player_settings_name_waiting_list, player_settings_name_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_name); 
	}

	// Do BSearch in waiting list again, we should find the player this time
	player_key_address = &player_key;
	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in player settings list
		Q_strcpy(player_key.steam_id, player->steam_id);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_waiting_list, player_settings_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id);
	}
	else
	{
		// Do BSearch for name in player settings list
		Q_strcpy(player_key.name, player->name);
		player_found_address = (player_settings_t **) bsearch (&player_key_address, player_settings_name_waiting_list, player_settings_name_waiting_list_size, sizeof(player_settings_t *), sort_settings_by_name);
	}

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		Q_strcpy(player_found->name, player->name);
		Q_strcpy(player_found->steam_id, player->steam_id);
		player_found->last_connected = current_time;
		return player_found;
	}

	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Find player settings from our list of players active on the server
//---------------------------------------------------------------------------------
player_settings_t *FindPlayerSettings (player_t *player)
{
	if (!active_player_settings[player->index - 1].active)
	{
		// Find/Add player to stored list
		player_settings_t *temp_player_settings = FindStoredPlayerSettings (player);
		if (!temp_player_settings) return NULL;

		// update active list
		active_player_settings[player->index - 1].ptr = temp_player_settings;
		active_player_settings[player->index - 1].active = true;	
	}

	return active_player_settings[player->index - 1].ptr;
}

//---------------------------------------------------------------------------------
// Purpose: Find player settings from our list of players active on the server
//---------------------------------------------------------------------------------
bool	FindPlayerFlag (player_settings_t *player_settings_ptr, int flag_index)
{
	int	int_index = 0;
	int	bit_index = 0;
    
	if (!flag_index == 0)
	{
		int_index = flag_index / 32;
	}

	bit_index = flag_index - (int_index * 32);

	return ((player_settings_ptr->bit_array[int_index] & (0x01 << bit_index)) ? true:false);
}

//---------------------------------------------------------------------------------
// Purpose: Set one of the player flags to either true or false
//---------------------------------------------------------------------------------
void	SetPlayerFlag (player_settings_t *player_settings_ptr, int flag_index, bool flag_value)
{
	int	int_index = 0;
	int	bit_index = 0;
    
	if (!flag_index == 0)
	{
		int_index = flag_index / 32;
	}

	bit_index = flag_index - (int_index * 32);

	if (flag_value)
	{
		player_settings_ptr->bit_array[int_index] |= (0x01 << bit_index);
	}
	else
	{
		player_settings_ptr->bit_array[int_index] &= (0xffffffff - (0x01 << bit_index));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Player disconnects from server, set their index to false
//---------------------------------------------------------------------------------
void PlayerSettingsDisconnect (player_t *player)
{
	active_player_settings[player->index - 1].active = false;
	active_player_settings[player->index - 1].ptr = NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Add Player Settings that are waiting to be added to the main list
//---------------------------------------------------------------------------------
void AddWaitingPlayerSettings(void)
{

	for (int i = 0; i < player_settings_waiting_list_size; i++)
	{
		AddToList((void **) &player_settings_list, sizeof(player_settings_t *), &player_settings_list_size);
		player_settings_list[player_settings_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
		player_settings_list[player_settings_list_size - 1] = player_settings_waiting_list[i];
	}

	if (player_settings_waiting_list_size != 0)
	{
//		MMsg("Added %i new players to the main player settings list\n", player_settings_waiting_list_size);
		// Re-sort the list
		qsort(player_settings_list, player_settings_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id); 
		FreeList((void **) &player_settings_waiting_list, &player_settings_waiting_list_size);
	}

	for (int i = 0; i < player_settings_name_waiting_list_size; i++)
	{
		AddToList((void **) &player_settings_name_list, sizeof(player_settings_t *), &player_settings_name_list_size);
		player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
		player_settings_name_list[player_settings_name_list_size - 1] = player_settings_name_waiting_list[i];
	}

	if (player_settings_name_waiting_list_size != 0)
	{
//		MMsg("Added %i new players to the main player name settings list\n", player_settings_name_waiting_list_size);
		// Re-sort the list
		qsort(player_settings_name_list, player_settings_name_list_size, sizeof(player_settings_t *), sort_settings_by_name); 
		FreeList((void **) &player_settings_name_waiting_list, &player_settings_name_waiting_list_size);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset the list of active players / called on level change
//---------------------------------------------------------------------------------
void	ResetActivePlayers(void)
{
	for (int i = 0; i < max_players; i ++)
	{
		player_t player;
		active_player_settings[i].active = false;
		active_player_settings[i].ptr = NULL;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		player_settings_t *player_settings = FindPlayerSettings (&player);
		if (player_settings)
		{
			active_player_settings[i].ptr = player_settings;
			active_player_settings[i].active = true;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: ProcessPlayerSettings
//---------------------------------------------------------------------------------
void	ProcessPlayerSettings(void)
{
	AddWaitingPlayerSettings();
	ResetActivePlayers();
	ReadPlayerSettings();
	DeleteOldPlayerSettings();
	WritePlayerSettings(player_settings_list, player_settings_list_size, "mani_player_settings.txt", true);
	WritePlayerSettings(player_settings_name_list, player_settings_name_list_size, "mani_player_name_settings.txt", false);

	int	steam_size = (sizeof(player_settings_t) * player_settings_list_size) + (sizeof(player_settings_t *) * player_settings_list_size);
	int	name_size = (sizeof(player_settings_t) * player_settings_name_list_size) + (sizeof(player_settings_t *) * player_settings_name_list_size);

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

	MMsg("Steam ID Player Settings memory usage %fMB with %i records\n", steam_size_meg, player_settings_list_size);
	MMsg("Name Player Settings memory usage %fMB with %i records\n", name_size_meg, player_settings_name_list_size);

}

//---------------------------------------------------------------------------------
// Purpose: Write player settings to disk
//---------------------------------------------------------------------------------
static
void	WritePlayerSettings(player_settings_t **ps_list, int ps_list_size, char *filename, bool use_steam_id)
{
	char	base_filename[512];
	ManiKeyValues *settings;

	//Write stats to disk
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/data/%s", mani_path.GetString(), filename);

	if (filesystem->FileExists( base_filename))
	{
//		MMsg("File %s exists, preparing to delete then write new updated stats\n", filename);
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			MMsg("Failed to delete %s\n", filename);
		}
	}

	settings = new ManiKeyValues( filename );

	settings->SetIndent(0);
	if (!settings->WriteStart(base_filename))
	{
		MMsg("Failed to open %s\n", base_filename);
		return;
	}

	settings->WriteKey("version", PLUGIN_VERSION_ID2);

	// Write Player setttings
	for (int i = 0; i < ps_list_size; i ++)
	{
		settings->WriteNewSubKey(i + 1);
		settings->WriteKey("na", ps_list[i]->name);
		settings->WriteKey("st", ps_list[i]->steam_id);
		if (ps_list[i]->damage_stats != 0) settings->WriteKey("ds", ps_list[i]->damage_stats);
		if (ps_list[i]->damage_stats_timeout != 0) settings->WriteKey("dt", ps_list[i]->damage_stats_timeout);
		if (ps_list[i]->show_destruction != 0) settings->WriteKey("de", ps_list[i]->show_destruction);
		if (ps_list[i]->quake_sounds != 0) settings->WriteKey("qs", ps_list[i]->quake_sounds);
		if (ps_list[i]->server_sounds != 0) settings->WriteKey("ss", ps_list[i]->server_sounds);
		if (ps_list[i]->show_death_beam != 0) settings->WriteKey("sd", ps_list[i]->show_death_beam);
		if (ps_list[i]->show_vote_results_progress != 0) settings->WriteKey("sv", ps_list[i]->show_vote_results_progress);

		settings->WriteKey("lc", (int) ps_list[i]->last_connected);

		// Only output if needed, saves disk space
		if (ps_list[i]->admin_t_model && !FStrEq(ps_list[i]->admin_t_model,"")) settings->WriteKey("at", ps_list[i]->admin_t_model);
		if (ps_list[i]->admin_ct_model && !FStrEq(ps_list[i]->admin_ct_model,"")) settings->WriteKey("ac", ps_list[i]->admin_ct_model);
		if (ps_list[i]->immunity_t_model && !FStrEq(ps_list[i]->immunity_t_model,"")) settings->WriteKey("it", ps_list[i]->immunity_t_model);
		if (ps_list[i]->immunity_ct_model && !FStrEq(ps_list[i]->immunity_ct_model,"")) settings->WriteKey("ic", ps_list[i]->immunity_ct_model);
		if (ps_list[i]->t_model && !FStrEq(ps_list[i]->t_model,"")) settings->WriteKey("nt", ps_list[i]->t_model);
		if (ps_list[i]->ct_model && !FStrEq(ps_list[i]->ct_model,"")) settings->WriteKey("nc", ps_list[i]->ct_model);

		if (ps_list[i]->language && !FStrEq(ps_list[i]->language,"")) settings->WriteKey("la", ps_list[i]->language);

		// Handle bit array
		if (ps_list[i]->bit_array[0] != 0) settings->WriteKey("b0", ps_list[i]->bit_array[0]);
		if (ps_list[i]->bit_array[1] != 0) settings->WriteKey("b1", ps_list[i]->bit_array[1]);
		if (ps_list[i]->bit_array[2] != 0) settings->WriteKey("b2", ps_list[i]->bit_array[2]);
		if (ps_list[i]->bit_array[3] != 0) settings->WriteKey("b3", ps_list[i]->bit_array[3]);
		if (ps_list[i]->bit_array[4] != 0) settings->WriteKey("b4", ps_list[i]->bit_array[4]);
		if (ps_list[i]->bit_array[5] != 0) settings->WriteKey("b5", ps_list[i]->bit_array[5]);
		if (ps_list[i]->bit_array[6] != 0) settings->WriteKey("b6", ps_list[i]->bit_array[6]);
		if (ps_list[i]->bit_array[7] != 0) settings->WriteKey("b7", ps_list[i]->bit_array[7]);

		// Do teleport coords if needed
		if (ps_list[i]->teleport_coords_list_size != 0)
		{
			// Write sub key
			settings->WriteNewSubKey("teleport");

			for (int j = 0; j < ps_list[i]->teleport_coords_list_size; j++)
			{
				// Add map subkey
				settings->WriteNewSubKey(ps_list[i]->teleport_coords_list[j].map_name);
				settings->WriteKey("x", ps_list[i]->teleport_coords_list[j].coords.x);
				settings->WriteKey("y", ps_list[i]->teleport_coords_list[j].coords.y);
				settings->WriteKey("z", ps_list[i]->teleport_coords_list[j].coords.z);
				settings->WriteEndSubKey();
			}

			settings->WriteEndSubKey();
		}

		settings->WriteEndSubKey();
	}

	settings->WriteEnd();
	delete settings;
}

//---------------------------------------------------------------------------------
// Purpose: Read player settings from disk
//---------------------------------------------------------------------------------
static
void	ReadPlayerSettings(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	ps_filename[512];
	char	stats_version[128];
	char	old_base_filename[512];
	char	name[64];
	char	steam_id[50];

	if (player_settings_list_size == 0)
	{
		bool	file_found = true;

		Q_strcpy(ps_filename, "mani_player_settings.dat");

		for (;;)
		{
			Q_strcpy(ps_filename, "mani_player_settings.dat");

			//			MMsg("Attempting to read %s file\n", ps_filename);

			//Get settings into memory
			snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), ps_filename);
			file_handle = filesystem->Open (base_filename,"rb",NULL);
			if (file_handle == NULL)
			{
				//				MMsg("Failed to load %s, did not find file\n", ps_filename);
				file_found = false;
				break;
			}

			if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
			{
				//				MMsg("Failed to get version string for %s!!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			if (!ParseLine(stats_version, true, false))
			{
				//				MMsg("Failed to get version string for %s, top line empty !!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			//			MMsg("%s version [%s]\n", ps_filename, stats_version);

			player_settings_t ps;

			Q_memset(&ps, 0, sizeof(player_settings_t));

			switch (DeriveVersion(stats_version))
			{
			case 0:
				{
					// Get player settings into	memory
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);

						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
						player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
						*(player_settings_list[player_settings_list_size - 1]) = ps;
					}

					break;
				}
			case 1:
				{
					// Get player settings into	memory (Version 1)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);

						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
						player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
						*(player_settings_list[player_settings_list_size - 1]) = ps;
					}

					break;
				}
			case 2:
				{
					// Get player settings into	memory (Version 2)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
						player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
						*(player_settings_list[player_settings_list_size - 1]) = ps;
					}

					break;
				}
			case 3:
				{
					// Get player settings into	memory (Version 3)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.show_death_beam), sizeof(ps.show_death_beam), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
						player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
						*(player_settings_list[player_settings_list_size - 1]) = ps;
					}

					break;
				}
			case 4:
				{
					// Get player settings into	memory (Version 3)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.show_death_beam), sizeof(ps.show_death_beam), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						for (int i = 0; i < 8; i++)
						{
							filesystem->Read(&(ps.bit_array[i]), sizeof(ps.bit_array[i]), file_handle);
						}

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
						player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
						*(player_settings_list[player_settings_list_size - 1]) = ps;
					}

					break;
				}
			default : break;
			}

			//			MMsg("Read %i player settings into memory from file %s\n", player_settings_list_size, ps_filename);
			filesystem->Close(file_handle);
			snprintf(old_base_filename, sizeof (old_base_filename), "./cfg/%s/mani_player_settings.dat.old", mani_path.GetString());
			filesystem->RenameFile(base_filename, old_base_filename);

			break;
		}

		if (file_found == false)
		{
			// Do KeyValues system for reading player settings in
			char	core_filename[256];
			ManiKeyValues *kv_ptr;

			Q_strcpy(ps_filename, "mani_player_settings.txt");

			kv_ptr = new ManiKeyValues("mani_player_settings.txt");
			snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/%s", mani_path.GetString(), ps_filename);

			kv_ptr->SetKeyPairSize(5,20);
			kv_ptr->SetKeySize(5, 500);

			if (!kv_ptr->ReadFile(core_filename))
			{
				MMsg("Failed to load %s\n", core_filename);
				kv_ptr->DeleteThis();
				return;
			}

			read_t *rd_ptr = kv_ptr->GetPrimaryKey();
			if (!rd_ptr)
			{
				kv_ptr->DeleteThis();
				return;
			}

			read_t *ply_ptr = kv_ptr->GetNextKey(rd_ptr);
			if (!ply_ptr)
			{
				kv_ptr->DeleteThis();
				return;
			}

			player_settings_t ps;

			for (;;)
			{
				Q_memset(&ps, 0, sizeof(player_settings_t));

				Q_strcpy(ps.name, kv_ptr->GetString("na","NULL"));
				Q_strcpy(ps.steam_id, kv_ptr->GetString("st","NULL"));

				ps.damage_stats = kv_ptr->GetInt("ds", 0);
				ps.damage_stats_timeout = kv_ptr->GetInt("dt", 15);
				ps.show_destruction = kv_ptr->GetInt("de", 0);
				ps.quake_sounds = kv_ptr->GetInt("qs", 0);
				ps.server_sounds = kv_ptr->GetInt("ss", 0);
				ps.show_death_beam = kv_ptr->GetInt("sd", 0);
				ps.show_vote_results_progress = kv_ptr->GetInt("sv", 0);
				ps.last_connected = kv_ptr->GetInt("lc", 0);

				Q_strcpy(ps.admin_t_model,kv_ptr->GetString("at", ""));
				Q_strcpy(ps.admin_ct_model,kv_ptr->GetString("ac", ""));
				Q_strcpy(ps.immunity_t_model,kv_ptr->GetString("it", ""));
				Q_strcpy(ps.immunity_ct_model,kv_ptr->GetString("ic", ""));
				Q_strcpy(ps.t_model, kv_ptr->GetString("nt", ""));
				Q_strcpy(ps.ct_model,kv_ptr->GetString("nc", ""));
				Q_strcpy(ps.language,kv_ptr->GetString("la", ""));

				// Bit array loading 
				ps.bit_array[0] = (unsigned int) kv_ptr->GetInt("b0", 0);
				ps.bit_array[1] = (unsigned int) kv_ptr->GetInt("b1", 0);
				ps.bit_array[2] = (unsigned int) kv_ptr->GetInt("b2", 0);
				ps.bit_array[3] = (unsigned int) kv_ptr->GetInt("b3", 0);
				ps.bit_array[4] = (unsigned int) kv_ptr->GetInt("b4", 0);
				ps.bit_array[5] = (unsigned int) kv_ptr->GetInt("b5", 0);
				ps.bit_array[6] = (unsigned int) kv_ptr->GetInt("b6", 0);
				ps.bit_array[7] = (unsigned int) kv_ptr->GetInt("b7", 0);

				read_t *t_key_ptr = kv_ptr->GetNextKey(ply_ptr);
				if (t_key_ptr && FStrEq(t_key_ptr->sub_key_name, "teleport"))
				{
					// Found teleport sub key so start getting coord lists
					read_t *tm_key_ptr = kv_ptr->GetNextKey(t_key_ptr);

					if (tm_key_ptr)
					{
						for (;;)
						{
							teleport_coords_t tc;

							Q_strcpy(tc.map_name, tm_key_ptr->sub_key_name);
							tc.coords.x = kv_ptr->GetFloat("x", 0);
							tc.coords.y = kv_ptr->GetFloat("y", 0);
							tc.coords.z = kv_ptr->GetFloat("z", 0);

							// Need	to add a new map to	the	list
							AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
							ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;

							// Find next map
							tm_key_ptr = kv_ptr->GetNextKey(t_key_ptr);
							if (!tm_key_ptr)
							{
								break;
							}
						}
					}
				}	

				AddToList((void	**)	&player_settings_list, sizeof(player_settings_t	*),	&player_settings_list_size);
				player_settings_list[player_settings_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
				*(player_settings_list[player_settings_list_size - 1]) = ps;

				ply_ptr = kv_ptr->GetNextKey(rd_ptr);
				if (!ply_ptr)
				{
					break;
				}
			}

			kv_ptr->DeleteThis();
			qsort(player_settings_list, player_settings_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id); 

		}
	}

	if (player_settings_name_list_size == 0)
	{
		Q_strcpy(ps_filename, "mani_player_name_settings.dat");
		bool file_found = true;

		for (;;)
		{
//			MMsg("Attempting to read %s file\n", ps_filename);

			//Get settings into memory
			snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), ps_filename);
			file_handle = filesystem->Open (base_filename,"rb",NULL);
			if (file_handle == NULL)
			{
//				MMsg("Failed to load %s, did not find file\n", ps_filename);
				file_found = false;
				break;
			}

			if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
			{
//				MMsg("Failed to get version string for %s!!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			if (!ParseLine(stats_version, true, false))
			{
//				MMsg("Failed to get version string for %s, top line empty !!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

//			MMsg("%s version [%s]\n", ps_filename, stats_version);

			player_settings_t ps;
			Q_memset(&ps, 0, sizeof(player_settings_t));

			switch (DeriveVersion(stats_version))
			{
			case 0:
				{
					// Get player settings into	memory
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void **) &player_settings_name_list, sizeof(player_settings_t), &player_settings_name_list_size);
						player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
						*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;
					}

					break;
				}
			case 1:
				{
					// Get player settings into	memory (Version 1)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void **) &player_settings_name_list, sizeof(player_settings_t), &player_settings_name_list_size);
						player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
						*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;
					}

					break;
				}
			case 2:
				{
					// Get player settings into	memory (Version 1)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void **) &player_settings_name_list, sizeof(player_settings_t), &player_settings_name_list_size);
						player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
						*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;
					}

					break;
				}
			case 3:
				{
					// Get player settings into	memory (Version 1)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.show_death_beam), sizeof(ps.show_death_beam), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void **) &player_settings_name_list, sizeof(player_settings_t), &player_settings_name_list_size);
						player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
						*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;
					}

					break;
				}
			case 4:
				{
					// Get player settings into	memory (Version 1)
					while (filesystem->Read(&steam_id,	sizeof(steam_id), file_handle) >	0)
					{
						filesystem->Read(&name, sizeof(name), file_handle);
						strcpy(ps.name, name);
						strcpy(ps.steam_id, steam_id);
						filesystem->Read(&(ps.damage_stats), sizeof(ps.damage_stats), file_handle);
						filesystem->Read(&(ps.quake_sounds), sizeof(ps.quake_sounds), file_handle);
						filesystem->Read(&(ps.server_sounds), sizeof(ps.server_sounds),	file_handle);
						filesystem->Read(&(ps.last_connected), sizeof(ps.last_connected), file_handle);
						filesystem->Read(&(ps.show_death_beam), sizeof(ps.show_death_beam), file_handle);

						filesystem->Read(&(ps.admin_t_model), sizeof(ps.admin_t_model), file_handle);
						filesystem->Read(&(ps.admin_ct_model), sizeof(ps.admin_ct_model), file_handle);
						filesystem->Read(&(ps.immunity_t_model), sizeof(ps.immunity_t_model), file_handle);
						filesystem->Read(&(ps.immunity_ct_model), sizeof(ps.immunity_ct_model), file_handle);
						filesystem->Read(&(ps.t_model), sizeof(ps.t_model), file_handle);
						filesystem->Read(&(ps.ct_model), sizeof(ps.ct_model), file_handle);
						filesystem->Read(&(ps.language), sizeof(ps.language), file_handle);

						for (int i = 0; i < 8; i++)
						{
							filesystem->Read(&(ps.bit_array[i]), sizeof(ps.bit_array[i]), file_handle);
						}

						filesystem->Read(&(ps.teleport_coords_list_size), sizeof(ps.teleport_coords_list_size),	file_handle);
						ps.teleport_coords_list	= NULL;

						if (ps.teleport_coords_list_size !=	0)
						{
							int	temp_list_size = ps.teleport_coords_list_size;
							for	(int i = 0;	i <	temp_list_size;	i++)
							{
								teleport_coords_t tc;
								float	x,y,z;

								filesystem->Read(&(tc.map_name), sizeof(tc.map_name), file_handle);
								filesystem->Read(&x, sizeof(float),	file_handle);
								filesystem->Read(&y, sizeof(float),	file_handle);
								filesystem->Read(&z, sizeof(float),	file_handle);

								tc.coords.x	= x;
								tc.coords.y	= y;
								tc.coords.z	= z;
								// Need	to add a new map to	the	list
								AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
								ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;
							}	
						}

						AddToList((void **) &player_settings_name_list, sizeof(player_settings_t), &player_settings_name_list_size);
						player_settings_name_list[player_settings_name_list_size - 1] = (player_settings_t *) malloc (sizeof(player_settings_t));
						*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;
					}

					break;
				}

			default : break;
			}

//			MMsg("Read %i player name settings into memory from file %s\n", player_settings_name_list_size, ps_filename);
			filesystem->Close(file_handle);
			snprintf(old_base_filename, sizeof (old_base_filename), "./cfg/%s/mani_player_name_settings.dat.old", mani_path.GetString());
			filesystem->RenameFile(base_filename, old_base_filename);
			break;
		}

		if (file_found == false)
		{
			// Do KeyValues system for reading player settings in
			char	core_filename[256];
			ManiKeyValues *kv_ptr;

			Q_strcpy(ps_filename, "mani_player_name_settings.txt");

			kv_ptr = new ManiKeyValues("mani_player_name_settings.txt");
			snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/%s", mani_path.GetString(), ps_filename);

			kv_ptr->SetKeyPairSize(5,20);
			kv_ptr->SetKeySize(5, 500);

			if (!kv_ptr->ReadFile(core_filename))
			{
				MMsg("Failed to load %s\n", core_filename);
				kv_ptr->DeleteThis();
				return;
			}

			read_t *rd_ptr = kv_ptr->GetPrimaryKey();
			if (!rd_ptr)
			{
				kv_ptr->DeleteThis();
				return;
			}

			read_t *ply_ptr = kv_ptr->GetNextKey(rd_ptr);
			if (!ply_ptr)
			{
				kv_ptr->DeleteThis();
				return;
			}

			player_settings_t ps;

			for (;;)
			{
				Q_memset(&ps, 0, sizeof(player_settings_t));

				Q_strcpy(ps.name, kv_ptr->GetString("na","NULL"));
				Q_strcpy(ps.steam_id, kv_ptr->GetString("st","NULL"));

				ps.damage_stats = kv_ptr->GetInt("ds", 0);
				ps.damage_stats_timeout = kv_ptr->GetInt("dt", 15);
				ps.show_destruction = kv_ptr->GetInt("de", 0);
				ps.quake_sounds = kv_ptr->GetInt("qs", 0);
				ps.server_sounds = kv_ptr->GetInt("ss", 0);
				ps.show_death_beam = kv_ptr->GetInt("sd", 0);
				ps.show_vote_results_progress = kv_ptr->GetInt("sv", 0);
				ps.last_connected = kv_ptr->GetInt("lc", 0);

				Q_strcpy(ps.admin_t_model,kv_ptr->GetString("at", ""));
				Q_strcpy(ps.admin_ct_model,kv_ptr->GetString("ac", ""));
				Q_strcpy(ps.immunity_t_model,kv_ptr->GetString("it", ""));
				Q_strcpy(ps.immunity_ct_model,kv_ptr->GetString("ic", ""));
				Q_strcpy(ps.t_model, kv_ptr->GetString("nt", ""));
				Q_strcpy(ps.ct_model,kv_ptr->GetString("nc", ""));
				Q_strcpy(ps.language,kv_ptr->GetString("la", ""));

				// Bit array loading
				ps.bit_array[0] = (unsigned int) kv_ptr->GetInt("b0", 0);
				ps.bit_array[1] = (unsigned int) kv_ptr->GetInt("b1", 0);
				ps.bit_array[2] = (unsigned int) kv_ptr->GetInt("b2", 0);
				ps.bit_array[3] = (unsigned int) kv_ptr->GetInt("b3", 0);
				ps.bit_array[4] = (unsigned int) kv_ptr->GetInt("b4", 0);
				ps.bit_array[5] = (unsigned int) kv_ptr->GetInt("b5", 0);
				ps.bit_array[6] = (unsigned int) kv_ptr->GetInt("b6", 0);
				ps.bit_array[7] = (unsigned int) kv_ptr->GetInt("b7", 0);

				
				read_t *t_key_ptr = kv_ptr->GetNextKey(ply_ptr);
				if (t_key_ptr && FStrEq(t_key_ptr->sub_key_name, "teleport"))
				{
					// Found teleport sub key so start getting coord lists
					read_t *tm_key_ptr = kv_ptr->GetNextKey(t_key_ptr);

					if (tm_key_ptr)
					{
						for (;;)
						{
							teleport_coords_t tc;

							Q_strcpy(tc.map_name, tm_key_ptr->sub_key_name);
							tc.coords.x = kv_ptr->GetFloat("x", 0);
							tc.coords.y = kv_ptr->GetFloat("y", 0);
							tc.coords.z = kv_ptr->GetFloat("z", 0);

							// Need	to add a new map to	the	list
							AddToList((void	**)	&(ps.teleport_coords_list),	sizeof(teleport_coords_t), &(ps.teleport_coords_list_size));
							ps.teleport_coords_list[ps.teleport_coords_list_size - 1] =	tc;

							// Find next map
							tm_key_ptr = kv_ptr->GetNextKey(t_key_ptr);
							if (!tm_key_ptr)
							{
								break;
							}
						}
					}
				}	

				AddToList((void	**)	&player_settings_name_list, sizeof(player_settings_t	*),	&player_settings_name_list_size);
				player_settings_name_list[player_settings_name_list_size - 1]	= (player_settings_t *)	malloc (sizeof(player_settings_t));
				*(player_settings_name_list[player_settings_name_list_size - 1]) = ps;

				ply_ptr = kv_ptr->GetNextKey(rd_ptr);
				if (!ply_ptr)
				{
					break;
				}
			}

			kv_ptr->DeleteThis();
			qsort(player_settings_name_list, player_settings_name_list_size, sizeof(player_settings_t *), sort_settings_by_name); 

		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Derive a version to use for reading settings
//---------------------------------------------------------------------------------
static
int	DeriveVersion
(
 const char	*version_string
 )
{
	if (FStrEq(version_string, "V1.1.0a") ||
		FStrEq(version_string, "V1.1.0b") ||
		FStrEq(version_string, "V1.1.0c") ||
		FStrEq(version_string, "V1.1.0d") ||
		FStrEq(version_string, "V1.1.0e") ||
		FStrEq(version_string, "V1.1.0f") ||
		FStrEq(version_string, "V1.1.0g") ||
		FStrEq(version_string, "V1.1.0h") ||
		FStrEq(version_string, "V1.1.0i") ||
		FStrEq(version_string, "V1.1.0j") ||
		FStrEq(version_string, "V1.1.0k") ||
		FStrEq(version_string, "V1.1.0l") ||
		FStrEq(version_string, "V1.1.0m") ||
		FStrEq(version_string, "V1.1.0n") ||
		FStrEq(version_string, "V1.1.0o") ||
		FStrEq(version_string, "V1.1.0p"))
	{
		return 0;
	}
	else if (FStrEq(version_string, "V1.1.0q"))
	{
		return 1;
	}
	else if (FStrEq(version_string, "V1.1.0r"))
	{
		return 2;
	}
	else if (FStrEq(version_string, "V1.1.0s") ||
		FStrEq(version_string, "V1.1.0t") ||
		FStrEq(version_string, "V1.1.0u") ||
		FStrEq(version_string, "V1.1.0v") ||
		FStrEq(version_string, "V1.1.0w") ||
		FStrEq(version_string, "V1.1.0x") ||
		FStrEq(version_string, "V1.1.0y") ||
		FStrEq(version_string, "V1.1.0z") ||
		FStrEq(version_string, "V1.1.0za") ||
		FStrEq(version_string, "V1.1.0zb") ||
		FStrEq(version_string, "V1.1.0zc") ||
		FStrEq(version_string, "V1.1.0zd") ||
		FStrEq(version_string, "V1.1.0ze") ||
		FStrEq(version_string, "V1.1.0zf") ||
		FStrEq(version_string, "V1.1.0zg") ||
		FStrEq(version_string, "V1.1.0zh") ||
		FStrEq(version_string, "V1.1.0zi") ||
		FStrEq(version_string, "V1.1.0zj") ||
		FStrEq(version_string, "V1.1.0zk") ||
		FStrEq(version_string, "V1.1.0zl") ||
		FStrEq(version_string, "V1.1.0zm") ||
		FStrEq(version_string, "V1.2BetaA") ||
		FStrEq(version_string, "V1.2BetaB") ||
		FStrEq(version_string, "V1.2BetaC") ||
		FStrEq(version_string, "V1.2BetaD") ||
		FStrEq(version_string, "V1.2BetaE"))
	{
		return 3;
	}
	else
	{
		return 4;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free Player Settings
//---------------------------------------------------------------------------------
void	FreePlayerSettings(void)
{
	for (int i = 0; i < player_settings_list_size; i++)
	{
		if (player_settings_list[i]->teleport_coords_list_size != 0)
		{
			FreeList ((void **) &(player_settings_list[i]->teleport_coords_list), &player_settings_list[i]->teleport_coords_list_size);
		}

		free(player_settings_list[i]);
	}

	FreeList ((void **) &player_settings_list, &player_settings_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Free Player Name Settings
//---------------------------------------------------------------------------------
void	FreePlayerNameSettings(void)
{
	for (int i = 0; i < player_settings_name_list_size; i++)
	{
		if (player_settings_name_list[i]->teleport_coords_list_size != 0)
		{
			FreeList ((void **) &(player_settings_name_list[i]->teleport_coords_list), &player_settings_name_list[i]->teleport_coords_list_size);
		}

		free(player_settings_name_list[i]);
	}

	FreeList ((void **) &player_settings_name_list, &player_settings_name_list_size);

}

//---------------------------------------------------------------------------------
// Purpose: Trim old players
//---------------------------------------------------------------------------------
static
void	DeleteOldPlayerSettings(void)
{
	player_settings_t	**temp_ps_list=NULL;
	int	temp_ps_list_size=0;

	time_t	current_time;
	time(&current_time);

	// Rebuild list
	for (int i = 0; i < player_settings_list_size; i++)
	{
		if (player_settings_list[i]->teleport_coords_list_size == 0 &&
			player_settings_list[i]->last_connected + (14 * 86400) < current_time)
		{
			// Player outside of time limit or has admin teleport coords
			free(player_settings_list[i]);
			continue;
		}

		AddToList((void **) &temp_ps_list, sizeof(player_settings_t *), &temp_ps_list_size);
		temp_ps_list[temp_ps_list_size - 1] = player_settings_list[i];
	}

	// Free the old list
	FreeList((void **) &player_settings_list, &player_settings_list_size);

	// Reset pointer for new list
	player_settings_list = temp_ps_list;
	player_settings_list_size = temp_ps_list_size;
	qsort(player_settings_list, player_settings_list_size, sizeof(player_settings_t *), sort_settings_by_steam_id); 

	temp_ps_list = NULL;
	temp_ps_list_size = 0;

	// Rebuild name list
	for (int i = 0; i < player_settings_name_list_size; i++)
	{
		if (player_settings_name_list[i]->teleport_coords_list_size == 0 &&
			player_settings_name_list[i]->last_connected + (14 * 86400) < current_time)
		{
			// Player outside of time limit or has admin teleport coords
			free(player_settings_name_list[i]);
			continue;
		}

		AddToList((void **) &temp_ps_list, sizeof(player_settings_t *), &temp_ps_list_size);
		temp_ps_list[temp_ps_list_size - 1] = player_settings_name_list[i];
	}

	// Free the old list
	FreeList((void **) &player_settings_name_list, &player_settings_name_list_size);

	// Reset pointer for new list
	player_settings_name_list = temp_ps_list;
	player_settings_name_list_size = temp_ps_list_size;
	qsort(player_settings_name_list, player_settings_name_list_size, sizeof(player_settings_t *), sort_settings_by_name); 
}

//---------------------------------------------------------------------------------
// Purpose: Handle menu selection via settings selection
//---------------------------------------------------------------------------------
int PlayerSettingsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *option;
	this->params.GetParam("option", &option);

	if (strcmp(option, "damagetype") == 0)
	{
		ProcessMaDamage (player_ptr->index);
	}
	else if (strcmp(option, "damagetimeout") == 0)
	{
		ProcessMaDamageTimeout (player_ptr->index);
	}
	else if (strcmp(option, "destruction") == 0)
	{
		ProcessMaDestruction (player_ptr->index);
	}
	else if (strcmp(option, "quake") == 0)
	{
		ProcessMaQuake (player_ptr->index);
	}
	else if (strcmp(option, "deathbeam") == 0)
	{
		ProcessMaDeathBeam (player_ptr->index);
	}
	else if (strcmp(option, "sounds") == 0)
	{
		ProcessMaSounds (player_ptr->index);
	}
	else if (strcmp(option, "voteprogress") == 0)
	{
		ProcessMaVoteProgress (player_ptr->index);
	}
	else if (strcmp(option, "admin_t") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_ADMIN_T_SKIN), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(option, "admin_ct") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_ADMIN_CT_SKIN), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(option, "immunity_t") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_RESERVE_T_SKIN), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(option, "immunity_ct") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_RESERVE_CT_SKIN), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(option, "public_t") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_T_SKIN), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(option, "public_ct") == 0)
	{
		MENUPAGE_CREATE_PARAM(SkinChoicePage, player_ptr, AddParam("skin_type", MANI_CT_SKIN), 0, -1);
		return NEW_MENU;
	}

	return REPOP_MENU;
}

bool PlayerSettingsPage::PopulateMenuPage(player_t *player_ptr)
{
	int team_index;
	if (war_mode) return false;
	player_settings_t *player_settings = FindPlayerSettings(player_ptr);
	if (player_settings == NULL) return false;

	this->SetEscLink("%s", Translate(player_ptr, 1370));
	this->SetTitle("%s", Translate(player_ptr, 1371));

	MenuItem *ptr = NULL;

	if (mani_show_victim_stats.GetInt() != 0)
	{
		ptr = new PlayerSettingsItem;

		if (player_settings->damage_stats == 0) ptr->SetDisplayText("%s", Translate(player_ptr, 1372));
		else if (player_settings->damage_stats == 1) ptr->SetDisplayText("%s", Translate(player_ptr, 1373));
		else if (player_settings->damage_stats == 3) ptr->SetDisplayText("%s", Translate(player_ptr, 1374));
		else ptr->SetDisplayText("%s", Translate(player_ptr, 1375));

		ptr->params.AddParam("option", "damagetype");
		this->AddItem(ptr);

		// Only show victim stats timer if AMX menu allowed and in GUI stats mode
		if (gpManiGameType->IsAMXMenuAllowed() && player_settings->damage_stats == 3)
		{
			ptr = new PlayerSettingsItem;

			char	seconds[4];
			snprintf(seconds, sizeof(seconds), "%is", player_settings->damage_stats_timeout);
			ptr->SetDisplayText("Damage Stats GUI Timer : %s", (player_settings->damage_stats_timeout == 0) ? Translate(player_ptr, 1376):seconds);
			ptr->params.AddParam("option", "damagetimeout");
			this->AddItem(ptr);
		}
	}

	if (mani_stats_most_destructive.GetInt() != 0 && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		ptr = new PlayerSettingsItem;
		ptr->params.AddParam("option", "destruction");
		ptr->SetDisplayText("%s", Translate(player_ptr, 1377, "%s",	(player_settings->show_destruction == 0) ? Translate(player_ptr, M_OFF):Translate(player_ptr, M_ON)));
		this->AddItem(ptr);
	}

	if (mani_quake_sounds.GetInt() != 0)
	{
		ptr = new PlayerSettingsItem;
		ptr->params.AddParam("option", "quake");
		ptr->SetDisplayText("%s", Translate(player_ptr, 1378, "%s",	(player_settings->quake_sounds == 0) ? Translate(player_ptr, M_OFF):Translate(player_ptr, M_ON)));
		this->AddItem(ptr);
	}

	if (mani_show_death_beams.GetInt() != 0 && gpManiGameType->IsDeathBeamAllowed())
	{
		ptr = new PlayerSettingsItem;
		ptr->params.AddParam("option", "deathbeam");
		ptr->SetDisplayText("%s", Translate(player_ptr, 1379, "%s",	(player_settings->show_death_beam == 0) ? Translate(player_ptr, M_OFF):Translate(player_ptr, M_ON)));
		this->AddItem(ptr);
	}

	ptr = new PlayerSettingsItem;
	ptr->params.AddParam("option", "sounds");
	ptr->SetDisplayText("%s", Translate(player_ptr, 1380, "%s", (player_settings->server_sounds == 0) ? Translate(player_ptr, M_OFF):Translate(player_ptr, M_ON)));
	this->AddItem(ptr);

	if (mani_voting.GetInt() == 1)
	{
		ptr = new PlayerSettingsItem;
		ptr->params.AddParam("option", "voteprogress");
		ptr->SetDisplayText("%s", Translate(player_ptr, 1381, "%s", (player_settings->show_vote_results_progress == 0) ? Translate(player_ptr, M_OFF):Translate(player_ptr, M_ON)));
		this->AddItem(ptr);
	}

	if (mani_skins_admin.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SKINS))
		{
			team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);
			ptr = new PlayerSettingsItem;
			ptr->params.AddParam("option", "admin_t");
			ptr->SetDisplayText("%s", Translate(player_ptr, 1382, "%s%s",
				Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(team_index)),
				(!FStrEq(player_settings->admin_t_model,"")) ? player_settings->admin_t_model:Translate(player_ptr, M_NONE)));
			this->AddItem(ptr);

			if (gpManiGameType->IsTeamPlayAllowed())
			{
				ptr = new PlayerSettingsItem;
				ptr->params.AddParam("option", "admin_ct");
				ptr->SetDisplayText("%s", Translate(player_ptr, 1382, "%s%s",
					Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(TEAM_B)),
					(!FStrEq(player_settings->admin_ct_model,"")) ? player_settings->admin_ct_model:Translate(player_ptr, M_NONE)));
				this->AddItem(ptr);
			}
		}
	}

	if (mani_skins_reserved.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_RESERVE_SKIN))
		{
			team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);
			ptr = new PlayerSettingsItem;
			ptr->params.AddParam("option", "immunity_t");
			ptr->SetDisplayText("%s", Translate(player_ptr, 1383, "%s%s",
				Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(team_index)),
				(!FStrEq(player_settings->immunity_t_model,"")) ? player_settings->immunity_t_model:Translate(player_ptr, M_NONE)));
			this->AddItem(ptr);

			if (gpManiGameType->IsTeamPlayAllowed())
			{
				ptr = new PlayerSettingsItem;
				ptr->params.AddParam("option", "immunity_ct");
				ptr->SetDisplayText("%s", Translate(player_ptr, 1383, "%s%s",
					Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(TEAM_B)),
					(!FStrEq(player_settings->immunity_ct_model,"")) ? player_settings->immunity_ct_model:Translate(player_ptr, M_NONE)));
				this->AddItem(ptr);
			}
		}
	}

	if (mani_skins_public.GetInt() != 0)
	{
		team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);
		ptr = new PlayerSettingsItem;
		ptr->params.AddParam("option", "public_t");
		ptr->SetDisplayText("%s", Translate(player_ptr, 1384, "%s%s",
				Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(team_index)),
				(!FStrEq(player_settings->t_model,"")) ? player_settings->t_model:"None"));
		this->AddItem(ptr);

		if (gpManiGameType->IsTeamPlayAllowed())
		{
			ptr = new PlayerSettingsItem;
			ptr->params.AddParam("option", "public_ct");
			ptr->SetDisplayText("%s", Translate(player_ptr, 1384, "%s%s",
						Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(TEAM_B)),
						(!FStrEq(player_settings->ct_model,"")) ? player_settings->ct_model:Translate(player_ptr, M_NONE)));
			this->AddItem(ptr);
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int SkinChoiceItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int index;
	int skin_type;
	char skin_name[20];

	this->params.GetParam("index", &index);
	m_page_ptr->params.GetParam("skin_type", &skin_type);

	if (index == 999)
	{
		Q_strcpy(skin_name,"");
	}
	else
	{
		Q_strcpy(skin_name, skin_list[index].skin_name);
	}

	// Found name to match !!!
	player_settings_t *player_settings;
	player_settings = FindPlayerSettings(player_ptr);
	if (player_settings)
	{
		switch(skin_type)
		{
		case MANI_ADMIN_T_SKIN: Q_strcpy(player_settings->admin_t_model, skin_name); break;
		case MANI_ADMIN_CT_SKIN: Q_strcpy(player_settings->admin_ct_model, skin_name); break;
		case MANI_RESERVE_T_SKIN: Q_strcpy(player_settings->immunity_t_model, skin_name); break;
		case MANI_RESERVE_CT_SKIN: Q_strcpy(player_settings->immunity_ct_model, skin_name); break;
		case MANI_T_SKIN: Q_strcpy(player_settings->t_model, skin_name); break;
		case MANI_CT_SKIN: Q_strcpy(player_settings->ct_model, skin_name); break;
		default:break;
		}

		current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;
	}

	return PREVIOUS_MENU;
}

bool SkinChoicePage::PopulateMenuPage(player_t *player_ptr)
{
	int skin_type;

	this->params.GetParam("skin_type", &skin_type);

	this->SetEscLink("%s", Translate(player_ptr, 1385));
	this->SetTitle("%s", Translate(player_ptr, 1386));

	MenuItem *ptr = NULL;
	if (mani_skins_force_public.GetInt() == 0 || skin_type == MANI_ADMIN_T_SKIN || skin_type == MANI_ADMIN_CT_SKIN
		|| skin_type == MANI_RESERVE_T_SKIN || skin_type == MANI_RESERVE_CT_SKIN)
	{
		ptr = new SkinChoiceItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, M_NONE));
		ptr->params.AddParam("index", 999);
		this->AddItem(ptr);
	}

	for( int i = 0; i < skin_list_size; i++ )
	{
		if (skin_list[i].skin_type != skin_type) continue;

		ptr = new SkinChoiceItem;
		ptr->SetDisplayText("%s", skin_list[i].skin_name);
		ptr->params.AddParam("index", i);
		this->AddItem(ptr);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the damage command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaDamage
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (mani_show_victim_stats.GetInt() != 1) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (player_settings->damage_stats == 0)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1387));
			player_settings->damage_stats = 1;
		}
		else if (player_settings->damage_stats == 1)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1388));
			player_settings->damage_stats = 2;
		}
		else if (player_settings->damage_stats == 2)
		{
			if (gpManiGameType->IsAMXMenuAllowed())
			{
				SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1389));
				player_settings->damage_stats = 3;
			}
			else
			{
				SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1390));
				player_settings->damage_stats = 0;
			}
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1390));
			player_settings->damage_stats = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the damage command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaDamageTimeout
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (mani_show_victim_stats.GetInt() != 1) return PLUGIN_STOP;
	if (!gpManiGameType->IsAMXMenuAllowed()) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		player_settings->damage_stats_timeout += 1;
		if (player_settings->damage_stats_timeout == 26)
		{
			player_settings->damage_stats_timeout = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the damage command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSounds
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->server_sounds)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1391));
			player_settings->server_sounds = 1;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1392));
			player_settings->server_sounds = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the vote progress command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaVoteProgress
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->show_vote_results_progress)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1393));
			player_settings->show_vote_results_progress = 1;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1394));
			player_settings->show_vote_results_progress = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the death beam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaDeathBeam
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (!gpManiGameType->IsDeathBeamAllowed()) return PLUGIN_STOP;

	if (war_mode) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->show_death_beam)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1395));
			player_settings->show_death_beam = 1;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1396));
			player_settings->show_death_beam = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the destruction command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaDestruction
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_STOP;
	if (mani_stats_most_destructive.GetInt() == 0) return PLUGIN_STOP;
	if (war_mode) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->show_destruction)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1397));
			player_settings->show_destruction = 1;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1398));
			player_settings->show_destruction = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the quake command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaQuake
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (mani_quake_sounds.GetInt() != 1) return PLUGIN_STOP;

	// Get player details
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

	player_settings = FindPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->quake_sounds)
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1399));
			player_settings->quake_sounds = 1;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, &player, "%s", Translate(&player, 1400));
			player_settings->quake_sounds = 0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Get the number of players attached to the server.
//---------------------------------------------------------------------------------
int GetNumberOfActivePlayers( bool include_bots)
{
	int	number_of_players = 0;
	player_t	player;

	for (int i = 1; i <= max_players; i ++)
	{
		player.index = i;
		if (FindPlayerByIndex(&player))
		{
			if ((player.is_bot && include_bots) || !player.is_bot )
			{
				number_of_players ++;
			}
		}
	}

	return number_of_players;
}

//---------------------------------------------------------------------------------
// Purpose: Kick a player
//---------------------------------------------------------------------------------
void UTIL_KickPlayer
(
 player_t *player_ptr, 
 char *short_reason, 
 char *long_reason, 
 char *log_reason
 )
{
	char	kick_cmd[256];

	// Handle a bot kick
	//if (player_ptr->is_bot)
	//{
	//	int j = Q_strlen(player_ptr->name) - 1;

	//	while (j != -1)
	//	{
	//		if (player_ptr->name[j] == '\0') break;
	//		if (player_ptr->name[j] == ' ') break;
	//		j--;
	//	}

	//	j++;

	//	snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i %s\n", player_ptr->user_id, short_reason);
	//	LogCommand (NULL, "Kick (%s) [%s] [%s] [%s] kickid \"%s\"\n", log_reason, player_ptr->name, player_ptr->steam_id, player_ptr->ip_address, &(player_ptr->name[j]));
	//	engine->ServerCommand(kick_cmd);
	//	engine->ServerExecute();
	//	return;
	//}

	if ( !player_ptr->is_bot )
		PrintToClientConsole(player_ptr->entity, "%s\n", long_reason);

	snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i %s\n", player_ptr->user_id, short_reason);
	engine->ServerCommand(kick_cmd);	
	engine->ServerExecute();
	LogCommand (NULL, "Kick (%s) [%s] [%s] [%s] kickid %i %s\n", log_reason, player_ptr->name, player_ptr->steam_id, player_ptr->ip_address, player_ptr->user_id, short_reason);
}

//---------------------------------------------------------------------------------
// Purpose: Drop C4
//---------------------------------------------------------------------------------
bool UTIL_DropC4(edict_t *pEntity)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;

	CBaseEntity *pPlayer = pEntity->GetUnknown()->GetBaseEntity();

	CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
	CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 4);

	if (pWeapon)
	{
		if (FStrEq("weapon_c4", CBaseCombatWeapon_GetName(pWeapon)))
		{
			CBasePlayer *pBase = (CBasePlayer *) pPlayer;
			CBasePlayer_WeaponDrop(pBase, pWeapon);
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Emit a sounds only to one player
//---------------------------------------------------------------------------------
void UTIL_EmitSoundSingle(player_t *player_ptr, const char *sound_id)
{
	if (esounds == NULL) return;

	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();

	mrf.AddPlayer(player_ptr->index);
	Vector pos = player_ptr->entity->GetCollideable()->GetCollisionOrigin();
	esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, sound_id, 0.7,  ATTN_NORM, 0, 100, &pos);
}
