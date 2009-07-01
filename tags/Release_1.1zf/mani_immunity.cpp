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
#include "mani_immunity_flags.h"
#include "mani_parser.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_immunity.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static void InvertImmunityFlags(immunity_t *immunity);
static void	InitImmunityFlags(void);


immunity_t			*immunity_list=NULL;
immunity_group_t	*immunity_group_list=NULL;
int					immunity_list_size=0;
int					immunity_group_list_size=0;
immunity_flag_t		immunity_flag_list[MAX_IMMUNITY_FLAGS];


//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
bool IsImmune(player_t *player, int *immunity_index)
{

	*immunity_index = -1;

	if (player->is_bot)
	{
		return false;
	}

	const char *password  = engine->GetClientConVarValue(player->index, "_password" );

	//Search through immunity list for match
	for (int i = 0; i < immunity_list_size; i ++)
	{
		// Check Steam ID
		if (immunity_list[i].steam_id)
		{
			if (FStrEq(immunity_list[i].steam_id, player->steam_id))
			{
				*immunity_index = i;
				return true;
			}
		}

		// Check IP address
		if (immunity_list[i].ip_address && player->ip_address)
		{
			if (FStrEq(immunity_list[i].ip_address, player->ip_address)) 
			{
				*immunity_index = i;
				return true;
			}
		}

		// Check name password combo
		if (immunity_list[i].name && immunity_list[i].password)
		{
			if (strcmp(immunity_list[i].name, player->name) == 0)
			{
				if (strcmp(immunity_list[i].password, password) == 0)
				{
					*immunity_index = i;
					return true;
				}
			}
		}	
	}
	
	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity config line setting flags
//---------------------------------------------------------------------------------

void AddImmunity(char *immunity_details)
{
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	char	password[128];
	int	i,j;
	immunity_t	immunity;

	if (!AddToList((void **) &immunity_list, sizeof(immunity_t), &immunity_list_size))
	{
		return;
	}

	// default immunity to have absolute power
	for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
	{
		immunity.flags[k] = false;
	}

	Q_strcpy(immunity.steam_id, "");
	Q_strcpy(steam_id, "");
	Q_strcpy(immunity.ip_address, "");
	Q_strcpy(ip_address, "");
	Q_strcpy(name, "");
	Q_strcpy(immunity.name, "");
	Q_strcpy(password, "");
	Q_strcpy(immunity.password, "");
	Q_strcpy(immunity.group_id, "");

	i = 0;

	if (immunity_details[i] != ';')
	{
		j = 0;
		for (;;)
		{
			if (immunity_details[i] == '\0')
			{
				// No more data
				steam_id[j] = '\0';
				Q_strcpy(immunity.steam_id, steam_id);
				InvertImmunityFlags(&immunity);
				immunity_list[immunity_list_size - 1] = immunity;
				return;
			}


			// If reached space or tab break out of loop
			if (immunity_details[i] == ' ' ||
				immunity_details[i] == '\t' ||
				immunity_details[i] == ';')
			{
				steam_id[j] = '\0';
				break;
			}

			steam_id[j] = immunity_details[i];
			i++;
			j++;
		}

		Q_strcpy(immunity.steam_id, steam_id);
	}

	Msg("%s ", steam_id);

	if (immunity_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (immunity_details[i] == '\0')
			{
				// No more data
				ip_address[j] = '\0';
				Q_strcpy(immunity.ip_address, ip_address);
				InvertImmunityFlags(&immunity);
				immunity_list[immunity_list_size - 1] = immunity;
				return;
			}

			// If reached space or tab break out of loop
			if (immunity_details[i] == ' ' ||
				immunity_details[i] == ';' ||
				immunity_details[i] == '\t')
			{
				ip_address[j] = '\0';
				break;
			}

			ip_address[j] = immunity_details[i];
			i++;
			j++;
		}

		Q_strcpy(immunity.ip_address, ip_address);
	}

	Msg("%s ", ip_address);
	
	if (immunity_details[i] == ';' && immunity_details[i + 1] == '\"')
	{
		i += 2;
		j = 0;

		for (;;)
		{
			if (immunity_details[i] == '\0')
			{
				// No more data
				name[j] = '\0';
				Q_strcpy(immunity.password, name);
				InvertImmunityFlags(&immunity);
				immunity_list[immunity_list_size - 1] = immunity;
				return;
			}

			// If reached space or tab break out of loop
			if (immunity_details[i] == '"')
			{
				i++;
				name[j] = '\0';
				break;
			}

			name[j] = immunity_details[i];
			i++;
			j++;
		}

		Q_strcpy(immunity.name, name);
	}

	Msg("%s ", name);
	
	if (immunity_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (immunity_details[i] == '\0')
			{
				// No more data
				password[j] = '\0';
				Q_strcpy(immunity.password, password);
				InvertImmunityFlags(&immunity);
				immunity_list[immunity_list_size - 1] = immunity;
				return;
			}

			// If reached space or tab break out of loop
			if (immunity_details[i] == ' ' ||
				immunity_details[i] == '\t')
			{
				password[j] = '\0';
				break;
			}

			password[j] = immunity_details[i];
			i++;
			j++;
		}

		Q_strcpy(immunity.password, password);
	}

	Msg("%s ", password);

	i++;

	while (immunity_details[i] == ' ' || immunity_details[i] == '\t')
	{
		i++;
	}

	char *flags_string = &(immunity_details[i]);
	bool found_group = false;

	for (int j = 0; j < immunity_group_list_size; j ++)
	{
		if (FStrEq(immunity_group_list[j].group_id, flags_string))
		{
			// Found matching group so copy flags 
			// This could be much neater if we just had a structure
			// for the flags only but it would mean changing all
			// if (immunity_list[i].allow_xxx statements
			for (int k = 0; k < MAX_IMMUNITY_FLAGS; k++)
			{
				immunity.flags[k] = immunity_group_list[j].flags[k];
			}

			found_group = true;
			Q_strcpy (immunity.group_id, flags_string);
			break;
		}
	}			

	while (immunity_details[i] != '\0' && found_group == false)
	{
		for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
		{
			if (immunity_details[i] == immunity_flag_list[k].flag)
			{
				immunity.flags[k] = true;
				break;
			}
		}

		i++;
	}

	InvertImmunityFlags(&immunity);

	Msg("\n");
	immunity_list[immunity_list_size - 1] = immunity;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity Group config line setting flags
