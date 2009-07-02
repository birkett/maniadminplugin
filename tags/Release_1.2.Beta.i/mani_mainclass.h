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
	PLUGIN_RESULT	ClientCommand( edict_t *pEntity );
	PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	// End of callbacks

	PLUGIN_RESULT	ProcessClientCommandSection2(edict_t	*pEntity,const char *pcmd,const char *pcmd2,const char *pcmd3,const char *say_string,char *trimmed_say,char *arg_string,int *say_argc);
	void			LoadCheatList(void);
	void			UpdateCurrentPlayerList(void);
	void			InitCheatPingList(void);
	char			*GenerateControlString(void);
	bool			ProcessCheatCVarPing(player_t *player, const char *pcmd);

	void			ProcessCheatCVarCommands(void);
	PLUGIN_RESULT	ProcessAdminMenu( edict_t *pEntity);
	void			ShowPrimaryMenu( edict_t *pEntity, int admin_index);
	void			ProcessChangeMap( player_t *player, int next_index, int argv_offset );
	void			ProcessSetNextMap( player_t *player, int next_index, int argv_offset );
	void			ProcessKickPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessSlayPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessRConVote( player_t *admin, int next_index, int argv_offset );
	void			ProcessQuestionVote( player_t *admin, int next_index, int argv_offset );
	void			ProcessExplodeAtCurrentPosition( player_t *player);
	void			ProcessRconCommand( player_t *admin, int next_index, int argv_offset );
	void			ProcessCExecCommand( player_t *admin, char *command, int next_index, int argv_offset );
	void			ProcessCExecPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset );
	void			ProcessMenuSystemVoteRandomMap( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuSystemVoteSingleMap( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuSystemVoteBuildMap( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuSystemVoteMultiMap( player_t *admin, int admin_index );
	void			ProcessMenuSlapPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuBlindPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuSwapPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuSpecPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuFreezePlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuBurnPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuNoClipPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuDrugPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuGimpPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuTimeBombPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuFireBombPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuFreezeBombPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuBeaconPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessMenuMutePlayer( player_t *admin, int next_index, int argv_offset );
	bool			CanTeleport(player_t *player);
	void			ProcessMenuTeleportPlayer( player_t *admin, int next_index, int argv_offset );
	void			ProcessBanOptions( edict_t *pEntity, const char *ban_command );
	void			ProcessDelayTypeOptions( player_t *player, const char *menu_command);
	void			ProcessSlapOptions( edict_t *pEntity);
	void			ProcessBlindOptions( edict_t *pEntity);
	void			ProcessBanType( player_t *player );
	void			ProcessKickType( player_t *player );
	void			ProcessMapManagementType( player_t *player, int admin_index );
	void			ProcessPlayerManagementType( player_t *player, int admin_index, int next_index );
	void			ProcessPunishType( player_t *player, int admin_index, int next_index );
	void			ProcessVoteType( player_t *player, int admin_index, int next_index );
	void			ProcessConfigOptions( edict_t *pEntity );
	void			ProcessCExecOptions( edict_t *pEntity );
	void			ProcessConfigToggle( edict_t *pEntity );
	void			ProcessPlayerSay( IGameEvent *event);
	void			ProcessChangeName( player_t *player, const char *new_name, char *old_name);
	edict_t *		GetEntityForUserID (const int user_id);
	void			PrettyPrinter(KeyValues *keyValue, int indent);
	void			ProcessConsoleVotemap( edict_t *pEntity);
	//			void			ProcessMenuVotemap( edict_t *pEntity, int next_index, int argv_offset );
	void			ProcessPlayerHurt(IGameEvent * event);
	void			ProcessReflectDamagePlayer( player_t *victim,  player_t *attacker, IGameEvent *event );
	void			ProcessPlayerTeam(IGameEvent * event);
	void			ProcessPlayerDeath(IGameEvent * event);
	void			ShowTampered(void);
	bool			IsPlayerImmuneFromPingCheck(player_t *player);
	void			ProcessCheatCVars(void);
	void			ProcessHighPingKick(void);
	bool			IsTampered (void);
	bool			HookSayCommand(void);
	bool			HookChangeLevelCommand(void);

	PLUGIN_RESULT	ProcessMaVoteRandom( int index,  bool svr_command,  bool say_command, int argc,  char *command_string,  char *delay_type_string, char *number_of_maps_string);
	PLUGIN_RESULT	ProcessMaVoteExtend( int index,  bool svr_command,  int argc,  char *command_string);
	PLUGIN_RESULT	ProcessMaVote( int index,  bool svr_command,  bool say_command, int argc,  char *command_string,  char *delay_type_string, char *map1, char *map2, char *map3, char *map4, char *map5, char *map6, char *map7, char *map8, char *map9, char *map10);
	bool			AddMapToVote(player_t *player, bool svr_command, bool say_command, char *map_name);
	PLUGIN_RESULT	ProcessMaVoteQuestion(int index,  bool svr_command,  bool say_command, bool menu_command, int argc,  char *command_string,  char *question, char *answer1, char *answer2, char *answer3, char *answer4, char *answer5, char *answer6, char *answer7, char *answer8, char *answer9, char *answer10);
	bool			AddQuestionToVote(player_t *player, bool svr_command, bool say_command, char *answer);
	PLUGIN_RESULT	ProcessMaVoteRCon( int index,  bool svr_command,  bool say_command, bool menu_command, int argc,  char *command_string,  char *question,  char *rcon_command);
	PLUGIN_RESULT	ProcessMaVoteCancel( int index,  bool svr_command,  int argc,  char *command_string);
	PLUGIN_RESULT	ProcessMaKick( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaBan( int index,  bool svr_command,  bool ban_by_ip, int argc,  char *command_string, char *time_to_ban, char *target_string);
	PLUGIN_RESULT	ProcessMaUnBan( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaSlay( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaOffset( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaTeamIndex( int index,  bool svr_command,  int argc,  char *command_string);
	PLUGIN_RESULT	ProcessMaOffsetScan( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *start_range, char *end_range);
	PLUGIN_RESULT	ProcessMaOffsetScanF( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *start_range, char *end_range);
	PLUGIN_RESULT	ProcessMaSlap( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *damage_string);
	PLUGIN_RESULT	ProcessMaCash( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *amount, int mode);
	PLUGIN_RESULT	ProcessMaHealth( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *amount, int mode);
	PLUGIN_RESULT	ProcessMaBlind( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *damage_string);
	PLUGIN_RESULT	ProcessMaFreeze( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaNoClip( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaBurn( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaDrug( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaDecal( int index,  bool svr_command,  int argc,  char *command_string,  char *decal_name);
	PLUGIN_RESULT	ProcessMaGive( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *item_name);
	PLUGIN_RESULT	ProcessMaGiveAmmo( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *weapon_slot_str, char *primary_fire_str, char *amount_str, char *noise_str);
	PLUGIN_RESULT	ProcessMaColour( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *red_str, char *green_str, char *blue_str, char *alpha_str);
	PLUGIN_RESULT	ProcessMaColourWeapon( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *red_str, char *green_str, char *blue_str, char *alpha_str);
	PLUGIN_RESULT	ProcessMaGravity( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *gravity_string);
	PLUGIN_RESULT	ProcessMaRenderMode( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *render_mode_str);
	PLUGIN_RESULT	ProcessMaRenderFX( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *render_fx_str);
	PLUGIN_RESULT	ProcessMaGimp( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaTimeBomb( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaFireBomb( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaFreezeBomb( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaBeacon( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaMute( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *toggle);
	PLUGIN_RESULT	ProcessMaTeleport( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *x_coords, char *y_coords, char *z_coords);
	PLUGIN_RESULT	ProcessMaPosition( int index,  bool svr_command,  int argc,  char *command_string);
	PLUGIN_RESULT	ProcessMaSwapTeam( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaSpec( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string);
	PLUGIN_RESULT	ProcessMaBalance ( int index,  bool svr_command,  bool mute_action);
	PLUGIN_RESULT	ProcessMaDropC4 ( int index,  bool svr_command);
	bool			ProcessMaBalancePlayerType ( player_t	*player, bool svr_command, bool mute_action, bool dead_only, bool dont_care);
	PLUGIN_RESULT	ProcessMaPSay( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *say_string);
	PLUGIN_RESULT	ProcessMaMSay( int index,  bool svr_command,  int argc,  char *command_string,  char *time_display, char *target_string, char *say_string);
	PLUGIN_RESULT	ProcessMaSay( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string);
	PLUGIN_RESULT	ProcessMaCSay( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string);
	PLUGIN_RESULT	ProcessMaChat( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string);
	PLUGIN_RESULT	ProcessMaRCon( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string);
	PLUGIN_RESULT	ProcessMaBrowse( int index,  int argc,  char *command_string,  char *url_string);
	PLUGIN_RESULT	ProcessMaCExecAll( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string);
	PLUGIN_RESULT	ProcessMaCExecTeam( int index,  bool svr_command,  int argc,  char *command_string,  char *say_string, int team);
	PLUGIN_RESULT	ProcessMaCExec( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *say_string);
	PLUGIN_RESULT	ProcessMaUsers( int index,  bool svr_command,  int argc,  char *command_string,  char *target_players);
	PLUGIN_RESULT	ProcessMaRates( int index,  bool svr_command,  int argc,  char *command_string,  char *target_players);
	PLUGIN_RESULT	ProcessMaConfig( int index,  bool svr_command, int argc, char *filter);
	PLUGIN_RESULT	ProcessMaHelp( int index,  bool svr_command, int argc, char *filter);
	PLUGIN_RESULT	ProcessMaSaveLoc( int index,  bool svr_command);
	PLUGIN_RESULT	ProcessMaWar( int index,  bool svr_command,  int argc, char *option);
	PLUGIN_RESULT	ProcessMaSettings( int index);
	PLUGIN_RESULT	ProcessMaTimeLeft( int index, bool svr_command);
	void			TurnOffOverviewMap(void);
	void			ProcessMapCycleMode (int map_cycle_mode);
	void			StartSystemVote (void);
	void			ProcessVotes (void);
	void			ProcessVoteConfirmation (player_t *player, bool accept);
	void			ProcessVoteWin (int win_index);
	void			ProcessMapWin (int win_index);
	void			ProcessQuestionWin (int win_index);
	void			ProcessExtendWin (int win_index);
	void			ProcessRConWin (int win_index);
	bool			IsYesNoVote (void);
	void			ProcessMenuSystemVotemap( player_t *player, int next_index, int argv_offset );
	void			BuildRandomMapVote( int max_maps);
	void			ProcessPlayerVoted( player_t *player, int vote_index);
	void			ProcessBuildUserVoteMaps(void);
	void			ShowCurrentUserMapVotes( player_t *player, int votes_required );
	void			ProcessMaUserVoteMap(player_t *player, int argc, const char *map_id);
	void			ProcessUserVoteMapWin(int map_index);
	bool			CanWeUserVoteMapYet( player_t *player );
	bool			CanWeUserVoteMapAgainYet( player_t *player );
	int				GetVotesRequiredForUserVote( bool player_leaving, float percentage, int minimum_votes );
	void			ProcessMenuUserVoteMap( player_t *player, int next_index, int argv_offset );
	void			ShowCurrentUserKickVotes( player_t *player, int votes_required );
	void			ShowCurrentRockTheVoteMaps( player_t *player);
	void			ProcessRockTheVoteWin (int win_index);
	void			ProcessRockTheVoteNominateMap(player_t *player);
	bool			CanWeRockTheVoteYet(player_t *player);
	bool 			CanWeNominateAgainYet( player_t *player );
	void			BuildRockTheVoteMapVote (void);
	void 			ProcessStartRockTheVote(void);
	void			ProcessMenuRockTheVoteNominateMap( player_t *player, int next_index, int argv_offset );
	void			ProcessMaRockTheVoteNominateMap(player_t *player, int argc, const char *map_id);
	void			ProcessMaRockTheVote(player_t *player);
	void			ProcessMaUserVoteKick(player_t *player, int argc, const char *user_id);
	void			ProcessUserVoteKickWin(player_t *player);
	bool			CanWeUserVoteKickYet( player_t *player );
	bool			CanWeUserVoteKickAgainYet( player_t *player );
	void			ProcessMenuUserVoteKick( player_t *player, int next_index, int argv_offset );
	void			ShowCurrentUserBanVotes( player_t *player, int votes_required );
	void			ProcessMaUserVoteBan(player_t *player, int argc, const char *user_id);
	void			ProcessUserVoteBanWin(player_t *player);
	bool			CanWeUserVoteBanYet( player_t *player );
	bool			CanWeUserVoteBanAgainYet( player_t *player );
	void			ProcessMenuUserVoteBan( player_t *player, int next_index, int argv_offset );

};

extern CAdminPlugin g_ManiAdminPlugin;
extern CAdminPlugin *gpManiAdminPlugin;

#endif
