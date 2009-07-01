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
#include "mani_sounds.h"
#include "mani_gametype.h"
#include "mani_weapon.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;

extern  weapon_type_t	*weapon_list;
extern	int				weapon_list_size;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

main_weapon_t	primary_weapon[18];
main_weapon_t	secondary_weapon[6];
weapon_type_t	*weapon_list = NULL;
int				weapon_list_size = 0;
bool			weapons_restricted = false;

static	void	AddWeapon(char *weapon_line);
static	bool	IsWeaponRestricted(char *weapon_name, int team_index);
static	bool	AreWeaponsRestricted(void);
static	int		GetDefaultWeaponIndex(char *weapon_name);
static	bool	GetNextWeapon(char *weapon_name, const char *string, int *index);
static	int		FindWeaponIndex(char *weapon_name);

//---------------------------------------------------------------------------------
// Purpose: Free the weapons used
//---------------------------------------------------------------------------------
void	FreeWeapons(void)
{
	FreeList((void **) &weapon_list, &weapon_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Load weapons information, this must be done before the call to load action models
//---------------------------------------------------------------------------------
void	LoadWeapons(const char *pMapName)
{
	FileHandle_t file_handle;
	char	weapon_line[512];
	char	weapon_name[128];
	char	restrict_filename[256];
	char	base_filename[256];

	Msg("**** LOADING WEAPON INFO ****\n");

	FreeWeapons();

	weapons_restricted = false;

	/* Setup primary and secondary weapons */
	Q_strcpy(primary_weapon[0].name, "awp");
	Q_strcpy(primary_weapon[1].name, "g3sg1");
	Q_strcpy(primary_weapon[2].name, "sg550");
	Q_strcpy(primary_weapon[3].name, "galil");
	Q_strcpy(primary_weapon[4].name, "ak47");
	Q_strcpy(primary_weapon[5].name, "scout");
	Q_strcpy(primary_weapon[6].name, "sg552");
	Q_strcpy(primary_weapon[7].name, "famas");
	Q_strcpy(primary_weapon[8].name, "m4a1");
	Q_strcpy(primary_weapon[9].name, "aug");
	Q_strcpy(primary_weapon[10].name, "m3");
	Q_strcpy(primary_weapon[11].name, "xm1014");
	Q_strcpy(primary_weapon[12].name, "mac10");
	Q_strcpy(primary_weapon[13].name, "tmp");
	Q_strcpy(primary_weapon[14].name, "mp5navy");
	Q_strcpy(primary_weapon[15].name, "ump45");
	Q_strcpy(primary_weapon[16].name, "p90");
	Q_strcpy(primary_weapon[17].name, "m249");

	Q_strcpy(secondary_weapon[0].name, "glock");
	Q_strcpy(secondary_weapon[1].name, "usp");
	Q_strcpy(secondary_weapon[2].name, "p228");
	Q_strcpy(secondary_weapon[3].name, "deagle");
	Q_strcpy(secondary_weapon[4].name, "elite");
	Q_strcpy(secondary_weapon[5].name, "fiveseven");

	for (int i = 0; i < 18; i ++)
	{
		primary_weapon[i].restrict_index = -1;
		Q_snprintf(primary_weapon[i].core_name, sizeof (primary_weapon[i].core_name), "weapon_%s", primary_weapon[i].name);
	}

	for (int i = 0; i < 6; i ++)
	{
		secondary_weapon[i].restrict_index = -1;
		Q_snprintf(secondary_weapon[i].core_name, sizeof (secondary_weapon[i].core_name), "weapon_%s", secondary_weapon[i].name);
	}

	//Get weapons available for restriction
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/restricted_weapons.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg("No default weapons restrictions found, weapon restrict menu will be empty\n");
	}
	else
	{
		Msg("Weapon Restrictions\n");
		while (filesystem->ReadLine (weapon_line, sizeof(weapon_line), file_handle) != NULL)
		{
			if (!ParseLine(weapon_line, true))
			{
				// String is empty after parsing
				continue;
			}

			AddWeapon(weapon_line);
		}

		filesystem->Close(file_handle);

		// Bind indexes so that we can handle rebuy restrictions
		for (int i = 0; i < 18; i ++)
		{
			int index = GetDefaultWeaponIndex(primary_weapon[i].name);

			if (index == -1)
			{
				Msg("Failed to bind primary weapon [%s]\n", primary_weapon[i].name);
				continue;
			}

			primary_weapon[i].restrict_index = index;
		}

		// Bind indexes so that we can handle rebuy restrictions
		for (int i = 0; i < 6; i ++)
		{
			int index = GetDefaultWeaponIndex(secondary_weapon[i].name);

			if (index == -1)
			{
				Msg("Failed to bind secondary weapon [%s]\n", secondary_weapon[i].name);
				continue;
			}

			secondary_weapon[i].restrict_index = index;
		}
	}

	//Get weapons restriction list for this map
	Q_snprintf( restrict_filename, sizeof(restrict_filename), "./cfg/%s/restrict/%s_restrict.txt", mani_path.GetString(), pMapName);
	file_handle = filesystem->Open (restrict_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg("No default weapons restrictions found for this map\n");
	}
	else
	{
		Msg("Weapon Restriction List for map\n");
		while (filesystem->ReadLine (weapon_name, sizeof(weapon_name), file_handle) != NULL)
		{
			if (!ParseLine(weapon_name, true))
			{
				// String is empty after parsing
				continue;
			}

			for (int i = 0; i < weapon_list_size; i ++)
			{
				if (FStrEq(weapon_list[i].weapon_name, weapon_name))
				{
					weapons_restricted = true;
					weapon_list[i].restricted = true;
					weapon_list[i].limit_per_team = 0;
					weapon_list[i].ct_count = 0;
					weapon_list[i].t_count = 0;
					break;
				}
			}
			Msg("[%s] is restricted\n", weapon_name);
		}

		filesystem->Close(file_handle);
	}

	//Get weapons restriction list for this map
	Q_snprintf( restrict_filename, sizeof(restrict_filename), "./cfg/%s/default_weapon_restrict.txt", mani_path.GetString());
	file_handle = filesystem->Open (restrict_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg("No default weapons restrictions found.\n");
	}
	else
	{
		Msg("Weapon Restriction List for all maps\n");
		while (filesystem->ReadLine (weapon_name, sizeof(weapon_name), file_handle) != NULL)
		{
			if (!ParseLine(weapon_name, true))
			{
				// String is empty after parsing
				continue;
			}

			for (int i = 0; i < weapon_list_size; i ++)
			{
				if (FStrEq(weapon_list[i].weapon_name, weapon_name))
				{
					weapons_restricted = true;
					weapon_list[i].restricted = true;
					weapon_list[i].limit_per_team = 0;
					weapon_list[i].ct_count = 0;
					weapon_list[i].t_count = 0;
					break;
				}
			}
			Msg("[%s] is restricted\n", weapon_name);
		}

		filesystem->Close(file_handle);
	}


	Msg("**** WEAPON INFO LOADED ****\n");

}


//---------------------------------------------------------------------------------
// Purpose: Parses the weapon config line and extracts aliases and weapon name
//---------------------------------------------------------------------------------

static
void AddWeapon(char *weapon_line)
{
	int		i,j;
	weapon_type_t	weapon;
	char	weapon_string[128];

	if (!AddToList((void **) &weapon_list, sizeof(weapon_type_t), &weapon_list_size))
	{
		return;
	}

	// default weapons details
	weapon.restricted = false; 
	weapon.number_of_weapons = 0; 
	weapon.ct_count = 0;
	weapon.t_count = 0;
	weapon.limit_per_team = 0;


	i = 0;
	j = 0;
	bool found_break = false;

	for (;;)
	{
		if (weapon_line[i] == '\0')
		{
			// No more data
			weapon_string[j] = '\0';
			Q_strcpy(weapon.weapon_name, weapon_string);
			if (weapon.number_of_weapons == 0)
			{
				Q_strcpy(weapon.weapon_alias[0], weapon_string);
				weapon.number_of_weapons = 1;
			}

			weapon_list[weapon_list_size - 1] = weapon;
			Msg ("Weapon Name [%s] Weapon Aliases ", weapon.weapon_name);
			for (i = 0; i < weapon.number_of_weapons; i++)
			{
				Msg ("[%s] ", weapon.weapon_alias[i]);
			}
			Msg ("\n");
			return;
		}


		// If reached space or tab break out of loop
		if (weapon_line[i] == ' ' ||
			weapon_line[i] == '\t')
		{
			if (found_break)
			{
				i++;
				continue;
			}
			else
			{
				weapon_string[j] = '\0';
				Q_strcpy(weapon.weapon_alias[weapon.number_of_weapons], weapon_string);
				weapon.number_of_weapons ++;
				found_break = true;
				j = 0;
				i++;
			}
		}
		else
		{
			weapon_string[j++] = weapon_line[i++];
			found_break = false;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check if weapon restricted for this weapon name
//---------------------------------------------------------------------------------
static
bool IsWeaponRestricted(char *weapon_name, int team_index)
{
	if (war_mode) return false;

	// Search for weapon restriction
	for (int count = 0; count < weapon_list_size; count ++)
	{
		for (int j = 0; j < weapon_list[count].number_of_weapons; j++)
		{
			if (FStrEq( weapon_name, weapon_list[count].weapon_alias[j]))
			{
				if (weapon_list[count].restricted == true)
				{
					if (weapon_list[count].limit_per_team == 0)
					{
						return true;
					}

					if (team_index == TEAM_B)
					{
						if (weapon_list[count].ct_count >= weapon_list[count].limit_per_team)
						{
							return true;
						}
						else
						{
							weapon_list[count].ct_count ++;
							return false;
						}
					}
					else if (team_index == TEAM_A)
					{
						if (weapon_list[count].t_count >= weapon_list[count].limit_per_team)
						{
							return true;
						}
						else
						{
							weapon_list[count].t_count ++;
							return false;
						}
					}
				}
			}
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any weapon restrictions are in place
//---------------------------------------------------------------------------------
static
bool	AreWeaponsRestricted(void)
{
	for( int i = 0; i < weapon_list_size; i++ )
	{
		if (weapon_list[i].restricted)
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check index in weapon_list for weapon name
//---------------------------------------------------------------------------------
static
int GetDefaultWeaponIndex(char *weapon_name)
{
	// Search for weapon restriction
	for (int count = 0; count < weapon_list_size; count ++)
	{
		for (int j = 0; j < weapon_list[count].number_of_weapons; j++)
		{
			if (FStrEq( weapon_name, weapon_list[count].weapon_alias[j]))
			{
				return count;
			}
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Check client command for buyammo shortcut
//---------------------------------------------------------------------------------
PLUGIN_RESULT ProcessClientBuy
(
 const char *pcmd, 
 const char *pcmd2, 
 const int pargc,
 player_t *player_ptr
)
{
	if (!FindPlayerByEntity(player_ptr)) return PLUGIN_CONTINUE;

	if (war_mode) return PLUGIN_CONTINUE;
	if (!weapons_restricted) return PLUGIN_CONTINUE;

	if ( FStrEq( pcmd, "buy"))
	{
		if (pargc > 1)
		{
			if (IsWeaponRestricted((char *) pcmd2, player_ptr->team))
			{
				ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
				return PLUGIN_STOP;
			}
		}
	}
	else if (FStrEq(pcmd, "buyammo1"))
	{
		if (IsWeaponRestricted("primammo", player_ptr->team))
		{
			ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
			return PLUGIN_STOP;
		}
	}
	else if ( FStrEq(pcmd, "buyammo2"))
	{
		if (IsWeaponRestricted("secammo", player_ptr->team))
		{
			ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;

}

//---------------------------------------------------------------------------------
// Purpose: Handle the restrict weapon menu
//---------------------------------------------------------------------------------
void ProcessRestrictWeapon( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	weapon_name[128];
		int		weapon_index;

		// check if valid weapon name 
		Q_strcpy(weapon_name, engine->Cmd_Argv(2 + argv_offset));
		weapon_index = -1;
		for (int i = 0; i < weapon_list_size; i++)
		{
			if (FStrEq(weapon_name, weapon_list[i].weapon_name))
			{
				weapon_index = i;
				break;
			}
		}

		if (weapon_index == -1)	return;

		if (weapon_list[weapon_index].restricted == true)
		{
			char	log_text[256];

			weapon_list[weapon_index].restricted = false;

			Q_snprintf( log_text, sizeof(log_text),	"un-restrict [%s]\n", weapon_list[weapon_index].weapon_name);
			LogCommand (admin->entity, "%s", log_text);
			SayToAll(true, Translate(M_RESTRICT_MENU_UNRESTRICT), weapon_list[weapon_index].weapon_name);
		}
		else
		{
			char	log_text[256];

			weapon_list[weapon_index].restricted = true;
			Q_snprintf( log_text, sizeof(log_text),	"restrict [%s]\n", weapon_list[weapon_index].weapon_name);
			LogCommand (admin->entity, "%s", log_text);
			SayToAll(true, Translate(M_RESTRICT_MENU_RESTRICT), weapon_list[weapon_index].weapon_name);
		}

		weapons_restricted = AreWeaponsRestricted();
	}
	else
	{
		if (weapon_list_size == 0)
		{
			return;
		}

		// Setup weapon list
		FreeMenu();
		
		CreateList ((void **) &menu_list, sizeof(menu_t), weapon_list_size, &menu_list_size);

		for( int i = 0; i < weapon_list_size; i++ )
		{
			Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s%s", (weapon_list[i].restricted) ? "* ":"", weapon_list[i].weapon_name);							
			Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin restrict_weapon %s", weapon_list[i].weapon_name);
			Q_strcpy(menu_list[i].sort_name, weapon_list[i].weapon_name);
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, Translate(M_RESTRICT_MENU_ESCAPE), Translate(M_RESTRICT_MENU_TITLE), next_index, "admin", "restrict_weapon", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Hook the autobuy command
//---------------------------------------------------------------------------------
bool HookAutobuyCommand(void)
{
	player_t player;
	char	*autobuy_string;

	if (war_mode || !weapons_restricted)
	{
		return true;
	}

//	if (IsCommandIssuedByServerAdmin()) return true;

	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player))
	{
		Msg("Did not find player\n");
		return true;
	}

	if (player.is_bot)
	{
		return true;
	}

	// Get what client is using.
	autobuy_string = (char *)engine->GetClientConVarValue(con_command_index + 1, "cl_autobuy");
	
	char	autobuy_command[512];
	char	weapon_name[512];
	bool	weapon_restricted = false;
	int		index = 0;
	
	// cl_setautobuy or cl_setrebuy has weapons in it
	// Go through weapons list to see if they are restricted

	Q_strcpy(autobuy_command, "cl_autobuy ");
	while (true == GetNextWeapon(weapon_name, autobuy_string, &index))
	{
		if (!IsWeaponRestricted(weapon_name, player.team))
		{
			// Useable weapon
			Q_strcat(autobuy_command, weapon_name);
			Q_strcat(autobuy_command, " ");
		}
		else
		{
			weapon_restricted = true;
		}
	}

	if (weapon_restricted)
	{
		// Re-issue autobuy command command
		autobuy_command[Q_strlen(autobuy_command) - 1] = '\n';
		engine->ClientCommand(player.entity, autobuy_command);
		engine->ClientCommand(player.entity, "autobuy\n");
		Q_snprintf(autobuy_command, sizeof(autobuy_command), "cl_autobuy %s\n", autobuy_string);
		engine->ClientCommand(player.entity, autobuy_command);
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Hook the rebuy command
//---------------------------------------------------------------------------------
bool HookRebuyCommand(void)
{
	player_t player;
	char	*rebuy_string;

	if (war_mode || !weapons_restricted)
	{
		return true;
	}

//	if (IsCommandIssuedByServerAdmin()) return true;

	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player))
	{
		Msg("Did not find player\n");
		return true;
	}

	if (player.is_bot)
	{
		return true;
	}

	// Get what client weapon classes client is using.
	rebuy_string = (char *)engine->GetClientConVarValue(con_command_index + 1, "cl_rebuy");
	
	char	rebuy_command[512];
	char	weapon_name[512];
	bool	weapon_restricted = false;
	int		index = 0;
	
	// Go through weapons list to see if they are restricted

	Q_strcpy(rebuy_command, "cl_rebuy ");
	while (true == GetNextWeapon(weapon_name, rebuy_string, &index))
	{
		if (FStrEq("NightVision", weapon_name))
		{
			if (!IsWeaponRestricted("nvgs", player.team))
			{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
			}
			else
			{
				weapon_restricted = true;
			}
		}
		else if (FStrEq("Armor", weapon_name))
		{
			if (IsWeaponRestricted("vest", player.team) || IsWeaponRestricted("vesthelm", player.team))
			{
				weapon_restricted = true;
			}
			else
			{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
			}
		}
		else if (FStrEq("PrimaryAmmo", weapon_name))
		{
			if (IsWeaponRestricted("primammo", player.team))
			{
				weapon_restricted = true;
			}
			else
			{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
			}
		}
		else if (FStrEq("SecondaryAmmo", weapon_name))
		{
			if (IsWeaponRestricted("secammo", player.team))
			{
				weapon_restricted = true;
			}
			else
			{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
			}
		}
		else if (FStrEq("PrimaryWeapon", weapon_name) ||
				FStrEq("PrimaryAmmo", weapon_name) ||
				FStrEq("SecondaryWeapon", weapon_name) ||
				FStrEq("SecondaryAmmo", weapon_name))
		{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
		}
		else
		{
			// Handle flash, he, smoke
			if (!IsWeaponRestricted(weapon_name, player.team))
			{
				// Useable weapon
				Q_strcat(rebuy_command, weapon_name);
				Q_strcat(rebuy_command, " ");
			}
			else
			{
				weapon_restricted = true;
			}
		}
	}

	if (weapon_restricted)
	{
		// Re-issue rebuy command command
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
		rebuy_command[Q_strlen(rebuy_command) - 1] = '\n';
		engine->ClientCommand(player.entity, rebuy_command);
		engine->ClientCommand(player.entity, "rebuy\n");
		Q_snprintf(rebuy_command, sizeof(rebuy_command), "cl_rebuy %s\n", rebuy_string);
		engine->ClientCommand(player.entity, rebuy_command);
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Strip primary and secondary weapons if necessary
//---------------------------------------------------------------------------------
void PostProcessRebuyCommand(void)
{
	player_t player;
	char	*rebuy_string;

	if (war_mode || !weapons_restricted)
	{
		return;
	}

	if (IsCommandIssuedByServerAdmin()) return;

	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player))
	{
		Msg("Did not find player\n");
		return;
	}

	if (player.is_bot)
	{
		return;
	}

	rebuy_string = (char *)engine->GetClientConVarValue(con_command_index + 1, "cl_rebuy");
	CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();
	CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();

	if (NULL != Q_strstr(rebuy_string, "PrimaryWeapon"))
	{
		// Primary Weapon requested so delete it :)

		CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(0);
		if (pWeapon)
		{
			const char *core_weapon_name = pWeapon->GetName();

			for (int i = 0; i < 18; i ++)
			{
				if (FStrEq(core_weapon_name, primary_weapon[i].core_name))
				{
					if (weapon_list[primary_weapon[i].restrict_index].restricted == true)
					{
						int	limit_per_team = weapon_list[primary_weapon[i].restrict_index].limit_per_team;
						// Found player with weapon
						if (limit_per_team == 0)
						{
							CBasePlayer *pBase = (CBasePlayer*) pPlayer;
							pBase->RemovePlayerItem(pWeapon);
							ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
							SayToPlayer(&player,"Weapon %s has been removed, do not buy it when restricted !!", primary_weapon[i].name);
						}
						else
						{
							if (player.team == TEAM_B)
							{
								if (weapon_list[primary_weapon[i].restrict_index].ct_count >= limit_per_team)
								{
									CBasePlayer *pBase = (CBasePlayer*) pPlayer;
									pBase->RemovePlayerItem(pWeapon);
									ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
									SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", primary_weapon[i].name, limit_per_team);
								}
								else
								{
									weapon_list[primary_weapon[i].restrict_index].ct_count ++;
								}
							}
							else if (player.team == TEAM_A)
							{
								if (weapon_list[primary_weapon[i].restrict_index].t_count >= limit_per_team)
								{
									CBasePlayer *pBase = (CBasePlayer*) pPlayer;
									pBase->RemovePlayerItem(pWeapon);
									ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
									SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", primary_weapon[i].name, limit_per_team);
								}
								else
								{
									weapon_list[primary_weapon[i].restrict_index].t_count ++;
								}
							}
						}
					}
				}
			}
		}		
	}

	if (NULL != Q_strstr(rebuy_string, "SecondaryWeapon"))
	{
		// Primary Weapon requested so delete it :)

		CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(1);
		if (pWeapon)
		{
			const char *core_weapon_name = pWeapon->GetName();

			for (int i = 0; i < 6; i ++)
			{
				if (FStrEq(core_weapon_name, secondary_weapon[i].core_name))
				{
					if (weapon_list[secondary_weapon[i].restrict_index].restricted == true)
					{
						int	limit_per_team = weapon_list[secondary_weapon[i].restrict_index].limit_per_team;
						// Found player with weapon
						if (limit_per_team == 0)
						{
							CBasePlayer *pBase = (CBasePlayer*) pPlayer;
							pBase->RemovePlayerItem(pWeapon);
							ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
							SayToPlayer(&player,"Weapon %s has been removed, do not buy it when restricted !!", secondary_weapon[i].name);
						}
						else
						{
							if (player.team == TEAM_B)
							{
								if (weapon_list[secondary_weapon[i].restrict_index].ct_count >= limit_per_team)
								{
									CBasePlayer *pBase = (CBasePlayer*) pPlayer;
									pBase->RemovePlayerItem(pWeapon);
									ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
									SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", secondary_weapon[i].name, limit_per_team);
								}
								else
								{
									weapon_list[secondary_weapon[i].restrict_index].ct_count ++;
								}
							}
							else if (player.team == TEAM_A)
							{
								if (weapon_list[secondary_weapon[i].restrict_index].t_count >= limit_per_team)
								{
									CBasePlayer *pBase = (CBasePlayer*) pPlayer;
									pBase->RemovePlayerItem(pWeapon);
									ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
									SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", secondary_weapon[i].name, limit_per_team);
								}
								else
								{
									weapon_list[secondary_weapon[i].restrict_index].t_count ++;
								}
							}
						}
					}
				}
			}
		}		
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Strip primary and secondary weapons if necessary
//---------------------------------------------------------------------------------
void RemoveRestrictedWeapons(void)
{
	player_t player;

	if (war_mode || !weapons_restricted)
	{
		return;
	}

	// Reset all weapons count
	for (int i = 0; i < weapon_list_size; i ++)
	{
		weapon_list[i].ct_count = 0;
		weapon_list[i].t_count = 0;
	}

	for (int i = 0; i < 18; i ++)
	{
		if (weapon_list[primary_weapon[i].restrict_index].restricted == true)
		{
			int	limit_per_team = weapon_list[primary_weapon[i].restrict_index].limit_per_team;

			for (int j = 1; j <= max_players; j ++)
			{
				player.index = j;
				if (!FindPlayerByIndex(&player)) continue;
				if (player.is_bot) continue;

				CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();
				CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
				if (!pCombat) continue;
				CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(0);
				if (!pWeapon) continue;

				const char *core_weapon_name = pWeapon->GetName();

				if (FStrEq(core_weapon_name, primary_weapon[i].core_name))
				{
					// Found player with weapon
					if (limit_per_team == 0)
					{
						CBasePlayer *pBase = (CBasePlayer*) pPlayer;
						pBase->RemovePlayerItem(pWeapon);
						SayToPlayer(&player,"Weapon %s has been removed, do not buy it when restricted !!", primary_weapon[i].name);
					}
					else
					{
						if (player.team == TEAM_B)
						{
							if (weapon_list[primary_weapon[i].restrict_index].ct_count >= limit_per_team)
							{
								CBasePlayer *pBase = (CBasePlayer*) pPlayer;
								pBase->RemovePlayerItem(pWeapon);
								SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", primary_weapon[i].name, limit_per_team);
							}
							else
							{
								weapon_list[primary_weapon[i].restrict_index].ct_count ++;
							}
						}
						else if (player.team == TEAM_A)
						{
							if (weapon_list[primary_weapon[i].restrict_index].t_count >= limit_per_team)
							{
								CBasePlayer *pBase = (CBasePlayer*) pPlayer;
								pBase->RemovePlayerItem(pWeapon);
								SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", primary_weapon[i].name, limit_per_team);
							}
							else
							{
								weapon_list[primary_weapon[i].restrict_index].t_count ++;
							}
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < 6; i ++)
	{
		if (weapon_list[secondary_weapon[i].restrict_index].restricted == true)
		{
			int	limit_per_team = weapon_list[secondary_weapon[i].restrict_index].limit_per_team;

			for (int j = 1; j <= max_players; j ++)
			{
				player.index = j;
				if (!FindPlayerByIndex(&player)) continue;
				if (player.is_bot) continue;

				CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();
				CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
				if (!pCombat) continue;
				CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(1);
				if (!pWeapon) continue;

				const char *core_weapon_name = pWeapon->GetName();

				if (FStrEq(core_weapon_name, secondary_weapon[i].core_name))
				{
					// Found player with weapon
					if (limit_per_team == 0)
					{
						CBasePlayer *pBase = (CBasePlayer*) pPlayer;
						pBase->RemovePlayerItem(pWeapon);
						SayToPlayer(&player,"Weapon %s has been removed, do not buy it when restricted !!", secondary_weapon[i].name);
					}
					else
					{
						if (player.team == TEAM_B)
						{
							if (weapon_list[secondary_weapon[i].restrict_index].ct_count >= limit_per_team)
							{
								CBasePlayer *pBase = (CBasePlayer*) pPlayer;
								pBase->RemovePlayerItem(pWeapon);
								SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", secondary_weapon[i].name, limit_per_team);
							}
							else
							{
								weapon_list[secondary_weapon[i].restrict_index].ct_count ++;
							}
						}
						else if (player.team == TEAM_A)
						{
							if (weapon_list[secondary_weapon[i].restrict_index].t_count >= limit_per_team)
							{
								CBasePlayer *pBase = (CBasePlayer*) pPlayer;
								pBase->RemovePlayerItem(pWeapon);
								SayToPlayer(&player,"Weapon %s has been removed, only %i allowed per team !!", secondary_weapon[i].name, limit_per_team);
							}
							else
							{
								weapon_list[secondary_weapon[i].restrict_index].t_count ++;
							}
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Extract weapon names from string
//---------------------------------------------------------------------------------
static
bool GetNextWeapon(char *weapon_name, const char *string, int *index)
{
	int	name_index = 0;
	bool	string_found = false;

	for (;;)
	{
		if (string[*index] != ' ' && string[*index] != '\0')
		{
			weapon_name[name_index] = string[*index];
			string_found = true;
			name_index ++;
			*index = *index + 1;
			continue;
		}

		if (string[*index] == ' ')
		{
			weapon_name[name_index] = '\0';
			*index = *index + 1;

			// traverse remaining spaces
			while (string[*index] == ' ')
			{
				*index = *index + 1;
			}

			return true;
		}

		if (string[*index] == '\0')
		{
			if (string_found)
			{
				weapon_name[name_index] = '\0';
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_showrestrict
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaShowRestrict
(
 int index, 
 bool svr_command
)
{
	player_t player;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player))
		{
			return PLUGIN_STOP;
		}
	}

	OutputToConsole(player.entity, svr_command, "Current weapons and their restrictions\n\n");
	OutputToConsole(player.entity, svr_command, "Weapon Alias         Restricted\n");
	OutputToConsole(player.entity, svr_command, "-------------------------------\n");

	for (int i = 0; i < weapon_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "%-20s %s\n", weapon_list[i].weapon_name, (weapon_list[i].restricted)? "YES":"NO");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_restrict command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaRestrictWeapon
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *weapon_name,
 char *limit_per_team,
 bool restrict
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
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (restrict)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <weapon name> <optional number per team>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <weapon name> <optional number per team>", command_string);
			}
		}
		else
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <weapon name>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <weapon name>", command_string);
			}
		}

		return PLUGIN_STOP;
	}	

	int	limit_weapon = 0;

	if (argc == 3)
	{
		limit_weapon = Q_atoi(limit_per_team);
	}

	// check if valid weapon name 
	int weapon_index = -1;

	for (int i = 0; i < weapon_list_size; i++)
	{
		if (FStrEq(weapon_name, weapon_list[i].weapon_name))
		{
			weapon_index = i;
			break;
		}
	}

	if (weapon_index == -1)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Weapon [%s] not found\n", weapon_name);
		}
		else
		{
			SayToPlayer(&player, "Weapon [%s] not found", weapon_name);
		}

		return PLUGIN_STOP;
	}

	if (!restrict)
	{
		// Un restrict weapon
		char	log_text[256];

		weapon_list[weapon_index].restricted = false;
		weapon_list[weapon_index].limit_per_team = 0;

		Q_snprintf( log_text, sizeof(log_text),	"un-restrict [%s]\n", weapon_list[weapon_index].weapon_name);
		LogCommand (player.entity, "%s", log_text);
		SayToAll(true, "Weapon [%s] is un-restricted", weapon_list[weapon_index].weapon_name);
	}
	else
	{
		char	log_text[256];

		// Restrict weapon

		weapon_list[weapon_index].restricted = true;
		weapon_list[weapon_index].limit_per_team = limit_weapon;
		Q_snprintf( log_text, sizeof(log_text),	"restrict [%s]\n", weapon_list[weapon_index].weapon_name);
		LogCommand (player.entity, "%s", log_text);
		if (limit_weapon == 0)
		{
			SayToAll(true, "Weapon [%s] is restricted", weapon_list[weapon_index].weapon_name);
		}
		else
		{
			SayToAll(true, "Weapon [%s] is restricted to %i per team", weapon_list[weapon_index].weapon_name, limit_weapon);
		}
	}

	if (restrict)
	{
		weapons_restricted = true;
	}
	else
	{
		weapons_restricted = AreWeaponsRestricted();
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_knives
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaKnives
(
 int index, 
 bool svr_command, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	for (int i = 0; i < weapon_list_size; i++)
	{
		weapon_list[i].restricted = true;
	}

	int	weapon_index;

	if (-1 != (weapon_index = FindWeaponIndex("vest"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("vesthelm"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("defuser"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("nvgs"))) weapon_list[weapon_index].restricted = false; 

	LogCommand (player.entity, "Only knives can be used next round !!!\n");
	SayToAll(true, "Only knives can be used next round !!!");

	weapons_restricted = true;

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_pistols
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaPistols
(
 int index, 
 bool svr_command, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	for (int i = 0; i < weapon_list_size; i++)
	{
		weapon_list[i].restricted = true;
	}

	int	weapon_index;

	if (-1 != (weapon_index = FindWeaponIndex("glock"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("usp"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("p228"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("deagle"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("elite"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("fiveseven"))) weapon_list[weapon_index].restricted = false; 

	if (-1 != (weapon_index = FindWeaponIndex("vest"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("primammo"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("secammo"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("vest"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("vesthelm"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("defuser"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("nvgs"))) weapon_list[weapon_index].restricted = false; 

	LogCommand (player.entity, "Only pistols can be used next round !!!\n");
	SayToAll(true, "Only pistols can be used next round !!!");

	weapons_restricted = true;

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_shotguns
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaShotguns
(
 int index, 
 bool svr_command, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	for (int i = 0; i < weapon_list_size; i++)
	{
		weapon_list[i].restricted = true;
	}

	int	weapon_index;

	if (-1 != (weapon_index = FindWeaponIndex("m3"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("xm1014"))) weapon_list[weapon_index].restricted = false;

	if (-1 != (weapon_index = FindWeaponIndex("primammo"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("secammo"))) weapon_list[weapon_index].restricted = false; 

	if (-1 != (weapon_index = FindWeaponIndex("vest"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("vesthelm"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("defuser"))) weapon_list[weapon_index].restricted = false; 
	if (-1 != (weapon_index = FindWeaponIndex("nvgs"))) weapon_list[weapon_index].restricted = false; 

	LogCommand (player.entity, "Only shotguns can be used next round !!!\n");
	SayToAll(true, "Only shotguns can be used next round !!!");

	weapons_restricted = true;

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_nosnipers
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaNoSnipers
(
 int index, 
 bool svr_command, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	for (int i = 0; i < weapon_list_size; i++)
	{
		weapon_list[i].restricted = false;
	}

	int	weapon_index;

	if (-1 != (weapon_index = FindWeaponIndex("awp"))) weapon_list[weapon_index].restricted = true; 
	if (-1 != (weapon_index = FindWeaponIndex("g3sg1"))) weapon_list[weapon_index].restricted = true;
	if (-1 != (weapon_index = FindWeaponIndex("sg550"))) weapon_list[weapon_index].restricted = true;
	if (-1 != (weapon_index = FindWeaponIndex("scout"))) weapon_list[weapon_index].restricted = true;

	LogCommand (player.entity, "No sniper weapons next round !!!\n");
	SayToAll(true, "No snipers weapons next round !!!");

	weapons_restricted = true;

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Find the index of a weapon name
//---------------------------------------------------------------------------------
int	FindWeaponIndex(char *weapon_name)
{
	for (int i = 0; i < weapon_list_size; i++)
	{
		for (int j = 0; j < weapon_list[i].number_of_weapons; j ++)
		{
			if (FStrEq(weapon_name, weapon_list[i].weapon_alias[j]))
			{
				return i;
			}
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unrestrictall command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaUnRestrictAll
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

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!gpManiClient->IsAdminAllowed(&player, command_string, ALLOW_RESTRICTWEAPON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	// UnRestrict all the weapons
	for (int i = 0; i < weapon_list_size; i++)
	{
		weapon_list[i].restricted = false;
		weapon_list[i].ct_count = 0;
		weapon_list[i].t_count = 0;
		weapon_list[i].limit_per_team = 0;
	}

	LogCommand (player.entity, "unrestricted all weapons\n");
	SayToAll(true, "All weapons are unrestricted");

	weapons_restricted = true;

	return PLUGIN_STOP;
}


CON_COMMAND(ma_showrestrict, "Prints all weapons and their restricted status")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaShowRestrict(0, true);
	return;
}

CON_COMMAND(ma_restrict, "Restricts a weapon, ma_restrict <weapon_name>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaRestrictWeapon
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // weapon name
					engine->Cmd_Argv(2),  // Number per team
					true
					);
	return;
}

CON_COMMAND(ma_knives, "Restricts weapons to knives only")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaKnives
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argv(0) // Command
					);
	return;
}

CON_COMMAND(ma_pistols, "Restricts weapons to pistols only")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaPistols
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argv(0) // Command
					);
	return;
}

CON_COMMAND(ma_shotguns, "Restricts weapons to shotguns only")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaShotguns
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argv(0) // Command
					);
	return;
}

CON_COMMAND(ma_nosnipers, "Restricts weapons to remove sniper rifles")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaNoSnipers
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argv(0) // Command
					);
	return;
}

CON_COMMAND(ma_unrestrict, "Un-restricts a weapon, ma_unrestrict <weapon_name>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaRestrictWeapon
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // weapon name
					"0",
					false
					);
	return;
}

CON_COMMAND(ma_unrestrictall, "Un-restricts all weapons")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaUnRestrictAll
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0) // Command
					);
	return;
}
