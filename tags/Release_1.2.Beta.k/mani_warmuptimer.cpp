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
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_maps.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_language.h"
#include "mani_customeffects.h"
#include "mani_warmuptimer.h"
#include "mani_gametype.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IGameEventManager2 *gameeventmanager;

extern	bool war_mode;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;

static void ManiWarmupTimerCVar ( ConVar *var, char const *pOldString );

ConVar mani_warmup_timer_show_countdown ("mani_warmup_timer_show_countdown", "1", 0, "1 = enable center say countdown, 0 = disable", true, 0, true, 1);
ConVar mani_warmup_timer_knives_only ("mani_warmup_timer_knives_only", "0", 0, "1 = enable knives only mode, 0 = all weapons allowed", true, 0, true, 1);
ConVar mani_warmup_timer ("mani_warmup_timer", "0", 0, "Time in seconds at the start of a map before performing mp_restartgame (0 = off)", true, 0, true, 180, ManiWarmupTimerCVar);
ConVar mani_warmup_timer_ignore_tk ("mani_warmup_timer_ignore_tk", "0", 0, "0 = tk punishment still allowed, 1 = no tk punishments", true, 0, true, 1);
ConVar mani_warmup_timer_knives_only_ignore_fyi_aim_maps ("mani_warmup_timer_knives_only_ignore_fyi_aim_maps", "0", 0, "0 = knive mode still allowed on fy/aim maps, 1 = no knive mode for fy_/aim_ maps", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiWarmupTimer::ManiWarmupTimer()
{
	// Init
	check_timer = false;
	next_check = -999.0;
	gpManiWarmupTimer = this;
}

ManiWarmupTimer::~ManiWarmupTimer()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::LevelInit(void)
{
	if (mani_warmup_timer.GetInt() == 0)
	{
		check_timer = false;
	}
	else
	{
		check_timer = true;
		next_check = -999.0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Game Frame has been run
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::GameFrame(void)
{

	if (war_mode) return;
	if (!check_timer) return;
	if (ProcessPluginPaused()) return;

	if (gpGlobals->curtime > next_check)
	{
		if (mani_warmup_timer_show_countdown.GetInt())
		{
			int time_left = mani_warmup_timer.GetInt() - ((int) gpGlobals->curtime);

			if (mani_warmup_timer_knives_only.GetInt() == 1 && 
				time_left % 5 == 0)
			{
				CSayToAll("Knives Only !!");
			}
			else 
			{
				CSayToAll("Warmup timer %i", time_left);
			}
		}

		next_check = gpGlobals->curtime + 1.0;
		if (gpGlobals->curtime > mani_warmup_timer.GetFloat())
		{
			check_timer = false;
			engine->ServerCommand("mp_restartgame 1\n");
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Knives only ?
//---------------------------------------------------------------------------------
bool		ManiWarmupTimer::KnivesOnly(void)
{
	if (!check_timer) return false;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;
	if (mani_warmup_timer_knives_only.GetInt() == 0) return false;

	// Knife mode enabled, check if we need to see what type of map this
	// is

	if (mani_warmup_timer_knives_only_ignore_fyi_aim_maps.GetInt() == 1)
	{
		// Dont care about map type
		return true;
	}

	int	length = Q_strlen(current_map);

	if (length > 2)
	{
		if (current_map[2] == '_' &&
			(current_map[1] == 'y' || current_map[1] == 'Y') &&
			(current_map[0] == 'f' || current_map[0] == 'F'))
		{
			return false;
		}
	}
	
	if (length > 3)
	{
		if (current_map[3] == '_' &&
			(current_map[2] == 'm' || current_map[2] == 'M') &&
			(current_map[1] == 'i' || current_map[1] == 'i') &&
			(current_map[0] == 'a' || current_map[0] == 'a'))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Ignore TK punishments
//---------------------------------------------------------------------------------
bool		ManiWarmupTimer::IgnoreTK(void)
{
	if (!check_timer) return false;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;

	return ((mani_warmup_timer_ignore_tk.GetInt() == 0) ? false:true);
}

ManiWarmupTimer	g_ManiWarmupTimer;
ManiWarmupTimer	*gpManiWarmupTimer;

static void ManiWarmupTimerCVar ( ConVar *var, char const *pOldString )
{
	gpManiWarmupTimer->LevelInit();
}