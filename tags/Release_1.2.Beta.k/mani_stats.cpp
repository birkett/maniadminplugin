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
#include "Color.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "bitbuf.h"
#include "mani_main.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_parser.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_maps.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_stats.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerPluginHelpers *helpers;
extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	IFileSystem	*filesystem;
extern	bool war_mode;
extern	int	max_players;
extern	bool just_loaded;

static	rank_t	**rank_player_list = NULL;
static	rank_t	**rank_player_name_list = NULL;
static	rank_t	**rank_player_waiting_list = NULL;
static	rank_t	**rank_player_name_waiting_list = NULL;
static	rank_t	**rank_list = NULL;
static	rank_t	**rank_name_list = NULL;

static	int	rank_list_size = 0;
static	int	rank_name_list_size = 0;
static	int	rank_player_list_size = 0;
static	int	rank_player_name_list_size = 0;
static	int	rank_player_waiting_list_size = 0;
static	int	rank_player_name_waiting_list_size = 0;


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static int sort_by_steam_id ( const void *m1,  const void *m2); 
static int sort_by_name ( const void *m1,  const void *m2); 

static int sort_by_kills ( const void *m1,  const void *m2); 
static int sort_by_kd_ratio ( const void *m1,  const void *m2);
static int sort_by_kills_deaths ( const void *m1,  const void *m2);

static int sort_by_name_kills ( const void *m1,  const void *m2); 
static int sort_by_name_kd_ratio ( const void *m1,  const void *m2);
static int sort_by_name_kills_deaths ( const void *m1,  const void *m2);

//---------------------------------------------------------------------------------
// Purpose: Load Stats into memory
//---------------------------------------------------------------------------------
void	LoadStats(void)
{
	if (mani_stats.GetInt() != 1) return;

	if (rank_player_list_size == 0)
	{
		// Try and read the stats file
		ReadStats();
	}

	if (rank_player_name_list_size == 0)
	{
		// Try and read the stats by name file
		ReadNameStats();
	}

	if (!just_loaded)
	{
//		Msg("Discarding players who haven't connected for stats in %i days\n", mani_stats_drop_player_days.GetInt());
	}

	ReBuildStatsPlayerList();
	ReBuildStatsPlayerNameList();
	CalculateStats(false);
	CalculateNameStats(false);
	WriteStats();
	WriteNameStats();

}

//---------------------------------------------------------------------------------
// Purpose: Free stats lists
//---------------------------------------------------------------------------------
void	FreeStats(void)
{
	for (int i = 0; i < rank_player_list_size; i++) free (rank_player_list[i]);
	for (int i = 0; i < rank_player_name_list_size; i++) free (rank_player_name_list[i]);
	for (int i = 0; i < rank_player_waiting_list_size; i++) free (rank_player_waiting_list[i]);
	for (int i = 0; i < rank_player_name_waiting_list_size; i++) free (rank_player_name_waiting_list[i]);
	for (int i = 0; i < rank_list_size; i++) free (rank_list[i]);
	for (int i = 0; i < rank_name_list_size; i++) free (rank_name_list[i]);

	FreeList((void **) &rank_player_list, &rank_player_list_size);
	FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);
	FreeList((void **) &rank_player_waiting_list, &rank_player_waiting_list_size);
	FreeList((void **) &rank_player_name_waiting_list, &rank_player_name_waiting_list_size);
	FreeList((void **) &rank_list, &rank_list_size);
	FreeList((void **) &rank_name_list, &rank_name_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Say string to specific teams
//---------------------------------------------------------------------------------
void AddPlayerToRankList
(
player_t	*player
)
{
	rank_t	rank_key;
	rank_t	*player_found = NULL;
	time_t	current_time;
	rank_t	*rank_key_address;
	rank_t	**player_found_address;

	if (FStrEq(player->steam_id,"BOT"))
	{
		// Is Bot
		return;
	}

	// Do BSearch for steam ID in ranked player list
	Q_strcpy(rank_key.steam_id, player->steam_id);
	rank_key_address = &rank_key;

	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_list, 
						rank_player_list_size, 
						sizeof(rank_t *), 
						sort_by_steam_id
						);

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		// Found player so update their details
		time(&current_time);
		Q_strcpy(player_found->name, player->name);
		player_found->last_connected = current_time;
		return;
	}

	// Do BSearch for steam ID in waiting player list in case they re-joined,
	// this stops duplicates entering the list
	Q_strcpy(rank_key.steam_id, player->steam_id);
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_waiting_list, 
						rank_player_waiting_list_size, 
						sizeof(rank_t *), 
						sort_by_steam_id
						);

	if (player_found_address != NULL)
	{
		// Found player so update their details
		player_found = *player_found_address;
		time(&current_time);
		Q_strcpy(player_found->name, player->name);
		player_found->last_connected = current_time;
		return;
	}

	// Player not in list so Add
	time(&current_time);
	AddToList((void **) &rank_player_waiting_list, sizeof(rank_t *), &rank_player_waiting_list_size);
	rank_player_waiting_list[rank_player_waiting_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));

	rank_t *rank_ptr = rank_player_waiting_list[rank_player_waiting_list_size - 1];
	memset((void *) rank_ptr, 0, sizeof(rank_t));

	Q_strcpy(rank_ptr->steam_id, player->steam_id);
	Q_strcpy(rank_ptr->name, player->name);
	rank_ptr->kills = 0;
	rank_ptr->deaths = 0;
	rank_ptr->suicides = 0;
	rank_ptr->headshots = 0;
	rank_ptr->kd_ratio = 0.0;
	rank_ptr->last_connected = current_time;
	rank_ptr->rank = -1;
	
	qsort(rank_player_waiting_list, rank_player_waiting_list_size, sizeof(rank_t *), sort_by_steam_id); 
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Add Player Name to Rank List
//---------------------------------------------------------------------------------
void AddPlayerNameToRankList
(
player_t	*player
)
{
	rank_t	rank_key;
	rank_t	*player_found = NULL;
	time_t	current_time;
	rank_t	*rank_key_address;
	rank_t	**player_found_address;

	if (FStrEq(player->steam_id,"BOT"))
	{
		// Is Bot
		return;
	}

	// Do BSearch for Name in ranked player list
	Q_strcpy(rank_key.name, player->name);
	rank_key_address = &rank_key;

	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_name_list, 
						rank_player_name_list_size, 
						sizeof(rank_t *), 
						sort_by_name
						);

	if (player_found_address != NULL)
	{
		// Found player so update their details
		player_found = *player_found_address;
		time(&current_time);
		player_found->last_connected = current_time;
		return;
	}

	// Do BSearch for Name in waiting player list in case they re-joined,
	// this stops duplicates entering the list
	Q_strcpy(rank_key.name, player->name);
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_name_waiting_list, 
						rank_player_name_waiting_list_size, 
						sizeof(rank_t *), 
						sort_by_name
						);

	if (player_found_address != NULL)
	{
		// Found player so update their details
		player_found = *player_found_address;
		time(&current_time);
		player_found->last_connected = current_time;
		return;
	}

	// Player not in list so Add
	time(&current_time);
	AddToList((void **) &rank_player_name_waiting_list, sizeof(rank_t *), &rank_player_name_waiting_list_size);
	rank_player_name_waiting_list[rank_player_name_waiting_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));

	rank_t *rank_ptr = rank_player_name_waiting_list[rank_player_name_waiting_list_size - 1];
	memset((void *) rank_ptr, 0, sizeof(rank_t));

	Q_strcpy(rank_ptr->steam_id, player->steam_id);
	Q_strcpy(rank_ptr->name, player->name);
	rank_ptr->kills = 0;
	rank_ptr->deaths = 0;
	rank_ptr->suicides = 0;
	rank_ptr->headshots = 0;
	rank_ptr->kd_ratio = 0.0;
	rank_ptr->last_connected = current_time;
	rank_ptr->rank = -1;
	
	qsort(rank_player_name_waiting_list, rank_player_name_waiting_list_size, sizeof(rank_t *), sort_by_name); 

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void CalculateStats(bool round_end)
{
	// Do BSearch for steam ID
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	**player_found;

	for (int i = 0; i < rank_player_waiting_list_size; i++)
	{
		AddToList((void **) &rank_player_list, sizeof(rank_t *), &rank_player_list_size);
		rank_player_list[rank_player_list_size - 1] = rank_player_waiting_list[i];
	}

	if (rank_player_waiting_list_size != 0)
	{
//		Msg("Added %i new players to the ranks\n", rank_player_waiting_list_size);
		// Re-sort the list
		qsort(rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id); 
		FreeList((void **) &rank_player_waiting_list, &rank_player_waiting_list_size);
	}

	// If not in per round mode, return
	if (round_end)
	{
		if (mani_stats_mode.GetInt() == 0)
		{
			return;
		}
	}

	if (rank_player_list_size == 0)
	{
		return;
	}

	for (int i = 0; i < rank_list_size; i++) 
	{
		free (rank_list[i]);
	}

	FreeList((void **) &rank_list, &rank_list_size);

	for (int i = 0; i < rank_player_list_size; i++)
	{
		if (mani_stats_kills_required.GetInt() > rank_player_list[i]->kills)
		{
			rank_player_list[i]->rank = -1;
			continue;
		}

		AddToList((void **) &rank_list, sizeof(rank_t *), &rank_list_size);
		rank_list[rank_list_size - 1] = (rank_t *) malloc(sizeof(rank_t));
		*(rank_list[rank_list_size - 1]) = *(rank_player_list[i]);
	}

	if (mani_stats_calculate.GetInt() == 0)
	{
		// By kills
		qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kills); 
	}
	else if (mani_stats_calculate.GetInt() == 1)
	{
		// By kd ratio
		qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kd_ratio); 
	}
	else 
	{
		// By Kills - Deaths
		qsort(rank_list, rank_list_size, sizeof(rank_t *), sort_by_kills_deaths); 
	}

	// Update player ranks
	for (int i = 0; i < rank_list_size; i++)
	{
		rank_list[i]->rank = i + 1;
		Q_strcpy(rank_key.steam_id, rank_list[i]->steam_id);
		rank_key_address = &rank_key;
		player_found = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_list, 
						rank_player_list_size, 
						sizeof(rank_t *), 
						sort_by_steam_id
						);
		if (player_found == NULL)
		{
			continue;
		}

		(*(rank_t **) player_found)->rank = i + 1;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Calculate Name stats
