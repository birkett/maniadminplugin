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
#include "mani_admin_flags.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_parser.h"
#include "mani_admin.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	int		con_command_index;
extern	bool	war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static void InvertAdminFlags(admin_t *admin);
static void	InitAdminFlags(void);


admin_t			*admin_list=NULL;
admin_group_t	*admin_group_list=NULL;
int				admin_list_size=0;
int				admin_group_list_size=0;
admin_flag_t	admin_flag_list[MAX_ADMIN_FLAGS];


//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
bool IsClientAdmin( player_t *player, int *admin_index)
{

	*admin_index = -1;

	if (player->is_bot)
	{
		return false;
	}

	const char *password  = engine->GetClientConVarValue(player->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i < admin_list_size; i ++)
	{
		// Check Steam ID
		if (admin_list[i].steam_id)
		{
			if (FStrEq(admin_list[i].steam_id, player->steam_id))
			{
				*admin_index = i;
				return true;
			}
		}

		// Check IP address
		if (admin_list[i].ip_address && player->ip_address)
		{
			if (FStrEq(admin_list[i].ip_address, player->ip_address)) 
			{
				*admin_index = i;
				return true;
			}
		}

		// Check name password combo
		if (admin_list[i].name && admin_list[i].password)
		{
			if (strcmp(admin_list[i].name, player->name) == 0)
			{
				if (strcmp(admin_list[i].password, password) == 0)
				{
					*admin_index = i;
					return true;
				}
			}
		}	
	}
	
	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin config line setting flags
//---------------------------------------------------------------------------------

void AddAdmin(char *admin_details)
{
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	char	password[128];
	int	i,j;
	admin_t	admin;

	if (!AddToList((void **) &admin_list, sizeof(admin_t), &admin_list_size))
	{
		return;
	}

	// default admin to have absolute power
	for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
	{
		admin.flags[k] = true;
	}

	Q_strcpy(admin.steam_id, "");
	Q_strcpy(steam_id, "");
	Q_strcpy(admin.ip_address, "");
	Q_strcpy(ip_address, "");
	Q_strcpy(name, "");
	Q_strcpy(admin.name, "");
	Q_strcpy(password, "");
	Q_strcpy(admin.password, "");
	Q_strcpy(admin.group_id, "");

	i = 0;

	if (admin_details[i] != ';')
	{
		j = 0;
		for (;;)
		{
			if (admin_details[i] == '\0')
			{
				// No more data
				steam_id[j] = '\0';
				Q_strcpy(admin.steam_id, steam_id);
				InvertAdminFlags(&admin);
				admin_list[admin_list_size - 1] = admin;
				return;
			}


			// If reached space or tab break out of loop
			if (admin_details[i] == ' ' ||
				admin_details[i] == '\t' ||
				admin_details[i] == ';')
			{
				steam_id[j] = '\0';
				break;
			}

			steam_id[j] = admin_details[i];
			i++;
			j++;
		}

		Q_strcpy(admin.steam_id, steam_id);
	}

	Msg("%s ", steam_id);

	if (admin_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (admin_details[i] == '\0')
			{
				// No more data
				ip_address[j] = '\0';
				Q_strcpy(admin.ip_address, ip_address);
				InvertAdminFlags(&admin);
				admin_list[admin_list_size - 1] = admin;
				return;
			}

			// If reached space or tab break out of loop
			if (admin_details[i] == ' ' ||
				admin_details[i] == ';' ||
				admin_details[i] == '\t')
			{
				ip_address[j] = '\0';
				break;
			}

			ip_address[j] = admin_details[i];
			i++;
			j++;
		}

		Q_strcpy(admin.ip_address, ip_address);
	}

	Msg("%s ", ip_address);
	
	if (admin_details[i] == ';' && admin_details[i + 1] == '\"')
	{
		i += 2;
		j = 0;

		for (;;)
		{
			if (admin_details[i] == '\0')
			{
				// No more data
				name[j] = '\0';
				Q_strcpy(admin.password, name);
				InvertAdminFlags(&admin);
				admin_list[admin_list_size - 1] = admin;
				return;
			}

			// If reached space or tab break out of loop
			if (admin_details[i] == '"')
			{
				i++;
				name[j] = '\0';
				break;
			}

			name[j] = admin_details[i];
			i++;
			j++;
		}

		Q_strcpy(admin.name, name);
	}

	Msg("%s ", name);
	
	if (admin_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (admin_details[i] == '\0')
			{
				// No more data
				password[j] = '\0';
				Q_strcpy(admin.password, password);
				InvertAdminFlags(&admin);
				admin_list[admin_list_size - 1] = admin;
				return;
			}

			// If reached space or tab break out of loop
			if (admin_details[i] == ' ' ||
				admin_details[i] == '\t')
			{
				password[j] = '\0';
				break;
			}

			password[j] = admin_details[i];
			i++;
			j++;
		}

		Q_strcpy(admin.password, password);
	}

	Msg("%s ", password);

	i++;

	while (admin_details[i] == ' ' || admin_details[i] == '\t')
	{
		i++;
	}

	char *flags_string = &(admin_details[i]);
	bool found_group = false;

	for (int j = 0; j < admin_group_list_size; j ++)
	{
		if (FStrEq(admin_group_list[j].group_id, flags_string))
		{
			// Found matching group so copy flags 
			// This could be much neater if we just had a structure
			// for the flags only but it would mean changing all
			// if (admin_list[i].allow_xxx statements
			for (int k = 0; k < MAX_ADMIN_FLAGS; k++)
			{
				admin.flags[k] = admin_group_list[j].flags[k];
			}

			found_group = true;
			Q_strcpy (admin.group_id, flags_string);
			break;
		}
	}			

	while (admin_details[i] != '\0' && found_group == false)
	{
		for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
		{
			if (admin_details[i] == admin_flag_list[k].flag)
			{
				admin.flags[k] = false;
				break;
			}
		}

		i++;
	}

	InvertAdminFlags(&admin);

	Msg("\n");
	admin_list[admin_list_size - 1] = admin;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------

