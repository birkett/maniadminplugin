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
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_vfuncs.h"
#include "mani_commands.h"
#include "mani_vars.h"
#include "mani_weapon.h"
#include "mani_file.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;

extern	CGlobalVars *gpGlobals;
extern	ConVar *mp_allowspectators;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiGameType::ManiGameType()
{
	// Init
	for (int i = 0; i < 200; i++)
	{
		var_index[i].index = -2;
	}

	strcpy(var_index[MANI_VAR_DEATHS].name, "m_iDeaths"); var_index[MANI_VAR_DEATHS].index = -1;
	strcpy(var_index[MANI_VAR_FRAGS].name, "m_iFrags"); var_index[MANI_VAR_FRAGS].index = -1;
	strcpy(var_index[MANI_VAR_GRAVITY].name, "m_flGravity"); var_index[MANI_VAR_GRAVITY].index = -1;
	strcpy(var_index[MANI_VAR_FRICTION].name, "m_flFriction"); var_index[MANI_VAR_FRICTION].index = -1;
	strcpy(var_index[MANI_VAR_ELASTICITY].name, "m_flElasticity"); var_index[MANI_VAR_ELASTICITY].index = -1;

	gpManiGameType = this;
}

ManiGameType::~ManiGameType()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: GameFrame check if invalid gametypes.txt file
//---------------------------------------------------------------------------------
void ManiGameType::GameFrame(void)
{
	if (!show_out_of_date) return;

	time_t	current_time;

	time(&current_time);
	if (current_time > this->next_time_check)
	{
		SayToAll(LIGHT_GREEN_CHAT, true, "MANI-ADMIN-PLUGIN: Warning, your server plugin gametypes.txt file is out of date which will cause instability!"); 
		SayToAll(LIGHT_GREEN_CHAT, true, "Please download http://www.mani-admin-plugin.com/mani_admin_plugin/gametypes/gametypes.txt");
		MMsg("MANI-ADMIN-PLUGIN: Warning, your server plugin gametypes.txt file is out of date which will cause instability!\n"); 
		MMsg("Please download http://www.mani-admin-plugin.com/mani_admin_plugin/gametypes/gametypes.txt\n");
		next_time_check = current_time + 30;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiGameType::Init(void)
{
	char	core_filename[256];
	team_class_t	temp_team;

//	MMsg("*********** Loading gametypes.txt ************\n");
	// Read the gametype.txt file

	//Get filename
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/gametypes.txt", mani_path.GetString());

	// Check if file exists
	if (!filesystem->FileExists(core_filename))
	{
		for (int i = 0; i < 100; i++)
		{
			MMsg("WARNING! YOU ARE MISSING GAMETYPES.TXT THIS MUST BE INSTALLED!\n");
		}
		return;
	}

	KeyValues *kv_ptr = new KeyValues("gametypes.txt");

	Q_strcpy(game_type, serverdll->GetGameDescription());
	MMsg("Searching for game type [%s]\n", game_type);

	if (FStrEq(MANI_GAME_CSS_STR, game_type)) game_type_index = MANI_GAME_CSS;
	else if (FStrEq(MANI_GAME_DM_STR, game_type) || FStrEq(MANI_GAME_DM1_STR, game_type)) game_type_index = MANI_GAME_DM;
	else if (FStrEq(MANI_GAME_TEAM_DM_STR, game_type)) game_type_index = MANI_GAME_TEAM_DM;
	else if (FStrEq(MANI_GAME_CTF_STR, game_type)) game_type_index = MANI_GAME_CTF;
	else if (FStrEq(MANI_GAME_HIDDEN_STR, game_type)) game_type_index = MANI_GAME_HIDDEN;
	else if (FStrEq(MANI_GAME_GARRYS_MOD_STR, game_type)) game_type_index = MANI_GAME_GARRYS_MOD;
	else if (FStrEq(MANI_GAME_DOD_STR1, game_type)) game_type_index = MANI_GAME_DOD;
	else if (FStrEq(MANI_GAME_DOD_STR2, game_type)) game_type_index = MANI_GAME_DOD;
	else if (FStrEq(MANI_GAME_TF_STR, game_type)) game_type_index = MANI_GAME_TF;
	else game_type_index = MANI_GAME_UNKNOWN;

	DefaultValues();

	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		MMsg("Failed to load gametypes.txt\n");
		kv_ptr->deleteThis();
		return;
	}

	this->show_out_of_date = false;
	int gametypes_version = kv_ptr->GetInt("version", -1);
	if (gametypes_version < gametypes_min_version)
	{
		this->show_out_of_date = true;
		next_time_check = 0;
	}

	KeyValues *base_key_ptr;

	bool found_match;

	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
		MMsg("No true subkey found\n");
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
			MMsg("Found gametypes for %s\n", game_type);
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
				MMsg("Using class unknown mod for defaults\n");
				break;
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				kv_ptr->deleteThis();
				MMsg("Failed to find 'Unknown Mod' entry\n");
				return;
			}
		}
	}

	//base_key_ptr should now hold our core key for our game type defaults
	Q_strcpy(linux_game_bin, base_key_ptr->GetString("linux_bin", "nothing"));