//---------------------------------------------------------------------------------
void CalculateNameStats(bool round_end)
{
	// Do BSearch for Name
	rank_t	rank_key;
	rank_t  *rank_key_address;
	rank_t	**player_found;

	for (int i = 0; i < rank_player_name_waiting_list_size; i++)
	{
		AddToList((void **) &rank_player_name_list, sizeof(rank_t *), &rank_player_name_list_size);
		rank_player_name_list[rank_player_name_list_size - 1] = rank_player_name_waiting_list[i];
	}

	if (rank_player_name_waiting_list_size != 0)
	{
//		Msg("Added %i new players to the name ranks\n", rank_player_name_waiting_list_size);
		// Re-sort the list
		qsort(rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name); 
		FreeList((void **) &rank_player_name_waiting_list, &rank_player_name_waiting_list_size);
	}

	// If not in per round mode, return
	if (round_end)
	{
		if (mani_stats_mode.GetInt() == 0)
		{
			return;
		}
	}

	if (rank_player_name_list_size == 0)
	{
		return;
	}

	for (int i = 0; i < rank_name_list_size; i++) 
	{
		free (rank_name_list[i]);
	}

	FreeList((void **) &rank_name_list, &rank_name_list_size);

	for (int i = 0; i < rank_player_name_list_size; i++)
	{
		if (mani_stats_kills_required.GetInt() > rank_player_name_list[i]->kills)
		{
			rank_player_name_list[i]->rank = -1;
			continue;
		}

		AddToList((void **) &rank_name_list, sizeof(rank_t *), &rank_name_list_size);
		rank_name_list[rank_name_list_size - 1] = (rank_t *) malloc(sizeof(rank_t));
		*(rank_name_list[rank_name_list_size - 1]) = *(rank_player_name_list[i]);
	}

	if (mani_stats_calculate.GetInt() == 0)
	{
		// By kills
		qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kills); 
	}
	else if (mani_stats_calculate.GetInt() == 1)
	{
		// By kd ratio
		qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kd_ratio); 
	}
	else 
	{
		// By Kills - Deaths
		qsort(rank_name_list, rank_name_list_size, sizeof(rank_t *), sort_by_name_kills_deaths); 
	}

	// Update player ranks
	for (int i = 0; i < rank_name_list_size; i++)
	{
		rank_name_list[i]->rank = i + 1;
		Q_strcpy(rank_key.name, rank_name_list[i]->name);
		rank_key_address = &rank_key;
		player_found = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_name_list, 
						rank_player_name_list_size, 
						sizeof(rank_t *), 
						sort_by_name
						);

		if (player_found == NULL)
		{
			continue;
		}

		(*(rank_t **) player_found)->rank = i + 1;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Rebuild the player list
