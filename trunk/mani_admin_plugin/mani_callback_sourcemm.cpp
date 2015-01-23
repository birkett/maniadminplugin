//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
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



#ifdef SOURCEMM

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

#ifdef GAME_ORANGE
#include <metamod_oslink.h>
#else
#include <oslink.h>
#endif

#include "mani_callback_sourcemm.h"
//#include "cvars.h"
#include "meta_hooks.h"
#include "mani_mainclass.h"
#include "mani_gametype.h"
#include "mani_sprayremove.h"
#include "mani_spawnpoints.h"
#include "mani_output.h"
#include "mani_voice.h"
#include "mani_client_interface.h"
#include "mani_globals.h"
#include "mani_weapon.h"
#include "mani_afk.h"
#include "mani_util.h"
#include "cbaseentity.h"

#define	FIND_IFACE(func, assn_var, num_var, name, type) \
	do { \
		if ( (assn_var=(type)((ismm->func())(name, NULL))) != NULL ) { \
			num = 0; \
			break; \
		} \
		if (num >= 999) \
			break; \
	} while ( num_var=ismm->FormatIface(name, sizeof(name)-1) ); \
	if (!assn_var) { \
		if (error) \
			snprintf(error, maxlen, "Could not find interface %s", name); \
		return false; \
	}

#define GET_V_IFACE(v_factory, v_var, v_type, v_name) \
	v_var = (v_type *)ismm->VInterfaceMatch(ismm->v_factory(), v_name); \
	if (!v_var) \
	{ \
		v_var = (v_type *)ismm->VInterfaceMatch(ismm->v_factory(), v_name, 0); \
		if (!v_var) \
		{ \
			if (error && maxlen) \
			{ \
				snprintf(error, maxlen, "Could not find interface: %s", v_name); \
			} \
			return false; \
		} \
	}

#include "mani_main.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
// 
// The plugin is a static singleton that is exported as an interface
//
// Don't forget to make an instance

extern unsigned int g_CallBackCount;
extern SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
extern ClientInterface g_ClientInterface;


CSourceMMMAP g_ManiCallback;
PLUGIN_EXPOSE(CSourceMMMAP, g_ManiCallback);

char	*pReplaceEnts = NULL;
extern int	max_players;

MyListener g_Listener;

ConCommand *pSayCmd = NULL;
ConCommand *pTeamSayCmd = NULL;
ConCommand *pChangeLevelCmd = NULL;
ConCommand *pAutoBuyCmd = NULL;
ConCommand *pReBuyCmd = NULL;
ConCommand *pRespawnEntities = NULL;

static int offset1 = -1;

//#if !defined GAME_ORANGE
//ICvar *g_pCVar = NULL;
//#endif

ICvar *GetICVar()
{
#if defined METAMOD_PLAPI_VERSION
#if defined GAME_ORANGE
	return (ICvar *)((g_SMAPI->GetEngineFactory())(CVAR_INTERFACE_VERSION, NULL));
#else
	return (ICvar *)((g_SMAPI->GetEngineFactory())(VENGINE_CVAR_INTERFACE_VERSION, NULL));
#endif
#else
	return (ICvar *)((g_SMAPI->engineFactory())(VENGINE_CVAR_INTERFACE_VERSION, NULL));
#endif
}


//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CSourceMMMAPCallback::CSourceMMMAPCallback()
{
	//m_iClientCommandIndex = 0;
	gpManiISPCCallback = this;
}

