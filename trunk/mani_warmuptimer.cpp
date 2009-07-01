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
ConVar mani_warmup_timer ("mani_warmup_timer", "0", 0, "Time in seconds at the start of a map before performing mp_restartgame (0 = off)", true, 0, true, 180, ManiWarmupTimerCVar);

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
			CSayToAll("Warmup timer %i", mani_warmup_timer.GetInt() - ((int) gpGlobals->curtime));
		}

		next_check = gpGlobals->curtime + 1.0;
		if (gpGlobals->curtime > mani_warmup_timer.GetFloat())
		{
			check_timer = false;
			engine->ServerCommand("mp_restartgame 1\n");
		}
	}
}

ManiWarmupTimer	g_ManiWarmupTimer;
ManiWarmupTimer	*gpManiWarmupTimer;

static void ManiWarmupTimerCVar ( ConVar *var, char const *pOldString )
{
	gpManiWarmupTimer->LevelInit();
}