//---------------------------------------------------------------------------------
void ReBuildStatsPlayerList(void)
{
	rank_t	**temp_player_list=NULL;
	int	temp_player_list_size=0;

	time_t	current_time;
	time(&current_time);

	// Rebuild list
	for (int i = 0; i < rank_player_list_size; i++)
	{
		if (!just_loaded && rank_player_list[i]->last_connected + (mani_stats_drop_player_days.GetInt() * 86400) < current_time)
		{
			// Player outside of time limit so drop them
			free (rank_player_list[i]);
			continue;
		}

		if (FStrEq(rank_player_list[i]->steam_id, "BOT"))
		{
			// Remove bots if they are in there
			free(rank_player_list[i]);
			continue;
		}

		AddToList((void **) &temp_player_list, sizeof(rank_t *), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = rank_player_list[i];
	}


	// Free the old list
	FreeList((void **) &rank_player_list, &rank_player_list_size);

	// Reset pointer for new list
	rank_player_list = temp_player_list;
	rank_player_list_size = temp_player_list_size;

	qsort(rank_player_list, rank_player_list_size, sizeof(rank_t *), sort_by_steam_id);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Rebuild the player list
//---------------------------------------------------------------------------------
void ReBuildStatsPlayerNameList(void)
{
	rank_t	**temp_player_list=NULL;
	int	temp_player_list_size=0;

	time_t	current_time;
	time(&current_time);

	// Rebuild list
	for (int i = 0; i < rank_player_name_list_size; i++)
	{
		if (!just_loaded && rank_player_name_list[i]->last_connected + (mani_stats_drop_player_days.GetInt() * 86400) < current_time)
		{
			// Player outside of time limit so drop them
			free (rank_player_name_list[i]);
			continue;
		}

		if (FStrEq(rank_player_name_list[i]->steam_id, "BOT"))
		{
			// Remove bots if they are in there
			free (rank_player_name_list[i]);
			continue;
		}

		AddToList((void **) &temp_player_list, sizeof(rank_t *), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = rank_player_name_list[i];
	}


	// Free the old list
	FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);

	// Reset pointer for new list
	rank_player_name_list = temp_player_list;
	rank_player_name_list_size = temp_player_list_size;

	qsort(rank_player_name_list, rank_player_name_list_size, sizeof(rank_t *), sort_by_name);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ProcessDeathStats(IGameEvent *event)
{
	// Do BSearch for steam ID
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found;
	rank_t	**player_found_address;

	int		headshot;
	player_t	victim;
	player_t	attacker;

	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);
	headshot = event->GetInt("headshot", -1);

//	Msg("Victim [%i] Attacker [%i] Headshot [%i]\n", victim.user_id, attacker.user_id, headshot);
	if (attacker.user_id == 0)
	{
		// World attacked
		return;
	}

	if (!FindPlayerByUserID(&attacker))
	{
		return;
	}

	if (!FindPlayerByUserID(&victim))
	{
		return;
	}

	// Ignore kills made by bots if set to do so
	if ((victim.is_bot || attacker.is_bot) && mani_stats_include_bot_kills.GetInt() == 0)
	{
		return;
	}

	bool suicide = false;
	if (victim.user_id == attacker.user_id)
	{
		// Suicide, don't register
		suicide = true;
	}

	Q_strcpy(rank_key.steam_id, attacker.steam_id);
	rank_key_address = &rank_key;
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_list, 
						rank_player_list_size, 
						sizeof(rank_t *), 
						sort_by_steam_id
						);

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		if (suicide == false)
		{
			if (attacker.team != victim.team || !gpManiGameType->IsTeamPlayAllowed())
			{
				// Not a team attack or not in team play mode
				if (headshot == 1)
				{
					player_found->headshots ++;
				}

				player_found->kills++;
				if (player_found->deaths == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = player_found->kills;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
			else
			{
				// This was a tk by the attacker, so bump up their deaths instead
				player_found->deaths ++;
				if (player_found->kills == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = 0;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
		}

		// Update name
		Q_strcpy(player_found->name, attacker.name);
	}
	
	Q_strcpy(rank_key.steam_id, victim.steam_id);
	rank_key_address = &rank_key;
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_list, 
						rank_player_list_size, 
						sizeof(rank_t *), 
						sort_by_steam_id
						);

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;

		if (suicide)
		{
			player_found->suicides ++;
		}
		else
		{
			if (attacker.team != victim.team || !gpManiGameType->IsTeamPlayAllowed())
			{
				player_found->deaths ++;
				if (player_found->kills == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = 0;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
		}

		// Update name
		Q_strcpy(player_found->name, victim.name);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ProcessNameDeathStats(IGameEvent *event)
{
	// Do BSearch for steam ID
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found;
	rank_t	**player_found_address;

	int		headshot;
	player_t	victim;
	player_t	attacker;

	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);
	headshot = event->GetInt("headshot", -1);

//	Msg("Victim [%i] Attacker [%i] Headshot [%i]\n", victim.user_id, attacker.user_id, headshot);
	if (attacker.user_id == 0)
	{
		// World attacked
		return;
	}

	if (!FindPlayerByUserID(&attacker))
	{
		return;
	}

	if (!FindPlayerByUserID(&victim))
	{
		return;
	}

	bool suicide = false;
	if (victim.user_id == attacker.user_id)
	{
		// Suicide, don't register
		suicide = true;
	}

	Q_strcpy(rank_key.name, attacker.name);
	rank_key_address = &rank_key;
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_name_list, 
						rank_player_name_list_size, 
						sizeof(rank_t *), 
						sort_by_name
						);

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		if (suicide == false)
		{
			if (attacker.team != victim.team || !gpManiGameType->IsTeamPlayAllowed())
			{
				// Not a team attack
				if (headshot == 1)
				{
					player_found->headshots ++;
				}

				player_found->kills++;
				if (player_found->deaths == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = player_found->kills;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
			else
			{
				// This was a tk by the attacker, so bump up their deaths instead
				player_found->deaths ++;
				if (player_found->kills == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = 0;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
		}
	}
	
	Q_strcpy(rank_key.name, victim.name);
	rank_key_address = &rank_key;
	player_found_address = (rank_t **) bsearch
						(
						&rank_key_address, 
						rank_player_name_list, 
						rank_player_name_list_size, 
						sizeof(rank_t *), 
						sort_by_name
						);

	if (player_found_address != NULL)
	{
		player_found = *player_found_address;
		if (suicide)
		{
			player_found->suicides ++;
		}
		else
		{
			if (attacker.team != victim.team || !gpManiGameType->IsTeamPlayAllowed())
			{
				player_found->deaths ++;
				if (player_found->kills == 0)
				{
					// Prevent math error div 0
					player_found->kd_ratio = 0;
				}
				else
				{
					player_found->kd_ratio = (float) ((float) player_found->kills / (float) player_found->deaths);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ShowRank(player_t *player)
{
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found;
	rank_t	**player_found_address = NULL;

	char	output_string[512];

	rank_key_address = &rank_key;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID
		Q_strcpy(rank_key.steam_id, player->steam_id);

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_list, 
							rank_player_list_size, 
							sizeof(rank_t *), 
							sort_by_steam_id
							);
	}
	else
	{
		// Do BSearch for name
		Q_strcpy(rank_key.name, player->name);

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_name_list, 
							rank_player_name_list_size, 
							sizeof(rank_t *), 
							sort_by_name
							);
	}

	if (player_found_address == NULL)
	{
		// Player not ranked yet
		if (mani_stats_mode.GetInt() == 1 && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			SayToPlayer(player, "You are not ranked yet, wait until the next round and try again");
		}
		else
		{
			SayToPlayer(player, "You are not ranked yet, wait until the next map and try again");
		}

		return;
	}

	player_found = *player_found_address;

	// Found player so update their details
	int	number_of_ranks;
	if (mani_stats_by_steam_id.GetInt() == 1) 
	{
		Q_strcpy(player_found->name, player->name);
		number_of_ranks = rank_list_size;
	}
	else
	{
		number_of_ranks = rank_name_list_size;
	}

	if (player_found->rank == -1)
	{
		Q_snprintf(output_string, sizeof(output_string), "Player %s is not ranked yet. %i kill%s, %i death%s, kd ratio %.2f",
															player->name,
															player_found->kills,
															(player_found->kills == 1) ? "":"s",
															player_found->deaths,
															(player_found->deaths == 1) ? "":"s",
															player_found->kd_ratio);

	}
	else
	{
		Q_snprintf(output_string, sizeof(output_string), "Player %s is ranked %i/%ld with %i kill%s, %i death%s, kd ratio %.2f",
															player->name,
															player_found->rank,
															number_of_ranks,
															player_found->kills,
															(player_found->kills == 1) ? "":"s",
															player_found->deaths,
															(player_found->deaths == 1) ? "":"s",
															player_found->kd_ratio);
	}

	if (mani_stats_show_rank_to_all.GetInt() == 1)
	{
		if (player->is_dead)
		{
			SayToDead("%s", output_string);
		}
		else
		{
			SayToAll(false, "%s", output_string);
		}
	}
	else
	{
		SayToPlayer(player, "%s", output_string);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ShowTop(player_t *player ,int number_of_ranks)
{
	char	menu_string[2048];
	char	player_string[512];
	char	player_name[15];
	char	rank_output[2048];

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		if (rank_list_size == 0) return;
	}
	else
	{
		if (rank_name_list_size == 0) return;
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		if (mani_stats_calculate.GetInt() == 0)
		{
			Q_snprintf(menu_string, sizeof(menu_string), "Top %i by K\nK=Kills, D=Deaths, KDR=KD Ratio\n_\n", 
													number_of_ranks);
		}
		else if (mani_stats_calculate.GetInt() == 1)
		{
			Q_snprintf(menu_string, sizeof(menu_string), "Top %i by KDR\nK=Kills, D=Deaths, KDR=KD Ratio\n_\n", 
													number_of_ranks);
		}
		else
		{
			Q_snprintf(menu_string, sizeof(menu_string), "Top %i by K-D\nK=Kills, D=Deaths, KDR=KD Ratio\n_\n", 
													number_of_ranks);
		}

		DrawMenu (player->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
	}
	else
	{
		// Escape style menu has more space so elaborate
		if (mani_stats_calculate.GetInt() == 0)
		{
			Q_snprintf(rank_output, sizeof(rank_output), "Top %i by K\nK=Kills, D=Deaths, KDR=KD Ratio\n\n", 
													number_of_ranks);
		}
		else if (mani_stats_calculate.GetInt() == 1)
		{
			Q_snprintf(rank_output, sizeof(rank_output), "Top %i by KDR\nK=Kills, D=Deaths, KDR=KD Ratio\n\n", 
													number_of_ranks);
		}
		else
		{
			Q_snprintf(rank_output, sizeof(rank_output), "Top %i by K-D\nK=Kills, D=Deaths, KDR=KD Ratio\n\n", 
													number_of_ranks);
		}
	}

//	Msg("Rank list size = %ld\n", rank_list_size);

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		for (int i = 0; i < number_of_ranks; i++)
		{

			if (mani_stats_by_steam_id.GetInt() == 1)
			{
				if (i == (int) rank_list_size)
				{
					break;
				}

				Q_strncpy(player_name, rank_list[i]->name, sizeof(player_name));
				Q_snprintf(player_string, sizeof(player_string), "%-2i %s K=%i D=%i KDR=%.2f\n",
														i + 1,
														player_name,
														rank_list[i]->kills,
														rank_list[i]->deaths,
														rank_list[i]->kd_ratio
														);
			}
			else
			{
				if (i == (int) rank_name_list_size)
				{
					break;
				}

				Q_strncpy(player_name, rank_name_list[i]->name, sizeof(player_name));
				Q_snprintf(player_string, sizeof(player_string), "%-2i %s K=%i D=%i KDR=%.2f\n",
														i + 1,
														player_name,
														rank_name_list[i]->kills,
														rank_name_list[i]->deaths,
														rank_name_list[i]->kd_ratio
														);
			}
	
			DrawMenu (player->index, mani_stats_top_display_time.GetInt(), 7, true, true, true, player_string, false);
		}

		DrawMenu (player->index, mani_stats_top_display_time.GetInt(), 7, true, true, true, "\n", true);
		menu_confirm[player->index - 1].in_use = false;
	}
	else
	{
		for (int i = 0; i < number_of_ranks; i++)
		{

			if (mani_stats_by_steam_id.GetInt() == 1)
			{
				if (i == (int) rank_list_size)
				{
					break;
				}

				Q_snprintf(player_string, sizeof(player_string), "%-2i %s K=%i D=%i KDR=%.2f\n",
														i + 1,
														rank_list[i]->name,
														rank_list[i]->kills,
														rank_list[i]->deaths,
														rank_list[i]->kd_ratio
														);
			}
			else
			{
				if (i == (int) rank_name_list_size)
				{
					break;
				}

				Q_snprintf(player_string, sizeof(player_string), "%-2i %s K=%i D=%i KDR=%.2f\n",
														i + 1,
														rank_name_list[i]->name,
														rank_name_list[i]->kills,
														rank_name_list[i]->deaths,
														rank_name_list[i]->kd_ratio
														);
			}
	
			Q_strcat(rank_output, player_string);
		}

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "Press Esc to see ranks" );
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "msg", rank_output);
		helpers->CreateMessage( player->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
	}

}

//---------------------------------------------------------------------------------
// Purpose: Show Top x Players
//---------------------------------------------------------------------------------
/*void ShowTop(player_t *player ,int number_of_ranks)
{
	char	player_string[512];
	char	rank_output[4096];
	char	rank_title[2048];

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		if (rank_list_size == 0) return;
	}
	else
	{
		if (rank_name_list_size == 0) return;
	}

	if (mani_stats_calculate.GetInt() == 0)
	{
		Q_snprintf(rank_title, sizeof(rank_title), "Top %i by pure kills", 
													number_of_ranks);
	}
	else if (mani_stats_calculate.GetInt() == 1)
	{
		Q_snprintf(rank_title, sizeof(rank_title), "Top %i by kill death ratio", 
													number_of_ranks);
	}
	else
	{
		Q_snprintf(rank_title, sizeof(rank_title), "Top %i by kills minus deaths", 
													number_of_ranks);
	}

	Q_strcat(rank_output, "<HTML>Rank  Player                         Kills Deaths Headshot KD Ratio\n");
	Q_strcat(rank_output, "-------------------------------------------------------------------\n");

	for (int i = 0; i < number_of_ranks; i++)
	{

		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			if (i == (int) rank_list_size)
			{
				break;
			}

			float headshot_percent = 0.0;

			if (rank_list[i].kills != 0 && rank_list[i].headshots != 0)
			{
				headshot_percent = ((float) rank_list[i].headshots / (float) rank_list[i].kills) * 100.0;
			}

			Q_snprintf(player_string, sizeof(player_string), "%-2i   %-30s %-6i %-7i %-.2f    %-.2f\n",
													i + 1,
													rank_list[i].name,
													rank_list[i].kills,
													rank_list[i].deaths,
													headshot_percent, 
													rank_list[i].kd_ratio
													);
		}
		else
		{
			if (i == (int) rank_name_list_size)
			{
				break;
			}

			float headshot_percent = 0.0;

			if (rank_name_list[i].kills != 0 && rank_name_list[i].headshots != 0)
			{
				headshot_percent = ((float) rank_name_list[i].headshots / (float) rank_name_list[i].kills) * 100.0;
			}

			Q_snprintf(player_string, sizeof(player_string), "%-2i   %-30s %-6i %-7i %-.2f    %-.2f\n",
													i + 1,
													rank_name_list[i].name,
													rank_name_list[i].kills,
													rank_name_list[i].deaths,
													rank_name_list[i].kd_ratio
													);
		}
	
		Q_strcat(rank_output, player_string);
	}

	Q_strcat(rank_output, "</HTML>");

	MRecipientFilter mrf;
	mrf.AddPlayer(player->index);
	DrawPanel(&mrf, rank_title, MANI_TOP_PANEL, rank_output, Q_strlen(rank_output));

}
*/
//---------------------------------------------------------------------------------
// Purpose: Calculate stats
//---------------------------------------------------------------------------------
void ShowStatsMe(player_t *player)
{
	char	menu_string[2048];
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found;
	rank_t	**player_found_address = NULL;

	rank_key_address = &rank_key;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID
		Q_strcpy(rank_key.steam_id, player->steam_id);
		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_list, 
							rank_player_list_size, 
							sizeof(rank_t *), 
							sort_by_steam_id
							);
	}
	else
	{
		// Do BSearch for Name
		Q_strcpy(rank_key.name, player->name);
		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_name_list, 
							rank_player_name_list_size, 
							sizeof(rank_t *), 
							sort_by_name
							);
	}

	if (player_found_address == NULL)
	{
		return;
	}
	
	player_found = *player_found_address;

	// Found player so update their details
	if (mani_stats_by_steam_id.GetInt() == 1) Q_strcpy(player_found->name, player->name);

	if (player_found->rank == -1)
	{
		Q_snprintf(menu_string, sizeof(menu_string), "Player %s\n____\nKills [%i]\nHeadshots [%i]\nDeaths [%i]\nKill Death Ratio [%.2f]\nSuicides [%i]\n", 
												player_found->name,
												player_found->kills,
												player_found->headshots,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides);
	}
	else
	{
		Q_snprintf(menu_string, sizeof(menu_string), "Player %s\n____\nRank [%i]\nKills  [%i]\nHeadshots  [%i]\nDeaths  [%i]\nKill Death Ratio  [%.2f]\nSuicides  [%i]\n", 
												player_found->name,
												player_found->rank,
												player_found->kills,
												player_found->headshots,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides);
	}

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawMenu (player->index, 10, 7, true, true, true, menu_string, true);
	}
	else
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "Press Esc to see stats" );
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "msg", menu_string);
		helpers->CreateMessage( player->entity, DIALOG_TEXT, kv, gpManiISPCCallback );
		kv->deleteThis();
	}

	menu_confirm[player->index - 1].in_use = false;

}

//---------------------------------------------------------------------------------
// Purpose: Show the Statistics of the player
//---------------------------------------------------------------------------------
/*
void ShowStatsMe(player_t *player)
{
	char	menu_string[2048];
	rank_t	rank_key;
	rank_t	*player_found = NULL;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID
		Q_strcpy(rank_key.steam_id, player->steam_id);
		player_found = (rank_t *) bsearch
							(
							&rank_key, 
							rank_player_list, 
							rank_player_list_size, 
							sizeof(rank_t), 
							sort_by_steam_id
							);
	}
	else
	{
		// Do BSearch for Name
		Q_strcpy(rank_key.name, player->name);
		player_found = (rank_t *) bsearch
							(
							&rank_key, 
							rank_player_name_list, 
							rank_player_name_list_size, 
							sizeof(rank_t), 
							sort_by_name
							);
	}

	if (player_found == NULL)
	{
		return;
	}
	
	float headshot_percent = 0.0;

	if (player_found->kills != 0 && player_found->headshots != 0)
	{
		headshot_percent = ((float) player_found->headshots / (float) player_found->kills) * 100.0;
	}

	// Found player so update their details
	if (mani_stats_by_steam_id.GetInt() == 1) Q_strcpy(player_found->name, player->name);

	if (player_found->rank == -1)
	{
		Q_snprintf(menu_string, sizeof(menu_string), "Player %s\n____\nKills  %i\nHeadshots  %.2f%%\nDeaths  %i\nKill Death Ratio  %.2f\nSuicides  %i\n", 
												player_found->name,
												player_found->kills,
												headshot_percent,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides);
	}
	else
	{
		Q_snprintf(menu_string, sizeof(menu_string), "Player %s\n____\nRank %i\nKills  %i\nHeadshots  %.2f%%\nDeaths  %i\nKill Death Ratio  %.2f\nSuicides  %i\n", 
												player_found->name,
												player_found->rank,
												player_found->kills,
												headshot_percent,
												player_found->deaths,
												player_found->kd_ratio,
												player_found->suicides);
	}

	MRecipientFilter mrf;
	mrf.AddPlayer(player->index);
	DrawPanel(&mrf, "Your Stats", MANI_STATSME_PANEL, menu_string, Q_strlen(menu_string));

}
*/
//---------------------------------------------------------------------------------
// Purpose: Reset the stats
//---------------------------------------------------------------------------------
void ResetStats(void)
{

	char	base_filename[512];

	// Delete the stats
	for (int i = 0; i < rank_player_list_size; i ++) free (rank_player_list[i]);
	for (int i = 0; i < rank_player_name_list_size; i ++) free (rank_player_name_list[i]);
	for (int i = 0; i < rank_list_size; i ++) free (rank_list[i]);

	FreeList((void **) &rank_player_list, &rank_player_list_size);
	FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);
	FreeList((void **) &rank_list, &rank_list_size);


	//Delete stats file
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_stats.dat", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_stats.dat exists, preparing to delete stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_stats.dat\n");
		}
	}

	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_name_stats.dat", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_name_stats.dat exists, preparing to delete stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_name_stats.dat\n");
		}
	}

	player_t	player;

	// Reload players that are on server
	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player))
		{
			continue;
		}

		if (player.is_bot)
		{
			continue;
		}

		// Don't add player if steam id is not confirmed
		if (mani_stats_by_steam_id.GetInt() == 1 && 
			FStrEq(player.steam_id, "STEAM_ID_PENDING"))
		{
			continue;
		}

		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			AddPlayerToRankList(&player);
		}
		else
		{
			AddPlayerNameToRankList(&player);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write the stats
//---------------------------------------------------------------------------------
void WriteStats(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	// Valves FPrintf is broken :/
	char	temp_string[2048];
	int temp_length;

	//Write stats to disk
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_stats.dat", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_stats.dat exists, preparing to delete then write new updated stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_stats.dat\n");
		}
	}

	file_handle = filesystem->Open (base_filename,"wb", NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to open mani_stats.dat for writing\n");
		return;
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), PLUGIN_VERSION_ID);

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
//		Msg ("Failed to write version info to mani_stats.dat\n");
		filesystem->Close(file_handle);
		return;
	}

	// Write Ranks
	for (int i = 0; i < rank_player_list_size; i ++)
	{
		if (filesystem->Write((void *) rank_player_list[i], sizeof(rank_t), file_handle) == 0)
		{
//			Msg("Failed to write rank player index %i to mani_stats.dat!!\n", i);
			filesystem->Close(file_handle);
			return;
		}
	}

