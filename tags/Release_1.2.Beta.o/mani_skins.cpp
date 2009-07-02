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
#include "mani_commands.h"
#include "mani_vars.h"
#include "mani_help.h"
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

//	MMsg("************ LOADING SKINS *************\n");

	// Do Admin T load first
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(0));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(0));
	}
	else
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(TEAM_A));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(TEAM_A));
	}

    snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if(!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_T_SKIN))
	{
//		MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Admin T load first
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir );
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_T_SKIN);
	}

	// Do Admin CT load
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(TEAM_B));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetAdminSkinDir(TEAM_B));

		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_CT_SKIN))
		{
//			MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Admin CT load
			snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_ADMIN_CT_SKIN);
		}
	}

	// Do Reserved T load first
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(0));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(0));
	}
	else
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(TEAM_A));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(TEAM_A));
	}

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_T_SKIN))
	{
//		MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Reserved T load first
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_T_SKIN);
	}

	// Do Reserved CT load
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetReservedSkinDir(TEAM_B));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetReservedSkinDir(TEAM_B));

		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_CT_SKIN))
		{
//			MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Reserved CT load
			snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_RESERVE_CT_SKIN);
		}
	}

	// Do Public T load 
	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetAdminSkinDir(0));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(0));
	}
	else
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetPublicSkinDir(TEAM_A));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(TEAM_A));
	}

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
	snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
	if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_T_SKIN))
	{
//		MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
		// Do Public T load 
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
		LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_T_SKIN);
	}

	// Do Public CT load 
	if (gpManiGameType->IsTeamPlayAllowed())
	{
		snprintf(skin_config_file, sizeof(skin_config_file), "%s.txt", gpManiGameType->GetPublicSkinDir(TEAM_B));
		snprintf(skin_short_dir, sizeof(skin_short_dir), "%s", gpManiGameType->GetPublicSkinDir(TEAM_B));

		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/%s", mani_path.GetString(), current_map, skin_config_file);
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/%s/", mani_path.GetString(), current_map, skin_short_dir);
		if (!LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_CT_SKIN))
		{
//			MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/%s\n", mani_path.GetString(), current_map, skin_config_file);
			// Do Public CT load 
			snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/%s", mani_path.GetString(), skin_config_file);
			snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/%s/", mani_path.GetString(), skin_short_dir);
			LoadSkinType (skin_directory, skin_config_file, core_filename, MANI_CT_SKIN);
		}
	}

	// Do Misc load 
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/maps/%s/misc.txt", mani_path.GetString(), current_map);
	snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/maps/%s/misc/", mani_path.GetString(), current_map);
	if (!LoadSkinType (skin_directory, "misc.txt", core_filename, MANI_MISC_SKIN))
	{
//		MMsg("Above warning due to failed attempt to load custom map skin ./cfg/%s/skins/maps/%s/misc.txt\n", mani_path.GetString(), current_map);
		// Do Misc load 
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/skins/misc.txt", mani_path.GetString());
		snprintf(skin_directory, sizeof (skin_directory), "./cfg/%s/skins/misc/", mani_path.GetString());
		LoadSkinType (skin_directory, "misc.txt", core_filename, MANI_MISC_SKIN);
	}

	SetupSkinsAutoDownloads();
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
//		MMsg("Failed to load [%s]\n", filename);
		result = false;
	}
	else
	{
//		MMsg("%s\n", filename);
		while (filesystem->ReadLine (skin_details, 512, file_handle) != NULL)
		{
			if (!ParseAliasLine(skin_details, skin_name, true, false))
			{
				// String is empty after parsing
				continue;
			}

			char	exists_string[512];

			// Check file exists on server
			snprintf(exists_string, sizeof(exists_string), "%s%s", skin_directory, skin_details);
			if (!filesystem->FileExists(exists_string))
			{
//				MMsg("Warning did not find [%s%s]\n", skin_directory, skin_details);
				continue;
			}

			AddToList((void **) &skin_list, sizeof(skin_t), &skin_list_size);
			skin_list[skin_list_size - 1].skin_type = skin_type;
			snprintf(skin_list[skin_list_size - 1].skin_name, sizeof(skin_list[skin_list_size - 1].skin_name), "%s", skin_name);
			skin_list[skin_list_size - 1].model_index = -1;
			skin_list[skin_list_size - 1].resource_list = NULL;
			skin_list[skin_list_size - 1].resource_list_size = 0;
			Q_strcpy(skin_list[skin_list_size - 1].precache_name, "");

//			MMsg("Resource file [%s]\n", exists_string);

			file_handle2 = filesystem->Open (exists_string, "rt", NULL);
			if (file_handle2 == NULL)
			{
//				MMsg("Failed to load [%s]\n", exists_string);
			}
			else
			{
				while (filesystem->ReadLine (resource_details, 512, file_handle2) != NULL)
				{				
					if (!ParseLine(resource_details, true, false))
					{
						// String is empty after parsing
						continue;
					}

					snprintf(exists_string, sizeof(exists_string), "%s", resource_details);
					if (!filesystem->FileExists(exists_string))
					{
//						MMsg("ERROR !!! Did not find resource file [%s]\n", resource_details);
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

//			MMsg("Loaded model [%s] Precached [%s] with %i resources\n", 
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
				MENUPAGE_CREATE_FIRST(JoinSkinChoicePage, player_ptr, 0, -1);
				return;
			}
			else
			{
				// Player changed to new team so give them settings menu option
				current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;
				MENUPAGE_CREATE_FIRST(PlayerSettingsPage, player_ptr, 0, -1);
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
					Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
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
	
	/* Do admin skins */
	for (;;)
	{
		if (mani_skins_admin.GetInt() != 0)
		{
			if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SKINS))
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
								Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
								return;
							}
						}
					}

					/* Nothing found */
					Q_strcpy(player_settings->admin_t_model, "");
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
								Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
								return;
							}
						}
					}

					/* Nothing found */
					Q_strcpy(player_settings->admin_ct_model, "");
				}
			}
		}

		break;
	}

	/* Do reserve skins */
	for (;;)
	{
		if (mani_skins_reserved.GetInt() != 0)
		{
			if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_RESERVE_SKIN))
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
								Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
								return;
							}
						}
					}

					/* Nothing found */
					Q_strcpy(player_settings->immunity_t_model, "");
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
								Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
								return;
							}
						}
					}

					/* Nothing found */
					Q_strcpy(player_settings->immunity_ct_model, "");
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
							Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
							return;
						}

						if (FStrEq(skin_list[i].skin_name, player_settings->t_model))
						{
							Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
							return;
						}
					}
				}

				/* Nothing found */
				Q_strcpy(player_settings->t_model, "");
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
							Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
							return;
						}

						if (FStrEq(skin_list[i].skin_name, player_settings->ct_model))
						{
							Prop_SetVal(player_ptr->entity, MANI_PROP_MODEL_INDEX, skin_list[i].model_index);
							return;
						}
					}
				}

				/* Nothing found */
				Q_strcpy(player_settings->ct_model, "");
			}
		}

		break;
	}
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int JoinSkinChoiceItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int index;
	if (!this->params.GetParam("index", &index)) return CLOSE_MENU;

	char	skin_name[20];
	if (current_skin_list[player_ptr->index - 1].team_id == player_ptr->team) return CLOSE_MENU;
	current_skin_list[player_ptr->index - 1].team_id = player_ptr->team;
	if (index >= skin_list_size && index != 999) return CLOSE_MENU;

	// Resonable number passed in
	// Need to validate player can actually choose this skin though

	if (!IsSkinValidForPlayer(&index, player_ptr))
	{
		return CLOSE_MENU;
	}

	if (index == 999)
	{
		// Found name to match !!!
		player_settings_t *player_settings;
		player_settings = FindPlayerSettings(player_ptr);
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
		}
		
		return CLOSE_MENU;	
	}
	else
	{
		Q_strcpy(skin_name, skin_list[index].skin_name);
	}

	int skin_type = skin_list[index].skin_type;

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
	}

	return CLOSE_MENU;
}

