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
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_help.h" 
#include "mani_vfuncs.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_commands.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiHelp::ManiHelp()
{
	gpManiHelp = this;
}

ManiHelp::~ManiHelp()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiHelp::ShowHelp(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (help_id == 0)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "No help available for this command");
		return PLUGIN_STOP;
	}

	int index = gpCmd->GetCmdIndexForHelpID(help_id);
	if (index == -1)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "No help available for this command");
		return PLUGIN_STOP;
	}

	gpCmd->DumpHelp(player_ptr, index, command_type);
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_help command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiHelp::ProcessMaHelp(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	bool	admin_flag = true;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			admin_flag = false;
		}
	}

	if (gpCmd->Cmd_Argc() == 1)
	{
		gpCmd->ShowAllCommands(player_ptr, admin_flag);
		return PLUGIN_STOP;
	}

	gpCmd->SearchCommands(player_ptr, admin_flag, gpCmd->Cmd_Argv(1), command_type);
	return PLUGIN_STOP;
}

ManiHelp	g_ManiHelp;
ManiHelp	*gpManiHelp;
