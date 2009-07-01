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
#include "mani_output.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_gametype.h"
#include "mani_skins.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	bool	war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

player_t	*target_player_list;
int			target_player_list_size;

player_settings_t	**player_settings_list = NULL;
player_settings_t	**player_settings_name_list = NULL;
player_settings_t	**player_settings_waiting_list = NULL;
player_settings_t	**player_settings_name_waiting_list = NULL;

player_settings_t	active_player_settings[MANI_MAX_PLAYERS];

int	player_settings_list_size = 0;
int	player_settings_name_list_size = 0;
int	player_settings_waiting_list_size = 0;
int	player_settings_name_waiting_list_size = 0;

static	void	WritePlayerSettings(player_settings_t **ps_list, int ps_list_size, char *filename);
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
bool FindTargetPlayers(player_t *requesting_player, char *target_string, int	immunity_flag)
{
	player_t player;
	player_t *temp_player_list = NULL;
	int temp_player_list_size = 0;
	
	int	target_user_id = Q_atoi(target_string);
	char target_steam_id[128];
	int	immunity_index;

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

				if (immunity_flag != IMMUNITY_DONT_CARE)
				{
					if (gpManiClient->IsImmune(&player, &immunity_index))
					{
						if (gpManiClient->IsImmunityAllowed(immunity_index, immunity_flag))
						{
							continue;
						}
					}
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

			if (immunity_flag != IMMUNITY_DONT_CARE)
			{

				if (gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, immunity_flag))
					{
						continue;
					}
				}
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
			if (player.player_info->IsFakeClient()) continue;

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
			if (requesting_player->entity && requesting_player->index == player.index)	immunity_flag = -1;

			if (immunity_flag == IMMUNITY_DONT_CARE || player.is_bot)
			{
				AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
				target_player_list[0] = player;
				return true;
			}

			if (gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, immunity_flag))
				{
					return false;
				}
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
				if (requesting_player->entity && requesting_player->index == player.index)	immunity_flag = -1;

				if (immunity_flag == IMMUNITY_DONT_CARE || player.is_bot)
				{
					AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
					target_player_list[0] = player;
					return true;
				}
	
				if (gpManiClient->IsImmune(&player, &immunity_index))
				{
					if (gpManiClient->IsImmunityAllowed(immunity_index, immunity_flag))
					{
						return false;
					}
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

		AddToList((void **) &temp_player_list, sizeof(player_t), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = player;
	}

	// Search using exact name
	for (int i = 0; i < temp_player_list_size; i++)
	{
		// Is an exact name found ?
		if (!FStruEq(temp_player_list[i].name, target_string))
		{
			continue;
		}

		if (requesting_player->entity && requesting_player->index == temp_player_list[i].index)	immunity_flag = -1;

		if (immunity_flag != IMMUNITY_DONT_CARE && !player.is_bot)
		{
			if (gpManiClient->IsImmune(&(temp_player_list[i]), &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, immunity_flag))
				{
					FreeList ((void **) &temp_player_list, &temp_player_list_size);
					return false;
				}
			}
		}

		AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
		target_player_list[target_player_list_size - 1] = temp_player_list[i];
		FreeList ((void **) &temp_player_list, &temp_player_list_size);
		return true;
	}

	// Search using exact name (case insensitive)
	for (int i = 0; i < temp_player_list_size; i++)
	{
		// Is an exact name found (case insensitive) ?
		if (!FStrEq(temp_player_list[i].name, target_string))
		{
			continue;
		}

		int	temp_immunity = immunity_flag;

		if (requesting_player->entity && requesting_player->index == temp_player_list[i].index)	temp_immunity = -1;

		if (immunity_flag != IMMUNITY_DONT_CARE && !player.is_bot)
		{
			if (gpManiClient->IsImmune(&(temp_player_list[i]), &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, temp_immunity))
				{
					FreeList ((void **) &temp_player_list, &temp_player_list_size);
					return false;
				}
			}
		}

		AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
		target_player_list[target_player_list_size - 1] = temp_player_list[i];
		FreeList ((void **) &temp_player_list, &temp_player_list_size);
		return true;
	}

	// Search using Partial name match
	for (int i = 0; i < temp_player_list_size; i++)
	{
		// Is a partial name found ?
		if (Q_stristr(temp_player_list[i].name, target_string) == NULL)
		{
			continue;
		}

		int	temp_immunity = immunity_flag;

		if (requesting_player->entity && requesting_player->index == temp_player_list[i].index)	temp_immunity = -1;

		if (immunity_flag != IMMUNITY_DONT_CARE && !player.is_bot)
		{
			if (gpManiClient->IsImmune(&(temp_player_list[i]), &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, temp_immunity))
				{
					FreeList ((void **) &temp_player_list, &temp_player_list_size);
					return false;
				}
			}
		}

		AddToList((void **) &target_player_list, sizeof(player_t), &target_player_list_size);
		target_player_list[target_player_list_size - 1] = temp_player_list[i];
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
					player_ptr->is_dead = playerinfo->IsDead();

					if (player_ptr->player_info->IsFakeClient())
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

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: FindPlayerByUserID
//---------------------------------------------------------------------------------
bool FindPlayerByUserID(player_t *player_ptr)
{
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
					if (player_ptr->player_info->IsFakeClient())
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

	return false;
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
			player_ptr->is_dead = playerinfo->IsDead();
			Q_strcpy(player_ptr->name, playerinfo->GetName());
			Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());

			if (player_ptr->player_info->IsFakeClient())
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
			player_ptr->is_dead = playerinfo->IsDead();
			player_ptr->entity = pEntity;

			if (player_ptr->player_info->IsFakeClient())
			{
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
	active_player_settings[player->index - 1] = *player_settings;
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

	if (player->player_info->IsFakeClient())
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


	Q_memset(&add_player, 0, sizeof(player_t));
	time(&current_time);
	Q_strcpy(add_player.steam_id, player->steam_id);
	Q_strcpy(add_player.name, player->name);
	add_player.damage_stats = mani_player_settings_damage.GetInt();
	add_player.quake_sounds = mani_player_settings_quake.GetInt();
	add_player.server_sounds = mani_player_settings_sounds.GetInt();
	add_player.show_death_beam = mani_player_settings_death_beam.GetInt();
	add_player.teleport_coords_list = NULL;
	add_player.teleport_coords_list_size = 0;
	add_player.last_connected = current_time;

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
		active_player_settings[player->index - 1] = *temp_player_settings;
		active_player_settings[player->index - 1].active = true;	
	}

	return &(active_player_settings[player->index - 1]);
}