bool JoinSkinChoicePage::PopulateMenuPage(player_t *player_ptr)
{
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return false;
	if (mani_skins_admin.GetInt() == 0 &&
		mani_skins_reserved.GetInt() == 0 &&
		mani_skins_public.GetInt() == 0)
	{
		// Skins not enabled
		return false;
	}

	int	skin_type;

	this->SetEscLink("Press Esc to choose skin");
	this->SetTitle("Choose your skin");

	if (current_skin_list[player_ptr->index - 1].team_id == player_ptr->team) return false;

	// Setup player list
	if (mani_skins_admin.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SKINS))
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
					MenuItem *ptr = new JoinSkinChoiceItem();
					ptr->SetDisplayText("Admin : %s", skin_list[i].skin_name);
					ptr->params.AddParam("index", i);
					this->AddItem(ptr);
				}
			}
		}
	}

	if (mani_skins_reserved.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_RESERVE_SKIN))
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
					MenuItem *ptr = new JoinSkinChoiceItem();
					ptr->SetDisplayText("Reserved : %s", skin_list[i].skin_name);	
					ptr->params.AddParam("index", i);
					this->AddItem(ptr);
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
				MenuItem *ptr = new JoinSkinChoiceItem();
				ptr->SetDisplayText("%s", skin_list[i].skin_name);
				ptr->params.AddParam("index", i);
				this->AddItem(ptr);
			}
		}
	}

	if (mani_skins_force_public.GetInt() == 0)
	{
		MenuItem *ptr = new JoinSkinChoiceItem();
		ptr->SetDisplayText("Standard");
		ptr->params.AddParam("index", 999);
		this->AddItem(ptr);
	}

	return true;
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
			if (skin_list[i].skin_type == skin_type)
			{
				*skin_index_ptr = i;
				return true;
			}
		}

		return false;
	}

	if (mani_skins_admin.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SKINS))
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

	if (mani_skins_reserved.GetInt() != 0)
	{
		if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_RESERVE_SKIN))
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
				snprintf(res_string, sizeof(res_string), "%s", skin_list[i].resource_list[j].resource);
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
				snprintf(res_string, sizeof(res_string), "sound/%s", action_model_list[i].sound_list[j].sound_file);
				if (filesystem->FileExists(res_string))
				{
					pDownloadablesTable->AddString(res_string, sizeof(res_string));
				}
				else
				{
//					MMsg("Sound file [%s] does not exist on server !!\n", res_string);
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
int SkinOptionsItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int	index;
	if (!this->params.GetParam("index", &index)) return CLOSE_MENU;

	MENUPAGE_CREATE_PARAM(SetSkinPage, player_ptr, AddParam("name", skin_list[index].skin_name), 0, -1);
	return NEW_MENU;
}

