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
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_weapon.h"
#include "mani_gametype.h"
#include "mani_maps.h"
#include "mani_skins.h"
#include "KeyValues.h"
#include "mani_vfuncs.h"
#include "cbaseentity.h"


extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IEngineSound *esounds; // sound
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

current_skin_t	current_skin_list[MANI_MAX_PLAYERS];

skin_t			*skin_list = NULL;
int				skin_list_size = 0;

action_model_t	*action_model_list = NULL;
int				action_model_list_size = 0;

static	bool LoadSkinType( const char *skin_directory,  const char *filename, const char *core_filename, const int	skin_type );
static	void ManiSkinsAutoDownload ( ConVar *var, char const *pOldString );
static	void	SetupSkinsAutoDownloads(void);
static  bool	IsSkinValidForPlayer( int *skin_index_ptr, player_t *player_ptr );
static	int IsValidSkinName(const char *skin_name);
static	void AddModelActions(KeyValues *kv_ptr);
static	void	SetupActionModelsAutoDownloads(void);

ConVar mani_skins_auto_download ("mani_skins_auto_download", "0", 0, "0 = Don't auto download files to client, 1 = automatically download files to client", true, 0, true, 1, ManiSkinsAutoDownload); 

//---------------------------------------------------------------------------------
// Purpose: Free the skins used
//---------------------------------------------------------------------------------
void	FreeSkins(void)
{
	for (int i = 0; i < skin_list_size; i ++)
	{
		if (skin_list[i].resource_list_size != 0)
		{
			free(skin_list[i].resource_list);
		}
	}

	FreeList((void **) &skin_list, &skin_list_size);

}

//---------------------------------------------------------------------------------
// Purpose: Free the skins used
//---------------------------------------------------------------------------------
void	FreeActionModels(void)
{
	for (int i = 0; i < action_model_list_size; i ++)
	{
		if (action_model_list[i].sound_list_size != 0)
		{
			free(action_model_list[i].sound_list);
		}

		if (action_model_list[i].weapon_list_size != 0)
		{
			free(action_model_list[i].weapon_list);
		}
	}

	FreeList((void **) &skin_list, &skin_list_size);

}
//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the skins used
//---------------------------------------------------------------------------------
void	LoadSkins(void)
{
	char	skin_directory[256];
	char	core_filename[256];
	char	skin_short_dir[256];
	char	skin_config_file[256];

	FreeSkins();
	FreeActionModels();

//	Msg("************ LOADING SKINS *************\n");

	// Do Admin T load first
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(0));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(0));
	}
	else
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(TEAM_A));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(TEAM_A));
	}

    Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if(!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_T_SKIN))
	{
//		Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Admin T load first
		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir );
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_T_SKIN);
	}

	// Do Admin CT load
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(TEAM_B));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(TEAM_B));

		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_CT_SKIN))
		{
//			Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Admin CT load
			Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_CT_SKIN);
		}
	}

	// Do Reserved T load first
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(0));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(0));
	}
	else
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(TEAM_A));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(TEAM_A));
	}

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_T_SKIN))
	{
//		Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Reserved T load first
		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_T_SKIN);
	}

	// Do Reserved CT load
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(TEAM_B));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(TEAM_B));

		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_CT_SKIN))
		{
//			Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Reserved CT load
			Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_CT_SKIN);
		}
	}

	// Do Public T load 
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(0));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(0));
	}
	else
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetPublicSkinDir(TEAM_A));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(TEAM_A));
	}

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_T_SKIN))
	{
//		Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Public T load 
		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_T_SKIN);
	}

	// Do Public CT load 
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		Q_snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetPublicSkinDir(TEAM_B));
		Q_snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(TEAM_B));

		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_CT_SKIN))
		{
//			Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Public CT load 
			Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_CT_SKIN);
		}
	}

	// Do Misc load 
	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/misc.txt", mani_path.GetString(), current_map);
	Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/misc/", mani_path.GetString(), current_map);
	if (!LoadSkinType (skin_directory, "misc.txt", core_filename, MANI_MISC_SKIN))
	{
//		Msg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/misc.txt\n", mani_path.GetString(), current_map);
		// Do Misc load 
		Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/misc.txt", mani_path.GetString());
		Q_snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/misc/", mani_path.GetString());
		LoadSkinType (skin_directory, "misc.txt", core_filename, MANI_MISC_SKIN);
	}

	SetupSkinsAutoDownloads();

//	Msg("************ SKINS LOADED *************\n");

