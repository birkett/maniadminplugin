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
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_team.h"
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_warmuptimer.h"
#include "mani_util.h"
#include "mani_anti_rejoin.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	time_t	g_RealTime;

ConVar mani_anti_rejoin ("mani_anti_rejoin", "0", 0, "0 = disabled, 1 = slay players who use the 'retry' trick", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiAntiRejoin::ManiAntiRejoin()
{
	gpManiAntiRejoin = this;
	ResetList();
}

ManiAntiRejoin::~ManiAntiRejoin()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiAntiRejoin::ClientDisconnect(player_t	*player_ptr)
{
	if (war_mode) return;
	if (IsLAN()) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (mani_anti_rejoin.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	int ct = 0;
	int t = 0;
	for (int i = 0; i < max_players; i++)
	{
		player_t player;
		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.team == 2) 
		{
			t ++;
		}
		else if (player.team == 3)
		{
			ct ++;
		}

		if (ct != 0 && t != 0) 
		{
			break;
		}
	}

	if (ct == 0 || t == 0)
	{
		return;
	}

	rejoin_list[player_ptr->steam_id] = gpManiTeam->GetTeamScore(2) + gpManiTeam->GetTeamScore(3);
}

//---------------------------------------------------------------------------------
// Purpose: Player Spawned
//---------------------------------------------------------------------------------
void	ManiAntiRejoin::PlayerSpawn(player_t *player_ptr)
{
	if (war_mode) return;
	if (IsLAN()) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;
	if (mani_anti_rejoin.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	if(rejoin_list.find(player_ptr->steam_id) == rejoin_list.end())
	{
		return;
	}

	if (rejoin_list[player_ptr->steam_id] == gpManiTeam->GetTeamScore(2) + gpManiTeam->GetTeamScore(3))
	{
		SlayPlayer(player_ptr, true, true, true);
		SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3060, "%s", player_ptr->name));
		LogCommand (NULL, "slayed user for rejoining the same round [%s] [%s]\n", player_ptr->name, player_ptr->steam_id);
	}
}


ManiAntiRejoin	g_ManiAntiRejoin;
ManiAntiRejoin	*gpManiAntiRejoin;