//	Msg("Wrote %i ranked players to mani_stats.dat\n", rank_player_list_size);
	filesystem->Close(file_handle);

	if (mani_stats_write_text_file.GetInt() == 0)
	{
		return;
	}

	//Write text formate to disk
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_ranks.txt", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_ranks.txt exists, preparing to delete then write new updated stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_ranks.txt\n");
		}
	}

	file_handle = filesystem->Open(base_filename,"wb",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to open mani_ranks.txt for writing\n");
		return;
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), PLUGIN_VERSION_ID);
	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
//		Msg ("Failed to write version info to mani_ranks.txt\n");
		filesystem->Close(file_handle);
		return;
	}

	// Write Ranks in human readable text format
	for (int i = 0; i < rank_list_size; i ++)
	{
		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "<%i> <%s> <%s> <%i> <%i> <%i> <%i> <%.3f>\n",
												i + 1,
												rank_list[i]->name,
												rank_list[i]->steam_id,
												rank_list[i]->kills,
												rank_list[i]->deaths,
												rank_list[i]->suicides,
												rank_list[i]->headshots,
												rank_list[i]->kd_ratio);
		
		if (temp_length == 0)
		{
//			Msg("Failed to write rank index %i to mani_ranks.txt!!\n", i);
			filesystem->Close(file_handle);
			return;
		}

		if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
		{
//			Msg("Failed to write rank index %i to mani_ranks.txt!!\n", i);
			filesystem->Close(file_handle);
			return;
		}
	}

