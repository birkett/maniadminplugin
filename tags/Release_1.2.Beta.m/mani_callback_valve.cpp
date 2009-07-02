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
#include "mani_callback_valve.h"
#include "mani_main.h"
#include "mani_sigscan.h"
#include "mani_output.h"
#include "mani_globals.h"
#include "mani_mainclass.h"

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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void InitCVars( CreateInterfaceFn cvarFactory );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CValveMAP::CValveMAP()
{
	m_iClientCommandIndex = con_command_index = 0;
	gpManiISPCCallback = this;
//	gpManiIGELCallback = this;
}

CValveMAP::~CValveMAP()
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CValveMAP::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{


	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	gameeventmanager = (IGameEventManager2 *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL);
	filesystem = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
	helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	networkstringtable = (INetworkStringTableContainer *)interfaceFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL);
	enginetrace = (IEngineTrace *)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL);
	randomStr = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);
	serverents = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
	effects = (IEffects*)gameServerFactory(IEFFECTS_INTERFACE_VERSION, NULL);
	esounds = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL);
	cvar = (ICvar*)interfaceFactory(VENGINE_CVAR_INTERFACE_VERSION, NULL);
	serverdll = (IServerGameDLL*) gameServerFactory("ServerGameDLL004", NULL);
	if(serverdll)
	{
		gamedll = serverdll;
	}
	else 
	{
		// Hack for unreleased interface version
		serverdll = (IServerGameDLL*) gameServerFactory("ServerGameDLL003", NULL);
		if(!serverdll)
		{
			// Hack for interface 004 not working on older mods
			serverdll = (IServerGameDLL*) gameServerFactory("ServerGameDLL003", NULL);
		}
	}

	voiceserver = (IVoiceServer*)interfaceFactory(INTERFACEVERSION_VOICESERVER, NULL);
	partition = (ISpatialPartition*)interfaceFactory(INTERFACEVERSION_SPATIALPARTITION, NULL);

	InitCVars( interfaceFactory ); // register any cvars we have defined

	gpGlobals = playerinfomanager->GetGlobalVars();

	if (!cvar)
	{
		MMsg("Failed to load cvar interface !!\n");
		return false;
	}

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
	if (!UTIL_InterfaceMsg(cvar,"ICvar", VENGINE_CVAR_INTERFACE_VERSION)) return false;
	if (!UTIL_InterfaceMsg(serverdll,"IServerGameDLL", "ServerGameDLL003")) return false;
	if (!UTIL_InterfaceMsg(voiceserver,"IVoiceServer", INTERFACEVERSION_VOICESERVER)) return false;
	if (!UTIL_InterfaceMsg(partition,"ISpatialPartition", INTERFACEVERSION_SPATIALPARTITION)) return false;


	MMsg("********************************************************\n");

	return (gpManiAdminPlugin->Load());
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CValveMAP::Unload( void )
{
	gameeventmanager->RemoveListener( gpManiIGELCallback ); // make sure we are unloaded from the event system
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CValveMAP::Pause( void )
{
	gpManiAdminPlugin->Pause();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CValveMAP::UnPause( void )
{
	gpManiAdminPlugin->UnPause();
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CValveMAP::GetPluginDescription( void )
{
	return (gpManiAdminPlugin->GetPluginDescription()); 
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CValveMAP::LevelInit( char const *pMapName )
{
	gpManiAdminPlugin->LevelInit(pMapName);
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CValveMAP::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	gpManiAdminPlugin->ServerActivate(pEdictList, edictCount, clientMax);
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CValveMAP::GameFrame( bool simulating )
{
	gpManiAdminPlugin->GameFrame(simulating);
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CValveMAP::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gpManiAdminPlugin->LevelShutdown();
	gameeventmanager->RemoveListener( gpManiIGELCallback );
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CValveMAP::ClientActive( edict_t *pEntity )
{
	gpManiAdminPlugin->ClientActive(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CValveMAP::ClientDisconnect( edict_t *pEntity )
{
	gpManiAdminPlugin->ClientDisconnect(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CValveMAP::ClientPutInServer( edict_t *pEntity, char const *playername )
{
	gpManiAdminPlugin->ClientPutInServer(pEntity, playername);
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CValveMAP::SetCommandClient( int index )
{
	m_iClientCommandIndex = con_command_index = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CValveMAP::ClientSettingsChanged( edict_t *pEdict )
{
	gpManiAdminPlugin->ClientSettingsChanged(pEdict);
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CValveMAP::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return (gpManiAdminPlugin->ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen));
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CValveMAP::ClientCommand( edict_t *pEntity )
{
	return (gpManiAdminPlugin->ClientCommand(pEntity));
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CValveMAP::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
//void CValveMAP::FireGameEvent( IGameEvent * event )
//{
//	gpManiAdminPlugin->FireGameEvent(event);
//}

CValveMAP g_ManiCallback;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CValveMAP, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_ManiCallback );

#endif
