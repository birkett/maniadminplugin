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
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_anti_rejoin.h"
#include "mani_css_betting.h"
#include "mani_css_bounty.h"
#include "mani_teamkill.h"
#include "mani_warmuptimer.h"
#include "mani_save_scores.h"
#include "mani_mp_restartgame.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	ICvar *g_pCVar;
extern	bool war_mode;
extern	time_t	g_RealTime;
extern  float timeleft_offset;

ConVar *pMPRestartGame = NULL;

#ifdef ORANGE
FnChangeCallback_t pMPRestartGameCallback = NULL;
#else
FnChangeCallback pMPRestartGameCallback = NULL;
#endif

CONVAR_CALLBACK_PROTO(mp_restart_game_callback);
CONVAR_CALLBACK_PROTO(ManiMPRestartGameKicker);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiMPRestartGame::ManiMPRestartGame()
{
	gpManiMPRestartGame = this;
	check_timers = false;
}

ManiMPRestartGame::~ManiMPRestartGame()
{
	// Cleanup
	check_timers = false;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::Load(void)
{
	//find the commands in the server's CVAR list
	ConCommandBase *pCmd = g_pCVar->GetCommands();
	while (pCmd)
	{
		if (!pCmd->IsCommand())
		{
			if (strcmp(pCmd->GetName(), "mp_restartgame") == 0) {
				pMPRestartGame = static_cast<ConVar *>(pCmd);
				break;
			}
		}

		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}

	if (pMPRestartGame)
	{
		if (pMPRestartGame->m_fnChangeCallback)
		{
			pMPRestartGameCallback = pMPRestartGame->m_fnChangeCallback;
		}

#ifdef ORANGE
		pMPRestartGame->m_fnChangeCallback = (FnChangeCallback_t) mp_restart_game_callback;
#else
		pMPRestartGame->m_fnChangeCallback = mp_restart_game_callback;
#endif
	}


}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::Unload(void)
{
	if (pMPRestartGame && pMPRestartGameCallback)
	{
		pMPRestartGame->m_fnChangeCallback = pMPRestartGameCallback;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level Init
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::LevelInit()
{
	check_timers = false;
}

//---------------------------------------------------------------------------------
// Purpose: Level Shutdown
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::LevelShutdown()
{
	check_timers = false;
}

//---------------------------------------------------------------------------------
// Purpose: Cvar has changed
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::CVarChanged(ConVar *cvar_ptr)
{
	int	time_till_restart = cvar_ptr->GetInt();
	if (time_till_restart == 0) return;

	check_timers = true;
	pre_fire_timeout = gpGlobals->curtime + ((float) time_till_restart) - 0.1;
	on_time_timeout = gpGlobals->curtime + ((float) time_till_restart);
	post_fire_timeout = gpGlobals->curtime + ((float) time_till_restart) + 0.1;
	pre_fire_check = true;
	on_time_check = true;
	post_fire_check = true;
}

//---------------------------------------------------------------------------------
// Purpose: Game frame hook to check when game will restart
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::GameFrame()
{
	if (war_mode) return;
	if (!check_timers) return;

	if (pre_fire_check && pre_fire_timeout <= gpGlobals->curtime)
	{
		pre_fire_check = false;
		PreFire();
	}
	
	if (on_time_check && on_time_timeout <= gpGlobals->curtime)
	{
		on_time_check = false;
		Fire();
	}

	if (post_fire_check && post_fire_timeout <= gpGlobals->curtime)
	{
		post_fire_check = false;
		check_timers = false;
		PostFire();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Pre restart calls
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::PreFire()
{
	FreeTKPunishments();
	gpManiAntiRejoin->LevelInit();
	gpManiCSSBounty->LevelInit();
	gpManiSaveScores->LevelInit();
}

//---------------------------------------------------------------------------------
// Purpose: On restart calls
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::Fire()
{
	timeleft_offset = gpGlobals->curtime;
	FreeTKPunishments();
	gpManiAntiRejoin->LevelInit();
	gpManiCSSBounty->LevelInit();
	gpManiSaveScores->LevelInit();
}

//---------------------------------------------------------------------------------
// Purpose: Post restart calls
//---------------------------------------------------------------------------------
void	ManiMPRestartGame::PostFire()
{
	FreeTKPunishments();
	gpManiAntiRejoin->LevelInit();
	gpManiCSSBounty->LevelInit();
	gpManiSaveScores->LevelInit();
}

ManiMPRestartGame	g_ManiMPRestartGame;
ManiMPRestartGame	*gpManiMPRestartGame;

CONVAR_CALLBACK_FN(mp_restart_game_callback)
{
	//Msg("hooked mp_restartgame %s\n", pVar->GetString());
	if (pMPRestartGameCallback)
	{
		g_ManiMPRestartGame.CVarChanged((ConVar*)pVar);
#ifdef ORANGE
		pMPRestartGameCallback(pVar, pOldString, pOldFloat);
#else
		pMPRestartGameCallback(pVar, pOldString);
#endif
	}
}

