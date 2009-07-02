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



#ifndef MANI_MANI_GLOBALS_H
#define MANI_MANI_GLOBALS_H

#include "mani_player.h"
#include "mani_main.h"

// Interfaces from the engine
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem		*filesystem; // file I/O 
extern	IServerGameDLL	*serverdll;
extern	IServerGameEnts	*serverents;
extern	IGameEventManager2 *gameeventmanager; // game events interface
extern	IPlayerInfoManager *playerinfomanager; // game dll interface to interact with players
extern	void *gamedll;
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern	IEffects *effects; // fx
extern	IEngineSound *esounds; // sound
extern	ICvar *cvar;	// console vars
extern	INetworkStringTableContainer *networkstringtable;
extern	INetworkStringTable *g_pStringTableManiScreen;
extern	IVoiceServer *voiceserver;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IEngineTrace *enginetrace;
extern	ISpatialPartition *partition;
extern	IServerGameClients *serverclients;

extern	bool g_PluginLoaded;
extern	bool g_PluginLoadedOnce;

extern	CGlobalVars *gpGlobals;
extern	char *mani_version;

extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	IGameEventListener2 *gpManiIGELCallback;
extern	int	con_command_index;

#ifdef SOURCEMM
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>

extern SourceHook::CallClass<IVEngineServer> *engine_cc;
extern SourceHook::CallClass<ITempEntsSystem> *temp_ents_cc;
extern SourceHook::CallClass<IVoiceServer> *voiceserver_cc;
extern SourceHook::CallClass<IServerGameDLL> *serverdll_cc;
extern SourceHook::CallClass<ConCommand> *rebuy_cc;

#endif

#endif