//	Msg("**** LOADING CUSTOM MODEL ACTIONS *****\n");

	KeyValues * kv = new KeyValues("modelconfig.txt");
	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/modelconfig.txt", mani_path.GetString());
	if (!kv->LoadFromFile( filesystem, core_filename, NULL))
	{
//		Msg("Failed to load modelconfig.txt\n");
	}

	KeyValues *temp_kv;

	temp_kv = kv->GetFirstTrueSubKey();
	if (!temp_kv)
	{
		kv->deleteThis();
		return;
	}

	// Load data into memory
	for (;;)
	{
		if (IsValidSkinName(temp_kv->GetName()) != -1)
		{
//			Msg("Adding action model %s\n", temp_kv->GetName());
			AddModelActions(temp_kv);
		}

		temp_kv = temp_kv->GetNextKey();
		if (!temp_kv)
		{
			break;
		}
	}

	kv->deleteThis();

	SetupActionModelsAutoDownloads();

//	Msg("**** CUSTOM MODEL ACTIONS LOADED *****\n");

}

//---------------------------------------------------------------------------------
// Purpose: Find index of skin_list for specified model name
//---------------------------------------------------------------------------------
static
void AddModelActions(KeyValues *kv_ptr)
{
	
	action_model_t	action_model;
	KeyValues	*temp_key_ptr;

	Q_strcpy(action_model.skin_name, kv_ptr->GetName());

	action_model.gravity = kv_ptr->GetInt("gravity", 100);
	action_model.random_sound = kv_ptr->GetInt("random_sound", 1);
	action_model.remove_all_weapons = kv_ptr->GetInt("remove_all_weapons", 0);
	action_model.allow_weapon_pickup = kv_ptr->GetInt("allow_weapon_pickup", 0);

	action_model.red = 255;
	action_model.green = 255;
	action_model.blue = 255;
	action_model.alpha = 255;

	temp_key_ptr = kv_ptr->FindKey("colour", false);
	if (temp_key_ptr)
	{
//		Msg("Found colour\n");
		action_model.red = temp_key_ptr->GetInt("red", 255);
		action_model.green = temp_key_ptr->GetInt("green", 255);
		action_model.blue = temp_key_ptr->GetInt("blue", 255);
		action_model.alpha = temp_key_ptr->GetInt("alpha", 255);
	}

	action_model.sound_list_size = 0;
	action_model.sound_list = NULL;
	action_model.intermission_min = 10;
	action_model.intermission_max = 20;

	temp_key_ptr = kv_ptr->FindKey("sound", false);
	if (temp_key_ptr)
	{
//		Msg("Found sound structure \n");
		action_model.intermission_max = temp_key_ptr->GetFloat("intermission_max", 20);
		action_model.intermission_min = temp_key_ptr->GetFloat("intermission_min", 10);

		action_model_sound_t model_sound;

		temp_key_ptr = temp_key_ptr->GetFirstTrueSubKey();
		if (temp_key_ptr)
		{
			int i = 1;

			for (;;)
			{
				// Read sound list
				model_sound.play_once = temp_key_ptr->GetInt("play_once", 0);
				model_sound.primary_sound = temp_key_ptr->GetInt("primary_sound", 0);
				model_sound.sequence = temp_key_ptr->GetInt("sequence", i);
				model_sound.sound_length = temp_key_ptr->GetFloat("sound_length", 0.0);
				model_sound.auto_download = temp_key_ptr->GetInt("auto_download", 0);
				Q_strcpy(model_sound.sound_file, temp_key_ptr->GetString("sound_file", "NONE"));
				if (!FStrEq("NONE", model_sound.sound_file))
				{
					esounds->PrecacheSound(model_sound.sound_file, true);
					AddToList((void **) &action_model.sound_list, sizeof(action_model_sound_t), &action_model.sound_list_size);
					action_model.sound_list[action_model.sound_list_size - 1] = model_sound;
//					Msg("Adding sound %s\n", temp_key_ptr->GetName());
				}
				temp_key_ptr = temp_key_ptr->GetNextKey();
				i++;

				if (!temp_key_ptr)
				{
					break;
				}
			}
		}
	}

	action_model.weapon_list = NULL;
	action_model.weapon_list_size = 0;

	temp_key_ptr = kv_ptr->FindKey("weapon_disable", false);
	if (temp_key_ptr)
	{
		for (int i = 0; i < weapon_list_size; i ++)
		{
			if (temp_key_ptr->FindKey(weapon_list[i].weapon_name, false) != NULL)
			{
				AddToList((void **) &action_model.weapon_list, sizeof(action_model_weapon_t), &action_model.weapon_list_size);
				action_model.weapon_list[action_model.weapon_list_size - 1].weapon_index = i;
			}
		}
	}

	AddToList((void **) &action_model_list, sizeof(action_model_t), &action_model_list_size);
	action_model_list[action_model_list_size - 1] = action_model;
}

