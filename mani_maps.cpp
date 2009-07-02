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
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_maps.h"

extern	IFileSystem	*filesystem;
extern	IServerPluginHelpers *helpers;
extern	IServerPluginHelpers *helpers;
extern	IVEngineServer *engine;
extern	IServerPluginCallbacks *gpManiClientPlugin;
extern	CGlobalVars *gpGlobals;
extern	int	max_players;
extern	bool war_mode;
extern	char *mani_version;
extern  system_vote_t system_vote;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

last_map_t		last_map_list[MAX_LAST_MAPS];
int				last_map_index = -1;

map_t	*map_list = NULL;
map_t	*votemap_list = NULL;
map_t	*map_in_cycle_list = NULL;
track_map_t	*proper_map_cycle_mode_list = NULL;
map_t	*map_not_in_cycle_list = NULL;

int		map_list_size = 0;
int		votemap_list_size = 0;
int		proper_map_cycle_mode_list_size = 0;
int		map_in_cycle_list_size = 0;
int		map_not_in_cycle_list_size = 0;

char	current_map[128] = "Unknown";
char	next_map[128] = "Unknown";
char	last_map_in_cycle[128] = "Unknown";
char	forced_nextmap[128] = "Unknown";
int		override_changelevel = 0;
bool	override_setnextmap = false;

ConVar	*mapcyclefile = NULL;
ConVar  *host_map = NULL;

static void ManiMapCycleMode ( ConVar *var, char const *pOldString );
static void ManiNextMap ( ConVar *var, char const *pOldString );
ConVar mani_mapcycle_mode ("mani_mapcycle_mode", "0", 0, "0 = standard map cycle is followed, 1 = custom cycle is selected, 2 = random map cycle, 3 = Maps are not skipped after voting", true, 0, true, 3, ManiMapCycleMode); 
ConVar mani_nextmap ("mani_nextmap", "Unknown", FCVAR_REPLICATED | FCVAR_NOTIFY, "Nextmap information");