CSourceMMMAPCallback::~CSourceMMMAPCallback()
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourceMMMAPCallback::SetCommandClient( int index )
{
	m_iClientCommandIndex = con_command_index = index;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
#ifdef GAME_ORANGE 
PLUGIN_RESULT CSourceMMMAPCallback::ClientCommand( edict_t *pEntity, const CCommand &args)
{
	return (gpManiAdminPlugin->ClientCommand(pEntity, args));
}
#else
PLUGIN_RESULT CSourceMMMAPCallback::ClientCommand( edict_t *pEntity )
{
	return (gpManiAdminPlugin->ClientCommand(pEntity));
}
#endif

CSourceMMMAPCallback g_HelperCallback;

bool CSourceMMMAP::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{
//	META_LOG(g_PLAPI, "LevelInit() called: pMapName=%s", pMapName); 
	gpManiAdminPlugin->LevelInit(pMapName);

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

void CSourceMMMAP::OnLevelShutdown()
{
//	META_LOG(g_PLAPI, "OnLevelShutdown() called from listener");
}

void CSourceMMMAP::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
//	META_LOG(g_PLAPI, "ServerActivate() called: edictCount=%d, clientMax=%d", edictCount, clientMax);
	gpManiAdminPlugin->ServerActivate(pEdictList, edictCount, clientMax);
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::GameFrame(bool simulating)
{
	//don't log this, it just pumps stuff to the screen ;]
	//META_LOG(g_PLAPI, "GameFrame() called: simulating=%d", simulating);
	gpManiAdminPlugin->GameFrame(simulating);
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::LevelShutdown( void )
{
//	META_LOG(g_PLAPI, "LevelShutdown() called");
	gpManiAdminPlugin->LevelShutdown();
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::ClientActive(edict_t *pEntity, bool bLoadGame)
{
//	META_LOG(g_PLAPI, "ClientActive called: pEntity=%d", pEntity ? IndexOfEdict(pEntity) : 0);
	gpManiAdminPlugin->ClientActive(pEntity);
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::ClientDisconnect(edict_t *pEntity)
{
//	META_LOG(g_PLAPI, "ClientDisconnect called: pEntity=%d", pEntity ? IndexOfEdict(pEntity) : 0);
	gpManiAdminPlugin->ClientDisconnect(pEntity);
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::ClientPutInServer(edict_t *pEntity, char const *playername)
{
//	META_LOG(g_PLAPI, "ClientPutInServer called: pEntity=%d, playername=%s", pEntity ? IndexOfEdict(pEntity) : 0, playername);
	gpManiAdminPlugin->ClientPutInServer(pEntity, playername);
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::SetCommandClient(int index)
{
//	META_LOG(g_PLAPI, "SetCommandClient() called: index=%d", index);
	m_iClientCommandIndex = con_command_index = index;
	RETURN_META(MRES_IGNORED);
}

void CSourceMMMAP::ClientSettingsChanged(edict_t *pEdict)
{
//	META_LOG(g_PLAPI, "ClientSettingsChanged called: pEdict=%d", pEdict ? IndexOfEdict(pEdict) : 0);
	gpManiAdminPlugin->ClientSettingsChanged(pEdict);
	RETURN_META(MRES_IGNORED);
}

bool CSourceMMMAP::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
//	META_LOG(g_PLAPI, "ClientConnect called: pEntity=%d, pszName=%s, pszAddress=%s", pEntity ? IndexOfEdict(pEntity) : 0, pszName, pszAddress);

	bool allow_connect = true;
	gpManiAdminPlugin->ClientConnect(&allow_connect, pEntity, pszName, pszAddress, reject, maxrejectlen);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

#ifdef GAME_ORANGE
void CSourceMMMAP::ClientCommand(edict_t *pEntity, const CCommand &args)
{
	int result = gpManiAdminPlugin->ClientCommand(pEntity, args);
#else
void CSourceMMMAP::ClientCommand(edict_t *pEntity)
{
	int result = gpManiAdminPlugin->ClientCommand(pEntity);
#endif

	if (result == PLUGIN_CONTINUE)
	{
		// Plugin continue
		RETURN_META(MRES_IGNORED);
	}
	else
	{
		// Plugin stop
		RETURN_META(MRES_SUPERCEDE);
	}
}

bool CSourceMMMAP::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	//char iface_buffer[255];
	//int num = 0;
#ifdef GAME_ORANGE
	GET_V_IFACE(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE(GetEngineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE(GetEngineFactory, networkstringtable, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE(GetEngineFactory, randomStr, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	GET_V_IFACE(GetServerFactory, serverents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE(GetServerFactory, effects, IEffects, IEFFECTS_INTERFACE_VERSION);
	GET_V_IFACE(GetEngineFactory, esounds, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE(GetServerFactory, serverdll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE(GetEngineFactory, voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER);
	GET_V_IFACE(GetServerFactory, serverclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	g_pCVar = GetICVar();
#else
	GET_V_IFACE(serverFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE(engineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE(engineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE(engineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE(engineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE(engineFactory, networkstringtable, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE(engineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE(engineFactory, randomStr, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	GET_V_IFACE(serverFactory, serverents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE(serverFactory, effects, IEffects, IEFFECTS_INTERFACE_VERSION);
	GET_V_IFACE(engineFactory, esounds, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE(engineFactory, g_pCVar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
	GET_V_IFACE(serverFactory, serverdll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE(engineFactory, voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER);
	GET_V_IFACE(serverFactory, serverclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
#endif
	ConVar *testload = g_pCVar->FindVar("mani_admin_plugin_version");
	if ( testload ) {
		MMsg( "Error:  Version %s of Mani Admin Plugin is already loaded.\n", testload->GetString() );
		return false;
	}

	META_LOG(g_PLAPI, "Starting plugin.\n");

	ismm->AddListener(this, &g_Listener);

	//Init our cvars/concmds
#if defined GAME_ORANGE
	ConVar_Register(0, this);
#else
	ConCommandBaseMgr::OneTimeInit(this);
#endif
	//We're hooking the following things as POST, in order to seem like Server Plugins.
	//However, I don't actually know if Valve has done server plugins as POST or not.
	//Change the last parameter to 'false' in order to change this to PRE.
	//SH_ADD_HOOK_MEMFUNC means "SourceHook, Add Hook, Member Function".

	//Hook LevelInit to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, serverdll, &g_ManiCallback, &CSourceMMMAP::LevelInit, false);
	//Hook ServerActivate to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, serverdll, &g_ManiCallback, &CSourceMMMAP::ServerActivate, true);
	//Hook GameFrame to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, serverdll, &g_ManiCallback, &CSourceMMMAP::GameFrame, true);
	//Hook LevelShutdown to our function -- this makes more sense as pre I guess
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, serverdll, &g_ManiCallback, &CSourceMMMAP::LevelShutdown, false);
	//Hook ClientActivate to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientActive, false);
	//Hook ClientDisconnect to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientDisconnect, false);
	//Hook ClientPutInServer to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientPutInServer, true);
	//Hook SetCommandClient to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverclients, &g_ManiCallback, &CSourceMMMAP::SetCommandClient, true);
	//Hook ClientSettingsChanged to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientSettingsChanged, true);

	//The following functions are pre handled, because that's how they are in IServerPluginCallbacks
	//Hook ClientConnect to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientConnect, false);
	//Hook ClientCommand to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientCommand, false);

	//This hook is a static hook, no member function
	//SH_ADD_HOOK_STATICFUNC(IGameEventManager2, FireEvent, gameeventmanager, FireEvent_Handler, false); 
#if !defined GAME_ORANGE && defined SOURCEMM
	//Get the call class for IVServerEngine so we can safely call functions without
	// invoking their hooks (when needed).
	engine_cc = SH_GET_CALLCLASS(engine);
	voiceserver_cc = SH_GET_CALLCLASS(voiceserver);
	serverdll_cc = SH_GET_CALLCLASS(serverdll);
#endif


#ifdef GAME_ORANGE 
	gamedll = g_SMAPI->GetServerFactory(false);
#else
	gamedll = g_SMAPI->serverFactory(false);
#endif

	g_SMAPI->AddListener(g_PLAPI, this);

#ifdef GAME_ORANGE 
	gpGlobals = g_SMAPI->GetCGlobals();
#else
	gpGlobals = g_SMAPI->pGlobals();
#endif

	FindConPrintf();

	MMsg("********************************************************\n");
	MMsg(" Loading ");
	MMsg("%s\n", mani_version);
	MMsg("\n");

	if (!UTIL_InterfaceMsg(playerinfomanager,"IPlayerInfoManager", INTERFACEVERSION_PLAYERINFOMANAGER)) return false;
	if (!UTIL_InterfaceMsg(engine,"IVEngineServer", INTERFACEVERSION_VENGINESERVER)) return false;
	if (!UTIL_InterfaceMsg(gameeventmanager,"IGameEventManager2", INTERFACEVERSION_GAMEEVENTSMANAGER2)) return false;
	if (!UTIL_InterfaceMsg(filesystem,"IFileSystem", FILESYSTEM_INTERFACE_VERSION)) return false;
	if (!UTIL_InterfaceMsg(helpers,"IServerPluginHelpers", INTERFACEVERSION_ISERVERPLUGINHELPERS)) return false;
	if (!UTIL_InterfaceMsg(networkstringtable,"INetworkStringTableContainer", INTERFACENAME_NETWORKSTRINGTABLESERVER)) return false;
	if (!UTIL_InterfaceMsg(enginetrace,"IEngineTrace", INTERFACEVERSION_ENGINETRACE_SERVER)) return false;
	if (!UTIL_InterfaceMsg(randomStr,"IUniformRandomStream", VENGINE_SERVER_RANDOM_INTERFACE_VERSION)) return false;
	if (!UTIL_InterfaceMsg(serverents,"IServerGameEnts", INTERFACEVERSION_SERVERGAMEENTS)) return false;
	if (!UTIL_InterfaceMsg(effects,"IEffects", IEFFECTS_INTERFACE_VERSION)) return false;
	if (!UTIL_InterfaceMsg(esounds,"IEngineSound", IENGINESOUND_SERVER_INTERFACE_VERSION)) return false;

#ifdef GAME_ORANGE 
	if (!UTIL_InterfaceMsg(g_pCVar,"ICvar", CVAR_INTERFACE_VERSION)) return false;
#else
	if (!UTIL_InterfaceMsg(g_pCVar,"ICvar", VENGINE_CVAR_INTERFACE_VERSION)) return false;
#endif

	if (!UTIL_InterfaceMsg(serverdll,"IServerGameDLL", "ServerGameDLL003")) return false;
	if (!UTIL_InterfaceMsg(voiceserver,"IVoiceServer", INTERFACEVERSION_VOICESERVER)) return false;
	//if (!UTIL_InterfaceMsg(partition,"ISpatialPartition", INTERFACEVERSION_SPATIALPARTITION)) return false;

	MMsg("********************************************************\n");

	// max players = 0 on first load, > 0 on late load
	max_players = gpGlobals->maxClients;
	if (late)
	{
		HookConCommands();
	}

	gpManiAdminPlugin->Load();

	return true;
}

bool CSourceMMMAP::Unload(char *error, size_t maxlen)
{
#if defined ( GAME_CSGO )
	ConCommand *remove_cheat_on_bot_kill = static_cast<ConCommand *>(g_pCVar->FindCommand("bot_kill"));

	if (remove_cheat_on_bot_kill)
	{
		if (!(remove_cheat_on_bot_kill->GetFlags() & FCVAR_CHEAT))
			remove_cheat_on_bot_kill->AddFlags(FCVAR_CHEAT);
	}
#endif

	if(g_CallBackCount > 0)
	{
		for(unsigned int i=0;i<g_CallBackCount;i++)
		{
			AdminInterfaceListner *ptr = (AdminInterfaceListner *)g_CallBackList[i]->ptr;
			if(!ptr)
				continue;

			ptr->OnAdminInterfaceUnload();
		}
	}

	gpManiAdminPlugin->Unload();
	//IT IS CRUCIAL THAT YOU REMOVE CVARS.
	//As of Metamod:Source 1.00-RC2, it will automatically remove them for you.
	//But this is only if you've registered them correctly!
    
	//Make sure we remove any hooks we did... this may not be necessary since
	//SourceHook is capable of unloading plugins' hooks itself, but just to be safe.

//	SH_REMOVE_HOOK_STATICFUNC(IGameEventManager2, FireEvent, gameeventmanager, FireEvent_Handler, false); 
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, serverdll, &g_ManiCallback, &CSourceMMMAP::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, serverdll, &g_ManiCallback, &CSourceMMMAP::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, serverdll, &g_ManiCallback, &CSourceMMMAP::GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, serverdll, &g_ManiCallback, &CSourceMMMAP::LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientDisconnect, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverclients, &g_ManiCallback, &CSourceMMMAP::SetCommandClient, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientSettingsChanged, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverclients, &g_ManiCallback, &CSourceMMMAP::ClientCommand, false);

	if (voiceserver && gpManiGameType->IsVoiceAllowed())
	{
		SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_ManiSMMHooks, &ManiSMMHooks::SetClientListening, false);
	}

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{
		SH_REMOVE_HOOK_MEMFUNC(ITempEntsSystem, PlayerDecal, temp_ents, &g_ManiSMMHooks, &ManiSMMHooks::PlayerDecal, false);
	}

	if (pSayCmd) SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pSayCmd, Say_handler, false);
	if (pRespawnEntities) SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pRespawnEntities, RespawnEntities_handler, false);
	if (pTeamSayCmd) SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pTeamSayCmd, TeamSay_handler, false);
	if (pChangeLevelCmd) SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pChangeLevelCmd, ChangeLevel_handler, false);
	if (pAutoBuyCmd) 
	{
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pAutoBuyCmd, AutoBuy_handler, false);
#if !defined GAME_ORANGE && defined SOURCEMM
		SH_RELEASE_CALLCLASS(autobuy_cc);
#endif
	}

	if (pReBuyCmd) 
	{
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, pReBuyCmd, ReBuy_handler, false);
#if !defined GAME_ORANGE && defined SOURCEMM
		SH_RELEASE_CALLCLASS(rebuy_cc);
#endif
	}

#if !defined GAME_ORANGE && defined SOURCEMM
	if (gpManiGameType->GetAdvancedEffectsAllowed())
	{
		SH_RELEASE_CALLCLASS(temp_ents_cc);
	}
	//this, sourcehook does not keep track of.  we must do this.
	SH_RELEASE_CALLCLASS(engine_cc);
	SH_RELEASE_CALLCLASS(voiceserver_cc);
	SH_RELEASE_CALLCLASS(serverdll_cc);
#endif
	//this, sourcehook does not keep track of.  we must do this.
#if defined ( GAME_ORANGE )
	ConVar_Unregister(); // probably not needed, but do it just in case.
#endif
	return true; 
}

void CSourceMMMAP::AllPluginsLoaded()
{
	//we don't really need this for anything other than interplugin communication
	//and that's not used in this plugin.
	//If we really wanted, we could override the factories so other plugins can request
	// interfaces we make.  In this callback, the plugin could be assured that either
	// the interfaces it requires were either loaded in another plugin or not.
	HookConCommands();
}

bool CSourceMMMAP::RegisterConCommandBase(ConCommandBase *pVar)
{
	//this will work on any type of concmd!
	return META_REGCVAR(pVar);
}

void CSourceMMMAP::HookConCommands()
{
	//find the commands in the server's CVAR list
#if defined (GAME_CSGO)
	pSayCmd = static_cast<ConCommand *>(g_pCVar->FindCommand("say"));
	pTeamSayCmd = static_cast<ConCommand *>(g_pCVar->FindCommand("say_team"));
	pChangeLevelCmd = static_cast<ConCommand *>(g_pCVar->FindCommand("changelevel"));
	pAutoBuyCmd = static_cast<ConCommand *>(g_pCVar->FindCommand("autobuy"));
	pReBuyCmd = static_cast<ConCommand *>(g_pCVar->FindCommand("rebuy"));
	pRespawnEntities = static_cast<ConCommand *>(g_pCVar->FindCommand("respawn_entities"));
#else
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
#endif 

#if !defined GAME_ORANGE && defined SOURCEMM
	if (pSayCmd) SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pSayCmd, Say_handler, false);
	if (pRespawnEntities) SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pRespawnEntities, RespawnEntities_handler, false);
	if (pTeamSayCmd) SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pTeamSayCmd, TeamSay_handler, false);
	if (pChangeLevelCmd) SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pChangeLevelCmd, ChangeLevel_handler, false);
	if (pAutoBuyCmd) 
	{
		SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pAutoBuyCmd, AutoBuy_handler, false);
		autobuy_cc = SH_GET_CALLCLASS(pAutoBuyCmd);
	}

	if (pReBuyCmd) 
	{
		SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pReBuyCmd, ReBuy_handler, false);
		rebuy_cc = SH_GET_CALLCLASS(pReBuyCmd);
	}
#else
	if (pSayCmd) SH_ADD_HOOK(ConCommand, Dispatch, pSayCmd, SH_STATIC(Say_handler), false);
	if (pRespawnEntities) SH_ADD_HOOK(ConCommand, Dispatch, pRespawnEntities, SH_STATIC(RespawnEntities_handler), false);
	if (pTeamSayCmd) SH_ADD_HOOK(ConCommand, Dispatch, pTeamSayCmd, SH_STATIC(TeamSay_handler), false);
	if (pChangeLevelCmd) SH_ADD_HOOK(ConCommand, Dispatch, pChangeLevelCmd, SH_STATIC(ChangeLevel_handler), false);
	if (pAutoBuyCmd) SH_ADD_HOOK(ConCommand, Dispatch, pAutoBuyCmd, SH_STATIC(AutoBuy_handler), false);
	if (pReBuyCmd) SH_ADD_HOOK(ConCommand, Dispatch, pReBuyCmd, SH_STATIC(ReBuy_handler), false);
#endif
}

void *MyListener::OnMetamodQuery(const char *iface, int *ret)
{
	if (strcmp(iface, "CSourceMMMAP")==0)
	{
		if (ret)
			*ret = IFACE_OK;
		return static_cast<void *>(&g_ManiCallback);
	}
	else if (strcmp(iface, "AdminInterface")==0)
	{
		if (ret)
			*ret = IFACE_OK;

		return (void *)(&g_ClientInterface);
	}

	if (ret)
		*ret = IFACE_FAILED;

	return NULL;
}

ManiSMMHooks g_ManiSMMHooks;

void	ManiSMMHooks::HookVFuncs(void)
{
	if (voiceserver && gpManiGameType->IsVoiceAllowed())
	{
		//MMsg("Hooking voiceserver\n");
		SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_ManiSMMHooks, &ManiSMMHooks::SetClientListening, false); // changed to false by Keeper
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

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		int offset = gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_CANUSE);
		if (offset != -1)
		{
			SH_MANUALHOOK_RECONFIGURE(Player_Weapon_CanUse, offset, 0, 0);
		}
	}

}

bool	ManiSMMHooks::SetClientListening(int iReceiver, int iSender, bool bListen)
{
	bool new_listen = false;

	if ( iSender == iReceiver )
		RETURN_META_VALUE ( MRES_IGNORED, bListen );

	if (ProcessMuteTalk(iReceiver, iSender, &new_listen))
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, new_listen));

	if (ProcessDeadAllTalk(iReceiver, iSender, &new_listen))
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, new_listen));

	RETURN_META_VALUE(MRES_IGNORED, bListen );
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


#ifdef GAME_ORANGE
void RespawnEntities_handler(const CCommand &command)
#else
void RespawnEntities_handler()
#endif
{
	//Override exploit
	RETURN_META(MRES_SUPERCEDE);
} 

#ifdef GAME_ORANGE
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

#ifdef GAME_ORANGE
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

#ifdef GAME_ORANGE
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

#ifdef GAME_ORANGE
void AutoBuy_handler(const CCommand &command)
#else
void AutoBuy_handler()
#endif
{
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);
	gpManiWeaponMgr->PreAutoBuyReBuy();
#ifdef GAME_ORANGE 
	SH_CALL(pAutoBuyCmd, &ConCommand::Dispatch)(command);
#else
#ifndef SOURCEMM
	SH_CALL(pAutoBuyCmd, &ConCommand::Dispatch)();
#else
	SH_CALL(autobuy_cc, &ConCommand::Dispatch)();
#endif
#endif
	gpManiWeaponMgr->AutoBuyReBuy();
	RETURN_META(MRES_SUPERCEDE);
} 

#ifdef GAME_ORANGE
void ReBuy_handler(const CCommand &command)
#else
void ReBuy_handler()
#endif
{
	if(ProcessPluginPaused()) RETURN_META(MRES_IGNORED);
	gpManiWeaponMgr->PreAutoBuyReBuy();
#ifdef GAME_ORANGE 
	SH_CALL(pReBuyCmd, &ConCommand::Dispatch)(command);
#else
#ifndef SOURCEMM
	SH_CALL(pReBuyCmd, &ConCommand::Dispatch)();
#else
	SH_CALL(rebuy_cc, &ConCommand::Dispatch)();	
#endif
#endif
	gpManiWeaponMgr->AutoBuyReBuy();
	RETURN_META(MRES_SUPERCEDE);
} 

#endif

