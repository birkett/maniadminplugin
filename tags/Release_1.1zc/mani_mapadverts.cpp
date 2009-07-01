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
#include "mani_admin.h"
#include "mani_output.h"
#include "mani_customeffects.h"
#include "mani_mapadverts.h"
#include "mani_gametype.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	ITempEntsSystem *temp_ents;
extern	bool war_mode;

extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiMapAdverts::ManiMapAdverts()
{
	// Init
	decal_list = NULL;
	decal_list_size = 0;
	gpManiMapAdverts = this;
}

ManiMapAdverts::~ManiMapAdverts()
{
	// Cleanup
	for (int i = 0; i < decal_list_size; i ++)
	{
		if (decal_list[i].decal_coord_list_size != 0)
		{
			free(decal_list[i].decal_coord_list);
		}
	}

	free (decal_list);
}

//---------------------------------------------------------------------------------
// Purpose: Free up any lists
//---------------------------------------------------------------------------------
void ManiMapAdverts::FreeMapAdverts(void)
{
	// Cleanup
	for (int i = 0; i < decal_list_size; i ++)
	{
		if (decal_list[i].decal_coord_list_size != 0)
		{
			free(decal_list[i].decal_coord_list);
		}
	}

	FreeList((void **) &decal_list, &decal_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiMapAdverts::Init(void)
{
	char	core_filename[256];

	if (!gpManiGameType->IsAdvertDecalAllowed()) return;

	Msg("*********** Loading mapadverts.txt ************\n");

	FreeMapAdverts();
	KeyValues *kv_ptr = new KeyValues("mapadverts.txt");

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/mapadverts.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		Msg("Failed to load mapadverts.txt\n");
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

	KeyValues *kv_decal_ptr;
	// Get first decal name
	kv_decal_ptr = base_key_ptr->GetFirstTrueSubKey();
	if (!kv_decal_ptr)
	{
		Msg("No decal key name found\n");
		kv_ptr->deleteThis();
		return;
	}

	for (;;)
	{
		char decal_name[64];

		Q_strcpy(decal_name, kv_decal_ptr->GetName());
		Msg("Decal Name [%s]\n", decal_name);		
		if (gpManiCustomEffects->GetDecal(decal_name) == -1)
		{
			Msg("Decal name [%s] is invalid !!\n", decal_name);
			// No decal of that name found
			kv_decal_ptr = kv_decal_ptr->GetNextKey();
			if (!kv_decal_ptr)
			{
				break;
			}
		}
		else
		{
			GetCoordList(kv_decal_ptr, decal_name);
		}

		kv_decal_ptr = kv_decal_ptr->GetNextKey();
		if (!kv_decal_ptr)
		{
			break;
		}
	}

	kv_ptr->deleteThis();

	Msg("*********** mapadverts.txt loaded ************\n");

/*	for (int i = 0; i < decal_list_size; i ++)
	{
		Msg("Decal Name [%s]\n" , decal_list[i].name);
		for (int j = 0; j < decal_list[i].decal_coord_list_size; j ++)
		{
			Msg ("%f %f %f\n", 
					decal_list[i].decal_coord_list[j].x,
					decal_list[i].decal_coord_list[j].y,
					decal_list[i].decal_coord_list[j].z);
		}
	}
*/
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiMapAdverts::GetCoordList(KeyValues *kv_ptr, char *decal_name)
{
	KeyValues *kv_xyz_ptr;
	decal_coord_t	coord;
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

		if (first_run)
		{
			AddToList((void **) &decal_list, sizeof(decal_name_t), &decal_list_size);
			Q_strcpy(decal_list[decal_list_size - 1].name, decal_name);
			decal_list[decal_list_size - 1].index = gpManiCustomEffects->GetDecal(decal_name);
			decal_list[decal_list_size - 1].decal_coord_list = NULL;
			decal_list[decal_list_size - 1].decal_coord_list_size = 0;
			first_run = false;
		}
		
		decal_name_t *decal_ptr = &(decal_list[decal_list_size - 1]);

		AddToList((void **) &decal_ptr->decal_coord_list, sizeof(decal_coord_t), &decal_ptr->decal_coord_list_size);
		decal_ptr->decal_coord_list[decal_ptr->decal_coord_list_size - 1] = coord;

		if (!kv_xyz_ptr)
		{
			break;
		}
	}

}

//---------------------------------------------------------------------------------
// Purpose: Show decal to player
//---------------------------------------------------------------------------------
void ManiMapAdverts::PlayerConnect(player_t *player_ptr)
{
	if (!gpManiGameType->IsAdvertDecalAllowed()) return;
	if (mani_map_adverts.GetInt() == 0) return;
	if (war_mode && mani_map_adverts_in_war.GetInt() == 0) return;

	if (!gpManiGameType->GetAdvancedEffectsAllowed()) return;

	for (int i = 0; i < decal_list_size; i ++)
	{
		for (int j = 0; j < decal_list[i].decal_coord_list_size; j ++)
		{
			MRecipientFilter filter;
			Vector position;
			int	   index = decal_list[i].index;

			position.x = decal_list[i].decal_coord_list[j].x;
			position.y = decal_list[i].decal_coord_list[j].y;
			position.z = decal_list[i].decal_coord_list[j].z;

			filter.AddPlayer(player_ptr->index);
			temp_ents->BSPDecal((IRecipientFilter &) filter, 0, &position, 0, index);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Show decal to player
//---------------------------------------------------------------------------------
void ManiMapAdverts::DumpCoords(player_t *player_ptr)
{
	if (mani_map_adverts.GetInt() == 0) return;
	if (war_mode && mani_map_adverts_in_war.GetInt() == 0) return;

	if (!gpManiGameType->GetAdvancedEffectsAllowed()) return;

	for (int i = 0; i < decal_list_size; i ++)
	{
		OutputToConsole(player_ptr->entity, false, "Decal Name [%s] Index [%i]\n", decal_list[i].name, decal_list[i].index);

		for (int j = 0; j < decal_list[i].decal_coord_list_size; j ++)
		{
			OutputToConsole(player_ptr->entity, false, "X %.4f  Y %.4f  Z %.4f\n", 
							decal_list[i].decal_coord_list[j].x,
							decal_list[i].decal_coord_list[j].y,
							decal_list[i].decal_coord_list[j].z);
		}
	}
}

ManiMapAdverts	g_ManiMapAdverts;
ManiMapAdverts	*gpManiMapAdverts;
