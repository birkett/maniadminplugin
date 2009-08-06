
/**
 * vim: set ts=4 :
 * ======================================================
 * Metamod:Source
 * Copyright (C) 2004-2008 AlliedModders LLC and authors.
 * All rights reserved.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, 
 * including commercial applications, and to alter it and redistribute it 
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not 
 * claim that you wrote the original software. If you use this software in a 
 * product, an acknowledgment in the product documentation would be 
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Version: $Id$
 */

#include <stdio.h>
#include "../metamod_oslink.h"
#include <sourcehook.h>
#include <convar.h>
#include <eiface.h>
#include <tier0/icommandline.h>
#include <tier1/utldict.h>
#include <sh_vector.h>
#include <sh_string.h>
#include "../metamod_util.h"
#include "provider_ep2.h"
#include "console.h"
#include "metamod_console.h"
#include "vsp_listener.h"
#include <filesystem.h>
#include "metamod.h"

/* Types */
typedef void (*CONPRINTF_FUNC)(const char *, ...);
struct UsrMsgInfo
{
	UsrMsgInfo()
	{
	}
	UsrMsgInfo(int s, const char *t) : size(s), name(t)
	{
	}
	int size;
	String name;
};

/* Imports */
#if SOURCE_ENGINE == SE_DARKMESSIAH
#undef CommandLine
DLL_IMPORT ICommandLine *CommandLine();
#endif

/* Functions */
bool CacheUserMessages();
#if SOURCE_ENGINE >= SE_ORANGEBOX
void ClientCommand(edict_t *pEdict, const CCommand &args);
void LocalCommand_Meta(const CCommand &args);
#else
void ClientCommand(edict_t *pEdict);
void LocalCommand_Meta();
#endif

void _ServerCommand();
/* Variables */
static bool usermsgs_extracted = false;
static CVector<UsrMsgInfo> usermsgs_list;
static VSPListener g_VspListener;
static BaseProvider g_Ep1Provider;
static List<ConCommandBase *> conbases_unreg;

ICvar *icvar = NULL;
IFileSystem *baseFs = NULL;
IServerGameDLL *server = NULL;
IVEngineServer *engine = NULL;
IServerGameClients *gameclients = NULL;
IMetamodSourceProvider *provider = &g_Ep1Provider;
ConCommand meta_local_cmd("meta", LocalCommand_Meta, "Metamod:Source control options");

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

bool AssumeUserMessages()
{
	int q, size;
	char buffer[256];

	q = 0;

	while (server->GetUserMessageInfo(q, buffer, sizeof(buffer), size))
	{
		usermsgs_list.push_back(UsrMsgInfo(size, buffer));
		q++;
	}

	return true;
}

void BaseProvider::ConsolePrint(const char *str)
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	ConMsg("%s", str);
#else
	Msg("%s", str);
#endif
}

