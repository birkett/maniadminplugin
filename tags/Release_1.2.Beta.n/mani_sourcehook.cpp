/* The following code uses SourceHook code for mangaging 
*  Virtual Function hooking, credits below. No modifcations
*  were made to the core SourceHook code, this plugin 
*  only uses the SourceHook interface
*/


/* ======== SourceHook ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Pavol "PM OnoTo" Marko
* Contributors: Scott "Damaged Soul" Ehlert
* ============================
*/

#ifndef SOURCEMM

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

//#include <oslink.h>
#include "mani_sourcehook.h"
#include "cvars.h"
#include "meta_hooks.h"
#include "mani_mainclass.h"
#include "mani_gametype.h"
#include "mani_sprayremove.h"
#include "mani_spawnpoints.h"
#include "mani_output.h"
#include "mani_voice.h"
#include "mani_globals.h"
#include "mani_weapon.h"
#include "mani_afk.h"
#include "cbaseentity.h"
#include "meta_hooks.h"

#include "mani_main.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// 
// The plugin is a static singleton that is exported as an interface
//
// Don't forget to make an instance

SourceHook::ISourceHook *g_SHPtr;
SourceHook::Plugin g_PLID;
char	*pReplaceEnts = NULL;

ManiSMMHooks g_ManiSMMHooks;
//GET_SHPTR(g_SHPtr);

SourceHook::ISourceHook *SourceHook_Factory()
{
	return new SourceHook::CSourceHookImpl();
}

void SourceHook_Delete(SourceHook::ISourceHook *shptr)
{
	delete static_cast<SourceHook::CSourceHookImpl *>(shptr);
}

void SourceHook_CompleteShutdown(SourceHook::ISourceHook *shptr)
{
	static_cast<SourceHook::CSourceHookImpl *>(shptr)->CompleteShutdown();
}

bool SourceHook_IsPluginInUse(SourceHook::ISourceHook *shptr, SourceHook::Plugin plug)
{
	return static_cast<SourceHook::CSourceHookImpl *>(shptr)->IsPluginInUse(plug);
}

void SourceHook_UnloadPlugin(SourceHook::ISourceHook *shptr, SourceHook::Plugin plug)
{
	static_cast<SourceHook::CSourceHookImpl *>(shptr)->UnloadPlugin(plug);
}

void SourceHook_PausePlugin(SourceHook::ISourceHook *shptr, SourceHook::Plugin plug)
{
	static_cast<SourceHook::CSourceHookImpl *>(shptr)->PausePlugin(plug);
}

void SourceHook_UnpausePlugin(SourceHook::ISourceHook *shptr, SourceHook::Plugin plug)
{
	static_cast<SourceHook::CSourceHookImpl *>(shptr)->UnpausePlugin(plug);
}

void SourceHook_InitSourceHook(void)
{
	g_SHPtr = SourceHook_Factory();
	g_PLID = 1337;
}

// Hook declarations
SH_DECL_HOOK7(IServerGameClients, ProcessUsercmds, SH_NOATTRIB, 0, float, edict_t *, bf_read *, int , int , int , bool , bool );
SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK5_void(ITempEntsSystem, PlayerDecal, SH_NOATTRIB, 0, IRecipientFilter &, float , const Vector* , int , int );
SH_DECL_MANUALHOOK5_void(Player_ProcessUsercmds, 0, 0, 0, CUserCmd *, int, int, int, bool);

static	bool hooked = false;

void	ManiSMMHooks::HookVFuncs(void)
{
	if (hooked) return;

	voiceserver_cc = SH_GET_CALLCLASS(voiceserver);

	if (voiceserver && gpManiGameType->IsVoiceAllowed())
	{
		//MMsg("Hooking voiceserver\n");
		SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_ManiSMMHooks, &ManiSMMHooks::SetClientListening, true);
	}

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{
		//MMsg("Hooking decals\n");
		SH_ADD_HOOK_MEMFUNC(ITempEntsSystem, PlayerDecal, temp_ents, &g_ManiSMMHooks, &ManiSMMHooks::PlayerDecal, true);
	}

	int offset = gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS);
	if (offset != -1)
	{
		SH_MANUALHOOK_RECONFIGURE(Player_ProcessUsercmds, offset, 0, 0);
	}

	if (serverdll)
	{
		SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, serverdll, &g_ManiSMMHooks, &ManiSMMHooks::LevelInit, false);
	}

	hooked = true;
}


bool ManiSMMHooks::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{
	if (!gpManiGameType->IsSpawnPointHookAllowed())
	{
		RETURN_META_VALUE(MRES_IGNORED, true);
	}

	// Do the spawnpoints hook control if on SourceMM
	// Copy the map entities
	if (!gpManiSpawnPoints->AddSpawnPoints(&pReplaceEnts, pMapEntities))
	{
		RETURN_META_VALUE(MRES_IGNORED, true);
	}

	RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, true, &IServerGameDLL::LevelInit, (pMapName, pReplaceEnts, pOldLevel, pLandmarkName, loadGame, background));
}

bool	ManiSMMHooks::SetClientListening(int iReceiver, int iSender, bool bListen)
{
	bool new_listen;
	bool return_value = true;

	if (ProcessDeadAllTalk(iReceiver, iSender, &new_listen))
	{
		return_value = SH_CALL(voiceserver_cc, &IVoiceServer::SetClientListening)(iReceiver, iSender, new_listen);
		RETURN_META_VALUE(MRES_SUPERCEDE, return_value);
	}

	RETURN_META_VALUE(MRES_IGNORED, return_value);
}

void	ManiSMMHooks::PlayerDecal(IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity)
{
	if (gpManiSprayRemove->SprayFired(pos, player))
	{
		// We let this one through.
		RETURN_META(MRES_IGNORED);
	}

	RETURN_META(MRES_SUPERCEDE);
}

void	ManiSMMHooks::HookProcessUsercmds(CBasePlayer *pPlayer)
{
	SH_ADD_MANUALHOOK_MEMFUNC(Player_ProcessUsercmds, pPlayer, &g_ManiSMMHooks, &ManiSMMHooks::ProcessUsercmds, false);
}

void	ManiSMMHooks::ProcessUsercmds(CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused)
{
	gpManiAFK->ProcessUsercmds(META_IFACEPTR(CBasePlayer), cmds, numcmds);
	RETURN_META(MRES_IGNORED);
}

void	ManiSMMHooks::UnHookProcessUsercmds(CBasePlayer *pPlayer)
{
	SH_REMOVE_MANUALHOOK_MEMFUNC(Player_ProcessUsercmds, pPlayer, &g_ManiSMMHooks, &ManiSMMHooks::ProcessUsercmds, false);
}
#endif