//	Msg("Wrote %i ranked players to mani_ranks.txt\n", rank_list_size);
	filesystem->Close(file_handle);

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Write the stats
//---------------------------------------------------------------------------------
void WriteNameStats(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	// Valves FPrintf is broken :/
	char	temp_string[2048];
	int temp_length;

	//Write stats to disk
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_name_stats.dat", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_name_stats.dat exists, preparing to delete then write new updated stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_name_stats.dat\n");
		}
	}

	file_handle = filesystem->Open (base_filename,"wb",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to open mani_name_stats.dat for writing\n");
		return;
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), PLUGIN_VERSION_ID);
	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
//		Msg ("Failed to write version info to mani_name_stats.dat\n");
		filesystem->Close(file_handle);
		return;
	}

	// Write Ranks
	for (int i = 0; i < rank_player_name_list_size; i ++)
	{
		if (filesystem->Write((void *) rank_player_name_list[i], sizeof(rank_t), file_handle) == 0)
		{
//			Msg("Failed to write rank player name index %i to mani_name_stats.dat!!\n", i);
			filesystem->Close(file_handle);
			return;
		}
	}

//	Msg("Wrote %i ranked players to mani_name_stats.dat\n", rank_player_name_list_size);
	filesystem->Close(file_handle);

	if (mani_stats_write_text_file.GetInt() == 0)
	{
		return;
	}

	//Write text formate to disk
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_name_ranks.txt", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
//		Msg("File mani_name_ranks.txt exists, preparing to delete then write new updated stats\n");
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
//			Msg("Failed to delete mani_name_ranks.txt\n");
		}
	}

	file_handle = filesystem->Open(base_filename,"wb",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to open mani_name_ranks.txt for writing\n");
		return;
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), PLUGIN_VERSION_ID);
	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