#ifdef __linux__
	Msg("Linux binary @ %s\n", linux_game_bin);
#endif
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
	death_beam_allowed = base_key_ptr->GetInt("death_beam_allowed", 1);
	browse_allowed = base_key_ptr->GetInt("browse_allowed", 1);
	debug_log = base_key_ptr->GetInt("debug_log", 0);
	Q_strcpy(team_manager, base_key_ptr->GetString("team_manager", "sdk_team_"));

	KeyValues *temp_ptr;

	advanced_effects = 0;
	advanced_effects_vfunc_offset = 12;
	advanced_effects_code_offset = 110;

	// Allow advanced effects via te
	temp_ptr = base_key_ptr->FindKey("advanced_effects", false);
	if (temp_ptr)
	{
//		MMsg("Found advanced effects\n");
#ifdef __linux__
        advanced_effects = temp_ptr->GetInt("enable_linux", 0);
#else
		advanced_effects = temp_ptr->GetInt("enable_win", 0);
		advanced_effects_vfunc_offset = temp_ptr->GetInt("vfunc_offset", 12);
		advanced_effects_code_offset = temp_ptr->GetInt("code_offset", 110);
#endif
	}


	// Allow voice control functionality
	temp_ptr = base_key_ptr->FindKey("voice_control", false);
	if (temp_ptr)
	{
//		MMsg("Found voice control\n");

		voice_allowed = 1;
#ifdef __linux__
        voice_offset = temp_ptr->GetInt("linux_offset", 3);
#else
		voice_offset = temp_ptr->GetInt("win_offset", 2);
#endif
	}

	// Allow spray hooking
	temp_ptr = base_key_ptr->FindKey("spray_hook_control", false);
	if (temp_ptr)
	{
//		MMsg("Found spray hook control\n");

		spray_hook_allowed = 1;
#ifdef __linux__
        spray_hook_offset = temp_ptr->GetInt("linux_offset", 28);
#else
		spray_hook_offset = temp_ptr->GetInt("win_offset", 28);
#endif
	}

	// Allow Level Init hooking
	temp_ptr = base_key_ptr->FindKey("spawn_point_control", false);
	if (temp_ptr)
	{
//		MMsg("Found spawn point control\n");

		spawn_point_allowed = 1;
#ifdef __linux__
        spawn_point_offset = temp_ptr->GetInt("linux_offset", 2);
#else
		spawn_point_offset = temp_ptr->GetInt("win_offset", 2);
#endif
	}

	// Get the network offsets
	temp_ptr = base_key_ptr->FindKey("props", false);
	if (temp_ptr)
	{
		this->GetProps(temp_ptr);
	}

	// Get the vfunc offsets
	temp_ptr = base_key_ptr->FindKey("vfuncs", false);
	if (temp_ptr)
	{
		this->GetVFuncs(temp_ptr);
	}

	if (this->IsGameType(MANI_GAME_CSS))
	{
		// Handle weapon costs
		temp_ptr = base_key_ptr->FindKey("weapons", false);
		if (temp_ptr)
		{
			this->GetWeaponDetails(temp_ptr);
		}
	}

	// Handle teams
	temp_ptr = base_key_ptr->FindKey("teams", false);
	if (temp_ptr)
	{
//		MMsg("Found teams\n");
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
				Q_strcpy(temp_team.spawnpoint_class_name,team_ptr->GetString("spawnpoint_class_name", "NULL"));
				Q_strcpy(temp_team.log_name,team_ptr->GetString("log_name", "NULL"));

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
//				MMsg("Found team [%s]\n", temp_team.group);

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

//	MMsg("*********** gametypes.txt loaded ************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Get Property offsets
//---------------------------------------------------------------------------------
#define GETPROPTYPE(_in_string, _prop_index) \
	prop_ptr = kv_ptr->GetString(_in_string, NULL); \
	if (prop_ptr) \
	{ \
		Q_strcpy(prop_index[_prop_index].name, prop_ptr); \
		prop_index[_prop_index].offset = UTIL_FindPropOffset(prop_ptr, type, true); \
		prop_index[_prop_index].type = type; \
	} \
	else \
	{ \
		Q_strcpy(prop_index[_prop_index].name, _in_string); \
		prop_index[_prop_index].type = -1; \
		prop_index[_prop_index].offset = -1; \
	}

void	ManiGameType::GetProps(KeyValues *kv_ptr)
{

	const char	*prop_ptr;
	int	type = 0;

	GETPROPTYPE("health", MANI_PROP_HEALTH)
	GETPROPTYPE("armor", MANI_PROP_ARMOR)
	GETPROPTYPE("render_mode", MANI_PROP_RENDER_MODE)
	GETPROPTYPE("render_fx", MANI_PROP_RENDER_FX)
	GETPROPTYPE("colour", MANI_PROP_COLOUR)
	prop_index[MANI_PROP_COLOUR].type = PROP_COLOUR;
	GETPROPTYPE("account", MANI_PROP_ACCOUNT)
	GETPROPTYPE("move_type", MANI_PROP_MOVE_TYPE)
	GETPROPTYPE("model_index", MANI_PROP_MODEL_INDEX)
	GETPROPTYPE("vec_origin", MANI_PROP_VEC_ORIGIN)
	prop_index[MANI_PROP_VEC_ORIGIN].type = PROP_VECTOR;
	GETPROPTYPE("ang_rotation", MANI_PROP_ANG_ROTATION)
	prop_index[MANI_PROP_ANG_ROTATION].type = PROP_QANGLE;

	prop_index[MANI_PROP_TEAM_NUMBER].offset = UTIL_FindPropOffset("CTeam.m_iTeamNum", type, true);
	prop_index[MANI_PROP_TEAM_NUMBER].type = type;
	Q_strcpy(prop_index[MANI_PROP_TEAM_NUMBER].name, "CTeam.m_iTeamNum");

	prop_index[MANI_PROP_TEAM_SCORE].offset = UTIL_FindPropOffset("CTeam.m_iScore", type, true);
	prop_index[MANI_PROP_TEAM_SCORE].type = type;
	Q_strcpy(prop_index[MANI_PROP_TEAM_SCORE].name, "CTeam.m_iScore");
	prop_index[MANI_PROP_TEAM_NAME].offset = UTIL_FindPropOffset("CTeam.m_szTeamname", type, true);
	prop_index[MANI_PROP_TEAM_NAME].type = type;
	Q_strcpy(prop_index[MANI_PROP_TEAM_NAME].name, "CTeam.m_szTeamname");

/*	for (int i = 0; i < MANI_PROP_SIZE; i++)
	{
		MMsg("Prop [%s] offset [%i] type [%i]\n", prop_index[i].name, prop_index[i].offset, prop_index[i].type);
	}*/

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Get Property offsets
//---------------------------------------------------------------------------------
void	ManiGameType::GetVFuncs(KeyValues *kv_ptr)
{
	vfunc_index[MANI_VFUNC_EYE_ANGLES] = kv_ptr->GetInt("eye_angles", 0x6d);
	vfunc_index[MANI_VFUNC_TELEPORT] = kv_ptr->GetInt("teleport", 0x5d);
	vfunc_index[MANI_VFUNC_SET_MODEL_INDEX] = kv_ptr->GetInt("set_model_index", 0x09);
	vfunc_index[MANI_VFUNC_EYE_POSITION] = kv_ptr->GetInt("eye_position", 0x6c);
	vfunc_index[MANI_VFUNC_MY_COMBAT_CHARACTER] = kv_ptr->GetInt("my_combat_character", 0x3e);
	vfunc_index[MANI_VFUNC_IGNITE] = kv_ptr->GetInt("ignite", 0xad);
	vfunc_index[MANI_VFUNC_REMOVE_PLAYER_ITEM] = kv_ptr->GetInt("remove_player_item", 0xd3);
	vfunc_index[MANI_VFUNC_GET_WEAPON_SLOT] = kv_ptr->GetInt("get_weapon_slot", 0xd1);
	vfunc_index[MANI_VFUNC_GIVE_AMMO] = kv_ptr->GetInt("give_ammo", 0xc6);
	vfunc_index[MANI_VFUNC_WEAPON_DROP] = kv_ptr->GetInt("weapon_drop", 0xcc);
	vfunc_index[MANI_VFUNC_GET_PRIMARY_AMMO_TYPE] = kv_ptr->GetInt("get_primary_ammo_type", 0x10d);
	vfunc_index[MANI_VFUNC_GET_SECONDARY_AMMO_TYPE] = kv_ptr->GetInt("get_secondary_ammo_type", 0x10e);
	vfunc_index[MANI_VFUNC_WEAPON_GET_NAME] = kv_ptr->GetInt("weapon_get_name", 0x107);
	vfunc_index[MANI_VFUNC_GET_VELOCITY] = kv_ptr->GetInt("get_velocity", 0x75);
	vfunc_index[MANI_VFUNC_WEAPON_SWITCH] = kv_ptr->GetInt("weapon_switch", 0xcd);
	vfunc_index[MANI_VFUNC_USER_CMDS] = kv_ptr->GetInt("user_cmds", -1); // 0x322 for windows
	vfunc_index[MANI_VFUNC_GIVE_ITEM] = kv_ptr->GetInt("give_item", -1); // 0x133
	vfunc_index[MANI_VFUNC_MAP] = kv_ptr->GetInt("map_desc", -1); // 0x0d
	vfunc_index[MANI_VFUNC_COMMIT_SUICIDE] = kv_ptr->GetInt("commit_suicide", -1); // 0x0d
	vfunc_index[MANI_VFUNC_SET_OBSERVER_TARGET] = kv_ptr->GetInt("set_observer_target", -1); // 0x0d
	vfunc_index[MANI_VFUNC_WEAPON_CANUSE] = kv_ptr->GetInt("weapon_canuse", -1); // 0x0d
	vfunc_index[MANI_VFUNC_GET_CLASS_NAME] = kv_ptr->GetInt("get_class_name", -1); // 0x03

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Get weapon costs
//---------------------------------------------------------------------------------
#define GETWEAPONCOST(_name, _cost) \
	weapon_cost = kv_ptr->GetInt(#_name, _cost); \
	SetWeaponCost(#_name, weapon_cost) 

void	ManiGameType::GetWeaponDetails(KeyValues *kv_ptr)
{
/* Not used anymore due to WeaponInfo struct
	int weapon_cost;


	GETWEAPONCOST(awp, 4750);
	GETWEAPONCOST(g3sg1, 5000);
	GETWEAPONCOST(sg550, 4200);
	GETWEAPONCOST(galil, 2000);
	GETWEAPONCOST(ak47, 2500);
	GETWEAPONCOST(scout, 2750);
	GETWEAPONCOST(sg552, 3500);
	GETWEAPONCOST(famas, 2250);
	GETWEAPONCOST(m4a1, 3100);
	GETWEAPONCOST(aug, 3500);
	GETWEAPONCOST(m3, 1700);
	GETWEAPONCOST(xm1014, 3000);
	GETWEAPONCOST(mac10, 1400);
	GETWEAPONCOST(tmp, 1250);
	GETWEAPONCOST(mp5navy, 1500);
	GETWEAPONCOST(ump45, 1700);
	GETWEAPONCOST(p90, 2350);
	GETWEAPONCOST(m249, 5750);

	GETWEAPONCOST(glock, 400);
	GETWEAPONCOST(usp, 500);
	GETWEAPONCOST(p228, 600);
	GETWEAPONCOST(deagle, 650);
	GETWEAPONCOST(elite, 800);
	GETWEAPONCOST(fiveseven, 750);
*/
	return;
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
// Purpose: Returns true if passed in string = game type string
//---------------------------------------------------------------------------------
int		ManiGameType::GetVFuncIndex(int	index)
{
#ifdef __linux__
	return (vfunc_index[index]);
#else
	// windows is one less all the time
	return (((vfunc_index[index] == -1) ? -1: vfunc_index[index] - 1));
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if passed in string = game type string
//---------------------------------------------------------------------------------
int		ManiGameType::GetPropIndex(int	index)
{
	return (prop_index[index].offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if passed in string = game type string
//---------------------------------------------------------------------------------
bool	ManiGameType::CanUseProp(int	index)
{
	if (prop_index[index].offset == -1)
	{
		return false;
	}

	return true;
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
	return ((spectator_allowed == 1 && mp_allowspectators && mp_allowspectators->GetInt() == 1) ? true:false);
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
// Purpose: Returns true if spray hook is allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsSprayHookAllowed(void)
{
	return ((spray_hook_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns virtual function index for effects list
//---------------------------------------------------------------------------------
int		ManiGameType::GetSprayHookOffset(void)
{
	return (spray_hook_offset);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if spawn point hook allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsSpawnPointHookAllowed(void)
{
	return ((spawn_point_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns virtual function index for spawn point hook
//---------------------------------------------------------------------------------
int		ManiGameType::GetSpawnPointHookOffset(void)
{
	return (spawn_point_offset);
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
// Purpose: Returns true if death beam allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsDeathBeamAllowed(void)
{
	return ((death_beam_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Returns true if browsing is allowed
//---------------------------------------------------------------------------------
bool		ManiGameType::IsBrowseAllowed(void)
{
	return ((browse_allowed == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Check whether we use CBase suicide or ClientCommmand('kill')
//---------------------------------------------------------------------------------
/*bool		ManiGameType::IsClientSuicide(void)
{
	return ((client_suicide == 0) ? false:true);
}*/

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
// Purpose: Returns the spawnpoint class name for team
//---------------------------------------------------------------------------------
char	*ManiGameType::GetTeamSpawnPointClassName(int index)
{
	if (this->IsValidActiveTeam(index))
	{
		if (!FStrEq(team_class_list[index].spawnpoint_class_name,""))
		{
			return (team_class_list[index].spawnpoint_class_name);
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the log entry name for team
//---------------------------------------------------------------------------------
char	*ManiGameType::GetTeamLogName(int index)
{
	if (this->IsValidActiveTeam(index))
	{
		if (!FStrEq(team_class_list[index].log_name,""))
		{
			return (team_class_list[index].log_name);
		}
	}

	return NULL;
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
	spray_hook_allowed = 0;
	spawn_point_allowed = 0;
//	client_suicide = 1;
	set_colour = 1;
	alpha_render_mode = 1;

	// Dod requires this atm
	slap_allowed = 1;
	drug_allowed = 1;
	teleport_allowed = 1;
	fire_allowed = 1;
	advert_decal_allowed = 1;
	death_beam_allowed = 1;
	browse_allowed = 1;
	debug_log = 0;
	Q_strcpy(team_manager,"cs_team");

	// Default CSS offsets
	vfunc_index[MANI_VFUNC_EYE_ANGLES] = 0x6d;
	vfunc_index[MANI_VFUNC_TELEPORT] = 0x5d;
	vfunc_index[MANI_VFUNC_SET_MODEL_INDEX] = 0x09;
	vfunc_index[MANI_VFUNC_EYE_POSITION] = 0x6c;
	vfunc_index[MANI_VFUNC_MY_COMBAT_CHARACTER] = 0x3e;
	vfunc_index[MANI_VFUNC_IGNITE] = 0xad;
	vfunc_index[MANI_VFUNC_REMOVE_PLAYER_ITEM] = 0xd3;
	vfunc_index[MANI_VFUNC_GET_WEAPON_SLOT] = 0xd1;
	vfunc_index[MANI_VFUNC_GIVE_AMMO] = 0xc6;
	vfunc_index[MANI_VFUNC_WEAPON_DROP] = 0xcc;
	vfunc_index[MANI_VFUNC_GET_PRIMARY_AMMO_TYPE] = 0x10d;
	vfunc_index[MANI_VFUNC_GET_SECONDARY_AMMO_TYPE] = 0x10e;
	vfunc_index[MANI_VFUNC_WEAPON_GET_NAME] = 0x107;
//	vfunc_index[MANI_VFUNC_GET_TEAM_NUMBER] = 0x9c;
//	vfunc_index[MANI_VFUNC_GET_TEAM_NAME] = 0x9d;
	vfunc_index[MANI_VFUNC_GET_VELOCITY] = 0x75;
	vfunc_index[MANI_VFUNC_MAP] = 0x0d;
	vfunc_index[MANI_VFUNC_COMMIT_SUICIDE] = 0x14d;

	for (int i = 0; i < 200; i++)
	{
		prop_index[i].type = -1;
		prop_index[i].offset = -1;
	}

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
//		MMsg("No true subkey found\n");
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

CON_COMMAND(ma_showprops, "Shows current prop types")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	
	for (int i = 0; i < MANI_PROP_SIZE; i ++)
	{
		MMsg("Prop name [%s] ", gpManiGameType->prop_index[i].name);
		switch (gpManiGameType->prop_index[i].type)
		{
		case PROP_INT:MMsg("[int]");break;
		case PROP_UNSIGNED_CHAR:MMsg("[unsigned char]");break;
		case PROP_CHAR_PTR:MMsg("[char *]");break;
		case PROP_CHAR:MMsg("[char]");break;
		case PROP_SHORT:MMsg("[short]");break;
		case PROP_UNSIGNED_SHORT:MMsg("[unsigned short]");break;
		case PROP_BOOL:MMsg("[bool]");break;
		case PROP_UNSIGNED_INT:MMsg("[unsigned int]");break;
		case PROP_FLOAT:MMsg("[float]");break;
		case PROP_UNSIGNED_CHAR_PTR:MMsg("[unsigned char *]");break;
		case PROP_QANGLE:MMsg("[qangle]");break;
		case PROP_VECTOR:MMsg("[vector]");break;
		case PROP_COLOUR:MMsg("[color32]");break;
		default : MMsg("[type unknown]");break;
		}

		MMsg(" Offset [%i]\n", gpManiGameType->prop_index[i].offset);
	}

	return;
}

ManiGameType	g_ManiGameType;
ManiGameType	*gpManiGameType;
