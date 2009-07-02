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
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_parser.h"
#include "mani_main.h"
#include "mani_mysql.h"
#include "mani_database.h"
#include "mani_language.h"
#include "mani_keyvalues.h"
#include "mani_commands.h"
#include "mani_help.h"
#include "mani_client.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	int		con_command_index;
extern	bool	war_mode;
extern	int	max_players;

static int sort_admin_level ( const void *m1,  const void *m2); 
static int sort_immunity_level ( const void *m1,  const void *m2); 
static int sort_player_admin_level ( const void *m1,  const void *m2); 

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}


ManiClient::ManiClient()
{
	// Init
	client_list = NULL;
	client_list_size = 0;
	admin_group_list = NULL;
	admin_group_list_size = 0;
	immunity_group_list = NULL;
	immunity_group_list_size = 0;
	admin_level_list = NULL;
	admin_level_list_size = 0;
	immunity_level_list = NULL;
	immunity_level_list_size = 0;

	this->InitAdminFlags();
	this->InitImmunityFlags();
	gpManiClient = this;
}

ManiClient::~ManiClient()
{
	// Cleanup
	FreeClients();
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiClient::NetworkIDValidated(player_t *player_ptr)
{
	int	index;

	active_admin_list[player_ptr->index - 1] = -1;
	active_immunity_list[player_ptr->index - 1] = -1;
	if (player_ptr->is_bot) return;

	if (IsPotentialAdmin(player_ptr, &index))
	{
		// Check levels
		if (active_admin_list[player_ptr->index - 1] == -1)
		{
			active_admin_list[player_ptr->index - 1] = index;
			ComputeAdminLevels();
		}
	}

	if (IsPotentialImmune(player_ptr, &index))
	{
		// Check levels
		if (active_immunity_list[player_ptr->index - 1] == -1)
		{
			active_immunity_list[player_ptr->index - 1] = index;
			ComputeImmunityLevels();
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Check if client is officially labelled as admin
//---------------------------------------------------------------------------------
void	ManiClient::ClientDisconnect(player_t *player_ptr)
{

	if (active_admin_list[player_ptr->index - 1] > -1)
	{
		active_admin_list[player_ptr->index - 1] = -1;
		ComputeAdminLevels();
	}

	if (active_immunity_list[player_ptr->index - 1] > -1)
	{
		active_immunity_list[player_ptr->index - 1] = -1;
		ComputeImmunityLevels();
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Re-calculate admin level flags
//---------------------------------------------------------------------------------
void	ManiClient::ComputeAdminLevels(void)
{
	player_level_t	*player_list = NULL;
	player_t	player;
	int			player_list_size = 0;
	int			index;

	if (admin_level_list_size == 0) return;

	// Create a list of admin indexes that are on the server and
	// have level_ids
	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->IsHLTV()) continue;
				Q_strcpy(player.steam_id, playerinfo->GetNetworkIDString());
				if (FStrEq(player.steam_id, "BOT")) continue;
				player.index = i;
				Q_strcpy(player.name, playerinfo->GetName());
				player.entity = pEntity;
				player.is_bot = false;

				GetIPAddressFromPlayer(&player);

				if (IsPotentialAdmin(&player, &index))
				{
					if (client_list[index].admin_level_id != -1)
					{
						// Add list of admins on server that 
						AddToList((void **) &player_list, sizeof(player_level_t), &player_list_size);
						player_list[player_list_size - 1].client_index = index;
						player_list[player_list_size - 1].level_id = client_list[index].admin_level_id;
						for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
						{
							client_list[index].admin_flags[j] = client_list[index].original_admin_flags[j];
							if (client_list[index].grouped_admin_flags[j])
							{
								client_list[index].admin_flags[j] = true;
							}
						}
					}
				}
			}
		}
	}

	if (player_list_size == 0) return;

	// Sort by level id for faster processing
	qsort(player_list, player_list_size, sizeof(player_level_t), sort_player_admin_level);

	int	min_level = player_list[0].level_id;

	// Remove flags for different levelled players on the server
	for (int j = 0; j < player_list_size; j++)
	{
		int client_index = player_list[j].client_index;

		for (int i = 0; i < admin_level_list_size; i ++)
		{
			if (admin_level_list[i].level_id >= min_level &&
				player_list[j].level_id > admin_level_list[i].level_id)
			{
				// Player here is at a lower level so apply filtering
				for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
				{
					if (admin_level_list[i].flags[k])
					{
						client_list[client_index].admin_flags[k] = false;
					}
				}
			}
		}
	}

	FreeList((void **) &player_list, &player_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Re-calculate immunity level flags
//---------------------------------------------------------------------------------
void	ManiClient::ComputeImmunityLevels(void)
{
	player_level_t	*player_list = NULL;
	player_t	player;
	int			player_list_size = 0;
	int			index;

	if (immunity_level_list_size == 0) return;

	// Create a list of client indexes that are on the server and
	// have level_ids
	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->IsHLTV()) continue;
				Q_strcpy(player.steam_id, playerinfo->GetNetworkIDString());
				if (FStrEq(player.steam_id, "BOT")) continue;

				player.index = i;
				Q_strcpy(player.name, playerinfo->GetName());
				player.entity = pEntity;
				player.is_bot = false;

				GetIPAddressFromPlayer(&player);

				if (IsPotentialImmune(&player, &index))
				{
					if (client_list[index].immunity_level_id != -1)
					{
						// Add list of clients on server
						AddToList((void **) &player_list, sizeof(player_level_t), &player_list_size);
						player_list[player_list_size - 1].client_index = index;
						player_list[player_list_size - 1].level_id = client_list[index].immunity_level_id;
						for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
						{
							client_list[index].immunity_flags[j] = client_list[index].original_immunity_flags[j];
							if (client_list[index].grouped_immunity_flags[j])
							{
								client_list[index].immunity_flags[j] = true;
							}
						}
					}
				}
			}
		}
	}

	if (player_list_size == 0) return;

	// Sort by level id for faster processing
	qsort(player_list, player_list_size, sizeof(player_level_t), sort_player_admin_level);

	int	min_level = player_list[0].level_id;

	// Remove flags for different levelled players on the server
	for (int j = 0; j < player_list_size; j++)
	{
		int client_index = player_list[j].client_index;

		for (int i = 0; i < immunity_level_list_size; i ++)
		{
			if (immunity_level_list[i].level_id >= min_level &&
				player_list[j].level_id > immunity_level_list[i].level_id)
			{
				// Player here is at a lower level so apply filtering
				for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
				{
					if (immunity_level_list[i].flags[k])
					{
						client_list[client_index].immunity_flags[k] = false;
					}
				}
			}
		}
	}

	FreeList((void **) &player_list, &player_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Check if client is officially labelled as admin
//---------------------------------------------------------------------------------
bool ManiClient::IsAdmin( player_t *player_ptr, int *admin_index)
{
	return IsAdminCore(player_ptr, admin_index, true);
}

//---------------------------------------------------------------------------------
// Purpose: Check if client is officially labelled as admin
//---------------------------------------------------------------------------------
bool ManiClient::IsAdminCore( player_t *player_ptr, int *client_index, bool recursive)
{
	*client_index = -1;

	if (player_ptr->is_bot)
	{
		return false;
	}

	const char *password  = engine->GetClientConVarValue(player_ptr->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].has_admin_potential)
		{
			continue;
		}

		// Check Steam ID
		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (client_list[i].steam_list[j].steam_id)
			{
				if (FStrEq(client_list[i].steam_list[j].steam_id, player_ptr->steam_id))
				{
					*client_index = i;
					if (recursive && active_admin_list[i] == -1)
					{
						active_admin_list[i] = i;
						ComputeAdminLevels();
						return IsAdmin(player_ptr, client_index);
					}

					return true;
				}
			}
		}

		// Check IP address
		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (client_list[i].ip_address_list[j].ip_address && player_ptr->ip_address)
			{
				if (FStrEq(client_list[i].ip_address_list[j].ip_address, player_ptr->ip_address)) 
				{
					*client_index = i;
					if (recursive && active_admin_list[i] == -1)
					{
						active_admin_list[i] = i;
						ComputeAdminLevels();
						return IsAdmin(player_ptr, client_index);
					}

					return true;
				}
			}
		}

		// Check name password combo
		if (client_list[i].nick_list_size != 0
			&& client_list[i].password)
		{
			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				if (strcmp(client_list[i].nick_list[j].nick, player_ptr->name) == 0)
				{
					if (strcmp(client_list[i].password, password) == 0)
					{
						*client_index = i;
						if (recursive && active_admin_list[i] == -1)
						{
							active_admin_list[i] = i;
							ComputeAdminLevels();
							return IsAdmin(player_ptr, client_index);
						}

						return true;
					}
				}
			}
		}	
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if client is potenially labelled as admin
//---------------------------------------------------------------------------------
bool ManiClient::IsPotentialAdmin( player_t *player_ptr, int *client_index)
{
	if (player_ptr->is_bot)
	{
		return false;
	}

	*client_index = -1;

	const char *password  = engine->GetClientConVarValue(player_ptr->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].has_admin_potential)
		{
			continue;
		}

		// Check Steam ID
		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (client_list[i].steam_list[j].steam_id)
			{
				if (FStrEq(client_list[i].steam_list[j].steam_id, player_ptr->steam_id))
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check IP address
		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (client_list[i].ip_address_list[j].ip_address && player_ptr->ip_address)
			{
				if (FStrEq(client_list[i].ip_address_list[j].ip_address, player_ptr->ip_address)) 
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check name password combo
		if (client_list[i].nick_list_size != 0
			&& client_list[i].password)
		{
			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				if (strcmp(client_list[i].nick_list[j].nick, player_ptr->name) == 0)
				{
					if (strcmp(client_list[i].password, password) == 0)
					{
						*client_index = i;
						return true;
					}
				}
			}
		}	
	}

	return false;
}


//---------------------------------------------------------------------------------
// Purpose: Checks if player is immune
//---------------------------------------------------------------------------------
bool ManiClient::IsImmune(player_t *player_ptr, int *immunity_index)
{
	return IsImmuneCore(player_ptr, immunity_index, true);
}
//---------------------------------------------------------------------------------
// Purpose: Checks if player is immune
//---------------------------------------------------------------------------------
bool ManiClient::IsImmuneCore(player_t *player_ptr, int *client_index, bool recursive)
{
	*client_index = -1;

	if (player_ptr->is_bot)
	{
		return false;
	}

	const char *password  = engine->GetClientConVarValue(player_ptr->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].has_immunity_potential)
		{
			continue;
		}

		// Check Steam ID
		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (client_list[i].steam_list[j].steam_id)
			{
				if (FStrEq(client_list[i].steam_list[j].steam_id, player_ptr->steam_id))
				{
					*client_index = i;
					if (recursive && active_immunity_list[i] == -1)
					{
						active_immunity_list[i] = i;
						ComputeImmunityLevels();
						return IsImmune(player_ptr, client_index);
					}

					return true;
				}
			}
		}

		// Check IP address
		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (client_list[i].ip_address_list[j].ip_address && player_ptr->ip_address)
			{
				if (FStrEq(client_list[i].ip_address_list[j].ip_address, player_ptr->ip_address)) 
				{
					*client_index = i;
					if (recursive && active_immunity_list[i] == -1)
					{
						active_immunity_list[i] = i;
						ComputeImmunityLevels();
						return IsImmune(player_ptr, client_index);
					}

					return true;
				}
			}
		}

		// Check name password combo
		if (client_list[i].nick_list_size != 0 
			&& client_list[i].password)
		{
			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				if (strcmp(client_list[i].nick_list[j].nick, player_ptr->name) == 0)
				{
					if (strcmp(client_list[i].password, password) == 0)
					{
						*client_index = i;
						if (recursive && active_immunity_list[i] == -1)
						{
							active_immunity_list[i] = i;
							ComputeImmunityLevels();
							return IsImmune(player_ptr, client_index);
						}

						return true;
					}
				}
			}	
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if player is immune (password already passed through)
//---------------------------------------------------------------------------------
bool ManiClient::IsImmuneNoPlayer(player_t *player_ptr, int *client_index)
{
	*client_index = -1;

	if (player_ptr->is_bot)
	{
		return false;
	}

	//Search through admin list for match
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].has_immunity_potential)
		{
			continue;
		}

		// Check Steam ID
		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (client_list[i].steam_list[j].steam_id)
			{
				if (FStrEq(client_list[i].steam_list[j].steam_id, player_ptr->steam_id))
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check IP address
		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (client_list[i].ip_address_list[j].ip_address && player_ptr->ip_address)
			{
				if (FStrEq(client_list[i].ip_address_list[j].ip_address, player_ptr->ip_address)) 
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check name password combo
		if (client_list[i].nick_list_size != 0 
			&& client_list[i].password)
		{
			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				if (strcmp(client_list[i].nick_list[j].nick, player_ptr->name) == 0)
				{
					if (strcmp(client_list[i].password, player_ptr->password) == 0)
					{
						*client_index = i;
						return true;
					}
				}
			}	
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if player is immune
//---------------------------------------------------------------------------------
bool ManiClient::IsPotentialImmune(player_t *player_ptr, int *client_index)
{
	*client_index = -1;

	if (player_ptr->is_bot)
	{
		return false;
	}

	const char *password  = engine->GetClientConVarValue(player_ptr->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].has_immunity_potential)
		{
			continue;
		}

		// Check Steam ID
		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (client_list[i].steam_list[j].steam_id)
			{
				if (FStrEq(client_list[i].steam_list[j].steam_id, player_ptr->steam_id))
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check IP address
		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (client_list[i].ip_address_list[j].ip_address && player_ptr->ip_address)
			{
				if (FStrEq(client_list[i].ip_address_list[j].ip_address, player_ptr->ip_address)) 
				{
					*client_index = i;
					return true;
				}
			}
		}

		// Check name password combo
		if (client_list[i].nick_list_size != 0 
			&& client_list[i].password)
		{
			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				if (strcmp(client_list[i].nick_list[j].nick, player_ptr->name) == 0)
				{
					if (strcmp(client_list[i].password, password) == 0)
					{
						*client_index = i;
						return true;
					}
				}
			}	
		}
	}

	return false;
}
//---------------------------------------------------------------------------------
// Purpose: Parses the Admin config line setting flags
//---------------------------------------------------------------------------------

bool ManiClient::OldAddClient
(
 char *file_details,
 old_style_client_t	*client_ptr,
 bool	is_admin
 )
{
	char	steam_id[MAX_NETWORKID_LENGTH]="";
	char	ip_address[128]="";
	char	name[MAX_PLAYER_NAME_LENGTH]="";
	char	password[128]="";
	int	i,j;

	Q_memset(client_ptr, 0, sizeof(old_style_client_t));

	// Setup flags for individual access
	if (is_admin)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i ++)
		{
			if (mani_reverse_admin_flags.GetInt() == 1)
			{
				client_ptr->flags[i] = false;
			}
			else
			{
				client_ptr->flags[i] = true;
			}
		}

		client_ptr->flags[ALLOW_CLIENT_ADMIN] = true;
	}
	else
	{ 
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i ++)
		{
			if (mani_reverse_immunity_flags.GetInt() == 1)
			{
				client_ptr->flags[i] = true;
			}
			else
			{
				client_ptr->flags[i] = false;
			}
		}

		client_ptr->flags[IMMUNITY_ALLOW_BASIC_IMMUNITY] = true;
	}

	i = 0;

	if (file_details[i] != ';')
	{
		j = 0;
		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				steam_id[j] = '\0';
				Q_strcpy(client_ptr->steam_id, steam_id);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == '\t' ||
				file_details[i] == ';')
			{
				steam_id[j] = '\0';
				break;
			}

			steam_id[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->steam_id, steam_id);
	}

	MMsg("%s ", steam_id);

	if (file_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				ip_address[j] = '\0';
				Q_strcpy(client_ptr->ip_address, ip_address);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == ';' ||
				file_details[i] == '\t')
			{
				ip_address[j] = '\0';
				break;
			}

			ip_address[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->ip_address, ip_address);
	}

	MMsg("%s ", ip_address);

	if (file_details[i] == ';' && file_details[i + 1] == '\"')
	{
		i += 2;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				name[j] = '\0';
				Q_strcpy(client_ptr->name, name);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == '"')
			{
				i++;
				name[j] = '\0';
				break;
			}

			name[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->name, name);
	}

	MMsg("%s ", name);

	if (file_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				password[j] = '\0';
				if (!client_ptr->steam_id && !client_ptr->ip_address && !client_ptr->name)
				{
					return false;
				}

				Q_strcpy(client_ptr->password, password);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == '\t')
			{
				password[j] = '\0';
				break;
			}

			password[j] = file_details[i];
			i++;
			j++;
		}

		if (!client_ptr->steam_id && !client_ptr->ip_address && !client_ptr->name)
		{
			return false;
		}

		Q_strcpy(client_ptr->password, password);
	}

	MMsg("%s ", password);

	i++;

	while (file_details[i] == ' ' || file_details[i] == '\t')
	{
		i++;
	}

	char *flags_string = &(file_details[i]);
	bool found_group = false;

	if (is_admin)
	{
		for (int j = 0; j < admin_group_list_size; j ++)
		{
			if (FStrEq(admin_group_list[j].group_id, flags_string))
			{
				found_group = true;
				Q_strcpy (client_ptr->group_id, flags_string);
				break;
			}
		}			
	}
	else
	{
		for (int j = 0; j < immunity_group_list_size; j ++)
		{
			if (FStrEq(immunity_group_list[j].group_id, flags_string))
			{
				found_group = true;
				Q_strcpy (client_ptr->group_id, flags_string);
				break;
			}
		}			
	}

	if (found_group)
	{
		// Setup flags for individual access
		if (is_admin)
		{
			for (int i = 0; i < MAX_ADMIN_FLAGS; i ++)
			{
				client_ptr->flags[i] = false;
			}
		}
		else
		{ 
			for (int i = 0; i < MAX_IMMUNITY_FLAGS; i ++)
			{
				client_ptr->flags[i] = false;
			}
		}
	}
	else
	{
		while (file_details[i] != '\0')
		{
			if (is_admin)
			{
				for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
				{
					if (file_details[i] == admin_flag_list[k].flag[0])
					{
						if (mani_reverse_admin_flags.GetInt() == 1)
						{
							client_ptr->flags[k] = true;
						}
						else
						{	
							client_ptr->flags[k] = false;
						}

						break;
					}
				}
			}
			else
			{
				for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
				{
					if (file_details[i] == immunity_flag_list[k].flag[0])
					{
						if (mani_reverse_immunity_flags.GetInt() == 1)
						{
							client_ptr->flags[k] = false;
						}
						else
						{	
							client_ptr->flags[k] = true;
						}

						break;
					}
				}
			}

			i++;
		}
	}

	MMsg("\n");
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------
void ManiClient::OldAddAdminGroup(char *file_details)
{
	char	group_id[128]="";
	int	i,j;
	admin_group_t	admin_group;

	if (file_details[0] != '\"') return;

	if (!AddToList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size))
	{
		return;
	}

	for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
	{
		if (mani_reverse_admin_flags.GetInt() == 1)
		{
			admin_group.flags[k] = false;
		}
		else
		{
			admin_group.flags[k] = true;
		}
	}

	admin_group.flags[ALLOW_CLIENT_ADMIN] = true;

	Q_strcpy(admin_group.group_id, "");

	i = 1;
	j = 0;

	for (;;)
	{
		if (file_details[i] == '\0')
		{
			// No more data
			group_id[j] = '\0';
			Q_strcpy(admin_group.group_id, group_id);
			admin_group_list[admin_group_list_size - 1] = admin_group;
			return;
		}


		// If reached end quote
		if (file_details[i] == '\"')
		{
			group_id[j] = '\0';
			break;
		}

		group_id[j] = file_details[i];
		i++;
		j++;
	}

	Q_strcpy(admin_group.group_id, group_id);

	MMsg("%s ", group_id);

	i++;

	while (file_details[i] != '\0')
	{
		for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
		{
			if (file_details[i] == admin_flag_list[k].flag[0])
			{
				if (mani_reverse_admin_flags.GetInt() == 1)
				{
					admin_group.flags[k] = true;
					break;
				}
				else
				{
					admin_group.flags[k] = false;
					break;
				}
			}
		}

		i++;
	}

	MMsg("\n");
	admin_group_list[admin_group_list_size - 1] = admin_group;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity Group config line setting flags