//		Msg ("Failed to write version info to mani_name_ranks.txt\n");
		filesystem->Close(file_handle);
		return;
	}

	// Write Ranks in human readable text format
	for (int i = 0; i < rank_name_list_size; i ++)
	{
		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "<%i> <%s> <%s> <%i> <%i> <%i> <%i> <%.3f>\n",
												i + 1,
												rank_name_list[i]->name,
												"N/A",
												rank_name_list[i]->kills,
												rank_name_list[i]->deaths,
												rank_name_list[i]->suicides,
												rank_name_list[i]->headshots,
												rank_name_list[i]->kd_ratio);
		
		if (temp_length == 0)
		{
//			Msg("Failed to write rank index %i to mani_name_ranks.txt!!\n", i);
			filesystem->Close(file_handle);
			return;
		}

		if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
		{
//			Msg("Failed to write rank index %i to mani_name_ranks.txt!!\n", i);
			filesystem->Close(file_handle);
			return;
		}
	}

//	Msg("Wrote %i ranked players to mani_name_ranks.txt\n", rank_name_list_size);
	filesystem->Close(file_handle);

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Read the stats
//---------------------------------------------------------------------------------
void ReadStats(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	stats_version[128];

	if (rank_player_list_size != 0)
	{
		return;
	}

	// Delete the stats just in case

	for (int i = 0; i < rank_player_list_size; i ++) free (rank_player_list[i]);
	for (int i = 0; i < rank_list_size; i ++) free (rank_list[i]);

	FreeList((void **) &rank_player_list, &rank_player_list_size);
	FreeList((void **) &rank_list, &rank_list_size);

//	Msg("Attempting to read mani_stats.dat file\n");

	//Get stats into memory
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_stats.dat", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rb",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to load mani_stats.dat, did not find file\n");
		return;
	}

	if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
	{
//		Msg("Failed to get version string for mani_stats.dat!!\n");
		filesystem->Close(file_handle);
		return;
	}

	if (!ParseLine(stats_version, true))
		{
//		Msg("Failed to get version string for mani_stats.dat, top line empty !!\n");
		filesystem->Close(file_handle);
		return;
		}