static void ManiNextMap ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
		{
			mani_nextmap.SetValue("Map decided by vote");
		}
		else
		{
			mani_nextmap.SetValue(next_map);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Initialise some vars
//---------------------------------------------------------------------------------
void	InitMaps(void)
{
	// Setup map history, LevelInit will pick up -1 value
	Q_strcpy(forced_nextmap,"");
	override_changelevel = 0;
	override_setnextmap = false;
	last_map_index = -1;

	for (int i = 0; i < MAX_LAST_MAPS; i ++)
	{
		Q_strcpy(last_map_list[i].map_name,"");
	}
}

//---------------------------------------------------------------------------------
// Purpose: Locate pointers to map cvars
//---------------------------------------------------------------------------------
void	FindMapCVars(void)
{
	mapcyclefile = cvar->FindVar( "mapcyclefile");
	host_map = cvar->FindVar( "host_map");
}

//---------------------------------------------------------------------------------
// Purpose: Free up map data
//---------------------------------------------------------------------------------
void	FreeMaps(void)
{
	FreeList((void **) &map_list, &map_list_size);
	FreeList((void **) &votemap_list, &votemap_list_size);
	FreeList((void **) &map_in_cycle_list, &map_in_cycle_list_size);
	FreeList((void **) &map_not_in_cycle_list, &map_not_in_cycle_list_size);
	FreeList((void **) &proper_map_cycle_mode_list, &proper_map_cycle_mode_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Load maps into memory
//---------------------------------------------------------------------------------
void	LoadMaps(const char *map_being_loaded)
{	
	FileHandle_t file_handle;
	char	base_filename[512];
	char	map_name[128];
	bool	map_is_in_map_cycle;
	bool	found_match;

	// Don't call FreeMaps() !!!!
	FreeList((void **) &map_list, &map_list_size);
	FreeList((void **) &votemap_list, &votemap_list_size);
	FreeList((void **) &map_in_cycle_list, &map_in_cycle_list_size);
	FreeList((void **) &map_not_in_cycle_list, &map_not_in_cycle_list_size);

	FindMapCVars();

//	Msg("************ LOADING MAP LISTS *************\n");
	override_changelevel = 0;

//	Msg("Loading Map [%s]\n", map_being_loaded);
	Q_strcpy(current_map, map_being_loaded);

	// Update last maps list
	last_map_index ++;
	if (last_map_index == MAX_LAST_MAPS)
	{
		last_map_index = 0;
	}

	Q_strcpy(last_map_list[last_map_index].map_name, current_map);

	// Reset force map stuff, mani_map_cycle_mode will set these if necessary
	// when server.cfg is run
	Q_strcpy(forced_nextmap,"");
	override_changelevel = 0;
	override_setnextmap = false;

	// Get nextmap on level change
	file_handle = filesystem->Open (mapcyclefile->GetString(),"rt",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to load %s\n", mapcyclefile->GetString());
		Q_strcpy(next_map, map_being_loaded);
		mani_nextmap.SetValue(next_map);
		AddToList((void **) &map_in_cycle_list, sizeof(map_t), &map_in_cycle_list_size);
		Q_strcpy(map_in_cycle_list[map_in_cycle_list_size - 1].map_name, map_being_loaded);
	}
	else
	{
//		Msg("Mapcycle list [%s]\n", mapcyclefile->GetString());

		while (filesystem->ReadLine (map_name, sizeof(map_name), file_handle) != NULL)
		{
			if (!ParseLine(map_name, true))
			{
				continue;
			}

			if (engine->IsMapValid(map_name) == 0) 
			{
//				Msg("\n*** Map [%s] is not a valid map !!! *****\n");
				continue;
			}

			AddToList((void **) &map_in_cycle_list, sizeof(map_t), &map_in_cycle_list_size);
			Q_strcpy(map_in_cycle_list[map_in_cycle_list_size - 1].map_name, map_name);
			map_in_cycle_list[map_in_cycle_list_size - 1].selected_for_vote = false;

//			Msg("[%s] ", map_name);
		}

//		Msg("\n");
		filesystem->Close(file_handle);
	}
	
	// Check if this map is in the map cycle
	map_is_in_map_cycle = false;
	for (int i = 0; i < map_in_cycle_list_size; i ++)
	{
		if (FStrEq(map_in_cycle_list[i].map_name, current_map))
		{
			map_is_in_map_cycle = true;
			break;
		}
	}

	if (!map_is_in_map_cycle)
	{
		// Map loaded is not in the map cycle list
		// so hl2 will default the next map to 
		// be the first on the map cycle list
		if (map_in_cycle_list_size != 0)
		{
			Q_strcpy(next_map, map_in_cycle_list[0].map_name);
			mani_nextmap.SetValue(next_map);
		}
	}
	else
	{
		// Search map cycle list for nextmap
		for (i = 0; i < map_in_cycle_list_size; i ++)
		{
			if (FStrEq( map_in_cycle_list[i].map_name, current_map))
			{
				if (i == (map_in_cycle_list_size - 1))
				{
					// End of map list so we must use the first
					// in the list
					Q_strcpy(next_map, map_in_cycle_list[0].map_name);
					mani_nextmap.SetValue(next_map);
				}
				else
				{
					// Set next map
					Q_strcpy(next_map, map_in_cycle_list[i+1].map_name);
					mani_nextmap.SetValue(next_map);
				}

				Q_strcpy(last_map_in_cycle, current_map);

				break;
			}
		}
	}

	//Get Map list
	file_handle = filesystem->Open ("maplist.txt","rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load maplist.txt YOU MUST HAVE A MAPLIST.TXT FILE!\n");
	}
	else
	{
//		Msg("Map list\n");

		while (filesystem->ReadLine (map_name, 128, file_handle) != NULL)
		{
			if (!ParseLine(map_name, true))
			{
				// String is empty after parsing
				continue;
			}


			if ((!FStrEq( map_name, "test_speakers"))
				&& (!FStrEq( map_name, "test_hardware")))
			{
				if (engine->IsMapValid(map_name) == 0) 
				{
					Msg("\n*** Map [%s] is not a valid map !!! *****\n");
					continue;
				}

				AddToList((void **) &map_list, sizeof(map_t), &map_list_size);
				Q_strcpy(map_list[map_list_size-1].map_name, map_name);
				map_list[map_list_size - 1].selected_for_vote = false;
//				Msg("[%s] ", map_name);
			}
		}

//		Msg("\n");
		filesystem->Close(file_handle);
	}

//	Msg("Maps not in [%s]\n", mapcyclefile->GetString());
	// Calculate maps not in mapcycle

	for (int i = 0; i < map_list_size; i ++)
	{
		found_match = false;
		for (int j = 0; j < map_in_cycle_list_size; j++)
		{
			if (FStrEq(map_list[i].map_name, map_in_cycle_list[j].map_name))
			{
				found_match = true;
				break;
			}
		}

		if (!found_match)
		{
			AddToList((void **) &map_not_in_cycle_list, sizeof(map_t), &map_not_in_cycle_list_size);
			Q_strcpy(map_not_in_cycle_list[map_not_in_cycle_list_size - 1].map_name, map_list[i].map_name);
//			Msg("[%s] ", map_not_in_cycle_list[map_not_in_cycle_list_size - 1].map_name);
		}
	}			

//	Msg("\n");

	// Check if votemaplist.txt exists, create a new one if it doesn't
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/votemaplist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load votemaplist.txt\n");
		Msg ("Attempting to write a new votemaplist.txt file based on maplist.txt\n");

		file_handle = filesystem->Open(base_filename,"wt",NULL);
		if (file_handle == NULL)
		{
//			Msg ("Failed to open votemaplist.txt for writing\n");
		}
		else
		{
			// Write votemaplist.txt in human readable text format
			for (int i = 0; i < map_list_size; i ++)
			{
				char	temp_string[512];
				int		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "%s\n", map_list[i].map_name);

				if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)											
				{
					Msg("Failed to write map [%s] to votemaplist.txt!!\n", map_list[i].map_name);
					filesystem->Close(file_handle);
					break;
				}
			}

			Msg("Wrote %i maps to votemaplist.txt\n", map_list_size);
			filesystem->Close(file_handle);
		}
	}
	else
	{
		filesystem->Close(file_handle);
	}

	// Read in votemaplist.txt
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to load votemaplist.txt\n");
	}
	else
	{
//		Msg("Votemap list\n");
		while (filesystem->ReadLine (map_name, sizeof(map_name), file_handle) != NULL)
		{
			if (!ParseLine(map_name, true))
			{
				// String is empty after parsing
				continue;
			}

			if (engine->IsMapValid(map_name) == 0) 
			{
				Msg("\n*** Map [%s] is not a valid map !!! *****\n");
				continue;
			}

			AddToList((void **) &votemap_list, sizeof(map_t), &votemap_list_size);
			Q_strcpy(votemap_list[votemap_list_size - 1].map_name, map_name);
			votemap_list[votemap_list_size - 1].selected_for_vote = false;
//			Msg("[%s] ", map_name);
		}

//		Msg("\n");
		filesystem->Close(file_handle);
	}

	// Check if loaded map cycle file is different than
	// the persistent one
	bool	rebuild_proper_map_cycle = false;

	if (map_in_cycle_list_size != proper_map_cycle_mode_list_size)
	{
		rebuild_proper_map_cycle = true;
	}
	else
	{
		// Both cycles the same size so check maps are in same
		// order
		for (int i = 0; i < map_in_cycle_list_size; i ++)
		{
			if (!FStrEq(map_in_cycle_list[i].map_name, 
						proper_map_cycle_mode_list[i].map_name))
			{
				rebuild_proper_map_cycle = true;
				break;
			}
		}
	}

	if (rebuild_proper_map_cycle)
	{
		// Free persistance map cycle and rebuild
		FreeList((void **) &proper_map_cycle_mode_list, &proper_map_cycle_mode_list_size);
		for (int i = 0; i < map_in_cycle_list_size; i++)
		{
			AddToList((void **) &proper_map_cycle_mode_list, sizeof(track_map_t), &proper_map_cycle_mode_list_size);
			Q_strcpy(proper_map_cycle_mode_list[i].map_name, map_in_cycle_list[i].map_name);
			proper_map_cycle_mode_list[i].played = false;
			if (FStrEq(proper_map_cycle_mode_list[i].map_name, current_map))
			{
				proper_map_cycle_mode_list[i].played = true;
			}
		}
	}