//---------------------------------------------------------------------------------
void ManiClient::OldAddImmunityGroup(char *immunity_details)
{
	char	group_id[128]="";
	int	i,j;
	immunity_group_t	immunity_group;

	if (immunity_details[0] != '\"') return;

	if (!AddToList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size))
	{
		return;
	}

	for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
	{
		if (mani_reverse_immunity_flags.GetInt() == 1)
		{
			immunity_group.flags[k] = true;
		}
		else
		{
			immunity_group.flags[k] = false;
		}
	}

	immunity_group.flags[IMMUNITY_ALLOW_BASIC_IMMUNITY] = true;

	Q_strcpy(immunity_group.group_id, "");

	i = 1;
	j = 0;

	for (;;)
	{
		if (immunity_details[i] == '\0')
		{
			// No more data
			group_id[j] = '\0';
			Q_strcpy(immunity_group.group_id, group_id);
			immunity_group_list[immunity_group_list_size - 1] = immunity_group;
			return;
		}


		// If reached end quote
		if (immunity_details[i] == '\"')
		{
			group_id[j] = '\0';
			break;
		}

		group_id[j] = immunity_details[i];
		i++;
		j++;
	}

	Q_strcpy(immunity_group.group_id, group_id);

	MMsg("%s ", group_id);

	i++;

	while (immunity_details[i] != '\0')
	{
		for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
		{
			if (immunity_details[i] == immunity_flag_list[k].flag[0])
			{
				if (mani_reverse_immunity_flags.GetInt() == 1)
				{
					immunity_group.flags[k] = false;
					break;
				}
				else
				{
					immunity_group.flags[k] = true;
					break;
				}
			}

		}

		i++;
	}

	MMsg("\n");
	immunity_group_list[immunity_group_list_size - 1] = immunity_group;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------

bool	ManiClient::Init(void)
{
	// Setup the flags
	FreeClients();
	if (LoadOldStyle())
	{
		// Loaded from old style adminlist.txt etc so write in new file format
		WriteClients();
		if (gpManiDatabase->GetDBEnabled())
		{
			if (this->CreateDBTables())
			{
				if (this->CreateDBFlags())
				{
					this->ExportDataToDB();
				}
			}
		}
	}

	FreeClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		if (!this->GetClientsFromDatabase())
		{
			FreeClients();
			LoadClients();
		}
		else
		{
			WriteClients();
		}
	}
	else
	{
		// Load up new style clients
		LoadClients();
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the old style admin flags
//---------------------------------------------------------------------------------
bool	ManiClient::LoadOldStyle(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	old_base_filename[512];
	char	data_in[1024];
	bool	loaded_old_style = false;


	//Get admin groups list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/admingroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style admingroups.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Admin Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			OldAddAdminGroup(data_in);
		}

		filesystem->Close(file_handle);

		Q_snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	//Get immunity groups list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitygroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style immunitygroups.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Immunity Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			OldAddImmunityGroup(data_in);
		}

		filesystem->Close(file_handle);

		Q_snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	//Get admin list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/adminlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style adminlist.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Admin steam id list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			old_style_client_t	temp_client;

			if (!OldAddClient(data_in, &temp_client, true)) continue;

			// Client data is okay, add it to client list
			ConvertOldClientToNewClient(&temp_client, true);
		}

		filesystem->Close(file_handle);
		Q_snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);

		loaded_old_style = true;
	}

	//Get immunity list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitylist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style immunitylist.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Immunity list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			old_style_client_t	temp_client;

			if (!OldAddClient(data_in, &temp_client, false)) continue;

			// Client data is okay, add it to client list
			ConvertOldClientToNewClient(&temp_client, false);
		}

		filesystem->Close(file_handle);

		Q_snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	// We have to botch in player names here that need to be unqiue
	for (int i = 0; i < client_list_size; i++)
	{
		// No name set up
		if (gpManiDatabase->GetDBEnabled())
		{
			/* Need unique name !! */
			Q_snprintf(client_list[i].name, sizeof(client_list[i].name), "Client_%i_%s", i+1, 
				gpManiDatabase->GetServerGroupID());
		}
		else
		{
			Q_snprintf(client_list[i].name, sizeof(client_list[i].name), "Client_%i", i+1);
		}
	}

	// Put into place the admin group flags for the clients
	for (int i = 0; i < client_list_size; i++)
	{
		client_list[i].admin_level_id = -1;
		client_list[i].immunity_level_id = -1;

		RebuildFlags(&(client_list[i]), MANI_ADMIN_TYPE);
		RebuildFlags(&(client_list[i]), MANI_IMMUNITY_TYPE);
	}

	return (loaded_old_style);
}

//---------------------------------------------------------------------------------
// Purpose: Add the client to an already existing entry or create a new one
//---------------------------------------------------------------------------------
void	ManiClient::RebuildFlags
(
 client_t	*client_ptr,
 int		type
 )
{

	if (MANI_ADMIN_TYPE == type)
	{
		client_ptr->has_admin_potential = false;

		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			client_ptr->admin_flags[i] = client_ptr->original_admin_flags[i];
			if (client_ptr->admin_flags[i]) client_ptr->has_admin_potential = true;
			client_ptr->grouped_admin_flags[i] = false;
		}

		for (int j = 0; j < client_ptr->admin_group_list_size; j++)
		{
			for (int k = 0; k < admin_group_list_size; k ++)
			{
				if (FStruEq(client_ptr->admin_group_list[j].group_id, admin_group_list[k].group_id))
				{
					// Group matches so setup flags for group for this admin
					for (int l = 0; l < MAX_ADMIN_FLAGS; l ++)
					{
						if (admin_group_list[k].flags[l])
						{
							client_ptr->admin_flags[l] = true;
							client_ptr->grouped_admin_flags[l] = true;
							client_ptr->has_admin_potential = true;
						}
					}
				}
			}
		}
	}
	else
	{
		client_ptr->has_immunity_potential = false;

		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			client_ptr->immunity_flags[i] = client_ptr->original_immunity_flags[i];
			if (client_ptr->immunity_flags[i]) client_ptr->has_immunity_potential = true;
			client_ptr->grouped_immunity_flags[i] = false;
		}

		for (int j = 0; j < client_ptr->immunity_group_list_size; j++)
		{
			for (int k = 0; k < immunity_group_list_size; k ++)
			{
				if (FStruEq(client_ptr->immunity_group_list[j].group_id, immunity_group_list[k].group_id))
				{
					// Group matches so setup flags for group for this admin
					for (int l = 0; l < MAX_IMMUNITY_FLAGS; l ++)
					{
						if (immunity_group_list[k].flags[l])
						{
							client_ptr->immunity_flags[l] = true;
							client_ptr->grouped_immunity_flags[l] = true;
							client_ptr->has_immunity_potential = true;
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Add the client to an already existing entry or create a new one
//---------------------------------------------------------------------------------
void	ManiClient::ConvertOldClientToNewClient
(
 old_style_client_t	*old_client_ptr,
 bool	is_admin
 )
{
	client_t	*client_ptr;
	int			client_index = -1;
	bool		by_steam = false;
	bool		by_ip = false;
	bool		by_name = false;

	// Find existing client record
	client_index = FindClientIndex(old_client_ptr->steam_id, true, true);
	if (client_index == -1)
	{
		client_index = FindClientIndex(old_client_ptr->ip_address, true, true);
		if (client_index == -1)
		{
			client_index = FindClientIndex(old_client_ptr->name, true, true);
			if (client_index != -1)
			{
				by_name = true;
			}
		}
		else
		{
			by_ip = true;
		}
	}
	else
	{
		by_steam = true;
	}

	if (client_index == -1)
	{
		// Create new client record as we didn't find one
		AddToList((void **) &client_list, sizeof(client_t), &client_list_size);
		client_ptr = &(client_list[client_list_size - 1]);
		Q_memset (client_ptr, 0, sizeof(client_t));
		MMsg("Adding client *********\n");
	}
	else
	{
		// We found a client so point there
		client_ptr = &(client_list[client_index]);
		MMsg("Found client *********\n");
	}

	if (is_admin)
	{
		client_ptr->original_admin_flags[ALLOW_BASIC_ADMIN] = true;
	}
	else
	{
		client_ptr->original_immunity_flags[IMMUNITY_ALLOW_BASIC_IMMUNITY] = true;
	}

	// Copy core information about player
	if (old_client_ptr->steam_id && !FStrEq(old_client_ptr->steam_id,""))
	{
		if (!by_steam)
		{
			AddToList((void **) &(client_ptr->steam_list), sizeof(steam_t), &(client_ptr->steam_list_size));
			Q_strcpy(client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id, old_client_ptr->steam_id);
		}
	}

	if (old_client_ptr->ip_address && !FStrEq(old_client_ptr->ip_address,""))
	{
		if (!by_ip)
		{
			AddToList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &(client_ptr->ip_address_list_size));
			Q_strcpy(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address, old_client_ptr->ip_address);
		}
	}

	if (old_client_ptr->name && !FStrEq(old_client_ptr->name,""))
	{
		if (!by_name)
		{
			AddToList((void **) &(client_ptr->nick_list), sizeof(nick_t), &(client_ptr->nick_list_size));
			Q_strcpy(client_ptr->nick_list[client_ptr->nick_list_size - 1].nick, old_client_ptr->name);
		}
	}

	if (old_client_ptr->password && !FStrEq(old_client_ptr->password,""))
	{
		Q_strcpy(client_ptr->password, old_client_ptr->password);
	}


	if (old_client_ptr->group_id && !FStrEq(old_client_ptr->group_id, ""))
	{
		if (is_admin)
		{
			AddToList((void **) &(client_ptr->admin_group_list), sizeof(admin_group_t), &(client_ptr->admin_group_list_size));
			Q_strcpy(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id, old_client_ptr->group_id);
		}
		else
		{
			AddToList((void **) &(client_ptr->immunity_group_list), sizeof(immunity_group_t), &(client_ptr->immunity_group_list_size));
			Q_strcpy(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id, old_client_ptr->group_id);
		}
	}


	// Handle flags for client
	if (is_admin)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			client_ptr->original_admin_flags[i] = old_client_ptr->flags[i];
			client_ptr->admin_flags[i] = old_client_ptr->flags[i];
		}
	}
	else
	{
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			client_ptr->original_immunity_flags[i] = old_client_ptr->flags[i];
			client_ptr->admin_flags[i] = old_client_ptr->flags[i];
		}
	}

}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin flags
//---------------------------------------------------------------------------------
void	ManiClient::LoadClients(void)
{
	char	core_filename[256];
	bool found_match;
	KeyValues *base_key_ptr;

//	MMsg("*********** Loading admin section of clients.txt ************\n");
	// Read the clients.txt file

	KeyValues *kv_ptr = new KeyValues("clients.txt");

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/clients.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		MMsg("Failed to load clients.txt\n");
		kv_ptr->deleteThis();
		return;
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our groups first
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "admingroups"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the groups in
		GetAdminGroups(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our immunity groups
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "immunitygroups"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the groups in
		GetImmunityGroups(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our levels next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "adminlevels"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the levels in
		GetAdminLevels(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our levels next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "immunitylevels"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the levels in
		GetImmunityLevels(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our clients next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "players"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the clients in
		GetClients(base_key_ptr);
	}

	ComputeAdminLevels();
	ComputeImmunityLevels();
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin groups
//---------------------------------------------------------------------------------
void	ManiClient::GetAdminGroups(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	char	group_name[128];
	admin_group_t	admin_group;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&admin_group, 0, sizeof(admin_group_t));

		Q_strcpy(group_name, kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString(NULL, "NULL"));
		Q_strcpy(admin_group.group_id, group_name);

		if (!FStrEq("NULL", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_ADMIN_TYPE);
				if (flag_index != -1)
				{
					// Valid flag index given
					admin_group.flags[flag_index] = true;
				}
			}

			AddToList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size);
			admin_group_list[admin_group_list_size - 1] = admin_group;
//			MMsg("Admin Group [%s]\n", admin_group.group_id);
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style immunity groups
//---------------------------------------------------------------------------------
void	ManiClient::GetImmunityGroups(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	char	group_name[128];
	immunity_group_t	immunity_group;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&immunity_group, 0, sizeof(immunity_group_t));

		Q_strcpy(group_name, kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString(NULL, "NULL"));
		Q_strcpy(immunity_group.group_id, group_name);

		if (!FStrEq("NULL", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_IMMUNITY_TYPE);
				if (flag_index != -1)
				{
					// Valid flag index given
					immunity_group.flags[flag_index] = true;
				}
			}

			AddToList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size);
			immunity_group_list[immunity_group_list_size - 1] = immunity_group;
//			MMsg("Immunity Group [%s]\n", immunity_group.group_id);
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin levels
//---------------------------------------------------------------------------------
void	ManiClient::GetAdminLevels(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	admin_level_t	admin_level;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&admin_level, 0, sizeof(admin_level_t));

		admin_level.level_id = Q_atoi(kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString(NULL, "NULL"));

		if (!FStrEq("NULL", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_ADMIN_TYPE);
				if (flag_index != -1)
				{
					// Valid flag index given
					admin_level.flags[flag_index] = true;
				}
			}

			AddToList((void **) &admin_level_list, sizeof(admin_level_t), &admin_level_list_size);
			admin_level_list[admin_level_list_size - 1] = admin_level;
//			MMsg("Admin Level [%i]\n", admin_level.level_id);
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}

	qsort(admin_level_list, admin_level_list_size, sizeof(admin_level_t), sort_admin_level); 

}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style immunity levels
//---------------------------------------------------------------------------------
void	ManiClient::GetImmunityLevels(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	immunity_level_t	immunity_level;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&immunity_level, 0, sizeof(immunity_level_t));
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i ++)
		{
			immunity_level.flags[i] = false;
		}

		immunity_level.level_id = Q_atoi(kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString(NULL, "NULL"));

		if (!FStrEq("NULL", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_IMMUNITY_TYPE);
				if (flag_index != -1)
				{
					// Valid flag index given
					immunity_level.flags[flag_index] = true;
				}
			}

			AddToList((void **) &immunity_level_list, sizeof(immunity_level_t), &immunity_level_list_size);
			immunity_level_list[immunity_level_list_size - 1] = immunity_level;
