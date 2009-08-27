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

#ifndef ORANGE
// SH_STATIC was introduced in SH 5.0
#define SH_STATIC(func) fastdelegate::MakeDelegate(func)
#endif

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
#include "iplayerinfo.h"
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

#if defined ( ORANGE )
SourceHook::Impl::CSourceHookImpl g_SourceHook;
#else
SourceHook::CSourceHookImpl g_SourceHook;
#endif

char	*pReplaceEnts = NULL;
extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;
SourceHook::ISourceHook *g_SHPtr = &g_SourceHook;
int g_PLID = 1337;

ManiSMMHooks g_ManiSMMHooks;
//GET_SHPTR(g_SHPtr);

ConCommand *pSayCmd = NULL;
ConCommand *pTeamSayCmd = NULL;
ConCommand *pChangeLevelCmd = NULL;
ConCommand *pAutoBuyCmd = NULL;
ConCommand *pReBuyCmd = NULL;
ConCommand *pRespawnEntities = NULL;

// Hook declarations
SH_DECL_HOOK7(IServerGameClients, ProcessUsercmds, SH_NOATTRIB, 0, float, edict_t *, bf_read *, int , int , int , bool , bool );
SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK5_void(ITempEntsSystem, PlayerDecal, SH_NOATTRIB, 0, IRecipientFilter &, float , const Vector* , int , int );
SH_DECL_MANUALHOOK5_void(Player_ProcessUsercmds, 0, 0, 0, CUserCmd *, int, int, int, bool);
SH_DECL_MANUALHOOK1(Player_Weapon_CanUse, 0, 0, 0, bool, CBaseCombatWeapon *);
#ifdef ORANGE
SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, 0, const CCommand &);
#else
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);
#endif

static	bool hooked = false;

void	ManiSMMHooks::HookVFuncs(void)
{
	if (hooked) return;

	if (voiceserver && gpManiGameType->IsVoiceAllowed())
	{
		//MMsg("Hooking voiceserver\n");
		SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_ManiSMMHooks, &ManiSMMHooks::SetClientListening, true);
	}

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{
		//MMsg("Hooking decals\n");
		SH_ADD_HOOK_MEMFUNC(ITempEntsSystem, PlayerDecal, temp_ents, &g_ManiSMMHooks, &ManiSMMHooks::PlayerDecal, false);
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

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		int offset = gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_CANUSE);
		if (offset != -1)
		{
			SH_MANUALHOOK_RECONFIGURE(Player_Weapon_CanUse, offset, 0, 0);
		}
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
		return_value = SH_CALL(SH_GET_CALLCLASS(voiceserver), &IVoiceServer::SetClientListening)(iReceiver, iSender, new_listen);
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

void	ManiSMMHooks::HookWeapon_CanUse(CBasePlayer *pPlayer)
{
	SH_ADD_MANUALHOOK_MEMFUNC(Player_Weapon_CanUse, pPlayer, &g_ManiSMMHooks, &ManiSMMHooks::Weapon_CanUse, false);
}

bool	ManiSMMHooks::Weapon_CanUse(CBaseCombatWeapon *pWeapon)
{
	if (!gpManiWeaponMgr->CanPickUpWeapon(META_IFACEPTR(CBasePlayer), pWeapon))
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void	ManiSMMHooks::UnHookWeapon_CanUse(CBasePlayer *pPlayer)
{
	SH_REMOVE_MANUALHOOK_MEMFUNC(Player_Weapon_CanUse, pPlayer, &g_ManiSMMHooks, &ManiSMMHooks::Weapon_CanUse, false);
}


#ifdef ORANGE
void RespawnEntities_handler(const CCommand &command)
#else
void RespawnEntities_handler()
#endif
{
	//Override exploit
	RETURN_META(MRES_SUPERCEDE);
} 

#ifdef ORANGE
void Say_handler(const CCommand &command)
{
#else
void Say_handler()
{
	CCommand command;
#endif
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);

	if (!g_ManiAdminPlugin.HookSayCommand(false, command))
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
} 

#ifdef ORANGE
void TeamSay_handler(const CCommand &command)
{
#else
void TeamSay_handler()
{
	CCommand command;
#endif
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);

	if(!g_ManiAdminPlugin.HookSayCommand(true, command))
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
} 

#ifdef ORANGE
void ChangeLevel_handler(const CCommand &command)
#else
void ChangeLevel_handler()
#endif
{
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);

	if(!g_ManiAdminPlugin.HookChangeLevelCommand())
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
} 

