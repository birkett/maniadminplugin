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



#ifndef MANI_MANI_CLASS__H
#define MANI_MANI_CLASS__H

#include "mani_player.h"
#include "mani_main.h"
#include "mani_menu.h"

#define MANI_MAX_EVENTS (24)
#define MANI_EVENT_HASH_SIZE (19)

enum LOADUP_STATUS {
	LOADUP_CREATED,
	LOADUP_EXISTS,
	LOADUP_FAILED,
};

//---------------------------------------------------------------------------------
// Purpose: Core class 
//---------------------------------------------------------------------------------
class CAdminPlugin: public IGameEventListener2
{
public:
	CAdminPlugin();
	~CAdminPlugin();

	virtual void FireGameEvent( IGameEvent * event );

	// Mimic the callback functions
	bool			Load(void);
	void			Unload( void );
	void			Pause( void );
	void			UnPause( void );
	const char     *GetPluginDescription( void );      
	void			LevelInit( char const *pMapName );
	void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	void			GameFrame( bool simulating );
	void			LevelShutdown( void );
	void			ClientActive( edict_t *pEntity );
	void			ClientDisconnect( edict_t *pEntity );
	void			ClientPutInServer( edict_t *pEntity, char const *playername );
	void			SetCommandClient( int index );
	void			ClientSettingsChanged( edict_t *pEdict );
	PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
#ifdef ORANGE
	PLUGIN_RESULT	ClientCommand( edict_t *pEntity,  const CCommand &args);
#else
	PLUGIN_RESULT	ClientCommand( edict_t *pEntity );
#endif
	PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	// End of callbacks

	void			GetVDFPath ( char *path, const char *SourceMMPath = NULL );
	LOADUP_STATUS	ScanLoadup ( void );
	LOADUP_STATUS	MakeVDF ( char *path, bool SMM);
	LOADUP_STATUS	MakeOrAddToINI ( char *path );

	void			WriteBans( void );
	void			PrintHeader ( FileHandle_t f, const char *fn, const char *ds );
	bool			AddBan ( ban_settings_t *ban );
	bool			AddBan ( player_t *player, const char *key, const char *initiator, int ban_time = 0, const char *prefix = NULL, const char *reason = NULL );
	bool			RemoveBan ( const char *key );

	void			ProcessExplodeAtCurrentPosition( player_t *player);
	bool			CanTeleport(player_t *player);
	void			ProcessChangeName( player_t *player, const char *new_name, char *old_name);
	void			ProcessConsoleVotemap( edict_t *pEntity);
	void			ProcessReflectDamagePlayer( player_t *victim,  player_t *attacker, IGameEvent *event );
	void			ProcessPlayerTeam(IGameEvent * event);
	void			ProcessPlayerDeath(IGameEvent * event);
	void			ProcessDODSPlayerDeath(IGameEvent * event);
	bool			HookSayCommand(bool team_say, const CCommand &args);
	bool			HookChangeLevelCommand(void);