//			MMsg("Immunity Level [%i]\n", immunity_level.level_id);
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}

	qsort(immunity_level_list, immunity_level_list_size, sizeof(immunity_level_t), sort_immunity_level);

	/*	for (int i = 0; i < immunity_level_list_size; i++)
	{
	MMsg("Im Lev [%i]\n", immunity_level_list[i].level_id);
	for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
	{
	if (immunity_level_list[i].flags[j])
	{
	MMsg("%s ", immunity_flag_list[j].flag_desc);
	}
	}

	MMsg("\n");
	}*/
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style clients flags
//---------------------------------------------------------------------------------
void	ManiClient::GetClients(KeyValues *ptr)
{
	KeyValues *players_ptr;
	KeyValues *kv_ptr;
	KeyValues *temp_ptr;

	char	flag_string[4096];
	char	temp_string[256];
	client_t	*client_ptr;

	// Should be at key 'players'
	players_ptr = ptr->GetFirstSubKey();
	if (!players_ptr)
	{
		return;
	}

	for (;;)
	{
		AddToList((void **) &client_list, sizeof(client_t), &client_list_size);
		client_ptr = &(client_list[client_list_size - 1]);
		Q_memset (client_ptr, 0, sizeof(client_t));

		kv_ptr = players_ptr;

		// Do simple stuff first
		Q_strcpy(client_ptr->email_address, kv_ptr->GetString("email"));
		client_ptr->admin_level_id = kv_ptr->GetInt("admin_level", -1);
		client_ptr->immunity_level_id = kv_ptr->GetInt("immunity_level", -1);
		Q_strcpy(client_ptr->name, kv_ptr->GetString("name"));
		Q_strcpy(client_ptr->password, kv_ptr->GetString("password"));
		Q_strcpy(client_ptr->notes, kv_ptr->GetString("notes"));

		/* Handle single steam id */
		Q_strcpy(temp_string, kv_ptr->GetString("steam", ""));
		if (!FStrEq(temp_string, ""))
		{
			AddToList((void **) &(client_ptr->steam_list), sizeof(steam_t), &(client_ptr->steam_list_size));
			Q_strcpy(client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id, temp_string);
		}


		/* Handle single ip */
		Q_strcpy(temp_string, kv_ptr->GetString("ip", ""));
		if (!FStrEq(temp_string, ""))
		{
			AddToList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &(client_ptr->ip_address_list_size));
			Q_strcpy(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address, temp_string);
		}

		/* Handle single nickname */
		Q_strcpy(temp_string, kv_ptr->GetString("nick", ""));
		if (!FStrEq(temp_string, ""))
		{
			AddToList((void **) &(client_ptr->nick_list), sizeof(nick_t), &(client_ptr->nick_list_size));
			Q_strcpy(client_ptr->nick_list[client_ptr->nick_list_size - 1].nick, temp_string);
		}

		/* Handle single admin group */
		Q_strcpy(temp_string, kv_ptr->GetString("admingroups", ""));
		if (!FStrEq(temp_string, ""))
		{
			AddToList((void **) &(client_ptr->admin_group_list), sizeof(group_t), &(client_ptr->admin_group_list_size));
			Q_strcpy(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id, temp_string);
//			MMsg("Group ID [%s]\n", client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id);
		}

		/* Handle single immunity group */
		Q_strcpy(temp_string, kv_ptr->GetString("immunitygroups", ""));
		if (!FStrEq(temp_string, ""))
		{
			AddToList((void **) &(client_ptr->immunity_group_list), sizeof(group_t), &(client_ptr->immunity_group_list_size));
			Q_strcpy(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id, temp_string);
//			MMsg("Group ID [%s]\n", client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id);
		}

		// Steam IDs
		temp_ptr = kv_ptr->FindKey("steam", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					AddToList((void **) &(client_ptr->steam_list), sizeof(steam_t), &(client_ptr->steam_list_size));
					Q_strcpy(client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id, temp_ptr->GetString());
//					MMsg("Steam ID [%s]\n", client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id);
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// IP Addresses
		temp_ptr = kv_ptr->FindKey("ip", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					AddToList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &(client_ptr->ip_address_list_size));
					Q_strcpy(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address, temp_ptr->GetString());
//					MMsg("IP Address [%s]\n", client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address);
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Player nicknames
		temp_ptr = kv_ptr->FindKey("nick", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					AddToList((void **) &(client_ptr->nick_list), sizeof(nick_t), &(client_ptr->nick_list_size));
					Q_strcpy(client_ptr->nick_list[client_ptr->nick_list_size - 1].nick, temp_ptr->GetString());
//					MMsg("Nick Name [%s]\n", client_ptr->nick_list[client_ptr->nick_list_size - 1].nick);
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Admin Groups 
		temp_ptr = kv_ptr->FindKey("admingroups", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					AddToList((void **) &(client_ptr->admin_group_list), sizeof(group_t), &(client_ptr->admin_group_list_size));
					Q_strcpy(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id, temp_ptr->GetString());
//					MMsg("Group ID [%s]\n", client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id);
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Immunity Groups 
		temp_ptr = kv_ptr->FindKey("immunitygroups", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					AddToList((void **) &(client_ptr->immunity_group_list), sizeof(group_t), &(client_ptr->immunity_group_list_size));
					Q_strcpy(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id, temp_ptr->GetString());
//					MMsg("Group ID [%s]\n", client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id);
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// do the admin flags 
		temp_ptr = kv_ptr->FindKey("adminflags", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					Q_strcpy(flag_string, temp_ptr->GetString(NULL, "NULL"));
					if (!FStrEq("NULL", flag_string))
					{
						int flag_string_index = 0;

						while(flag_string[flag_string_index] != '\0')
						{
							int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_ADMIN_TYPE);
							if (flag_index != -1)
							{
								// Valid flag index given so set original flags
								// Actual flags are computed when a player leaves
								// or joins based on the levels setup and
								// who is on the server.
								client_ptr->original_admin_flags[flag_index] = true;
							}
						}
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Do the immunity flags
		temp_ptr = kv_ptr->FindKey("immunityflags", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					Q_strcpy(flag_string, temp_ptr->GetString(NULL, "NULL"));
					if (!FStrEq("NULL", flag_string))
					{
						int flag_string_index = 0;

						while(flag_string[flag_string_index] != '\0')
						{
							int flag_index = this->GetNextFlag(flag_string, &flag_string_index, MANI_IMMUNITY_TYPE);
							if (flag_index != -1)
							{
								// Valid flag index given so set original flags
								// Actual flags are computed when a player leaves
								// or joins based on the levels setup and
								// who is on the server.
								client_ptr->original_immunity_flags[flag_index] = true;
							}
						}
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		players_ptr = players_ptr->GetNextKey();
		if (!players_ptr)
		{
			break;
		}
	}

	// Put into place the admin group flags for the clients
	for (int i = 0; i < client_list_size; i++)
	{
		RebuildFlags(&(client_list[i]), MANI_ADMIN_TYPE);
		RebuildFlags(&(client_list[i]), MANI_IMMUNITY_TYPE);
	}

	ComputeAdminLevels();
	ComputeImmunityLevels();

//	DumpClientsToConsole();
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin flags
//---------------------------------------------------------------------------------
int		ManiClient::GetNextFlag(char *flags_ptr, int *index, int type)
{
	char	flag_name[4096];

	// Skip space seperators
	while (flags_ptr[*index] == ';' || flags_ptr[*index] == ' ' || flags_ptr[*index] == '\t')
	{
		*index = *index + 1;
	}

	if (flags_ptr[*index] == '\0') return -1;
	int	i = 0;

	while (flags_ptr[*index] != ' ' && 
		flags_ptr[*index] != ';' && 
		flags_ptr[*index] != '\t' && 
		flags_ptr[*index] != '\0')
	{
		flag_name[i++] = flags_ptr[*index];
		*index = *index + 1;
	}

	flag_name[i] = '\0';

	if (type == MANI_ADMIN_TYPE)
	{
		for (i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			if (FStruEq(flag_name, admin_flag_list[i].flag))
			{
				return i;
			}
		}
	}
	else
	{
		for (i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			if (FStruEq(flag_name, immunity_flag_list[i].flag))
			{
				return i;
			}
		}
	}

//	MMsg("Flag [%s] is invalid !!\n", flag_name);
	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Dumps the clients list to the console
//---------------------------------------------------------------------------------
void		ManiClient::DumpClientsToConsole(void)
{
	for (int i = 0; i < client_list_size; i ++)
	{
		MMsg("Name [%s]\n", client_list[i].name);
		MMsg("Email [%s]\n", client_list[i].email_address);
		MMsg("Admin Level [%i]\n", client_list[i].admin_level_id);
		MMsg("Immunity Level [%i]\n", client_list[i].immunity_level_id);

		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			MMsg("Steam : [%s]\n", client_list[i].steam_list[j].steam_id);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the clients.txt file based on data in memory
//---------------------------------------------------------------------------------
void		ManiClient::WriteClients(void)
{
	char temp_string[1024];
	char temp_string2[1024];

	char	core_filename[256];
	ManiKeyValues *client;
	bool	found_flag;

//	MMsg("*********** Writing clients.txt ************\n");

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/clients.txt", mani_path.GetString());

	client = new ManiKeyValues( "clients.txt" );

	if (!client->WriteStart(core_filename))
	{
		MMsg("Failed to write %s\n", core_filename);
		delete client;
		return;
	}

	client->WriteComment("This key group lists all your client players");
	client->WriteNewSubKey("players");

//	MMsg("Writing %i client(s)\n",  client_list_size);
	// Loop through all clients
	for (int i = 0; i < client_list_size; i ++)
	{
		if (!client_list[i].name || FStrEq(client_list[i].name,""))
		{
			// No name set up
			if (gpManiDatabase->GetDBEnabled())
			{
				/* Need unique name !! */
				Q_snprintf(temp_string, sizeof(temp_string), "Client_%i_%s", i+1, 
					gpManiDatabase->GetServerGroupID());
			}
			else
			{
				Q_snprintf(temp_string, sizeof(temp_string), "Client_%i", i+1);
			}

			Q_strcpy(client_list[i].name, temp_string);
		}
		else
		{
			Q_snprintf(temp_string, sizeof(temp_string), "%s", client_list[i].name);
		}

		client->WriteComment("This must be a unique client name");
		client->WriteNewSubKey(temp_string);

		if (client_list[i].email_address) client->WriteKey("email", client_list[i].email_address);
		if (client_list[i].admin_level_id != -1) client->WriteKey("admin_level", client_list[i].admin_level_id);
		if (client_list[i].immunity_level_id != -1) client->WriteKey("immunity_level", client_list[i].immunity_level_id);
		if (client_list[i].name)
		{
			client->WriteComment("Client real name");
			client->WriteKey("name", client_list[i].name);
		}
		if (client_list[i].password) client->WriteKey("password", client_list[i].password);
		if (client_list[i].notes) client->WriteKey("notes", client_list[i].notes);
		if (client_list[i].steam_list_size == 1) 
		{
			client->WriteComment("Steam ID for client");
			client->WriteKey("steam", client_list[i].steam_list[0].steam_id);
		}
		if (client_list[i].ip_address_list_size == 1) client->WriteKey("ip", client_list[i].ip_address_list[0].ip_address);
		if (client_list[i].nick_list_size == 1) client->WriteKey("nick", client_list[i].nick_list[0].nick);

		// Write Steam IDs if needed.
		if (client_list[i].steam_list_size > 1)
		{
			client->WriteComment("This sub key is for multiple steam ids");
			client->WriteNewSubKey("steam");

			for (int j = 0; j < client_list[i].steam_list_size; j++)
			{
				Q_snprintf(temp_string, sizeof(temp_string), "steam%i", j + 1);
				client->WriteKey(temp_string, client_list[i].steam_list[j].steam_id);
			}

			client->WriteEndSubKey();
		}

		// Write IP Addresses if needed.
		if (client_list[i].ip_address_list_size > 1)
		{
			client->WriteComment("This sub key is for multiple ip addresses");
			client->WriteNewSubKey("ip");

			for (int j = 0; j < client_list[i].steam_list_size; j++)
			{
				Q_snprintf(temp_string, sizeof(temp_string), "IP%i", j + 1);
				client->WriteKey(temp_string, client_list[i].ip_address_list[j].ip_address);
			}

			client->WriteEndSubKey();
		}

		// Write Player Nicknames if needed.
		if (client_list[i].nick_list_size > 1)
		{
			client->WriteComment("This sub key is for multiple player nick names");
			client->WriteNewSubKey("nick");

			for (int j = 0; j < client_list[i].nick_list_size; j++)
			{
				Q_snprintf(temp_string, sizeof(temp_string), "nick%i", j + 1);
				client->WriteKey(temp_string, client_list[i].nick_list[j].nick);
			}

			client->WriteEndSubKey();
		}

		// Write Admin flags if needed.
		found_flag = false;
		for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
		{
			if (client_list[i].original_admin_flags[j])
			{
				found_flag = true;
				break;
			}
		}

		if (found_flag)
		{
			client->WriteComment("These are personal admin flags for a client");
			client->WriteNewSubKey("adminflags");

			int flags_number = 1;
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_ADMIN_FLAGS; j++)
			{
				if (client_list[i].original_admin_flags[j])
				{
					Q_strcat(temp_string, admin_flag_list[j].flag);
					Q_strcat(temp_string, " "); // Seperator

					// Split up strings
					if (Q_strlen(temp_string) > 60)
					{
						Q_snprintf(temp_string2, sizeof(temp_string2), "flags%i", flags_number ++);
						temp_string[Q_strlen(temp_string) - 1] = '\0';
						client->WriteKey(temp_string2, temp_string);
						Q_strcpy(temp_string, "");
					}
				}
			}

			if (Q_strlen(temp_string) <= 60)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "flags%i", flags_number ++);
				if (!FStrEq(temp_string,""))
				{
					temp_string[Q_strlen(temp_string) - 1] = '\0';
				}
				client->WriteKey(temp_string2, temp_string);
				Q_strcpy(temp_string, "");
			}

			client->WriteEndSubKey();
		}

		if (client_list[i].admin_group_list_size == 1) 
		{
			client->WriteComment("This is the admin group the client is subscribed to");
			client->WriteKey("admingroups", client_list[i].admin_group_list[0].group_id);
		}

		// Write Admin Groups if needed
		if (client_list[i].admin_group_list_size > 1)
		{
			client->WriteComment("These are the admin groups this client is subscribed to");
			client->WriteNewSubKey("admingroups");

			for (int j = 0; j < client_list[i].admin_group_list_size; j++)
			{
				Q_snprintf(temp_string, sizeof(temp_string), "group%i", j + 1);
				client->WriteKey(temp_string, client_list[i].admin_group_list[j].group_id);
			}

			client->WriteEndSubKey();
		}

		// Write immunity flags if needed.
		found_flag = false;
		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
		{
			if (client_list[i].original_immunity_flags[j])
			{
				found_flag = true;
				break;
			}
		}

		if (found_flag)
		{
			client->WriteComment("These are personal immunity flags for a client");
			client->WriteNewSubKey("immunityflags");

			int flags_number = 1;
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_IMMUNITY_FLAGS; j++)
			{
				if (client_list[i].original_immunity_flags[j])
				{
					Q_strcat(temp_string, immunity_flag_list[j].flag);
					Q_strcat(temp_string, " "); // Seperator

					// Split up strings
					if (Q_strlen(temp_string) > 60)
					{
						Q_snprintf(temp_string2, sizeof(temp_string2), "flags%i", flags_number ++);
						temp_string[Q_strlen(temp_string) - 1] = '\0';
						client->WriteKey(temp_string2, temp_string);
						Q_strcpy(temp_string, "");
					}
				}
			}

			if (Q_strlen(temp_string) <= 60)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "flags%i", flags_number ++);
				if (!FStrEq(temp_string,""))
				{
					temp_string[Q_strlen(temp_string) - 1] = '\0';
				}
				client->WriteKey(temp_string2, temp_string);
				Q_strcpy(temp_string, "");
			}

			client->WriteEndSubKey();
		}
		
		if (client_list[i].immunity_group_list_size == 1) 
		{
			client->WriteComment("This is the immunity group the client is subscribed to");
			client->WriteKey("immunitygroups", client_list[i].immunity_group_list[0].group_id);
		}

		// Write Immunity Groups if needed
		if (client_list[i].immunity_group_list_size > 1)
		{
			client->WriteNewSubKey("immunitygroups");

			for (int j = 0; j < client_list[i].immunity_group_list_size; j++)
			{
				Q_snprintf(temp_string, sizeof(temp_string), "group%i", j + 1);
				client->WriteKey(temp_string, client_list[i].immunity_group_list[j].group_id);
			}

			client->WriteEndSubKey();
		}

		client->WriteEndSubKey();
	}

	// End Players
	client->WriteEndSubKey();

	if (admin_group_list_size != 0)
	{
		client->WriteComment("These are the admins groups a client can subscribed to");
		client->WriteNewSubKey("admingroups");

		for (int i = 0; i < admin_group_list_size; i ++)
		{
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
			{
				if (admin_group_list[i].flags[j])
				{
					// found flag
					Q_strcat(temp_string, admin_flag_list[j].flag);
					Q_strcat(temp_string, " ");
				}
			}

			if (!FStrEq(temp_string,""))
			{
				temp_string[Q_strlen(temp_string) - 1] = '\0';
			}
			client->WriteKey(admin_group_list[i].group_id, temp_string);
		}

		client->WriteEndSubKey();
	}

	if (immunity_group_list_size != 0)
	{
		client->WriteComment("These are the immunity groups a client can subscribed to");
		client->WriteNewSubKey("immunitygroups");

		for (int i = 0; i < immunity_group_list_size; i ++)
		{
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
			{
				if (immunity_group_list[i].flags[j])
				{
					// found flag
					Q_strcat(temp_string, immunity_flag_list[j].flag);
					Q_strcat(temp_string, " ");
				}
			}

			if (!FStrEq(temp_string,""))
			{
				temp_string[Q_strlen(temp_string) - 1] = '\0';
			}
			client->WriteKey(immunity_group_list[i].group_id, temp_string);
		}

		client->WriteEndSubKey();
	}

	if (admin_level_list_size != 0)
	{
		client->WriteComment("These are the admin level flag definitions that a client can subscribed to");
		client->WriteNewSubKey("adminlevels");

		for (int i = 0; i < admin_level_list_size; i ++)
		{
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
			{
				if (admin_level_list[i].flags[j])
				{
					// found flag
					Q_strcat(temp_string, admin_flag_list[j].flag);
					Q_strcat(temp_string, " ");
				}
			}

			if (!FStrEq(temp_string,""))
			{
				temp_string[Q_strlen(temp_string) - 1] = '\0';
			}
			Q_snprintf(temp_string2, sizeof(temp_string2), "%i", admin_level_list[i].level_id);
			client->WriteKey(temp_string2, temp_string);
		}

		client->WriteEndSubKey();
	}

	if (immunity_level_list_size != 0)
	{
		client->WriteComment("These are the immunity level flag definitions that a client can subscribed to");
		client->WriteNewSubKey("immunitylevels");

		for (int i = 0; i < immunity_level_list_size; i ++)
		{
			Q_strcpy(temp_string, "");

			for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
			{
				if (immunity_level_list[i].flags[j])
				{
					// found flag
					Q_strcat(temp_string, immunity_flag_list[j].flag);
					Q_strcat(temp_string, " ");
				}
			}

			if (!FStrEq(temp_string,""))
			{
				temp_string[Q_strlen(temp_string) - 1] = '\0';
			}
			Q_snprintf(temp_string2, sizeof(temp_string2), "%i", immunity_level_list[i].level_id);
			client->WriteKey(temp_string2, temp_string);
		}

		client->WriteEndSubKey();
	}

	client->WriteEnd();

	delete client;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Free's up single client memory allocation
//---------------------------------------------------------------------------------
void	ManiClient::FreeClient(client_t *client_ptr)
{
	if (client_ptr->steam_list_size)
	{
		free (client_ptr->steam_list);
		client_ptr->steam_list_size = 0;
		client_ptr->steam_list = NULL;
	}

	if (client_ptr->ip_address_list_size)
	{
		free (client_ptr->ip_address_list);
		client_ptr->ip_address_list_size = 0;
		client_ptr->ip_address_list = NULL;
	}

	if (client_ptr->nick_list_size)
	{
		free (client_ptr->nick_list);
		client_ptr->nick_list_size = 0;
		client_ptr->nick_list = NULL;
	}

	if (client_ptr->admin_group_list_size)
	{
		free (client_ptr->admin_group_list);
		client_ptr->admin_group_list_size = 0;
		client_ptr->admin_group_list = NULL;
	}

	if (client_ptr->immunity_group_list_size)
	{
		free (client_ptr->immunity_group_list);
		client_ptr->immunity_group_list_size = 0;
		client_ptr->immunity_group_list = NULL;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free's up admin list memory
//---------------------------------------------------------------------------------

void	ManiClient::FreeClients(void)
{
	for (int i = 0; i < client_list_size; i++)
	{
		FreeClient(&(client_list[i]));
	}

	FreeList((void **) &client_list, &client_list_size);
	FreeList((void **) &admin_group_list, &admin_group_list_size);
	FreeList((void **) &immunity_group_list, &immunity_group_list_size);
	FreeList((void **) &admin_level_list, &admin_level_list_size);
	FreeList((void **) &immunity_level_list, &immunity_level_list_size);

}

//---------------------------------------------------------------------------------
// Purpose: Checks if admin is elligible to run the command
//---------------------------------------------------------------------------------

bool	ManiClient::IsAdminAllowed(player_t *player, const char *command, int admin_flag, bool check_war, int *admin_index)
{
	*admin_index = -1;

	if (check_war)
	{
		OutputHelpText(ORANGE_CHAT, player, "Mani Admin Plugin: Command %s is disabled in war mode", command);
		return false;
	}

	if (!IsAdmin(player, admin_index) || check_war)
	{
		OutputHelpText(ORANGE_CHAT, player, "Mani Admin Plugin: You are not an admin");
		return false;
	}

	if (admin_flag != ADMIN_DONT_CARE)
	{
		if (!client_list[*admin_index].admin_flags[admin_flag])
		{
			OutputHelpText(ORANGE_CHAT, player, "Mani Admin Plugin: You are not authorised to run %s", command);
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Sets up admin flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
bool IsCommandIssuedByServerAdmin( void )
{
	if ( engine->IsDedicatedServer() && con_command_index > -1 )
		return false;

	if ( !engine->IsDedicatedServer() && con_command_index > 0 )
		return false;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Creates database tables
//---------------------------------------------------------------------------------
bool ManiClient::CreateDBTables(void)
{
	ManiMySQL *mani_mysql = new ManiMySQL();

	MMsg("Creating DB tables if not existing....\n");
	if (!mani_mysql->Init())
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL auto_increment, "
		"name varchar(32) NOT NULL, "
		"password varchar(32) default '', "
		"email varchar(255) default '', "
		"notes varchar(255) default '', "
		"PRIMARY KEY (user_id), "
		"UNIQUE KEY (name) "
		") TYPE=MyISAM AUTO_INCREMENT=1", 
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClient()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s( "
		"user_id mediumint(8) NOT NULL default '0', "
		"steam_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, steam_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBSteam()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"nick varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, nick) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBNick()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"ip_address varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, ip_address) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBIP()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %sflag\n", gpManiDatabase->GetDBTablePrefix());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %sflag ( "
		"flag_id varchar(20) BINARY NOT NULL default '', "
		"type varchar(32) NOT NULL default '', "
		"description varchar(128) NOT NULL default '', "
		"PRIMARY KEY (flag_id, type) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%sn", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"server_id mediumint(8) NOT NULL default '0', "
		"name varchar(128) NOT NULL default '', "
		"ip_address varchar(32) NOT NULL default '', "
		"port mediumint(8) NOT NULL default '0', "
		"mod_name varchar(64) NOT NULL default '', "
		"rcon_password varchar(64) default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (server_id), "
		"UNIQUE KEY (name) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBGroup());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"group_id varchar(32) NOT NULL default '', "
		"flag_string text, "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (group_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBGroup()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"group_id varchar(32) NOT NULL default '', "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, group_id, type, server_group_id) "
		") TYPE=MyISAM",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientGroup()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"flag_string text, "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientFlag()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s( "
		"user_id mediumint(8) NOT NULL default '0', "
		"level_id mediumint(8) NOT NULL default '-1', "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, level_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientLevel()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBLevel());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"level_id mediumint(8) NOT NULL default '-1', "
		"type varchar(32) NOT NULL default '', "
		"flag_string text, "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (level_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBLevel()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"server_group_id varchar(32) NOT NULL default '0', "
		"PRIMARY KEY (user_id, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientServer()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Creating %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
	if (!mani_mysql->ExecuteQuery(
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"version_id varchar(20) NOT NULL)",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Checking %s%s\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());

	int row_count;
	if (mani_mysql->ExecuteQuery(&row_count, "SELECT 1 FROM %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion()))
	{
		if (row_count == 0)
		{
			MMsg("No rows found, inserting into %s%s\n",  gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
			// No rows so insert one
			if (!mani_mysql->ExecuteQuery("INSERT INTO %s%s VALUES ('%s')", 
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBVersion(),
				PLUGIN_VERSION_ID2))
			{
				delete mani_mysql;
				return false;
			}
		}
		else
		{
			MMsg("Row found, updating %s%s\n",  gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
			if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET version_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBVersion(),
				PLUGIN_VERSION_ID2))
			{
				delete mani_mysql;
				return false;
			}
		}
	}
	else
	{
		delete mani_mysql;
		return false;
	}

	delete mani_mysql;	

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Creates flag information in tables
//---------------------------------------------------------------------------------
bool ManiClient::CreateDBFlags(void)
{
	char	temp_sql[8192];

	MMsg("Generating DB access flags if not existing....\n");

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init())
	{
		delete mani_mysql;
		return false;
	}

	Q_snprintf (temp_sql, sizeof(temp_sql), "INSERT IGNORE INTO %sflag VALUES ", gpManiDatabase->GetDBTablePrefix());

	for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
	{
		char	temp_flags[256];

		Q_snprintf(temp_flags, sizeof(temp_flags), "('%s', 'Admin', '%s'),", 
			admin_flag_list[i].flag,
			admin_flag_list[i].flag_desc
			);

		Q_strcat(temp_sql, temp_flags);
	}

	for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
	{
		char	temp_flags[256];

		Q_snprintf(temp_flags, sizeof(temp_flags), "('%s', 'Immunity', '%s'),", 
			immunity_flag_list[i].flag,
			immunity_flag_list[i].flag_desc
			);

		Q_strcat(temp_sql, temp_flags);
	}

	MMsg("Updating version id..\n");

	temp_sql[Q_strlen(temp_sql) - 1] = '\0';
	if (mani_mysql->ExecuteQuery("%s", temp_sql))
	{
		mani_mysql->ExecuteQuery("UPDATE %s%s "
			"SET version_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBVersion(),
			PLUGIN_CORE_VERSION);
	}

	delete mani_mysql;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Export data in memory to the database
//---------------------------------------------------------------------------------
bool ManiClient::ExportDataToDB(void)
{
	char	flag_string[1024];
	bool	found_flag;

	MMsg("Exporting data from clients.txt to DB....\n");

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init())
	{
		delete mani_mysql;
		return false;
	}

	// Clean out tables for this server id
	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBGroup(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBLevel(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE server_id = %i", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer(), gpManiDatabase->GetServerID()))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Deleted existing DB data for this server....\n");

	// Do server details
	if (!mani_mysql->ExecuteQuery("INSERT INTO %s%s VALUES (%i, '%s', '%s', %i, '%s', '%s', '%s')",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer(),
		gpManiDatabase->GetServerID(),
		gpManiDatabase->GetServerName(),
		gpManiDatabase->GetServerIPAddress(),
		gpManiDatabase->GetServerPort(),
		gpManiDatabase->GetModName(),
		gpManiDatabase->GetRCONPassword(),
		gpManiDatabase->GetServerGroupID()
		))
	{
		delete mani_mysql;
		return false;
	}

	MMsg("Generated server details....\n");

	// Do the level groups next

	for (int i = 0; i < admin_level_list_size; i ++)
	{
		Q_strcpy (flag_string,"");
		for (int j = 0; j < MAX_ADMIN_FLAGS; j++)
		{
			char	temp_flag[20];

			if (admin_level_list[i].flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", admin_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES ('%s', '%s', 'Admin', '%s')", 
						gpManiDatabase->GetDBTablePrefix(),
						gpManiDatabase->GetDBTBLevel(),
						admin_level_list[i].level_id,
						flag_string,
						gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	for (int i = 0; i < immunity_level_list_size; i ++)
	{
		Q_strcpy (flag_string,"");

		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j++)
		{
			char	temp_flag[20];

			if (immunity_level_list[i].flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", immunity_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES ('%s', '%s', 'Immunity', '%s')", 
						gpManiDatabase->GetDBTablePrefix(),
						gpManiDatabase->GetDBTBLevel(),
						immunity_level_list[i].level_id,
						flag_string,
						gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	MMsg("Generated level groups....\n");

	// Do the flag groups next

	for (int i = 0; i < admin_group_list_size; i ++)
	{
		Q_strcpy (flag_string,"");
		for (int j = 0; j < MAX_ADMIN_FLAGS; j++)
		{
			char	temp_flag[20];

			if (admin_group_list[i].flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", admin_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES ('%s', '%s', 'Admin', '%s')", 
						gpManiDatabase->GetDBTablePrefix(),
						gpManiDatabase->GetDBTBGroup(),
						admin_group_list[i].group_id,
						flag_string,
						gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	for (int i = 0; i < immunity_group_list_size; i ++)
	{
		Q_strcpy (flag_string,"");

		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j++)
		{
			char	temp_flag[20];

			if (immunity_group_list[i].flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", immunity_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES ('%s', '%s', 'Immunity', '%s')", 
						gpManiDatabase->GetDBTablePrefix(),
						gpManiDatabase->GetDBTBGroup(),
						immunity_group_list[i].group_id,
						flag_string,
						gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	MMsg("Generated DB admin/immunity groups....\n");

	MMsg("Building DB client data for %i clients\n", client_list_size);
	// Populate client list for players that already exist on the server
	// and generate new clients if necessary with user ids
	for (int i = 0; i < client_list_size; i ++)
	{
		int	row_count;

		MMsg(".");

		client_list[i].user_id = -1;

		if (!mani_mysql->ExecuteQuery(&row_count, "SELECT user_id FROM %s%s WHERE name = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(), client_list[i].name))
		{
			delete mani_mysql;
			return false;
		}

		if (row_count != 0)
		{
			if (!mani_mysql->FetchRow())
			{
				// Should get at least 1 row
				delete mani_mysql;
				return false;
			}

			client_list[i].user_id = mani_mysql->GetInt(0);
		}
		else
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s "
				"(name, password, email, notes) "
				"VALUES "
				"('%s', '%s', '%s', '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				client_list[i].name,
				client_list[i].password,
				client_list[i].email_address,
				client_list[i].notes))
			{
				delete mani_mysql;
				return false;
			}

			client_list[i].user_id = mani_mysql->GetRowID();
		}

		// Setup steam ids
		if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBSteam(),
			client_list[i].user_id))
		{
			delete mani_mysql;
			return false;
		}

		for (int j = 0; j < client_list[i].steam_list_size; j++)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBSteam(),
				client_list[i].user_id,
				client_list[i].steam_list[j].steam_id))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup ip addresses
		if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBIP(),
			client_list[i].user_id))
		{
			delete mani_mysql;
			return false;
		}

		for (int j = 0; j < client_list[i].ip_address_list_size; j++)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBIP(),
				client_list[i].user_id,
				client_list[i].ip_address_list[j].ip_address))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup nickname ids
		if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBNick(),
			client_list[i].user_id))
		{
			delete mani_mysql;
			return false;
		}

		for (int j = 0; j < client_list[i].nick_list_size; j++)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBNick(),
				client_list[i].user_id,
				client_list[i].nick_list[j].nick))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup client_server record
		if (!mani_mysql->ExecuteQuery("INSERT INTO %s%s VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientServer(),
			client_list[i].user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}

		// Do the client flags next
		found_flag = false;
		Q_strcpy (flag_string,"");

		for (int j = 0; j < MAX_ADMIN_FLAGS; j++)
		{
			char	temp_flag[20];

			if (client_list[i].original_admin_flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", admin_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (found_flag)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,'%s','Admin','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientFlag(),
				client_list[i].user_id,
				flag_string,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		found_flag = false;
		Q_strcpy (flag_string,"");

		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j++)
		{
			char	temp_flag[20];

			if (client_list[i].original_immunity_flags[j])
			{
				Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", immunity_flag_list[j].flag);
				Q_strcat(flag_string, temp_flag);
				found_flag = true;
			}
		}

		if (found_flag)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,'%s','Immunity','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientFlag(),
				client_list[i].user_id,
				flag_string,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Do the client_group flags next
		for (int j = 0; j < client_list[i].admin_group_list_size; j++)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,'%s','Admin','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				client_list[i].user_id,
				client_list[i].admin_group_list[j].group_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		for (int j = 0; j < client_list[i].immunity_group_list_size; j++)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,'%s','Immunity','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				client_list[i].user_id,
				client_list[i].immunity_group_list[j].group_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Do the client_level groups next
		if (client_list[i].admin_level_id != -1)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,%i,'Admin','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				client_list[i].user_id,
				client_list[i].admin_level_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		if (client_list[i].immunity_level_id != -1)
		{
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,%i,'Immunity','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				client_list[i].user_id,
				client_list[i].immunity_level_id,
				gpManiDatabase->GetServerGroupID()))
			{	
				delete mani_mysql;
				return false;
			}
		}
	}

	MMsg("\nClients built on DB\n");


	return true;

}

//---------------------------------------------------------------------------------
// Purpose: Get Client data from database
//---------------------------------------------------------------------------------
bool ManiClient::GetClientsFromDatabase(void)
{
	bool	found_flag;
	int	row_count;
	char	flags_string[1024];

	// Upgrade the database to V1.2 Beta M functionality
	UpgradeDB1();

	MMsg("Getting client info from the database....\n");

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init())
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET server_group_id = '%s' WHERE server_id = %i",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer(),
		gpManiDatabase->GetServerGroupID(),
		gpManiDatabase->GetServerID()))
	{
		delete mani_mysql;
		mani_mysql = new ManiMySQL();
		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return false;
		}
	}


	// Get admin groups
	if (!mani_mysql->ExecuteQuery(&row_count, 
		"SELECT g.group_id, g.flag_string, g.type "
		"FROM %s%s g "
		"WHERE g.server_group_id = '%s' ",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBGroup(),
		gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (row_count != 0)
	{
		// Found rows
		while (mani_mysql->FetchRow())
		{
			if (FStrEq(mani_mysql->GetString(2), "Admin"))
			{
				admin_group_t   admin_group;
				Q_memset(&admin_group, 0, sizeof(admin_group_t));

				Q_strcpy(admin_group.group_id, mani_mysql->GetString(0));

				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(1));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_ADMIN_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						admin_group.flags[flag_index] = true;
					}
				}

				AddToList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size);
				admin_group_list[admin_group_list_size - 1] = admin_group;
//				MMsg("Admin Group [%s]\n", admin_group.group_id);
			}
			else
			{
				immunity_group_t   immunity_group;
				Q_memset(&immunity_group, 0, sizeof(immunity_group_t));
				Q_strcpy(immunity_group.group_id, mani_mysql->GetString(0));

				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(1));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_IMMUNITY_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						immunity_group.flags[flag_index] = true;
					}
				}

				AddToList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size);
				immunity_group_list[immunity_group_list_size - 1] = immunity_group;
//				MMsg("Immunity Group [%s]\n", immunity_group.group_id);
			}
		}
	}


	// Get admin levels
	if (!mani_mysql->ExecuteQuery(&row_count, 
		"SELECT l.level_id, l.flag_string, l.type "
		"FROM %s%s l "
		"WHERE l.server_group_id = '%s' "
		"AND l.type = 'Admin' "
		"ORDER BY l.level_id",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBLevel(),
		gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (row_count != 0)
	{

		// Found rows
		while (mani_mysql->FetchRow())
		{
			admin_level_t   admin_level;
			Q_memset(&admin_level, 0, sizeof(admin_level_t));

			if (FStrEq(mani_mysql->GetString(2), "Admin"))
			{
				admin_level.level_id = mani_mysql->GetInt(0);

				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(1));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_ADMIN_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						admin_level.flags[flag_index] = true;
					}
				}

				AddToList((void **) &admin_level_list, sizeof(admin_level_t), &admin_level_list_size);
				admin_level_list[admin_level_list_size - 1] = admin_level;
//				MMsg("Admin Level ID [%i]\n", admin_level.level_id);
			}
			else
			{
				immunity_level_t   immunity_level;

				Q_memset(&immunity_level, 0, sizeof(immunity_level_t));
				immunity_level.level_id = mani_mysql->GetInt(0);

				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(1));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_IMMUNITY_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						immunity_level.flags[flag_index] = true;
					}
				}

				AddToList((void **) &immunity_level_list, sizeof(immunity_level_t), &immunity_level_list_size);
				immunity_level_list[immunity_level_list_size - 1] = immunity_level;
//				MMsg("Immunity Level [%s]\n", immunity_level.level_id);
			}	
		}
	}


	// Get clients for this server
	// Ridiculous search that brings back too many rows but ultimately
	// is faster than doing all the seperate selects per client
	// to do the same thing :(

	Msg("SQL server version [%s]\n", mani_mysql->GetServerVersion());
	Msg("Major [%i] Minor [%i] Issue [%i]\n", mani_mysql->GetMajor(), mani_mysql->GetMinor(), mani_mysql->GetIssue());

	if (mani_mysql->IsHigherVer(5,0,11))
	{
		// Post 5.0.11
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"select c.user_id, c.name, c.password, c.email, c.notes, "
			"cf.type, cf.flag_string, "
			"s.steam_id, n.nick, i.ip_address, "
			"cg.type, cg.group_id, "
			"cl.type, cl.level_id "
			"from (%s%s c, %s%s cs) "
			"LEFT JOIN (%s%s s) ON (s.user_id = c.user_id) "
			"LEFT JOIN (%s%s cf) ON (cf.user_id = c.user_id) "
			"LEFT JOIN (%s%s n) ON (n.user_id = c.user_id) "
			"LEFT JOIN (%s%s i) ON (i.user_id = c.user_id) "
			"LEFT JOIN (%s%s cg) ON (cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id) "
			"LEFT JOIN (%s%s cl) ON (cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id) "
			"where c.user_id = cs.user_id "
			"and cs.server_group_id = '%s' "
			"order by c.user_id, cf.type, s.steam_id, n.nick, i.ip_address, cg.type,  "
			"cg.group_id, cl.type, cl.level_id ",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(),
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}
	else
	{
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"select c.user_id, c.name, c.password, c.email, c.notes, "
			"cf.type, cf.flag_string, "
			"s.steam_id, n.nick, i.ip_address, "
			"cg.type, cg.group_id, "
			"cl.type, cl.level_id "
			"from %s%s c, %s%s cs "
			"LEFT JOIN %s%s s ON (s.user_id = c.user_id) "
			"LEFT JOIN %s%s cf ON (cf.user_id = c.user_id) "
			"LEFT JOIN %s%s n ON (n.user_id = c.user_id) "
			"LEFT JOIN %s%s i ON (i.user_id = c.user_id) "
			"LEFT JOIN %s%s cg ON (cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id) "
			"LEFT JOIN %s%s cl ON (cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id) "
			"where c.user_id = cs.user_id "
			"and cs.server_group_id = '%s' "
			"order by c.user_id, cf.type, s.steam_id, n.nick, i.ip_address, cg.type,  "
			"cg.group_id, cl.type, cl.level_id ",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(),
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	// Declare 'last' vars to keep track of changing data
	int last_user_id = -1;
	bool done_aflags = false;
	bool done_iflags = false;
	bool done_alevel = false;
	bool done_ilevel = false;

	if (row_count != 0)
	{
		client_t	*client_ptr;
		// Found rows
		while (mani_mysql->FetchRow())
		{
			if (last_user_id != mani_mysql->GetInt(0))
			{
				AddToList((void **) &client_list, sizeof(client_t), &client_list_size);
				client_ptr = &(client_list[client_list_size - 1]);
				Q_memset(client_ptr, 0, sizeof(client_t));

				client_ptr->user_id = mani_mysql->GetInt(0);
				client_ptr->admin_level_id = -1;
				client_ptr->immunity_level_id = -1;
				Q_strcpy(client_ptr->name, mani_mysql->GetString(1));
				Q_strcpy(client_ptr->password, mani_mysql->GetString(2));
				Q_strcpy(client_ptr->email_address, mani_mysql->GetString(3));
				Q_strcpy(client_ptr->notes, mani_mysql->GetString(4));
				last_user_id = client_ptr->user_id;
				done_aflags = false;
				done_iflags = false;
				done_alevel = false;
				done_ilevel = false;
			}

			if (!done_aflags && mani_mysql->GetString(5) && 
				FStrEq(mani_mysql->GetString(5),"Admin"))
			{
				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(6));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_ADMIN_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						client_ptr->original_admin_flags[flag_index] = true;
					}
				}

				done_aflags = true;
			}

			if (!done_iflags && mani_mysql->GetString(5) && 
				FStrEq(mani_mysql->GetString(5),"Immunity"))
			{
				int flags_string_index = 0;

				Q_strcpy(flags_string, mani_mysql->GetString(6));

				while(flags_string[flags_string_index] != '\0')
				{
					int flag_index = this->GetNextFlag(flags_string, &flags_string_index, MANI_IMMUNITY_TYPE);
					if (flag_index != -1)
					{
						// Valid flag index given
						client_ptr->original_immunity_flags[flag_index] = true;
					}
				}

				done_iflags = true;
			}

			if (!done_alevel && mani_mysql->GetString(12) && 
				FStrEq(mani_mysql->GetString(12),"Admin"))
			{
				client_ptr->admin_level_id = mani_mysql->GetInt(13); 
				done_alevel = true;
			}

			if (!done_ilevel && mani_mysql->GetString(12) && 
				FStrEq(mani_mysql->GetString(12),"Immunity"))
			{
				client_ptr->immunity_level_id = mani_mysql->GetInt(13); 
				done_ilevel = true;
			}

			// Check Steam ID
			if (mani_mysql->GetString(7))
			{
				// Found Steam ID, search for exising entry
				found_flag = false;
				for (int i = 0; i < client_ptr->steam_list_size; i++)
				{
					if (FStrEq(mani_mysql->GetString(7), client_ptr->steam_list[i].steam_id))
					{
						found_flag = true;
						break;
					}
				}

				if (!found_flag)
				{
					// Add steam ID to client
					AddToList((void **) &(client_ptr->steam_list), sizeof(steam_t), &(client_ptr->steam_list_size));
					Q_strcpy(client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id, mani_mysql->GetString(7));
//					MMsg("Steam ID [%s]\n", client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id);
				}
			}

			// Check Nick names
			if (mani_mysql->GetString(8))
			{
				// Found Nick ID, search for exising entry
				found_flag = false;
				for (int i = 0; i < client_ptr->nick_list_size; i++)
				{
					if (FStrEq(mani_mysql->GetString(8), client_ptr->nick_list[i].nick))
					{
						found_flag = true;
						break;
					}
				}

				if (!found_flag)
				{
					// Add nickname to client
					AddToList((void **) &(client_ptr->nick_list), sizeof(nick_t), &(client_ptr->nick_list_size));
					Q_strcpy(client_ptr->nick_list[client_ptr->nick_list_size - 1].nick, mani_mysql->GetString(8));
//					MMsg("Nick [%s]\n", client_ptr->nick_list[client_ptr->nick_list_size - 1].nick);
				}
			}

			// Check IP Addresses
			if (mani_mysql->GetString(9))
			{
				// Found IP Address, search for exising entry
				found_flag = false;
				for (int i = 0; i < client_ptr->ip_address_list_size; i++)
				{
					if (FStrEq(mani_mysql->GetString(9), client_ptr->ip_address_list[i].ip_address))
					{
						found_flag = true;
						break;
					}
				}

				if (!found_flag)
				{
					// Add IP Address to client
					AddToList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &(client_ptr->ip_address_list_size));
					Q_strcpy(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address, mani_mysql->GetString(9));
//					MMsg("Nick [%s]\n", client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address);
				}
			}

			// Check admin groups
			if (mani_mysql->GetString(10) && FStrEq(mani_mysql->GetString(10),"Admin"))
			{
				// Found group
				found_flag = false;
				for (int i = 0; i < client_ptr->admin_group_list_size; i++)
				{
					if (FStrEq(mani_mysql->GetString(11), client_ptr->admin_group_list[i].group_id))
					{
						found_flag = true;
						break;
					}
				}

				if (!found_flag)
				{
					AddToList((void **) &(client_ptr->admin_group_list), sizeof(group_t), &(client_ptr->admin_group_list_size));
					Q_strcpy(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id, mani_mysql->GetString(11));
//					MMsg("Group ID [%s]\n", client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id);
				}
			}

			if (mani_mysql->GetString(10) && FStrEq(mani_mysql->GetString(10),"Immunity"))
			{
				// Found group
				found_flag = false;
				for (int i = 0; i < client_ptr->immunity_group_list_size; i++)
				{
					if (FStrEq(mani_mysql->GetString(11), client_ptr->immunity_group_list[i].group_id))
					{
						found_flag = true;
						break;
					}
				}

				if (!found_flag)
				{
					AddToList((void **) &(client_ptr->immunity_group_list), sizeof(group_t), &(client_ptr->immunity_group_list_size));
					Q_strcpy(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id, mani_mysql->GetString(11));
//					MMsg("Group ID [%s]\n", client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id);
				}
			}
		}
	}

	// Put into place the admin group flags for the clients
	for (int i = 0; i < client_list_size; i++)
	{
		RebuildFlags(&(client_list[i]), MANI_ADMIN_TYPE);
		RebuildFlags(&(client_list[i]), MANI_IMMUNITY_TYPE);
	}

	ComputeAdminLevels();
	ComputeImmunityLevels();