#ifdef ORANGE
void AutoBuy_handler(const CCommand &command)
#else
void AutoBuy_handler()
#endif
{
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);
	gpManiWeaponMgr->PreAutoBuyReBuy();
#if defined ( ORANGE )
	SH_CALL(pAutoBuyCmd, &ConCommand::Dispatch)(command);
#else
	SH_CALL(SH_GET_CALLCLASS(pAutoBuyCmd), &ConCommand::Dispatch)();
#endif
	gpManiWeaponMgr->AutoBuyReBuy();
	RETURN_META(MRES_SUPERCEDE);
} 

#ifdef ORANGE
void ReBuy_handler(const CCommand &command)
#else
void ReBuy_handler()
#endif
{
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);
	gpManiWeaponMgr->PreAutoBuyReBuy();
#ifdef ORANGE 
	SH_CALL(pReBuyCmd, &ConCommand::Dispatch)(command);
#else
	SH_CALL(SH_GET_CALLCLASS(pReBuyCmd), &ConCommand::Dispatch)();
#endif
	gpManiWeaponMgr->AutoBuyReBuy();
	RETURN_META(MRES_SUPERCEDE);
} 

void	ManiSMMHooks::HookConCommands()
{
	//find the commands in the server's CVAR list
	ConCommandBase *pCmd = g_pCVar->GetCommands();
	while (pCmd)
	{
		if (pCmd->IsCommand())
		{
			if (strcmp(pCmd->GetName(), "say") == 0)
				pSayCmd = static_cast<ConCommand *>(pCmd);
			else if (strcmp(pCmd->GetName(), "say_team") == 0)
				pTeamSayCmd = static_cast<ConCommand *>(pCmd);
			else if (strcmp(pCmd->GetName(), "changelevel") == 0)
				pChangeLevelCmd = static_cast<ConCommand *>(pCmd);
			else if (strcmp(pCmd->GetName(), "autobuy") == 0)
				pAutoBuyCmd = static_cast<ConCommand *>(pCmd);
			else if (strcmp(pCmd->GetName(), "rebuy") == 0)
				pReBuyCmd = static_cast<ConCommand *>(pCmd);
			else if (strcmp(pCmd->GetName(), "respawn_entities") == 0)
				pRespawnEntities = static_cast<ConCommand *>(pCmd);
		}

		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}

	if (pSayCmd) SH_ADD_HOOK(ConCommand, Dispatch, pSayCmd, SH_STATIC(Say_handler), false);
	if (pRespawnEntities) SH_ADD_HOOK(ConCommand, Dispatch, pRespawnEntities, SH_STATIC(RespawnEntities_handler), false);
	if (pTeamSayCmd) SH_ADD_HOOK(ConCommand, Dispatch, pTeamSayCmd, SH_STATIC(TeamSay_handler), false);
	if (pChangeLevelCmd) SH_ADD_HOOK(ConCommand, Dispatch, pChangeLevelCmd, SH_STATIC(ChangeLevel_handler), false);
	if (pAutoBuyCmd) SH_ADD_HOOK(ConCommand, Dispatch, pAutoBuyCmd, SH_STATIC(AutoBuy_handler), false);
	if (pReBuyCmd) SH_ADD_HOOK(ConCommand, Dispatch, pReBuyCmd, SH_STATIC(ReBuy_handler), false);
}


#endif