void BaseProvider::Notify_DLLInit_Pre(CreateInterfaceFn engineFactory, 
									  CreateInterfaceFn serverFactory)
{
	engine = (IVEngineServer *)((engineFactory)(INTERFACEVERSION_VENGINESERVER, NULL));
	if (!engine)
	{
		DisplayError("Could not find IVEngineServer! Metamod cannot load.");
		return;
	}
#if SOURCE_ENGINE >= SE_ORANGEBOX
	icvar = (ICvar *)((engineFactory)(CVAR_INTERFACE_VERSION, NULL));
#else
	icvar = (ICvar *)((engineFactory)(VENGINE_CVAR_INTERFACE_VERSION, NULL));
#endif
	if (!icvar)
	{
		DisplayError("Could not find ICvar! Metamod cannot load.");
		return;
	}


	if ((gameclients = (IServerGameClients *)(serverFactory("ServerGameClients003", NULL)))
		== NULL)
	{
		gameclients = (IServerGameClients *)(serverFactory("ServerGameClients004", NULL));
	}

	baseFs = (IFileSystem *)((engineFactory)(FILESYSTEM_INTERFACE_VERSION, NULL));
	if (baseFs == NULL)
	{
		mm_LogMessage("Unable to find \"%s\": .vdf files will not be parsed", FILESYSTEM_INTERFACE_VERSION);
		return;
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX
	g_pCVar = icvar;
#endif

	g_SMConVarAccessor.RegisterConCommandBase(&meta_local_cmd);

	if ((usermsgs_extracted = CacheUserMessages()) == false)
	{
		usermsgs_extracted = AssumeUserMessages();
	}

#if SOURCE_ENGINE == SE_DARKMESSIAH
	if (!g_SMConVarAccessor.InitConCommandBaseList())
	{
		/* This is very unlikely considering it's old engine */
		mm_LogMessage("[META] Warning: Failed to find ConCommandBase list!");
		mm_LogMessage("[META] Warning: ConVars and ConCommands cannot be unregistered properly! Please file a bug report.");
	}
#endif

	if (gameclients)
	{
		SH_ADD_HOOK_STATICFUNC(IServerGameClients, ClientCommand, gameclients, ClientCommand, false);
	}
}

void BaseProvider::Notify_DLLShutdown_Pre()
{
	g_SMConVarAccessor.RemoveMetamodCommands();

#if SOURCE_ENGINE == SE_DARKMESSIAH
	if (g_Metamod.IsLoadedAsGameDLL())
	{
		icvar->UnlinkVariables(FCVAR_GAMEDLL);
	}
#endif
}

bool BaseProvider::IsRemotePrintingAvailable()
{
	return true;
}

void BaseProvider::ClientConsolePrint(edict_t *client, const char *message)
{
	engine->ClientPrintf(client, message);
}

void BaseProvider::ServerCommand(const char *cmd)
{
	engine->ServerCommand(cmd);
}

const char *BaseProvider::GetConVarString(ConVar *convar)
{
	if (convar == NULL)
	{
		return NULL;
	}

	return convar->GetString();
}

void BaseProvider::SetConVarString(ConVar *convar, const char *str)
{
	convar->SetValue(str);
}

bool BaseProvider::IsConCommandBaseACommand(ConCommandBase *pCommand)
{
	return pCommand->IsCommand();
}


bool BaseProvider::IsSourceEngineBuildCompatible(int build)
{
	return (build == SOURCE_ENGINE_ORIGINAL
			|| build == SOURCE_ENGINE_EPISODEONE);
}

const char *BaseProvider::GetCommandLineValue(const char *key, const char *defval)
{
	if (key[0] == '-' || key[0] == '+')
	{
		return CommandLine()->ParmValue(key, defval);
	}
	else if (icvar)
	{
		const char *val;
		if ((val = icvar->GetCommandLineValue(key)) == NULL)
		{
			return defval;
		}

		return val;
	}

	return NULL;
}

int BaseProvider::TryServerGameDLL(const char *iface)
{
	if (strncmp(iface, "ServerGameDLL", 13) != 0)
	{
		return 0;
	}

	return atoi(&iface[13]);
}

bool BaseProvider::LogMessage(const char *buffer)
{
	if (!engine)
	{
		return false;
	}

	engine->LogPrint(buffer);

	return true;
}

bool BaseProvider::GetHookInfo(ProvidedHooks hook, SourceHook::MemFuncInfo *pInfo)
{
	SourceHook::MemFuncInfo mfi = {true, -1, 0, 0};

	if (hook == ProvidedHook_LevelInit)
	{
		SourceHook::GetFuncInfo(&IServerGameDLL::LevelInit, mfi);
	}
	else if (hook == ProvidedHook_LevelShutdown)
	{
		SourceHook::GetFuncInfo(&IServerGameDLL::LevelShutdown, mfi);
	}
	else if (hook == ProvidedHook_GameInit)
	{
		SourceHook::GetFuncInfo(&IServerGameDLL::GameInit, mfi);
	}

	*pInfo = mfi;

	return (mfi.thisptroffs >= 0);
}

void BaseProvider::DisplayError(const char *fmt, ...)
{
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	UTIL_FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	Error(buffer);
}

void BaseProvider::DisplayWarning(const char *fmt, ...)
{
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	UTIL_FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	Warning(buffer);
}

IConCommandBaseAccessor *BaseProvider::GetConCommandBaseAccessor()
{
	return &g_SMConVarAccessor;
}

bool BaseProvider::RegisterConCommandBase(ConCommandBase *pCommand)
{
	return g_SMConVarAccessor.Register(pCommand);
}

void BaseProvider::UnregisterConCommandBase(ConCommandBase *pCommand)
{
	return g_SMConVarAccessor.Unregister(pCommand);
}

int BaseProvider::GetUserMessageCount()
{
	if (!usermsgs_extracted)
	{
		return -1;
	}

	return (int)usermsgs_list.size();
}

int BaseProvider::FindUserMessage(const char *name, int *size)
{
	for (size_t i = 0; i < usermsgs_list.size(); i++)
	{
		if (usermsgs_list[i].name.compare(name) == 0)
		{
			if (size)
			{
				*size = usermsgs_list[i].size;
			}
			return (int)i;
		}
	}
	
	return -1;
}

const char *BaseProvider::GetUserMessage(int index, int *size)
{
	if (!usermsgs_extracted || index < 0 || index >= (int)usermsgs_list.size())
	{
		return NULL;
	}

	if (size)
	{
		*size = usermsgs_list[index].size;
	}

	return usermsgs_list[index].name.c_str();
}

const char *BaseProvider::GetGameDescription()
{
	return server->GetGameDescription();
}

int BaseProvider::DetermineSourceEngine(const char *game)
{
#if SOURCE_ENGINE == SE_LEFT4DEAD
	return SOURCE_ENGINE_LEFT4DEAD;
#elif SOURCE_ENGINE == SE_ORANGEBOX
	return SOURCE_ENGINE_ORANGEBOX;
#else
	return SOURCE_ENGINE_DARKMESSIAH;
#endif
}

ConVar *BaseProvider::CreateConVar(const char *name,
								   const char *defval,
								   const char *help,
								   int flags)
{
	int newflags = 0;
	if (flags & ConVarFlag_Notify)
	{
		newflags |= FCVAR_NOTIFY;
	}
	if (flags & ConVarFlag_SpOnly)
	{
		newflags |= FCVAR_SPONLY;
	}

	ConVar *pVar = new ConVar(name, defval, newflags, help);

	g_SMConVarAccessor.RegisterConCommandBase(pVar);

	return pVar;
}

IServerPluginCallbacks *BaseProvider::GetVSPCallbacks(int version)
{
	if (version > 2)
	{
		return NULL;
	}

	g_VspListener.SetLoadable(true);
	return &g_VspListener;
}

bool BaseProvider::ProcessVDF(const char *file, char path[], size_t path_len, char alias[], size_t alias_len)
{
	if (baseFs == NULL)
	{
		return false;
	}

	KeyValues *pValues;
	const char *plugin_file, *p_alias;

	pValues = new KeyValues("Metamod Plugin");

	if (!pValues->LoadFromFile(baseFs, file))
	{
		pValues->deleteThis();
		return false;
	}

	if ((plugin_file = pValues->GetString("file", NULL)) == NULL)
	{
		pValues->deleteThis();
		return false;
	}

	UTIL_Format(path, path_len, "%s", plugin_file);

	if ((p_alias = pValues->GetString("alias", NULL)) != NULL)
	{
		UTIL_Format(alias, alias_len, "%s", p_alias);
	}
	else
	{
		UTIL_Format(alias, alias_len, "");
	}

	pValues->deleteThis();

	return true;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
class GlobCommand : public IMetamodSourceCommandInfo
{
public:
	GlobCommand(const CCommand *cmd) : m_cmd(cmd)
	{
	}
public:
	unsigned int GetArgCount()
	{
		return m_cmd->ArgC() - 1;
	}

	const char *GetArg(unsigned int num)
	{
		return m_cmd->Arg(num);
	}

	const char *GetArgString()
	{
		return m_cmd->ArgS();
	}
private:
	const CCommand *m_cmd;
};
#else
class GlobCommand : public IMetamodSourceCommandInfo
{
public:
	unsigned int GetArgCount()
	{
		return engine->Cmd_Argc() - 1;
	}

	const char *GetArg(unsigned int num)
	{
		return engine->Cmd_Argv(num);
	}

	const char *GetArgString()
	{
		return engine->Cmd_Args();
	}
};
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
void LocalCommand_Meta(const CCommand &args)
{
	GlobCommand cmd(&args);
#else
void LocalCommand_Meta()
{
	GlobCommand cmd;
#endif
	Command_Meta(&cmd);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void ClientCommand(edict_t *pEdict, const CCommand &_cmd)
{
	GlobCommand cmd(&_cmd);
#else
void ClientCommand(edict_t *pEdict)
{
	GlobCommand cmd;
#endif
	if (strcmp(cmd.GetArg(0), "meta") == 0)
	{
		Command_ClientMeta(pEdict, &cmd);
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

//////////////////////////////////////////////////////////////////////
// EVEN MORE HACKS HERE! YOU HAVE BEEN WARNED!                      //
// Signatures necessary in finding the pointer to the CUtlDict that //
//   stores user message information.                               //
// IServerGameDLL::GetUserMessageInfo() normally crashes with bad   //
//   message indices. This is our answer to it. Yuck! <:-(          //
//////////////////////////////////////////////////////////////////////
#ifdef OS_WIN32
	/* General Windows sig */
	#define MSGCLASS_SIGLEN		7
	#define MSGCLASS_SIG		"\x8B\x0D\x2A\x2A\x2A\x2A\x56"
	#define MSGCLASS_OFFS		2

	/* Dystopia Windows hack */
	#define MSGCLASS2_SIGLEN	16
	#define MSGCLASS2_SIG		"\x56\x8B\x74\x24\x2A\x85\xF6\x7C\x2A\x3B\x35\x2A\x2A\x2A\x2A\x7D"
	#define MSGCLASS2_OFFS		11

	/* Windows frame pointer sig */
	#define MSGCLASS3_SIGLEN	18
	#define MSGCLASS3_SIG		"\x55\x8B\xEC\x51\x89\x2A\x2A\x8B\x2A\x2A\x50\x8B\x0D\x2A\x2A\x2A\x2A\xE8"
	#define MSGCLASS3_OFFS		13
#elif defined OS_LINUX
	/* No frame pointer sig */
	#define MSGCLASS_SIGLEN		14
	#define MSGCLASS_SIG		"\x53\x83\xEC\x2A\x8B\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89"
	#define MSGCLASS_OFFS		9

	/* Frame pointer sig */
	#define MSGCLASS2_SIGLEN	16
	#define MSGCLASS2_SIG		"\x55\x89\xE5\x53\x83\xEC\x2A\x8B\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89"
	#define MSGCLASS2_OFFS		11
#endif

struct UserMessage
{
	int size;
	const char *name;
};

typedef CUtlDict<UserMessage *, int> UserMsgDict;

/* This is the ugliest function in all of MM:S */
bool CacheUserMessages()
{
	UserMsgDict *dict = NULL;

	/* Get address of original GetUserMessageInfo() */
	char *vfunc = (char *)SH_GET_ORIG_VFNPTR_ENTRY(server, &IServerGameDLL::GetUserMessageInfo);

	/* Oh dear, we have a relative jump on our hands
	 * PVK II on Windows made me do this, but I suppose it doesn't hurt to check this on Linux too...
	 */
	if (*vfunc == '\xE9')
	{
		/* Get address from displacement...
		 *
		 * Add 5 because it's relative to next instruction:
		 * Opcode <1 byte> + 32-bit displacement <4 bytes> 
		 */
		vfunc += *reinterpret_cast<int *>(vfunc + 1) + 5;
	}

	if (UTIL_VerifySignature(vfunc, MSGCLASS_SIG, MSGCLASS_SIGLEN))
	{
		/* Get address of CUserMessages instance */
		char **userMsgClass = *reinterpret_cast<char ***>(vfunc + MSGCLASS_OFFS);

		/* Get address of CUserMessages::m_UserMessages */
		dict = reinterpret_cast<UserMsgDict *>(*userMsgClass);
	} 
	else if (UTIL_VerifySignature(vfunc, MSGCLASS2_SIG, MSGCLASS2_SIGLEN)) 
	{
	#ifdef OS_WIN32
		/* If we get here, the code is possibly inlined like in Dystopia */

		/* Get the address of the CUtlRBTree */
		char *rbtree = *reinterpret_cast<char **>(vfunc + MSGCLASS2_OFFS);

		/* CUtlDict should be 8 bytes before the CUtlRBTree (hacktacular!) */
		dict = reinterpret_cast<UserMsgDict *>(rbtree - 8);
	#elif defined OS_LINUX
		/* Get address of CUserMessages instance */
		char **userMsgClass = *reinterpret_cast<char ***>(vfunc + MSGCLASS2_OFFS);

		/* Get address of CUserMessages::m_UserMessages */
		dict = reinterpret_cast<UserMsgDict *>(*userMsgClass);
	#endif
	#ifdef OS_WIN32
	} 
	else if (UTIL_VerifySignature(vfunc, MSGCLASS3_SIG, MSGCLASS3_SIGLEN)) 
	{
		/* Get address of CUserMessages instance */
		char **userMsgClass = *reinterpret_cast<char ***>(vfunc + MSGCLASS3_OFFS);

		/* Get address of CUserMessages::m_UserMessages */
		dict = reinterpret_cast<UserMsgDict *>(*userMsgClass);
	#endif
	}

	#if !defined OS_WIN32
	if (dict == NULL)
	{
		char path[255];
		if (GetFileOfAddress(vfunc, path, sizeof(path)))
		{
			void *handle = dlopen(path, RTLD_NOW);
			if (handle != NULL)
			{
				void *addr = dlsym(handle, "usermessages");
				if (addr == NULL)
				{
					return false;
				}
				dict = (UserMsgDict *)*(void **)addr;
				dlclose(handle);
			}
		}
	}
	#endif

	if (dict != NULL)
	{
		int msg_count = dict->Count();

		/* Ensure that count is within bounds of an unsigned byte, because that's what engine supports */
		if (msg_count < 0 || msg_count > 255)
		{
			return false;
		}

		UserMessage *msg;
		UsrMsgInfo u_msg;

		/* Cache messages in our CUtlDict */
		for (int i = 0; i < msg_count; i++)
		{
			msg = dict->Element(i);
			u_msg.name = msg->name;
			u_msg.size = msg->size;
			usermsgs_list.push_back(u_msg);
		}

		return true;
	}

	return false;
}