//	Msg("mani_stats.dat version [%s]\n",stats_version);

	rank_t player_rank;

	// Get ranks into memory
	while (filesystem->Read(&player_rank, sizeof(rank_t), file_handle) > 0)
	{
		AddToList((void **) &rank_player_list, sizeof(rank_t *), &rank_player_list_size);
		rank_player_list[rank_player_list_size - 1] = (rank_t *) malloc (sizeof(rank_t));
		*(rank_player_list[rank_player_list_size - 1]) = player_rank;
	}

//	Msg("Read %i player ranks into memory\n", rank_player_list_size);

	filesystem->Close(file_handle);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Read the named stats
//---------------------------------------------------------------------------------
void ReadNameStats(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	stats_version[128];

	if (rank_player_name_list_size != 0)
	{
		return;
	}

	for (int i = 0; i < rank_player_name_list_size; i ++) free (rank_player_name_list[i]);
	for (int i = 0; i < rank_name_list_size; i ++) free (rank_name_list[i]);

	// Delete the stats just in case
	FreeList((void **) &rank_player_name_list, &rank_player_name_list_size);
	FreeList((void **) &rank_name_list, &rank_name_list_size);

//	Msg("Attempting to read mani_name_stats.dat file\n");

	//Get stats into memory
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/mani_name_stats.dat", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rb",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to load mani_name_stats.dat, did not find file\n");
		return;
	}

	if (filesystem->ReadLine (stats_version, sizeof(stats_version) , file_handle) == NULL)
	{
//		Msg("Failed to get version string for mani_name_stats.dat!!\n");
		filesystem->Close(file_handle);
		return;
	}

	if (!ParseLine(stats_version, true))
		{
//		Msg("Failed to get version string for mani_name_stats.dat, top line empty !!\n");
		filesystem->Close(file_handle);
		return;
		}

//	Msg("mani_name_stats.dat version [%s]\n",stats_version);

	rank_t player_rank;

	// Get ranks into memory
	while (filesystem->Read(&player_rank, sizeof(rank_t), file_handle) > 0)
	{
		AddToList((void **) &rank_player_name_list, sizeof(rank_t *), &rank_player_name_list_size);
		rank_player_name_list[rank_player_name_list_size - 1] = (rank_t *) malloc(sizeof(rank_t));
		*(rank_player_name_list[rank_player_name_list_size - 1]) = player_rank;
	}

//	Msg("Read %i player name ranks into memory\n", rank_player_name_list_size);

	filesystem->Close(file_handle);
	return;
}



