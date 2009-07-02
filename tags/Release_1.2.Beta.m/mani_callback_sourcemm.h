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



#ifndef MANI_CALLBACK_SOURCEMM_H
#define MANI_CALLBACK_SOURCEMM_H

#ifdef SOURCEMM
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <igameevents.h>

#include "mani_main.h"
#include "cbaseentity.h"

class CSourceMMMAP : public ISmmPlugin, public IMetamodListener
{
public:
//	CSourceMMMAP();
//	~CSourceMMMAP();
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen)
	{
		return true;
	}
	bool Unpause(char *error, size_t maxlen)
	{
		return true;
	}
public:
	int GetApiVersion() { return PLAPI_VERSION; }
public:
	const char *GetAuthor()
	{
		return "Mani";
	}
	const char *GetName()
	{
		return "Mani Admin Plugin";
	}
	const char *GetDescription()
	{
		return "Fully loaded admin tool";
	}
	const char *GetURL()
	{
		return "http://www.mani-admin-plugin.com/";
	}
	const char *GetLicense()
	{
		return "close source";
	}
	const char *GetVersion()
	{
		return PLUGIN_CORE_VERSION;
	}
	const char *GetDate()
	{
		return __DATE__;
	}
	const char *GetLogTag()
	{
		return "MANI_ADMIN_PLUGIN";
	}
public:
	//These functions are from IServerPluginCallbacks
	//Note, the parameters might be a little different to match the actual calls!

	//Called on LevelInit.  Server plugins only have pMapName
	bool LevelInit(const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);

	//Called on ServerActivate.  Same definition as server plugins
	void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

	//Called on a game tick.  Same definition as server plugins
	void GameFrame(bool simulating);

	//Called on level shutdown.  Same definition as server plugins 
	void LevelShutdown(void);

	//Client is activate (whatever that means).  We get an extra parameter...
	// "If bLoadGame is true, don't spawn the player because its state is already setup."
	void ClientActive(edict_t *pEntity, bool bLoadGame);

	//Client disconnects - same as server plugins
	void ClientDisconnect(edict_t *pEntity);

	//Client is put in server - same as server plugins
	void ClientPutInServer(edict_t *pEntity, char const *playername);

	//Sets the client index - same as server plugins
	void SetCommandClient(int index);

	//Called on client settings changed (duh) - same as server plugins
	void ClientSettingsChanged(edict_t *pEdict);

	//Called on client connect.  Unlike server plugins, we return whether the 
	// connection is allowed rather than set it through a pointer in the first parameter.
	// You can still supercede the GameDLL through RETURN_META_VALUE(MRES_SUPERCEDE, true/false)
	bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

	//Called when a client uses a command.  Unlike server plugins, it's void.
	// You can still supercede the gamedll through RETURN_META(MRES_SUPERCEDE).
	void ClientCommand(edict_t *pEntity);

	//From IMetamodListener
	virtual void OnLevelShutdown();

private:
/*	IGameEventManager2 *m_GameEventManager;	
	IVEngineServer *m_Engine;
	IServerGameDLL *m_ServerDll;*/
	int m_iClientCommandIndex;
};

class MyListener : public IMetamodListener
{
public:
	virtual void *OnMetamodQuery(const char *iface, int *ret);
};

class CSourceMMMAPCallback : public IServerPluginCallbacks
{
public:
	CSourceMMMAPCallback();
	~CSourceMMMAPCallback();
	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory ) {return true;}
	virtual void			Unload( void ) {return;}
	virtual void			Pause( void ) {return;}
	virtual void			UnPause( void ) {return;}
	virtual const char     *GetPluginDescription( void ) {return NULL;}    
	virtual void			LevelInit( char const *pMapName ) {return;}
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {return;}
	virtual void			GameFrame( bool simulating ) {return;}
	virtual void			LevelShutdown( void ) {return;}
	virtual void			ClientActive( edict_t *pEntity ) {return;}
	virtual void			ClientDisconnect( edict_t *pEntity ) {return;}
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) {return;}
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict ) {return;}
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )  {return PLUGIN_CONTINUE;}
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )  {return PLUGIN_CONTINUE;}
	virtual int GetCommandIndex() { return m_iClientCommandIndex; }
private:
	int m_iClientCommandIndex;
};

class ManiSMMHooks
{
public:
	void	HookVFuncs(void);
	void	HookProcessUsercmds(CBasePlayer *pPlayer);
	void	UnHookProcessUsercmds(CBasePlayer *pPlayer);
	void	ProcessUsercmds(CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused);
	bool	SetClientListening(int iReceiver, int iSender, bool bListen);
	void	PlayerDecal(IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity);
};

extern	void Say_handler();
extern	void TeamSay_handler();
extern	void ChangeLevel_handler();
extern	void AutoBuy_handler();
extern	void ReBuy_handler();

extern CSourceMMMAP g_ManiCallback;
extern ManiSMMHooks g_ManiSMMHooks;

PLUGIN_GLOBALVARS();

#endif


#endif