//	DumpClientsToConsole();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setadminflag command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaSetAdminFlag(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!IsAdminAllowed(player_ptr, command_name, ALLOW_SETADMINFLAG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	char *target_string = (char *) gpCmd->Cmd_Argv(1);

	admin_index = FindClientIndex(target_string, true, false);
	if (admin_index != -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found an admin to update in memory via admin_index
	// Loop through flags to set them

	char *flags = (char *) gpCmd->Cmd_Argv(2);
	int	length = Q_strlen(flags);

	bool add_flag;
	for (int i = 0; i < length; i ++)
	{
		if (flags[i] == '+') 
		{
			add_flag = true;
			continue;
		}

		if (flags[i] == '-')
		{
			add_flag = false;
			continue;
		}

		if (flags[i] == ' ') continue;

		bool found_flag = false;

		int flag_index = this->GetNextFlag(&(flags[i]), &i, MANI_ADMIN_TYPE);
		if (flag_index != -1)
		{
			// Valid flag index given
			client_list[admin_index].admin_flags[flag_index] = ((add_flag == true) ? false:true);
			client_list[admin_index].original_admin_flags[flag_index] = ((add_flag == true) ? false:true);
			found_flag = true;
		}

		if (!found_flag)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Flag %s is invalid", flags[i]);
		}
	}

	LogCommand (player_ptr, "%s [%s] [%s]\n", command_name, target_string, flags);

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Find an entry for the target string in the client list
//---------------------------------------------------------------------------------
int		ManiClient::FindClientIndex
(
 char	*target_string,
 bool	check_if_admin,
 bool	check_if_immune
 )
{
	player_t player;
	int	client_index = -1;
	player.entity = NULL;

	// Whoever issued the commmand is authorised to do it.

	int	target_user_id = Q_atoi(target_string);
	char target_steam_id[MAX_NETWORKID_LENGTH];
	player_t	target_player;

	if (!target_string) return -1;
	if (FStrEq(target_string,"")) return -1;

	// Try looking for user id first in players on server
	if (target_user_id != 0)
	{
		target_player.user_id = target_user_id;
		if (FindPlayerByUserID(&target_player))
		{
			if (check_if_admin && IsAdmin(&target_player, &client_index))
			{
				return client_index;
			}
			else if (check_if_immune && IsImmune(&target_player, &client_index))
			{
				return client_index;
			}
		}
	}

	// Try steam id next
	Q_strcpy(target_steam_id, target_string);
	if (Q_strlen(target_steam_id) > 6)
	{
		target_steam_id[6] = '\0';
		if (FStruEq(target_steam_id, "STEAM_"))
		{
			for (int i = 0; i < client_list_size; i++)
			{
				for (int j = 0; j < client_list[i].steam_list_size; j ++)
				{
					if (FStrEq(client_list[i].steam_list[j].steam_id, target_string))
					{
						client_index = i;
						return client_index;
					}
				}
			}
		}
	}

	// Try ip address next
	for (int i = 0; i < client_list_size; i++)
	{
		for (int j = 0; j < client_list[i].ip_address_list_size; j ++)
		{
			if (FStrEq(client_list[i].ip_address_list[j].ip_address, target_string))
			{
				client_index = i;
				return client_index;
			}
		}
	}

	// Try name id next
	for (int i = 0; i < client_list_size; i++)
	{
		if (client_list[i].name)
		{
			if (FStrEq(client_list[i].name, target_string))
			{
				client_index = i;
				return client_index;
			}
		}
	}

	// Try nick name next
	for (int i = 0; i < client_list_size; i++)
	{
		for (int j = 0; j < client_list[i].nick_list_size; j ++)
		{
			if (FStrEq(client_list[i].nick_list[j].nick, target_string))
			{
				client_index = i;
				return client_index;
			}
		}
	}

	return client_index;
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaClient(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	int	client_index;
	bool	invalid_args = false;

	if (player_ptr)
	{
		// Check if player is admin
		if (!IsAdminAllowed(player_ptr, command_name, ALLOW_CLIENT_ADMIN, war_mode, &client_index)) return PLUGIN_STOP;
	}

	// Cover only command and subcommand being passed in
	int argc = gpCmd->Cmd_Argc();

	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	char *sub_command = (char *) gpCmd->Cmd_Argv(1);
	char *param1 = (char *) gpCmd->Cmd_Argv(2);
	char *param2 = (char *) gpCmd->Cmd_Argv(3);

	// Parse sub command
	if (FStrEq(sub_command, "addclient"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessAddClient(player_ptr, param1);
	}
	else if (FStrEq(sub_command, "addsteam"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddSteam(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addip"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddIP(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addnick"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddNick(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setname"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetName(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setpassword"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetPassword(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setemail"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetEmail(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setnotes"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetNotes(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setalevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetLevel(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setilevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetLevel(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroup(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroup(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setaflag"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetFlag(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setiflag"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetFlag(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeclient"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveClient(player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removesteam"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveSteam(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeip"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveIP(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removenick"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveNick(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveGroup(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveGroup(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "status"))
	{
		if (argc == 3)
		{
			ProcessClientStatus(player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientStatus(player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else if (FStrEq(sub_command, "aflag"))
	{
		if (argc == 3)
		{
			ProcessClientFlagDesc(MANI_ADMIN_TYPE, player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientFlagDesc(MANI_ADMIN_TYPE, player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}	
	else if (FStrEq(sub_command, "iflag"))
	{
		if (argc == 3)
		{
			ProcessClientFlagDesc(MANI_IMMUNITY_TYPE, player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientFlagDesc(MANI_IMMUNITY_TYPE, player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else if (FStrEq(sub_command, "upload"))
	{
		if (argc != 2) invalid_args = true;
		else ProcessClientUpload(player_ptr);
	}
	else if (FStrEq(sub_command, "download"))
	{
		if (argc != 2) invalid_args = true;
		else ProcessClientDownload(player_ptr);
	}
	else
	{
		invalid_args = true;
	}

	if (invalid_args)
	{
		gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddClient
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddClient
(
 player_t *player_ptr, 
 char *param1
 )
{
	client_t	*client_ptr;

	for (int i = 0; i < client_list_size; i ++)
	{
		if (FStrEq(client_list[i].name, param1))
		{
			// Bad, client already exists 
			OutputHelpText(ORANGE_CHAT, player_ptr, "ERROR: This client name already exists !!");
			return;
		}
	}

	// Add a new client
	AddToList((void **) &client_list, sizeof(client_t), &client_list_size);
	client_ptr = &(client_list[client_list_size - 1]);
	Q_memset (client_ptr, 0, sizeof(client_t));

	client_ptr->admin_level_id = -1;
	client_ptr->immunity_level_id = -1;
	Q_strcpy(client_ptr->name, param1);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s "
			"(name, password, email, notes) "
			"VALUES "
			"('%s', '', '', '')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClient(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		client_ptr->user_id = mani_mysql->GetRowID();

		// Setup client_server record
		if (!mani_mysql->ExecuteQuery("INSERT INTO %s%s VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientServer(),
			client_ptr->user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return;
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has been added", client_ptr->name);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddSteam
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddSteam
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	AddToList((void **) &(client_ptr->steam_list), sizeof(steam_t), &(client_ptr->steam_list_size));
	Q_strcpy(client_ptr->steam_list[client_ptr->steam_list_size - 1].steam_id, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBSteam(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added Steam ID [%s] for client [%s]", param2, client_ptr->name);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddIP
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddIP
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	AddToList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &(client_ptr->ip_address_list_size));
	Q_strcpy(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1].ip_address, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBIP(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added IP Address [%s] for client [%s]", param2, client_ptr->name);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddNick
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddNick
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	AddToList((void **) &(client_ptr->nick_list), sizeof(nick_t), &(client_ptr->nick_list_size));
	Q_strcpy(client_ptr->nick_list[client_ptr->nick_list_size - 1].nick, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBNick(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added Nickname [%s] for client [%s]", param2, client_ptr->name);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetName
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetName
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;
	char	old_name[256];

	if (param2 == NULL || FStrEq(param2, ""))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "You cannot set a client name to be blank !!");
		return;
	}

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	if (FStrEq(client_ptr->name, param2))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Both names [%s] and [%s] are the same !!", client_ptr->name, param2);
		return;
	}

	// Check for existing clients with the same name
	for (int i = 0; i < client_list_size; i++)
	{
		// Skip target client name
		if (i == client_index)
		{
			continue;
		}

		if (FStrEq(client_list[i].name, param2))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "A Client already exists with this name !!");
			return;
		}
	}
		
	Q_strcpy(old_name, client_ptr->name);	
	Q_strcpy(client_ptr->name, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			old_name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET name = '%s' WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				param2,
				client_ptr->user_id))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new name of [%s]", old_name, param2);
	return;

}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetPassword
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetPassword
(
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	Q_strcpy(client_ptr->password, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET password = '%s' WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				param2,
				client_ptr->user_id))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new password of [%s]", client_ptr->name, param2);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetEmail
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetEmail
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	Q_strcpy(client_ptr->email_address, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET email = '%s' WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				param2,
				client_ptr->user_id))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new email address of [%s]", client_ptr->name, param2);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddNotes
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetNotes
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);
	Q_strcpy(client_ptr->notes, param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get clients for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		if (row_count != 0)
		{
			mani_mysql->FetchRow();

			client_ptr->user_id = mani_mysql->GetInt(0);
			if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET notes = '%s' WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				param2,
				client_ptr->user_id))
			{
				delete mani_mysql;
				return;
			}
		}

		delete mani_mysql;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new notes of [%s]", client_ptr->name, param2);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetLevel
(
 int	type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;
	int	level_id;
	bool	found_level = false;
	char	flag_type[32];

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	if (param2 == NULL || FStrEq(param2, ""))
	{
		level_id = -1;
	}
	else
	{
		level_id = Q_atoi(param2);
	}

	client_ptr = &(client_list[client_index]);
	if (MANI_ADMIN_TYPE == type)
	{
		client_ptr->admin_level_id = level_id;
	}
	else
	{
		client_ptr->immunity_level_id = level_id;
	}

	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		int row_count;

		// Add client to database too 
		ManiMySQL *mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			return;
		}

		// Get client for this server
		if (!mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			return;
		}

		// Get User ID
		if (row_count != 0)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			// Drop level id reference
			if (!mani_mysql->ExecuteQuery(&row_count,
				"DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND server_group_id = '%s' "
				"AND type = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				client_ptr->user_id,
				gpManiDatabase->GetServerGroupID(),
				flag_type))
			{
				delete mani_mysql;
				return;
			}

			if (level_id > -1)
			{	
				if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,%i,'%s','%s')",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBClientLevel(),
					client_ptr->user_id,
					level_id,
					flag_type,
					gpManiDatabase->GetServerGroupID()))
				{	
					delete mani_mysql;
					return;
				}		
			}
		}

		delete mani_mysql;
	}

	if (MANI_ADMIN_TYPE == type)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Updated client [%s] with new admin level id [%s]", client_ptr->name, param2);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Updated client [%s] with new immunity level id [%s]", client_ptr->name, param2);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddGroup
(
 int	type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;
	bool	found_group = false;
	char	flag_type[32];

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < admin_group_list_size; i++)
		{
			if (FStrEq(admin_group_list[i].group_id, param2))
			{
				found_group = true;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < immunity_group_list_size; i++)
		{
			if (FStrEq(immunity_group_list[i].group_id, param2))
			{
				found_group = true;
				break;
			}
		}
	}

	if (!found_group)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group ID [%s] is invalid !!", param2);
		return;
	}

	found_group = false;

	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < client_ptr->admin_group_list_size; i++)
		{
			if (FStrEq(client_ptr->admin_group_list[i].group_id, param2))
			{
				found_group = true;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < client_ptr->immunity_group_list_size; i++)
		{
			if (FStrEq(client_ptr->immunity_group_list[i].group_id, param2))
			{
				found_group = true;
				break;
			}
		}
	}

	if (found_group)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group ID [%s] is already setup for this user", param2);
		return;
	}


	if (MANI_ADMIN_TYPE == type)
	{
		AddToList((void **) &(client_ptr->admin_group_list), sizeof(group_t), &(client_ptr->admin_group_list_size));
		Q_strcpy(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1].group_id, param2);
	}
	else
	{
		AddToList((void **) &(client_ptr->immunity_group_list), sizeof(group_t), &(client_ptr->immunity_group_list_size));
		Q_strcpy(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1].group_id, param2);
	}


	WriteClients();
	RebuildFlags(client_ptr, type);

	if (gpManiDatabase->GetDBEnabled())
	{
		for (;;)
		{
			int row_count;

			// Add client to database too 
			ManiMySQL *mani_mysql = new ManiMySQL();

			if (!mani_mysql->Init())
			{
				delete mani_mysql;
				break; 
			}

			// Get client for this server
			if (!mani_mysql->ExecuteQuery(&row_count, 
				"SELECT c.user_id "
				"FROM %s%s c, %s%s cs "
				"where cs.server_group_id = '%s' "
				"and cs.user_id = c.user_id "
				"and c.name = '%s'",
				gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
				gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
				gpManiDatabase->GetServerGroupID(),
				client_ptr->name))
			{
				delete mani_mysql;
				break;
			}

			// Get User ID
			if (row_count != 0)
			{
				mani_mysql->FetchRow();
				client_ptr->user_id = mani_mysql->GetInt(0);

				// Check if group actually exists
				if (!mani_mysql->ExecuteQuery(&row_count,
					"SELECT 1 "
					"FROM %s%s "
					"WHERE group_id = '%s' "
					"AND server_group_id = '%s' "
					"AND type = '%s'",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBGroup(),
					param2,
					gpManiDatabase->GetServerGroupID(),
					flag_type))
				{
					delete mani_mysql;
					break;
				}

				if (row_count == 0)
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Group ID [%s] is invalid !!", param2);
					delete mani_mysql;
					break;
				}

				// Drop group id reference
				if (!mani_mysql->ExecuteQuery(&row_count,
					"DELETE FROM %s%s "
					"WHERE group_id = '%s' "
					"AND user_id = %i "
					"AND server_group_id = '%s' "
					"AND type = '%s'",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBClientGroup(),
					param2,
					client_ptr->user_id,
					gpManiDatabase->GetServerGroupID(),
					flag_type))
				{
					delete mani_mysql;
					break;
				}

				if (!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s VALUES (%i,'%s','%s',%i)",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBClientGroup(),
					client_ptr->user_id,
					param2,
					flag_type,
					gpManiDatabase->GetServerGroupID()))
				{	
					delete mani_mysql;
					break;
				}		
			}

			delete mani_mysql;
			break;
		}
	}

	if (MANI_ADMIN_TYPE == type)
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Client [%s] now has admin group [%s] access", client_ptr->name, param2);
	}
	else
	{
		ComputeImmunityLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Client [%s] now has immunity group [%s] access", client_ptr->name, param2);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command AddGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddGroupType
(
 int	type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int		group_index = -1;
	char	flag_type[32];
	bool	insert_required = false;

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	// See if group already exists
	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < admin_group_list_size; i++)
		{
			if (FStrEq(admin_group_list[i].group_id, param1))
			{
				group_index = i;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < immunity_group_list_size; i++)
		{
			if (FStrEq(immunity_group_list[i].group_id, param1))
			{
				group_index = i;
				break;
			}
		}
	}

	if (group_index == -1)
	{
		insert_required = true;

		if (MANI_ADMIN_TYPE == type)
		{
			AddToList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size);
			Q_memset(&(admin_group_list[admin_group_list_size - 1]), 0, sizeof(admin_group_t));
			Q_strcpy(admin_group_list[admin_group_list_size - 1].group_id, param1);
			group_index = admin_group_list_size - 1;
		}
		else
		{
			AddToList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size);
			Q_memset(&(immunity_group_list[immunity_group_list_size - 1]), 0, sizeof(immunity_group_t));
			Q_strcpy(immunity_group_list[immunity_group_list_size - 1].group_id, param1);
			group_index = immunity_group_list_size - 1;
		}

	}

	int		param2_index = 0;

	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/remove all 
			// Valid flag index	given
			if (MANI_ADMIN_TYPE == type)
			{
				for (int x = 0; x < MAX_ADMIN_FLAGS; x ++)
				{
					admin_group_list[group_index].flags[x] = ((flag_add) ? true:false);
				}
			}
			else
			{
				for (int x = 0; x < MAX_IMMUNITY_FLAGS; x ++)
				{
					immunity_group_list[group_index].flags[x] = ((flag_add) ? true:false);
				}
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}			

		int	flag_index = this->GetNextFlag(param2, &param2_index, type);
		if (flag_index != -1)
		{
			// Valid flag index	given
			if (MANI_ADMIN_TYPE	== type)
			{
				admin_group_list[group_index].flags[flag_index] = ((flag_add) ? true:false);
			}
			else
			{
				immunity_group_list[group_index].flags[flag_index] = ((flag_add) ? true:false);
			}
		}
	}

	WriteClients();

	if (!insert_required)
	{
		for (int i = 0; i < client_list_size; i++)
		{
			RebuildFlags(&(client_list[i]), type);
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		// Build the strings needed
		bool found_flag	= false;
		char	flag_string[1024];
		Q_strcpy (flag_string,"");

		if (MANI_ADMIN_TYPE	== type)
		{
			for	(int j = 0;	j <	MAX_ADMIN_FLAGS; j++)
			{
				char	temp_flag[20];

				if (admin_group_list[group_index].flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	admin_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}
		else
		{
			for	(int j = 0;	j <	MAX_IMMUNITY_FLAGS;	j++)
			{
				char	temp_flag[20];

				if (immunity_group_list[group_index].flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	immunity_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}

		for (;;)
		{
			// Add client to database too 
			ManiMySQL *mani_mysql = new ManiMySQL();

			if (!mani_mysql->Init())
			{
				delete mani_mysql;
				break; 
			}

			if (insert_required)
			{
				// Drop group id reference
				if (!mani_mysql->ExecuteQuery(
					"INSERT IGNORE INTO %s%s VALUES ('%s', '%s', '%s', '%s')",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBGroup(),
					param1,
					flag_string,
					flag_type,
					gpManiDatabase->GetServerGroupID()))
				{
					delete mani_mysql;
					break;
				}
			}
			else
			{
				if (!mani_mysql->ExecuteQuery(
					"UPDATE %s%s "
					"SET flag_string = '%s' "
					"WHERE group_id = '%s' "
					"AND type = '%s' "
					"AND server_group_id = '%s'",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBGroup(),
					flag_string,
					param1,
					flag_type,
					gpManiDatabase->GetServerGroupID()))
				{	
					delete mani_mysql;
					break;
				}		
			}

			delete mani_mysql;
			break;
		}
	}

	if (MANI_ADMIN_TYPE == type)
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Admin group [%s] updated", param1);
	}
	else
	{
		ComputeImmunityLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity group [%s] updated", param1);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command RemoveGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveGroupType
(
 int	type,
 player_t *player_ptr, 
 char *param1
 )
{
	int		group_index = -1;
	char	flag_type[32];
	bool	insert_required = false;
	char	group_id[128]="";

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	// See if group already exists
	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < admin_group_list_size; i++)
		{
			if (FStrEq(admin_group_list[i].group_id, param1))
			{
				group_index = i;
				RemoveIndexFromList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size, i, (void *) &(admin_group_list[i]), (void *) &(admin_group_list[admin_group_list_size - 1]));
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < immunity_group_list_size; i++)
		{
			if (FStrEq(immunity_group_list[i].group_id, param1))
			{
				group_index = i;
				RemoveIndexFromList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size, i, (void *) &(immunity_group_list[i]), (void *) &(immunity_group_list[immunity_group_list_size - 1]));
				break;
			}
		}
	}

	if (group_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group [%s] does not exist !!", param1);
		return;
	}

	for (int i = 0; i < client_list_size; i++)
	{
		if (MANI_ADMIN_TYPE == type)
		{
			for (int j = 0; j < client_list[i].admin_group_list_size; j++)
			{
				if (FStrEq(client_list[i].admin_group_list[j].group_id, param1))
				{
					RemoveIndexFromList((void **) &(client_list[i].admin_group_list), sizeof(group_t), &(client_list[i].admin_group_list_size), j,
						(void *) &(client_list[i].admin_group_list[j]), (void *) &(client_list[i].admin_group_list[client_list[i].admin_group_list_size - 1]));
				}
			}
		}
		else
		{
			for (int j = 0; j < client_list[i].immunity_group_list_size; j++)
			{
				if (FStrEq(client_list[i].immunity_group_list[j].group_id, param1))
				{
					RemoveIndexFromList((void **) &(client_list[i].immunity_group_list), sizeof(group_t), &(client_list[i].immunity_group_list_size), j,
												(void *) &(client_list[i].immunity_group_list[j]), (void *) &(client_list[i].immunity_group_list[client_list[i].immunity_group_list_size - 1]));

				}
			}
		}
	}

	for (int i = 0; i < client_list_size; i++)
	{
		RebuildFlags(&(client_list[i]), type);
	}

	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		for (;;)
		{
			// Add client to database too 
			ManiMySQL *mani_mysql = new ManiMySQL();

			if (!mani_mysql->Init())
			{
				delete mani_mysql;
				break; 
			}

			// Drop group id reference
			if (!mani_mysql->ExecuteQuery(
				"DELETE FROM %s%s "
				"WHERE group_id = '%s' "
				"AND type = '%s' "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBGroup(),
				param1,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				break;
			}

			if (!mani_mysql->ExecuteQuery(
				"DELETE FROM %s%s "
				"WHERE group_id = '%s' "
				"AND type = '%s' "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				param1,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{	
				delete mani_mysql;
				break;
			}		

			delete mani_mysql;
			break;
		}
	}

	if (MANI_ADMIN_TYPE == type)
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Admin group [%s] updated", param1);
	}
	else
	{
		ComputeImmunityLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity group [%s] updated", param1);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command AddLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddLevelType
(
 int	type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int		level_index = -1;
	int		level_id = 0;
	char	flag_type[32];
	bool	insert_required = false;

	level_id = Q_atoi(param1);

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	// See if level already exists
	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < admin_level_list_size; i++)
		{
			if (admin_level_list[i].level_id == level_id)
			{
				level_index = i;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < immunity_level_list_size; i++)
		{
			if (immunity_level_list[i].level_id == level_id)
			{
				level_index = i;
				break;
			}
		}
	}

	if (level_index == -1)
	{
		insert_required = true;

		if (MANI_ADMIN_TYPE == type)
		{
			AddToList((void **) &admin_level_list, sizeof(admin_level_t), &admin_level_list_size);
			for (int i = 0; i < MAX_ADMIN_FLAGS; i++) admin_level_list[admin_level_list_size - 1].flags[i] = false;
			admin_level_list[admin_level_list_size - 1].level_id = level_id;
			level_index = admin_level_list_size - 1;

		}
		else
		{
			AddToList((void **) &immunity_level_list, sizeof(immunity_level_t), &immunity_level_list_size);
			for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++) immunity_level_list[immunity_level_list_size - 1].flags[i] = false;
			immunity_level_list[immunity_level_list_size - 1].level_id = level_id;
			level_index = immunity_level_list_size - 1;
		}

	}

	int		param2_index = 0;
	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/remove all 
			// Valid flag index	given
			if (MANI_ADMIN_TYPE == type)
			{
				for (int x = 0; x < MAX_ADMIN_FLAGS; x ++)
				{
					admin_level_list[level_index].flags[x] = ((flag_add) ? true:false);
				}
			}
			else
			{
				for (int x = 0; x < MAX_IMMUNITY_FLAGS; x ++)
				{
					immunity_level_list[level_index].flags[x] = ((flag_add) ? true:false);
				}
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}

		int	flag_index = this->GetNextFlag(param2, &param2_index, type);
		if (flag_index != -1)
		{
			// Valid flag index	given
			if (MANI_ADMIN_TYPE	== type)
			{
				admin_level_list[level_index].flags[flag_index] = ((flag_add) ? true:false);
			}
			else
			{
				immunity_level_list[level_index].flags[flag_index] = ((flag_add) ? true:false);
			}
		}
	}

	WriteClients();

	if (!insert_required)
	{
		for (int i = 0; i < client_list_size; i++)
		{
			RebuildFlags(&(client_list[i]), type);
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		// Build the strings needed
		bool found_flag	= false;
		char	flag_string[1024];
		Q_strcpy (flag_string,"");

		if (MANI_ADMIN_TYPE	== type)
		{
			for	(int j = 0;	j <	MAX_ADMIN_FLAGS; j++)
			{
				char	temp_flag[20];

				if (admin_level_list[level_index].flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	admin_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}
		else
		{
			for	(int j = 0;	j <	MAX_IMMUNITY_FLAGS;	j++)
			{
				char	temp_flag[20];

				if (immunity_level_list[level_index].flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	immunity_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}

		for (;;)
		{
			// Add client to database too 
			ManiMySQL *mani_mysql = new ManiMySQL();

			if (!mani_mysql->Init())
			{
				delete mani_mysql;
				break; 
			}

			if (insert_required)
			{
				// Insert level id reference
				if (!mani_mysql->ExecuteQuery(
					"INSERT IGNORE INTO %s%s VALUES (%i, '%s', '%s', '%s')",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBLevel(),
					level_id,
					flag_type,
					flag_string,
					gpManiDatabase->GetServerGroupID()))
				{
					delete mani_mysql;
					break;
				}
			}
			else
			{
				if (!mani_mysql->ExecuteQuery(
					"UPDATE %s%s "
					"SET flag_string = '%s' "
					"WHERE level_id = %i "
					"AND type = '%s' "
					"AND server_group_id = '%s'",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBLevel(),
					flag_string,
					level_id,
					flag_type,
					gpManiDatabase->GetServerGroupID()))
				{	
					delete mani_mysql;
					break;
				}		
			}

			delete mani_mysql;
			break;
		}
	}

	if (MANI_ADMIN_TYPE == type)
	{
		qsort(admin_level_list, admin_level_list_size, sizeof(admin_level_t), sort_admin_level); 
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Admin level [%s] updated", param1);
	}
	else
	{
		qsort(immunity_level_list, immunity_level_list_size, sizeof(immunity_level_t), sort_immunity_level);
		ComputeImmunityLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity level [%s] updated", param1);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command RemoveLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveLevelType
(
 int	type,
 player_t *player_ptr,  
 char *param1
 )
{
	int		level_index = -1;
	char	flag_type[32];
	bool	insert_required = false;
	int		level_id = 0;

	level_id = Q_atoi(param1);

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	// See if level already exists
	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < admin_level_list_size; i++)
		{
			if (admin_level_list[i].level_id == level_id)
			{
				level_index = i;
				RemoveIndexFromList((void **) &admin_level_list, sizeof(admin_level_t), &admin_level_list_size, i,
					(void *) &(admin_level_list[i]), (void *) &(admin_level_list[admin_level_list_size - 1]));
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < immunity_level_list_size; i++)
		{
			if (immunity_level_list[i].level_id == level_id)
			{
				level_index = i;
				RemoveIndexFromList((void **) &immunity_level_list, sizeof(immunity_level_t), &immunity_level_list_size, i,
					(void *) &(immunity_level_list[i]), (void *) &(immunity_level_list[immunity_level_list_size - 1]));
				break;
			}
		}
	}

	if (level_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Level [%s] does not exist !!", param1);
		return;
	}

	for (int i = 0; i < client_list_size; i++)
	{
		RebuildFlags(&(client_list[i]), type);
	}

	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		for (;;)
		{
			// Add client to database too 
			ManiMySQL *mani_mysql = new ManiMySQL();

			if (!mani_mysql->Init())
			{
				delete mani_mysql;
				break; 
			}

			// Drop level id reference
			if (!mani_mysql->ExecuteQuery(
				"DELETE FROM %s%s "
				"WHERE level_id = %i "
				"AND type = '%s' "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBLevel(),
				level_id,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				break;
			}

			if (!mani_mysql->ExecuteQuery(
				"DELETE FROM %s%s "
				"WHERE level_id = %i "
				"AND type = '%s' "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				level_id,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{	
				delete mani_mysql;
				break;
			}		

			delete mani_mysql;
			break;
		}
	}

	if (MANI_ADMIN_TYPE == type)
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Admin level [%s] updated", param1);
	}
	else
	{
		ComputeImmunityLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity level [%s] updated", param1);
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetFlag
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetFlag
(
 int	type,
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;
	bool	found_group	= false;
	char	flag_type[32];
	int	param2_index = 0;
	ManiMySQL *mani_mysql =	NULL;


	client_index = FindClientIndex(param1, false, false);
	if (client_index ==	-1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable	to find	target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	if (MANI_ADMIN_TYPE	== type)
	{
		Q_strcpy(flag_type,	"Admin");
	}
	else
	{
		Q_strcpy(flag_type,	"Immunity");
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int	row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count,	
			"SELECT	c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id	= '%s' "
			"and cs.user_id	= c.user_id	"
			"and c.name	= '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User	ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id	= mani_mysql->GetInt(0);
		}
	}

	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/remove all 
			// Valid flag index	given
			if (MANI_ADMIN_TYPE == type)
			{
				for (int x = 0; x < MAX_ADMIN_FLAGS; x ++)
				{
					client_ptr->original_admin_flags[x] = ((flag_add) ? true:false);
				}
			}
			else
			{
				for (int x = 0; x < MAX_ADMIN_FLAGS; x ++)
				{
					client_ptr->original_immunity_flags[x]	= ((flag_add) ?	true:false);
				}
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}

		int	flag_index = this->GetNextFlag(param2, &param2_index, type);
		if (flag_index != -1)
		{
			// Valid flag index	given
			if (MANI_ADMIN_TYPE	== type)
			{
				client_ptr->original_admin_flags[flag_index] = ((flag_add) ? true:false);
			}
			else
			{
				client_ptr->original_immunity_flags[flag_index]	= ((flag_add) ?	true:false);
			}
		}
	}

	if (mani_mysql)
	{
		// Remove flags
		if (mani_mysql &&
			!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i	"
			"AND type =	'%s' "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientFlag(),
			client_ptr->user_id,
			flag_type,
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Build the strings needed
		bool found_flag	= false;
		char	flag_string[1024];
		Q_strcpy (flag_string,"");

		if (MANI_ADMIN_TYPE	== type)
		{
			for	(int j = 0;	j <	MAX_ADMIN_FLAGS; j++)
			{
				char	temp_flag[20];

				if (client_ptr->original_admin_flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	admin_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}
		else
		{
			for	(int j = 0;	j <	MAX_IMMUNITY_FLAGS;	j++)
			{
				char	temp_flag[20];

				if (client_ptr->original_immunity_flags[j])
				{
					Q_snprintf(temp_flag, sizeof(temp_flag), "%s ",	immunity_flag_list[j].flag);
					Q_strcat(flag_string, temp_flag);
					found_flag = true;
				}
			}
		}

		if (found_flag)
		{
			// Add them
			if (mani_mysql &&
				!mani_mysql->ExecuteQuery("INSERT IGNORE INTO %s%s	VALUES (%i,'%s','%s','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientFlag(),
				client_ptr->user_id,
				flag_string,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}
	}

	WriteClients();
	RebuildFlags(client_ptr, type);

	if (mani_mysql)
	{
		delete mani_mysql;
	}

	if (MANI_ADMIN_TYPE	== type) 
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Processed admin flags to client [%s]",	client_ptr->name);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Processed immunity flags to client [%s]", client_ptr->name);
		ComputeImmunityLevels();
	}

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddClient
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveClient
(
 player_t *player_ptr,  
 char *param1
 )
{
	client_t	*client_ptr;
	int		client_index;
	ManiMySQL *mani_mysql = NULL;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientServer(),
				client_ptr->user_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			row_count = 0;
			if (mani_mysql && 
				!mani_mysql->ExecuteQuery(&row_count,
				"SELECT 1 FROM %s%s "
				"WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientServer(),
				client_ptr->user_id))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (row_count == 0)
			{
				// No client server records so delete unique client
				if (mani_mysql && 
					!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
					"WHERE user_id = %i",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBClient(),
					client_ptr->user_id))
				{
					delete mani_mysql;
					mani_mysql = NULL;
				}
			}				

			// Delete rest of the crap
			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientFlag(),
				client_ptr->user_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				client_ptr->user_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND server_group_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				client_ptr->user_id,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i ",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBSteam(),
				client_ptr->user_id))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i ",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBIP(),
				client_ptr->user_id))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}

			if (mani_mysql && 
				!mani_mysql->ExecuteQuery("DELETE FROM %s%s"
				"WHERE user_id = %i ",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBNick(),
				client_ptr->user_id))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}

		if (mani_mysql)
		{
			delete mani_mysql;
		}
	}


	FreeClient(client_ptr);
	RemoveIndexFromList((void **) &client_list, sizeof(client_t), &client_list_size, client_index,
		(void *) &(client_list[client_index]), (void *) &(client_list[client_list_size - 1]));
	WriteClients();
	ComputeAdminLevels();
	ComputeImmunityLevels();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has been removed !!", param1);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveSteam
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveSteam
(
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	client_t	*client_ptr;
	int		client_index;
	ManiMySQL *mani_mysql = NULL;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	for (int i = 0; i < client_ptr->steam_list_size; i++)
	{
		if (FStrEq(client_ptr->steam_list[i].steam_id, param2))
		{
			RemoveIndexFromList((void **) &(client_ptr->steam_list), sizeof(steam_t), &client_ptr->steam_list_size, i,
				(void *) &(client_ptr->steam_list[i]), (void *) &(client_ptr->steam_list[client_ptr->steam_list_size - 1]));
			break;
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND steam_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBSteam(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}

		if (mani_mysql)
		{
			delete mani_mysql;
		}
	}

	WriteClients();
	ComputeAdminLevels();
	ComputeImmunityLevels();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had steam id [%s] removed", client_ptr->name, param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveIP
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveIP
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	client_t	*client_ptr;
	int		client_index;
	ManiMySQL *mani_mysql = NULL;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	for (int i = 0; i < client_ptr->ip_address_list_size; i++)
	{
		if (FStrEq(client_ptr->ip_address_list[i].ip_address, param2))
		{
			RemoveIndexFromList((void **) &(client_ptr->ip_address_list), sizeof(ip_address_t), &client_ptr->ip_address_list_size, i,
				(void *) &(client_ptr->ip_address_list[i]), &(client_ptr->ip_address_list[client_ptr->ip_address_list_size - 1]));
			break;
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND ip_address = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBIP(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}

		if (mani_mysql)
		{
			delete mani_mysql;
		}
	}

	WriteClients();
	ComputeAdminLevels();
	ComputeImmunityLevels();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had IP Address [%s] removed", client_ptr->name, param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveNick
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveNick
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	client_t	*client_ptr;
	int		client_index;
	ManiMySQL *mani_mysql = NULL;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	for (int i = 0; i < client_ptr->nick_list_size; i++)
	{
		if (FStrEq(client_ptr->nick_list[i].nick, param2))
		{
			RemoveIndexFromList((void **) &(client_ptr->nick_list), sizeof(nick_t), &client_ptr->nick_list_size, i,
				(void *) &(client_ptr->nick_list[i]), (void *) &(client_ptr->nick_list[client_ptr->nick_list_size - 1]));
			break;
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND nick = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBNick(),
				client_ptr->user_id,
				param2))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}

		if (mani_mysql)
		{
			delete mani_mysql;
		}
	}

	WriteClients();
	ComputeAdminLevels();
	ComputeImmunityLevels();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had nickname [%s] removed", client_ptr->name, param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveGroup
(
 int type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	client_t	*client_ptr;
	bool	found_group = false;
	char	flag_type[32];
	ManiMySQL *mani_mysql = NULL;

	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	if (MANI_ADMIN_TYPE == type)
	{
		Q_strcpy(flag_type, "Admin");
	}
	else
	{
		Q_strcpy(flag_type, "Immunity");
	}

	if (MANI_ADMIN_TYPE == type)
	{
		for (int i = 0; i < client_ptr->admin_group_list_size; i++)
		{
			if (FStrEq(client_ptr->admin_group_list[i].group_id, param2))
			{
				RemoveIndexFromList((void **) &(client_ptr->admin_group_list), sizeof(group_t), &client_ptr->admin_group_list_size, i,
					(void *) &(client_ptr->admin_group_list[i]), (void *) &(client_ptr->admin_group_list[client_ptr->admin_group_list_size - 1]));
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < client_ptr->immunity_group_list_size; i++)
		{
			if (FStrEq(client_ptr->immunity_group_list[i].group_id, param2))
			{
				RemoveIndexFromList((void **) &(client_ptr->immunity_group_list), sizeof(group_t), &client_ptr->immunity_group_list_size, i,
					(void *) &(client_ptr->immunity_group_list[i]), (void *) &(client_ptr->immunity_group_list[client_ptr->immunity_group_list_size - 1]));
				break;
			}
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		mani_mysql = new ManiMySQL();

		if (!mani_mysql->Init())
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		int row_count;

		// Get client for this server
		if (mani_mysql && !mani_mysql->ExecuteQuery(&row_count, 
			"SELECT c.user_id "
			"FROM %s%s c, %s%s cs "
			"where cs.server_group_id = '%s' "
			"and cs.user_id = c.user_id "
			"and c.name = '%s'",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetServerGroupID(),
			client_ptr->name))
		{
			delete mani_mysql;
			mani_mysql = NULL;
		}

		// Get User ID
		if (mani_mysql && row_count)
		{
			mani_mysql->FetchRow();
			client_ptr->user_id = mani_mysql->GetInt(0);

			if (!mani_mysql->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND group_id = '%s' "
				"AND type = '%s' "
				"AND server_group_id = '%s' ",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				client_ptr->user_id,
				param2,
				flag_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				mani_mysql = NULL;
			}
		}

		if (mani_mysql)
		{
			delete mani_mysql;
		}
	}

	WriteClients();
	RebuildFlags(client_ptr, type);


	if (mani_mysql)
	{
		delete mani_mysql;
	}

	if (MANI_ADMIN_TYPE == type)
	{
		ComputeAdminLevels();
		OutputHelpText(ORANGE_CHAT, player_ptr, "Removed client [%s] from admin flag group [%s]", client_ptr->name, param2);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Removed client [%s] from immunity flag group [%s]", client_ptr->name, param2);
		ComputeImmunityLevels();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAllClientFlagDesc
(
 int	type,
 player_t *player_ptr
 )
{
	if (type == MANI_ADMIN_TYPE)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", admin_flag_list[i].flag, admin_flag_list[i].flag_desc);
		}
	}
	else
	{
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", immunity_flag_list[i].flag, immunity_flag_list[i].flag_desc);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientFlagDesc
(
 int	type,
 player_t *player_ptr, 
 char *param1
 )
{
	if (type == MANI_ADMIN_TYPE)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			if (FStruEq(admin_flag_list[i].flag, param1))
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", admin_flag_list[i].flag, admin_flag_list[i].flag_desc);
				return;
			}

		}

		OutputHelpText(ORANGE_CHAT, player_ptr, "Admin flag [%s] does not exist !!", param1);
	}
	else
	{
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			if (FStruEq(immunity_flag_list[i].flag, param1))
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", admin_flag_list[i].flag, admin_flag_list[i].flag_desc);
				return;
			}
		}

		OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity flag [%s] does not exist !!", param1);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAllClientStatus
(
 player_t *player_ptr
 )
{
	if (client_list_size == 0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "No clients setup yet !!");
		return;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "List of clients, use ma_client status <name> for detailed info on a client");
	for (int i = 0; i < client_list_size; i++)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", client_list[i].name);
	}	
}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientStatus
(
 player_t *player_ptr,  
 char *param1 // player
 )
{
	int	client_index;
	client_t	*client_ptr;
	char	temp_string[8192];
	char	temp_string2[512];


	client_index = FindClientIndex(param1, false, false);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = &(client_list[client_index]);

	OutputHelpText(ORANGE_CHAT, player_ptr, "Name              : %s", client_ptr->name);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Email             : %s", client_ptr->email_address);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Notes             : %s", client_ptr->notes);
	OutputHelpText(ORANGE_CHAT, player_ptr, "Password          : %s", client_ptr->password);
	if (client_ptr->admin_level_id > -1) OutputHelpText(ORANGE_CHAT, player_ptr, "Admin Level ID    : %i", client_ptr->admin_level_id);
	if (client_ptr->immunity_level_id > -1) OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity Level ID : %i", client_ptr->immunity_level_id);

	if (client_ptr->steam_list_size != 0)
	{
		if (client_ptr->steam_list_size == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Steam ID : %s", client_ptr->steam_list[0].steam_id);
		}
		else
		{
			Q_strcpy(temp_string, "Steam IDs : ");
			for (int i = 0; i < client_ptr->steam_list_size; i ++)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", client_ptr->steam_list[i].steam_id);
				Q_strcat(temp_string, temp_string2);
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
		}
	}

	if (client_ptr->ip_address_list_size != 0)
	{
		if (client_ptr->ip_address_list_size == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "IP Address : %s", client_ptr->ip_address_list[0].ip_address);
		}
		else
		{
			Q_strcpy(temp_string, "IP Addresses : ");
			for (int i = 0; i < client_ptr->ip_address_list_size; i ++)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", client_ptr->ip_address_list[i].ip_address);
				Q_strcat(temp_string, temp_string2);
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
		}
	}

	if (client_ptr->nick_list_size != 0)
	{
		if (client_ptr->nick_list_size == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Nickname : %s", client_ptr->nick_list[0].nick);
		}
		else
		{
			Q_strcpy(temp_string, "Nicknames : ");
			for (int i = 0; i < client_ptr->nick_list_size; i ++)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", client_ptr->nick_list[i].nick);
				Q_strcat(temp_string, temp_string2);
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
		}
	}

	if (client_ptr->admin_group_list_size != 0)
	{
		if (client_ptr->admin_group_list_size == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Admin Group : %s", client_ptr->admin_group_list[0].group_id);
		}
		else
		{
			Q_strcpy(temp_string, "Admin Groups : ");
			for (int i = 0; i < client_ptr->admin_group_list_size; i ++)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", client_ptr->admin_group_list[i].group_id);
				Q_strcat(temp_string, temp_string2);
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
		}
	}

	if (client_ptr->immunity_group_list_size != 0)
	{
		if (client_ptr->immunity_group_list_size == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Immunity Group : %s", client_ptr->immunity_group_list[0].group_id);
		}
		else
		{
			Q_strcpy(temp_string, "Immunity Groups : ");
			for (int i = 0; i < client_ptr->immunity_group_list_size; i ++)
			{
				Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", client_ptr->immunity_group_list[i].group_id);
				Q_strcat(temp_string, temp_string2);
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
		}
	}

	bool found_flag = false;

	Q_strcpy(temp_string, "Admin Flags : ");
	for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
	{
		if (client_ptr->original_admin_flags[i])
		{
			Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", admin_flag_list[i].flag);
			Q_strcat(temp_string, temp_string2);
			found_flag = true;
		}
	}

	if (found_flag)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	found_flag = false;
	Q_strcpy(temp_string, "Immunity Flags : ");
	for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
	{
		if (client_ptr->original_immunity_flags[i])
		{
			Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", immunity_flag_list[i].flag);
			Q_strcat(temp_string, temp_string2);
			found_flag = true;
		}
	}

	if (found_flag)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	Q_strcpy(temp_string, "Active Admin Flags : ");
	for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
	{
		if (client_ptr->admin_flags[i])
		{
			Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", admin_flag_list[i].flag);
			Q_strcat(temp_string, temp_string2);
			found_flag = true;
		}
	}

	if (found_flag)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	found_flag = false;
	Q_strcpy(temp_string, "Active Immunity Flags : ");
	for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
	{
		if (client_ptr->immunity_flags[i])
		{
			Q_snprintf(temp_string2, sizeof(temp_string2), "%s ", immunity_flag_list[i].flag);
			Q_strcat(temp_string, temp_string2);
			found_flag = true;
		}
	}

	if (found_flag)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientGroupStatus
(
 player_t *player_ptr, 
 char *param1 // type
 )
{
	char	flags_string[1024] = "";

	if (FStrEq(param1, "agroup"))
	{
		if (admin_group_list_size == 0)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "No Admin Groups setup");
		}
		else 
		{
			for (int i = 0; i < admin_group_list_size; i++)
			{
				char temp_flag[20];
				Q_strcpy(flags_string,"");

				for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
				{
					if (admin_group_list[i].flags[j])
					{
						Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", admin_flag_list[j].flag);
						Q_strcat(flags_string, temp_flag);
					}
				}

				OutputHelpText(ORANGE_CHAT, player_ptr, "[%s]   %s", admin_group_list[i].group_id, flags_string);
			}
		}
	}
	else if (FStrEq(param1, "igroup"))
	{
		if (immunity_group_list_size == 0)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "No Immunity Groups setup");
		}
		else
		{
			for (int i = 0; i < immunity_group_list_size; i++)
			{
				char temp_flag[20];
				Q_strcpy(flags_string,"");
				for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
				{
					if (immunity_group_list[i].flags[j])
					{
						Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", immunity_flag_list[j].flag);
						Q_strcat(flags_string, temp_flag);
					}
				}

				OutputHelpText(ORANGE_CHAT, player_ptr, "[%s]   %s", immunity_group_list[i].group_id, flags_string);
			}
		}
	}
	else if (FStrEq(param1, "alevel"))
	{
		if (admin_level_list_size == 0)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "No Admin Levels setup");
		}
		else
		{
			for (int i = 0; i < admin_level_list_size; i++)
			{
				char temp_flag[20];
				Q_strcpy(flags_string,"");
				for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
				{
					if (admin_level_list[i].flags[j])
					{
						Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", admin_flag_list[j].flag);
						Q_strcat(flags_string, temp_flag);
					}
				}

				OutputHelpText(ORANGE_CHAT, player_ptr, "[%i]   %s", admin_level_list[i].level_id, flags_string);
			}
		}
	}
	else if (FStrEq(param1, "ilevel"))
	{
		if (immunity_level_list_size == 0)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "No Immunity Levels setup");
		}
		else
		{
			for (int i = 0; i < immunity_level_list_size; i++)
			{
				char temp_flag[20];
				Q_strcpy(flags_string,"");
				for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
				{
					if (immunity_level_list[i].flags[j])
					{
						Q_snprintf(temp_flag, sizeof(temp_flag), "%s ", immunity_flag_list[j].flag);
						Q_strcat(flags_string, temp_flag);
					}
				}

				OutputHelpText(ORANGE_CHAT, player_ptr, "[%i]   %s", immunity_level_list[i].level_id, flags_string);
			}
		}	
	}
	else
	{
		gpManiHelp->ShowHelp(player_ptr, "ma_clientgroup", 2223, M_CCONSOLE);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Upload
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientUpload
(
 player_t *player_ptr
 )
{

	if (!gpManiDatabase->GetDBEnabled())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot upload as database functionality not enabled, see database.txt");
		return;
	}

	// Upload to the database

	OutputHelpText(ORANGE_CHAT, player_ptr, "Uploading data.....");
	if (this->CreateDBTables())
	{
		if (this->CreateDBFlags())
		{
			this->ExportDataToDB();
			OutputHelpText(ORANGE_CHAT, player_ptr, "Upload suceeded");
			return;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Upload failed !!");

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Download
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientDownload
(
 player_t *player_ptr
 )
{

	if (!gpManiDatabase->GetDBEnabled())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot download as database functionality not enabled, see database.txt");
		return;
	}

	// Upload to the database

	FreeClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Downloading data.....");
	if (!this->GetClientsFromDatabase())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Download failed, using existing clients.txt file instead");
		FreeClients();
		LoadClients();
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Download succeeded, updating clients.txt");
		WriteClients();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
void		ManiClient::UpgradeDB1( void )
{
	bool update_db = false;
	char version_string[16];
	int	row_count = 0;

	if (!gpManiDatabase->GetDBEnabled())
	{
		return;
	}

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init())
	{
		delete mani_mysql;
		return;
	}

	if (!mani_mysql->ExecuteQuery(&row_count, 
		"SELECT v.version_id "
		"FROM %s%s v",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion()))
	{
		delete mani_mysql;
		return;
	}

	if (row_count != 0)
	{
		// Found rows
		while (mani_mysql->FetchRow())
		{
			Q_strcpy(version_string, mani_mysql->GetString(0));

			if (Q_strlen(version_string) < 8)
			{
				delete mani_mysql;
				return;
			}

			char v_letter = version_string[7];

			if (v_letter == 'A' || 
				v_letter == 'B' ||
				v_letter == 'C' ||
				v_letter == 'D' ||
				v_letter == 'E' ||
				v_letter == 'F' ||
				v_letter == 'G' ||
				v_letter == 'H' ||
				v_letter == 'I' ||
				v_letter == 'J' ||
				v_letter == 'K' ||
				v_letter == 'L')
			{
				update_db = true; 
			}
	
			break;
		}
	}

	if (!update_db) 
	{
		delete mani_mysql;
		return;
	}

	MMsg("Updating database from pre V%s to new format\n", version_string);

	bool column_exists;

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBGroup(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBGroup())) {delete mani_mysql; return;}

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientGroup(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientGroup())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientFlag(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientFlag())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientLevel(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientLevel())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBLevel(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBLevel())) {delete mani_mysql; return;}

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientServer(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientServer())) {delete mani_mysql; return;}

	bool column_matches;

	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientFlag(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientFlag())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientGroup(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientGroup())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientLevel(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientLevel())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBGroup(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBGroup())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBLevel(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBLevel())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBFlag(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBFlag())) {delete mani_mysql; return;}

	MMsg("Updating table %s%s to have column 'server_group_id'....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBServer(),"server_group_id", &column_exists)){delete mani_mysql; return;}
	if (!column_exists)
	{
		MMsg("Updating table %s%s to have column 'server_group_id'....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
		if (!mani_mysql->ExecuteQuery("ALTER TABLE %s%s ADD server_group_id varchar(32) NOT NULL default ''", 
			gpManiDatabase->GetDBTablePrefix(),	gpManiDatabase->GetDBTBServer()))
		{
			return ;
		}
	}

	MMsg("Updating table %s%s to have 'Default' for server group id....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET server_group_id = 'Default'", 
		gpManiDatabase->GetDBTablePrefix(),	gpManiDatabase->GetDBTBServer()))
	{
		return ;
	}

	MMsg("Updating stored database version ID to %s....\n",  PLUGIN_VERSION_ID2);
	if (!mani_mysql->ExecuteQuery("UPDATE %s%s SET version_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion(),
			PLUGIN_VERSION_ID2))
	{
		delete mani_mysql;
		return;
	}

	MMsg("Update completed successfully!\n", version_string);

	delete mani_mysql;
}

//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
bool		ManiClient::UpgradeServerIDToServerGroupID
(
 ManiMySQL *mani_mysql_ptr,
 const char	*table_name
 )
{
	MMsg("Updating 'server_id' to 'server_group_id' on table '%s%s'....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery("ALTER TABLE %s%s CHANGE server_id server_group_id varchar(32) NOT NULL default ''", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Defaulting 'server_group_id' to 'Default' on table '%s%s'....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery("UPDATE %s%s t1 SET t1.server_group_id = 'Default'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Test a column exists
//---------------------------------------------------------------------------------
bool		ManiClient::TestColumnExists(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, bool *found_column)
{
	int row_count;

	MMsg("Testing column '%s' exists on table '%s%s'....\n", column_name, gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(&row_count, "SHOW COLUMNS FROM %s%s LIKE '%s'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name, column_name))
	{
		*found_column = false;
		return false;
	}

	if (row_count != 0)
	{
		MMsg("Column exists\n");
		*found_column = true;
	}
	else
	{
		MMsg("Column does not exist\n");
		*found_column = false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Test a column type matches our one
//---------------------------------------------------------------------------------
bool		ManiClient::TestColumnType(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, char *column_type, bool *column_matches)
{
	int row_count;

	MMsg("Testing column type '%s' matches column '%s' on table '%s%s'....\n", column_type, column_name, gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(&row_count, "SHOW COLUMNS FROM %s%s LIKE '%s'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name, column_name))
	{
		*column_matches = false;
		return false;
	}

	if (row_count == 0)
	{
		return false;
	}

	mani_mysql_ptr->FetchRow();
	if (FStrEq(mani_mysql_ptr->GetString(1), column_type))
	{
		*column_matches = true;
		return true;
	}

	*column_matches = true;
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
bool		ManiClient::UpgradeClassTypes
(
 ManiMySQL *mani_mysql_ptr,
 const char	*table_name
 )
{
	MMsg("Updating 'type' to be varchar(32) on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery("ALTER TABLE %s%s CHANGE type type varchar(32) NOT NULL default ''", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Updating 'A' type to be 'Admin' on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery("UPDATE %s%s t1 SET t1.type = 'Admin' where type = 'A'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Updating 'I' type to be 'Immunity' on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery("UPDATE %s%s t1 SET t1.type = 'Immunity' where type = 'I'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------

SCON_COMMAND(ma_client, 2221, MaClient, true);

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaClientGroup(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	int	client_index;
	bool	invalid_args = false;

	if (player_ptr)
	{
		// Check if player is admin
		if (!IsAdminAllowed(player_ptr, command_name, ALLOW_CLIENT_ADMIN, war_mode, &client_index)) return PLUGIN_STOP;
	}

	int argc = gpCmd->Cmd_Argc();

	// Cover only command and subcommand being passed in
	if (argc < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	char *sub_command = (char *) gpCmd->Cmd_Argv(1);
	char *param1 = (char *) gpCmd->Cmd_Argv(2);
	char *param2 = (char *) gpCmd->Cmd_Argv(3);

	// Parse sub command
	if (FStrEq(sub_command, "addagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroupType(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroupType(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addalevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddLevelType(MANI_ADMIN_TYPE, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addilevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddLevelType(MANI_IMMUNITY_TYPE, player_ptr, param1, param2);
	}	
	else if (FStrEq(sub_command, "removeagroup"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveGroupType(MANI_ADMIN_TYPE, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removeigroup"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveGroupType(MANI_IMMUNITY_TYPE, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removealevel"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveLevelType(MANI_ADMIN_TYPE, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removeilevel"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveLevelType(MANI_IMMUNITY_TYPE, player_ptr, param1);
	}	
	else if (FStrEq(sub_command, "status"))
	{
		if (argc == 3)
		{
			ProcessClientGroupStatus(player_ptr, param1);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else
	{
		invalid_args = true;
	}

	if (invalid_args)
	{
		gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_clientgroup, 2223, MaClientGroup, true);

//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_setadminflag, 0, MaSetAdminFlag, true);

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaReloadClients(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	int	client_index;
	bool	invalid_args = false;

	if (player_ptr)
	{
		// Check if player is admin
		if (!IsAdminAllowed(player_ptr, command_name, ALLOW_CLIENT_ADMIN, war_mode, &client_index)) return PLUGIN_STOP;
	}

	this->Init();
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for re loading admin lists
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_reloadclients, 2167, MaReloadClients, true);

//---------------------------------------------------------------------------------
// Purpose: Sets up immunity flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
void	ManiClient::InitImmunityFlags(void)
{
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag, IMMUNITY_ALLOW_GIMP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag_desc,IMMUNITY_ALLOW_GIMP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_KICK].flag, IMMUNITY_ALLOW_KICK_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_KICK].flag_desc,IMMUNITY_ALLOW_KICK_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag, IMMUNITY_ALLOW_SLAY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag_desc,IMMUNITY_ALLOW_SLAY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BAN].flag, IMMUNITY_ALLOW_BAN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BAN].flag_desc,IMMUNITY_ALLOW_BAN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag, IMMUNITY_ALLOW_CEXEC_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag_desc,IMMUNITY_ALLOW_CEXEC_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag, IMMUNITY_ALLOW_BLIND_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag_desc,IMMUNITY_ALLOW_BLIND_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag, IMMUNITY_ALLOW_SLAP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag_desc,IMMUNITY_ALLOW_SLAP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag, IMMUNITY_ALLOW_FREEZE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag_desc,IMMUNITY_ALLOW_FREEZE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag, IMMUNITY_ALLOW_TELEPORT_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag_desc,IMMUNITY_ALLOW_TELEPORT_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag, IMMUNITY_ALLOW_DRUG_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag_desc,IMMUNITY_ALLOW_DRUG_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag, IMMUNITY_ALLOW_SWAP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag_desc,IMMUNITY_ALLOW_SWAP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TAG].flag, IMMUNITY_ALLOW_TAG_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TAG].flag_desc,IMMUNITY_ALLOW_TAG_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag, IMMUNITY_ALLOW_BALANCE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag_desc,IMMUNITY_ALLOW_BALANCE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BURN].flag, IMMUNITY_ALLOW_BURN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BURN].flag_desc,IMMUNITY_ALLOW_BURN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag, IMMUNITY_ALLOW_MUTE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag_desc,IMMUNITY_ALLOW_MUTE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag, IMMUNITY_ALLOW_RESERVE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag_desc,IMMUNITY_ALLOW_RESERVE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag, IMMUNITY_ALLOW_SETSKIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag_desc,IMMUNITY_ALLOW_SETSKIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag, IMMUNITY_ALLOW_RESERVE_SKIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag_desc,IMMUNITY_ALLOW_RESERVE_SKIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag, IMMUNITY_ALLOW_TIMEBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag_desc,IMMUNITY_ALLOW_TIMEBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag, IMMUNITY_ALLOW_FIREBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag_desc,IMMUNITY_ALLOW_FIREBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag, IMMUNITY_ALLOW_FREEZEBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag_desc,IMMUNITY_ALLOW_FREEZEBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag, IMMUNITY_ALLOW_BEACON_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag_desc,IMMUNITY_ALLOW_BEACON_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag, IMMUNITY_ALLOW_GHOST_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag_desc,IMMUNITY_ALLOW_GHOST_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag, IMMUNITY_ALLOW_GIVE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag_desc,IMMUNITY_ALLOW_GIVE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag, IMMUNITY_ALLOW_COLOUR_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag_desc, IMMUNITY_ALLOW_COLOUR_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BASIC_IMMUNITY].flag, IMMUNITY_ALLOW_BASIC_IMMUNITY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BASIC_IMMUNITY].flag_desc, IMMUNITY_ALLOW_BASIC_IMMUNITY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GRAVITY].flag, IMMUNITY_ALLOW_GRAVITY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GRAVITY].flag_desc, IMMUNITY_ALLOW_GRAVITY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AUTOJOIN].flag, IMMUNITY_ALLOW_AUTOJOIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AUTOJOIN].flag_desc, IMMUNITY_ALLOW_AUTOJOIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AFK].flag, IMMUNITY_ALLOW_AFK_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AFK].flag_desc, IMMUNITY_ALLOW_AFK_DESC);

}


//---------------------------------------------------------------------------------
// Purpose: Sets up admin flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
void	ManiClient::InitAdminFlags(void)
{
	Q_strcpy(admin_flag_list[ALLOW_GIMP].flag, ALLOW_GIMP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GIMP].flag_desc,ALLOW_GIMP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_KICK].flag, ALLOW_KICK_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_KICK].flag_desc,ALLOW_KICK_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON].flag, ALLOW_RCON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON].flag_desc,ALLOW_RCON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON_MENU].flag, ALLOW_RCON_MENU_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON_MENU].flag_desc,ALLOW_RCON_MENU_DESC);

	Q_strcpy(admin_flag_list[ALLOW_EXPLODE].flag, ALLOW_EXPLODE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_EXPLODE].flag_desc,ALLOW_EXPLODE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SLAY].flag, ALLOW_SLAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SLAY].flag_desc,ALLOW_SLAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BAN].flag, ALLOW_BAN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BAN].flag_desc,ALLOW_BAN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SAY].flag, ALLOW_SAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SAY].flag_desc,ALLOW_SAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CHAT].flag, ALLOW_CHAT_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CHAT].flag_desc,ALLOW_CHAT_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PSAY].flag, ALLOW_PSAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PSAY].flag_desc,ALLOW_PSAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CHANGEMAP].flag, ALLOW_CHANGEMAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CHANGEMAP].flag_desc,ALLOW_CHANGEMAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PLAYSOUND].flag, ALLOW_PLAYSOUND_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PLAYSOUND].flag_desc,ALLOW_PLAYSOUND_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RESTRICTWEAPON].flag, ALLOW_RESTRICTWEAPON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RESTRICTWEAPON].flag_desc,ALLOW_RESTRICTWEAPON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CONFIG].flag, ALLOW_CONFIG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CONFIG].flag_desc,ALLOW_CONFIG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CEXEC].flag, ALLOW_CEXEC_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CEXEC].flag_desc,ALLOW_CEXEC_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CEXEC_MENU].flag, ALLOW_CEXEC_MENU_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CEXEC_MENU].flag_desc,ALLOW_CEXEC_MENU_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BLIND].flag, ALLOW_BLIND_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BLIND].flag_desc,ALLOW_BLIND_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SLAP].flag, ALLOW_SLAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SLAP].flag_desc,ALLOW_SLAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FREEZE].flag, ALLOW_FREEZE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FREEZE].flag_desc,ALLOW_FREEZE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_TELEPORT].flag, ALLOW_TELEPORT_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_TELEPORT].flag_desc,ALLOW_TELEPORT_DESC);

	Q_strcpy(admin_flag_list[ALLOW_DRUG].flag, ALLOW_DRUG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_DRUG].flag_desc,ALLOW_DRUG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SWAP].flag, ALLOW_SWAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SWAP].flag_desc,ALLOW_SWAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag, ALLOW_RCON_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag_desc,ALLOW_RCON_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MENU_RCON_VOTE].flag, ALLOW_MENU_RCON_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MENU_RCON_VOTE].flag_desc,ALLOW_MENU_RCON_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag, ALLOW_RANDOM_MAP_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag_desc,ALLOW_RANDOM_MAP_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MAP_VOTE].flag, ALLOW_MAP_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MAP_VOTE].flag_desc,ALLOW_MAP_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_QUESTION_VOTE].flag, ALLOW_QUESTION_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_QUESTION_VOTE].flag_desc,ALLOW_QUESTION_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag, ALLOW_MENU_QUESTION_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag_desc,ALLOW_MENU_QUESTION_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CANCEL_VOTE].flag, ALLOW_CANCEL_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CANCEL_VOTE].flag_desc,ALLOW_CANCEL_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_ACCEPT_VOTE].flag, ALLOW_ACCEPT_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_ACCEPT_VOTE].flag_desc,ALLOW_ACCEPT_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MA_RATES].flag, ALLOW_MA_RATES_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MA_RATES].flag_desc,ALLOW_MA_RATES_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BURN].flag, ALLOW_BURN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BURN].flag_desc,ALLOW_BURN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_NO_CLIP].flag, ALLOW_NO_CLIP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_NO_CLIP].flag_desc,ALLOW_NO_CLIP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_WAR].flag, ALLOW_WAR_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_WAR].flag_desc,ALLOW_WAR_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MUTE].flag, ALLOW_MUTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MUTE].flag_desc,ALLOW_MUTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RESET_ALL_RANKS].flag, ALLOW_RESET_ALL_RANKS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RESET_ALL_RANKS].flag_desc,ALLOW_RESET_ALL_RANKS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CASH].flag, ALLOW_CASH_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CASH].flag_desc,ALLOW_CASH_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCONSAY].flag, ALLOW_RCONSAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCONSAY].flag_desc,ALLOW_RCONSAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SKINS].flag, ALLOW_SKINS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SKINS].flag_desc,ALLOW_SKINS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SETSKINS].flag, ALLOW_SETSKINS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SETSKINS].flag_desc,ALLOW_SETSKINS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_DROPC4].flag, ALLOW_DROPC4_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_DROPC4].flag_desc,ALLOW_DROPC4_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SETADMINFLAG].flag, ALLOW_SETADMINFLAG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SETADMINFLAG].flag_desc,ALLOW_SETADMINFLAG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_COLOUR].flag, ALLOW_COLOUR_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_COLOUR].flag_desc,ALLOW_COLOUR_DESC);

	Q_strcpy(admin_flag_list[ALLOW_TIMEBOMB].flag, ALLOW_TIMEBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_TIMEBOMB].flag_desc,ALLOW_TIMEBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FIREBOMB].flag, ALLOW_FIREBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FIREBOMB].flag_desc,ALLOW_FIREBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FREEZEBOMB].flag, ALLOW_FREEZEBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FREEZEBOMB].flag_desc,ALLOW_FREEZEBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_HEALTH].flag, ALLOW_HEALTH_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_HEALTH].flag_desc,ALLOW_HEALTH_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BEACON].flag, ALLOW_BEACON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BEACON].flag_desc,ALLOW_BEACON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_GIVE].flag, ALLOW_GIVE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GIVE].flag_desc,ALLOW_GIVE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BASIC_ADMIN].flag, ALLOW_BASIC_ADMIN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BASIC_ADMIN].flag_desc,ALLOW_BASIC_ADMIN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CLIENT_ADMIN].flag, ALLOW_CLIENT_ADMIN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CLIENT_ADMIN].flag_desc,ALLOW_CLIENT_ADMIN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PERM_BAN].flag, ALLOW_PERM_BAN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PERM_BAN].flag_desc,ALLOW_PERM_BAN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SPRAY_TAG].flag, ALLOW_SPRAY_TAG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SPRAY_TAG].flag_desc,ALLOW_SPRAY_TAG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_GRAVITY].flag, ALLOW_GRAVITY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GRAVITY].flag_desc,ALLOW_GRAVITY_DESC);

}