static void InvertAdminFlags(admin_t *admin)
{

	if (mani_reverse_admin_flags.GetInt() == 1)
	{
		// Invert the flags
		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			admin->flags[i] = (admin->flags[i]) ? false:true;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------

void AddAdminGroup(char *admin_details)
{
	char	group_id[128]="";
	int	i,j;
	admin_group_t	admin_group;

	if (admin_details[0] != '\"') return;

	if (!AddToList((void **) &admin_group_list, sizeof(admin_group_t), &admin_group_list_size))
	{
		return;
	}

	// default admin to have absolute power
	for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
	{
		admin_group.flags[k] = true;
	}

	Q_strcpy(admin_group.group_id, "");

	i = 1;
	j = 0;

	for (;;)
	{
		if (admin_details[i] == '\0')
		{
			// No more data
			group_id[j] = '\0';
			Q_strcpy(admin_group.group_id, group_id);
			admin_group_list[admin_group_list_size - 1] = admin_group;
			return;
		}


		// If reached end quote
		if (admin_details[i] == '\"')
		{
			group_id[j] = '\0';
			break;
		}

		group_id[j] = admin_details[i];
		i++;
		j++;
	}

	Q_strcpy(admin_group.group_id, group_id);

	Msg("%s ", group_id);

	i++;

	while (admin_details[i] != '\0')
	{
		for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
		{
			if (admin_details[i] == admin_flag_list[k].flag)
			{
				admin_group.flags[k] = false;
				break;
			}
		}

		i++;
	}

	Msg("\n");
	admin_group_list[admin_group_list_size - 1] = admin_group;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------

void	LoadAdmins(void)

{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	data_in[1024];

	// Setup the flags
	InitAdminFlags();
	FreeAdmins();

	//Get admin groups list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/admingroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load admingroups.txt\n");
	}
	else
	{
		Msg("Admin Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true))
			{
				// String is empty after parsing
				continue;
			}
		
			AddAdminGroup(data_in);
		}

		filesystem->Close(file_handle);
	}

	//Get admin list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/adminlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load adminlist.txt\n");
	}
	else
	{
		Msg("Admin steam id list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true))
			{
				// String is empty after parsing
				continue;
			}
		
			AddAdmin(data_in);
		}

		filesystem->Close(file_handle);
	}

	if (admin_list_size == 0)
	{
		Msg("WARNING YOU HAVE NO ADMINS IN YOUR ADMINLIST.TXT FILE !!!\n");
	}

}

//---------------------------------------------------------------------------------
// Purpose: Free's up admin list memory
//---------------------------------------------------------------------------------

void	FreeAdmins(void)
{
	FreeList((void **) &admin_list, &admin_list_size);
	FreeList((void **) &admin_group_list, &admin_group_list_size);

}

//---------------------------------------------------------------------------------
// Purpose: Checks if admin is elligible to run the command
//---------------------------------------------------------------------------------

bool	IsAdminAllowed(player_t *player, char *command, int admin_flag, bool check_war, int *admin_index)
{
	*admin_index = -1;

	if (check_war)
	{
		SayToPlayer(player, "Mani Admin Plugin: Command %s is disabled in war mode", command);
		return false;
	}

	if (!IsClientAdmin(player, admin_index) || check_war)
	{
		SayToPlayer(player, "Mani Admin Plugin: You are not an admin");
		return false;
	}

	if (admin_flag != ADMIN_DONT_CARE)
	{
		if (!admin_list[*admin_index].flags[admin_flag])
		{
			SayToPlayer(player, "Mani Admin Plugin: You are not authorised to run %s", command);
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
// Purpose: Process the ma_setadminflag command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaSetAdminFlag
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *flags
)
{
	player_t player;
	int	admin_index = -1;
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
		if (!IsAdminAllowed(&player, "ma_setadminflag", ALLOW_SETADMINFLAG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <user name, user id or steam id> <flag list>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <user name, user id or steam id> <flag list>", command_string);
		}

		return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.

	int	target_user_id = Q_atoi(target_string);
	char target_steam_id[128];
	player_t	target_player;
	bool	found_player = false;

	// Try looking for user id first in players on server
	if (target_user_id != 0)
	{
		target_player.user_id = target_user_id;
		if (FindPlayerByUserID(&target_player))
		{
			if (IsClientAdmin(&target_player, &admin_index))
			{
				found_player = true;
			}
		}
	}

	if (!found_player)
	{
		// Try steam id next
		Q_strcpy(target_steam_id, target_string);
		if (Q_strlen(target_steam_id) > 6)
		{
			target_steam_id[6] = '\0';
			if (FStruEq(target_steam_id, "STEAM_"))
			{
				for (int i = 0; i < admin_list_size; i++)
				{
					if (FStrEq(admin_list[i].steam_id, target_string))
					{
						admin_index = i;
						found_player = true;
						break;
					}
				}
			}
		}
	}

	if (!found_player)
	{
		// Try name id next
		for (int i = 0; i < admin_list_size; i++)
		{
			if (admin_list[i].name)
			{
				if (FStrEq(admin_list[i].name, target_string))
				{
					admin_index = i;
					found_player = true;
					break;
				}
			}
		}
	}

	if (!found_player)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find admin %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find admin %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found an admin to update in memory via admin_index
	// Loop through flags to set them

	bool add_flag = true;
	for (int i = 0; i < Q_strlen(flags); i ++)
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

		bool found_flag = false;

		for (int j = 0; j < MAX_ADMIN_FLAGS; j++)
		{
			if (admin_flag_list[j].flag == flags[i])
			{
				// flag is legal
				if (mani_reverse_admin_flags.GetInt() == 1)
				{
					admin_list[admin_index].flags[j] = ((add_flag == true) ? false:true);
				}
				else
				{
					admin_list[admin_index].flags[j] = ((add_flag == false) ? false:true);
				}

				found_flag = true;
				break;
			}
		}

		if (!found_flag)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Flag %c is invalid\n", flags[i]);
			}
			else
			{
				SayToPlayer(&player, "Flag %c is invalid", flags[i]);
			}
		}
	}

	LogCommand (player.entity, "ma_setadminflag [%s] [%s]\n", target_string, flags);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------

CON_COMMAND(ma_setadminflag, "Changes an admins flags using +a -a to affect them")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaSetAdminFlag
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // admin name
					engine->Cmd_Argv(2) // flag list
					);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for re loading admin lists
//---------------------------------------------------------------------------------
CON_COMMAND(ma_reloadadmin, "Forces the plugin to reload the admin lists")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	LoadAdmins();
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Sets up admin flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
static
void	InitAdminFlags(void)
{
	admin_flag_list[ALLOW_GIMP].flag = ALLOW_GIMP_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_GIMP].flag_desc,ALLOW_GIMP_DESC);

	admin_flag_list[ALLOW_KICK].flag = ALLOW_KICK_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_KICK].flag_desc,ALLOW_KICK_DESC);

	admin_flag_list[ALLOW_RCON].flag = ALLOW_RCON_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RCON].flag_desc,ALLOW_RCON_DESC);

	admin_flag_list[ALLOW_RCON_MENU].flag = ALLOW_RCON_MENU_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RCON_MENU].flag_desc,ALLOW_RCON_MENU_DESC);

	admin_flag_list[ALLOW_EXPLODE].flag = ALLOW_EXPLODE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_EXPLODE].flag_desc,ALLOW_EXPLODE_DESC);

	admin_flag_list[ALLOW_SLAY].flag = ALLOW_SLAY_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SLAY].flag_desc,ALLOW_SLAY_DESC);

	admin_flag_list[ALLOW_BAN].flag = ALLOW_BAN_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_BAN].flag_desc,ALLOW_BAN_DESC);

	admin_flag_list[ALLOW_SAY].flag = ALLOW_SAY_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SAY].flag_desc,ALLOW_SAY_DESC);

	admin_flag_list[ALLOW_CHAT].flag = ALLOW_CHAT_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CHAT].flag_desc,ALLOW_CHAT_DESC);

	admin_flag_list[ALLOW_PSAY].flag = ALLOW_PSAY_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_PSAY].flag_desc,ALLOW_PSAY_DESC);

	admin_flag_list[ALLOW_CHANGEMAP].flag = ALLOW_CHANGEMAP_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CHANGEMAP].flag_desc,ALLOW_CHANGEMAP_DESC);

	admin_flag_list[ALLOW_PLAYSOUND].flag = ALLOW_PLAYSOUND_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_PLAYSOUND].flag_desc,ALLOW_PLAYSOUND_DESC);

	admin_flag_list[ALLOW_RESTRICTWEAPON].flag = ALLOW_RESTRICTWEAPON_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RESTRICTWEAPON].flag_desc,ALLOW_RESTRICTWEAPON_DESC);

	admin_flag_list[ALLOW_CONFIG].flag = ALLOW_CONFIG_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CONFIG].flag_desc,ALLOW_CONFIG_DESC);

	admin_flag_list[ALLOW_CEXEC].flag = ALLOW_CEXEC_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CEXEC].flag_desc,ALLOW_CEXEC_DESC);

	admin_flag_list[ALLOW_CEXEC_MENU].flag = ALLOW_CEXEC_MENU_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CEXEC_MENU].flag_desc,ALLOW_CEXEC_MENU_DESC);

	admin_flag_list[ALLOW_BLIND].flag = ALLOW_BLIND_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_BLIND].flag_desc,ALLOW_BLIND_DESC);

	admin_flag_list[ALLOW_SLAP].flag = ALLOW_SLAP_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SLAP].flag_desc,ALLOW_SLAP_DESC);

	admin_flag_list[ALLOW_FREEZE].flag = ALLOW_FREEZE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_FREEZE].flag_desc,ALLOW_FREEZE_DESC);

	admin_flag_list[ALLOW_TELEPORT].flag = ALLOW_TELEPORT_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_TELEPORT].flag_desc,ALLOW_TELEPORT_DESC);

	admin_flag_list[ALLOW_DRUG].flag = ALLOW_DRUG_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_DRUG].flag_desc,ALLOW_DRUG_DESC);

	admin_flag_list[ALLOW_SWAP].flag = ALLOW_SWAP_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SWAP].flag_desc,ALLOW_SWAP_DESC);

	admin_flag_list[ALLOW_RCON_VOTE].flag = ALLOW_RCON_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag_desc,ALLOW_RCON_VOTE_DESC);

	admin_flag_list[ALLOW_MENU_RCON_VOTE].flag = ALLOW_MENU_RCON_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_MENU_RCON_VOTE].flag_desc,ALLOW_MENU_RCON_VOTE_DESC);

	admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag = ALLOW_RANDOM_MAP_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag_desc,ALLOW_RANDOM_MAP_VOTE_DESC);

	admin_flag_list[ALLOW_MAP_VOTE].flag = ALLOW_MAP_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag_desc,ALLOW_MAP_VOTE_DESC);

	admin_flag_list[ALLOW_QUESTION_VOTE].flag = ALLOW_QUESTION_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_QUESTION_VOTE].flag_desc,ALLOW_QUESTION_VOTE_DESC);

	admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag = ALLOW_MENU_QUESTION_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag_desc,ALLOW_MENU_QUESTION_VOTE_DESC);

	admin_flag_list[ALLOW_CANCEL_VOTE].flag = ALLOW_CANCEL_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CANCEL_VOTE].flag_desc,ALLOW_CANCEL_VOTE_DESC);

	admin_flag_list[ALLOW_ACCEPT_VOTE].flag = ALLOW_ACCEPT_VOTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_ACCEPT_VOTE].flag_desc,ALLOW_ACCEPT_VOTE_DESC);

	admin_flag_list[ALLOW_MA_RATES].flag = ALLOW_MA_RATES_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_MA_RATES].flag_desc,ALLOW_MA_RATES_DESC);

	admin_flag_list[ALLOW_BURN].flag = ALLOW_BURN_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_BURN].flag_desc,ALLOW_BURN_DESC);

	admin_flag_list[ALLOW_NO_CLIP].flag = ALLOW_NO_CLIP_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_NO_CLIP].flag_desc,ALLOW_NO_CLIP_DESC);

	admin_flag_list[ALLOW_WAR].flag = ALLOW_WAR_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_WAR].flag_desc,ALLOW_WAR_DESC);

	admin_flag_list[ALLOW_MUTE].flag = ALLOW_MUTE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_MUTE].flag_desc,ALLOW_MUTE_DESC);

	admin_flag_list[ALLOW_RESET_ALL_RANKS].flag = ALLOW_RESET_ALL_RANKS_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RESET_ALL_RANKS].flag_desc,ALLOW_RESET_ALL_RANKS_DESC);

	admin_flag_list[ALLOW_CASH].flag = ALLOW_CASH_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_CASH].flag_desc,ALLOW_CASH_DESC);

	admin_flag_list[ALLOW_RCONSAY].flag = ALLOW_RCONSAY_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_RCONSAY].flag_desc,ALLOW_RCONSAY_DESC);

	admin_flag_list[ALLOW_SKINS].flag = ALLOW_SKINS_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SKINS].flag_desc,ALLOW_SKINS_DESC);

	admin_flag_list[ALLOW_SETSKINS].flag = ALLOW_SETSKINS_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SETSKINS].flag_desc,ALLOW_SETSKINS_DESC);

	admin_flag_list[ALLOW_DROPC4].flag = ALLOW_DROPC4_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_DROPC4].flag_desc,ALLOW_DROPC4_DESC);

	admin_flag_list[ALLOW_SETADMINFLAG].flag = ALLOW_SETADMINFLAG_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SETADMINFLAG].flag_desc,ALLOW_SETADMINFLAG_DESC);

	admin_flag_list[ALLOW_COLOUR].flag = ALLOW_COLOUR_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_COLOUR].flag_desc,ALLOW_COLOUR_DESC);

	admin_flag_list[ALLOW_TIMEBOMB].flag = ALLOW_TIMEBOMB_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_TIMEBOMB].flag_desc,ALLOW_TIMEBOMB_DESC);

	admin_flag_list[ALLOW_FIREBOMB].flag = ALLOW_FIREBOMB_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_FIREBOMB].flag_desc,ALLOW_FIREBOMB_DESC);

	admin_flag_list[ALLOW_FREEZEBOMB].flag = ALLOW_FREEZEBOMB_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_FREEZEBOMB].flag_desc,ALLOW_FREEZEBOMB_DESC);

	admin_flag_list[ALLOW_HEALTH].flag = ALLOW_HEALTH_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_HEALTH].flag_desc,ALLOW_HEALTH_DESC);

	admin_flag_list[ALLOW_BEACON].flag = ALLOW_BEACON_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_BEACON].flag_desc,ALLOW_BEACON_DESC);

	admin_flag_list[ALLOW_GIVE].flag = ALLOW_GIVE_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_GIVE].flag_desc,ALLOW_GIVE_DESC);

	admin_flag_list[ALLOW_SPRAY_TAG].flag = ALLOW_SPRAY_TAG_FLAG;
	Q_strcpy(admin_flag_list[ALLOW_SPRAY_TAG].flag_desc,ALLOW_SPRAY_TAG_DESC);

}
