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
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_replace.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	bool	war_mode;

struct command_t
{
	char	command_type[2];
	char	command_string[512];
	char	command_alias[512];
};

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static	command_t	*command_list = NULL;
static	int	command_list_size = 0;

//---------------------------------------------------------------------------------
// Purpose: Frees the command list
//---------------------------------------------------------------------------------

void	FreeCommandList(void)
{
	FreeList((void **) &command_list, &command_list_size);
}


//---------------------------------------------------------------------------------
// Purpose: Parses the commandlist.txt file to create substitute commands
//---------------------------------------------------------------------------------

void	LoadCommandList(void)

{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	data_in[1024];
	char	alias[512];
	char	command_type[2];
	char	command_string[512];

	FreeList((void **) &command_list, &command_list_size);

	//Get command list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/commandlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load commandlist.txt\n");
	}
	else
	{
//		MMsg("Replacement command list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseCommandReplace(data_in, alias, command_type, command_string))
			{
				// String is empty after parsing
				continue;
			}
		
			AddToList((void **) &command_list, sizeof(command_t), &command_list_size);
			Q_strcpy(command_list[command_list_size - 1].command_type, command_type);
			Q_strcpy(command_list[command_list_size - 1].command_string, command_string);
			Q_strcpy(command_list[command_list_size - 1].command_alias, alias);

//			MMsg("Alias [%s] Type [%s] Command [%s]\n", alias, command_type, command_string);
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Checks the incomming command for a replacement
//---------------------------------------------------------------------------------
bool	CheckForReplacement 
(
 player_t *player,
 char	*command_string
)
{
	int		admin_index;

	for (int i = 0; i < command_list_size; i++)
	{
		if (FStrEq(command_string, command_list[i].command_alias))
		{
			// RCON command string
			// found a match
			if (FStrEq(command_list[i].command_type, "R"))
			{
				if (!gpManiClient->IsAdminAllowed(player, "ma_kick", ALLOW_RCONSAY, war_mode, &admin_index))
				{
					return true;
				}

				char	rcon_cmd[512];
				Q_snprintf(rcon_cmd, sizeof(rcon_cmd), "%s\n", command_list[i].command_string);

				if (Q_strstr(rcon_cmd, "ma_setcash") ||
					Q_strstr(rcon_cmd, "ma_givecash") ||
					Q_strstr(rcon_cmd, "ma_givecashp") ||
					Q_strstr(rcon_cmd, "ma_takecash") ||
					Q_strstr(rcon_cmd, "ma_takecashp") ||
					Q_strstr(rcon_cmd, "ma_sethealth") ||
					Q_strstr(rcon_cmd, "ma_takehealth") ||
					Q_strstr(rcon_cmd, "ma_takehealthp") ||
					Q_strstr(rcon_cmd, "ma_givehealth") ||
					Q_strstr(rcon_cmd, "ma_givehealthp"))
				{
					SayToPlayer(ORANGE_CHAT, player,"The command [%s] should be used as a 'C' type command only");
					return false;
				}

				LogCommand(player, "%s => %s\n", command_string, command_list[i].command_string);
				engine->ServerCommand(rcon_cmd);
				return false;
			}
			else if (FStrEq(command_list[i].command_type, "C"))
			{
				char	client_cmd[512];
				Q_snprintf(client_cmd, sizeof(client_cmd), "%s\n", command_list[i].command_string);
				engine->ClientCommand(player->entity, client_cmd);
				return false;
			}
			else
			{
				Q_strcpy(command_string, command_list[i].command_string);
				return true;
			}
		}
	}

	return true;
}