//---------------------------------------------------------------------------------
// Purpose: Update a players settings
//---------------------------------------------------------------------------------
void UpdatePlayerSettings (player_t *player, player_settings_t *new_player_settings)
{
	active_player_settings[player->index - 1] = *new_player_settings;
	active_player_settings[player->index - 1].active = true;
}

//---------------------------------------------------------------------------------
// Purpose: Player disconnects from server, set their index to false
//---------------------------------------------------------------------------------
void PlayerSettingsDisconnect (player_t *player)
{
	active_player_settings[player->index - 1].active = false;
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
		Msg("Added %i new players to the main player settings list\n", player_settings_waiting_list_size);
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
		Msg("Added %i new players to the main player name settings list\n", player_settings_name_waiting_list_size);
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

		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		player_settings_t *player_settings = FindPlayerSettings (&player);
		if (player_settings)
		{
			active_player_settings[i] = *player_settings;
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
	WritePlayerSettings(player_settings_list, player_settings_list_size, "mani_player_settings.dat");
	WritePlayerSettings(player_settings_name_list, player_settings_name_list_size, "mani_player_name_settings.dat");
}

//---------------------------------------------------------------------------------
// Purpose: Write player settings to disk
//---------------------------------------------------------------------------------
static
void	WritePlayerSettings(player_settings_t **ps_list, int ps_list_size, char *filename)
{
	FileHandle_t file_handle;
	char	base_filename[512];

	//Write stats to disk
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename);

	if (filesystem->FileExists( base_filename))
	{
		Msg("File %s exists, preparing to delete then write new updated stats\n", filename);
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete %s\n", filename);
		}
	}

	file_handle = filesystem->Open (base_filename,"wb", NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open %s for writing\n", filename);
		return;
	}

	char temp_string[64];
	int temp_length = Q_snprintf(temp_string, sizeof(temp_string), "%s", PLUGIN_VERSION_ID);

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		Msg ("Failed to write version info to %s\n", filename);
		filesystem->Close(file_handle);
		return;
	}

	// Write Player setttings
	for (int i = 0; i < ps_list_size; i ++)
	{
		filesystem->Write((void *) &(ps_list[i]->steam_id), sizeof(ps_list[i]->steam_id), file_handle);
		filesystem->Write((void *) &(ps_list[i]->name), sizeof(ps_list[i]->name), file_handle);
		filesystem->Write((void *) &(ps_list[i]->damage_stats), sizeof(ps_list[i]->damage_stats), file_handle);
		filesystem->Write((void *) &(ps_list[i]->quake_sounds), sizeof(ps_list[i]->quake_sounds), file_handle);
		filesystem->Write((void *) &(ps_list[i]->server_sounds), sizeof(ps_list[i]->server_sounds), file_handle);
		filesystem->Write((void *) &(ps_list[i]->last_connected), sizeof(ps_list[i]->last_connected), file_handle);
		filesystem->Write((void *) &(ps_list[i]->show_death_beam), sizeof(ps_list[i]->show_death_beam), file_handle);

		filesystem->Write((void *) &(ps_list[i]->admin_t_model), sizeof(ps_list[i]->admin_t_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->admin_ct_model), sizeof(ps_list[i]->admin_ct_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->immunity_t_model), sizeof(ps_list[i]->immunity_t_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->immunity_ct_model), sizeof(ps_list[i]->immunity_ct_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->t_model), sizeof(ps_list[i]->t_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->ct_model), sizeof(ps_list[i]->ct_model), file_handle);
		filesystem->Write((void *) &(ps_list[i]->language), sizeof(ps_list[i]->language), file_handle);
		
		filesystem->Write((void *) &(ps_list[i]->teleport_coords_list_size), sizeof(ps_list[i]->teleport_coords_list_size), file_handle);
		if (ps_list[i]->teleport_coords_list_size != 0)
		{
			// Write coord list to disk 
			for (int j = 0; j < ps_list[i]->teleport_coords_list_size; j++)
			{
				filesystem->Write((void *) &(ps_list[i]->teleport_coords_list[j].map_name), sizeof(ps_list[i]->teleport_coords_list[j].map_name), file_handle);
				filesystem->Write((void *) &(ps_list[i]->teleport_coords_list[j].coords.x), sizeof(float), file_handle);
				filesystem->Write((void *) &(ps_list[i]->teleport_coords_list[j].coords.y), sizeof(float), file_handle);
				filesystem->Write((void *) &(ps_list[i]->teleport_coords_list[j].coords.z), sizeof(float), file_handle);
			}
		}
	}

	Msg("Wrote %i player settings to %s\n", ps_list_size, filename);
	filesystem->Close(file_handle);
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

	if (player_settings_list_size == 0)
	{
		Q_strcpy(ps_filename, "mani_player_settings.dat");

		for (;;)
		{
			Q_strcpy(ps_filename, "mani_player_settings.dat");

			Msg("Attempting to read %s file\n", ps_filename);

			//Get settings into memory
			Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), ps_filename);
			file_handle = filesystem->Open (base_filename,"rb",NULL);
			if (file_handle == NULL)
			{
				Msg ("Failed to load %s, did not find file\n", ps_filename);
				break;
			}

			if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
			{
				Msg("Failed to get version string for %s!!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			if (!ParseLine(stats_version, true))
			{
				Msg("Failed to get version string for %s, top line empty !!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			Msg("%s version [%s]\n", ps_filename, stats_version);

			player_settings_t ps;

			Q_memset(&ps, 0, sizeof(player_settings_t));

			switch (DeriveVersion(stats_version))
			{
			case 0:
				{
					// Get player settings into	memory
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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

			Msg("Read %i player settings into memory from file %s\n", player_settings_list_size, ps_filename);
			filesystem->Close(file_handle);
			break;
		}
	}

	if (player_settings_name_list_size == 0)
	{
		Q_strcpy(ps_filename, "mani_player_name_settings.dat");

		for (;;)
		{
			Msg("Attempting to read %s file\n", ps_filename);

			//Get settings into memory
			Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), ps_filename);
			file_handle = filesystem->Open (base_filename,"rb",NULL);
			if (file_handle == NULL)
			{
				Msg ("Failed to load %s, did not find file\n", ps_filename);
				break;
			}

			if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
			{
				Msg("Failed to get version string for %s!!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			if (!ParseLine(stats_version, true))
			{
				Msg("Failed to get version string for %s, top line empty !!\n", ps_filename);
				filesystem->Close(file_handle);
				break;
			}

			Msg("%s version [%s]\n", ps_filename, stats_version);

			player_settings_t ps;
			Q_memset(&ps, 0, sizeof(player_settings_t));

			switch (DeriveVersion(stats_version))
			{
			case 0:
				{
					// Get player settings into	memory
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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
					while (filesystem->Read(&(ps.steam_id),	sizeof(ps.steam_id), file_handle) >	0)
					{
						filesystem->Read(&(ps.name), sizeof(ps.name), file_handle);
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
								tc.coords.y	= x;
								tc.coords.z	= x;
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

			Msg("Read %i player name settings into memory from file %s\n", player_settings_name_list_size, ps_filename);
			filesystem->Close(file_handle);
			break;
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
	else
	{
		return 3;
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
void	ShowSettingsPrimaryMenu(player_t *player)
{
	/* Draw Main Menu */
	int	 range = 0;
	int  team_index = 0;

	if (war_mode) return;

	player_settings_t *player_settings = FindPlayerSettings(player);
	if (player_settings == NULL) return;

	FreeMenu();

	if (mani_show_victim_stats.GetInt() != 0)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size);
		if (player_settings->damage_stats == 0) Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Damage Settings : Off");
		else if (player_settings->damage_stats == 1) Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Damage Settings : Partial text");
		else Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Damage Settings : Full text");

		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings damagetype");
	}

	if (mani_quake_sounds.GetInt() != 0)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Quake Sounds : %s",
											(player_settings->quake_sounds == 0) ? "Off":"On");
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings quake");
	}

	if (mani_show_death_beams.GetInt() != 0 && gpManiGameType->IsDeathBeamAllowed())
	{
//		char	beam_mode[32];

//		if (player_settings->show_death_beam == 0) Q_snprintf(beam_mode, sizeof(beam_mode), "Off");
//		else if (player_settings->show_death_beam
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Death Beam : %s",
											(player_settings->show_death_beam == 0) ? "Off":"On");
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings deathbeam");
	}

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "System Sounds : %s",
											(player_settings->server_sounds == 0) ? "Off":"On");
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings sounds");

	if (mani_skins_admin.GetInt() != 0)
	{
		int admin_index;
		if (gpManiClient->IsAdmin(player, &admin_index))
		{
			if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
			{
				team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);
				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Admin %s Model : %s",
					Translate(gpManiGameType->GetTeamShortTranslation(team_index)),
					(!FStrEq(player_settings->admin_t_model,"")) ? player_settings->admin_t_model:"None");
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings admin_t");

				if (gpManiGameType->IsTeamPlayAllowed())
				{
					AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
					Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Admin %s Model : %s",
						Translate(gpManiGameType->GetTeamShortTranslation(TEAM_B)),
						(!FStrEq(player_settings->admin_ct_model,"")) ? player_settings->admin_ct_model:"None");
					Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings admin_ct");
				}
			}
		}
	}

	if (mani_skins_reserved.GetInt() != 0)
	{
		int immunity_index;
		if (gpManiClient->IsImmune(player, &immunity_index))
		{
			if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
			{
				team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);
				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Reserved %s Model : %s",
					Translate(gpManiGameType->GetTeamShortTranslation(team_index)),
					(!FStrEq(player_settings->immunity_t_model,"")) ? player_settings->immunity_t_model:"None");
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings immunity_t");

				if (gpManiGameType->IsTeamPlayAllowed())
				{
					AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
					Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Reserved %s Model : %s",
						Translate(gpManiGameType->GetTeamShortTranslation(TEAM_B)),
						(!FStrEq(player_settings->immunity_ct_model,"")) ? player_settings->immunity_ct_model:"None");
					Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings immunity_ct");
				}
			}
		}
	}

	if (mani_skins_public.GetInt() != 0)
	{
		team_index = (gpManiGameType->IsTeamPlayAllowed() ? TEAM_A:0);

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s Model : %s",
				Translate(gpManiGameType->GetTeamShortTranslation(team_index)),
				(!FStrEq(player_settings->t_model,"")) ? player_settings->t_model:"None");
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings public_t");

		if (gpManiGameType->IsTeamPlayAllowed())
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s Model : %s",
						Translate(gpManiGameType->GetTeamShortTranslation(TEAM_B)),
						(!FStrEq(player_settings->ct_model,"")) ? player_settings->ct_model:"None");
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings public_ct");
		}
	}

	if (menu_list_size == 0) return;
	DrawStandardMenu(player, "Press Esc to alter settings", "Configure your settings", true);
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
PLUGIN_RESULT ProcessSettingsMenu( edict_t *pEntity)
{
	player_t player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return PLUGIN_STOP;
	}

	if (1 == engine->Cmd_Argc())
	{
		// User typed manisettings at console
		ShowSettingsPrimaryMenu(&player);
		return PLUGIN_STOP;
	}

	if (engine->Cmd_Argc() > 1) 
	{
		const char *temp_command = engine->Cmd_Argv(1);
		int next_index = 0;
		int argv_offset = 0;

		if (FStrEq (temp_command, "more"))
		{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
		}

		char *menu_command = engine->Cmd_Argv(1 + argv_offset);

		if (FStrEq (menu_command, "damagetype") && 
			mani_show_victim_stats.GetInt() != 0)
		{
			ProcessMaDamage (player.index);
			ShowSettingsPrimaryMenu(&player);
			return PLUGIN_STOP;
		}
		else if (FStrEq (menu_command, "quake") &&
			mani_quake_sounds.GetInt() != 0)
		{
			ProcessMaQuake (player.index);
			ShowSettingsPrimaryMenu(&player);
			return PLUGIN_STOP;
		}		
		else if (FStrEq (menu_command, "sounds"))
		{
			ProcessMaSounds (player.index);
			ShowSettingsPrimaryMenu(&player);
			return PLUGIN_STOP;
		}		
		else if (FStrEq (menu_command, "deathbeam"))
		{
			ProcessMaDeathBeam (player.index);
			ShowSettingsPrimaryMenu(&player);
			return PLUGIN_STOP;
		}		
		else if (FStrEq (menu_command, "joinskin"))
		{
			// Handle skin choice menu
			ProcessJoinSkinChoiceMenu (&player, next_index, argv_offset, menu_command);
			return PLUGIN_STOP;
		}
		else if (FStrEq (menu_command, "admin_t") && mani_skins_admin.GetInt() != 0)
		{
			int admin_index;
			if (gpManiClient->IsAdmin(&player, &admin_index))
			{
				if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
				{
					// Handle skin choice menu
					if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_ADMIN_T_SKIN, menu_command))
					{
						ShowSettingsPrimaryMenu(&player);
					}
					return PLUGIN_STOP;
				}
			}
		}
	
		else if (FStrEq (menu_command, "admin_ct") && mani_skins_admin.GetInt() != 0 && gpManiGameType->IsTeamPlayAllowed())
		{
			int admin_index;
			if (gpManiClient->IsAdmin(&player, &admin_index))
			{
				if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
				{
					// Handle skin choice menu
					if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_ADMIN_CT_SKIN, menu_command))
					{
						ShowSettingsPrimaryMenu(&player);
					}
					return PLUGIN_STOP;
				}
			}
		}
		else if (FStrEq (menu_command, "immunity_t") && mani_skins_reserved.GetInt() != 0)
		{
			int immunity_index;
			if (gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
				{
					// Handle skin choice menu
					if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_RESERVE_T_SKIN, menu_command))
					{
						ShowSettingsPrimaryMenu(&player);
					}
					return PLUGIN_STOP;
				}
			}
		}	
		else if (FStrEq (menu_command, "immunity_ct") && mani_skins_reserved.GetInt() != 0 && gpManiGameType->IsTeamPlayAllowed())
		{
			int immunity_index;
			if (gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
				{
					// Handle skin choice menu
					if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_RESERVE_CT_SKIN, menu_command))
					{
						ShowSettingsPrimaryMenu(&player);
					}
					return PLUGIN_STOP;
				}
			}
		}		else if (FStrEq (menu_command, "public_t") && mani_skins_public.GetInt() != 0)
		{
			// Handle skin choice menu
			if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_T_SKIN, menu_command))
			{
				ShowSettingsPrimaryMenu(&player);
			}
			return PLUGIN_STOP;
		}
		else if (FStrEq (menu_command, "public_ct") && mani_skins_public.GetInt() != 0 && gpManiGameType->IsTeamPlayAllowed())
		{
			// Handle skin choice menu
			if (ProcessSkinChoiceMenu (&player, next_index, argv_offset, MANI_CT_SKIN, menu_command))
			{
				ShowSettingsPrimaryMenu(&player);
			}
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
static
bool ProcessSkinChoiceMenu
( 
  player_t *player_ptr, 
  int next_index, 
  int argv_offset, 
  int skin_type, 
  char *menu_command 
)
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	skin_name[20];
		int index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));

		if (index == 0) return true;
		if (index > skin_list_size && index != 999) return true;
		if (index != 999) index --;

		if (index == 999 || skin_list[index].skin_type == skin_type)
		{
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
			player_settings = FindStoredPlayerSettings(player_ptr);
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

				UpdatePlayerSettings(player_ptr, player_settings);
				current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;
			}
		}

		return true;
	}
	else
	{

		// Setup player list
		FreeMenu();

		if (mani_skins_force_public.GetInt() == 0 || skin_type == MANI_ADMIN_T_SKIN || skin_type == MANI_ADMIN_CT_SKIN
			|| skin_type == MANI_RESERVE_T_SKIN || skin_type == MANI_RESERVE_CT_SKIN)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "None");							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s 999", menu_command);
		}

		for( int i = 0; i < skin_list_size; i++ )
		{
			if (skin_list[i].skin_type != skin_type) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", skin_list[i].skin_name);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s %i", menu_command, i + 1);
		}

		if (menu_list_size == 0)
		{
			return true;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player_ptr, "Press Esc to choose skin", "Choose your skin", next_index, "manisettings", menu_command, true, -1);
	}


	return false;
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

	player_settings = FindStoredPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (player_settings->damage_stats == 0)
		{
			SayToPlayer(&player, "Partial round based stats will be shown on your screen when you die or at the end of a round");
			player_settings->damage_stats = 1;
		}
		else if (player_settings->damage_stats == 1)
		{
			SayToPlayer(&player, "Full round based stats with hit groups are now enabled");
			player_settings->damage_stats = 2;
		}
		else
		{
			SayToPlayer(&player, "Round based stats have been turned off");
			player_settings->damage_stats = 0;
		}

		UpdatePlayerSettings(&player, player_settings);
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

	player_settings = FindStoredPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->server_sounds)
		{
			SayToPlayer(&player, "You will now be able to hear plugin triggered sounds, type 'sounds' again to turn them off");
			player_settings->server_sounds = 1;
		}
		else
		{
			SayToPlayer(&player, "You will not be able to hear plugin triggered sounds, type 'sounds' again to turn them on");
			player_settings->server_sounds = 0;
		}

		UpdatePlayerSettings(&player, player_settings);
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

	player_settings = FindStoredPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->show_death_beam)
		{
			SayToPlayer(&player, "You will now be able to see the point of origin of your death");
			player_settings->show_death_beam = 1;
		}
		else
		{
			SayToPlayer(&player, "You will not be able to see the point of origin of your death");
			player_settings->show_death_beam = 0;
		}

		UpdatePlayerSettings(&player, player_settings);
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

	player_settings = FindStoredPlayerSettings(&player);
	if (player_settings)
	{
		// Found player settings
		if (!player_settings->quake_sounds)
		{
			SayToPlayer(&player, "Quake style sounds enabled, type 'quake' again to turn them off");
			player_settings->quake_sounds = 1;
		}
		else
		{
			SayToPlayer(&player, "Quake style sounds disabled, type 'quake' again to turn them on");
			player_settings->quake_sounds = 0;
		}

		UpdatePlayerSettings(&player, player_settings);
	}

	return PLUGIN_STOP;
}