//---------------------------------------------------------------------------------
// Purpose: Find index of skin_list for specified model name
//---------------------------------------------------------------------------------
static
int IsValidSkinName(const char *skin_name)
{
	for (int i = 0; i < skin_list_size; i++)
	{
		if (FStrEq(skin_list[i].skin_name, skin_name))
		{
			return i;
		}
	}

	// No match found
	return -1;
}
//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the skins used for a single file
//---------------------------------------------------------------------------------
static
bool LoadSkinType
(
 const char *skin_directory, 
 const char *filename,
 const char *core_filename,
 const int	skin_type
 )
{
	FileHandle_t file_handle;
	FileHandle_t file_handle2;
	char	skin_details[512];
	char	resource_details[256];
	char	skin_name[512];
	bool	result = true;

	file_handle = filesystem->Open (core_filename, "rt", NULL);
	if (file_handle == NULL)
	{
//		Msg ("Failed to load [%s]\n", filename);
		result = false;
	}
	else
	{
//		Msg("%s\n", filename);
		while (filesystem->ReadLine (skin_details, 512, file_handle) != NULL)
		{
			if (!ParseAliasLine(skin_details, skin_name, true))
			{
				// String is empty after parsing
				continue;
			}

			char	exists_string[512];

			// Check file exists on server
			Q_snprintf(exists_string, sizeof(exists_string), "%s%s", skin_directory, skin_details);
			if (!filesystem->FileExists(exists_string))
			{
//				Msg("Warning did not find [%s%s]\n", skin_directory, skin_details);
				continue;
			}

			AddToList((void **) &skin_list, sizeof(skin_t), &skin_list_size);
			skin_list[skin_list_size - 1].skin_type = skin_type;
			Q_snprintf(skin_list[skin_list_size - 1].skin_name, sizeof(skin_list[skin_list_size - 1].skin_name), "%s", skin_name);
			skin_list[skin_list_size - 1].model_index = -1;
			skin_list[skin_list_size - 1].resource_list = NULL;
			skin_list[skin_list_size - 1].resource_list_size = 0;
			Q_strcpy(skin_list[skin_list_size - 1].precache_name, "");

//			Msg("Resource file [%s]\n", exists_string);

			file_handle2 = filesystem->Open (exists_string, "rt", NULL);
			if (file_handle2 == NULL)
			{
//				Msg ("Failed to load [%s]\n", exists_string);
			}
			else
			{
				while (filesystem->ReadLine (resource_details, 512, file_handle2) != NULL)
				{				
					if (!ParseLine(resource_details, true))
					{
						// String is empty after parsing
						continue;
					}

					Q_snprintf(exists_string, sizeof(exists_string), "%s", resource_details);
					if (!filesystem->FileExists(exists_string))
					{
//						Msg("ERROR !!! Did not find resource file [%s]\n", resource_details);
						continue;
					}

					int details_length = Q_strlen(resource_details);

					if (details_length > 4)
					{
						if (resource_details[details_length - 4] == '.' &&
							resource_details[details_length - 3] == 'm' &&
							resource_details[details_length - 2] == 'd' &&
							resource_details[details_length - 1] == 'l')
						{
							Q_strcpy (skin_list[skin_list_size - 1].precache_name, resource_details);
						}
					}

					AddToList((void **) &(skin_list[skin_list_size - 1].resource_list), 
								sizeof(skin_resource_t), 
								&(skin_list[skin_list_size - 1].resource_list_size));
					Q_strcpy(skin_list[skin_list_size - 1].resource_list[skin_list[skin_list_size - 1].resource_list_size - 1].resource, resource_details);
				}

				filesystem->Close(file_handle2);
			}

			int found_index = -1;

			if (skin_list_size > 1)
			{
				for (int i = 0; i < skin_list_size - 2; i++)
				{
					if (FStrEq(skin_list[skin_list_size - 1].precache_name, skin_list[i].precache_name))
					{
						found_index = skin_list[i].model_index;
						break;
					}
				}
			}

			if (found_index == -1)
			{
				// Precache index
				skin_list[skin_list_size - 1].model_index = engine->PrecacheModel(skin_list[skin_list_size - 1].precache_name, true);
			}
			else
			{
				// Use existing model index
				skin_list[skin_list_size - 1].model_index = found_index;
			}

//			Msg("Loaded model [%s] Precached [%s] with %i resources\n", 
//								skin_list[skin_list_size - 1].skin_name,
//								skin_list[skin_list_size - 1].precache_name,
//								skin_list[skin_list_size - 1].resource_list_size);

		}

		filesystem->Close(file_handle);
	}

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: Player has disconnected
//---------------------------------------------------------------------------------
void SkinPlayerDisconnect
(
 player_t	*player_ptr
)
{
	current_skin_list[player_ptr->index - 1].team_id = -1;
}

//---------------------------------------------------------------------------------
// Purpose: Reset all players to -1 team index
//---------------------------------------------------------------------------------
void SkinResetTeamID(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		current_skin_list[i].team_id = -1;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Force client to use the skin
//---------------------------------------------------------------------------------
void SkinTeamJoin
(
 player_t	*player_ptr
 )
{
	if (mani_skins_admin.GetInt() == 0 &&
		mani_skins_public.GetInt() == 0 &&
		mani_skins_reserved.GetInt() == 0) return;

	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
	{
		// Not on any team
		return;
	}

	if (player_ptr->player_info->IsHLTV()) return;
	if (FStrEq(player_ptr->steam_id, "BOT")) return;

	player_settings_t	*player_settings;
	player_settings = FindPlayerSettings(player_ptr);
	if (!player_settings) return;
	
	if (mani_skins_force_choose_on_join.GetInt() != 0)
	{
		if (current_skin_list[player_ptr->index - 1].team_id != player_ptr->team)
		{
			if (mani_skins_force_choose_on_join.GetInt() == 1)
			{			
				// Player changed to new team so give them menu option
				engine->ClientCommand(player_ptr->entity, "manisettings joinskin\n");
				return;
			}
			else
			{
				// Player changed to new team so give them settings menu option
				engine->ClientCommand(player_ptr->entity, "manisettings\n");
				current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;
				return;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Force client to use the skin
//---------------------------------------------------------------------------------
void ForceSkinType
(
 player_t	*player_ptr
 )
{
	if (war_mode) return;

	if (mani_skins_admin.GetInt() == 0 &&
		mani_skins_public.GetInt() == 0 &&
		mani_skins_reserved.GetInt() == 0) return;

	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
	{
		// Not on any team
		return;
	}

	if (player_ptr->is_bot && mani_skins_random_bot_skins.GetInt() == 1)
	{
		int skin_type = 0;

		if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
		{
			skin_type = MANI_T_SKIN;
		}
		else
		{
			skin_type = MANI_CT_SKIN;
		}

		int skin_count = 0;

		for (int i = 0; i < skin_list_size; i ++)
		{
			if (skin_list[i].skin_type == skin_type)
			{
				skin_count ++;
			}
		}

		if (skin_count == 0) return;

		if (mani_skins_force_public.GetInt() == 0) 
		{
			skin_count ++;
		}

		int	chosen_skin = rand() % skin_count;

		if (chosen_skin == 0 && mani_skins_force_public.GetInt() == 0)
		{
			// Use default skin
			return;
		}

		if (mani_skins_force_public.GetInt() == 0)
		{
			chosen_skin --;
		}

		for (int i = 0; i < skin_list_size; i ++)
		{
			if (skin_list[i].skin_type == skin_type)
			{
				chosen_skin --;
				if (chosen_skin == -1)
				{
					Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
					return;
				}
			}
		}

		return;
	}

	if (FStrEq(player_ptr->steam_id, "BOT")) return;
	if (player_ptr->player_info->IsHLTV()) return;

	player_settings_t	*player_settings;
	player_settings = FindPlayerSettings(player_ptr);
	if (!player_settings) return;
	
	int admin_index = -1;

	/* Do admin skins */
	for (;;)
	{
		if (mani_skins_admin.GetInt() != 0)
		{
			if (gpManiClient->IsAdmin(player_ptr, &admin_index))
			{
				if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
				{
					// Use admin skin for this player
					if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
					{
						// No model in use
						if (!player_settings->admin_t_model) break;
						if (FStrEq(player_settings->admin_t_model,"")) break;

						for (int i = 0; i < skin_list_size; i++)
						{
							if (skin_list[i].skin_type == MANI_ADMIN_T_SKIN)
							{
								if (FStrEq(skin_list[i].skin_name, player_settings->admin_t_model))
								{
									Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
									return;
								}
							}
						}

						/* Nothing found */
						player_settings = FindStoredPlayerSettings(player_ptr);
						Q_strcpy(player_settings->admin_t_model, "");
						UpdatePlayerSettings(player_ptr, player_settings);
					}
					else if (player_ptr->team == TEAM_B)
					{
						// No model in use
						if (!player_settings->admin_ct_model) break;
						if (FStrEq(player_settings->admin_ct_model,"")) break;

						for (int i = 0; i < skin_list_size; i++)
						{
							if (skin_list[i].skin_type == MANI_ADMIN_CT_SKIN)
							{
								if (FStrEq(skin_list[i].skin_name, player_settings->admin_ct_model))
								{
									Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
									return;
								}
							}
						}

						/* Nothing found */
						player_settings = FindStoredPlayerSettings(player_ptr);
						Q_strcpy(player_settings->admin_ct_model, "");
						UpdatePlayerSettings(player_ptr, player_settings);
					}
				}
			}
		}

		break;
	}

	int immunity_index = -1;

	/* Do reserve skins */
	for (;;)
	{
		if (mani_skins_reserved.GetInt() != 0)
		{
			if (gpManiClient->IsImmune(player_ptr, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
				{
					// Use admin skin for this player
					if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
					{
						// No model in use
						if (!player_settings->immunity_t_model) break;
						if (FStrEq(player_settings->immunity_t_model,"")) break;

						for (int i = 0; i < skin_list_size; i++)
						{
							if (skin_list[i].skin_type == MANI_RESERVE_T_SKIN)
							{
								if (FStrEq(skin_list[i].skin_name, player_settings->immunity_t_model))
								{
									Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
									return;
								}
							}
						}

						/* Nothing found */
						player_settings = FindStoredPlayerSettings(player_ptr);
						Q_strcpy(player_settings->immunity_t_model, "");
						UpdatePlayerSettings(player_ptr, player_settings);
					}
					else if (player_ptr->team == TEAM_B)
					{
						// No model in use
						if (!player_settings->immunity_ct_model) break;
						if (FStrEq(player_settings->immunity_ct_model,"")) break;

						for (int i = 0; i < skin_list_size; i++)
						{
							if (skin_list[i].skin_type == MANI_RESERVE_CT_SKIN)
							{
								if (FStrEq(skin_list[i].skin_name, player_settings->immunity_ct_model))
								{
									Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
									return;
								}
							}
						}

						/* Nothing found */
						player_settings = FindStoredPlayerSettings(player_ptr);
						Q_strcpy(player_settings->immunity_ct_model, "");
						UpdatePlayerSettings(player_ptr, player_settings);
					}
				}
			}
		}

		break;
	}

	/* Do public skins */
	for (;;)
	{
		if (mani_skins_public.GetInt() != 0)
		{
			// Use admin skin for this player
			if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
			{
				// No model in use
				if (!player_settings->t_model) break;
				if (FStrEq(player_settings->t_model,"")) break;

				for (int i = 0; i < skin_list_size; i++)
				{
					if (skin_list[i].skin_type == MANI_T_SKIN)
					{
						if (mani_skins_force_public.GetInt() == 1)
						{
							Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
							return;
						}

						if (FStrEq(skin_list[i].skin_name, player_settings->t_model))
						{
							Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
							return;
						}
					}
				}

				/* Nothing found */
				player_settings = FindStoredPlayerSettings(player_ptr);
				Q_strcpy(player_settings->t_model, "");
				UpdatePlayerSettings(player_ptr, player_settings);
			}
			else if (player_ptr->team == TEAM_B)
			{
				// No model in use
				if (!player_settings->ct_model) break;
				if (FStrEq(player_settings->ct_model,"")) break;
				for (int i = 0; i < skin_list_size; i++)
				{
					if (skin_list[i].skin_type == MANI_CT_SKIN)
					{
						if (mani_skins_force_public.GetInt() == 1)
						{
							Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
							return;
						}

						if (FStrEq(skin_list[i].skin_name, player_settings->ct_model))
						{
							Prop_SetModelIndex(player_ptr->entity, skin_list[i].model_index);
							return;
						}
					}
				}

				/* Nothing found */
				player_settings = FindStoredPlayerSettings(player_ptr);
				Q_strcpy(player_settings->ct_model, "");
				UpdatePlayerSettings(player_ptr, player_settings);
			}
		}

		break;
	}
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ProcessJoinSkinChoiceMenu
( 
  player_t *player_ptr, 
  int next_index, 
  int argv_offset, 
  char *menu_command 
)
{
	const int argc = engine->Cmd_Argc();
	int	skin_type;

	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;
	if (mani_skins_admin.GetInt() == 0 &&
		mani_skins_reserved.GetInt() == 0 &&
		mani_skins_public.GetInt() == 0)
	{
		// Skins not enabled
		return;
	}

	if (argc - argv_offset == 3)
	{
		char	skin_name[20];
		if (current_skin_list[player_ptr->index - 1].team_id == player_ptr->team)
		{
			return;
		}

		current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;

		int index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));

		if (index == 0) return;

		if (index > skin_list_size && index != 999) return;
		if (index != 999) index --;

		// Resonable number passed in
		// Need to validate player can actually choose this skin though

		if (!IsSkinValidForPlayer(&index, player_ptr))
		{
			return;
		}

		if (index == 999)
		{
			// Found name to match !!!
			player_settings_t *player_settings;
			player_settings = FindStoredPlayerSettings(player_ptr);
			if (player_settings)
			{
				if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
				{
					Q_strcpy(player_settings->admin_ct_model, "");
					Q_strcpy(player_settings->immunity_ct_model, "");
					Q_strcpy(player_settings->ct_model, "");
				}
				else
				{
					Q_strcpy(player_settings->admin_t_model, "");
					Q_strcpy(player_settings->immunity_t_model, "");
					Q_strcpy(player_settings->t_model, "");
				}

				UpdatePlayerSettings(player_ptr, player_settings);
			}
			
			return;	
		}
		else
		{
			Q_strcpy(skin_name, skin_list[index].skin_name);
		}

		skin_type = skin_list[index].skin_type;

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
		}

		return;
	}
	else
	{
		if (current_skin_list[player_ptr->index - 1].team_id == player_ptr->team)
		{
			return;
		}

		// Setup player list
		FreeMenu();

		if (mani_skins_admin.GetInt() != 0)
		{
			int admin_index = -1;

			if (gpManiClient->IsAdmin(player_ptr, &admin_index))
			{
				if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
				{
					if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
					{
						skin_type = MANI_ADMIN_T_SKIN;
					}
					else
					{
						skin_type = MANI_ADMIN_CT_SKIN;
					}

					for (int i = 0; i < skin_list_size; i ++)
					{
						if (skin_list[i].skin_type == skin_type)
						{
							AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
							Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Admin : %s", skin_list[i].skin_name);							
							Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s %i", menu_command, i + 1);
						}
					}
				}
			}
		}

		if (mani_skins_reserved.GetInt() != 0)
		{
			int immunity_index = -1;

			if (gpManiClient->IsImmune(player_ptr, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
				{
					if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
					{
						skin_type = MANI_RESERVE_T_SKIN;
					}
					else
					{
						skin_type = MANI_RESERVE_CT_SKIN;
					}

					for (int i = 0; i < skin_list_size; i ++)
					{
						if (skin_list[i].skin_type == skin_type)
						{
							AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
							Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Reserved : %s", skin_list[i].skin_name);							
							Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s %i", menu_command, i + 1);
						}
					}
				}
			}
		}

		if (mani_skins_public.GetInt() != 0)
		{
			if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
			{
				skin_type = MANI_T_SKIN;
			}
			else
			{
				skin_type = MANI_CT_SKIN;
			}

			for (int i = 0; i < skin_list_size; i ++)
			{
				if (skin_list[i].skin_type == skin_type)
				{
					AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
					Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", skin_list[i].skin_name);							
					Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s %i", menu_command, i + 1);
				}
			}
		}

		if (menu_list_size == 0)
		{
			return;
		}

		if (mani_skins_force_public.GetInt() == 0)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Standard");							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "manisettings %s 999", menu_command);
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player_ptr, "Press Esc to choose skin", "Choose your skin", next_index, "manisettings", menu_command, false, -1);
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if skin is ok for player to use
//---------------------------------------------------------------------------------
static
bool	IsSkinValidForPlayer
(
 int *skin_index_ptr,
 player_t *player_ptr
 )
{
	int skin_type;

	// If skin is 'None' (999) check if we need to default it
	if (*skin_index_ptr == 999 && mani_skins_force_public.GetInt() == 1)
	{
		if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed())
		{
			skin_type = MANI_CT_SKIN;
		}
		else
		{
			skin_type = MANI_T_SKIN;
		}

		for (int i = 0; i < skin_list_size; i ++)
		{
			if (skin_list[i].skin_type = skin_type)
			{
				*skin_index_ptr = i;
				return true;
			}
		}

		return false;
	}

	if (mani_skins_admin.GetInt() != 0)
	{
		int admin_index = -1;

		if (gpManiClient->IsAdmin(player_ptr, &admin_index))
		{
			if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_SKINS))
			{
				if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
				{
					skin_type = MANI_ADMIN_T_SKIN;
				}
				else
				{
					skin_type = MANI_ADMIN_CT_SKIN;
				}

				for (int i = 0; i < skin_list_size; i ++)
				{
					if (skin_list[i].skin_type == skin_type)
					{
						if (*skin_index_ptr == i) return true;
					}
				}
			}
		}
	}

	if (mani_skins_reserved.GetInt() != 0)
	{
		int immunity_index = -1;

		if (gpManiClient->IsImmune(player_ptr, &immunity_index))
		{
			if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_RESERVE_SKIN))
			{
				if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
				{
					skin_type = MANI_RESERVE_T_SKIN;
				}
				else
				{
					skin_type = MANI_RESERVE_CT_SKIN;
				}

				for (int i = 0; i < skin_list_size; i ++)
				{
					if (skin_list[i].skin_type == skin_type)
					{
						if (*skin_index_ptr == i) return true;
					}
				}
			}
		}
	}

	if (mani_skins_public.GetInt() != 0)
	{
		if (player_ptr->team == TEAM_A || !gpManiGameType->IsTeamPlayAllowed()) 
		{
			skin_type = MANI_T_SKIN;
		}
		else
		{
			skin_type = MANI_CT_SKIN;
		}

		for (int i = 0; i < skin_list_size; i ++)
		{
			if (skin_list[i].skin_type == skin_type)
			{
				if (*skin_index_ptr == i) return true;
			}
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Setup the skins for auto download to client
//---------------------------------------------------------------------------------
static
void	SetupSkinsAutoDownloads(void)
{
	if (mani_skins_auto_download.GetInt() == 0) return;

	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);
	char	res_string[256];

	for (int i = 0; i < skin_list_size; i++)
	{
		for (int j = 0; j < skin_list[i].resource_list_size; j ++)
		{
			// Set up .res downloadables
			if (pDownloadablesTable)
			{
				Q_snprintf(res_string, sizeof(res_string), "%s", skin_list[i].resource_list[j].resource);
				pDownloadablesTable->AddString(res_string, sizeof(res_string));
			}
		}
	}

	engine->LockNetworkStringTables(save);
}

//---------------------------------------------------------------------------------
// Purpose: Setup the skins for auto download to client
//---------------------------------------------------------------------------------
static
void	SetupActionModelsAutoDownloads(void)
{
	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);
	char	res_string[256];

	for (int i = 0; i < action_model_list_size; i++)
	{
		for (int j = 0; j < action_model_list[i].sound_list_size; j ++)
		{
			if (action_model_list[i].sound_list[j].auto_download != 1) continue;

			// Set up .res downloadables
			if (pDownloadablesTable)
			{
				Q_snprintf(res_string, sizeof(res_string), "sound/%s", action_model_list[i].sound_list[j].sound_file);
				if (filesystem->FileExists(res_string))
				{
					pDownloadablesTable->AddString(res_string, sizeof(res_string));
				}
				else
				{
//					Msg("Sound file [%s] does not exist on server !!\n", res_string);
				}
			}
		}
	}

	engine->LockNetworkStringTables(save);
}

//---------------------------------------------------------------------------------
// Purpose: Handle change of server sound download cvar
//---------------------------------------------------------------------------------
static void ManiSkinsAutoDownload ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (atoi(var->GetString()) == 1)
		{
			SetupSkinsAutoDownloads();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void ProcessMenuSkinOptions (player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		int found_skin = -1;
		for (int i = 0; i < skin_list_size; i ++)
		{
			if (mani_skins_setskin_misc_only.GetInt() == 1 &&
				skin_list[i].skin_type != MANI_MISC_SKIN)
			{
				continue;
			}

			if (FStrEq(skin_list[i].skin_name, engine->Cmd_Argv(2 + argv_offset)))
			{
				found_skin = i;
				break;
			}
		}

		if (found_skin != -1)
		{
			/* Find Player menu */
			engine->ClientCommand(admin->entity, "admin setskin \"%s\"\n", skin_list[i].skin_name);
		}

		return;
	}
	else
	{
		// Setup skin list to choose
		FreeMenu();

		for( int i = 0; i < skin_list_size; i++ )
		{
			if (mani_skins_setskin_misc_only.GetInt() == 1 &&
				skin_list[i].skin_type != MANI_MISC_SKIN)
			{
				continue;
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", skin_list[i].skin_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin skinoptions \"%s\"", skin_list[i].skin_name);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SKINOPTIONS_MENU_ESCAPE), Translate(M_SKINOPTIONS_MENU_TITLE), next_index, "admin", "skinoptions", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void ProcessMenuSetSkin (player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		// Slap the player
		ProcessMaSetSkin (admin->index, false, 3, "ma_setskin", engine->Cmd_Argv(3 + argv_offset), engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)	continue;

			int immunity_index = -1;
			if (admin->index != player.index && gpManiClient->IsImmune(&player, &immunity_index))
			{
				if (gpManiClient->IsImmunityAllowed(immunity_index, IMMUNITY_ALLOW_SETSKIN))
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin setskin \"%s\" %i", engine->Cmd_Argv(2 + argv_offset), player.user_id);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char more_cmd[128];
		Q_snprintf( more_cmd, sizeof(more_cmd), "setskin \"%s\"", engine->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SETSKIN_MENU_ESCAPE), Translate(M_SETSKIN_MENU_TITLE), next_index, "admin", more_cmd, true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setskin command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSetSkin
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *skin_name
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, "ma_setskin", ALLOW_SETSKINS, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <skin name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <skin name>", command_string);
		}

		return PLUGIN_STOP;
	}
	
	int found_skin = -1;
	for (int i = 0; i < skin_list_size; i++)
	{
		if (FStrEq(skin_list[i].skin_name, skin_name))
		{
			found_skin = i;
			break;
		}
	}

	if (found_skin == -1)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Invalid skin name [%s]\n", skin_name);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Invalid skin name [%s]", skin_name);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_SETSKIN))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

//		CBaseEntity *pPlayer = target_player_list[i].entity->GetUnknown()->GetBaseEntity();
//		CBaseEntity_SetModelIndex(pPlayer, skin_list[found_skin].model_index);
		Prop_SetModelIndex(target_player_list[i].entity, skin_list[found_skin].model_index);

		LogCommand (player.entity, "skinned user [%s] [%s] with skin %s\n", target_player_list[i].name, target_player_list[i].steam_id, skin_name);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminsetskin_anonymous.GetInt(), "has set player %s to have skin %s", target_player_list[i].name, skin_name ); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Force Skin client cvars on ClientActive