static int sort_admin_level ( const void *m1,  const void *m2) 
{
	struct admin_level_t *mi1 = (struct admin_level_t *) m1;
	struct admin_level_t *mi2 = (struct admin_level_t *) m2;
	return mi1->level_id - mi2->level_id;
}

static int sort_immunity_level ( const void *m1,  const void *m2) 
{
	struct immunity_level_t *mi1 = (struct immunity_level_t *) m1;
	struct immunity_level_t *mi2 = (struct immunity_level_t *) m2;
	return mi1->level_id - mi2->level_id;
}

static int sort_player_admin_level ( const void *m1,  const void *m2) 
{
	struct player_level_t *mi1 = (struct player_level_t *) m1;
	struct player_level_t *mi2 = (struct player_level_t *) m2;
	return mi1->level_id - mi2->level_id;
}

ManiClient	g_ManiClient;
ManiClient	*gpManiClient;


/*
select c.user_id, c.name, cf.flag_string, cf.type, s.steam_id, n.nick, i.ip_address, cg.group_id, cg.type, cl.level_id, cl.type
from map_client c, map_client_server cs
LEFT JOIN map_steam s ON s.user_id = c.user_id
LEFT JOIN map_client_flag cf ON cf.user_id = c.user_id
LEFT JOIN map_nick n ON n.user_id = c.user_id
LEFT JOIN map_ip i ON i.user_id = c.user_id
LEFT JOIN map_client_group cg ON cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id
LEFT JOIN map_client_level cl ON cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id
where c.user_id = cs.user_id
and cs.server_group_id = 1
order by 1, 4, 5,6,7,9, 8, 11, 10
*/
