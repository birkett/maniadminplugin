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
#include "mani_main.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_parser.h"
#include "mani_gametype.h"
#include "mani_team.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	int		con_command_index;
extern	bool	war_mode;

static	team_t	*team_list = NULL;
static	int		team_list_size = 0;

CGameRules *gamerules_ptr = NULL;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//---------------------------------------------------------------------------------
// Purpose: Free up team manager
//---------------------------------------------------------------------------------
void FreeTeamList(void)
{
	FreeList((void **) &team_list, &team_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Slay any players that are up for being slayed due to tk/tw
//---------------------------------------------------------------------------------
bool	IsOnSameTeam (player_t *victim, player_t *attacker)
{
	// Is suicide damage ?
	if (victim->user_id == attacker->user_id)
	{
		return false;
	}

	if (!victim->entity)
	{
		// Player not found yet
		if (!FindPlayerByUserID (victim))
		{
			return false;
		}
	}

	if (!attacker->entity)
	{
		// Player not found yet
		if (!FindPlayerByUserID (attacker))
		{
			return false;
		}
	}

	if (attacker->team != victim->team)
	{
		return false;
	}

	return true;
}
