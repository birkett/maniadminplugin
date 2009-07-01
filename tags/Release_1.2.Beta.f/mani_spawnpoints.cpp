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
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_skins.h"
#include "mani_maps.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_customeffects.h"
#include "mani_spawnpoints.h"
#include "mani_gametype.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	bool war_mode;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiSpawnPoints::ManiSpawnPoints()
{
	// Init
	for (int i = 0; i < 10; i ++)
	{
		spawn_team[i].spawn_list = NULL;
		spawn_team[i].spawn_list_size = 0;
		spawn_team[i].last_spawn_index = 0;
	}

	gpManiSpawnPoints = this;
}

ManiSpawnPoints::~ManiSpawnPoints()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Free up any lists
//---------------------------------------------------------------------------------
void ManiSpawnPoints::CleanUp(void)
{
	// Cleanup
	for (int i = 0; i < 10; i ++)
	{
		if (spawn_team[i].spawn_list_size != 0)
		{
			free(spawn_team[i].spawn_list);
			spawn_team[i].spawn_list = NULL;
			spawn_team[i].spawn_list_size = 0;
			spawn_team[i].last_spawn_index = 0;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Player has been spawned so relocate them
//---------------------------------------------------------------------------------
void		ManiSpawnPoints::Spawn(player_t *player_ptr)
{
	int	spawn_list_size;

	if (war_mode) return;

	// Not enabled ?
	if (!gpManiGameType->IsTeleportAllowed()) return;
	if (mani_spawnpoints_mode.GetInt() == 0) return;

	// Valid team for spawn points ?
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;

	// No spawn points setup
	spawn_list_size = spawn_team[player_ptr->team].spawn_list_size;
	
	if (spawn_list_size == 0) return;

	// Taken from Valve's team.cpp code and modified
	// Randomize the start spot

	int	m_iLastSpawn = spawn_team[player_ptr->team].last_spawn_index;

	int iSpawn = m_iLastSpawn + randomStr->RandomInt( 1,3 );
	if ( iSpawn >= spawn_list_size )
		iSpawn -= spawn_list_size;

	int iStartingSpawn = iSpawn;

	// Now loop through the spawnpoints and pick one
	int loopCount = 0;
	do 
	{
		if ( iSpawn >= spawn_list_size )
		{
			++loopCount;
			iSpawn = 0;
		}

		// check if pSpot is valid, and that the player is on the right team
		if ( (loopCount > 3) || (!this->IsToClose(player_ptr)))
		{

			CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity(); 

			Vector Pos;
			Vector vVel;
			vVel.x = 0;
			vVel.y = 0;
			vVel.z = 0;

			Pos.x = spawn_team[player_ptr->team].spawn_list[iSpawn].x;
			Pos.y = spawn_team[player_ptr->team].spawn_list[iSpawn].y;
			Pos.z = spawn_team[player_ptr->team].spawn_list[iSpawn].z;

			m_pCBaseEntity->Teleport(&Pos, NULL, &vVel);
			spawn_team[player_ptr->team].last_spawn_index = iSpawn;
			return;
		}

		iSpawn++;
	} while ( iSpawn != iStartingSpawn );

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Server loaded plugin
//---------------------------------------------------------------------------------
void		ManiSpawnPoints::Load(char	*map_name)
{
	this->CleanUp();
	this->LoadData(map_name);
}

//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiSpawnPoints::LevelInit(char	*map_name)
{
	this->CleanUp();
	this->LoadData(map_name);
}

//---------------------------------------------------------------------------------
// Purpose: Check if player is too close to another player
//---------------------------------------------------------------------------------
bool		ManiSpawnPoints::IsToClose(player_t *player_ptr)
{

	for (int i = 1; i <= max_players; i++)
	{
		player_t	target_player;

		if (player_ptr->index == i) continue;

		target_player.index = i;
		if (!FindPlayerByIndex(&target_player))
		{
			continue;
		}

		if (target_player.is_dead) continue;

		Vector	v = player_ptr->player_info->GetAbsOrigin() - target_player.player_info->GetAbsOrigin();
		if (v.Length() < mani_spawnpoints_radius_limit.GetFloat())
		{
			// Found a player too close !!!!
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiSpawnPoints::LoadData(char *map_name)
{
	char	core_filename[256];

	Msg("*********** Loading spawnpoints.txt ************\n");

	KeyValues *kv_ptr = new KeyValues("spawnpoints.txt");

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/spawnpoints.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		Msg("Failed to load spawnpoints.txt\n");
		kv_ptr->deleteThis();
		return;
	}

	KeyValues *base_key_ptr;

	bool found_match;

	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
		Msg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our map entry
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), current_map))
		{
			found_match = true;
			Msg("Found record for %s\n", current_map);
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	// No match found quit
	if (!found_match)
	{
		kv_ptr->deleteThis();
		Msg("Map entry %s not found\n", current_map);
		return;
	}

	KeyValues *kv_map_ptr;
	// Get first decal name
	kv_map_ptr = base_key_ptr->GetFirstTrueSubKey();
	if (!kv_map_ptr)
	{
		Msg("No team number name found\n");
		kv_ptr->deleteThis();
		return;
	}

	for (;;)
	{
		int team_number;


		team_number = Q_atoi(kv_map_ptr->GetName());
		if (team_number == 0 || !gpManiGameType->IsValidActiveTeam(team_number))
		{
			Msg("Team number [%s] is invalid !!\n", kv_map_ptr->GetName());
			// No decal of that name found
			kv_map_ptr = kv_map_ptr->GetNextKey();
			if (!kv_map_ptr)
			{
				break;
			}
		}
		else
		{
			Msg("Team number [%i]\n", team_number);
			GetCoordList(kv_map_ptr, team_number);
		}

		kv_map_ptr = kv_map_ptr->GetNextKey();
		if (!kv_map_ptr)
		{
			break;
		}
	}

	kv_ptr->deleteThis();

	Msg("*********** spawnpoints.txt loaded ************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiSpawnPoints::GetCoordList(KeyValues *kv_ptr, int team_number)
{
	KeyValues *kv_xyz_ptr;
	spawn_vector_t	coord;
	bool	failed;
	bool	first_run = true;

	kv_xyz_ptr = kv_ptr->GetFirstValue();
	if (!kv_xyz_ptr)
	{
		return;
	}

	for (;;)
	{
		failed = false;

		coord.x = kv_xyz_ptr->GetFloat(NULL, 0);
		kv_xyz_ptr = kv_xyz_ptr->GetNextValue();
		if (!kv_xyz_ptr)
		{
			failed = true;
			break;
		}

		coord.y = kv_xyz_ptr->GetFloat(NULL, 0);
		kv_xyz_ptr = kv_xyz_ptr->GetNextValue();
		if (!kv_xyz_ptr)
		{
			failed = true;
			break;
		}

		coord.z = kv_xyz_ptr->GetFloat(NULL, 0);
		kv_xyz_ptr = kv_xyz_ptr->GetNextValue();

		AddToList((void **) &(spawn_team[team_number].spawn_list), sizeof(spawn_vector_t), &(spawn_team[team_number].spawn_list_size));
		spawn_team[team_number].spawn_list[spawn_team[team_number].spawn_list_size - 1] = coord;
		if (!kv_xyz_ptr)
		{
			break;
		}
	}
}

// Command that dumps the current map spawn point positions (does not include custom spawnpoints
CON_COMMAND(ma_dumpspawnpoints, "ma_dumpspawnpoints (Dumps built in default spawn points for current to clipboard.txt file)")
{
	FileHandle_t file_handle;
	char	base_filename[512];
	// Valves FPrintf is broken :/
	char	temp_string[2048];
	int temp_length;

	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	int edict_count = engine->GetEntityCount();
	bool first_time;

	Msg("This command will write the default coordinates for the map to clipboard.txt\n");
	Msg("You can then copy and paste into spawnpoints.txt for the map\n");

	//Write to clipboard.txt
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/clipboard.txt", mani_path.GetString());

	if (filesystem->FileExists( base_filename))
	{
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete clipboard.txt\n");
		}
	}

	file_handle = filesystem->Open (base_filename,"wb", NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open clipboard.txt for writing\n");
		return;
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), 
					"\"spawnpoints.txt\"\n"
					"{\n"
					"\t// Spawn points for map %s\n"
					"\t\"%s\"\n"
					"\t{\n", 
					current_map,
					current_map);

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		Msg ("Failed to write to clipboard.txt\n");
		filesystem->Close(file_handle);
		return;
	}

	for (int j = 0; j < 10; j++)
	{
		// Get classname for spawnpoints per team
		char *classname = gpManiGameType->GetTeamSpawnPointClassName(j);

		if (!classname) continue;

		int count;

		count = 9;
		first_time = true;
		for (int i = 0; i < edict_count; i++)
		{
			edict_t *pEntity = engine->PEntityOfEntIndex(i);
			if(pEntity)
			{
				if (Q_stristr(pEntity->GetClassName(), classname) != NULL)
				{
					if (first_time)
					{
						temp_length = Q_snprintf(temp_string, sizeof(temp_string), 
									"\t\t// Spawn points for team index %i\n"
									"\t\t\"%i\"\n"
									"\t\t{\n", j,j);

						if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
						{
							Msg ("Failed to write to clipboard.txt\n");
							filesystem->Close(file_handle);
							return;
						}

						first_time = false;
					}

					CBaseEntity *pSpawnEnt = pEntity->GetUnknown()->GetBaseEntity();
					const Vector position = pSpawnEnt->m_vecAbsOrigin;

					temp_length = Q_snprintf(temp_string, sizeof(temp_string), 
									"\t\t\t\"x\"\t\"%0.5f\"\n"
									"\t\t\t\"y\"\t\"%0.5f\"\n"
									"\t\t\t\"z\"\t\"%0.5f\"\n\n",
									position.x, position.y, position.z);

					if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
					{
						Msg ("Failed to write to clipboard.txt\n");
						filesystem->Close(file_handle);
						return;
					}

					count ++;
				}
			}
		}

		if (!first_time)
		{
			temp_length = Q_snprintf(temp_string, sizeof(temp_string), 
									"\t\t}\n\n");

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				Msg ("Failed to write to clipboard.txt\n");
				filesystem->Close(file_handle);
				return;
			}
		}

		Msg("%i coordinates for classname %s\n", count, classname);
	}

	temp_length = Q_snprintf(temp_string, sizeof(temp_string), 
									"}\n");

	if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
	{
		Msg ("Failed to write to clipboard.txt\n");
		filesystem->Close(file_handle);
		return;
	}

	filesystem->Close(file_handle);

	Msg("Written to clipboard.txt\n");
}

ManiSpawnPoints	g_ManiSpawnPoints;
ManiSpawnPoints	*gpManiSpawnPoints;