//	Msg("Persistant Map Cycle\n");
	for (int i = 0; i < proper_map_cycle_mode_list_size; i++)
	{
		if (FStrEq(proper_map_cycle_mode_list[i].map_name, current_map))
		{
			proper_map_cycle_mode_list[i].played = true;
		}

		if (proper_map_cycle_mode_list[i].played)
		{
//			Msg("*[%s] ", proper_map_cycle_mode_list[i].map_name);
		}
		else
		{
//			Msg("[%s] ", proper_map_cycle_mode_list[i].map_name);
		}
	}

//	Msg("\n");
//	Msg("************ MAP LISTS LOADED *************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Process the listmaps command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaListMaps
(
 int index, 
 bool svr_command
)
{
	player_t player;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	}

	int	count;
	char map_text[128];

	OutputToConsole(player.entity, svr_command,  "\n");
	OutputToConsole(player.entity, svr_command, "%s\n", mani_version);
	OutputToConsole(player.entity, svr_command, "\nMaps in cycle:-\n--------------\n");

	for (count = 0; count < map_in_cycle_list_size; count ++)
	{
		if (FStrEq(map_in_cycle_list[count].map_name, current_map))
		{
			Q_snprintf( map_text, sizeof(map_text), "%s -> CURRENT MAP BEING PLAYED\n", map_in_cycle_list[count].map_name);
		}
		else if (FStrEq(map_in_cycle_list[count].map_name, next_map) && 
			!(mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false))
		{
			Q_snprintf( map_text, sizeof(map_text), "%s -> NEXT MAP\n", map_in_cycle_list[count].map_name);
		}
		else
		{
			Q_snprintf( map_text, sizeof(map_text), "%s\n", map_in_cycle_list[count].map_name);
		}
		OutputToConsole(player.entity, svr_command, "%s", map_text);
	}

	if (map_not_in_cycle_list_size > 0)
	{
		OutputToConsole(player.entity, svr_command, "\nMaps not in cycle but are on server:-\n------------------------------------\n");
		for (count = 0; count < map_not_in_cycle_list_size; count ++)
		{
			if (FStrEq(map_not_in_cycle_list[count].map_name, current_map))
			{
				Q_snprintf( map_text, sizeof(map_text), "%s -> CURRENT MAP BEING PLAYED\n", map_not_in_cycle_list[count].map_name);
			}
			else
			{
				Q_snprintf( map_text, sizeof(map_text), "%s\n", map_not_in_cycle_list[count].map_name);
			}

			OutputToConsole(player.entity, svr_command, "%s", map_text);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the nextmap command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaNextMap
(
 int index, 
 bool svr_command
)
{
	player_t player;
	player.entity = NULL;
	char	map_text[128];

	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	}

	if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
	{
		Q_strcpy(map_text, "Map decided by vote");
	}
	else
	{
		Q_snprintf(map_text, sizeof (map_text), "Nextmap: %s", next_map);
	}

	OutputToConsole(player.entity, svr_command, "%s\n", map_text);

	if (!svr_command)
	{
		if (mani_nextmap_player_only.GetInt() == 1)
		{
			ClientMsgSinglePlayer(player.entity, 10, 4, "%s", map_text);
		}
		else
		{
			Color	white(255,255,255,255);

			ClientMsg(&white, 10, false, 4, "%s", map_text);
		}
	}
	else
	{
		Color	white(255,255,255,255);

		ClientMsg(&white, 10, false, 1, "%s", map_text);
	}


	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_map command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSetNextMap
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *map_name
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

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
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CHANGEMAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <map name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <map name>", command_string);
		}

		return PLUGIN_STOP;
	}


	for( int i = 0; i < map_list_size; i++ )
	{
		// String must be valid map !!
		if (FStrEq(map_list[i].map_name, map_name))
		{
			Q_strcpy(forced_nextmap,map_name);
			Q_strcpy(next_map, map_name);
			mani_nextmap.SetValue(next_map);
			LogCommand (player.entity, "%s %s\n", command_string, map_name);
			override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
			override_setnextmap = true;

			// Make sure end of map vote doesn't try and override it
			system_vote.map_decided = true;
			AdminSayToAll(&player, mani_adminmap_anonymous.GetInt(), "set nextmap to %s", map_name); 
			return PLUGIN_STOP;
		}
	}

	LogCommand(player.entity, "User attempted to set mapname [%s] as the nextmap\n", map_name);
	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Map [%s] is not in maplist.txt file\n", map_name);
	}
	else
	{
		SayToPlayer(&player, "Map [%s] is not in maplist.txt file", map_name);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_maplist command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaMapList
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, "ma_maplist", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current maps in the maplist.txt file\n\n");

	for (int i = 0; i < map_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "%s\n", map_list[i].map_name);
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_maplist command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaMapCycle
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, "ma_mapcycle", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current maps in the mapcycle.txt file\n\n");

	for (int i = 0; i < map_in_cycle_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "%s\n", map_in_cycle_list[i].map_name);
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_votelist command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaVoteMapList
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, "ma_votemaplist", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current maps in the votemaplist.txt file\n\n");

	for (int i = 0; i < votemap_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "%s\n", votemap_list[i].map_name);
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_map command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaMap
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *map_name
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CHANGEMAP, false, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <map name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <map name>", command_string);
		}

		return PLUGIN_STOP;
	}

	
	// Change the map
	char		changemap_cmd[128];

	int	valid_map = engine->IsMapValid(map_name);
	if (valid_map != 0)
	{
		Q_snprintf( changemap_cmd, sizeof(changemap_cmd), "changelevel %s\n", map_name);
		LogCommand (player.entity, "%s", changemap_cmd);
		override_changelevel = 0;
		override_setnextmap = false;

		engine->ServerCommand(changemap_cmd);
//		engine->ChangeLevel(map_name, NULL);
		return PLUGIN_STOP;
	}

	LogCommand(player.entity, "User attempted to change to mapname [%s]\n", map_name);

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Map [%s] is not a valid .bsp map file\n", map_name);
	}
	else
	{
		SayToPlayer(&player, "Map [%s] is not a valid .bsp map file", map_name);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_skipmap command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSkipMap
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_CHANGEMAP, false, &admin_index)) return PLUGIN_STOP;
	}

	// Change the map
	char		changemap_cmd[128];

	int	valid_map = engine->IsMapValid(next_map);
	if (valid_map != 0)
	{
		Q_snprintf( changemap_cmd, sizeof(changemap_cmd), "changelevel %s\n", next_map);
		LogCommand (player.entity, "%s", changemap_cmd);
		override_changelevel = 0;
		override_setnextmap = false;

		engine->ServerCommand(changemap_cmd);
//		engine->ChangeLevel(map_name, NULL);
		return PLUGIN_STOP;
	}

	LogCommand(player.entity, "User attempted to change to mapname  [%s] using ma_skipmap\n", next_map);

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Map [%s] is not a valid .bsp map file\n", next_map);
	}
	else
	{
		SayToPlayer(&player, "Map [%s] is not a valid .bsp map file", next_map);
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the mani_map_cycle_mode cvar change
//---------------------------------------------------------------------------------
static 
void ManiMapCycleMode ( ConVar *var, char const *pOldString )
{
	int	map_cycle_mode;
	map_t	*select_list;
	map_t	select_map;
	int	select_list_size;
	last_map_t *last_maps;
	int	maps_to_skip;

	map_cycle_mode = var->GetInt();
	select_list = NULL;
	select_list_size = 0;

	if (map_cycle_mode == 0)
	{
		if (override_setnextmap != true)
		{
			// Only reset changelevel override if setnextmap isn't in use
			override_changelevel = 0;
		}
	}
	else if (map_cycle_mode == 1)
	{
		// Check if already overriden by admin
		if (override_setnextmap) return;
		if (!last_map_in_cycle) return;

		// Search map cycle list for nextmap
		for (int i = 0; i < map_in_cycle_list_size; i ++)
		{
			if (FStrEq(map_in_cycle_list[i].map_name, current_map))
			{
				// Already on current map cycle, no need to change it
				return;
			}

			if (FStrEq( map_in_cycle_list[i].map_name, last_map_in_cycle))
			{
				if (i == (map_in_cycle_list_size - 1))
				{
					// End of map list so we must use the first
					// in the list
					Q_strcpy(next_map, map_in_cycle_list[0].map_name);
					if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
					{
						mani_nextmap.SetValue("Map decided by vote");
					}
					else
					{
						mani_nextmap.SetValue(next_map);
					}
				}
				else
				{
					// Set next map
					Q_strcpy(next_map, map_in_cycle_list[i+1].map_name);
					if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
					{
						mani_nextmap.SetValue("Map decided by vote");
					}
					else
					{
						mani_nextmap.SetValue(next_map);
					}
				}

				break;
			}
		}

//		Msg("Setting nextmap to [%s]\n", next_map); 
		Q_strcpy(forced_nextmap, next_map);
		override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
	}
	else if (map_cycle_mode == 2)
	{
		// Check if already overriden by admin
		if (override_setnextmap) return;

		// Get list of maps already played and exclude them from random selection
		last_maps = GetLastMapsPlayed(&maps_to_skip, mani_vote_dont_show_last_maps.GetInt());

		// Exclude maps already played by pretending they are already selected.
		int exclude = 0;
		for (int i = 0; i < map_in_cycle_list_size; i++)
		{
			bool exclude_map = false;

			for (int j = 0; j < maps_to_skip; j ++)
			{
				if (FStrEq(last_maps[j].map_name, map_in_cycle_list[i].map_name))
				{
					exclude_map = true;
					break;
				}
			}

			if (!exclude_map)
			{
				// Add map to selection list
				Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", map_in_cycle_list[i].map_name);
				AddToList((void **) &select_list, sizeof(map_t), &select_list_size);
				select_list[select_list_size - 1] = select_map;
			}
		}

		if (select_list_size != 0)
		{	
			srand( (unsigned)time(NULL));
			int map_index = rand() % select_list_size;
			Q_strcpy(next_map, select_list[map_index].map_name);
			if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
			{
				mani_nextmap.SetValue("Map decided by vote");
			}
			else
			{
				mani_nextmap.SetValue(next_map);
			}
			Q_strcpy(forced_nextmap, next_map);
			override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
//			Msg("Setting nextmap to [%s]\n", select_list[map_index].map_name); 
			FreeList((void **) &select_list, &select_list_size);
		}
	}
	else if (map_cycle_mode == 3)
	{
		for (int i = 0; i < proper_map_cycle_mode_list_size; i++)
		{
			// Check if map has been played
			if (!proper_map_cycle_mode_list[i].played)
			{
				// Nope, so set this as the next map
				Q_strcpy(next_map, proper_map_cycle_mode_list[i].map_name);
				if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
				{
					mani_nextmap.SetValue("Map decided by vote");
				}
				else
				{
					mani_nextmap.SetValue(next_map);
				}
				Q_strcpy(forced_nextmap, next_map);
				override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
//				Msg("Setting nextmap to [%s]\n", proper_map_cycle_mode_list[i].map_name); 
				return;
			}
		}

		// Nothing found so reset played count
		for (int i = 0; i < proper_map_cycle_mode_list_size; i++)
		{
			proper_map_cycle_mode_list[i].played = false;
		}

		if (proper_map_cycle_mode_list_size != 0)
		{
			// Choose first map
			Q_strcpy(next_map, proper_map_cycle_mode_list[0].map_name);
			if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && system_vote.map_decided == false)
			{
				mani_nextmap.SetValue("Map decided by vote");
			}
			else
			{
				mani_nextmap.SetValue(next_map);
			}
			Q_strcpy(forced_nextmap, next_map);
			override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
//			Msg("Setting nextmap to [%s]\n", proper_map_cycle_mode_list[0].map_name); 
		}
		else
		{
			Msg("Failed to set map cycle\n");
		}

		return;
	}
}