//---------------------------------------------------------------------------------
// Purpose: Process the ma_ranks command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaRanks
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *start_rank,
 char *end_rank
)
{
	player_t player;
	int	admin_index;
	int	start_index;
	int	end_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CONFIG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	int players_ranked = 0;
	time_t current_time;
	time(&current_time);

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		for (int i = 0; i < rank_player_list_size; i++)
		{
			if (rank_player_list[i]->rank != -1) players_ranked ++;
		}

		float percent_ranked = 0;

		if (rank_player_list_size != 0 && players_ranked != 0)
		{
			percent_ranked = 100.0 *  ((float) players_ranked / (float) rank_player_list_size);
		}

		if (argc == 1)
		{
			start_index = 0;
			end_index = rank_player_list_size;
		}
		else if (argc == 2)
		{
			start_index = 0;
			end_index = Q_atoi(start_rank);
		}
		else
		{
			start_index = Q_atoi(start_rank) - 1;
			end_index = Q_atoi(end_rank);
			if (start_index < 0) start_index = 0;
			if (end_index > rank_player_list_size) end_index = rank_player_list_size;
		}
	
		OutputToConsole(player.entity, svr_command, "Currently %i Players in rank list (Steam Mode)\n", rank_player_list_size);
		OutputToConsole(player.entity, svr_command, "Out of those players %.2f percent are ranked\n\n", percent_ranked);
		OutputToConsole(player.entity, svr_command, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = start_index; i < end_index; i++)
		{
			OutputToConsole(player.entity, svr_command, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_player_list[i]->name,
						rank_player_list[i]->steam_id,
						rank_player_list[i]->rank,
						rank_player_list[i]->kills,
						rank_player_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_player_list[i]->last_connected)
						);
		}
	}
	else
	{
		for (int i = 0; i < rank_player_name_list_size; i++)
		{
			if (rank_player_name_list[i]->rank != -1) players_ranked ++;
		}

		float percent_ranked = 0;

		if (rank_player_name_list_size != 0 && players_ranked != 0)
		{
			percent_ranked = 100.0 * ((float) players_ranked / (float) rank_player_name_list_size);
		}

		if (argc == 1)
		{
			start_index = 0;
			end_index = rank_player_name_list_size;
		}
		else if (argc == 2)
		{
			start_index = 0;
			end_index = Q_atoi(start_rank);
		}
		else
		{
			start_index = Q_atoi(start_rank) - 1;
			end_index = Q_atoi(end_rank);
			if (start_index < 0) start_index = 0;
			if (end_index > rank_player_name_list_size) end_index = rank_player_name_list_size;
		}

		OutputToConsole(player.entity, svr_command, "Currently %i Players in rank list (Name mode)\n\n", rank_player_name_list_size);
		OutputToConsole(player.entity, svr_command, "Out of those players %.2f percent are ranked\n\n", percent_ranked);
		OutputToConsole(player.entity, svr_command, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = start_index; i < end_index; i++)
		{
			OutputToConsole(player.entity, svr_command, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_player_name_list[i]->name,
						"N/A",
						rank_player_name_list[i]->rank,
						rank_player_name_list[i]->kills,
						rank_player_name_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_player_name_list[i]->last_connected)
						);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ranks command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaPLRanks
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *start_rank,
 char *end_rank
 )
{
	player_t player;
	int	admin_index;
	int start_index;
	int end_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CONFIG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	time_t	current_time;
	time(&current_time);

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		if (argc == 1)
		{
			start_index = 0;
			end_index = rank_list_size;
		}
		else if (argc == 2)
		{
			start_index = 0;
			end_index = Q_atoi(start_rank);
		}
		else
		{
			start_index = Q_atoi(start_rank) - 1;
			end_index = Q_atoi(end_rank);
			if (start_index < 0) start_index = 0;
			if (end_index > rank_list_size) end_index = rank_list_size;
		}

		OutputToConsole(player.entity, svr_command, "Currently %i Ranked Players list (Steam Mode)\n\n", rank_list_size);
		OutputToConsole(player.entity, svr_command, "Name                      Steam ID             Rank   Kills  Deaths  Days\n");

		for (int i = 0; i < end_index; i++)
		{
			OutputToConsole(player.entity, svr_command, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_list[i]->name,
						rank_list[i]->steam_id,
						rank_list[i]->rank,
						rank_list[i]->kills,
						rank_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_list[i]->last_connected)
						);
		}
	}
	else
	{
		if (argc == 1)
		{
			start_index = 0;
			end_index = rank_name_list_size;
		}
		else if (argc == 2)
		{
			start_index = 0;
			end_index = Q_atoi(start_rank);
		}
		else
		{
			start_index = Q_atoi(start_rank) - 1;
			end_index = Q_atoi(end_rank);
			if (start_index < 0) start_index = 0;
			if (end_index > rank_name_list_size) end_index = rank_name_list_size;
		}

		OutputToConsole(player.entity, svr_command, "Currently %i Ranked Players list (Steam Mode)\n\n", rank_name_list_size);
		OutputToConsole(player.entity, svr_command, "Name                      Steam ID             Rank   Kills  Deaths Days\n");

		for (int i = 0; i < end_index; i++)
		{
			OutputToConsole(player.entity, svr_command, "%-25s %-20s %-6i %-6i %-6i  %.2f\n", 
						rank_name_list[i]->name,
						"N/A",
						rank_name_list[i]->rank,
						rank_name_list[i]->kills,
						rank_name_list[i]->deaths,
						(1.0/86400.0) * (float) (current_time - rank_name_list[i]->last_connected)
						);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_resetrank command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaResetPlayerRank
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_steam_id
)
{
	player_t player;
	int	admin_index;
	rank_t	rank_key;
	rank_t	*rank_key_address;
	rank_t	*player_found = NULL;
	player.entity = NULL;
	rank_t	**player_found_address = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CONFIG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player steam id>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <player steam id>", command_string);
			}
		}
		else
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player name>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <player name>", command_string);
			}
		}

		return PLUGIN_STOP;
	}

	rank_key_address = &rank_key;

	if (mani_stats_by_steam_id.GetInt() == 1)
	{
		// Do BSearch for steam ID in ranked player list
		Q_strcpy(rank_key.steam_id, player_steam_id);

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_list, 
							rank_player_list_size, 
							sizeof(rank_t *), 
							sort_by_steam_id
							);
	}
	else
	{
		// Do BSearch for name in ranked player name list
		Q_strcpy(rank_key.name, player_steam_id);

		player_found_address = (rank_t **) bsearch
							(
							&rank_key_address, 
							rank_player_name_list, 
							rank_player_name_list_size, 
							sizeof(rank_t *), 
							sort_by_name
							);
	}

	if (player_found_address == NULL)
	{
		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			// Did not find player
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Did not find steam id [%s]\n", player_steam_id);
			}
			else
			{
				SayToPlayer(&player, "Did not find steam id [%s]", player_steam_id);
			}
		}
		else
		{
			// Did not find player
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Did not find name [%s]\n", player_steam_id);
			}
			else
			{
                SayToPlayer(&player, "Did not find name [%s]", player_steam_id);
			}
		}

		return PLUGIN_STOP;
	}

	// Player in list so reset rank
	player_found = *player_found_address;
	player_found->kills = 0;
	player_found->deaths = 0;
	player_found->suicides = 0;
	player_found->headshots = 0;
	player_found->kd_ratio = 0.0;
	player_found->last_connected = 0;
	player_found->rank = -1;

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Reset rank of player [%s] steam id [%s]\n", player_found->name, player_steam_id);
		LogCommand (NULL, "Reset rank of player [%s] steam id [%s]\n", player_found->name, player_steam_id);
	}
	else
	{
		SayToPlayer(&player, "Reset rank of player [%s] steam id [%s]", player_found->name, player_steam_id);
		LogCommand (player.entity, "Reset rank of player [%s] steam id [%s]\n", player_found->name, player_steam_id);
	}

	return PLUGIN_STOP;
}

static int sort_by_steam_id ( const void *m1,  const void *m2) 
{
	return strcmp((*(rank_t **) m1)->steam_id, (*(rank_t **) m2)->steam_id);
}

static int sort_by_name ( const void *m1,  const void *m2) 
{
	return strcmp((*(rank_t **) m1)->name, (*(rank_t **) m2)->name);
}

static int sort_by_kills ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills > mi2->kills)
	{
		return -1;
	}
	else if (mi1->kills < mi2->kills)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_kd_ratio ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kd_ratio > mi2->kd_ratio)
	{
		return -1;
	}
	else if (mi1->kd_ratio < mi2->kd_ratio)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_kills_deaths ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills - mi1->deaths > mi2->kills - mi2->deaths)
	{
		return -1;
	}
	else if (mi1->kills - mi1->deaths < mi2->kills - mi2->deaths)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_by_name_kills ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills > mi2->kills)
	{
		return -1;
	}
	else if (mi1->kills < mi2->kills)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_name_kd_ratio ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kd_ratio > mi2->kd_ratio)
	{
		return -1;
	}
	else if (mi1->kd_ratio < mi2->kd_ratio)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

static int sort_by_name_kills_deaths ( const void *m1,  const void *m2) 
{
	rank_t *mi1 = *(rank_t **) m1;
	rank_t *mi2 = *(rank_t **) m2;

	if (mi1->kills - mi1->deaths > mi2->kills - mi2->deaths)
	{
		return -1;
	}
	else if (mi1->kills - mi1->deaths < mi2->kills - mi2->deaths)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

CON_COMMAND(ma_ranks, "Prints all players held in ranks list (whether they are ranked or not)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaRanks(0,	true, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2));
	return;
}

CON_COMMAND(ma_plranks, "Prints players who actually have a real rank")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaPLRanks(0,true, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2));
	return;
}

CON_COMMAND(ma_resetrank, "Resets a players rank, ma_resetrank <steam_id>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaResetPlayerRank
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // steam id
					);
	return;
}