//---------------------------------------------------------------------------------

static void InvertImmunityFlags(immunity_t *immunity)
{

	if (mani_reverse_immunity_flags.GetInt() == 1)
	{
		// Invert the flags
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			immunity->flags[i] = (immunity->flags[i]) ? false:true;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity Group config line setting flags
//---------------------------------------------------------------------------------

void AddImmunityGroup(char *immunity_details)
{
	char	group_id[128]="";
	int	i,j;
	immunity_group_t	immunity_group;

	if (immunity_details[0] != '\"') return;

	if (!AddToList((void **) &immunity_group_list, sizeof(immunity_group_t), &immunity_group_list_size))
	{
		return;
	}

	// default immunity to have absolute power
	for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
	{
		immunity_group.flags[k] = false;
	}

	Q_strcpy(immunity_group.group_id, "");

	i = 1;
	j = 0;

	for (;;)
	{
		if (immunity_details[i] == '\0')
		{
			// No more data
			group_id[j] = '\0';
			Q_strcpy(immunity_group.group_id, group_id);
			immunity_group_list[immunity_group_list_size - 1] = immunity_group;
			return;
		}


		// If reached end quote
		if (immunity_details[i] == '\"')
		{
			group_id[j] = '\0';
			break;
		}

		group_id[j] = immunity_details[i];
		i++;
		j++;
	}

	Q_strcpy(immunity_group.group_id, group_id);

	Msg("%s ", group_id);

	i++;

	while (immunity_details[i] != '\0')
	{
		for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
		{
			if (immunity_details[i] == immunity_flag_list[k].flag)
			{
				immunity_group.flags[k] = true;
				break;
			}
		}

		i++;
	}

	Msg("\n");
	immunity_group_list[immunity_group_list_size - 1] = immunity_group;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity Group config line setting flags
//---------------------------------------------------------------------------------

void	LoadImmunity(void)

{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	data_in[1024];

	// Setup the flags
	InitImmunityFlags();
	FreeImmunity();

	//Get immunity groups list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitygroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load immunitygroups.txt\n");
	}
	else
	{
		Msg("Immunity Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true))
			{
				// String is empty after parsing
				continue;
			}
		
			AddImmunityGroup(data_in);
		}

		filesystem->Close(file_handle);
	}

	//Get immunity list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitylist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load immunitylist.txt\n");
	}
	else
	{
		Msg("Immunity list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true))
			{
				// String is empty after parsing
				continue;
			}
		
			AddImmunity(data_in);
		}

		filesystem->Close(file_handle);
	}

	if (immunity_list_size == 0)
	{
		Msg("WARNING YOU HAVE NO IMMUNITY MEMBERS IN YOUR IMMUNITYLIST.TXT FILE !!!\n");
	}

}

//---------------------------------------------------------------------------------
// Purpose: Parses the Immunity Group config line setting flags
//---------------------------------------------------------------------------------

void	FreeImmunity(void)
{
	FreeList((void **) &immunity_list, &immunity_list_size);
	FreeList((void **) &immunity_group_list, &immunity_group_list_size);

}
//---------------------------------------------------------------------------------
// Purpose: Sets up immunity flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
static
void	InitImmunityFlags(void)
{
	immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag = IMMUNITY_ALLOW_GIMP_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag_desc,IMMUNITY_ALLOW_GIMP_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_KICK].flag = IMMUNITY_ALLOW_KICK_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_KICK].flag_desc,IMMUNITY_ALLOW_KICK_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag = IMMUNITY_ALLOW_SLAY_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag_desc,IMMUNITY_ALLOW_SLAY_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_BAN].flag = IMMUNITY_ALLOW_BAN_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BAN].flag_desc,IMMUNITY_ALLOW_BAN_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag = IMMUNITY_ALLOW_CEXEC_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag_desc,IMMUNITY_ALLOW_CEXEC_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag = IMMUNITY_ALLOW_BLIND_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag_desc,IMMUNITY_ALLOW_BLIND_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag = IMMUNITY_ALLOW_SLAP_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag_desc,IMMUNITY_ALLOW_SLAP_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag = IMMUNITY_ALLOW_FREEZE_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag_desc,IMMUNITY_ALLOW_FREEZE_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag = IMMUNITY_ALLOW_TELEPORT_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag_desc,IMMUNITY_ALLOW_TELEPORT_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag = IMMUNITY_ALLOW_DRUG_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag_desc,IMMUNITY_ALLOW_DRUG_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag = IMMUNITY_ALLOW_SWAP_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag_desc,IMMUNITY_ALLOW_SWAP_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_TAG].flag = IMMUNITY_ALLOW_TAG_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TAG].flag_desc,IMMUNITY_ALLOW_TAG_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag = IMMUNITY_ALLOW_BALANCE_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag_desc,IMMUNITY_ALLOW_BALANCE_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_BURN].flag = IMMUNITY_ALLOW_BURN_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BURN].flag_desc,IMMUNITY_ALLOW_BURN_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag = IMMUNITY_ALLOW_MUTE_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag_desc,IMMUNITY_ALLOW_MUTE_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag = IMMUNITY_ALLOW_RESERVE_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag_desc,IMMUNITY_ALLOW_RESERVE_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag = IMMUNITY_ALLOW_SETSKIN_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag_desc,IMMUNITY_ALLOW_SETSKIN_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag = IMMUNITY_ALLOW_RESERVE_SKIN_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag_desc,IMMUNITY_ALLOW_RESERVE_SKIN_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag = IMMUNITY_ALLOW_TIMEBOMB_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag_desc,IMMUNITY_ALLOW_TIMEBOMB_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag = IMMUNITY_ALLOW_FIREBOMB_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag_desc,IMMUNITY_ALLOW_FIREBOMB_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag = IMMUNITY_ALLOW_FREEZEBOMB_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag_desc,IMMUNITY_ALLOW_FREEZEBOMB_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag = IMMUNITY_ALLOW_BEACON_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag_desc,IMMUNITY_ALLOW_BEACON_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag = IMMUNITY_ALLOW_GHOST_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag_desc,IMMUNITY_ALLOW_GHOST_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag = IMMUNITY_ALLOW_GIVE_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag_desc,IMMUNITY_ALLOW_GIVE_DESC);

	immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag = IMMUNITY_ALLOW_COLOUR_FLAG;
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag_desc,IMMUNITY_ALLOW_COLOUR_DESC);

}
