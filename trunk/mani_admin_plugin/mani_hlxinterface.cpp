//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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
#include "inetchannelinfo.h"
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
#include "mani_output.h"
#include "mani_stats.h"
#include "mani_menu.h"
#include "mani_panel.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_gametype.h"
#include "mani_vfuncs.h"
#include "mani_language.h"
#include "mani_sigscan.h"
#include "mani_commands.h"
#include "mani_hlxinterface.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerGameDLL	*serverdll;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern	bool	war_mode;

extern	CGlobalVars *gpGlobals;
extern	int	max_players;
extern	bf_write *msg_buffer;
extern	int	text_message_index;
extern	int saytext2_message_index;
extern	int saytext_message_index;


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ConVar mani_hlx_prefix ("mani_hlx_prefix", "gameME", 0, "Prefix to be used in HLstatX/gameME messages");
// Just a load of con commands for HLX stuff


// Menu say
CON_COMMAND(ma_hlx_msay, "ma_hlx_msay (<time 0 = permanent> <target> <message>)")
{
	player_t player;
	player.entity = NULL;
	int	time_to_display;
	msay_t	*lines_list = NULL;
	int		lines_list_size = 0;
	char	temp_line[2048];

	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
	if (mani_use_amx_style_menu.GetInt() == 0 || !gpManiGameType->IsAMXMenuAllowed()) return ;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 4) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <time to display> <part of user name, user id or steam id> <message>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *time_to_display_str = gpCmd->Cmd_Argv(1);
	const char *target_string = gpCmd->Cmd_Argv(2);

	//					say_argv[0].argv_string, // The time to display
	//					say_argv[1].argv_string, // The player target string

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	const char *say_string = gpCmd->Cmd_Args(3);

	time_to_display = atoi(time_to_display_str);
	if (time_to_display < 0) time_to_display = 0;
	else if (time_to_display > 100) time_to_display = 100;

	if (time_to_display == 0) time_to_display = -1;

	// Build up lines to display
//	int	message_length = Q_strlen (say_string);

	Q_strcpy(temp_line,"");
	int	j = 0;
	int	i = 0;

	while (say_string[i] != '\0')
	{
		if (say_string[i] == '\\' && say_string[i + 1] != '\0')
		{
			switch (say_string[i + 1])
			{
			case 'n':
				{
					AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
					temp_line[j] = '\0';
					strcat(temp_line,"\n");
					Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
					Q_strcpy(temp_line,"");
					j = -1;
					i++;
					break;
				}
			case '\\': temp_line[j] = '\\'; i++; break;
			default : temp_line[j] = say_string[i];break;
			}
		}
		else
		{
			temp_line[j] = say_string[i];
		}

		i ++;
		j ++;
	}

	temp_line[j] = '\0';
	if (temp_line[0] != '\0')
	{
		AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
		Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
	}

	// Found some players to talk to
	for (i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;
		for (j = 0; j < lines_list_size; j ++)
		{
			if (j == lines_list_size - 1)
			{
				DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, true);
			}
			else
			{
				DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, false);
			}
		}
	}


	FreeList((void **) &lines_list, &lines_list_size);

	return;
}

// HLX version of CSay with player target functions
CON_COMMAND(ma_hlx_csay, "ma_hlx_csay <target> <message>)")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	bool	fire_message = false;

	if (gpCmd->Cmd_Argc() < 3) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target> <message>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Argv(1);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	const char *say_string = gpCmd->Cmd_Args(2);

	MRecipientFilter mrf;
	mrf.RemoveAllRecipients();
	mrf.MakeReliable();

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;
		mrf.AddPlayer(target_player->index);
		fire_message = true;
	}

	if (!fire_message) return;

//	char	temp_string[1024];

//	snprintf(temp_string, sizeof(temp_string), "%s", say_string);

	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(say_string);
	engine->MessageEnd();

	return ;
}

// HLX version of CSay with player target functions
CON_COMMAND(ma_hlx_browse, "ma_hlx_browse <target> <URL>")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 3) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target> <url>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Argv(1);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	const char *url_string = gpCmd->Cmd_Args(2);

	MRecipientFilter mrf;
	mrf.RemoveAllRecipients();
	mrf.MakeReliable();

	bool fire_message = false;

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;
		mrf.AddPlayer(target_player->index);
		fire_message = true;
	}

	if (!fire_message) return;
	DrawURL(&mrf, (char *)mani_hlx_prefix.GetString(), url_string);

	return ;
}

// HLX version of CSay with player target functions
CON_COMMAND(ma_hlx_swap, "ma_hlx_swap <target>)")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 2) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Args(1);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			if (!CCSPlayer_SwitchTeam(EdictToCBE(target_player_list[i].entity),gpManiGameType->GetOpposingTeam(target_player_list[i].team)))
			{
				target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
			}
			else
			{
				UTIL_DropC4(target_player_list[i].entity);

				// If not dead then force model change
				if (!target_player_list[i].player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(target_player_list[i].entity));
				}
			}
		}
		else
		{
			target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
		}
	}

	return ;
}

// HLX Version of ma_psay
CON_COMMAND(ma_hlx_psay, "ma_hlx_psay <target> <message>")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 3) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target> <message>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	char	temp_string[1024];

	snprintf (temp_string, sizeof (temp_string), "%s: %s", mani_hlx_prefix.GetString(), say_string);

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;

		MRecipientFilter mrf;
		mrf.RemoveAllRecipients();
		mrf.MakeReliable();
		mrf.AddPlayer(target_player->index);


		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			// Do special version for CSS
			msg_buffer = engine->UserMessageBegin( &mrf, saytext_message_index ); 
			msg_buffer->WriteByte(target_player->index); // Entity index
			msg_buffer->WriteString(temp_string);
			msg_buffer->WriteByte(1); // Chat flag
			engine->MessageEnd();
		}
		else
		{ 
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteString(temp_string);
			engine->MessageEnd();
		}	
	}

	return ;
}

CON_COMMAND(ma_hlx_cexec, "ma_hlx_cexec <target> <command>")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 3) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target> <command>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *cmd_string = gpCmd->Cmd_Args(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	char	client_cmd[2048];
	snprintf(client_cmd, sizeof (client_cmd), "%s\n", cmd_string);
			
	// Found some players to run the command on
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);
		if (target_player->is_bot) continue;
		helpers->ClientCommand(target_player->entity, client_cmd);
	}

	return;
}

// HLX Version of ma_psay
CON_COMMAND(ma_hlx_hint, "ma_hlx_hint <target> <message>")
{
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused() || war_mode) return;
#ifndef ORANGE
	const CCommand args;
#endif
	gpCmd->ExtractClientAndServerCommand(args);

	if (gpCmd->Cmd_Argc() < 3) 
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s <target> <message>\n", gpCmd->Cmd_Argv(0));
		return;
	}

	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(NULL, target_string, NULL))
	{
		OutputToConsole(NULL, "%s\n", Translate(NULL, M_NO_TARGET, "%s", target_string));
		return;
	}

	char	temp_string[192];
	snprintf(temp_string, sizeof(temp_string), "%s", say_string); 
	SplitHintString(temp_string, 35);

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;

		MRecipientFilter mrf;
		mrf.RemoveAllRecipients();
		mrf.MakeReliable();
		mrf.AddPlayer(target_player->index);
		UTIL_SayHint(&mrf, temp_string);
	}

	return ;
}
