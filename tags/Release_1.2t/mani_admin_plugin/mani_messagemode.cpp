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
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "Color.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_commands.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_help.h"
#include "mani_parser.h"
#include "mani_gametype.h"
#include "mani_maps.h"
#include "mani_messagemode.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	time_t	g_RealTime;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiMessageMode::ManiMessageMode()
{
	gpManiMessageMode = this;
}

ManiMessageMode::~ManiMessageMode()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiMessageMode::Load(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiMessageMode::Unload(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiMessageMode::LevelInit(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiMessageMode::NetworkIDValidated(player_t *player_ptr)
{
	this->DisableMessageMode(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiMessageMode::ClientDisconnect(player_t	*player_ptr)
{
	this->DisableMessageMode(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Clean Up status list
//---------------------------------------------------------------------------------
void ManiMessageMode::CleanUp(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		mess_mode_list[i].admin_psay = false;
		for (int j = 0; j < MANI_MAX_PLAYERS; j++)
		{
			mess_mode_list[i].admin_psay_list[j] = false;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Clean Up status list
//---------------------------------------------------------------------------------
void ManiMessageMode::DisableMessageMode(int index)
{
	if (mess_mode_list[index].admin_psay)
	{
		for (int i = 0; i < max_players; i++)
		{
			mess_mode_list[index].admin_psay_list[i] = false;
		}

		mess_mode_list[index].admin_psay = false;
	}

	for (int i = 0; i < max_players; i++)
	{
		if (mess_mode_list[i].admin_psay)
		{
			if (mess_mode_list[i].admin_psay_list[index])
			{
				// Disable chat for this person and the other person
				player_t player;
				player.index = i + 1;
				if (!FindPlayerByIndex(&player)) continue;
				OutputHelpText(ORANGE_CHAT, &player, "%s", Translate(&player, 3083, "%s", player.name));
				mess_mode_list[i].admin_psay_list[index] = false;
				mess_mode_list[i].admin_psay = false;
				for (int j = 0; j < max_players; j++)
				{
					if (mess_mode_list[i].admin_psay_list[j])
					{
						mess_mode_list[i].admin_psay = true;
						break;
					}
				}

				if (!mess_mode_list[i].admin_psay)
				{
					OutputHelpText(ORANGE_CHAT, &player, "%s", Translate(&player, 3082));
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_pmess command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaPMess(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!player_ptr) return PLUGIN_STOP;

	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PSAY, war_mode)) return PLUGIN_BAD_ADMIN;
	// Whoever issued the commmand is authorised to do it.

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	for (int argc = 1; argc < gpCmd->Cmd_Argc(); argc++)
	{
		const char *target_string = gpCmd->Cmd_Argv(argc);

		// Find targets
		if (!FindTargetPlayers(player_ptr, target_string, NULL))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
			return PLUGIN_STOP;
		}

		mess_mode_list[player_ptr->index - 1].admin_psay = false;
		for (int i = 0; i < target_player_list_size; i++)
		{
			player_t *target_ptr = &(target_player_list[i]);

			// Ignore bots
			if (target_ptr->is_bot) continue;
			if (mess_mode_list[player_ptr->index - 1].admin_psay_list[target_ptr->index - 1])
			{
				mess_mode_list[player_ptr->index - 1].admin_psay_list[target_ptr->index - 1] = false;
				OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3081, "%s", target_ptr->name));
			}
			else
			{
				mess_mode_list[player_ptr->index - 1].admin_psay_list[target_ptr->index - 1] = true;
				OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3080, "%s", target_ptr->name));
				mess_mode_list[player_ptr->index - 1].admin_psay = true;
			}
		}
	}

	if (!mess_mode_list[player_ptr->index - 1].admin_psay)
	{
		for (int i = 0; i < max_players; i++)
		{
			if (mess_mode_list[player_ptr->index - 1].admin_psay_list[i])
			{
				mess_mode_list[player_ptr->index - 1].admin_psay = true;
				break;
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_exit command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaExit(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!player_ptr) return PLUGIN_STOP;

	int index = player_ptr->index - 1;

	if (mess_mode_list[index].admin_psay)
	{
		mess_mode_list[index].admin_psay = false;

		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3082));
		for (int i = 0; i < max_players; i++)
		{
			if (mess_mode_list[index].admin_psay_list[i])
			{
				// Disable chat for this person and the other person
				mess_mode_list[index].admin_psay_list[i] = false;
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_psay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaPSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);
	const char *say_string = gpCmd->Cmd_Args(2);
	player_t *target_player = NULL;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PSAY, war_mode)) return PLUGIN_BAD_ADMIN;
		if (mess_mode_list[player_ptr->index - 1].admin_psay)
		{
			for (int i = 0; i < max_players; i ++)
			{
				if (mess_mode_list[player_ptr->index - 1].admin_psay_list[i])
				{
					player_t player;
					target_player = &player;
					target_player->index = i + 1;
					if (!FindPlayerByIndex(target_player)) continue;
					if (target_player->is_bot) continue;

					bool target_is_admin;
					target_is_admin = false;

					if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
					{
						target_is_admin = true;
					}

					if (mani_adminsay_anonymous.GetInt() == 1 && !target_is_admin)
					{
						SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) to (%s) : %s", target_player->name, gpCmd->Cmd_Args(1));
						SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) to (%s) : %s", target_player->name, gpCmd->Cmd_Args(1));
					}
					else
					{
						SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, gpCmd->Cmd_Args(1));
						SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, gpCmd->Cmd_Args(1));
					}

					LogCommand(player_ptr, "%s (ADMIN) %s to (%s) : %s\n", command_name, player_ptr->name, target_player->name, gpCmd->Cmd_Args(1));
				}
			}

			return PLUGIN_STOP;
		}
	}
	
	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_TARGET_BOT,"%s", target_player_list[i].name));
			continue;
		}

		if (player_ptr)
		{
			bool target_is_admin;

			target_is_admin = false;

			if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
			{
				target_is_admin = true;
			}

			if (mani_adminsay_anonymous.GetInt() == 1 && !target_is_admin)
			{
				SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) to (%s) : %s", target_player->name, say_string);
				SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) to (%s) : %s", target_player->name, say_string);
			}
			else
			{
				SayToPlayer(GREEN_CHAT, target_player, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, say_string);
				SayToPlayer(GREEN_CHAT, player_ptr, "(ADMIN) %s to (%s) : %s", player_ptr->name, target_player->name, say_string);
			}	

			LogCommand(player_ptr, "%s %s (ADMIN) %s to (%s) : %s\n", command_name, target_string, player_ptr->name, target_player->name, say_string);
		}
		else
		{
			SayToPlayer(GREEN_CHAT, target_player, "%s", say_string);
			LogCommand(player_ptr, "%s %s (CONSOLE) to (%s) : %s\n", command_name, target_string, target_player->name, say_string);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_msay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaMSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	int	time_to_display;
	msay_t	*lines_list = NULL;
	int		lines_list_size = 0;
	char	temp_line[2048];
	const char *time_display = gpCmd->Cmd_Argv(1);
	const char *target_string = gpCmd->Cmd_Argv(2);
	const char *say_string = gpCmd->Cmd_Args(3);

	if (mani_use_amx_style_menu.GetInt() == 0 || !gpManiGameType->IsAMXMenuAllowed()) return PLUGIN_STOP;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PSAY, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	time_to_display = atoi(time_display);
	if (time_to_display < 0) time_to_display = 0;
	else if (time_to_display > 100) time_to_display = 100;

	if (time_to_display == 0) time_to_display = -1;

	// Build up lines to display

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

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_say command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			if (!war_mode)
			{
				if (mani_allow_chat_to_admin.GetInt() == 1)
				{
					SayToAdmin (ORANGE_CHAT, player_ptr, "%s", say_string);
				}
				else
				{
					SayToPlayer (ORANGE_CHAT, player_ptr, "You are not allowed to chat directly to admin !!");
				}
			}

			return PLUGIN_STOP;
		}

		// Player is Admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SAY))
		{
			if (!war_mode)
			{
				SayToAdmin (GREEN_CHAT, player_ptr, "%s", say_string);
			}

			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(gpCmd->Cmd_Args(1), substitute_text, &col);

	LogCommand (player_ptr, "(ALL) %s %s\n", gpCmd->Cmd_Args(1), substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1 && !war_mode)
	{
		ClientMsg(&col, 10, false, 2, "%s", substitute_text);
	}

	if (mani_adminsay_chat_area.GetInt() == 1 || war_mode)
	{
		AdminSayToAll(LIGHT_GREEN_CHAT, player_ptr, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
	}

	if (mani_adminsay_bottom_area.GetInt() == 1 && !war_mode)
	{
		AdminHSayToAll(player_ptr, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_csay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaCSay(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SAY, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	LogCommand (player_ptr, "%s %s\n", command_name, say_string); 

	if (player_ptr)
	{
		AdminCSayToAll(player_ptr, mani_adminsay_anonymous.GetInt(), "%s", say_string);
	}
	else
	{
		CSayToAll("%s", say_string);
	}
			
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_chat command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiMessageMode::ProcessMaChat(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *say_string = gpCmd->Cmd_Args(1);

	if (player_ptr)
	{
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			if (!war_mode)
			{
				if (mani_allow_chat_to_admin.GetInt() == 1)
				{
					SayToAdmin (ORANGE_CHAT, player_ptr, "%s", say_string);
				}
				else
				{
					SayToPlayer (ORANGE_CHAT, player_ptr, "You are not allowed to chat directly to admin !!");
				}
			}
			return PLUGIN_STOP;
		}

		// Player is Admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CHAT) || war_mode)
		{
			if (!war_mode)
			{
				SayToAdmin (GREEN_CHAT, player_ptr, "%s", say_string);
			}
			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(gpCmd->Cmd_Args(1), substitute_text, &col);

	LogCommand (player_ptr, "(CHAT) %s %s\n", command_name, substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1)
	{
		ClientMsg(&col, 15, true, 2, "%s", substitute_text);
	}
 
	if (mani_adminsay_chat_area.GetInt() == 1)
	{
		AdminSayToAdmin(GREEN_CHAT, player_ptr, "%s", substitute_text);
	}
					
	return PLUGIN_STOP;
}

SCON_COMMAND(ma_psay, 2009, MaPSay, false);
SCON_COMMAND(ma_msay, 2007, MaMSay, true);
SCON_COMMAND(ma_say, 2005, MaSay, true);
SCON_COMMAND(ma_csay, 2013, MaCSay, true);
SCON_COMMAND(ma_chat, 2011, MaChat, false);

ManiMessageMode	g_ManiMessageMode;
ManiMessageMode	*gpManiMessageMode;
