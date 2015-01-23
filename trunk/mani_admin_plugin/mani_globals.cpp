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
#include "mani_main.h"

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IFileSystem		*filesystem = NULL; // file I/O 
IServerGameDLL	*serverdll = NULL;
IServerGameEnts	*serverents = NULL;
IGameEventManager2 *gameeventmanager = NULL; // game events interface
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
void *gamedll = NULL;
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IEffects *effects = NULL; // fx
IEngineSound *esounds = NULL; // sound
ICvar *g_pCVar = NULL;	// console vars
INetworkStringTableContainer *networkstringtable = NULL;
INetworkStringTable *g_pStringTableManiScreen = NULL;
IVoiceServer *voiceserver = NULL;
ITempEntsSystem *temp_ents = NULL;
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;
ISpatialPartition *spatialpartition = NULL;
IServerGameClients *serverclients = NULL;

bool g_PluginLoaded = false;
bool g_PluginLoadedOnce = false;

CGlobalVars *gpGlobals = NULL;
char *mani_version = PLUGIN_VERSION;
char *mani_build_date = __DATE__;

IServerPluginCallbacks *gpManiISPCCallback = NULL;
IGameEventListener2 *gpManiIGELCallback = NULL;

int		con_command_index = 0;

#if !defined GAME_ORANGE && defined SOURCEMM
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>

SourceHook::CallClass<IVEngineServer> *engine_cc = NULL;
SourceHook::CallClass<ITempEntsSystem> *temp_ents_cc = NULL;
SourceHook::CallClass<IVoiceServer> *voiceserver_cc = NULL;
SourceHook::CallClass<IServerGameDLL> *serverdll_cc = NULL;
SourceHook::CallClass<ConCommand> *rebuy_cc = NULL;
SourceHook::CallClass<ConCommand> *autobuy_cc = NULL;
#endif