//*******************************************************************************
// Get the last x maps played
//*******************************************************************************
last_map_t	*GetLastMapsPlayed (int *number_of_maps_found, int max_number_of_maps)
{
	int	i,j;
	static last_map_t last_maps_played[MAX_LAST_MAPS];
	
	for (i = 0; i < MAX_LAST_MAPS; i++)
	{
		Q_strcpy(last_maps_played[i].map_name, "");
	}

	i = last_map_index;
	j = 0;
	while (j != max_number_of_maps)
	{
		if (!last_map_list[i].map_name) break;

		Q_strcpy(last_maps_played[j].map_name, last_map_list[i].map_name);
		j++;
		i --;
		if (i < 0) i = MAX_LAST_MAPS - 1;
	}

	*number_of_maps_found = j;
	return &(last_maps_played[0]);
}

//*******************************************************************************
// CON COMMANDS
//*******************************************************************************

CON_COMMAND(ma_map, "Changes the server to a new map, ma_map <map name>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaMap
			(
			0, // Client index
			true,  // Sever console command type
			engine->Cmd_Argc(), // Number of arguments
			engine->Cmd_Argv(0), // Command
			engine->Cmd_Argv(1) // map name
			);

	return;
}

CON_COMMAND(ma_skipmap, "Changes the server to the next map in the cycle, ma_map")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaSkipMap
			(
			0, // Client index
			true,  // Sever console command type
			engine->Cmd_Argc(), // Number of arguments
			engine->Cmd_Argv(0) // Command
			);

	return;
}

CON_COMMAND(nextmap, "Prints the nextmap")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaNextMap(0, true);
	return;
}

CON_COMMAND(listmaps, "Prints current map cycle information")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaListMaps(0, true);
	return;
}

CON_COMMAND(ma_maplist, "Prints current maplist.txt")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaMapList(0, true);
	return;
}

CON_COMMAND(ma_mapcycle, "Prints current mapcycle.txt")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaMapCycle(0, true);
	return;
}

CON_COMMAND(ma_votemaplist, "Prints current votemaplist.txt")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaVoteMapList(0, true);
	return;
}

CON_COMMAND(ma_setnextmap, "Sets the next map to be played, ma_setnextmap <map name>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaSetNextMap
				(
				0, // Client index
				true,  // Sever console command type
				engine->Cmd_Argc(), // Number of arguments
				engine->Cmd_Argv(0), // Command
				engine->Cmd_Argv(1) // map name
				);
	return;
}
