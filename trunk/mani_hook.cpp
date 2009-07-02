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
#ifndef __linux__
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <winsock.h>

#endif
#include <mysql.h>

#ifdef __linux__
#include <dlfcn.h>
#else
typedef unsigned long DWORD;
#define PVFN2( classptr , offset ) ((*(DWORD*) classptr ) + offset)
#define VFN2( classptr , offset ) *(DWORD*)PVFN2( classptr , offset )
#endif
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "shake.h" 
#include "mrecipientfilter.h" 
//#include "enginecallback.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "bitbuf.h"
#include "icvar.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "itempents.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_mainclass.h"
#include "mani_hook.h"
#include "mani_voice.h"
#include "mani_spawnpoints.h"
#include "mani_sprayremove.h"
#include "mani_gametype.h"
#include "mani_globals.h"

#ifndef SOURCEMM
DEFVFUNC_(voiceserver_SetClientListening, bool, (IVoiceServer* VoiceServer, int iReceiver, int iSender, bool bListen));
bool VFUNC mysetclientlistening(IVoiceServer* VoiceServer, int iReceiver, int iSender, bool bListen)
{
	bool new_listen;

	if (!g_PluginLoaded)
	{
		return voiceserver_SetClientListening(VoiceServer, iReceiver, iSender, bListen);
	}

	if (ProcessDeadAllTalk(iReceiver, iSender, &new_listen))
	{
		return voiceserver_SetClientListening(VoiceServer, iReceiver, iSender, new_listen);
	}
	else
	{
		return voiceserver_SetClientListening(VoiceServer, iReceiver, iSender, bListen);
	}
}


DEFVFUNC_(org_LevelInit, bool, (IServerGameDLL* pServerGameDLL, char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background));

bool VFUNC myLevelInit(IServerGameDLL *pServerGameDLL, char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	if (!g_PluginLoaded)
	{
		return (org_LevelInit(pServerGameDLL, pMapName, pMapEntities, pOldLevel, pLandmarkName, loadGame, background));
	}

	char	*pReplaceEnts = NULL;

	// Copy the map entities
	if (!gpManiSpawnPoints->AddSpawnPoints(&pReplaceEnts, pMapEntities))
	{
		return (org_LevelInit(pServerGameDLL, pMapName, pMapEntities, pOldLevel, pLandmarkName, loadGame, background));
	}

	bool result = org_LevelInit(pServerGameDLL, pMapName, pReplaceEnts, pOldLevel, pLandmarkName, loadGame, background);
	free(pReplaceEnts);
	return result;
}

DEFVFUNC_(te_PlayerDecal, void, (ITempEntsSystem *pTESys, IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity));

void VFUNC myplayerdecal(ITempEntsSystem *pTESys, IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity)
{
	if (!g_PluginLoaded)
	{
		te_PlayerDecal(pTESys, filter, delay, pos, player, entity);
		return;
	}

	if (gpManiSprayRemove->SprayFired(pos, player))
	{
		// We let this one through.
		te_PlayerDecal(pTESys, filter, delay, pos, player, entity);
	}
}

void	HookVFuncs(void)
{
	if (voiceserver && gpManiGameType->IsVoiceAllowed() && !g_PluginLoadedOnce)
	{
		Msg("Hooking voiceserver\n");
		HOOKVFUNC(voiceserver, gpManiGameType->GetVoiceOffset(), voiceserver_SetClientListening, mysetclientlistening);
	}

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed() && !g_PluginLoadedOnce)
	{
		Msg("Hooking decals\n");
		HOOKVFUNC(temp_ents, gpManiGameType->GetSprayHookOffset(), te_PlayerDecal, myplayerdecal);
	}

	if (!g_PluginLoadedOnce && gpManiGameType->IsSpawnPointHookAllowed())
	{
		Msg("Hooking spawnpoints\n");
		HOOKVFUNC(serverdll, gpManiGameType->GetSpawnPointHookOffset(), org_LevelInit, myLevelInit);
	}
}

#endif