//---------------------------------------------------------------------------------
void	ForceSkinCExec(player_t *player_ptr)
{

	if (war_mode) return;

	if (mani_skins_force_cl_minmodels.GetInt() && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		engine->ClientCommand(player_ptr->entity, "cl_minmodels 1");
		engine->ClientCommand(player_ptr->entity, "cl_min_t 4");
		engine->ClientCommand(player_ptr->entity, "cl_min_ct 4");
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Con command for setting skins for models
//---------------------------------------------------------------------------------

CON_COMMAND(ma_setskin, "Changes a players skin to the requested model")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaSetSkin
			(
			0, // Client index
			true,  // Sever console command type
			engine->Cmd_Argc(), // Number of arguments
			engine->Cmd_Argv(0), // Command
			engine->Cmd_Argv(1), // player
			engine->Cmd_Argv(2) // Model name
			);

	return;
}

/*
CON_COMMAND( EntityList, "Liste des entites sur le serveur" )
{   edict_t *zEntity;
   int edictCount = engine->GetEntityCount();
   Msg("Nombre d'entite : %d\n",edictCount);
   for(int i=0;i<edictCount;i++)
   {   zEntity = engine->PEntityOfEntIndex(i);
      if(zEntity)
	  {
			if (FStrEq(zEntity->GetClassName(), "cs_team_manager"))
			{
				CBaseEntity *pTeam = zEntity->GetUnknown()->GetBaseEntity();
				CTeam *team_a = (CTeam *) pTeam;
				Msg("Team [%i] = [%s]\n", i, team_a->GetClassname());
				Msg("Team number [%i]\n", team_a->GetTeamNumber());
				Msg("Team name = [%s]\n", team_a->GetName());
			}

			if (FStrEq("cs_gamerules", zEntity->GetClassName()))
			{
				Msg("Gamerules\n");
			}
	  }
   }   

} */
