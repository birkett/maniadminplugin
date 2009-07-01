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
#include "mani_admin.h"
#include "mani_gametype.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;

extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiGameType::ManiGameType()
{
	// Init
	gpManiGameType = this;
}

ManiGameType::~ManiGameType()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiGameType::Init(void)
{
	char	core_filename[256];
	team_class_t	temp_team;

	Msg("*********** Loading gametypes.txt ************\n");
	// Read the gametype.txt file

	KeyValues *kv_ptr = new KeyValues("gametypes.txt");

	Q_strcpy(game_type, serverdll->GetGameDescription());
	Msg("Searching for game type [%s]\n", game_type);

	if (FStrEq(MANI_GAME_CSS_STR, game_type)) game_type_index = MANI_GAME_CSS;
	else if (FStrEq(MANI_GAME_DM_STR, game_type)) game_type_index = MANI_GAME_DM;
	else if (FStrEq(MANI_GAME_TEAM_DM_STR, game_type)) game_type_index = MANI_GAME_TEAM_DM;
	else if (FStrEq(MANI_GAME_CTF_STR, game_type)) game_type_index = MANI_GAME_CTF;
	else if (FStrEq(MANI_GAME_HIDDEN_STR, game_type)) game_type_index = MANI_GAME_HIDDEN;
	else if (FStrEq(MANI_GAME_GARRYS_MOD_STR, game_type)) game_type_index = MANI_GAME_GARRYS_MOD;
	else if (FStrEq(MANI_GAME_DOD_STR1, game_type)) game_type_index = MANI_GAME_DOD;
	else if (FStrEq(MANI_GAME_DOD_STR2, game_type)) game_type_index = MANI_GAME_DOD;
	else game_type_index = MANI_GAME_UNKNOWN;

	DefaultValues();

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/gametypes.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		Msg("Failed to load gametypes.txt\n");
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

	// Find our game type base key
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), game_type))
		{
			found_match = true;
			Msg("Found gametypes for %s\n", game_type);
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	// No match found so find the default key
	if (!found_match)
	{
		base_key_ptr = kv_ptr->GetFirstTrueSubKey();

		found_match = false;

		// Find default mod base key
		for (;;)
		{
			if (FStrEq(base_key_ptr->GetName(), "Unknown Mod"))
			{
				found_match = true;
				Msg("Using class unknown mod for defaults\n");
				break;
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				kv_ptr->deleteThis();
				return;
			}
		}
	}

	//base_key_ptr should now hold our core key for our game type defaults
	Q_strcpy(linux_game_bin, base_key_ptr->GetString("linux_bin", "nothing"));
	spectator_allowed = base_key_ptr->GetInt("spectator_allowed", 0);
	spectator_index = base_key_ptr->GetInt("spectator_index", 1);
	Q_strcpy(spectator_group, base_key_ptr->GetString("spectator_group", "#SPEC"));
	hl1_menu_compatible = base_key_ptr->GetInt("hl1_menu_compatible", 0);
	team_play = base_key_ptr->GetInt("team_play", 0);
	max_messages = base_key_ptr->GetInt("max_messages", 22);
	set_colour = base_key_ptr->GetInt("set_colour_allowed", 1);
	alpha_render_mode = base_key_ptr->GetInt("alpha_render_mode", 1);

	// Dod requires this atm
	slap_allowed = base_key_ptr->GetInt("slap_allowed", 1);
	drug_allowed = base_key_ptr->GetInt("drug_allowed", 1);
	teleport_allowed = base_key_ptr->GetInt("teleport_allowed", 1);
	fire_allowed = base_key_ptr->GetInt("fire_allowed", 1);
	advert_decal_allowed = base_key_ptr->GetInt("advert_decal_allowed", 1);

	KeyValues *temp_ptr;

	advanced_effects = 0;
	advanced_effects_vfunc_offset = 12;
	advanced_effects_code_offset = 110;

	// Allow advanced effects via te
	temp_ptr = base_key_ptr->FindKey("advanced_effects", false);
	if (temp_ptr)
	{
		Msg("Found advanced effects\n");
#ifdef __linux__
        advanced_effects = temp_ptr->GetInt("enable_linux", 0);
#else
		advanced_effects = temp_ptr->GetInt("enable_win", 0);
		advanced_effects_vfunc_offset = temp_ptr->GetInt("vfunc_offset", 12);
		advanced_effects_code_offset = temp_ptr->GetInt("code_offset", 110);
#endif
	}


	// Allow advanced effects via te
	temp_ptr = base_key_ptr->FindKey("voice_control", false);
	if (temp_ptr)
	{
		Msg("Found voice control\n");

		voice_allowed = 1;
#ifdef __linux__
        voice_offset = temp_ptr->GetInt("linux_offset", 3);
#else
		voice_offset = temp_ptr->GetInt("win_offset", 2);
#endif
	}


	cash_allowed = 0;

	// Find cash offset
	temp_ptr = base_key_ptr->FindKey("cash_offset", false);
	if (temp_ptr)
	{
		Msg("Found cash offset\n");
		cash_allowed = 1;

#ifdef __linux__
		cash_offset = temp_ptr->GetInt("linux_offset", 880);
#else
		cash_offset = temp_ptr->GetInt("win_offset", 875);
#endif
	}

	kills_allowed = 0;

	// Find cash offset
	temp_ptr = base_key_ptr->FindKey("kills_offset", false);
	if (temp_ptr)
	{
		Msg("Found kills offset\n");
		kills_allowed = 1;

#ifdef __linux__
		kills_offset = temp_ptr->GetInt("linux_offset", 747);
#else
		kills_offset = temp_ptr->GetInt("win_offset", 742);
#endif
	}

	// Handle teams
	temp_ptr = base_key_ptr->FindKey("teams", false);
	if (temp_ptr)
	{
		Msg ("Found teams\n");
		KeyValues *team_ptr;

		team_ptr = temp_ptr->GetFirstTrueSubKey();
		if (team_ptr)
		{
			for (;;)
			{
				Q_memset(&temp_team, 0, sizeof(team_class_t));

				temp_team.team_index = team_ptr->GetInt("index", -1);
				temp_team.team_short_translation_index = team_ptr->GetInt("short_translation_index", 0); // CT, T, C, R etc
				temp_team.team_translation_index = team_ptr->GetInt("translation_index", 0); // Long proper name
				Q_strcpy(temp_team.group, team_ptr->GetString("group","#DEF"));

				switch (temp_team.team_index)
				{
				case 0: // single player game
					Q_strcpy(temp_team.admin_skin_dir, team_ptr->GetString("admin_skin","admin"));
					Q_strcpy(temp_team.reserved_skin_dir, team_ptr->GetString("reserved_skin","reserved"));
					Q_strcpy(temp_team.public_skin_dir, team_ptr->GetString("public_skin","public"));
					break;
				case TEAM_A: // Normally T as CSS
					Q_strcpy(temp_team.admin_skin_dir, team_ptr->GetString("admin_skin","admin_t"));
					Q_strcpy(temp_team.reserved_skin_dir, team_ptr->GetString("reserved_skin","reserved_t"));
					Q_strcpy(temp_team.public_skin_dir, team_ptr->GetString("public_skin","public_t"));
					break;
				case TEAM_B: // Normally CT as CSS
					Q_strcpy(temp_team.admin_skin_dir, team_ptr->GetString("admin_skin","admin_ct"));
					Q_strcpy(temp_team.reserved_skin_dir, team_ptr->GetString("reserved_skin","reserved_ct"));
					Q_strcpy(temp_team.public_skin_dir, team_ptr->GetString("public_skin","public_ct"));
					break;
				default : // Just default something !!
					Q_strcpy(temp_team.admin_skin_dir, team_ptr->GetString("admin_skin","admin"));
					Q_strcpy(temp_team.reserved_skin_dir, team_ptr->GetString("reserved_skin","reserved"));
					Q_strcpy(temp_team.public_skin_dir, team_ptr->GetString("public_skin","public"));
					break;
				}

				team_class_list[temp_team.team_index] = temp_team;
				Msg("Found team [%s]\n", temp_team.group);

				team_ptr = team_ptr->GetNextKey();
				if (!team_ptr)
				{
					// End of teams defined
					break;
				}

			}
		}
	}

	kv_ptr->deleteThis();

	Msg("*********** gametypes.txt loaded ************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Return the game type string
//---------------------------------------------------------------------------------
const char	*ManiGameType::GetGameType(void)
{
	return ((const char *) game_type);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if passed in string = game type string
//---------------------------------------------------------------------------------
bool	ManiGameType::IsGameType(const char *game_str)
{
	return ((FStrEq(game_type, game_str) ? true:false));
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if passed in integer = game type we know (faster than string compare)
//---------------------------------------------------------------------------------
bool	ManiGameType::IsGameType(int game_index)
{
	return ((game_index == game_type_index) ? true:false);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if advanced effects allowed via temp ents
//---------------------------------------------------------------------------------
bool	ManiGameType::GetAdvancedEffectsAllowed(void)
{
	return ((advanced_effects == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if set colour is allowed
//---------------------------------------------------------------------------------
bool	ManiGameType::IsSetColourAllowed(void)
{
	return ((set_colour == 0) ? false:true);
}
//---------------------------------------------------------------------------------
// Purpose: Set whether advanced effects can be used
//---------------------------------------------------------------------------------
void	ManiGameType::SetAdvancedEffectsAllowed(bool allowed)
{
	if (allowed)
	{
		advanced_effects = 1;
	}
	else
	{
		advanced_effects = 0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns the windows te vfunc offset
//---------------------------------------------------------------------------------
int		ManiGameType::GetAdvancedEffectsVFuncOffset(void)
{
	return (advanced_effects_vfunc_offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns the windows code offset within the virt function
//---------------------------------------------------------------------------------
int		ManiGameType::GetAdvancedEffectsCodeOffset(void)
{
	return (advanced_effects_code_offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns the linux bin and path used for dlsym
//---------------------------------------------------------------------------------
const char	*ManiGameType::GetLinuxBin(void)
{
	return ((const char *) linux_game_bin);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if team play allowed
//---------------------------------------------------------------------------------
bool	ManiGameType::IsTeamPlayAllowed(void)
{
	return ((team_play == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if spectators are allowed
//---------------------------------------------------------------------------------
bool	ManiGameType::IsSpectatorAllowed(void)
{
	return ((spectator_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns the spectator team index
//---------------------------------------------------------------------------------
int		ManiGameType::GetSpectatorIndex(void)
{
	return spectator_index;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the maximum number of messages to scan for
//---------------------------------------------------------------------------------
int		ManiGameType::GetMaxMessages(void)
{
	return max_messages;
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if voice hooking is allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsVoiceAllowed(void)
{
	return ((voice_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns virtual function index for voiceserver
//---------------------------------------------------------------------------------
int		ManiGameType::GetVoiceOffset(void)
{
	return (voice_offset);
}


//---------------------------------------------------------------------------------
// Purpose: Returns true if cash is allowed for the mod
//---------------------------------------------------------------------------------
bool		ManiGameType::IsCashAllowed(void)
{
	return ((cash_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns cash offset into player
//---------------------------------------------------------------------------------
int		ManiGameType::GetCashOffset(void)
{
	return (cash_offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if amx style menu is allowed in this mod
//---------------------------------------------------------------------------------
bool		ManiGameType::IsAMXMenuAllowed(void)
{
	return ((hl1_menu_compatible == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if slap allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsSlapAllowed(void)
{
	return ((slap_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if teleport allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsTeleportAllowed(void)
{
	return ((teleport_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if fire allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsFireAllowed(void)
{
	return ((fire_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if drug allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsDrugAllowed(void)
{
	return ((drug_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if advert bsp decals allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsAdvertDecalAllowed(void)
{
	return ((advert_decal_allowed == 0) ? false:true);
}


//---------------------------------------------------------------------------------
// Purpose: Check whether we use CBase suicide or ClientCommmand('kill')
//---------------------------------------------------------------------------------
/*bool		ManiGameType::IsClientSuicide(void)
{
	return ((client_suicide == 0) ? false:true);
}*/

//---------------------------------------------------------------------------------
// Purpose: Check whether we use decrement frags
//---------------------------------------------------------------------------------
bool		ManiGameType::IsKillsAllowed(void)
{
	return ((kills_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns kills offset into player
//---------------------------------------------------------------------------------
int		ManiGameType::GetKillsOffset(void)
{
	return (kills_offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns alpha render mode
//---------------------------------------------------------------------------------
int		ManiGameType::GetAlphaRenderMode(void)
{
	return (alpha_render_mode);
}

//---------------------------------------------------------------------------------
// Purpose: Returns the index of a team for a passed in group string
//---------------------------------------------------------------------------------
int			ManiGameType::GetIndexFromGroup(const char *group_id)
{
	for (int i = 0; i < 10; i++)
	{
		if (team_class_list[i].team_index == -1) continue;
		if (FStrEq(team_class_list[i].group, group_id))
		{
			return i;
		}
	}

	if (IsSpectatorAllowed())
	{
		if (FStrEq(group_id, spectator_group))
		{
			return spectator_index;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if the team index is part of the active teams
//---------------------------------------------------------------------------------
bool	ManiGameType::IsValidActiveTeam(int	index)
{
	if (index >= 10) return false;

	if (team_class_list[index].team_index == -1)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the 'short' translation index for an active team
//---------------------------------------------------------------------------------
int		ManiGameType::GetTeamShortTranslation(int index)
{
	if (IsValidActiveTeam(index))
	{
		return (team_class_list[index].team_short_translation_index);
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the 'long' translation index for an active team
//---------------------------------------------------------------------------------
int		ManiGameType::GetTeamTranslation(int index)
{
	if (IsValidActiveTeam(index))
	{
		return (team_class_list[index].team_translation_index);
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the opposing team index for the passed in team index
//---------------------------------------------------------------------------------
int		ManiGameType::GetOpposingTeam(int index)
{
	if (IsValidActiveTeam(index))
	{
		for (int i = 0; i < 10; i++)
		{
			if (team_class_list[i].team_index != -1 &&
				team_class_list[i].team_index != index)
			{
				return i;
			}
		}
	}

	return index;
}

//---------------------------------------------------------------------------------
// Purpose: Returns a char pointer to the skin directory/file name to use for the team
//---------------------------------------------------------------------------------
const char *ManiGameType::GetAdminSkinDir(int index)
{
	return (team_class_list[index].admin_skin_dir);
}

//---------------------------------------------------------------------------------
// Purpose: Returns a char pointer to the skin directory/file name to use for the team
//---------------------------------------------------------------------------------
const char *ManiGameType::GetReservedSkinDir(int index)
{
	return (team_class_list[index].reserved_skin_dir);
}

//---------------------------------------------------------------------------------
// Purpose: Returns a char pointer to the skin directory/file name to use for the team
//---------------------------------------------------------------------------------
const char *ManiGameType::GetPublicSkinDir(int index)
{
	return (team_class_list[index].public_skin_dir);
}

// Private member functions
//---------------------------------------------------------------------------------
// Purpose: Default some minimum values for operation
//---------------------------------------------------------------------------------

void	ManiGameType::DefaultValues(void)
{
	for (int i = 0; i < 10; i++)
	{
		team_class_list[i].team_index = -1;
	}

	Q_strcpy(linux_game_bin, "nothing");
	spectator_allowed = 0;
	spectator_index = 1;
	Q_strcpy(spectator_group, "#SPEC");
	hl1_menu_compatible = 0;
	team_play = 0;
	advanced_effects = 0;
	advanced_effects_vfunc_offset = 12;
	advanced_effects_code_offset = 110;
	max_messages = 22;
	voice_allowed = 0;
//	client_suicide = 1;
	kills_allowed = 0;
	set_colour = 1;
	alpha_render_mode = 1;

	// Dod requires this atm
	slap_allowed = 1;
	drug_allowed = 1;
	teleport_allowed = 1;
	fire_allowed = 1;
	advert_decal_allowed = 1;

	return ;
}

//---------------------------------------------------------------------------------
// Purpose: Find base KeyValues key for game type
//---------------------------------------------------------------------------------
bool	ManiGameType::FindBaseKey(KeyValues *kv, KeyValues *base_key_ptr)
{
	bool found_match;

	base_key_ptr = kv->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
		Msg("No true subkey found\n");
		return false;
	}

	found_match = false;

	// Find our game type base key
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), game_type))
		{
			found_match = true;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	// No match found so find the default key
	if (!found_match)
	{
		base_key_ptr = kv->GetFirstTrueSubKey();

		found_match = false;

		// Find default mod base key
		for (;;)
		{
			if (FStrEq(base_key_ptr->GetName(), "Unknown Mod"))
			{
				found_match = true;
				break;
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				return false;
			}
		}
	}

	return true;
}

CON_COMMAND(ma_forcegametype, "Forces the game type detection to run")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	gpManiGameType->Init();
	LoadSkins();
	return;
}

ManiGameType	g_ManiGameType;
ManiGameType	*gpManiGameType;
