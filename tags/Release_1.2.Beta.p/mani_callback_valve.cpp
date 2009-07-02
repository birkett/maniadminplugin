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
#include "mani_memory.h"
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
extern int max_players;

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CValveMAP::CValveMAP()
{
	m_iClientCommandIndex = con_command_index = 0;
	gpManiISPCCallback = this;
}

CValveMAP::~CValveMAP()
{
}

// Crap macro to try multiple versions of an interface, Valve tend to release
// ServerGameDLL versions that are not documented in the SDK. Best we can do 
// is start at the base version and work up through to _max_iterations until 
// we get a non-null pointer back.
#define GET_INTERFACE(_store, _type, _version, _max_iterations, _interface_type) \
{ \
	char	interface_version[128]; \
	int i, length; \
	Q_strcpy(interface_version, _version); \
	length = Q_strlen(_version); \
	i = atoi(&(interface_version[length - 3])); \
	interface_version[length - 3] = '\0'; \
	AddToList((void **) &interface_list, sizeof(interface_data_t), &interface_list_size); \
	Q_strcpy(interface_list[interface_list_size - 1].interface_name, _version); \
	Q_strcpy(interface_list[interface_list_size - 1].base_interface, _version); \
	for (int j = i; j < i + _max_iterations; j ++) \
	{ \
		char new_interface[128]; \
		snprintf(new_interface, sizeof(new_interface), "%s%03i", interface_version, j); \
		_store = (_type *) _interface_type (new_interface, NULL); \
		if(_store) \
		{ \
			Q_strcpy(interface_list[interface_list_size - 1].interface_name, new_interface); \
			break; \
		} \
	} \
	if (!_store) \
	{ \
		for (int j = i; j > 0; j --) \
		{ \
			char new_interface[128]; \
			snprintf(new_interface, sizeof(new_interface), "%s%03i", interface_version, j); \
			_store = (_type *) _interface_type (new_interface, NULL); \
			if(_store) \
			{ \
				Q_strcpy(interface_list[interface_list_size - 1].interface_name, new_interface); \
				break; \
			} \
		} \
	} \
	interface_list[interface_list_size - 1].ptr = (char *) _store; \
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CValveMAP::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	interface_data_t *interface_list = NULL;
	int	interface_list_size = 0;

	GET_INTERFACE(playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER, 20, gameServerFactory)
	GET_INTERFACE(engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER, 20, interfaceFactory)
	GET_INTERFACE(gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2, 20, interfaceFactory)
	GET_INTERFACE(filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION, 20, interfaceFactory)
	GET_INTERFACE(helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS, 20, interfaceFactory)
	GET_INTERFACE(networkstringtable, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER, 20, interfaceFactory)
	GET_INTERFACE(enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER, 20, interfaceFactory)
	GET_INTERFACE(randomStr, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION, 20, interfaceFactory)
	GET_INTERFACE(serverents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS, 20, gameServerFactory)
	GET_INTERFACE(effects, IEffects, IEFFECTS_INTERFACE_VERSION, 20, gameServerFactory)
	GET_INTERFACE(esounds, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION, 20, interfaceFactory)
	GET_INTERFACE(serverclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS, 20, gameServerFactory)
	GET_INTERFACE(cvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION, 20, interfaceFactory)
	GET_INTERFACE(serverdll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL, 20, gameServerFactory)
	GET_INTERFACE(voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER, 20, interfaceFactory)
	GET_INTERFACE(spatialpartition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION, 20, interfaceFactory)

	gamedll = serverdll;
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

	// Show interfaces that worked
	for (int i = 0; i < interface_list_size; i++)
	{
		if (interface_list[i].ptr)
		{
			if (strcmp(interface_list[i].base_interface, interface_list[i].interface_name) == 0)
			{
				// Base interface used
				MMsg("%p SDK %s\n", interface_list[i].ptr, interface_list[i].base_interface);
			}
			else
			{
				MMsg("%p SDK %s => Upgraded to %s\n", interface_list[i].ptr, interface_list[i].base_interface, interface_list[i].interface_name);
			}
		}
	}

	// Show ones that didn't
	bool	found_failure = false;
	for (int i = 0; i < interface_list_size; i++)
	{
		if (!interface_list[i].ptr)
		{
			if (strcmp(interface_list[i].base_interface, interface_list[i].interface_name) == 0)
			{
				// Base interface used
				MMsg("FAILED !! : %s\n", interface_list[i].base_interface);
				found_failure = true;
			}
		}
	}

	FreeList((void **) &interface_list, &interface_list_size);

	if (found_failure) 
	{
		MMsg("Failure on loading interface, quitting plugin load\n");
		return false;
	}

	MMsg("********************************************************\n");

	// max players = 0 on first load, > 0 on late load
	max_players = gpGlobals->maxClients;

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