bool SkinOptionsPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 782));
	this->SetTitle("%s", Translate(player_ptr, 783));

	for( int i = 0; i < skin_list_size; i++ )
	{
		if (mani_skins_setskin_misc_only.GetInt() == 1 &&
			skin_list[i].skin_type != MANI_MISC_SKIN)
		{
			continue;
		}

		MenuItem *ptr = new SkinOptionsItem();
		ptr->SetDisplayText("%s", skin_list[i].skin_name);
		ptr->params.AddParam("index", i);
		this->AddItem(ptr);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Admin selects player to have skin applied to them
//---------------------------------------------------------------------------------
int SetSkinItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int	user_id;
	char *name;
	if (!this->params.GetParam("user_id", &user_id)) return CLOSE_MENU;
	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	gpCmd->NewCmd();
	gpCmd->AddParam("ma_setskin");
	gpCmd->AddParam("%i", user_id);
	gpCmd->AddParam("%s", name);
	ProcessMaSetSkin(player_ptr, "ma_setskin", 0, M_MENU);
	return RePopOption(REPOP_MENU);
}

bool SetSkinPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 780));
	this->SetTitle("%s", Translate(player_ptr, 781));

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead)	continue;

		if (player_ptr->index != player.index &&
			gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_SETSKIN))
		{
			continue;
		}

		MenuItem *ptr = new SetSkinItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParam("user_id", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setskin command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSetSkin(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *skin_name = gpCmd->Cmd_Argv(2);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SETSKINS, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

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
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Invalid skin name [%s]", skin_name);
		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SETSKIN))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			continue;
		}

//		CBaseEntity *pPlayer = target_player_list[i].entity->GetUnknown()->GetBaseEntity();
//		CBaseEntity_SetModelIndex(pPlayer, skin_list[found_skin].model_index);
		Prop_SetVal(target_player_list[i].entity, MANI_PROP_MODEL_INDEX, skin_list[found_skin].model_index);

		LogCommand (player_ptr, "skinned user [%s] [%s] with skin %s\n", target_player_list[i].name, target_player_list[i].steam_id, skin_name);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminsetskin_anonymous.GetInt(), "has set player %s to have skin %s", target_player_list[i].name, skin_name ); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for setting skins for models
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_setskin, 2027, MaSetSkin, false);