	PLUGIN_RESULT	ProcessMaKick(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBan(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBanIP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUnBan(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSlay(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaOffset(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTeamIndex(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaOffsetScan(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaOffsetScanF(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSlap(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSetCash(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGiveCash(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGiveCashP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTakeCash(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTakeCashP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCash(player_t *player_ptr, const char *command_name, const int help_id, const int command_type, const int mode);
	PLUGIN_RESULT	ProcessMaSetHealth(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGiveHealth(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGiveHealthP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTakeHealth(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTakeHealthP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaHealth(player_t *player_ptr, const char *command_name, const int help_id, const int command_type, const int mode);
	PLUGIN_RESULT	ProcessMaBlind(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaFreeze(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaNoClip(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBurn(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaDrug(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaDecal(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGive(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGiveAmmo(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaColour(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaColourWeapon(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaGravity(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRenderMode(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRenderFX(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTimeBomb(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaFireBomb(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaFreezeBomb(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBeacon(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaMute(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTeleport(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaPosition(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaDropC4 (player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRCon(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBrowse(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCExec(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCExecT(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCExecCT(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCExecAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCExecSpec(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUsers(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaAdmins(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRates(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaConfig(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSaveLoc(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaWar(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaTimeLeft(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSettings(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	void			TurnOffOverviewMap(void);
	void			ProcessMapCycleMode (int map_cycle_mode);


	// Event functions

	void			InitEvents();

	void			EvPlayerHurt(IGameEvent *event);
	void			EvPlayerTeam(IGameEvent *event);
	void			EvPlayerDeath(IGameEvent *event);
	void			EvPlayerSay(IGameEvent *event);
	void			EvPlayerSpawn(IGameEvent *event);
	void			EvWeaponFire(IGameEvent *event);
	void			EvHostageStopsFollowing(IGameEvent *event);
	void			EvBombPlanted(IGameEvent *event);
	void			EvBombDropped(IGameEvent *event);
	void			EvBombExploded(IGameEvent *event);
	void			EvBombDefused(IGameEvent *event);
	void			EvBombBeginDefuse(IGameEvent *event);
	void			EvBombPickUp(IGameEvent *event);
	void			EvHostageRescued(IGameEvent *event);
	void			EvHostageFollows(IGameEvent *event);
	void			EvHostageKilled(IGameEvent *event);
	void			EvRoundStart(IGameEvent *event);
	void			EvRoundEnd(IGameEvent *event);
	void			EvRoundFreezeEnd(IGameEvent *event);
	void			EvVIPKilled(IGameEvent *event);
	void			EvVIPEscaped(IGameEvent *event);
	void			EvDodStatsWeaponAttack(IGameEvent *event);
	void			EvDodPointCaptured(IGameEvent *event);
	void			EvDodCaptureBlocked(IGameEvent *event);
	void			EvDodRoundWin(IGameEvent *event);
	void			EvDodStatsPlayerDamage(IGameEvent *event);
	void			EvDodStatsPlayerKilled(IGameEvent *event);
	void			EvDodGameOver(IGameEvent *event);

private:

	struct	event_fire_t
	{
		char	event_name[256];
		void	(CAdminPlugin::*funcPtr)( IGameEvent *event );
	};

	int				GetEventIndex(const char *event_string, const int loop_length);


	int     event_table[256];
	event_fire_t	event_fire[256];
	int		max_events;
	bool	event_duplicate;
};

MENUALL_DEC(PrimaryMenu);
MENUALL_DEC(RCon);
MENUALL_DEC(MapManagement);
MENUALL_DEC(PlayerManagement);
MENUALL_DEC(PunishType);
MENUALL_DEC(VoteType);
MENUALL_DEC(VoteDelayType);
MENUALL_DEC(BanOptions);
MENUALL_DEC(SlapOptions);
MENUALL_DEC(BlindOptions);
MENUALL_DEC(BanType);
MENUALL_DEC(UnBanType);
MENUALL_DEC(KickType);
MENUALL_DEC(CExecOptions);
MENUALL_DEC(ConfigOptions);
MENUALL_DEC(BanPlayer);
MENUALL_DEC(UnBanPlayer);
MENUALL_DEC(UnBanPlayerDetails);

MENUALL_DEC(ChangeMap);
MENUALL_DEC(SetNextMap);
MENUALL_DEC(KickPlayer);
MENUALL_DEC(SlapMore);
MENUALL_DEC(SlapPlayer);
MENUALL_DEC(BlindPlayer);
MENUALL_DEC(FreezePlayer);
MENUALL_DEC(BurnPlayer);
MENUALL_DEC(DrugPlayer);
MENUALL_DEC(NoClipPlayer);
MENUALL_DEC(TimeBombPlayer);
MENUALL_DEC(FireBombPlayer);
MENUALL_DEC(FreezeBombPlayer);
MENUALL_DEC(BeaconPlayer);
MENUALL_DEC(MutePlayer);
MENUALL_DEC(TeleportPlayer);
MENUALL_DEC(SlayPlayer);
MENUALL_DEC(CExec);
MENUALL_DEC(CExecPlayer);

extern CAdminPlugin g_ManiAdminPlugin;
extern CAdminPlugin *gpManiAdminPlugin;

#endif
