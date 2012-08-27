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


#ifndef MANI_SOURCE_HOOK_H
#define MANI_SOURCE_HOOK_H

#ifndef SOURCEMM

#include "cbaseentity.h"

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook_impl.h>
#include <igameevents.h>

#include "mani_main.h"

// This file is used for backwards compatibility testing
// It allows us to test binary backwards compatibility by using an older include file HERE:
//#include "sourcehook.h"			// <-- here
// There. main.cpp which implements all of the following function is always usign sourcehook.h
// and the up-to-date sourcehook_impl.h/sourcehook.cpp. The tests use this file however.
// If the test needs an up-to-date version (like the recall test), it can simply
// #include "sourcehook.h" before including this, thus overriding our decision.

class ManiSMMHooks
{
public:
	void	HookVFuncs(void);
	bool	LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background);
	bool	SetClientListening(int iReceiver, int iSender, bool bListen);
	void	PlayerDecal(IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity);
	void	HookProcessUsercmds(CBasePlayer *pPlayer);
	void	ProcessUsercmds(CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused);
	void	UnHookProcessUsercmds(CBasePlayer *pPlayer);
	void	HookWeapon_CanUse(CBasePlayer *pPlayer);
	bool	Weapon_CanUse(CBaseCombatWeapon *pWeapon);
	void	UnHookWeapon_CanUse(CBasePlayer *pPlayer);
	void	HookConCommands();

#if defined ( GAME_CSGO )
	bf_write *UserMessageBegin (IRecipientFilter *filter, int msg_type, const char * msg);
#elif defined ( GAME_ORANGE )
	bf_write *UserMessageBegin (IRecipientFilter *filter, int msg_type);
#endif

};

extern ManiSMMHooks g_ManiSMMHooks;
extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;

#endif

#endif
