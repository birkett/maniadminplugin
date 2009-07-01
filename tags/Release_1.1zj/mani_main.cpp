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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_timers.h"
#include "mani_player.h"
#include "mani_admin_flags.h"
#include "mani_immunity_flags.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_memory.h"
#include "mani_parser.h"
#include "mani_convar.h"
#include "mani_admin.h"
#include "mani_immunity.h"
#include "mani_menu.h"
#include "mani_output.h"
#include "mani_panel.h"
#include "mani_quake.h"
#include "mani_crontab.h"
#include "mani_adverts.h"
#include "mani_maps.h"
#include "mani_stats.h"
#include "mani_replace.h"
#include "mani_effects.h"
#include "mani_sounds.h"
#include "mani_voice.h"
#include "mani_skins.h"
#include "mani_weapon.h"
#include "mani_team.h"
#include "mani_teamkill.h"
#include "mani_ghost.h"
#include "mani_customeffects.h"
#include "mani_mapadverts.h"
#include "mani_downloads.h"
#include "mani_victimstats.h"
#include "mani_sprayremove.h"
#include "mani_warmuptimer.h"
#include "mani_vfuncs.h"
#include "mani_vars.h"

#include "shareddefs.h"
#include "cbaseentity.h"
//#include "baseentity.h"
//#include "edict.h"

#include "mani_hook.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#include "beam_flags.h" 

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IFileSystem		*filesystem = NULL; // file I/O 
IServerGameDLL	*serverdll = NULL;
IServerGameEnts	*serverents = NULL;
IGameEventManager2 *gameeventmanager = NULL; // game events interface
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IEffects *effects = NULL; // fx
IEngineSound *esounds = NULL; // sound
ICvar *cvar = NULL;	// console vars
INetworkStringTableContainer *networkstringtable = NULL;
INetworkStringTable *g_pStringTableManiScreen = NULL;
IVoiceServer *voiceserver = NULL;
ITempEntsSystem *temp_ents = NULL;
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;

CGlobalVars *gpGlobals = NULL;
char *mani_version = PLUGIN_VERSION;
int			menu_message_index = 10;
int			text_message_index = 5;
int			fade_message_index = 12;
int			vgui_message_index = 13;
int			saytext2_message_index = 4;
int			saytext_message_index = 3;
int			radiotext_message_index = 21;
int			hudmsg_message_index = 6;

int			tp_beam_index = 0;
int			plasmabeam_index = 0;
int			lgtning_index = 0;
int			explosion_index = 0;
int			orangelight_index = 0;
int			bluelight_index = 0;
int			purplelaser_index = 0;

cheat_cvar_t	*cheat_cvar_list = NULL;
cheat_cvar_t	*cheat_cvar_list2 = NULL;
cheat_pinger_t	cheat_ping_list[MANI_MAX_PLAYERS];
int				cheat_cvar_list_size = 0;
int				cheat_cvar_list_size2 = 0;

// General sounds
char *menu_select_exit_sound="buttons/combine_button7.wav";
char *menu_select_sound="buttons/button14.wav";

//declare the original func and the gate if in windows. syntax is:
//DEFVFUNC_
//        <name of the var you want to hold the original func>,
//        <return type>,
//        <args INCLUDING a pointer to the type of class it is in the beginning>
DEFVFUNC_(voiceserver_SetClientListening, bool, (IVoiceServer* VoiceServer, int iReceiver, int iSender, bool bListen));

//just define the func you want to be called instead of the original
//making sure to include VFUNC between the return type and name
bool VFUNC mysetclientlistening(IVoiceServer* VoiceServer, int iReceiver, int iSender, bool bListen)
{
	bool new_listen;

	if (ProcessDeadAllTalk(iReceiver, iSender, &new_listen))
	{
		return voiceserver_SetClientListening(VoiceServer, iReceiver, iSender, new_listen);
	}
	else
	{
		return voiceserver_SetClientListening(VoiceServer, iReceiver, iSender, bListen);
	}
}

//	virtual void PlayerDecal( IRecipientFilter& filer, float delay,
//		const Vector* pos, int player, int entity ) = 0;

DEFVFUNC_(te_PlayerDecal, void, (ITempEntsSystem *pTESys, IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity));

void VFUNC myplayerdecal(ITempEntsSystem *pTESys, IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity)
{
//	Msg("Spray detected !!\n");
	if (gpManiSprayRemove->SprayFired(pos, player))
	{
		// We let this one through.
		te_PlayerDecal(pTESys, filter, delay, pos, player, entity);
	}
}

//declare the original func and the gate if in windows. syntax is:
//DEFVFUNC_
//        <name of the var you want to hold the original func>,
//        <return type>,
//        <args INCLUDING a pointer to the type of class it is in the beginning>
DEFVFUNC_(OrgEngineEntityMessageBegin, bf_write*, (IVEngineServer* EngineServer, MRecipientFilter *filter, int msg_type));

//just define the func you want to be called instead of the original
//making sure to include VFUNC between the return type and name
bf_write* VFUNC ManiEntityMessageBeginHook(IVEngineServer* EngineServer, MRecipientFilter *filter, int msg_type)
{
	Msg("Message [%i]\n", msg_type);
	return OrgEngineEntityMessageBegin(EngineServer, filter, msg_type);
}

//declare the original func and the gate if in windows. syntax is:
//DEFVFUNC_
//        <name of the var you want to hold the original func>,
//        <return type>,
//        <args INCLUDING a pointer to the type of class it is in the beginning>
DEFVFUNC_(grules_DefaultFOV, int, (CGameRules* GameRules));

//just define the func you want to be called instead of the original
//making sure to include VFUNC between the return type and name
int VFUNC MyDefaultFOV(CGameRules* GameRules)
{

	//ReSpawnPlayer(pPlayer);
	Msg("******Default FOV************\n");
	return grules_DefaultFOV(GameRules);
}
// function to initialize any cvars/command in this plugin
void InitCVars( CreateInterfaceFn cvarFactory );

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}


static int sort_reserve_slots_by_steam_id ( const void *m1,  const void *m2);
static int sort_ping_immunity_by_steam_id ( const void *m1,  const void *m2);
static int sort_active_players_by_ping ( const void *m1,  const void *m2);
static int sort_active_players_by_connect_time ( const void *m1,  const void *m2);
static int sort_autokick_steam ( const void *m1,  const void *m2);
static int sort_autokick_ip ( const void *m1,  const void *m2);
static int sort_nominations_by_votes_cast ( const void *m1,  const void *m2);

static void ManiAdminPluginVersion ( ConVar *var, char const *pOldString );
static void ManiTickrate ( ConVar *var, char const *pOldString );
static void WarModeChanged ( ConVar *var, char const *pOldString );
static void HighPingKick ( ConVar *var, char const *pOldString );
static void HighPingKickSamples ( ConVar *var, char const *pOldString );
static void ManiStatsBySteamID ( ConVar *var, char const *pOldString );
static void ManiCSStackingNumLevels ( ConVar *var, char const *pOldString );
static void ManiUnlimitedGrenades ( ConVar *var, char const *pOldString );

ConVar mani_admin_plugin_version ("mani_admin_plugin_version", PLUGIN_CORE_VERSION, FCVAR_REPLICATED | FCVAR_NOTIFY, "This is the version of the plugin", ManiAdminPluginVersion); 
ConVar mani_war_mode ("mani_war_mode", "0", 0, "This defines whether war mode is enabled or disabled (1 = enabled)", true, 0, true, 1, WarModeChanged); 
ConVar mani_high_ping_kick ("mani_high_ping_kick", "0", 0, "This defines whether the high ping kicker is enabled or not", true, 0, true, 1, HighPingKick); 
ConVar mani_high_ping_kick_samples_required ("mani_high_ping_kick_samples_required", "60", 0, "This defines the amount of samples required before the player is kicked", true, 0, true, 10000, HighPingKickSamples); 
ConVar mani_stats_by_steam_id ("mani_stats_by_steam_id", "1", 0, "This defines whether the steam id is used or name is used to organise the stats (1 = steam id)", true, 0, true, 1, ManiStatsBySteamID); 
ConVar mani_tickrate ("mani_tickrate", "", FCVAR_REPLICATED | FCVAR_NOTIFY, "Server tickrate information", ManiTickrate);
ConVar mani_cs_stacking_num_levels ("mani_cs_stacking_num_levels", "1", 0, "Set number of players that can build a stack", true, 1, true, 50, ManiCSStackingNumLevels);
ConVar mani_unlimited_grenades ("mani_unlimited_grenades", "0", 0, "0 = normal CSS mode, 1 = Grenades replenished after throw (CSS Only)", true, 0, true, 1, ManiUnlimitedGrenades);

bool war_mode = false;
float	next_ping_check;
int	max_players = 0;
average_ping_t	average_ping_list[MANI_MAX_PLAYERS];
disconnected_player_t	disconnected_player_list[MANI_MAX_PLAYERS];
check_ping_t	check_ping_list[MANI_MAX_PLAYERS];
say_argv_t		say_argv[MAX_SAY_ARGC];
float			chat_flood[MANI_MAX_PLAYERS];

int				last_slapped_player;
float			last_slapped_time;
tw_spam_t		tw_spam_list[MANI_MAX_PLAYERS];

system_vote_t	system_vote;
voter_t			voter_list[MANI_MAX_PLAYERS];
vote_option_t	*vote_option_list;
int				vote_option_list_size;
int				name_changes[MANI_MAX_PLAYERS];

user_vote_t		user_vote_list[MANI_MAX_PLAYERS];
name_change_t	user_name[MANI_MAX_PLAYERS];
team_scores_t	team_scores;
int				server_tickrate = 33;

bool		trigger_changemap;
float		trigger_changemap_time;
bool		change_team;
float		change_team_time;
float		timeleft_offset;
bool		get_new_timeleft_offset;
bool		round_end_found;
bool		first_map_loaded = false;
char		custom_map_config[512]="";
bool		just_loaded;
float		end_spawn_protection_time;
int			round_number;
float		round_start_time = 0;

ConVar	*mp_friendlyfire; 
ConVar	*mp_freezetime;
ConVar	*mp_winlimit; 
ConVar	*mp_maxrounds; 
ConVar	*mp_timelimit; 
ConVar	*mp_fraglimit; 
ConVar	*mp_limitteams; 
ConVar	*sv_lan; 
ConVar	*sv_cheats; 
ConVar	*sv_alltalk; 
ConVar	*mp_restartgame; 
ConVar	*sv_gravity; 
ConVar  *hostname;
ConVar	*cs_stacking_num_levels;
ConVar  *phy_pushscale;

int		con_command_index = 0;


//RenderMode_t mani_render_mode = kRenderNormal;

bf_write *msg_buffer;

//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CAdminPlugin: public IServerPluginCallbacks, public IGameEventListener2
{
public:
	CAdminPlugin();
	~CAdminPlugin();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void FireGameEvent( IGameEvent * event );
	virtual int GetCommandIndex() { return m_iClientCommandIndex; }

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
			void			ProcessAutoBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset );
			void			ProcessAutoKickPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset );
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
			void			AddAutoKickIP(char *details);
			void			AddAutoKickSteamID(char *details);
			void			AddAutoKickName(char *details);
			void			AddAutoKickPName(char *details);
			void			ProcessConsoleVotemap( edict_t *pEntity);
			void			ProcessMenuVotemap( edict_t *pEntity, int next_index, int argv_offset );
			int				GetNumberOfActivePlayers(void );
			void			ProcessPlayerHurt(IGameEvent * event);
			void			ProcessReflectDamagePlayer( player_t *victim,  player_t *attacker, IGameEvent *event );
			void			ProcessPlayerTeam(IGameEvent * event);
			void			ProcessPlayerDeath(IGameEvent * event);
			bool			ProcessAutoKick(player_t *player);
			bool			ProcessReserveSlotJoin(player_t *player);
			bool			IsPlayerInReserveList(player_t *player);
			bool			IsPlayerImmuneFromPingCheck(player_t *player);
			void			BuildPlayerKickList(player_t *player, int *players_on_server);
			void			DisconnectPlayer(player_t *player);
			void			ProcessCheatCVars(void);
			void			ProcessHighPingKick(void);
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
			PLUGIN_RESULT	ProcessMaAutoKickBanShowName( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaAutoKickBanShowPName( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaAutoKickBanShowSteam( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaAutoKickBanShowIP( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaWar( int index,  bool svr_command,  int argc, char *option);
			PLUGIN_RESULT	ProcessMaSettings( int index);
			PLUGIN_RESULT	ProcessMaAdmins( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaAdminGroups( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaImmunity( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaImmunityGroups( int index,  bool svr_command);
			PLUGIN_RESULT	ProcessMaTimeLeft( int index, bool svr_command);
			PLUGIN_RESULT	ProcessMaAutoKickBanName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_name, char *ban_time_string, bool kick);
			PLUGIN_RESULT	ProcessMaAutoKickBanPName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_pname, char *ban_time_string, bool kick);
			PLUGIN_RESULT	ProcessMaAutoKickSteam( int index,  bool svr_command,  int argc,  char *command_string,  char *player_steam_id);
			PLUGIN_RESULT	ProcessMaAutoKickIP( int index,  bool svr_command,  int argc,  char *command_string,  char *player_ip_address);
			PLUGIN_RESULT	ProcessMaUnAutoKickBanName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_name, bool kick);
			PLUGIN_RESULT	ProcessMaUnAutoKickBanPName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_pname, bool kick);
			PLUGIN_RESULT	ProcessMaUnAutoKickSteam( int index,  bool svr_command,  int argc,  char *command_string,  char *player_steam_id);
			PLUGIN_RESULT	ProcessMaUnAutoKickIP( int index,  bool svr_command,  int argc,  char *command_string,  char *player_ip_address);
			void			WriteNameList(char *filename_string);
			void			WritePNameList(char *filename_string);
			void			WriteIPList(char *filename_string);
			void			WriteSteamList(char *filename_string);
			void			ParseSayString(const char *say_string, char *trimmed_string_out, int *say_argc);
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

private:

	int m_iClientCommandIndex;
	rcon_t		*rcon_list;
	vote_rcon_t	*vote_rcon_list;
	vote_question_t	*vote_question_list;
	map_vote_t	map_vote[MANI_MAX_PLAYERS];
	reserve_slot_t	*reserve_slot_list;
	ping_immunity_t	*ping_immunity_list;
	active_player_t	*active_player_list;
	map_t		*user_vote_map_list;

	swear_t		*swear_list;

	cexec_t		*cexec_list;
	cexec_t		*cexec_t_list;
	cexec_t		*cexec_ct_list;
	cexec_t		*cexec_spec_list;
	cexec_t		*cexec_all_list;

	autokick_ip_t		*autokick_ip_list;
	autokick_steam_t	*autokick_steam_list;
	autokick_name_t		*autokick_name_list;
	autokick_pname_t	*autokick_pname_list;
	lang_trans_t		*lang_trans_list;

	gimp_t				*gimp_phrase_list;

	int	rcon_list_size;
	int	vote_rcon_list_size;
	int	vote_question_list_size;
	int	swear_list_size;
	int	reserve_slot_list_size;
	int	ping_immunity_list_size;
	int	active_player_list_size;
	int user_vote_map_list_size;

	int	cexec_list_size;
	int	cexec_t_list_size;
	int	cexec_ct_list_size;
	int	cexec_spec_list_size;
	int	cexec_all_list_size;

	int	autokick_ip_list_size;
	int	autokick_steam_list_size;
	int	autokick_name_list_size;
	int	autokick_pname_list_size;

	int	lang_trans_list_size;

	int gimp_phrase_list_size;

	float	map_start_time;
	float	last_cheat_check_time;

	bool vote_started;
	int	level_changed;
	int	message_type;
	float	test_val;
	time_t	last_stats_write_time;


};

// 
// The plugin is a static singleton that is exported as an interface
//
// Don't forget to make an instance
CAdminPlugin g_ManiAdminPlugin;
IServerPluginCallbacks *gpManiAdminPlugin;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAdminPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_ManiAdminPlugin );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CAdminPlugin::CAdminPlugin()
{
	m_iClientCommandIndex = 0;
	vote_started = false;
	rcon_list = NULL;
	rcon_list_size = 0;
	vote_rcon_list = NULL;
	vote_rcon_list_size = 0;
	vote_question_list = NULL;
	vote_question_list_size = 0;

	round_number = 0;
	swear_list = NULL;
	swear_list_size = 0;
	target_player_list = NULL;
	target_player_list_size = 0;
	
	cexec_list = NULL;
	cexec_t_list = NULL;
	cexec_ct_list = NULL;
	cexec_spec_list = NULL;
	cexec_all_list = NULL;

	cexec_list_size = 0;
	cexec_t_list_size = 0;
	cexec_ct_list_size = 0;
	cexec_spec_list_size = 0;
	cexec_all_list_size = 0;

	reserve_slot_list = NULL;
	reserve_slot_list_size = 0;
	ping_immunity_list = NULL;
	ping_immunity_list_size = 0;
	active_player_list = NULL;
	active_player_list_size = 0;

	autokick_ip_list = NULL;
	autokick_steam_list = NULL;
	autokick_name_list = NULL;
	autokick_pname_list = NULL;

	autokick_ip_list_size = 0;
	autokick_steam_list_size = 0;
	autokick_name_list_size = 0;
	autokick_pname_list_size = 0;

	lang_trans_list = NULL;
	lang_trans_list_size = 0;

	user_vote_map_list = NULL;
	user_vote_map_list_size = 0;

	gimp_phrase_list = NULL;
	gimp_phrase_list_size = 0;

	vote_option_list = NULL;
	vote_option_list_size = 0;

	war_mode = false;
	InitCheatPingList();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		disconnected_player_list[i].in_use = false;
		average_ping_list[i].in_use = false;
		check_ping_list[i].in_use = false;
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;

		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = 0;
		user_vote_list[i].kick_vote_timestamp = 0;
		user_vote_list[i].nominate_timestamp = 0;
		user_vote_list[i].map_vote_timestamp = 0;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;

		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	time(&last_stats_write_time);
	next_ping_check = 0.0;
	message_type = 0;
	test_val = 0;
	just_loaded = true;
	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;

	change_team = false;
	timeleft_offset = 0;
	get_new_timeleft_offset = true;
	trigger_changemap = false;
	round_end_found = false;

	InitMaps();
	InitPlayerSettingsLists();
	gpManiAdminPlugin = this;
}

CAdminPlugin::~CAdminPlugin()
{
}

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
		Q_snprintf(new_interface, sizeof(new_interface), "%s%03i", interface_version, j); \
		_store = (_type *) _interface_type (new_interface, NULL); \
		if(_store) \
		{ \
			Q_strcpy(interface_list[interface_list_size - 1].interface_name, new_interface); \
			break; \
		} \
	} \
	interface_list[interface_list_size - 1].ptr = (char *) _store; \
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CAdminPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	Msg("********************************************************\n");
	Msg(" Loading ");
	Msg("%s\n", mani_version);
	Msg("\n");

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
	GET_INTERFACE(cvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION, 20, interfaceFactory)
	GET_INTERFACE(serverdll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL, 20, gameServerFactory)
	GET_INTERFACE(voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER, 20, interfaceFactory)

	gpGlobals = playerinfomanager->GetGlobalVars();
	InitCVars( interfaceFactory ); // register any cvars we have defined

	// Show interfaces that worked
	for (int i = 0; i < interface_list_size; i++)
	{
		if (interface_list[i].ptr)
		{
			if (strcmp(interface_list[i].base_interface, interface_list[i].interface_name) == 0)
			{
				// Base interface used
				Msg("%p SDK %s\n", interface_list[i].ptr, interface_list[i].base_interface);
			}
			else
			{
				Msg("%p SDK %s => Upgraded to %s\n", interface_list[i].ptr, interface_list[i].base_interface, interface_list[i].interface_name);
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
				Msg("FAILED !! : %s\n", interface_list[i].base_interface);
				found_failure = true;
			}
		}
	}

	FreeList((void **) &interface_list, &interface_list_size);

	if (found_failure) 
	{
		Msg("Failure on loading interface, quitting plugin load\n");
		return false;
	}

	Msg("********************************************************\n");



	LoadWeapons("Unknown");
	gpManiGameType->Init();

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{

#ifdef __linux__
		void	*handle;
		void	*var_address;

		handle = dlopen(gpManiGameType->GetLinuxBin(), RTLD_NOW);
	
		if (handle == NULL)
		{
			Msg("Failed to open server image, error [%s]\n", dlerror());
			gpManiGameType->SetAdvancedEffectsAllowed(false);
		}
		else
		{ 
			Msg("Program Start at [%p]\n", handle);

			var_address = dlsym(handle, "te");
			if (var_address == NULL)
			{
				Msg("dlsym failure : Error [%s]\n", dlerror());
				gpManiGameType->SetAdvancedEffectsAllowed(false);
			}
			else
			{
				Msg("var_address = %p\n", var_address);
				temp_ents = *(ITempEntsSystem **) var_address;
			}

			dlclose(handle);
		}
#else
		temp_ents = **(ITempEntsSystem***)(VFN2(effects, gpManiGameType->GetAdvancedEffectsVFuncOffset()) + (gpManiGameType->GetAdvancedEffectsCodeOffset()));
#endif
	}



	const char *game_type = serverdll->GetGameDescription();

	Msg("Game Type [%s]\n", game_type);

	vote_started = false;

	InitCheatPingList();
	UpdateCurrentPlayerList();
	gpManiVictimStats->RoundStart();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		average_ping_list[i].in_use = false;
		disconnected_player_list[i].in_use = false;
		menu_confirm[i].in_use = false;
		check_ping_list[i].in_use = false;
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;

		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;
		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = 0;
		user_vote_list[i].kick_vote_timestamp = 0;
		user_vote_list[i].nominate_timestamp = 0;
		user_vote_list[i].map_vote_timestamp = 0;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;

		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");

	}

	next_ping_check = 0.0;
	mp_friendlyfire = cvar->FindVar( "mp_friendlyfire");
	mp_freezetime = cvar->FindVar( "mp_freezetime");
	mp_winlimit = cvar->FindVar( "mp_winlimit");
	mp_maxrounds = cvar->FindVar( "mp_maxrounds");
	mp_timelimit = cvar->FindVar( "mp_timelimit");
	mp_fraglimit = cvar->FindVar( "mp_fraglimit");
	mp_limitteams = cvar->FindVar( "mp_limitteams");
	mp_restartgame = cvar->FindVar( "mp_restartgame");
	sv_lan = cvar->FindVar( "sv_lan");
	sv_gravity = cvar->FindVar( "sv_gravity");
	cs_stacking_num_levels = cvar->FindVar( "cs_stacking_num_levels");

	sv_cheats = cvar->FindVar( "sv_cheats");
	sv_alltalk = cvar->FindVar( "sv_alltalk");
	hostname = cvar->FindVar( "hostname");
	phy_pushscale = cvar->FindVar( "phys_pushscale");

	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;
	change_team = false;
	trigger_changemap = false;
	Q_strcpy(custom_map_config,"");

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	char	message_name[1024];
	int	msg_size;


	message_type = 0;
/*	while (serverdll->GetUserMessageInfo(message_type, message_name, sizeof(message_name), (int &) msg_size))
	{
		Msg("Msg [%i] Name [%s] size[%i]\n", message_type, message_name, msg_size);
		message_type ++;
		if (message_type > 20)
		{
			while(1);
		}
	}
*/

	for (int i = 0; i < gpManiGameType->GetMaxMessages(); i++)
	{
		serverdll->GetUserMessageInfo(i, message_name, sizeof(message_name), (int &) msg_size);
		if (FStrEq(message_name, "ShowMenu")) 
		{
			menu_message_index = i;
			Msg("Hooked ShowMenu [%i]\n", i);
		}
		else if (FStrEq(message_name, "TextMsg"))
		{
			text_message_index = i;
			Msg("Hooked TextMsg [%i]\n", i);
		}
		else if (FStrEq(message_name, "Fade"))
		{
			fade_message_index = i;
			Msg("Hooked Fade [%i]\n", i);
		}
		else if (FStrEq(message_name, "VGUIMenu"))
		{
			vgui_message_index = i;
			Msg("Hooked VGUIMenu [%i]\n", i);
		}
		else if (FStrEq(message_name, "SayText2"))
		{
			saytext2_message_index = i;
			Msg("Hooked SayText2 [%i]\n", i);
		}
		else if (FStrEq(message_name, "SayText"))
		{
			saytext_message_index = i;
			Msg("Hooked SayText [%i]\n", i);
		}
		else if (FStrEq(message_name, "RadioText"))
		{
			radiotext_message_index = i;
			Msg("Hooked RadioText [%i]\n", i);
		}
		else if (FStrEq(message_name, "HudMsg"))
		{
			hudmsg_message_index = i;
			Msg("Hooked HudMsg [%i]\n", i);
		}
	}

	timeleft_offset = 0;
	get_new_timeleft_offset = false;
	round_end_found = false;

	SetPluginPausedStatus(false);
	InitEffects();
	InitTKPunishments();
	gpManiDownloads->Init();

	if (first_map_loaded)
	{
		if (!LoadLanguage())
		{
			return false;
		}

		LoadAdmins();
		LoadImmunity();
		InitPanels();	// Fails if plugin loaded on server startup but placed here in case user runs plugin_load
		ResetActivePlayers();
		LoadQuakeSounds();
		LoadCronTabs();
		LoadAdverts();
		LoadMaps("Unknown"); //host_maps cvar is borked :/
		LoadWebShortcuts();
		ResetLogCount();
		LoadCheatList();
		LoadCommandList();
		LoadSounds();
		LoadSkins();
		SkinResetTeamID();
		FreeTKPunishments();
		gpManiGhost->Init();
		gpManiCustomEffects->Init();
		gpManiMapAdverts->Init();
		gpManiSprayRemove->Load();
	}


	// Work out server tickrate
#ifdef __linux__
	server_tickrate = (int) ((1.0 / serverdll->GetTickInterval()) + 0.5);
#else
	server_tickrate = (int) (1.0 / serverdll->GetTickInterval());
#endif
	mani_tickrate.SetValue(server_tickrate);

	// Hook our changelevel here
	//HOOKVFUNC(engine, 0, OrgEngineChangeLevel, ManiChangeLevelHook);
//	HOOKVFUNC(engine, 43, OrgEngineEntityMessageBegin, ManiEntityMessageBeginHook);
	//HOOKVFUNC(engine, 44, OrgEngineMessageEnd, ManiMessageEndHook);

	if (voiceserver && gpManiGameType->IsVoiceAllowed())
	{
		Msg("Hooking voiceserver\n");
		HOOKVFUNC(voiceserver, gpManiGameType->GetVoiceOffset(), voiceserver_SetClientListening, mysetclientlistening);
	}

	if (effects && gpManiGameType->GetAdvancedEffectsAllowed())
	{
		HOOKVFUNC(temp_ents, gpManiGameType->GetSprayHookOffset(), te_PlayerDecal, myplayerdecal);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CAdminPlugin::Unload( void )
{
	// Free up any lists being used
	FreeAdmins ();
	FreeImmunity ();
	FreeMenu();
	FreeCronTabs();
	FreeAdverts();
	FreeWebShortcuts();
	FreeMaps();
	FreePlayerSettings();
	FreePlayerNameSettings();
	FreeStats();
	FreeCommandList();
	FreeSounds();
	FreeSkins();
	FreeTeamList();
	FreeTKPunishments();

	FreeList((void **) &rcon_list, &rcon_list_size);
	FreeList((void **) &vote_rcon_list, &vote_rcon_list_size);
	FreeList((void **) &vote_question_list, &vote_question_list_size);

	FreeList((void **) &tk_player_list, &tk_player_list_size);
	FreeList((void **) &swear_list, &swear_list_size);

	FreeList((void **) &cexec_list, &cexec_list_size);
	FreeList((void **) &cexec_t_list, &cexec_t_list_size);
	FreeList((void **) &cexec_ct_list, &cexec_ct_list_size);
	FreeList((void **) &cexec_spec_list, &cexec_spec_list_size);
	FreeList((void **) &cexec_all_list, &cexec_all_list_size);
	FreeList((void **) &target_player_list, &target_player_list_size);

	FreeList((void **) &reserve_slot_list, &reserve_slot_list_size);
	FreeList((void **) &ping_immunity_list, &ping_immunity_list_size);

	FreeList ((void **) &autokick_ip_list, &autokick_ip_list_size);
	FreeList ((void **) &autokick_steam_list, &autokick_steam_list_size);
	FreeList ((void **) &autokick_name_list, &autokick_name_list_size);
	FreeList ((void **) &autokick_pname_list, &autokick_pname_list_size);
	FreeList ((void **) &gimp_phrase_list, &gimp_phrase_list_size);
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);
	FreeList ((void **) &cheat_cvar_list, &cheat_cvar_list_size);
	FreeList ((void **) &cheat_cvar_list2, &cheat_cvar_list_size2);

	trigger_changemap = false;
	FreeLanguage();

	gameeventmanager->RemoveListener( this ); // make sure we are unloaded from the event system
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CAdminPlugin::Pause( void )
{
	SayToAll(true, "Mani Admin Plugin is paused");
	DirectLogCommand("[MANI_ADMIN_PLUGIN] Mani Admin Plugin is paused\n");
	SetPluginPausedStatus(true);
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CAdminPlugin::UnPause( void )
{
	SayToAll(true, "Mani Admin Plugin is un-paused");
	DirectLogCommand("[MANI_ADMIN_PLUGIN] Mani Admin Plugin is un-paused\n");
	SetPluginPausedStatus(false);
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CAdminPlugin::GetPluginDescription( void )
{
	return mani_version;
}



//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::LevelInit( char const *pMapName )
{
	FileHandle_t file_handle;
	char	swear_word[128];
	char	rcon_command[512];
	char	autokickban_id[512];
	char	steam_id[128];
	int		i;
	char	map_config_filename[256];
	char	base_filename[256];
	char	alias_command[512];
	char	gimp_phrase[256];
	char	question[512];
	int		total_load_index;

	// Reset game type info (mp_teamplay may have changed)
	LoadWeapons(pMapName);
	gpManiGameType->Init();
	total_load_index = ManiGetTimer();

	filesystem->PrintSearchPaths();

	Msg("mani_path = [%s]\n", mani_path.GetString());

	InitPanels();

	FreeList((void **) &rcon_list, &rcon_list_size);
	FreeList((void **) &vote_question_list, &vote_question_list_size);
	FreeList((void **) &vote_rcon_list, &vote_rcon_list_size);

	FreeList((void **) &tk_player_list, &tk_player_list_size);
	FreeList((void **) &swear_list, &swear_list_size);

	FreeList((void **) &cexec_list, &cexec_list_size);
	FreeList((void **) &cexec_t_list, &cexec_t_list_size);
	FreeList((void **) &cexec_ct_list, &cexec_ct_list_size);
	FreeList((void **) &cexec_spec_list, &cexec_spec_list_size);
	FreeList((void **) &cexec_all_list, &cexec_all_list_size);
	FreeList((void **) &target_player_list, &target_player_list_size);
	FreeList((void **) &reserve_slot_list, &reserve_slot_list_size);
	FreeList((void **) &ping_immunity_list, &ping_immunity_list_size);

	FreeList ((void **) &autokick_ip_list, &autokick_ip_list_size);
	FreeList ((void **) &autokick_steam_list, &autokick_steam_list_size);
	FreeList ((void **) &autokick_name_list, &autokick_name_list_size);
	FreeList ((void **) &autokick_pname_list, &autokick_pname_list_size);
	FreeList ((void **) &gimp_phrase_list, &gimp_phrase_list_size);
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);

	mp_friendlyfire = cvar->FindVar( "mp_friendlyfire");
	mp_freezetime = cvar->FindVar( "mp_freezetime");
	mp_winlimit = cvar->FindVar( "mp_winlimit");
	mp_maxrounds = cvar->FindVar( "mp_maxrounds");
	mp_timelimit = cvar->FindVar( "mp_timelimit");
	mp_fraglimit = cvar->FindVar( "mp_fraglimit");
	mp_limitteams = cvar->FindVar( "mp_limitteams");
	mp_restartgame = cvar->FindVar( "mp_restartgame");
	sv_lan = cvar->FindVar( "sv_lan");
	sv_gravity = cvar->FindVar( "sv_gravity");
	cs_stacking_num_levels = cvar->FindVar( "cs_stacking_num_levels");

	sv_cheats = cvar->FindVar( "sv_cheats");
	sv_alltalk = cvar->FindVar( "sv_alltalk");
	hostname = cvar->FindVar( "hostname");
	phy_pushscale = cvar->FindVar( "phys_pushscale");

	next_ping_check = 0.0;
	last_cheat_check_time = 0;
	last_slapped_player = -1;
	last_slapped_time = 0;
	timeleft_offset = 0;
	get_new_timeleft_offset = true;
	trigger_changemap = false;
	round_end_found = false;

	// Set plugin defaults in case they have been changed
	vote_started = false;
	round_number = 0;

	// Used for map config
	level_changed = true;

	for (i = 0; i < MANI_MAX_TEAMS; i++)
	{
		team_scores.team_score[i] = 0;
	}

	InitEffects();
	InitCheatPingList();
	SkinResetTeamID();
	gpManiDownloads->Init();
	gpManiGhost->Init();
	gpManiCustomEffects->Init();
	gpManiVictimStats->RoundStart();
	gpManiWarmupTimer->LevelInit();

	// Init votes and menu system
	for (i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		menu_confirm[i].in_use = false;
		disconnected_player_list[i].in_use = false;
		check_ping_list[i].in_use = false;
		average_ping_list[i].in_use = false;
		chat_flood[i] = -99;
		sounds_played[i] = 0;
		name_changes[i] = 0;
		tw_spam_list[i].last_time = -99.0;
		tw_spam_list[i].index = -99;

		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = -99;
		user_vote_list[i].kick_vote_timestamp = -99;
		user_vote_list[i].nominate_timestamp = -99;
		user_vote_list[i].map_vote_timestamp = -99;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;

		user_name[i].in_use = false;
		Q_strcpy(user_name[i].name,"");
	}

	change_team = false;

	system_vote.vote_in_progress = false;
	system_vote.map_decided = false;
	system_vote.start_rock_the_vote = false;
	system_vote.no_more_rock_the_vote = false;
	system_vote.number_of_extends = 0;

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		esounds->PrecacheSound(slay_sound_name, true);
		esounds->PrecacheSound(slap_sound_name[0].sound_name, true);
		esounds->PrecacheSound(slap_sound_name[1].sound_name, true);
		esounds->PrecacheSound(slap_sound_name[2].sound_name, true);
	}
	else
	{
		esounds->PrecacheSound(hl2mp_slay_sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[0].sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[1].sound_name, true);
		esounds->PrecacheSound(hl2mp_slap_sound_name[2].sound_name, true);
	}

	// Stuff for time bomb
	esounds->PrecacheSound(countdown_beep, true);
	esounds->PrecacheSound(final_beep, true);

	// Beacon sound
	esounds->PrecacheSound(beacon_sound, true);

	// Menu select noises
	esounds->PrecacheSound(menu_select_sound, true);
	esounds->PrecacheSound(menu_select_exit_sound, true);

	tp_beam_index = engine->PrecacheModel( "sprites/tp_beam001.vmt", true );
	plasmabeam_index = engine->PrecacheModel( "sprites/plasmabeam.vmt", true );
	lgtning_index = engine->PrecacheModel( "sprites/lgtning.vmt", true );
	explosion_index = engine->PrecacheModel( "sprites/sprite_fire01.vmt", true );
	orangelight_index = engine->PrecacheModel( "sprites/orangelight1.vmt", true );
	bluelight_index = engine->PrecacheModel( "sprites/bluelight1.vmt", true );
	purplelaser_index = engine->PrecacheModel( "sprites/purplelaser1.vmt", true );

	if (!first_map_loaded)
	{
		LoadLanguage();
		first_map_loaded = true;
		ResetLogCount();
	}

	// Required for vote time restriction
	map_start_time = gpGlobals->curtime;

	LoadAdmins();
	LoadImmunity();
	LoadQuakeSounds();
	LoadCronTabs();
	LoadAdverts();
	LoadMaps(pMapName);
	LoadWebShortcuts();
	LoadCheatList();
	LoadCommandList();
	LoadSounds();
	LoadSkins();
	FreeTKPunishments();
	gpManiMapAdverts->Init();
	gpManiSprayRemove->LevelInit();

	//Get reserve player list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/reserveslots.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load reserveslots.txt\n");
	}
	else
	{
		Msg("Reserve Slot list\n");
		while (filesystem->ReadLine (steam_id, sizeof(steam_id), file_handle) != NULL)
		{
			if (!ParseLine(steam_id, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &reserve_slot_list, sizeof(reserve_slot_t), &reserve_slot_list_size);
			Q_strcpy(reserve_slot_list[reserve_slot_list_size - 1].steam_id, steam_id);
			Msg("[%s]\n", steam_id);
		}

		qsort(reserve_slot_list, reserve_slot_list_size, sizeof(reserve_slot_t), sort_reserve_slots_by_steam_id); 
		filesystem->Close(file_handle);
	}

	//Get ping immunity player list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/pingimmunity.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load pingimmunity.txt\n");
	}
	else
	{
		Msg("Ping Immunity list\n");
		while (filesystem->ReadLine (steam_id, sizeof(steam_id), file_handle) != NULL)
		{
			if (!ParseLine(steam_id, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &ping_immunity_list, sizeof(ping_immunity_t), &ping_immunity_list_size);
			Q_strcpy(ping_immunity_list[ping_immunity_list_size - 1].steam_id, steam_id);
			Msg("[%s]\n", steam_id);
		}

		qsort(ping_immunity_list, ping_immunity_list_size, sizeof(ping_immunity_t), sort_ping_immunity_by_steam_id); 
		filesystem->Close(file_handle);
	}

	//Get gimp phrase list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/gimpphrase.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load gimpphrase.txt\n");
	}
	else
	{
		Msg("Gimp phrase list\n");
		while (filesystem->ReadLine (gimp_phrase, sizeof(gimp_phrase), file_handle) != NULL)
		{
			if (!ParseLine(gimp_phrase, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &gimp_phrase_list, sizeof(gimp_t), &gimp_phrase_list_size);
			Q_strcpy(gimp_phrase_list[gimp_phrase_list_size - 1].phrase, gimp_phrase);
			Msg("[%s]\n", gimp_phrase);
		}

		filesystem->Close(file_handle);
	}


	//Get swear word list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/wordfilter.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load wordfilter.txt\n");
	}
	else
	{
		Msg("Swearword list\n");
		while (filesystem->ReadLine (swear_word, sizeof(swear_word), file_handle) != NULL)
		{
			if (!ParseLine(swear_word, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &swear_list, sizeof(swear_t), &swear_list_size);
			// Convert to upper case
			Q_strupr(swear_word);
			Q_strcpy(swear_list[swear_list_size - 1].swear_word, swear_word);
			swear_list[swear_list_size - 1].length = Q_strlen(swear_word);
			Msg("[%s] ", swear_word);
		}

		Msg("\n");
		filesystem->Close(file_handle);
	}

	//Get Map Config for this map
	Q_snprintf( map_config_filename, sizeof(map_config_filename), "./cfg/%s/map_config/%s.cfg", mani_path.GetString(), pMapName);
	if (filesystem->FileExists(map_config_filename))
	{
		Q_snprintf(custom_map_config, sizeof(custom_map_config), "exec ./%s/map_config/%s.cfg\n", mani_path.GetString(), pMapName);
		Msg("Custom map config [%s] found for this map\n", custom_map_config);
	}
	else
	{
		Msg("No custom map config found for this map\n");
		Q_strcpy(custom_map_config,"");
	}

	//Get rcon list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/rconlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load rconlist.txt\n");
	}
	else
	{
		Msg("rcon list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &rcon_list, sizeof(rcon_t), &rcon_list_size);
			Q_strcpy(rcon_list[rcon_list_size - 1].rcon_command, rcon_command);
			Q_strcpy(rcon_list[rcon_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get rcon vote list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/voterconlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load voterconlist.txt\n");
	}
	else
	{
		Msg("Vote RCON List\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine2(rcon_command, alias_command, question,  true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &vote_rcon_list, sizeof(vote_rcon_t), &vote_rcon_list_size);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].rcon_command, rcon_command);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].alias, alias_command);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].question, question);
			Msg("Menu Alias[%s] Question [%s] Command[%s]\n", alias_command, question, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get question vote list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/votequestionlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load votequestionlist.txt\n");
	}
	else
	{
		Msg("Vote Question List\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine3(rcon_command, alias_command, question,  true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &vote_question_list, sizeof(vote_question_t), &vote_question_list_size);
			Q_strcpy(vote_question_list[vote_question_list_size - 1].alias, alias_command);
			Q_strcpy(vote_question_list[vote_question_list_size - 1].question, question);
			Msg("Menu Alias[%s] Question [%s]\n", alias_command, question); 
		}

		filesystem->Close(file_handle);
	}

	//Get single player cexec list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_player.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load cexeclist_player.txt\n");
	}
	else
	{
		Msg("cexeclist_player list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_list, sizeof(cexec_t), &cexec_list_size);
			Q_strcpy(cexec_list[cexec_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_list[cexec_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get all cexec list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_all.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load cexeclist_all.txt\n");
	}
	else
	{
		Msg("cexeclist_all list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_all_list, sizeof(cexec_t), &cexec_all_list_size);
			Q_strcpy(cexec_all_list[cexec_all_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_all_list[cexec_all_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get t cexec list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_t.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load cexeclist_t.txt\n");
	}
	else
	{
		Msg("cexeclist_t list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_t_list, sizeof(cexec_t), &cexec_t_list_size);
			Q_strcpy(cexec_t_list[cexec_t_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_t_list[cexec_t_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get ct cexec list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_ct.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load cexeclist_ct.txt\n");
	}
	else
	{
		Msg("cexeclist_ct list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_ct_list, sizeof(cexec_t), &cexec_ct_list_size);
			Q_strcpy(cexec_ct_list[cexec_ct_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_ct_list[cexec_ct_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get spec cexec list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cexeclist_spec.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load cexeclist_spec.txt\n");
	}
	else
	{
		Msg("cexeclist_spec list\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine(rcon_command, alias_command, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &cexec_spec_list, sizeof(cexec_t), &cexec_spec_list_size);
			Q_strcpy(cexec_spec_list[cexec_spec_list_size - 1].cexec_command, rcon_command);
			Q_strcpy(cexec_spec_list[cexec_spec_list_size - 1].alias, alias_command);
			Msg("Alias[%s] Command[%s]\n", alias_command, rcon_command); 
		}

		filesystem->Close(file_handle);
	}

	//Get autokickban IP list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_ip.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Did not load autokick_ip.txt\n");
	}
	else
	{
		Msg("autokickban IP list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, false)) continue;
			AddAutoKickIP(autokickban_id);
		}
		filesystem->Close(file_handle);
		qsort(autokick_ip_list, autokick_ip_list_size, sizeof(autokick_ip_t), sort_autokick_ip); 
	}

	//Get autokickban Steam list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_steam.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Did not load autokick_steam.txt\n");
	}
	else
	{
		Msg("autokickban Steam list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, false)) continue;
			AddAutoKickSteamID(autokickban_id);
		}
		filesystem->Close(file_handle);
		qsort(autokick_steam_list, autokick_steam_list_size, sizeof(autokick_steam_t), sort_autokick_steam); 
	}

	//Get autokickban Name list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_name.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Did not load autokick_name.txt\n");
	}
	else
	{
		Msg("autokickban Name list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, false)) continue;
			AddAutoKickName(autokickban_id);
		}
		filesystem->Close(file_handle);
	}

	//Get autokickban PName list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_pname.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Did not load autokick_pname.txt\n");
	}
	else
	{
		Msg("autokickban PName list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, false)) continue;
			AddAutoKickPName(autokickban_id);
		}
		filesystem->Close(file_handle);
	}

	int timer_index = ManiGetTimer();
	LoadStats();
	Msg("Stats Loaded in %.4f seconds\n", ManiGetTimerDuration(timer_index));

	just_loaded = false;
	ProcessBuildUserVoteMaps();

	timer_index = ManiGetTimer();
	ProcessPlayerSettings();
	Msg("Player Lists Loaded in %.4f seconds\n", ManiGetTimerDuration(timer_index));

	gameeventmanager->AddListener( this, "weapon_fire", true );
	gameeventmanager->AddListener( this, "round_start", true );
	gameeventmanager->AddListener( this, "round_end", true );
	gameeventmanager->AddListener( this, "round_freeze_end", true );
	gameeventmanager->AddListener( this, "player_hurt", true );
	gameeventmanager->AddListener( this, "player_team", true );
	gameeventmanager->AddListener( this, "player_death", true );
	gameeventmanager->AddListener( this, "player_say", true );
//		gameeventmanager->AddListener( this, "player_changename", true );
	gameeventmanager->AddListener( this, "player_spawn", true );
	gameeventmanager->AddListener( this, "player_team", true );

	Msg("********************************************************\n");
	Msg(" Mani Admin Plugin Level Init Time = %.3f seconds\n", ManiGetTimerDuration(total_load_index));
	Msg("********************************************************\n");

}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CAdminPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	max_players = clientMax; 
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		average_ping_list[i].in_use = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CAdminPlugin::GameFrame( bool simulating )
{
/*static float start_time = 0.0;

	if (gpGlobals->curtime >= start_time)
	{
		Msg("curtime = [%f]\n", gpGlobals->curtime);
		start_time += 1.0;
	}
*/

	if (ProcessPluginPaused())
	{
		return;
	}

	gpManiSprayRemove->GameFrame();
	gpManiWarmupTimer->GameFrame();

	if (mani_protect_against_cheat_cvars.GetInt() == 1)
	{
		ProcessCheatCVars();
	}

	if (war_mode && mani_war_mode_force_overview_zero.GetInt() == 1)
	{
		TurnOffOverviewMap();
	}

	if (mani_stats.GetInt() == 1 && mani_stats_write_to_disk_frequency.GetInt() != 0)
	{
		time_t	current_time;

		time(&current_time);
		if (last_stats_write_time + (mani_stats_write_to_disk_frequency.GetInt() * 60) < current_time)
		{
			Msg("Calculating current stats list\n");
			time(&last_stats_write_time);

			if (mani_stats_by_steam_id.GetInt() == 1)
			{
				ReBuildStatsPlayerList();
				CalculateStats(false);
				WriteStats();
			}
			else
			{
				ReBuildStatsPlayerNameList();
				CalculateNameStats(false);
				WriteNameStats();
			}
		}
	}

	if (war_mode) return;

	if (change_team && change_team_time < gpGlobals->curtime && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		change_team = false;
		ProcessMaBalance(0, true, true);
	}

	if (mani_high_ping_kick.GetInt() == 1)
	{
		ProcessHighPingKick();
	}

	ProcessAdverts();
	ProcessInGamePunishments();

	if (trigger_changemap && gpGlobals->curtime >= trigger_changemap_time)
	{
		char	server_cmd[512];
		
		trigger_changemap = false;
//		engine->ChangeLevel(next_map, NULL);
		Q_snprintf(server_cmd, sizeof(server_cmd), "changelevel %s\n", next_map);
		engine->ServerCommand(server_cmd);
	}

	if (system_vote.vote_in_progress)
	{
		if (!system_vote.waiting_decision)
		{
			if (gpGlobals->curtime > system_vote.end_vote_time)
			{
				ProcessVotes();
			}
		}
		else
		{
			if (gpGlobals->curtime > system_vote.waiting_decision_time)
			{
				player_t player;
				
				player.entity = NULL;
				player.index = system_vote.vote_starter;
				FindPlayerByIndex(&player);
				ProcessVoteConfirmation(&player, true);
			}
		}
	}

	if (!system_vote.vote_in_progress && !system_vote.map_decided)
	{
		// We can run end of map votes
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1)
		{
			if (mp_timelimit && mp_timelimit->GetInt() != 0)
			{
				// Check timeleft
				float time_left = (mp_timelimit->GetFloat() * 60) - (gpGlobals->curtime - timeleft_offset);
				if (time_left < mani_vote_time_before_end_of_map_vote.GetFloat() * 60)
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
					}

					StartSystemVote();
					system_vote.vote_in_progress = true;
				}
			}
		}
	}

	if (!system_vote.vote_in_progress && !system_vote.map_decided)
	{
		// We can run end of map votes
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1)
		{
			if (mp_winlimit && mp_winlimit->GetInt() != 0)
			{
				// Check win limit threshold about to be broken
				int highest_score = 0;
				for (int i = 0; i < MANI_MAX_TEAMS; i++)
				{
					if (team_scores.team_score[i] > highest_score) highest_score = team_scores.team_score[i];
				}

				if ((mp_winlimit->GetInt() - highest_score) <= mani_vote_rounds_before_end_of_map_vote.GetInt())
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
					}

					StartSystemVote();
					system_vote.vote_in_progress = true;
				}
			}
		}
	}

	if (!system_vote.vote_in_progress && !system_vote.map_decided)
	{
		// We can run end of map votes
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1)
		{
			if (mp_maxrounds && mp_maxrounds->GetInt() != 0)
			{
				// Check win limit threshold about to be broken
				int total_rounds = 0;
				for (int i = 0; i < MANI_MAX_TEAMS; i++)
				{
					total_rounds += team_scores.team_score[i];
				}

				if ((mp_maxrounds->GetInt() - total_rounds) <= mani_vote_rounds_before_end_of_map_vote.GetInt())
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
					}

					StartSystemVote();
					system_vote.vote_in_progress = true;
				}
			}
		}
	}

	if ( system_vote.start_rock_the_vote &&
		!system_vote.no_more_rock_the_vote && 
		!system_vote.vote_in_progress &&
		!system_vote.map_decided)
	{
		// Run rock the vote
		system_vote.start_rock_the_vote = false;
		ProcessStartRockTheVote();
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CAdminPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gameeventmanager->RemoveListener( this );
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientActive( edict_t *pEntity )
{
//  Msg("Mani -> Client Active\n");

	if (ProcessPluginPaused())
	{
		return;
	}

	player_t player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return;
	if (player.player_info->IsHLTV()) return;
	if (strcmp(player.player_info->GetNetworkIDString(),"BOT") == 0) return;

	gpManiGhost->PlayerConnect(&player);
	gpManiVictimStats->PlayerConnect(&player);
	gpManiMapAdverts->PlayerConnect(&player);

	check_ping_list[player.index - 1].in_use = true;
	average_ping_list[player.index - 1].in_use = false;

	if (!player.is_bot)
	{
		user_name[player.index - 1].in_use = true;
		Q_strcpy(user_name[player.index - 1].name, engine->GetClientConVarValue(player.index, "name"));

		// Player settings
		PlayerJoinedInitSettings(&player);
		cheat_ping_list[player.index - 1].connected = true;
		cheat_ping_list[player.index - 1].waiting_for_control = false;
		cheat_ping_list[player.index - 1].waiting_for_cvar = false;
		cheat_ping_list[player.index - 1].waiting_to_send = true;
		cheat_ping_list[player.index - 1].count = true;
	}

	if (ProcessAutoKick(&player))
	{
		// Player was let through
		if (mani_reserve_slots.GetInt() == 1)
		{
			if (!ProcessReserveSlotJoin (&player))
			{
				// Joining player was kicked so return immediately
				return ;
			}
		}
	}

	if (!player.is_bot)
	{
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_JOINSERVER);
	}
}



//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientDisconnect( edict_t *pEntity )
{
	player_t player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	gpManiSprayRemove->ClientDisconnect(&player);

	// Handle player settings
	PlayerSettingsDisconnect(&player);
	SkinPlayerDisconnect(&player);
	ProcessTKDisconnected (&player);
	gpManiGhost->PlayerDisconnect(&player);
	gpManiVictimStats->PlayerDisconnect(&player);

	user_name[player.index - 1].in_use = false;
	Q_strcpy(user_name[player.index - 1].name,"");
	name_changes[player.index - 1] = 0;

	cheat_ping_list[player.index - 1].connected = false;

	if (player.is_bot)
	{
		return;
	}

	disconnected_player_list[player.index - 1].in_use = false;
	check_ping_list[player.index - 1].in_use = false;
	average_ping_list[player.index - 1].in_use = false;
	EffectsClientDisconnect(player.index - 1, false);

	voter_list[player.index - 1].allowed_to_vote = false;

	// If vote starter drops out, just confirm the vote without them
	if (system_vote.vote_starter != -1 && system_vote.vote_in_progress)
	{
		if (player.index == system_vote.vote_starter)
		{
			system_vote.vote_confirmation = false;
		}
	}

	if (mani_tk_protection.GetInt() == 1)
	{
		if (sv_lan && sv_lan->GetInt() == 1)
		{
			// Reset user id if in list 
			for (int i = 0; i < tk_player_list_size; i++)
			{
				if (tk_player_list[i].user_id == player.user_id)
				{
					tk_player_t temp_tk_player;
					tk_player_t *temp_tk_player_ptr;

					if (i != tk_player_list_size - 1)
					{
						temp_tk_player = tk_player_list[tk_player_list_size - 1];
						tk_player_list[tk_player_list_size - 1] = tk_player_list[i];
						tk_player_list[i] = temp_tk_player;
					}

					if (tk_player_list_size != 1)
					{
						// Shrink array by 1
						temp_tk_player_ptr = (tk_player_t *) realloc(tk_player_list, (tk_player_list_size - 1) * sizeof(tk_player_t));
						tk_player_list = temp_tk_player_ptr;
						tk_player_list_size --;
					}
					else
					{
						free(tk_player_list);
						tk_player_list = NULL;
						tk_player_list_size = 0;
					}

					break;
				}
			}
		}
	}

	user_vote_list[player.index - 1].nominated_map = -1;
	user_vote_list[player.index - 1].nominate_timestamp = -99;
	user_vote_list[player.index - 1].rock_the_vote = false;

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_map.GetInt() == 1)
	{
		// De-vote map for player
		user_vote_list[player.index - 1].map_index = -1;
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());
		for (int i = 0; i <= user_vote_map_list_size; i++)
		{
			int votes_counted = 0;

			for (int j = 0; j < max_players; j++)
			{
				if (user_vote_list[j].map_index == i)
				{
					votes_counted ++;
				}
			}

			if (votes_counted >= votes_required)
			{
				ProcessUserVoteMapWin(i);
				SayToAll(true,"Player leaving server triggered vote completion");
				break;
			}
		}
	}

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_kick.GetInt() == 1)
	{
		// De-kick vote player

		if (user_vote_list[player.index - 1].kick_id != -1)
		{
			player_t target_player;

			target_player.user_id = user_vote_list[player.index - 1].kick_id;
			if (FindPlayerByUserID(&target_player))
			{
				if (!target_player.is_bot)
				{
					if (user_vote_list[target_player.index - 1].kick_votes > 0)
					{
						user_vote_list[target_player.index - 1].kick_votes --;
					}
				}
			}
		}

		user_vote_list[player.index - 1].kick_votes = 0;
		user_vote_list[player.index - 1].kick_id = -1;

		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].kick_id == player.user_id)
			{
				user_vote_list[i].kick_id = -1;
			}
		}
			
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());
		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].kick_votes >= votes_required)
			{
				player_t server_player;
				server_player.index = i + 1;
				if (!FindPlayerByIndex(&server_player)) continue;

				ProcessUserVoteKickWin(&server_player);
				SayToAll(true,"Player leaving server triggered vote kick");
				break;
			}
		}
	}

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_ban.GetInt() == 1)
	{
		// De-ban vote player
		if (user_vote_list[player.index - 1].ban_id != -1)
		{
			player_t target_player;

			target_player.user_id = user_vote_list[player.index - 1].ban_id;
			if (FindPlayerByUserID(&target_player))
			{
				if (!target_player.is_bot)
				{
					if (user_vote_list[target_player.index - 1].ban_votes > 0)
					{
						user_vote_list[target_player.index - 1].ban_votes --;
					}
				}
			}
		}

		user_vote_list[player.index - 1].ban_votes = 0;
		user_vote_list[player.index - 1].ban_id = -1;

		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].ban_id == player.user_id)
			{
				user_vote_list[i].ban_id = -1;
			}
		}
			
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());
		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].ban_votes >= votes_required)
			{
				player_t server_player;
				server_player.index = i + 1;
				if (!FindPlayerByIndex(&server_player)) continue;

				ProcessUserVoteBanWin(&server_player);
				SayToAll(true,"Player leaving server triggered vote ban");
				break;
			}
		}
	}

	menu_confirm[player.index - 1].in_use = false;
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
//  Msg("Client [%s] Put in Server\n",playername );

	if (level_changed)
	{
		level_changed = false;

		if (ProcessPluginPaused())	return;

		if (mani_tk_protection.GetInt() == 1 && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			// Force tk punish off (required for plugin tk punishment)
			engine->ServerCommand("mp_tkpunish 0\n");
			if (mani_tk_spawn_time.GetInt() != 0)
			{
				engine->ServerCommand("mp_spawnprotectiontime 0\n");
			}
		}

		// Execute any pre-map configs
		ExecuteCronTabs(false);

		if (custom_map_config)
		{
			if (!FStrEq(custom_map_config,""))
			{
				Msg("Executing %s\n", custom_map_config);
				engine->ServerCommand(custom_map_config);
			}
		}

		// Execute any post-map configs
		ExecuteCronTabs(true);
	}	
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::SetCommandClient( int index )
{
	m_iClientCommandIndex = con_command_index = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdminPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	if ( playerinfomanager )
	{
		int	player_index = engine->IndexOfEdict(pEdict);

		if (user_name[player_index - 1].in_use)
		{
			const char * name = engine->GetClientConVarValue(player_index, "name" );
			if (strcmp(user_name[player_index - 1].name, name) != 0)
			{
				player_t player;
				player.index = player_index;
				if (FindPlayerByIndex(&player))
				{
					if (!player.is_bot)
					{
						// Handle name change
						PlayerJoinedInitSettings(&player);
						ProcessChangeName(&player, name, user_name[player_index - 1].name);
						Q_strcpy(user_name[player_index - 1].name, name);
					}
				}
			}
		}

	}
	return;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
//	if (!ProcessPluginPaused())
//	{
//		Msg("Mani -> Client Connected [%s] [%s]\n", pszName, engine->GetPlayerNetworkIDString( pEntity ));
//	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ClientCommand( edict_t *pEntity )
{
	const char *pcmd = engine->Cmd_Argv(0);
	const char *pcmd2 = engine->Cmd_Argv(1);
	const char *pcmd3 = engine->Cmd_Argv(2);
	const char *say_string = engine->Cmd_Args();
	player_t	player;
	char	trimmed_say[2048]="";
	char	arg_string[2048];
	int		say_argc;

	if (ProcessPluginPaused())
	{
		return PLUGIN_CONTINUE;
	}

	int pargc = engine->Cmd_Argc();

	if ( !pEntity || pEntity->IsFree() ) return PLUGIN_CONTINUE;

	Q_snprintf(arg_string, sizeof (arg_string), "%s", engine->Cmd_Args());

/*	Msg("Argc = [%i]\n", engine->Cmd_Argc());
	Msg("Argv(0) = [%s]\n", engine->Cmd_Argv(0));
	Msg("Say String = [%s]\n", engine->Cmd_Args());

	for (int i = 0; i < engine->Cmd_Argc(); i++)
	{
		Msg("[%s] ", engine->Cmd_Argv(i));
	}

	Msg("\n");
*/
//	if (FStrEq(engine->Cmd_Argv(0),"jointeam"))
//	{
//		return PLUGIN_STOP;
//	}

	player.entity = pEntity;

	if (FStrEq(pcmd, "menuselect"))
	{
		if (pargc != 2)	return PLUGIN_STOP;

		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;

		if (menu_confirm[player.index - 1].in_use)
		{
			int command_index = Q_atoi(pcmd2);
			menu_confirm[player.index - 1].in_use = false;
			char client_cmd[256];

			// Exit pressed
			if (command_index == 10)
			{
				ProcessPlayMenuSound(&player, menu_select_exit_sound);
				return PLUGIN_STOP;
			}

			if (command_index == 0)	return PLUGIN_STOP;
			command_index --;
			// Found menu command we want to use, cheat by running the command from the client console 
			if (!menu_confirm[player.index - 1].menu_select[command_index].in_use)	return PLUGIN_STOP;

			Q_snprintf(client_cmd, sizeof(client_cmd), "%s\n", menu_confirm[player.index - 1].menu_select[command_index].command);
//			helpers->ClientCommand(pEntity, client_cmd);
			ProcessPlayMenuSound(&player, menu_select_sound);
			engine->ClientCommand(pEntity, "cmd %s\n", menu_confirm[player.index - 1].menu_select[command_index].command);
			return PLUGIN_STOP;
		}
	}
	else if ( FStrEq( pcmd, "admin" )) return (ProcessAdminMenu(pEntity));
	else if ( FStrEq( pcmd, "manisettings")) return (ProcessSettingsMenu(pEntity));
	else if ( FStrEq( pcmd, "votemap" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_map.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		ProcessMaUserVoteMap(&player, pargc, engine->Cmd_Args());
		return PLUGIN_STOP;
	}	
	else if ( FStrEq( pcmd, "votekick" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_kick.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_kick_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		ProcessMaUserVoteKick(&player, pargc, engine->Cmd_Args());
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "voteban" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_ban.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_ban_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		ProcessMaUserVoteBan(&player, pargc, engine->Cmd_Args());
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		ProcessMaRockTheVoteNominateMap(&player, pargc, engine->Cmd_Args());
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_rtvnominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return PLUGIN_CONTINUE;

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		ProcessMenuRockTheVoteNominateMap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotemapmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_map.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		ProcessMenuUserVoteMap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotekickmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_kick.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_kick_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		ProcessMenuUserVoteKick (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_uservotebanmenu" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
		if (mani_vote_allow_user_vote_ban.GetInt() == 0) return PLUGIN_CONTINUE;

		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
		// Stop ghosters from voting
		if (mani_vote_allow_user_vote_ban_ghost.GetInt() == 0 && gpManiGhost->IsGhosting(&player)) return PLUGIN_CONTINUE;

		ProcessMenuUserVoteBan(&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "favourites" ) && !war_mode)
	{
		int	next_index = 0;
		int argv_offset = 0;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;

		ProcessMenuMaFavourites (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq(pcmd, "buyammo1") || FStrEq(pcmd, "buyammo2") || FStrEq(pcmd, "buy"))
	{
		return (ProcessClientBuy(pcmd, pcmd2, pargc, &player));
	}
	else if ( FStrEq( pcmd, "nextmap" )) return (ProcessMaNextMap(engine->IndexOfEdict(pEntity), false));
	else if ( FStrEq( pcmd, "timeleft" )) return (ProcessMaTimeLeft(engine->IndexOfEdict(pEntity), false));
	else if ( FStrEq( pcmd, "listmaps" )) return (ProcessMaListMaps(engine->IndexOfEdict(pEntity), false));
	else if ( FStrEq( pcmd, "damage" )) return (ProcessMaDamage(engine->IndexOfEdict(pEntity)));
	else if ( FStrEq( pcmd, "deathbeam" )) return (ProcessMaDeathBeam(engine->IndexOfEdict(pEntity)));
	else if ( FStrEq( pcmd, "sounds" )) return (ProcessMaSounds(engine->IndexOfEdict(pEntity)));
	else if ( FStrEq( pcmd, "quake" )) return (ProcessMaQuake(engine->IndexOfEdict(pEntity)));	
	else if (FStrEq( pcmd, "maniadminversion" ) || FStrEq( pcmd, "ma_version" ))
	{
		PrintToClientConsole(pEntity, "%s\n", mani_version);
		PrintToClientConsole(pEntity, "Server Tickrate %i\n", server_tickrate);

		if (mani_dead_alltalk.GetInt() == 1)
		{
			PrintToClientConsole(pEntity, "Dead AllTalk is On\n");
		}

		if (war_mode)
		{
			PrintToClientConsole(pEntity, "War mode enabled\n");
		}

		return PLUGIN_STOP;
	}
	else if (FStrEq( pcmd, "ma_kick" ))	return (ProcessMaKick(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_spray" )) return (gpManiSprayRemove->ProcessMaSpray(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_slay" ))	return (ProcessMaSlay(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_offset" ))	return (ProcessMaOffset(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_teamindex" ))	return (ProcessMaTeamIndex(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_offsetscan" ))	return (ProcessMaOffsetScan(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), engine->Cmd_Argv(3)));
	else if (FStrEq( pcmd, "ma_slap" ))	return (ProcessMaSlap(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_setadminflag" ))	return (ProcessMaSetAdminFlag(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_setskin" ))	return (ProcessMaSetSkin(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_setcash" ))	return (ProcessMaCash(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_SET_CASH));
	else if (FStrEq( pcmd, "ma_givecash" ))	return (ProcessMaCash(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_GIVE_CASH));
	else if (FStrEq( pcmd, "ma_givecashp" ))	return (ProcessMaCash(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_GIVE_CASH_PERCENT));
	else if (FStrEq( pcmd, "ma_takecash" ))	return (ProcessMaCash(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_TAKE_CASH));
	else if (FStrEq( pcmd, "ma_takecashp" ))	return (ProcessMaCash(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_TAKE_CASH_PERCENT));
	else if (FStrEq( pcmd, "ma_sethealth" ))	return (ProcessMaHealth(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_SET_HEALTH));
	else if (FStrEq( pcmd, "ma_givehealth" ))	return (ProcessMaHealth(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_GIVE_HEALTH));
	else if (FStrEq( pcmd, "ma_givehealthp" ))	return (ProcessMaHealth(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_GIVE_HEALTH_PERCENT));
	else if (FStrEq( pcmd, "ma_takehealth" ))	return (ProcessMaHealth(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_TAKE_HEALTH));
	else if (FStrEq( pcmd, "ma_takehealthp" ))	return (ProcessMaHealth(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), MANI_TAKE_HEALTH_PERCENT));
	else if (FStrEq( pcmd, "ma_resetrank" )) return (ProcessMaResetPlayerRank(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_map" )) return (ProcessMaMap(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_setnextmap" )) return (ProcessMaSetNextMap(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_voterandom" )) return (ProcessMaVoteRandom(engine->IndexOfEdict(pEntity), false, false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_voteextend" )) return (ProcessMaVoteExtend(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_votercon" )) return (ProcessMaVoteRCon(engine->IndexOfEdict(pEntity), false, false, false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_vote" )) return (ProcessMaVote(engine->IndexOfEdict(pEntity), false, false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), 
												engine->Cmd_Argv(3), engine->Cmd_Argv(4), engine->Cmd_Argv(5), engine->Cmd_Argv(6), engine->Cmd_Argv(7), engine->Cmd_Argv(8), 
												engine->Cmd_Argv(9), engine->Cmd_Argv(10), engine->Cmd_Argv(11)));
	else if (FStrEq( pcmd, "ma_votequestion" )) return (ProcessMaVoteQuestion(engine->IndexOfEdict(pEntity), false, false, false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), 
												engine->Cmd_Argv(3), engine->Cmd_Argv(4), engine->Cmd_Argv(5), engine->Cmd_Argv(6), engine->Cmd_Argv(7), engine->Cmd_Argv(8), 
												engine->Cmd_Argv(9), engine->Cmd_Argv(10), engine->Cmd_Argv(11)));
	else if (FStrEq( pcmd, "ma_votecancel" )) return (ProcessMaVoteCancel(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_play" )) return (ProcessMaPlaySound(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_restrict" )) return (ProcessMaRestrictWeapon(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), true));
	else if (FStrEq( pcmd, "ma_knives" )) return (ProcessMaKnives(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_pistols" )) return (ProcessMaPistols(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_shotguns" )) return (ProcessMaShotguns(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_nosnipers" )) return (ProcessMaNoSnipers(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_unrestrict" )) return (ProcessMaRestrictWeapon(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), "0", false));
	else if (FStrEq( pcmd, "ma_unrestrictall" )) return (ProcessMaUnRestrictAll(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_blind" )) return (ProcessMaBlind(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_freeze" )) return (ProcessMaFreeze(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_noclip" )) return (ProcessMaNoClip(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_burn" )) return (ProcessMaBurn(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_colour" )) return (ProcessMaColour(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), engine->Cmd_Argv(3), engine->Cmd_Argv(4), engine->Cmd_Argv(5)));
	else if (FStrEq( pcmd, "ma_render" )) return (ProcessMaRenderMode(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_renderfx" )) return (ProcessMaRenderFX(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_color" )) return (ProcessMaColour(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), engine->Cmd_Argv(3), engine->Cmd_Argv(4), engine->Cmd_Argv(5)));
	else if (FStrEq( pcmd, "ma_give" )) return (ProcessMaGive(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_giveammo" )) return (ProcessMaGiveAmmo(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), engine->Cmd_Argv(3), engine->Cmd_Argv(4), engine->Cmd_Argv(5)));
	else if (FStrEq( pcmd, "ma_drug" )) return (ProcessMaDrug(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_decal" )) return (ProcessMaDecal(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_gimp" )) return (ProcessMaGimp(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_timebomb" )) return (ProcessMaTimeBomb(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_firebomb" )) return (ProcessMaFireBomb(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_freezebomb" )) return (ProcessMaFreezeBomb(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_beacon" )) return (ProcessMaBeacon(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_mute" )) return (ProcessMaMute(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_teleport" )) return (ProcessMaTeleport(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), engine->Cmd_Argv(3), engine->Cmd_Argv(4)));
	else if (FStrEq( pcmd, "ma_position" )) return (ProcessMaPosition(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0)));
	else if (FStrEq( pcmd, "ma_swapteam" )) return (ProcessMaSwapTeam(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_spec" )) return (ProcessMaSpec(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_balance" )) return (ProcessMaBalance(engine->IndexOfEdict(pEntity), false, false));
	else if (FStrEq( pcmd, "ma_dropc4" )) return (ProcessMaDropC4(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_ban" )) return (ProcessMaBan(engine->IndexOfEdict(pEntity), false, false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_banip" )) return (ProcessMaBan(engine->IndexOfEdict(pEntity), false, true, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_unban" )) return (ProcessMaUnBan(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0),	engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_aban_name"))	return (ProcessMaAutoKickBanName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), false));
	else if (FStrEq( pcmd, "ma_aban_pname")) return (ProcessMaAutoKickBanPName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2), false));
	else if (FStrEq( pcmd, "ma_akick_name")) return (ProcessMaAutoKickBanName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), 0, true));
	else if (FStrEq( pcmd, "ma_akick_pname")) return (ProcessMaAutoKickBanPName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), 0, true));
	else if (FStrEq( pcmd, "ma_akick_steam")) return (ProcessMaAutoKickSteam(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_akick_ip")) return (ProcessMaAutoKickIP(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_unaban_name")) return (ProcessMaUnAutoKickBanName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), false));
	else if (FStrEq( pcmd, "ma_unaban_pname")) return (ProcessMaUnAutoKickBanPName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), false));
	else if (FStrEq( pcmd, "ma_unakick_name")) return (ProcessMaUnAutoKickBanName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), true));
	else if (FStrEq( pcmd, "ma_unakick_pname"))	return (ProcessMaUnAutoKickBanPName(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), true));
	else if (FStrEq( pcmd, "ma_unakick_steam"))	return (ProcessMaUnAutoKickSteam(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_unakick_ip")) return (ProcessMaUnAutoKickIP(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_ashow_name" )) return (ProcessMaAutoKickBanShowName(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_ashow_pname" )) return (ProcessMaAutoKickBanShowPName(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_ashow_steam" )) return (ProcessMaAutoKickBanShowSteam(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_ashow_ip" ))	return (ProcessMaAutoKickBanShowIP(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "adminsay" ) || FStrEq( pcmd, "ma_say" )) return ProcessMaSay(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string);
	else if (FStrEq( pcmd, "ma_csay" )) return ProcessMaCSay(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string);
	else if (FStrEq( pcmd, "adminonlysay" ) || FStrEq(pcmd, "adminosay") || FStrEq(pcmd, "ma_chat")) return ProcessMaChat(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string);
	else
	{
		return ProcessClientCommandSection2(pEntity, pcmd, pcmd2, pcmd3, say_string, trimmed_say, arg_string, &say_argc);
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Handle second part of client command as MSVC complains of nested limit
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ProcessClientCommandSection2
(
edict_t	*pEntity,
const char *pcmd,
const char *pcmd2,
const char *pcmd3,
const char *say_string,
char *trimmed_say,
char *arg_string,
int *say_argc
)
{
	player_t	player;

	player.entity = pEntity;

	if (FStrEq( pcmd, "ma_psay" ))
	{
		g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, say_argc);
		return(g_ManiAdminPlugin.ProcessMaPSay (engine->IndexOfEdict(pEntity),false, engine->Cmd_Argc(), engine->Cmd_Argv(0), say_argv[0].argv_string, &(trimmed_say[say_argv[1].index])));
	}
	else if (FStrEq( pcmd, "adminshowranks" ) || FStrEq( pcmd, "ma_ranks" )) return (ProcessMaRanks(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "ma_showsounds" )) return (ProcessMaShowSounds(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_saveloc" )) {return (ProcessMaSaveLoc(engine->IndexOfEdict(pEntity), false));}
	else if (FStrEq( pcmd, "ma_showrestrict" )) return (ProcessMaShowRestrict(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "adminshowrankedplayers" ) || FStrEq( pcmd, "ma_plranks" )) return (ProcessMaPLRanks(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1), engine->Cmd_Argv(2)));
	else if (FStrEq( pcmd, "adminshowconfig" ) || FStrEq( pcmd, "ma_config" )) return (ProcessMaConfig(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_help" ))	return (ProcessMaHelp(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_users" )) return (ProcessMaUsers(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_rates" )) return (ProcessMaRates(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "adminshowtkviolations" ) || FStrEq( pcmd, "ma_tklist" )) return (ProcessMaTKList(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_war" )) return (ProcessMaWar(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(1)));
	else if (FStrEq( pcmd, "ma_settings" )) return (ProcessMaSettings(engine->IndexOfEdict(pEntity)));
	else if (FStrEq( pcmd, "adminshowadmins" ) || FStrEq( pcmd, "ma_admins" )) return (ProcessMaAdmins(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_admingroups" )) return (ProcessMaAdminGroups(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_immunity" )) return (ProcessMaImmunity(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_immunitygroups" )) return (ProcessMaImmunityGroups(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_maplist" )) return (ProcessMaMapList(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_mapcycle" )) return (ProcessMaMapCycle(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "ma_votemaplist" )) return (ProcessMaVoteMapList(engine->IndexOfEdict(pEntity), false));
	else if (FStrEq( pcmd, "adminrcon" ) || FStrEq( pcmd, "ma_rcon" )) return ProcessMaRCon(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string);
	else if (FStrEq( pcmd, "admincexec" ) || FStrEq( pcmd, "ma_cexec" ))
	{
		g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, say_argc);
		return(g_ManiAdminPlugin.ProcessMaCExec (engine->IndexOfEdict(pEntity),false, engine->Cmd_Argc(), engine->Cmd_Argv(0), say_argv[0].argv_string, &(trimmed_say[say_argv[1].index])));
	}	
	else if (FStrEq( pcmd, "admincexec_all" ) || FStrEq( pcmd, "ma_cexec_all" )) return ProcessMaCExecAll(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string);
	else if (FStrEq( pcmd, "admincexec_t" ) || FStrEq( pcmd, "admincexec_ct") || FStrEq( pcmd, "admincexec_spec") ||
			 FStrEq( pcmd, "ma_cexec_t" ) || FStrEq( pcmd, "ma_cexec_ct") || FStrEq( pcmd, "ma_cexec_spec"))
	{
		if (FStrEq( pcmd, "admincexec_spec") || FStrEq( pcmd, "ma_cexec_spec"))
		{
			if (!gpManiGameType->IsSpectatorAllowed()) return PLUGIN_CONTINUE;
		}

		int	team;
		if (FStrEq( pcmd, "admincexec_t") || FStrEq( pcmd, "ma_cexec_t")) team = TEAM_A;
		else if (FStrEq(pcmd, "admincexec_ct") || FStrEq( pcmd, "ma_cexec_ct"))	team = TEAM_B;
		else team = gpManiGameType->GetSpectatorIndex();

		return ProcessMaCExecTeam(engine->IndexOfEdict(pEntity), false, engine->Cmd_Argc(), engine->Cmd_Argv(0), arg_string, team);
	}
	else if ( FStrEq( pcmd, "mani_votemenu" ))
	{
		int	next_index = 0;
		int argv_offset = 0;

		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;

		if (FStrEq (engine->Cmd_Argv(1), "more"))
			{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
			}

		ProcessMenuSystemVotemap (&player, next_index, argv_offset);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "mani_vote_accept" ))
	{
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		ProcessVoteConfirmation (&player, true);
		return PLUGIN_STOP;
	}
	else if (FStrEq( pcmd, "adminexplode" ) || FStrEq( pcmd, "ma_explode" ))
	{
		int admin_index;
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_explode", ALLOW_EXPLODE, war_mode, &admin_index)) return PLUGIN_STOP;

		ProcessExplodeAtCurrentPosition (&player);
		return PLUGIN_STOP;
	} 
	else if ( FStrEq( pcmd, "mani_vote_refuse" ))
	{
		if (!FindPlayerByEntity(&player)) return PLUGIN_STOP;
		ProcessVoteConfirmation (&player, false);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "tkbot" ) || FStrEq( pcmd, "tkhuman"))
	{
		if (ProcessTKCmd(&player)) return PLUGIN_STOP;
		return PLUGIN_STOP;
	}
	else
	{
		if (ProcessCheatCVarPing(&player, pcmd))
		{
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Init cheat ping list
//---------------------------------------------------------------------------------
bool	CAdminPlugin::ProcessCheatCVarPing(player_t *player, const char *pcmd)
{
	if (!FindPlayerByEntity(player)) return false;
	if (player->is_bot) return false;
	if (strcmp(player->steam_id,"BOT") == 0) return false;
	if (player->player_info->IsHLTV()) return false;

	if (cheat_ping_list[player->index - 1].waiting_for_control)
	{
		if (FStrEq(cheat_ping_list[player->index - 1].control_string, pcmd))
		{
			cheat_ping_list[player->index - 1].waiting_for_control = false;
			if (!cheat_ping_list[player->index - 1].waiting_for_cvar)
			{
				cheat_ping_list[player->index - 1].waiting_to_send = true;
			}

			return true;
		}
	}

	if (cheat_ping_list[player->index - 1].waiting_for_cvar)
	{
		if (FStrEq(cheat_ping_list[player->index - 1].cvar_string, pcmd))
		{
			cheat_ping_list[player->index - 1].waiting_for_cvar = false;
			if (!cheat_ping_list[player->index - 1].waiting_for_control)
			{
				cheat_ping_list[player->index - 1].waiting_to_send = true;
			}

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Init cheat ping list
//---------------------------------------------------------------------------------
void	CAdminPlugin::InitCheatPingList(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		cheat_ping_list[i].connected = false;
		cheat_ping_list[i].count = 0;
		Q_strcpy(cheat_ping_list[i].cvar_string,"");
		Q_strcpy(cheat_ping_list[i].control_string, "");
		cheat_ping_list[i].waiting_for_control = false;
		cheat_ping_list[i].waiting_for_cvar = false;
		cheat_ping_list[i].waiting_to_send = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Update with player information
//---------------------------------------------------------------------------------
void	CAdminPlugin::UpdateCurrentPlayerList(void)
{
	for (int i = 1; i <= max_players; i++)
	{
		player_t	player;

		cheat_ping_list[i - 1].connected = false;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (strcmp(player.steam_id,"BOT") == 0) continue;
		if (player.player_info->IsHLTV()) continue;

		// Must be a player connected
		cheat_ping_list[i - 1].connected = true;
	}
}
//---------------------------------------------------------------------------------
// Purpose: Load cheat list
//---------------------------------------------------------------------------------
void	CAdminPlugin::LoadCheatList(void)
{
	// Free list first
	FreeList((void **) &cheat_cvar_list, &cheat_cvar_list_size);
	FreeList((void **) &cheat_cvar_list2, &cheat_cvar_list_size2);

	// hlh_norecoil 2
	AddToList((void **) &cheat_cvar_list, sizeof(cheat_cvar_t), &cheat_cvar_list_size);
	Q_strcpy(cheat_cvar_list[cheat_cvar_list_size - 1].cheat_cvar, "rvrixy|omysv*<");

	AddToList((void **) &cheat_cvar_list2, sizeof(cheat_cvar_t), &cheat_cvar_list_size2);
	Q_strcpy(cheat_cvar_list2[cheat_cvar_list_size2 - 1].cheat_cvar, "rvrixy|omysv");

	// omfg_norecoil 2
	AddToList((void **) &cheat_cvar_list, sizeof(cheat_cvar_t), &cheat_cvar_list_size);
	Q_strcpy(cheat_cvar_list[cheat_cvar_list_size - 1].cheat_cvar, "ywpqixy|omysv*<");
	
	AddToList((void **) &cheat_cvar_list2, sizeof(cheat_cvar_t), &cheat_cvar_list_size2);
	Q_strcpy(cheat_cvar_list2[cheat_cvar_list_size2 - 1].cheat_cvar, "ywpqixy|omysv");


	for (int i = 0; i < cheat_cvar_list_size; i++)
	{
		for (int j = 0; j < Q_strlen(cheat_cvar_list[i].cheat_cvar); j ++)
		{
			cheat_cvar_list[i].cheat_cvar[j] -= 10;
		}
	}

	for (int i = 0; i < cheat_cvar_list_size2; i++)
	{
		for (int j = 0; j < Q_strlen(cheat_cvar_list2[i].cheat_cvar); j ++)
		{
			cheat_cvar_list2[i].cheat_cvar[j] -= 10;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check for cheat cvar list
//---------------------------------------------------------------------------------
char	*CAdminPlugin::GenerateControlString(void)
{
	static	char	control_string[10];

	for (int i = 0; i < 9; i ++)
	{
		control_string[i] = (char) ((rand() % 26) + 97);
	}

	control_string[9] = '\0';

	return (char *) control_string;
}

//---------------------------------------------------------------------------------
// Purpose: Check for cheat cvar list
//---------------------------------------------------------------------------------
void	CAdminPlugin::ProcessCheatCVarCommands(void)
{
	// Game frame has called this
	player_t	player;
	char	ban_cmd[128];

	for (int i = 0; i < max_players; i++)
	{
		// Player not connected
		if (!cheat_ping_list[i].connected) continue;

		player.index = i + 1;

		if (cheat_ping_list[i].waiting_for_control &&
			cheat_ping_list[i].waiting_for_cvar)
		{
			cheat_ping_list[i].waiting_to_send = true;
			cheat_ping_list[i].waiting_for_cvar = false;
			cheat_ping_list[i].waiting_for_control = false;
		}

		if (!cheat_ping_list[i].waiting_for_control &&
			cheat_ping_list[i].waiting_for_cvar &&
			!cheat_ping_list[i].waiting_to_send)
		{
			// Hacker alert, we got the control cvar back
			// but not the cheat cvar

			if (!FindPlayerByIndex(&player))
			{
				cheat_ping_list[i].connected = false;
				continue;
			}

			// Process how we handle cheats

			cheat_ping_list[i].count ++;
			if (cheat_ping_list[i].count < mani_protect_against_cheat_cvars_threshold.GetInt())
			{
				int tries_left = mani_protect_against_cheat_cvars_threshold.GetInt() - cheat_ping_list[i].count;

				for (int j = 1; j <= max_players; j++)
				{
					player_t	admin;
					int			admin_index;

					admin.index = j;
					if (!FindPlayerByIndex(&admin)) continue;
					if (admin.is_bot) continue;
					if (!IsClientAdmin(&admin, &admin_index)) continue;

					SayToPlayer(&admin, "[MANI_ADMIN_PLUGIN] Warning !! Player %s may have a cheat installed", player.name);
					SayToPlayer(&admin, "%i more detection attempt%s until automatic action is taken", tries_left, (tries_left == 1) ? "":"s");
				}

				continue;
			}

			if (mani_protect_against_cheat_cvars_mode.GetInt() == 0 && sv_lan->GetInt() != 1)
			{
				// Ban by user id
				SayToAll(false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_protect_against_cheat_cvars_ban_time.GetInt(), 
											player.user_id);
				LogCommand(NULL, "Banned (Hack detected) [%s] [%s] %s", player.name, player.steam_id, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
				cheat_ping_list[i].connected = false;
				continue;
			}
			else if (mani_protect_against_cheat_cvars_mode.GetInt() == 1)
			{
				// Ban by user ip address
				SayToAll(false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
						mani_protect_against_cheat_cvars_ban_time.GetInt(), 
						player.ip_address);
				LogCommand(NULL, "Banned IP (Hack detected) [%s] [%s] %s", player.name, player.ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				cheat_ping_list[i].connected = false;
				continue;
			}
			else if (mani_protect_against_cheat_cvars_mode.GetInt() == 2)
			{
				// Ban by user id and ip address
				SayToAll(false,"[MANI_ADMIN_PLUGIN] Player %s was banned for having a detected hack", player.name);
				PrintToClientConsole(player.entity, "You have been auto banned for hacking\n");
				if (sv_lan->GetInt() != 1)
				{
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_protect_against_cheat_cvars_ban_time.GetInt(), 
											player.user_id);
					LogCommand(NULL, "Banned (Hack detected) [%s] [%s] %s", player.name, player.steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);	
					engine->ServerCommand("writeid\n");
				}

				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_protect_against_cheat_cvars_ban_time.GetInt(), 
										player.ip_address);
				LogCommand(NULL, "Banned IP (Hack detected) [%s] [%s] %s", player.name, player.ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");

				cheat_ping_list[i].connected = false;
				continue;
			}
		}

		if (cheat_ping_list[i].waiting_to_send ||
			(!cheat_ping_list[i].waiting_for_cvar &&
			  cheat_ping_list[i].waiting_for_control))
		{
			int cvar_to_use = rand () % cheat_cvar_list_size;

			if (!FindPlayerByIndex(&player))
			{
				cheat_ping_list[i].connected = false;
				continue;
			}

			// Ready to send a control string and cheat cvar
			Q_strcpy(cheat_ping_list[i].control_string, GenerateControlString());
			Q_strcpy(cheat_ping_list[i].cvar_string, cheat_cvar_list2[cvar_to_use].cheat_cvar);

			// throw it out to client
			engine->ClientCommand(player.entity, "%s\n", cheat_ping_list[i].control_string);
			engine->ClientCommand(player.entity, "%s\n", cheat_cvar_list[cvar_to_use].cheat_cvar);

			cheat_ping_list[i].waiting_to_send = false;
			cheat_ping_list[i].waiting_for_cvar = true;
			cheat_ping_list[i].waiting_for_control = true;
			continue;
		}
	}
}
//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::ProcessAdminMenu( edict_t *pEntity)
{
	int	admin_index;
	player_t player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return PLUGIN_STOP;
	}

	if (!IsAdminAllowed(&player, "admin", ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;

	if (1 == engine->Cmd_Argc())
	{
		// User typed admin at console
		ShowPrimaryMenu(pEntity, admin_index);
		return PLUGIN_STOP;
	}

	if (engine->Cmd_Argc() > 1) 
	{
		const char *temp_command = engine->Cmd_Argv(1);
		int next_index = 0;
		int argv_offset = 0;

		if (FStrEq (temp_command, "more"))
		{
			// Get next index for menu
			next_index = Q_atoi(engine->Cmd_Argv(2));
			argv_offset = 2;
		}

		const char *menu_command = engine->Cmd_Argv(1 + argv_offset);

		if (FStrEq (menu_command, "changemap"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CHANGEMAP])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change map\n");
				return PLUGIN_STOP;
			}

			// Change map command selected via menu or console
			ProcessChangeMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "setnextmap"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CHANGEMAP])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to set the next map\n");
				return PLUGIN_STOP;
			}

			// Set Next Map command selected via menu or console
			ProcessSetNextMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "kicktype"))
		{
			if (!admin_list[admin_index].flags[ALLOW_KICK] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to kick players\n");
				return PLUGIN_STOP;
			}

			// kick command selected via menu or console
			ProcessKickType (&player);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "kick"))
		{
			if (!admin_list[admin_index].flags[ALLOW_KICK] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to kick players\n");
				return PLUGIN_STOP;
			}

			// kick command selected via menu or console
			ProcessKickPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "punish"))
		{
			if (( !admin_list[admin_index].flags[ALLOW_BLIND] &&
				!admin_list[admin_index].flags[ALLOW_SLAP] &&
				!admin_list[admin_index].flags[ALLOW_FREEZE] &&
				!admin_list[admin_index].flags[ALLOW_TELEPORT] &&
				!admin_list[admin_index].flags[ALLOW_GIMP] &&
				!admin_list[admin_index].flags[ALLOW_DRUG] &&
				!admin_list[admin_index].flags[ALLOW_BURN] &&
				!admin_list[admin_index].flags[ALLOW_NO_CLIP] &&
				!admin_list[admin_index].flags[ALLOW_SETSKINS] &&
				!admin_list[admin_index].flags[ALLOW_TIMEBOMB] &&
				!admin_list[admin_index].flags[ALLOW_FIREBOMB] &&
				!admin_list[admin_index].flags[ALLOW_FREEZEBOMB] &&
				!admin_list[admin_index].flags[ALLOW_BEACON])
				|| war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to punish players\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessPunishType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "playeroptions"))
		{
			if ((!admin_list[admin_index].flags[ALLOW_KICK] &&
				!admin_list[admin_index].flags[ALLOW_SLAY] &&
				!admin_list[admin_index].flags[ALLOW_BAN] &&
				!admin_list[admin_index].flags[ALLOW_CEXEC_MENU] &&
				!admin_list[admin_index].flags[ALLOW_SWAP] &&
				!admin_list[admin_index].flags[ALLOW_SPRAY_TAG])
				|| war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to manage players\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessPlayerManagementType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "mapoptions"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CHANGEMAP])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change maps\n");
				return PLUGIN_STOP;
			}

			// Punish command selected via menu or console
			ProcessMapManagementType (&player, admin_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "voteoptions"))
		{
			if ((!admin_list[admin_index].flags[ALLOW_RANDOM_MAP_VOTE]) &&
				(!admin_list[admin_index].flags[ALLOW_MAP_VOTE]) &&
				(!admin_list[admin_index].flags[ALLOW_MENU_RCON_VOTE]) &&
				(!admin_list[admin_index].flags[ALLOW_MENU_QUESTION_VOTE]) &&
				(!admin_list[admin_index].flags[ALLOW_CANCEL_VOTE]) ||
				 war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to manage votes\n");
				return PLUGIN_STOP;
			}

			// Vote management options
			ProcessVoteType (&player, admin_index, next_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slay"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SLAY] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slay players\n");
				return PLUGIN_STOP;
			}

			// Slay command selected via menu or console
			ProcessSlayPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "spray") ||
			FStrEq (menu_command, "spraywarn") ||
			FStrEq (menu_command, "spraykick") ||
			FStrEq (menu_command, "sprayban") ||
			FStrEq (menu_command, "spraypermban"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SPRAY_TAG] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to check spray tags\n");
				return PLUGIN_STOP;
			}

			// Slay command selected via menu or console
			gpManiSprayRemove->ProcessMaSprayMenu (&player, admin_index, next_index, argv_offset, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "votercon"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MENU_RCON_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run a rcon vote\n");
				return PLUGIN_STOP;
			}

			// RCon vote command selected via menu or console
			ProcessRConVote (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "votequestion"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MENU_QUESTION_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run a question vote\n");
				return PLUGIN_STOP;
			}

			// Question vote command selected via menu or console
			ProcessQuestionVote (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "voteextend"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MAP_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run an extension vote\n");
				return PLUGIN_STOP;
			}

			// Extend vote command selected via menu or console
			ProcessMaVoteExtend(player.index, false, 1, "ma_voteextend");
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "ban") || FStrEq (menu_command, "banip"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BAN] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessBanPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "randommapvote"))
		{
			if (!admin_list[admin_index].flags[ALLOW_RANDOM_MAP_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run random map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSystemVoteRandomMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "buildmapvote"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MAP_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run random map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSystemVoteBuildMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "singlemapvote"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MAP_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSystemVoteSingleMap (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "multimapvote"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MAP_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run map votes\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSystemVoteMultiMap (&player, admin_index);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "autobanname"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BAN] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// AutoBan command selected via menu or console
			ProcessAutoBanPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "autokicksteam") || FStrEq (menu_command, "autokickip") || FStrEq (menu_command, "autokickname"))
		{
			if (!admin_list[admin_index].flags[ALLOW_KICK] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to autokick players\n");
				return PLUGIN_STOP;
			}

			// Autokick command selected via menu or console
			ProcessAutoKickPlayer (&player, menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slap"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SLAP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slap players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuSlapPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "blind"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BLIND] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to blind players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuBlindPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "freeze"))
		{
			if (!admin_list[admin_index].flags[ALLOW_FREEZE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to freeze players\n");
				return PLUGIN_STOP;
			}

			// Freeze command selected via menu or console
			ProcessMenuFreezePlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "burn"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BURN] || war_mode || !gpManiGameType->IsFireAllowed())
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to burn players\n");
				return PLUGIN_STOP;
			}

			// Burn command selected via menu or console
			ProcessMenuBurnPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "swapteam"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SWAP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to swap players to different teams\n");
				return PLUGIN_STOP;
			}

			// SwapTeam command selected via menu or console
			ProcessMenuSwapPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "specplayer"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SWAP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to move players to be spectator\n");
				return PLUGIN_STOP;
			}

			// Spec command selected via menu or console
			ProcessMenuSpecPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "balanceteam"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SWAP] || war_mode || !gpManiGameType->IsTeamPlayAllowed())
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to balance teams\n");
				return PLUGIN_STOP;
			}

			// Balance players on the server
			ProcessMaBalance (player.index, false, false);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cancelvote"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CANCEL_VOTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to cancel votes\n");
				return PLUGIN_STOP;
			}

			// Cancel vote on server
			ProcessMaVoteCancel (player.index, false, 1, "ma_votecancel");
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "drug"))
		{
			if (!admin_list[admin_index].flags[ALLOW_DRUG] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to drug players\n");
				return PLUGIN_STOP;
			}

			// Drug command selected via menu or console
			ProcessMenuDrugPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "noclip"))
		{
			if (!admin_list[admin_index].flags[ALLOW_NO_CLIP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to no clip players\n");
				return PLUGIN_STOP;
			}

			// No Clip command selected via menu or console
			ProcessMenuNoClipPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "skinoptions"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SETSKINS] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change skins of players\n");
				return PLUGIN_STOP;
			}

			// Skin Options command selected via menu or console
			ProcessMenuSkinOptions (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "setskin"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SETSKINS] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change skins of players\n");
				return PLUGIN_STOP;
			}

			// Set Skin command selected via menu or console
			ProcessMenuSetSkin (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "timebomb"))
		{
			if (!admin_list[admin_index].flags[ALLOW_TIMEBOMB] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into time bombs\n");
				return PLUGIN_STOP;
			}

			// Set Time Bomb command selected via menu or console
			ProcessMenuTimeBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "firebomb"))
		{
			if (!admin_list[admin_index].flags[ALLOW_FIREBOMB] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into fire bombs\n");
				return PLUGIN_STOP;
			}

			// Set Fire Bomb command selected via menu or console
			ProcessMenuFireBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "freezebomb"))
		{
			if (!admin_list[admin_index].flags[ALLOW_FREEZEBOMB] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into freeze bombs\n");
				return PLUGIN_STOP;
			}

			// Set Freeze Bomb command selected via menu or console
			ProcessMenuFreezeBombPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "beacon"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BEACON] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change players into beacons\n");
				return PLUGIN_STOP;
			}

			// Set Freeze Bomb command selected via menu or console
			ProcessMenuBeaconPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "gimp"))
		{
			if (!admin_list[admin_index].flags[ALLOW_GIMP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to gimp players\n");
				return PLUGIN_STOP;
			}

			// Gimp command selected via menu or console
			ProcessMenuGimpPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "mute"))
		{
			if (!admin_list[admin_index].flags[ALLOW_MUTE] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to mute players\n");
				return PLUGIN_STOP;
			}

			// Gimp command selected via menu or console
			ProcessMenuMutePlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "teleport"))
		{
			if (!admin_list[admin_index].flags[ALLOW_TELEPORT] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to teleport players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMenuTeleportPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "savelocation"))
		{
			if (!admin_list[admin_index].flags[ALLOW_TELEPORT] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to teleport players\n");
				return PLUGIN_STOP;
			}

			// Ban command selected via menu or console
			ProcessMaSaveLoc (player.index, false);
			return PLUGIN_STOP;
		}		
		if (FStrEq (menu_command, "banoptions") || FStrEq (menu_command, "banoptionsip") ||
			FStrEq (menu_command, "autobanoptionsname"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BAN] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessBanOptions (pEntity, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "randomvoteoptions") || FStrEq (menu_command, "mapvoteoptions") ||
			FStrEq (menu_command, "multimapvoteoptions"))
		{
			if ((!admin_list[admin_index].flags[ALLOW_RANDOM_MAP_VOTE] &&
				 !admin_list[admin_index].flags[ALLOW_MAP_VOTE]) || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to run votes\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessDelayTypeOptions (&player, menu_command);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "slapoptions"))
		{
			if (!admin_list[admin_index].flags[ALLOW_SLAP] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to slap players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (amount of damage)
			ProcessSlapOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "blindoptions"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BLIND] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to blind players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (amount of blindness)
			ProcessBlindOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "bantype"))
		{
			if (!admin_list[admin_index].flags[ALLOW_BAN] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to ban players\n");
				return PLUGIN_STOP;
			}

			// Ban command options (time of ban)
			ProcessBanType (&player);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "config"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CONFIG])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change the plugin config\n");
				return PLUGIN_STOP;
			}

			// Config options 
			ProcessConfigOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "toggle"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CONFIG])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to change the plugin config\n");
				return PLUGIN_STOP;
			}

			// Process toggle of options
			ProcessConfigToggle (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "restrict_weapon"))
		{
			if (!admin_list[admin_index].flags[ALLOW_RESTRICTWEAPON] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to restrict weapons\n");
				return PLUGIN_STOP;
			}

			// restrict weapon command selected via menu or console
			ProcessRestrictWeapon (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "play_sound"))
		{
			if (!admin_list[admin_index].flags[ALLOW_PLAYSOUND] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to play sounds\n");
				return PLUGIN_STOP;
			}

			// play sound command selected via menu or console
			ProcessPlaySound (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
			
		if (FStrEq (menu_command, "rcon"))
		{
			if (!admin_list[admin_index].flags[ALLOW_RCON_MENU])
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run rcon command
			ProcessRconCommand (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cexecoptions"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CEXEC_MENU] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run rcon command
			ProcessCExecOptions (pEntity);
			return PLUGIN_STOP;
		}
		if (FStrEq (menu_command, "cexec") ||
			FStrEq (menu_command, "cexec_t") ||
			FStrEq (menu_command, "cexec_ct") ||
			FStrEq (menu_command, "cexec_spec") ||
			FStrEq (menu_command, "cexec_all"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CEXEC_MENU] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			// run exec command
			ProcessCExecCommand (&player, (char *) menu_command, next_index, argv_offset);
			return PLUGIN_STOP;
		}

		if (FStrEq (menu_command, "cexec_player"))
		{
			if (!admin_list[admin_index].flags[ALLOW_CEXEC_MENU] || war_mode)
			{
				PrintToClientConsole(pEntity, "Mani Admin Plugin: You are not authorised to select rcon commands\n");
				return PLUGIN_STOP;
			}

			ProcessCExecPlayer (&player, next_index, argv_offset);
			return PLUGIN_STOP;
		}
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Draw Primary menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowPrimaryMenu( edict_t *pEntity, int admin_index )
{
	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	if (!war_mode)
	{
		if ((admin_list[admin_index].flags[ALLOW_KICK] || 
			 admin_list[admin_index].flags[ALLOW_BAN] || 
			 admin_list[admin_index].flags[ALLOW_CEXEC_MENU] ||
			 admin_list[admin_index].flags[ALLOW_MUTE] ||
			 admin_list[admin_index].flags[ALLOW_SWAP]) 
			 && !war_mode)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_PLAYER_MANAGEMENT));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");
		}

		if ((admin_list[admin_index].flags[ALLOW_SLAY] || 
			 (admin_list[admin_index].flags[ALLOW_SLAP] && gpManiGameType->IsSlapAllowed()) || 
			 admin_list[admin_index].flags[ALLOW_BLIND] ||
			 admin_list[admin_index].flags[ALLOW_FREEZE] ||
			 (admin_list[admin_index].flags[ALLOW_TELEPORT] && gpManiGameType->IsTeleportAllowed()) ||
			 admin_list[admin_index].flags[ALLOW_GIMP] ||
			 (admin_list[admin_index].flags[ALLOW_DRUG] && gpManiGameType->IsDrugAllowed()) ||
			 (admin_list[admin_index].flags[ALLOW_BURN] && gpManiGameType->IsFireAllowed()) ||
			 admin_list[admin_index].flags[ALLOW_NO_CLIP]))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_FUN_PLAYER_MANAGEMENT));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");
		}

		if (admin_list[admin_index].flags[ALLOW_CHANGEMAP])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_MAP_MANAGEMENT));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mapoptions");
		}

		if ((admin_list[admin_index].flags[ALLOW_RANDOM_MAP_VOTE] && !system_vote.vote_in_progress) ||
			(admin_list[admin_index].flags[ALLOW_MAP_VOTE] && !system_vote.vote_in_progress) ||
			(admin_list[admin_index].flags[ALLOW_MENU_RCON_VOTE] && !system_vote.vote_in_progress) ||
			(admin_list[admin_index].flags[ALLOW_MENU_QUESTION_VOTE] && !system_vote.vote_in_progress) ||
			(admin_list[admin_index].flags[ALLOW_CANCEL_VOTE] && system_vote.vote_in_progress))
			
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_VOTE_MANAGEMENT));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteoptions");
		}

		if (admin_list[admin_index].flags[ALLOW_RESTRICTWEAPON] && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_RESTRICT_WEAPONS));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin restrict_weapon");
		}

		if (admin_list[admin_index].flags[ALLOW_PLAYSOUND])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_PLAY_SOUND));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin play_sound");
		}

		if (admin_list[admin_index].flags[ALLOW_RCON_MENU])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_RCON_COMMANDS));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin rcon");
		}

		if (admin_list[admin_index].flags[ALLOW_CONFIG])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_PLUGIN_CONFIG));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin config");
		}
	}
	else
	{
		// Draw war options
		if (admin_list[admin_index].flags[ALLOW_CHANGEMAP])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_CHANGE_MAP));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin changemap");
		}

		if (admin_list[admin_index].flags[ALLOW_RCON_MENU])
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_RCON_COMMANDS));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin rcon");
		}

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PRIMARY_MENU_TURN_OFF_WAR_MODE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle warmode");

	}

	if (menu_list_size == 0) return;
	DrawStandardMenu(&player, Translate(M_PRIMARY_MENU_ESCAPE), Translate(M_PRIMARY_MENU_TITLE), true);
	
	return;
}


//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessChangeMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		ProcessMaMap(player->index, false, 2, "ma_map", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		int	i;

		// Setup menu list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), map_list_size, &menu_list_size);

		for (i = 0; i < map_list_size; i++)
		{
			Q_strcpy(menu_list[i].menu_text, map_list[i].map_name);
			Q_snprintf(menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin changemap %s", map_list[i].map_name);
			Q_strcpy(menu_list[i].sort_name, map_list[i].map_name);
		}

		SortMenu();

		// List size may have changed
		if (next_index > map_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player, Translate(M_MAP_MENU_ESCAPE), Translate(M_MAP_MENU_TITLE), next_index, "admin", "changemap", true, -1);
	}
		
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Changemap menu draw and actual changemap request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSetNextMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		ProcessMaSetNextMap(player->index, false, 2, "ma_setnextmap", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		int	i;

		// Setup menu list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), map_list_size, &menu_list_size);

		for (i = 0; i < map_list_size; i++)
		{
			Q_strcpy(menu_list[i].menu_text, map_list[i].map_name);
			Q_snprintf(menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin setnextmap %s", map_list[i].map_name);
		}

		// List size may have changed
		if (next_index > map_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (player, Translate(M_NEXT_MAP_MENU_ESCAPE), Translate(M_NEXT_MAP_MENU_TITLE), next_index, "admin", "setnextmap", true, -1);
	}
		
	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessKickPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		ProcessMaKick(admin->index, false, 2, "ma_kick",	engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_KICK])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]", player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kick %i", player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();
		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, Translate(M_KICK_MENU_ESCAPE), Translate(M_KICK_MENU_TITLE), next_index,"admin","kick",	true, -1);
		// Draw menu list
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Kick Player menu draw and actual kick request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSlapPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsSlapAllowed()) return;

	if (argc - argv_offset == 4)
	{
		// Slap the player
		ProcessMaSlap (admin->index, false, 3, "ma_slap", engine->Cmd_Argv(3 + argv_offset), engine->Cmd_Argv(2 + argv_offset));

		FreeMenu();

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_MENU_SLAP_AGAIN));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap %s %s", engine->Cmd_Argv(2 + argv_offset), engine->Cmd_Argv(3 + argv_offset));
		if (menu_list_size == 0) return;
		DrawStandardMenu(admin, Translate(M_SLAP_REPEAT_MENU_ESCAPE), Translate(M_SLAP_REPEAT_MENU_TITLE), true);

		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_SLAP])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]", player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap %s %i", engine->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char more_cmd[128];
		Q_snprintf( more_cmd, sizeof(more_cmd), "slap %s", engine->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SLAP_MENU_ESCAPE), Translate(M_SLAP_MENU_TITLE), next_index, "admin", more_cmd, true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Blind Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBlindPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		int blind_amount = atoi(engine->Cmd_Argv(2 + argv_offset));
		if (blind_amount < 0)
		{
			blind_amount = 0;
		}

		if (blind_amount > 255)
		{
			blind_amount = 255;
		}

		char blind_amount_str[128];
		Q_snprintf(blind_amount_str, sizeof(blind_amount_str), "%i", blind_amount);

		// Blind the player
		ProcessMaBlind(admin->index, false, 3, "ma_blind", engine->Cmd_Argv(3 + argv_offset), blind_amount_str);
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BLIND])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind %s %i", engine->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy(menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char more_cmd[128];
		Q_snprintf( more_cmd, sizeof(more_cmd), "blind %s", engine->Cmd_Argv(2 + argv_offset));

		// Draw menu list
		DrawSubMenu (admin, Translate(M_BLIND_MENU_ESCAPE), Translate(M_BLIND_MENU_TITLE), next_index, "admin", more_cmd, true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Swap Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSwapPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Swap the player to opposite team
		ProcessMaSwapTeam(admin->index, false, 2, "ma_swapteam", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;


			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_SWAP])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] [%s] %i", 
											Translate(gpManiGameType->GetTeamShortTranslation(player.team)),
											player.name, 
											player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin swapteam %i", player.user_id);
			Q_strcpy( menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();
		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SWAP_MENU_ESCAPE), Translate(M_SWAP_MENU_TITLE), next_index, "admin", "swapteam", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Move of player to spectator side
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSpecPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Swap the player to opposite team
		ProcessMaSpec(admin->index, false, 2, "ma_spec", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;

			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_SWAP])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] [%s] %i", 
											Translate(gpManiGameType->GetTeamShortTranslation(player.team)),
											player.name, 
											player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin specplayer %i", player.user_id);
			Q_strcpy( menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SPEC_MENU_ESCAPE), Translate(M_SPEC_MENU_TITLE), next_index, "admin", "specplayer", true, -1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFreezePlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Freeze the player
		ProcessMaFreeze(admin->index, false, 2, "ma_freeze", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_FREEZE])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].frozen) ? Translate(M_FREEZE_MENU_FROZEN):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freeze %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_FREEZE_MENU_ESCAPE), Translate(M_FREEZE_MENU_TITLE), next_index, "admin", "freeze", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Burn Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBurnPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsFireAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Freeze the player
		ProcessMaBurn(admin->index, false, 3, "ma_burn", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BURN])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin burn %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, 
							Translate(M_BURN_MENU_ESCAPE), 
							Translate(M_BURN_MENU_TITLE), 
							next_index,
							"admin",
							"burn",
							true,
							-1);
		// Draw menu list
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Handle Drug Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuDrugPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsDrugAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Drug the player
		ProcessMaDrug(admin->index, false, 2, "ma_drug", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_DRUG])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].drugged) ? Translate(M_DRUG_MENU_DRUGGED):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin drug %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_DRUG_MENU_ESCAPE), Translate(M_DRUG_MENU_TITLE), next_index, "admin", "drug", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle No Clip Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuNoClipPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Drug the player
		ProcessMaNoClip(admin->index, false, 3, "ma_noclip", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)	continue;
			if (player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].no_clip) ? Translate(M_NO_CLIP_MENU_NO_CLIP):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin noclip %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_NO_CLIP_MENU_ESCAPE), Translate(M_NO_CLIP_MENU_TITLE), next_index, "admin", "noclip", true,	-1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Gimp Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuGimpPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Gimp the player
		ProcessMaGimp(admin->index, false, 2, "ma_gimp", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_GIMP])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].gimped) ? Translate(M_GIMP_MENU_GIMPED):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin gimp %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_GIMP_MENU_ESCAPE), Translate(M_GIMP_MENU_TITLE), next_index, "admin", "gimp", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Gimp Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuTimeBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Time Bomb
		ProcessMaTimeBomb(admin->index, false, 2, "ma_timebomb", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_TIMEBOMB])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
								(punish_mode_list[player.index - 1].time_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin timebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_TIME_BOMB_MENU_ESCAPE), Translate(M_TIME_BOMB_MENU_TITLE), next_index, "admin", "timebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Fire Bomb Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFireBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Time Bomb
		ProcessMaFireBomb(admin->index, false, 2, "ma_firebomb", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_FIREBOMB])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
				(punish_mode_list[player.index - 1].fire_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin firebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_FREEZE_BOMB_MENU_ESCAPE), Translate(M_FIRE_BOMB_MENU_TITLE), next_index, "admin", "firebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Freeze Bomb Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuFreezeBombPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Time Bomb
		ProcessMaFreezeBomb(admin->index, false, 2, "ma_freezebomb", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_FREEZEBOMB])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
				(punish_mode_list[player.index - 1].freeze_bomb != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freezebomb %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_FREEZE_BOMB_MENU_ESCAPE), Translate(M_FREEZE_BOMB_MENU_TITLE), next_index, "admin", "freezebomb", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Beacon Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuBeaconPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Turn Player into Beacon
		ProcessMaBeacon(admin->index, false, 2, "ma_beacon", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BEACON])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s%s %i", 
								(punish_mode_list[player.index - 1].beacon != 0) ? "-> ":"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin beacon %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_BEACON_MENU_ESCAPE), Translate(M_BEACON_MENU_TITLE), next_index, "admin", "beacon", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Mute Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuMutePlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		// Mute/Un mute the player
		ProcessMaMute(admin->index, false, 2, "ma_mute", engine->Cmd_Argv(2 + argv_offset), "");
		return;
	}
	else
	{
		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_bot)
			{
				continue;
			}

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_MUTE])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s[%s] %i", 
										(punish_mode_list[player.index - 1].muted) ? Translate(M_MUTE_MENU_MUTED):"",
										player.name, 
										player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mute %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_MUTE_MENU_ESCAPE), Translate(M_MUTE_MENU_TITLE), next_index, "admin", "mute", true, -1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Test if player has saved this location
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanTeleport(player_t *player)
{
	player_settings_t *player_settings = FindPlayerSettings(player);

	for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
	{
		if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Teleport Player draw and request
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuTeleportPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (!gpManiGameType->IsTeleportAllowed()) return;

	if (argc - argv_offset == 3)
	{
		// Teleport the player
		ProcessMaTeleport(admin->index, false, 2, "ma_teleport", engine->Cmd_Argv(2 + argv_offset), "", "", "");
		return;
	}
	else
	{
		if (!CanTeleport(admin))
		{
			PrintToClientConsole(admin->entity, Translate(M_TELEPORT_MENU_SAVE_LOC_FIRST));
			return;
		}

		// Setup player list
		FreeMenu();
		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_TELEPORT])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i", player.name, player.user_id);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin teleport %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_TELEPORT_MENU_ESCAPE), Translate(M_TELEPORT_MENU_TITLE), next_index, "admin", "teleport", true, -1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSlayPlayer( player_t *admin, int next_index, int argv_offset )
{
	player_t	player;
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		ProcessMaSlay(admin->index, false, 2, "ma_slay", engine->Cmd_Argv(2 + argv_offset));
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				// Player entity is bad
				continue;
			}

			if (player.is_dead)
			{
				continue;
			}

			if (!player.is_bot)
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_SLAY])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (player.is_bot)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "BOT [%s]",  player.name);							
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slay %i", player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(M_SLAY_MENU_ESCAPE), 	Translate(M_SLAY_MENU_TITLE), next_index,"admin","slay",true,-1);
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Run the RCON vote list menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessRConVote( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		int vote_rcon_index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));

		if (vote_rcon_index == 0) return;
		vote_rcon_index --;

		ProcessMaVoteRCon(admin->index, false, true, true, 3, "ma_votercon", 
								vote_rcon_list[vote_rcon_index].question, 
								vote_rcon_list[vote_rcon_index].rcon_command); 
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 0; i < vote_rcon_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", vote_rcon_list[i].alias);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votercon %i", i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, 
							Translate(M_RCON_VOTE_MENU_ESCAPE), 
							Translate(M_RCON_VOTE_MENU_TITLE), 
							next_index,
							"admin",
							"votercon",
							true,
							-1);
		// Draw menu list
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Run the Question vote list menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessQuestionVote( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		int vote_question_index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));

		if (vote_question_index == 0) return;
		vote_question_index --;

		ProcessMaVoteQuestion(admin->index, false, true, true, 2, "ma_votequestion", 
								vote_question_list[vote_question_index].question, 
								"","","","","","","","","",""); 
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 0; i < vote_question_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", vote_question_list[i].alias);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votequestion %i", i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, 
							Translate(M_QUESTION_VOTE_MENU_ESCAPE), 
							Translate(M_QUESTION_VOTE_MENU_TITLE), 
							next_index,
							"admin",
							"votequestion",
							true,
							-1);
		// Draw menu list
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------

void CAdminPlugin::ProcessExplodeAtCurrentPosition( player_t *player)
{
	Vector pos = player->entity->GetCollideable()->GetCollisionOrigin();

/*	virtual void BloodStream( IRecipientFilter& filter, float delay,
		const Vector* org, const Vector* dir, int r, int g, int b, int a, int amount )
	{
		if ( !SuppressTE( filter ) )
		{
			TE_BloodStream( filter, delay, org, dir, r, g, b, a, amount );
		}
	}
*/
/*	MRecipientFilter mrf2;
	mrf2.MakeReliable();
	mrf2.AddAllPlayers(max_players);

	Vector direction;

	direction.x = randomStr->RandomFloat ( -1, 1 );
	direction.y = randomStr->RandomFloat ( -1, 1 );
	direction.z = randomStr->RandomFloat ( 0, 1 );

	pos.z += 60;
	temp_ents->BloodStream((IRecipientFilter &) mrf2, 0, &pos, &direction, 247, 0, 0, 255, randomStr->RandomInt(50, 150));
	return;*/

/*
	MRecipientFilter mrf2; // this is my class, I'll post it later.
	mrf2.MakeReliable();

	mrf2.AddAllPlayers(max_players);

	int	rocket_trail_index1 = engine->PrecacheModel("sprites/muzzleflash1.vmt", true);
	int	rocket_trail_index2 = engine->PrecacheModel("sprites/muzzleflash2.vmt", true);
	int	rocket_trail_index3 = engine->PrecacheModel("sprites/muzzleflash3.vmt", true);
	int	rocket_trail_index4 = engine->PrecacheModel("sprites/muzzleflash4.vmt", true);
	int	smoke_index = engine->PrecacheModel("sprites/steam1.vmt", true);

	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index1, 0, 1.0, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index2, 0, 1.0, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index3, 0, 1.0, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index4, 0, 1.0, 25, 25, 0.0, 255,255,255,255); 

	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index1, 0, 1.01, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index2, 0, 1.02, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index3, 0, 1.03, 25, 25, 0.0, 255,255,255,255); 
	temp_ents->BeamFollow(mrf2, 0.0,  player->index, rocket_trail_index4, 0, 1.04, 25, 25, 0.0, 255,255,255,255); 

	temp_ents->Smoke(mrf2, 0.0, &pos, smoke_index, 5.0, 12); 

	CBaseEntity *pPlayer = player->entity->GetUnknown()->GetBaseEntity();
	pPlayer->SetGravity(-0.2);

	Vector tap(0,0,200);
	pPlayer->Teleport(NULL, NULL, &tap);


	return;
*/

	//ReSpawnPlayer(player);
/*	CBaseEntity *pPlayer = player->entity->GetUnknown()->GetBaseEntity();
	float gravity = pPlayer->GetGravity();

	gravity += 0.05;
	pPlayer->SetGravity(gravity);

	Msg("Gravity = [%f]\n", gravity);*/

	//SwitchTeam(player);


	// Get player location, so we know where to play the sound.
//	Vector end = pos;

//	end.y += 1000;


	/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/

	// Try beam weapon
//	effects->Beam(pos, end, m_spriteTexture, 0, 0, 10, 10.0, 15, 15, 0, 0, 255, 255, 255, 255, 128);

	if (esounds)
	{
		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();

		mrf.AddAllPlayers(max_players);

		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, slay_sound_name, 0.5,  ATTN_NORM, 0, 100, &pos);
		}
		else
		{
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, hl2mp_slay_sound_name, 0.6,  ATTN_NORM, 0, 100, &pos);
		}
	}

	if (effects)
	{
		// Generate some FXs
		int mag = 60;
		int trailLen = 4;
		pos.z+=12; // rise the effect for better results :)
		effects->Sparks(pos, mag, trailLen, NULL);
	}

//msg_buffer = engine->UserMessageBegin( &mrf2, 9 );

/*	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();
	mrf.AddAllPlayers(max_players);

  msg_buffer = engine->UserMessageBegin( &mrf, radiotext_message_index );

		msg_buffer->WriteByte(0); // Entity Index
//		msg_buffer->WriteByte(player->index); // Entity Index
		msg_buffer->WriteByte(1);

		msg_buffer->WriteString("#Game_radio_location");
//		msg_buffer->WriteString("Param 1");
		msg_buffer->WriteString("Giles");
//		msg_buffer->WriteString("Param 2");
		msg_buffer->WriteString("");
//		msg_buffer->WriteString("Param 3");
		msg_buffer->WriteString("");
//		msg_buffer->WriteString("Param 4");
		msg_buffer->WriteString("");
    	engine->MessageEnd();
*/


//	msg_buffer = engine->UserMessageBegin( &mrf2, 9 );

	//static unsigned char local_step = 0;

	//MRecipientFilter mrf; // this is my class, I'll post it later.
	//mrf.MakeReliable();
	//mrf.AddAllPlayers(max_players);

 // msg_buffer = engine->UserMessageBegin( &mrf, 5 );
 // Msg("Local step = [%d]\n", local_step);

	//	msg_buffer->WriteByte(3); // message type
	//	msg_buffer->WriteByte(local_step++); // Entity Index
	//	msg_buffer->WriteString("Giles Console GILES 1");
	//engine->MessageEnd();

/*

	/*	for (int i = 1; i <= max_players; i++)
	{
		player_t test_player;

		test_player.index = i;
		if (!FindPlayerByIndex(&test_player)) continue;
//		if (test_player.is_bot) continue;
		CBaseEntity *pPlayer = test_player.entity->GetUnknown()->GetBaseEntity();
//		Msg("Model Index = [%i]\n", pPlayer->GetModelIndex());
		pPlayer->SetModelIndex(124);

		//if (pPlayer->m_iTeamNum == 2)
		//{
		//	pPlayer->m_iTeamNum = 3;
		//}
		//else if (pPlayer->m_iTeamNum == 3)
		//{
		//	pPlayer->m_iTeamNum = 2;
		//}

		//test_player.player_info->ChangeTeam(pPlayer->m_iTeamNum);

//		CBaseEntity *pPlayer = test_player.entity->GetUnknown()->GetBaseEntity();
//		CBasePlayer *pBase = (CBasePlayer*) pPlayer;
//		pPlayer->m_fFlags = FL_ONGROUND;
//		pPlayer->m_MoveType = MOVETYPE_WALK;

	}
/*
//		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
//		CBasePlayer *pBase = (CBasePlayer*) pPlayer;
	
//		pBase->m_Local.m_iFOV --;
		//pCombat->m_flFieldOfView --;

// pPlayer->m_MoveType = MOVETYPE_NONE;
		// pPlayer->m_MoveType = MOVETYPE_NOCLIP;
//		pPlayer->m_fFlags = FL_KILLME;
/*		CBasePlayer *pBase = (CBasePlayer*) pPlayer;
		pBase->Ignite(100 ,false, 12, false);
		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
		pCombat->Ignite(30 ,false, 12, false);
	}
/*
	if (mani_render_mode == kRenderNormal) mani_render_mode = kRenderTransColor;
	else if (mani_render_mode == kRenderTransColor) mani_render_mode = kRenderTransTexture;
	else if (mani_render_mode == kRenderTransTexture) mani_render_mode = kRenderGlow;
	else if (mani_render_mode == kRenderGlow) mani_render_mode = kRenderTransAlpha;
	else if (mani_render_mode == kRenderTransAlpha) mani_render_mode = kRenderTransAdd;
	else if (mani_render_mode == kRenderTransAdd) mani_render_mode = kRenderEnvironmental;
	else if (mani_render_mode == kRenderEnvironmental) mani_render_mode = kRenderTransAddFrameBlend;
	else if (mani_render_mode == kRenderTransAddFrameBlend) mani_render_mode = kRenderTransAlphaAdd;
	else if (mani_render_mode == kRenderTransAlphaAdd) mani_render_mode = kRenderWorldGlow;
	else if (mani_render_mode == kRenderWorldGlow) mani_render_mode = kRenderTransAlphaAdd;
	else if (mani_render_mode == kRenderTransAlphaAdd) mani_render_mode = kRenderNone;
	else if (mani_render_mode == kRenderNone) mani_render_mode = kRenderNormal;
*/

//	punish_timings.drugged_in_use = true;
//	punish_mode_list[player->index - 1].drugged = true;
//	CBaseEntity *pPlayer = pPlayerEdict->GetUnknown()->GetBaseEntity();
//	CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();

//	CBaseCombatWeapon *pWeapon = pCombat->static_cast<CBaseCombatWeapon *>( GetActiveWeapon() );
//	CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(message_type);
	
//	if (pWeapon)
//	{
//		Msg("Dropped slot %i weapon_name [%s]\n", message_type ++, pWeapon->GetName());

//		CBasePlayer *pBase = (CBasePlayer*) pPlayer;
//		pBase->RemovePlayerItem(pWeapon);
//	}

/*	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();
	Vector Start;
	Vector End;
	mrf.AddAllPlayers(max_players);

	te->BeamFollow( (IRecipientFilter &)mrf, 0,	1, 0, 0, 10.0, 100, 1, 	10,255, 0, 0, 128 ); */
//	te->BeamPoints( (IRecipientFilter &)mrf, 0.0, &Start, &End, 1, 1, 1, 1,	1,	1, 1, 1, 1, 1, 1, 1, 1, 1 );
//	player_t player;

/*	player.entity = pPlayerEdict;
	FindPlayerByEntity(&player);
	MRecipientFilter mrf2; // this is my class, I'll post it later.
	mrf2.AddAllPlayers(max_players);
*/
//Msg("message_type [%i]\n", message_type);
//msg_buffer = engine->UserMessageBegin( &mrf2, 9 );

/*msg_buffer = engine->UserMessageBegin( &mrf2, vgui_message_index );

		msg_buffer->WriteString( "RANKINGS" ); // menu name
		msg_buffer->WriteByte(1);
		msg_buffer->WriteByte( 2 );
		
		// write additional data (be carefull not more than 192 bytes!)
		while ( subkey )
		{
			msg_buffer->WriteString( subkey->GetName() );
			msg_buffer->WriteString( subkey->GetString() );
			subkey = subkey->GetNextKey();
		}
	MessageEnd();
*/

//		msg_buffer->WriteShort(1536);
//		msg_buffer->WriteShort(1536);
//		msg_buffer->WriteShort(0x0002 | 0x0008);
//		msg_buffer->WriteByte(0);
//		msg_buffer->WriteByte(0);
//		msg_buffer->WriteByte(0);
//		msg_buffer->WriteByte(255);
//		engine->MessageEnd();

// This works for HL2 MP
/*	MRecipientFilter mrf2; // this is my class, I'll post it later.
	mrf2.AddAllPlayers(max_players);

	msg_buffer = engine->UserMessageBegin( &mrf2, hudmsg_message_index );
		msg_buffer->WriteByte ( 1); // channel
		msg_buffer->WriteFloat( 0.5 ); // x
		msg_buffer->WriteFloat( 0.5 ); // y
		msg_buffer->WriteByte ( 255 ); // r
		msg_buffer->WriteByte ( 170 ); // g
		msg_buffer->WriteByte ( 0 ); // b
		msg_buffer->WriteByte ( 255 ); // a
		msg_buffer->WriteByte ( 255 ); // r2
		msg_buffer->WriteByte ( 255 ); // g2
		msg_buffer->WriteByte ( 170 ); // b2
		msg_buffer->WriteByte ( 255 ); // a2
		msg_buffer->WriteByte ( 0 ); // effect
		msg_buffer->WriteFloat( 0 ); // fade in time
		msg_buffer->WriteFloat( 0 ); // fase out time
		msg_buffer->WriteFloat( 3.1 ); // hold time
		msg_buffer->WriteFloat( 0 ); // fxTime
		msg_buffer->WriteString( "TEST" );
engine->MessageEnd();
*/

//msg_buffer = engine->UserMessageBegin( &mrf2, message_type );
//	msg_buffer->WriteByte( 1 );
//	msg_buffer->WriteString("TEST");

// Print to client say area (TextMsg)
//	if (message_type == 5) message_type = 0;

//	msg_buffer->WriteByte(message_type);
//	msg_buffer->WriteString("Hi There");
//	engine->MessageEnd();
//	Msg("Message Type [%i]\n", message_type);
//	message_type ++;

// Print to center of screen (TextMsg)
//	msg_buffer->WriteByte(4);
//	msg_buffer->WriteString("TEST1");

// HudText (NA)
//	msg_buffer->WriteString("Hello");

//	msg_buffer->WriteByte(0);
//	msg_buffer->WriteByte(128);
//	msg_buffer->WriteString("HELLO THERE");
//	engine->MessageEnd();

//	msg_buffer->WriteByte(20);
//	msg_buffer->WriteByte(128);
//	msg_buffer->WriteString("HELLO THERE");

//	message_type ++;

//		virtual bool			GetUserMessageInfo( int msg_type, char *name, int maxnamelength, int& size ) = 0;

/*	char	message_name[1024];
	int		msg_size;

	message_type = 0;
	while (serverdll->GetUserMessageInfo(message_type, message_name, sizeof(message_name), (int &) msg_size))
	{
		Msg("Msg [%i] Name [%s] size[%i]\n", message_type, message_name, msg_size);
		message_type ++;
	}
*/
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Allows admin to run rcon commands from a list
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessRconCommand( player_t *admin, int next_index, int argv_offset )
{

	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	rcon_cmd[512];
		int		rcon_index;

		rcon_index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));
		if (rcon_index < 0 || rcon_index >= rcon_list_size)
		{
			return;
		}

		Q_snprintf( rcon_cmd, sizeof(rcon_cmd),	"%s\n", rcon_list[rcon_index].rcon_command);

		LogCommand (admin->entity, "rcon command [%s]\n", rcon_list[rcon_index].rcon_command);
		engine->ServerCommand(rcon_cmd);
	}
	else
	{
		// Setup rcon list
		FreeMenu();
		CreateList ((void **) &menu_list, sizeof(menu_t), rcon_list_size, &menu_list_size);

		for( int i = 0; i < rcon_list_size; i++ )
		{
			Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", rcon_list[i].alias);							
			Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin rcon %i", i);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		DrawSubMenu (admin, 
							Translate(M_RCON_MENU_ESCAPE), 
							Translate(M_RCON_MENU_TITLE), 
							next_index,
							"admin",
							"rcon",
							true,
							-1);
		// Draw menu list
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Allows admin to run client commands from a list
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecCommand( player_t *admin, char *command, int next_index, int argv_offset )
{

	const int argc = engine->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		char	cexec_cmd[512];
		char	cexec_log_cmd[512];
		int		cexec_index;
		int		length;
		player_t	player;

		cexec_index = Q_atoi(engine->Cmd_Argv(2 + argv_offset));

		if (cexec_index < 0) return;

	    if (FStrEq(command,"cexec_t"))
		{
			// All T's
			if (cexec_index >= cexec_t_list_size) return;

			Q_snprintf( cexec_cmd, sizeof(cexec_cmd), "%s\n", cexec_t_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != TEAM_A)
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin->entity, "[%s] on player [%s]\n", cexec_t_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_ct"))
		{
			// All CT's
			if (cexec_index >= cexec_ct_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_ct_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != TEAM_B)
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin->entity, "[%s] on player [%s]\n", cexec_ct_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_spec"))
		{
			if (!gpManiGameType->IsSpectatorAllowed()) return;

			// All spectators
			if (cexec_index >= cexec_spec_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_spec_list[cexec_index].cexec_command);

			// All CT's
			if (cexec_index >= cexec_ct_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_ct_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.team != gpManiGameType->GetSpectatorIndex())
				{
					continue;
				}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin->entity, "[%s] on player [%s]\n", cexec_spec_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else if (FStrEq(command,"cexec_all"))
		{
			// All players
			if (cexec_index >= cexec_all_list_size) return;
			Q_snprintf( cexec_cmd, sizeof(cexec_cmd),	"%s\n", cexec_all_list[cexec_index].cexec_command);

			for (int i = 1; i <= max_players; i++)
			{
				player.index = i;
				if (!FindPlayerByIndex(&player))
					{
					continue;
					}

				if (player.is_bot)
				{
					continue;
				}

				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
					{
						continue;
					}
				}

				engine->ClientCommand(player.entity, cexec_cmd);	
				LogCommand (admin->entity, "[%s] on player [%s]\n", cexec_all_list[cexec_index].cexec_command, player.steam_id);
			}
		}
		else
		{
			return;
		}

		Q_strcpy(cexec_log_cmd, cexec_cmd);
		// Chop carriage return
		if (cexec_log_cmd)
		{
			length = Q_strlen(cexec_log_cmd);
			if (length != 0)
			{
				cexec_log_cmd[Q_strlen(cexec_log_cmd) - 1] = '\0';
			}
		}

		LogCommand (admin->entity, "%s command [%s]\n", command, cexec_log_cmd);
	}
	else
	{
		FreeMenu();

		// Setup cexec lists
		if (FStrEq(command,"cexec"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_list_size, &menu_list_size);
			for( int i = 0; i < cexec_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin cexec_player %i", i);
			}
		}
		else if (FStrEq(command,"cexec_t"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_t_list_size, &menu_list_size);
			for( int i = 0; i < cexec_t_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_t_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_ct"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_ct_list_size, &menu_list_size);
			for( int i = 0; i < cexec_ct_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_ct_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_spec"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_spec_list_size, &menu_list_size);
			for( int i = 0; i < cexec_spec_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_spec_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}
		else if (FStrEq(command,"cexec_all"))
		{
			CreateList ((void **) &menu_list, sizeof(menu_t), cexec_all_list_size, &menu_list_size);
			for( int i = 0; i < cexec_all_list_size; i++ )
			{
				// Single client
				Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", cexec_all_list[i].alias);							
				Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "admin %s %i", command, i);
			}
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, 
							Translate(M_CLIENT_COMMAND_MENU_ESCAPE), 
							Translate(M_CLIENT_COMMAND_MENU_TITLE), 
							next_index,
							"admin",
							command,
							true,
							-1);
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecPlayer( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		char	client_cmd[512];
		const int	command_index = atoi(engine->Cmd_Argv(2 + argv_offset));

		if (command_index < 0 || command_index >= cexec_list_size)
		{
			return;
		}

		player.user_id = Q_atoi(engine->Cmd_Argv(3 + argv_offset));
		if (!FindPlayerByUserID(&player))
		{
			return;
		}

		if (player.is_bot)
		{
			return;
		}

		int immunity_index = -1;
		if (admin->index != player.index && IsImmune(&player, &immunity_index))
		{
			if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
			{
				return;
			}
		}

		Q_snprintf( client_cmd, sizeof(client_cmd), 
					"%s\n", 
					cexec_list[command_index].cexec_command);
		
		LogCommand (admin->entity, "[%s] on player [%s]\n", cexec_list[command_index].cexec_command, player.steam_id);
		engine->ClientCommand(player.entity, client_cmd);
		return;
	}
	else
	{
		char	more_player_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				continue;
			}

			if (player.is_bot) continue;

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_CEXEC])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_player %s %i", engine->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_player_cmd, sizeof(more_player_cmd), "cexec_player %s", engine->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, 
							Translate(M_CLIENT_PLAYER_COMMAND_MENU_ESCAPE), 
							Translate(M_CLIENT_PLAYER_COMMAND_MENU_TITLE), 
							next_index,
							"admin",
							more_player_cmd,
							true,
							-1);
		// Draw menu list
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose:  Show the Map Management menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMapManagementType(player_t *player, int admin_index )
{

	FreeMenu();


	if (admin_list[admin_index].flags[ALLOW_CHANGEMAP])
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MAP_MANAGE_MENU_CHANGE_MAP));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin changemap");
	}

	if (admin_list[admin_index].flags[ALLOW_CHANGEMAP] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MAP_MANAGE_MENU_SET_NEXT_MAP));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin setnextmap");
	}


	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin");

	DrawStandardMenu(player, Translate(M_MAP_MANAGE_MENU_ESCAPE), Translate(M_MAP_MANAGE_MENU_TITLE), true);

}

//---------------------------------------------------------------------------------
// Purpose:  Show the player Management screens
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerManagementType(player_t *player, int admin_index, int next_index)
{

	FreeMenu();

	if (admin_list[admin_index].flags[ALLOW_SLAY] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_SLAY_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slay");
	}

	if (admin_list[admin_index].flags[ALLOW_KICK] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_KICK_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kicktype");
	}

	if (admin_list[admin_index].flags[ALLOW_BAN] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_BAN_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin bantype");
	}

	if (admin_list[admin_index].flags[ALLOW_SWAP] && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_SWAP_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin swapteam");
	}

	if (admin_list[admin_index].flags[ALLOW_SWAP] && !war_mode && gpManiGameType->IsTeamPlayAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_SPEC_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin specplayer");
	}

	if (admin_list[admin_index].flags[ALLOW_SWAP] && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_BALANCE_TEAMS));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin balanceteam");
	}

	if (admin_list[admin_index].flags[ALLOW_CEXEC_MENU] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_CLIENT_COMMANDS));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexecoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_MUTE] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLAYER_MANAGE_MENU_MUTE_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mute");
	}

	if (admin_list[admin_index].flags[ALLOW_SPRAY_TAG] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Spray Tag Tracking");
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin spray");
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, Translate(M_PLAYER_MANAGE_MENU_ESCAPE), Translate(M_PLAYER_MANAGE_MENU_TITLE), next_index, "admin", "playeroptions", true, -1);

}
//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPunishType(player_t *player, int admin_index, int next_index)
{

	FreeMenu();


	if (admin_list[admin_index].flags[ALLOW_SLAP] && !war_mode && gpManiGameType->IsSlapAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_SLAP_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slapoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_BLIND] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_BLIND_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blindoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_FREEZE] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_FREEZE_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freeze");
	}

	if (admin_list[admin_index].flags[ALLOW_DRUG] && !war_mode && gpManiGameType->IsDrugAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_DRUG_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin drug");
	}

	if (admin_list[admin_index].flags[ALLOW_GIMP] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_GIMP_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin gimp");
	}

	if (admin_list[admin_index].flags[ALLOW_TELEPORT] && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		player_settings_t *player_settings;

		player_settings = FindPlayerSettings(player);
		if (player_settings)
		{
			for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
			{
				if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
				{
					AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
					Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_TELEPORT_PLAYER));
					Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin teleport");
					break;
				}
			}
		}
	}

	if (admin_list[admin_index].flags[ALLOW_TELEPORT] && !war_mode && gpManiGameType->IsTeleportAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_SAVE_TELEPORT_LOC));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin savelocation");
	}

	if (admin_list[admin_index].flags[ALLOW_BURN] && !war_mode && gpManiGameType->IsFireAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_BURN_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin burn");
	}

	if (admin_list[admin_index].flags[ALLOW_NO_CLIP] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_NO_CLIP_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin noclip");
	}

	if (admin_list[admin_index].flags[ALLOW_SETSKINS] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_SET_SKIN));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin skinoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_TIMEBOMB] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_TIME_BOMB_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin timebomb");
	}

	if (admin_list[admin_index].flags[ALLOW_FIREBOMB] && !war_mode && gpManiGameType->IsFireAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_FIRE_BOMB_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin firebomb");
	}

	if (admin_list[admin_index].flags[ALLOW_FREEZEBOMB] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_FREEZE_BOMB_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin freezebomb");
	}

	if (admin_list[admin_index].flags[ALLOW_BEACON] && !war_mode)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_FUN_PLAYER_MANAGE_MENU_BEACON_PLAYER));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin beacon");
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, Translate(M_FUN_PLAYER_MANAGE_MENU_ESCAPE), Translate(M_FUN_PLAYER_MANAGE_MENU_TITLE), next_index, "admin", "punish", true, -1);

}

//---------------------------------------------------------------------------------
// Purpose:  Show the punishment types menu
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessVoteType(player_t *player, int admin_index, int next_index)
{

	if (war_mode) return;

	FreeMenu();

	if (admin_list[admin_index].flags[ALLOW_MENU_RCON_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_RCON_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votercon");
	}

	if (admin_list[admin_index].flags[ALLOW_MENU_QUESTION_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_QUESTION_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votequestion");
	}

	if (admin_list[admin_index].flags[ALLOW_MAP_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_EXTEND_MAP_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteextend");
	}

	if (admin_list[admin_index].flags[ALLOW_RANDOM_MAP_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_RANDOM_MAP_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin randomvoteoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_MAP_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_SINGLE_MAP_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin mapvoteoptions");
	}

	if (admin_list[admin_index].flags[ALLOW_MAP_VOTE] && !system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_BUILD_MULTIMAP_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin buildmapvote");
	}

	if (admin_list[admin_index].flags[ALLOW_MAP_VOTE] && !system_vote.vote_in_progress)
	{
		int		m_list_size = 0;
		map_t	*m_list = NULL;

		// Set pointer to correct list
		switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
		{
			case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
			case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
			case 2: m_list = map_list; m_list_size = map_list_size;break;
			default : break;
		}

		int selected_map = 0;

		for (int i = 0; i < m_list_size; i++)
		{
			if (m_list[i].selected_for_vote)
			{
				selected_map ++;
			}
		}

		if (selected_map != 0)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_START_MULTIMAP_VOTE));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin multimapvoteoptions");
		}
	}

	if (admin_list[admin_index].flags[ALLOW_CANCEL_VOTE] && system_vote.vote_in_progress)
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_VOTE_TYPE_MENU_CANCEL_VOTE));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cancelvote");
	}

	if (menu_list_size == 0) return;


	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	DrawSubMenu (player, 
						Translate(M_VOTE_TYPE_MENU_ESCAPE), 
						Translate(M_VOTE_TYPE_MENU_TITLE), 
						next_index,
						"admin",
						"voteoptions",
						true,
						-1);

}

//---------------------------------------------------------------------------------
// Purpose: Show Delay Type Options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessDelayTypeOptions( player_t *player, const char *menu_command)
{

	char	next_menu[128];

	if (system_vote.vote_in_progress) return;

	if (FStrEq(menu_command,"randomvoteoptions"))
	{
		Q_strcpy(next_menu, "randommapvote");
	}
	else if(FStrEq(menu_command,"mapvoteoptions"))
	{
		Q_strcpy(next_menu, "singlemapvote");
	}
	else 
	{
		// Start multimap vote
		Q_strcpy(next_menu, "multimapvote");
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MAP_CHANGE_TYPE_MENU_IMMEDIATE));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_NO_DELAY);

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MAP_CHANGE_TYPE_MENU_END_OF_ROUND));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_END_OF_ROUND_DELAY);
	}

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MAP_CHANGE_TYPE_MENU_END_OF_MAP));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", next_menu, VOTE_END_OF_MAP_DELAY);

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin voteoptions");

	DrawStandardMenu(player, Translate(M_MAP_CHANGE_TYPE_MENU_ESCAPE), Translate(M_MAP_CHANGE_TYPE_MENU_TITLE), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanOptions( edict_t *pEntity, const char *ban_command)
{
	player_t	player;
	char	ban_type[128];

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	if (FStrEq(ban_command,"banoptions"))
	{
		Q_strcpy(ban_type, "ban");
	}
	else if(FStrEq(ban_command,"banoptionsip"))
	{
		Q_strcpy(ban_type, "banip");
	}
	else 
	{
		Q_strcpy(ban_type, "autobanname");
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_5_MINUTE));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 5", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_30_MINUTE));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 30", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_1_HOUR));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 60", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_2_HOUR));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 120", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_1_DAY));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 1440", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_7_DAY));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 10080", ban_type);

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_OPTIONS_MENU_PERMANENT));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s 0", ban_type);

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(&player, Translate(M_BAN_OPTIONS_MENU_ESCAPE), Translate(M_BAN_OPTIONS_MENU_TITLE), true);

	return;
}


//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessSlapOptions( edict_t *pEntity)
{
	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	if (!gpManiGameType->IsSlapAllowed()) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_0_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 0");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_1_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 1");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_5_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 5");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_10_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 10");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_20_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 20");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_50_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 50");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_SLAP_OPTIONS_MENU_99_HEALTH));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin slap 99");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");

	DrawStandardMenu(&player, Translate(M_SLAP_OPTIONS_MENU_ESCAPE), Translate(M_SLAP_OPTIONS_MENU_TITLE), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Show user the blindness amounts
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBlindOptions( edict_t *pEntity)
{
	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BLIND_OPTIONS_MENU_UNBLIND));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 0");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BLIND_OPTIONS_MENU_PARTIAL));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 245");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BLIND_OPTIONS_MENU_FULL));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin blind 255");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin punish");

	DrawStandardMenu(&player, Translate(M_BLIND_OPTIONS_MENU_ESCAPE), Translate(M_BLIND_OPTIONS_MENU_TITLE), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanType( player_t *player )
{
	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_TYPE_MENU_STEAM));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin banoptions");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_TYPE_MENU_IP));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin banoptionsip");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BAN_TYPE_MENU_AUTOBAN_NAME));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autobanoptionsname");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(player, Translate(M_BAN_TYPE_MENU_ESCAPE), Translate(M_BAN_TYPE_MENU_TITLE), true);

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessKickType(player_t *player )
{
	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_KICK_TYPE_MENU_KICK_NOW));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin kick");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_KICK_TYPE_MENU_NAME));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokickname");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_KICK_TYPE_MENU_STEAM));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokicksteam");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_KICK_TYPE_MENU_IP));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin autokickip");

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(player, Translate(M_KICK_TYPE_MENU_ESCAPE), Translate(M_KICK_TYPE_MENU_TITLE), true);

	return;
}


//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCExecOptions( edict_t *pEntity )
{
	player_t	player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}


	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_CLIENT_EXEC_OPTIONS_MENU_SINGLE));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_CLIENT_EXEC_OPTIONS_MENU_ALL));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_all");

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_CLIENT_EXEC_OPTIONS_MENU_T));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_t");

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_CLIENT_EXEC_OPTIONS_MENU_CT));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_ct");
	}

	if (gpManiGameType->IsSpectatorAllowed())
	{
		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_CLIENT_EXEC_OPTIONS_MENU_SPEC));
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin cexec_spec");
	}

	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin playeroptions");

	DrawStandardMenu(&player, Translate(M_CLIENT_EXEC_OPTIONS_MENU_ESCAPE), Translate(M_CLIENT_EXEC_OPTIONS_MENU_TITLE), true);

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Plugin config options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessConfigOptions( edict_t *pEntity )
{

	player_t	player;

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	FreeMenu();

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_adverts.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_DISABLE_ADVERTS));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_ENABLE_ADVERTS));
	}

	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle adverts");

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_tk_protection.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_DISABLE_TK_PROTECT));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_ENABLE_TK_PROTECT));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle tk_protection");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_tk_forgive.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_DISABLE_TK_FORGIVE));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_ENABLE_TK_FORGIVE));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle tk_forgive");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_war_mode.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_DISABLE_WAR_MODE));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_ENABLE_WAR_MODE));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle warmode");


	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	if (mani_stats.GetInt() == 1)
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_DISABLE_STATS));
	}
	else
	{
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_ENABLE_STATS));
	}
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle stats");


	if (mani_stats.GetInt() == 1)
	{
		int admin_index = -1;
		if (IsClientAdmin(&player, &admin_index))
		{
			if (admin_list[admin_index].flags[ALLOW_RESET_ALL_RANKS])
			{
				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_PLUGIN_CONFIG_MENU_RESET_RANKS));
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin toggle resetstats");
			}
		}
	}


	if (menu_list_size == 0) return;

	AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
	Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_BACK));
	Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin");

	DrawStandardMenu(&player, Translate(M_PLUGIN_CONFIG_MENU_ESCAPE), Translate(M_PLUGIN_CONFIG_MENU_TITLE), true);

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Plugin config options
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessConfigToggle( edict_t *pEntity )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	// Must have 3 arguements
	if (argc != 3)
	{
		return;
	}

	player.entity = pEntity;
	if (!FindPlayerByEntity(&player))
	{
		return;
	}

	if (FStrEq(engine->Cmd_Argv(2), "adverts"))
	{
		ToggleAdverts(&player);
	}
	else if (FStrEq(engine->Cmd_Argv(2), "tk_protection"))
	{
		if (mani_tk_protection.GetInt() == 1)
		{
			mani_tk_protection.SetValue(0);
			FreeList((void **) &tk_player_list, &tk_player_list_size);
			SayToAll (true, "ADMIN %s disabled tk protection", player.name);
			LogCommand (player.entity, "Disable tk protection\n");
		}
		else
		{
			mani_tk_protection.SetValue(1);
			SayToAll (true, "ADMIN %s enabled tk protection", player.name);
			LogCommand (player.entity, "Enable tk protection\n");
			// Need to turn off tk punish
			engine->ServerCommand("mp_tkpunish 0\n");
		}
		return;
	}
	else if (FStrEq(engine->Cmd_Argv(2), "tk_forgive"))
	{
		if (mani_tk_forgive.GetInt() == 1)
		{
			mani_tk_forgive.SetValue(0);
			SayToAll (true, "ADMIN %s disabled tk forgive options", player.name);
			LogCommand (player.entity, "Disable tk forgive\n");
		}
		else
		{
			mani_tk_forgive.SetValue(1);
			SayToAll (true, "ADMIN %s enabled tk forgive options", player.name);
			LogCommand (player.entity, "Enable tk forgive\n");
		}
		return;
	}
	else if (FStrEq(engine->Cmd_Argv(2), "warmode"))
	{
		if (mani_war_mode.GetInt() == 1)
		{
			mani_war_mode.SetValue(0);
			SayToAll (true, "ADMIN %s disabled War Mode", player.name);
			LogCommand (player.entity, "Disable war mode\n");
		}
		else
		{
			SayToAll (true, "ADMIN %s enabled War Mode", player.name);
			LogCommand (player.entity, "Enable war mode\n");
			mani_war_mode.SetValue(1);
		}
		return;
	}
	else if (FStrEq(engine->Cmd_Argv(2), "stats"))
	{
		if (mani_stats.GetInt() == 1)
		{
			mani_stats.SetValue(0);
			SayToAll (true, "ADMIN %s disabled stats", player.name);
			LogCommand (player.entity, "Disable stats\n");
		}
		else
		{
			mani_stats.SetValue(1);
			SayToAll (true, "ADMIN %s enabled stats", player.name);
			LogCommand (player.entity, "Enable stats\n");
		}
		return;
	}
	else if (FStrEq(engine->Cmd_Argv(2), "resetstats"))
	{
		int admin_index = -1;
		if (!IsClientAdmin(&player, &admin_index)) return;
		if (!admin_list[admin_index].flags[ALLOW_RESET_ALL_RANKS]) return;

		ResetStats ();
		SayToAll (true, "ADMIN %s reset the stats", player.name);
		LogCommand (player.entity, "Reset stats\n");
		return;
	}
	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		if (FStrEq("ban", ban_command))
		{
			ProcessMaBan(admin->index, false, false, 3, "ma_ban", 
					engine->Cmd_Argv(2 + argv_offset), 
					engine->Cmd_Argv(3 + argv_offset));
		}
		else
		{
			ProcessMaBan(admin->index, false, true, 3, "ma_ban", 
					engine->Cmd_Argv(2 + argv_offset), 
					engine->Cmd_Argv(3 + argv_offset));
		}

		return;
	}
	else
	{
		char	more_ban_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;

			int immunity_index = -1;
			if (admin->index != player.index && IsImmune(&player, &immunity_index))
			{
				if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BAN])
				{
					continue;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %s %i", ban_command, engine->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_ban_cmd, sizeof(more_ban_cmd), "%s %s", ban_command, engine->Cmd_Argv(2 + argv_offset));

		if (FStrEq("ban", ban_command))
		{
			DrawSubMenu (admin, Translate(M_BAN_PLAYER_MENU_ESCAPE), Translate(M_BAN_PLAYER_MENU_STEAM), next_index, "admin", more_ban_cmd,	true, -1);
		}
		else
		{
			DrawSubMenu (admin, Translate(M_BAN_PLAYER_MENU_ESCAPE), Translate(M_BAN_PLAYER_MENU_IP), next_index, "admin", more_ban_cmd, true, -1);
		}
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSystemVoteRandomMap( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	if (argc - argv_offset == 4)
	{
		char	delay_type_string[64];
		int		delay_type = Q_atoi(engine->Cmd_Argv(2 + argv_offset)); // delay type

		if (delay_type == VOTE_NO_DELAY)
		{
			Q_strcpy (delay_type_string, "now");
		}
		else if (delay_type == VOTE_END_OF_ROUND_DELAY)
		{
			Q_strcpy (delay_type_string, "round");
		}
		else
		{
			Q_strcpy (delay_type_string, "end");
		}

		// Run the system vote
		ProcessMaVoteRandom(admin->index, false, true, 3, "ma_voterandom", delay_type_string, engine->Cmd_Argv(3 + argv_offset));
		return;
	}
	else
	{
		char	more_cmd[128];
		int		m_list_size;

		// Setup player list
		FreeMenu();

		// Pick the right map list size
		if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 0)
		{
			m_list_size = map_in_cycle_list_size;
		}
		else if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 1)
		{
			m_list_size = votemap_list_size;
		}
		else
		{
			m_list_size = map_list_size;
		}

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " [%i]",  i + 1);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin randommapvote %s %i", engine->Cmd_Argv(2 + argv_offset), i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "randommapvote %s", engine->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(M_RANDOM_MAP_MENU_ESCAPE), Translate(M_RANDOM_MAP_MENU_TITLE), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSystemVoteSingleMap( player_t *admin, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	if (argc - argv_offset == 4)
	{
		char	delay_type_string[64];
		int		delay_type = Q_atoi(engine->Cmd_Argv(2 + argv_offset)); // delay type

		if (delay_type == VOTE_NO_DELAY)
		{
			Q_strcpy (delay_type_string, "now");
		}
		else if (delay_type == VOTE_END_OF_ROUND_DELAY)
		{
			Q_strcpy (delay_type_string, "round");
		}
		else
		{
			Q_strcpy (delay_type_string, "end");
		}

		// Run the system vote
		ProcessMaVote(admin->index, false, true, 3, "ma_vote", delay_type_string, engine->Cmd_Argv(3 + argv_offset),
										"","","","","","","","","");
		return;
	}
	else
	{
		char	more_cmd[128];
		int		m_list_size = 0;
		map_t	*m_list = NULL;

		// Setup player list
		FreeMenu();

		// Set pointer to correct list
		switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
		{
			case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
			case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
			case 2: m_list = map_list; m_list_size = map_list_size;break;
			default : break;
		}

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " %s",  m_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin singlemapvote %s %s", engine->Cmd_Argv(2 + argv_offset), m_list[i].map_name);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, m_list[i].map_name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "singlemapvote %s", engine->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(M_SINGLE_MAP_MENU_ESCAPE), Translate(M_SINGLE_MAP_MENU_TITLE), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSystemVoteBuildMap( player_t *admin, int next_index, int argv_offset )
{
	int		m_list_size = 0;
	map_t	*m_list = NULL;

	const int argc = engine->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	if (argc - argv_offset == 3)
	{
		// Map added or taken away from list

		int		map_index = -1;

		for (int i = 0; i < m_list_size; i++)
		{
			if (FStrEq(m_list[i].map_name, engine->Cmd_Argv(2 + argv_offset)))
			{
				map_index = i;
				break;
			}
		}

		if (map_index == -1) 
		{
			SayToPlayer(admin, "Invalid map [%s]", engine->Cmd_Argv(2 + argv_offset));
			return;
		}

		if (m_list[map_index].selected_for_vote)
		{
			m_list[map_index].selected_for_vote = false;
			SayToPlayer(admin, Translate(M_BUILD_MAP_MENU_DESELECT_MAP), m_list[map_index].map_name);
		}
		else
		{
			m_list[map_index].selected_for_vote = true;
			SayToPlayer(admin, Translate(M_BUILD_MAP_MENU_SELECT_MAP), m_list[map_index].map_name);
		}

		engine->ClientCommand(admin->entity, "admin buildmapvote\n");
		return;
	}
	else
	{
		char	more_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (m_list[i].selected_for_vote)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_BUILD_MAP_MENU_ADDED),  m_list[i].map_name);
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " %s",  m_list[i].map_name);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin buildmapvote %s %s", engine->Cmd_Argv(2 + argv_offset), m_list[i].map_name);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, m_list[i].map_name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "buildmapvote %s", engine->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(M_BUILD_MAP_MENU_ESCAPE), Translate(M_BUILD_MAP_MENU_TITLE), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSystemVoteMultiMap( player_t *admin, int admin_index )
{
	if (system_vote.vote_in_progress) return;

	char	delay_type_string[64];
	int		delay_type = Q_atoi(engine->Cmd_Argv(2)); // delay type

	if (delay_type == VOTE_NO_DELAY)
	{
		Q_strcpy (delay_type_string, "now");
	}
	else if (delay_type == VOTE_END_OF_ROUND_DELAY)
	{
		Q_strcpy (delay_type_string, "round");
	}
	else
	{
		Q_strcpy (delay_type_string, "end");
	}

	int		m_list_size = 0;
	map_t	*m_list = NULL;

	// Setup player list
	FreeMenu();

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	int selected_map = 0;

	for (int i = 0; i < m_list_size; i++)
	{
		if (m_list[i].selected_for_vote)
		{
			selected_map ++;
		}
	}

	if (selected_map == 0) return;

	// Build the vote maps
	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	if (mani_vote_allow_extend.GetInt() == 1 && selected_map != 1)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend Map");
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	for (int i = 0; i < m_list_size; i++)
	{
		if (m_list[i].selected_for_vote)
		{
			AddMapToVote(admin, false, true, m_list[i].map_name);
			m_list[i].selected_for_vote = false;
		}
	}

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	system_vote.vote_starter = admin->index;
	system_vote.vote_confirmation = false;

	if (admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
	}

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(admin->entity, "Started a random map vote\n");
	AdminSayToAll(admin, mani_adminvote_anonymous.GetInt(), "started a map vote"); 
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessAutoBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 4)
	{
		player.user_id = Q_atoi(engine->Cmd_Argv(3 + argv_offset));
		if (!FindPlayerByUserID(&player))
		{
			return;
		}

		if (player.is_bot)
		{
			return;
		}

		g_ManiAdminPlugin.ProcessMaAutoKickBanName
				(
				admin->index, // Client index
				false,  // Sever console command type
				3, // Number of arguments
				"ma_aban_name", // Command
				player.name, // ip address
				engine->Cmd_Argv(2 + argv_offset), // Ban time
				false // Ban only
				);	

		return;
	}
	else
	{
		char	more_ban_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player))
			{
				continue;
			}
			
			if (player.is_bot)
			{
				continue;
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %s %i", ban_command, engine->Cmd_Argv(2 + argv_offset), player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_ban_cmd, sizeof(more_ban_cmd), "%s %s", ban_command, engine->Cmd_Argv(2 + argv_offset));
		DrawSubMenu (admin, Translate(M_AUTOBAN_NAME_MENU_ESCAPE), Translate(M_AUTOBAN_NAME_MENU_TITLE), next_index, "admin", more_ban_cmd,	true, -1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessAutoKickPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	player_t	player;

	if (argc - argv_offset == 3)
	{
		player.user_id = Q_atoi(engine->Cmd_Argv(2 + argv_offset));
		if (!FindPlayerByUserID(&player))
		{
			return;
		}

		if (player.is_bot)
		{
			return;
		}

		if (FStrEq("autokicksteam", ban_command))
		{
			g_ManiAdminPlugin.ProcessMaAutoKickSteam
					(
					admin->index, // Client index
					false,  // Sever console command type
					2, // Number of arguments
					"ma_akick_steam", // Command
					player.steam_id
					);	
		}
		else if (FStrEq("autokickip", ban_command))
		{
			g_ManiAdminPlugin.ProcessMaAutoKickIP
					(
					admin->index, // Client index
					false,  // Sever console command type
					2, // Number of arguments
					"ma_akick_ip", // Command
					player.ip_address
					);	
		}
		else
		{
			g_ManiAdminPlugin.ProcessMaAutoKickBanName
					(
					admin->index, // Client index
					false,  // Sever console command type
					2, // Number of arguments
					"ma_akick_name", // Command
					player.name, // name
					0, // Fake time
					true // Kick only
					);	
		}

		return;
	}
	else
	{
		char	more_ban_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 1; i <= max_players; i++ )
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue; 
			if (player.is_bot) continue;

			if (FStrEq("autokicksteam", ban_command) ||
				FStrEq("autokickip", ban_command))
			{
				int immunity_index = -1;
				if (admin->index != player.index && IsImmune(&player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BAN])
					{
						continue;
					}
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%s] %i",  player.name, player.user_id);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin %s %i", ban_command, player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, player.name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_ban_cmd, sizeof(more_ban_cmd), "%s %s", ban_command, engine->Cmd_Argv(2 + argv_offset));

		if (FStrEq("autokicksteam", ban_command))
		{
			DrawSubMenu (admin, Translate(M_AUTOKICK_MENU_ESCAPE_STEAM), Translate(M_AUTOKICK_MENU_TITLE_STEAM), next_index, "admin", more_ban_cmd,	true, -1);
		}
		else if (FStrEq("autokickip", ban_command))
		{
			DrawSubMenu (admin, Translate(M_AUTOKICK_MENU_ESCAPE_IP), Translate(M_AUTOKICK_MENU_TITLE_IP), next_index, "admin", more_ban_cmd, true, -1);
		}
		else
		{
			DrawSubMenu (admin, Translate(M_AUTOKICK_MENU_ESCAPE_NAME), Translate(M_AUTOKICK_MENU_TITLE_NAME), next_index, "admin", more_ban_cmd,	true, -1);
		}
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdminPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{

	if (ProcessPluginPaused()) return PLUGIN_CONTINUE;
	if (mani_stats.GetInt() == 1)
	{
		player_t player;

		Q_strcpy(player.steam_id,pszNetworkID);
		Q_strcpy(player.name, pszUserName);
		if (mani_stats_by_steam_id.GetInt() == 1)
		{
			AddPlayerToRankList(&player);
		}
		else
		{
			AddPlayerNameToRankList(&player);
		}
	}

	return PLUGIN_CONTINUE;
}
//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CAdminPlugin::FireGameEvent( IGameEvent * event )
{

	if (ProcessPluginPaused()) return;
//		PrettyPrinter(event, 4);

	const char *eventname = event->GetName();

 // Msg("Event Name [%s]\n", eventname);
	if (FStrEq(eventname, "weapon_fire"))
	{
		bool hegrenade = false;

		int user_id = event->GetInt("userid", -1);
		if (FStrEq(event->GetString("weapon", "NULL"),"hegrenade"))
		{
			hegrenade = true;
		}
		
		for (int i = 1; i <= max_players; i++)
		{
			edict_t *pPlayerEdict = engine->PEntityOfEntIndex(i);
			if(pPlayerEdict && !pPlayerEdict->IsFree() )
			{
				IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pPlayerEdict );
				if (playerinfo && playerinfo->IsConnected())
				{
					if (playerinfo->IsHLTV()) continue;

					int player_user_id = playerinfo->GetUserID();
					if (player_user_id == user_id)
					{
						if (punish_mode_list[i - 1].frozen && strcmp(playerinfo->GetNetworkIDString(),"BOT") != 0)
						{
							engine->ClientCommand(pPlayerEdict,"drop\n");
						}
						else
						{
							if (!war_mode &&
								hegrenade && 
								mani_unlimited_grenades.GetInt() != 0 &&  
								gpManiGameType->IsGameType(MANI_GAME_CSS))
							{
								CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(pPlayerEdict), "weapon_hegrenade");
							}
						}
					}
				}
			}
		}

		return;
	}
	else if (FStrEq(eventname, "round_start"))
	{
		EffectsRoundStart();

		ProcessQuakeRoundStart();
		ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_ROUNDSTART);

		if (get_new_timeleft_offset || 	!round_end_found)
		{
			get_new_timeleft_offset = false;
			timeleft_offset = gpGlobals->curtime;
			round_number = 0;
		}

		round_end_found = false;
		gpManiVictimStats->RoundStart();

		for (int i = 0; i < MANI_MAX_PLAYERS; i++)
		{
			sounds_played[i] = 0;
			tw_spam_list[i].index = -99;
			tw_spam_list[i].last_time = -99.0;
			if (mani_player_name_change_reset.GetInt() == 0)
			{
				name_changes[i] = 0;
			}
		}

		round_number ++;
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			RemoveRestrictedWeapons();
		}

		gpManiGhost->RoundStart();
	}
	else if (FStrEq(eventname, "player_team") && gpManiGameType->IsTeamPlayAllowed())
	{
		player_t join_player;

		join_player.user_id = event->GetInt("userid", -1);
		if (join_player.user_id == -1) return;
		if (!FindPlayerByUserID(&join_player)) return;
		join_player.team = event->GetInt("team", 1);

		SkinTeamJoin(&join_player);
		return;
	}
	else if (FStrEq(eventname, "round_end"))
	{
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			int winning_team = event->GetInt("winner", -1);
			if (winning_team > 1 && winning_team < MANI_MAX_TEAMS)
			{
				team_scores.team_score[winning_team] ++;
			}
		}

		ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_ROUNDSTART);

		if(FStrEq("#Game_Commencing", event->GetString("message", "NULL")))
		{
			get_new_timeleft_offset = true;
			timeleft_offset = gpGlobals->curtime;
			for (int i = 0; i < MANI_MAX_TEAMS; i++)
			{
				team_scores.team_score[i] = 0;
			}
		}

		round_end_found = true;

		if (mani_autobalance_teams.GetInt() == 1 && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			change_team = true;
			change_team_time = gpGlobals->curtime + 3.0;
		}

		gpManiVictimStats->RoundEnd();

		if (mani_stats.GetInt() == 1 && !war_mode)
		{
			Msg("Recalculating stats\n");
			if (mani_stats_by_steam_id.GetInt() == 1)
			{
				CalculateStats(true);
			}
			else
			{
				CalculateNameStats(true);
			}
		}

		// Reset drug mode
		for (int i = 0; i < MANI_MAX_PLAYERS; i++)
		{
			punish_mode_list[i].drugged = 0;
		}
	}
	else if (FStrEq(eventname, "round_freeze_end"))
	{
		if (mani_tk_protection.GetInt() == 1 && !war_mode)
		{
			// Start spawn protection timer
			float spawn_protection_start_time;
			spawn_protection_start_time = gpGlobals->curtime;
			// Pre calculate time
			end_spawn_protection_time = spawn_protection_start_time + mani_tk_spawn_time.GetFloat();
		}
	}
	else if (FStrEq(eventname, "player_hurt"))
	{
		ProcessPlayerHurt (event);
	}
//	else if (FStrEq(eventname, "player_team"))
//	{
//		if (mani_tk_protection.GetInt() == 1 && !war_mode)
//		{
			// Check if player is doing the 'rejoin trick'
			//ProcessPlayerTeam (event);
//		}
//	}
	else if (FStrEq(eventname, "player_spawn"))
	{
		player_t spawn_player;

		spawn_player.user_id = event->GetInt("userid", -1);
		if (spawn_player.user_id == -1) return;
		if (!FindPlayerByUserID(&spawn_player)) return;
		//CBaseEntity *pPlayer = spawn_player.entity->GetUnknown()->GetBaseEntity();
		ProcessSetColour(spawn_player.entity, 255, 255, 255, 255 );

		ForceSkinType(&spawn_player);

		// Reset any effects flags
		EffectsClientDisconnect(spawn_player.index - 1, true);
		gpManiVictimStats->PlayerSpawn(&spawn_player);

		if (mani_tk_protection.GetInt() == 1 && 
			(mp_friendlyfire && mp_friendlyfire->GetInt() == 1) && 
			!war_mode)
		{
			// Handle delayed punishments
			ProcessTKSpawnPunishment(&spawn_player);
		}

		if (!gpManiGameType->IsTeamPlayAllowed())
		{
			SkinTeamJoin(&spawn_player);
		}

		// Give grenade to player if unlimited grenades
		if (!war_mode &&
			mani_unlimited_grenades.GetInt() != 0 &&  
			gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(spawn_player.entity), "weapon_hegrenade");
		}
	}
	else if (FStrEq(eventname, "player_death"))
	{
		if (mani_stats.GetInt() == 1 && !war_mode)
		{
			if (mani_stats_by_steam_id.GetInt() == 1)
			{
				ProcessDeathStats(event);
			}
			else
			{
				ProcessNameDeathStats(event);
			}
		}

		ProcessPlayerDeath(event);
		ProcessQuakeDeath(event);
	}
	else if (FStrEq(eventname, "player_say"))
	{
		ProcessPlayerSay (event);
	}

//	else if (FStrEq(eventname, "player_changename"))
//	{
//		Msg("Changed name via event\n");
//		ProcessChangeName (event->GetString("newname","NA"), event->GetString("oldname", "NA"), event->GetInt("userid", -1)); 
//	}
//	else if (FStrEq(eventname, "player_spawn"))
//	{
//		Msg("Player Spawn [%i]\n", event->GetInt("userid", -1));
//	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle a player changing name
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessChangeName(player_t *player, const char *new_name, char *old_name)
{
//	Msg("ProcessChangeName : Name changed to \"%s\" (from \"%s\"\n", new_name, old_name);

	if (war_mode) return;

	bool tag_immunity = false;
	int immunity_index = -1;
	if (IsImmune(player, &immunity_index))
	{
		if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_TAG])
		{
			tag_immunity = true;
		}
	}

	char	kick_cmd[512];
	char	ban_cmd[512];

	name_changes[player->index - 1] ++;

	if (mani_player_name_change_threshold.GetInt() != 0)
	{
		if (mani_player_name_change_threshold.GetInt() < name_changes[player->index - 1])
		{
			if (mani_player_name_change_punishment.GetInt() == 0)
			{
				SayToAll(false,"Player was kicked for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto kicked for name hacking\n");
				Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
				LogCommand (NULL, "Kick (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
				engine->ServerCommand(kick_cmd);				
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 1 && sv_lan->GetInt() != 1)
			{
				// Ban by user id
				SayToAll(false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->user_id);
				LogCommand(NULL, "Banned (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 2)
			{
				// Ban by user ip address
				SayToAll(false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->ip_address);
				LogCommand(NULL, "Banned IP (Name change threshold) [%s] [%s] %s", player->name, player->ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				name_changes[player->index - 1] = 0;
				return;
			}
			else if (mani_player_name_change_punishment.GetInt() == 3)
			{
				// Ban by user id and ip address
				SayToAll(false,"Player was banned for name change hacking");
				PrintToClientConsole(player->entity, "You have been auto banned for name hacking\n");

				if (sv_lan->GetInt() != 1)
				{
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
											mani_player_name_change_ban_time.GetInt(), 
											player->user_id);
					LogCommand(NULL, "Banned (Name change threshold) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);	
					engine->ServerCommand("writeid\n");
				}

				Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_player_name_change_ban_time.GetInt(), 
										player->ip_address);
				LogCommand(NULL, "Banned IP (Name change threshold) [%s] [%s] %s", player->name, player->ip_address, ban_cmd);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeip\n");
				name_changes[player->index - 1] = 0;
				return;
			}
		}
	}

	if (mani_stats.GetInt() == 1 && mani_stats_by_steam_id.GetInt() == 0)
	{
		AddPlayerNameToRankList(player);
	}

	if (!tag_immunity)
	{
		for (int i = 0; i < autokick_name_list_size; i++)
		{
			if (FStrEq(new_name, autokick_name_list[i].name))
			{
				// Matched name of player
				if (autokick_name_list[i].kick)
				{
					PrintToClientConsole(player->entity, "You have been autokicked\n");
					Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
					engine->ServerCommand(kick_cmd);				
					return;
				}
				else if (autokick_name_list[i].ban && sv_lan->GetInt() != 1)
				{
					// Ban by user id
					PrintToClientConsole(player->entity, "You have been auto banned\n");
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", autokick_name_list[i].ban_time, player->user_id);
					LogCommand(NULL, "Banned (Bad Name) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);
					engine->ServerCommand("writeid\n");
					return;
				}
			}
		}	
	}
	
	if (!tag_immunity)
	{
		for (int i = 0; i < autokick_pname_list_size; i++)
		{
			if (NULL != Q_stristr(new_name, autokick_pname_list[i].pname))
			{
				// Matched name of player
				if (autokick_pname_list[i].kick)
				{
					PrintToClientConsole(player->entity, "You have been autokicked\n");
					Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
					engine->ServerCommand(kick_cmd);				
					return;
				}
				else if (autokick_pname_list[i].ban && sv_lan->GetInt() != 1)
				{
					// Ban by user id
					PrintToClientConsole(player->entity, "You have been auto banned\n");
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", autokick_pname_list[i].ban_time, player->user_id);
					LogCommand(NULL, "Banned (Bad Name) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);
					engine->ServerCommand("writeid\n");
					return;
				}
			}
		}	
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle player say event
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerSay( IGameEvent *event)
{
	player_t player;
	int user_id = event->GetInt("userid", -1);
	const char *say_string = event->GetString("text", "");

	if (user_id == -1) return;

	player.user_id = user_id;
	if (!FindPlayerByUserID(&player)) return;

	// Check for web shortcuts that have been said in game
	if (ProcessWebShortcuts(player.entity, say_string))
	{
		return;
	}

	if (FStrEq(say_string, "nextmap") && !war_mode) {ProcessMaNextMap(player.index, false); return;}
	else if (FStrEq(say_string, "damage") && !war_mode) {ProcessMaDamage(player.index); return;}
	else if (FStrEq(say_string, "deathbeam") && !war_mode) {ProcessMaDeathBeam(player.index); return;}
	else if (FStrEq(say_string, "sounds") && !war_mode) {ProcessMaSounds(player.index); return;}
	else if (FStrEq(say_string, "quake") && !war_mode) {ProcessMaQuake(player.index); return;}
	else if (FStrEq(say_string, "settings") && !war_mode) {ShowSettingsPrimaryMenu(&player, 0); return;}
	else if (FStrEq(say_string, "timeleft") && !war_mode) {ProcessMaTimeLeft(player.index, false); return;}
	else if (FStrEq(say_string, "listmaps") && !war_mode) 
	{
		ProcessMaListMaps(player.index, false);
		SayToPlayer(&player,"Check your console for the list of maps !!");
	}
	else if ((FStrEq(say_string, "motd") || FStrEq(say_string, "rules")) && !war_mode)
	{
		MRecipientFilter mrf;
		mrf.AddPlayer(player.index);
		DrawMOTD(&mrf);
		return;
	}
	else if (FStrEq(say_string, "votemap") && !war_mode && 
			 mani_voting.GetInt() == 1 && 
			 mani_vote_allow_user_vote_map.GetInt() == 1) 
	{
		engine->ClientCommand(player.entity, "mani_uservotemapmenu\n"); return;
	}
	else if (FStrEq(say_string, "votekick") && !war_mode && 
			 mani_voting.GetInt() == 1 && 
			 mani_vote_allow_user_vote_kick.GetInt() == 1) 
	{
		engine->ClientCommand(player.entity, "mani_uservotekickmenu\n"); return;
	}
	else if (FStrEq(say_string, "voteban") && !war_mode && 
			 mani_voting.GetInt() == 1 && 
			 mani_vote_allow_user_vote_ban.GetInt() == 1) 
	{
		engine->ClientCommand(player.entity, "mani_uservotebanmenu\n"); return;
	}
	else if (FStrEq(say_string, "thetime") && !war_mode)
	{
		char	time_text[128];
		char	tmp_buf[128];
		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		current_time += (time_t) (mani_adjust_time.GetInt() * 60);

		time_now = localtime(&current_time);
		if (mani_military_time.GetInt() == 1)
		{
			// Miltary 24 hour
			strftime(tmp_buf, sizeof(tmp_buf), "%H:%M:%S",time_now);
		}
		else
		{
			// Standard 12 hour clock
			strftime(tmp_buf, sizeof(tmp_buf), "%I:%M:%S %p",time_now);
		}

		Q_snprintf( time_text, sizeof(time_text), "The time is : %s %s\n", tmp_buf, mani_thetime_timezone.GetString());
		Msg("The local time [%s]\n", time_text);
		PrintToClientConsole(player.entity, "%s", time_text);
		if (mani_thetime_player_only.GetInt() == 1)
		{
			ClientMsgSinglePlayer(player.entity, 10, 4, "The time is : %s %s", tmp_buf, mani_thetime_timezone.GetString());
		}
		else
		{
			Color white(255,255,255,255);
			ClientMsg(&white, 15, false, 4, "The time is : %s %s", tmp_buf, mani_thetime_timezone.GetString());
		}
	}
	else if (FStrEq(say_string, "ff") && !war_mode)
	{
		char	ff_message[128];

		if (mp_friendlyfire)
		{
			if (mp_friendlyfire->GetInt() == 1 )
			{
				Q_snprintf(ff_message, sizeof(ff_message), "Friendly fire is on");
				PrintToClientConsole(player.entity, "Friendly fire is on\n");
			}
			else
			{
				Q_snprintf(ff_message, sizeof(ff_message), "Friendly fire is off");
				PrintToClientConsole(player.entity, "Friendly fire is off\n");
			}

			if (mani_ff_player_only.GetInt() == 1)
			{
				ClientMsgSinglePlayer(player.entity, 15, 4, "%s", ff_message);
			}
			else
			{
				Color	white(255,255,255,255);
				ClientMsg(&white, 15, false, 4, "%s", ff_message);
			}
		}
	}
	else if (toupper(say_string[0]) == 'T' &&
			toupper(say_string[1]) == 'O' &&
			toupper(say_string[2]) == 'P')
	{
		if (mani_stats.GetInt() == 0 || war_mode)
		{
			return;
		}

		if (FStrEq(say_string,"TOP")) ShowTop(&player, 5);
		else if (FStrEq(say_string,"TOP1"))	 ShowTop(&player, 1);
		else if (FStrEq(say_string,"TOP2"))	 ShowTop(&player, 2);
		else if (FStrEq(say_string,"TOP3"))	 ShowTop(&player, 3);
		else if (FStrEq(say_string,"TOP4"))	 ShowTop(&player, 4);
		else if (FStrEq(say_string,"TOP5"))	 ShowTop(&player, 5);
		else if (FStrEq(say_string,"TOP6"))	 ShowTop(&player, 6);
		else if (FStrEq(say_string,"TOP7"))	 ShowTop(&player, 7);
		else if (FStrEq(say_string,"TOP8"))	 ShowTop(&player, 8);
		else if (FStrEq(say_string,"TOP9"))	 ShowTop(&player, 9);
		else if (FStrEq(say_string,"TOP10")) ShowTop(&player, 10);
	}
	else if (FStrEq(say_string,"rank") && !war_mode)
	{
		if (mani_stats.GetInt() == 0)
		{
			if (!FStrEq(mani_stats_alternative_rank_message.GetString(),""))
			{
				SayToPlayer(&player, "%s", mani_stats_alternative_rank_message.GetString());
			}

			return;
		}

		ShowRank(&player);
	}
	else if (FStrEq(say_string,"statsme") && !war_mode)
	{
		if (mani_stats.GetInt() == 0) return;
		ShowStatsMe(&player);
	}
	else if ( FStrEq( say_string, "vote" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		engine->ClientCommand(player.entity, "mani_votemenu\n");
		return;
	}

	else if ( FStrEq( say_string, "nominate" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		ProcessRockTheVoteNominateMap(&player);
	}
	else if ( FStrEq( say_string, "rockthevote" ) && !war_mode)
	{
		if (mani_voting.GetInt() == 0) return;
		if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;

		// Nominate allowed up to this point
		ProcessMaRockTheVote(&player);
	}
	else if ( FStrEq( say_string, "favourites" ) && !war_mode)
	{
		// Show web favourites menu
		engine->ClientCommand(player.entity, "favourites\n");
	}
}

//---------------------------------------------------------------------------------
// Purpose: Get a point to a player entity for a given user id
//---------------------------------------------------------------------------------
edict_t *CAdminPlugin::GetEntityForUserID (const int user_id)
{

	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pPlayerEdict = engine->PEntityOfEntIndex(i);
		if(pPlayerEdict && !pPlayerEdict->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pPlayerEdict );
			if (playerinfo && playerinfo->IsConnected())
			{
				int player_user_id = playerinfo->GetUserID();
				if (player_user_id == user_id)
				{
					return (edict_t *) pPlayerEdict;
				}
			}
		}
	}

	return (edict_t *) NULL;

}



//---------------------------------------------------------------------------------
// Purpose: Routine for showing KeyValue contents
//---------------------------------------------------------------------------------

void  CAdminPlugin::PrettyPrinter(KeyValues *keyValue, int indent) {

   // Sanity check!
   if (!keyValue)
      return;

   // This code is just for creating blank strings of size <indent>.
   // It maks our output look nested like the example file.
   // There are probably many better ways of doing this!
   char buff[100];
   for (int i = 0; i < indent; i++)
      buff[i] = ' ';
   buff[indent] = '\0';

   // Remember, not all nodes have a value, so we check that here!
   // Also, see notes below!
   if (Q_strlen(keyValue->GetString()) > 0)
      DirectLogCommand("%skey: %s, value: %s\n", buff,
         keyValue->GetName(),
         keyValue->GetString());
   else
      DirectLogCommand("%skey: %s\n", buff,
         keyValue->GetName(),
         keyValue->GetString());

   // This is the guts of the function.  See below for an explanation!
   KeyValues *kv;
   kv = keyValue->GetFirstSubKey();
   while (kv) {

      // Recursive call!
      PrettyPrinter(kv, indent + 4);

      // Get the next sibling.
      kv = kv->GetNextKey();
   }
}


//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick IP string
//---------------------------------------------------------------------------------

void CAdminPlugin::AddAutoKickIP(char *details)
{
	char	ip_address[128];
	autokick_ip_t	autokick_ip;

	if (!AddToList((void **) &autokick_ip_list, sizeof(autokick_ip_t), &autokick_ip_list_size))
	{
		return;
	}

	// default admin to have absolute power
	autokick_ip.kick = false;
	Q_strcpy(autokick_ip.ip_address, "");
	Q_strcpy(ip_address, "");

	int i = 0;
	int j = 0;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			ip_address[j] = '\0';
			Q_strcpy(autokick_ip.ip_address, ip_address);
			autokick_ip.kick = true;
			autokick_ip_list[autokick_ip_list_size - 1] = autokick_ip;
			Msg("%s Kick\n", ip_address);
			return;
		}

		// If reached space or tab break out of loop
		if (details[i] == ' ' ||
			details[i] == '\t')
		{
			ip_address[j] = '\0';
			break;
		}

		ip_address[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_ip.ip_address, ip_address);

	Msg("%s ", ip_address);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_ip.kick = true; break;
			default : break;
		}

		i++;

		if (autokick_ip.kick)
		{
			break;
		}
	}

	if (autokick_ip.kick)
	{
		Msg("Kick");
	}

	Msg("\n");

	autokick_ip_list[autokick_ip_list_size - 1] = autokick_ip;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Steam ID string
//---------------------------------------------------------------------------------

void CAdminPlugin::AddAutoKickSteamID(char *details)
{
	char	steam_id[128];
	autokick_steam_t	autokick_steam;

	if (!AddToList((void **) &autokick_steam_list, sizeof(autokick_steam_t), &autokick_steam_list_size))
	{
		return;
	}

	// default admin to have absolute power
	autokick_steam.kick = false;
	Q_strcpy(autokick_steam.steam_id, "");
	Q_strcpy(steam_id, "");

	int i = 0;
	int j = 0;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			steam_id[j] = '\0';
			Q_strcpy(autokick_steam.steam_id, steam_id);
			autokick_steam.kick = true;
			autokick_steam_list[autokick_steam_list_size - 1] = autokick_steam;
			Msg("%s Kick\n", steam_id);
			return;
		}

		// If reached space or tab break out of loop
		if (details[i] == ' ' ||
			details[i] == '\t')
		{
			steam_id[j] = '\0';
			break;
		}

		steam_id[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_steam.steam_id, steam_id);

	Msg("%s ", steam_id);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_steam.kick = true; break;
			default : break;
		}

		i++;

		if (autokick_steam.kick)
		{
			break;
		}
	}

	if (autokick_steam.kick)
	{
		Msg("Kick");
	}

	Msg("\n");

	autokick_steam_list[autokick_steam_list_size - 1] = autokick_steam;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Name string
//---------------------------------------------------------------------------------

void CAdminPlugin::AddAutoKickName(char *details)
{
	char	name[128];
	autokick_name_t	autokick_name;

	if (!AddToList((void **) &autokick_name_list, sizeof(autokick_name_t), &autokick_name_list_size))
	{
		return;
	}

	// default admin to have absolute power
	autokick_name.ban = false;
	autokick_name.ban_time = 0;
	autokick_name.kick = false;
	Q_strcpy(autokick_name.name, "");
	Q_strcpy(name, "");

	int i = 0;
	int j = 0;

	// Filter 
	while (details[i] != '\"' && details[i] != '\0')
	{
		i++;
	}

	if (details[i] == '\0')
	{
		return;
	}

	i++;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			name[j] = '\0';
			Q_strcpy(autokick_name.name, name);
			autokick_name.kick = true;
			autokick_name_list[autokick_name_list_size - 1] = autokick_name;
			Msg("%s Kick\n", name);
			return;
		}

		// If reached space or tab break out of loop
		if (details[i] == '\"')
		{
			name[j] = '\0';
			break;
		}

		name[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_name.name, name);

	Msg("%s ", name);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_name.kick = true; break;
			case ('b') : autokick_name.ban = true; break;
			default : break;
		}

		i++;

		if (autokick_name.ban)
		{
			// Need to get the time in minutes
			j = 0;
			char time_string[512];

			while(details[i] != '\0')
			{

				if (details[i] == ' ' ||
					details[i] == '\t')
				{
					i++;
					continue;
				}

				time_string[j++] = details[i++];
				if (j == sizeof(time_string))
				{
					j--;
					break;
				}
			}

			time_string[j] = '\0';
			autokick_name.ban_time = Q_atoi(time_string);
			break;
		}
	}

	if (!autokick_name.ban && !autokick_name.kick)
	{
		autokick_name.kick = true;
	}

	if (autokick_name.ban)
	{
		if (autokick_name.ban_time == 0)
		{
			Msg("Ban permanent");
		}
		else
		{
			Msg("Ban %i minutes", autokick_name.ban_time);
		}
	}
	else
	{
		Msg("Kick");
	}

	Msg("\n");

	autokick_name_list[autokick_name_list_size - 1] = autokick_name;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Name string
//---------------------------------------------------------------------------------

void CAdminPlugin::AddAutoKickPName(char *details)
{
	char	name[128];
	autokick_pname_t	autokick_pname;

	if (!AddToList((void **) &autokick_pname_list, sizeof(autokick_pname_t), &autokick_pname_list_size))
	{
		return;
	}

	// default admin to have absolute power
	autokick_pname.ban = false;
	autokick_pname.ban_time = 0;
	autokick_pname.kick = false;
	Q_strcpy(autokick_pname.pname, "");
	Q_strcpy(name, "");

	int i = 0;
	int j = 0;

	// Filter 
	while (details[i] != '\"' && details[i] != '\0')
	{
		i++;
	}

	if (details[i] == '\0')
	{
		return;
	}

	i++;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			name[j] = '\0';
			Q_strcpy(autokick_pname.pname, name);
			autokick_pname.kick = true;
			autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;
			Msg("%s Kick\n", name);
			return;
		}

		// If reached space or tab break out of loop
		if (details[i] == '\"')
		{
			name[j] = '\0';
			break;
		}

		name[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_pname.pname, name);

	Msg("%s ", name);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_pname.kick = true; break;
			case ('b') : autokick_pname.ban = true; break;
			default : break;
		}

		i++;

		if (autokick_pname.ban)
		{
			// Need to get the time in minutes
			j = 0;
			char time_string[512];

			while(details[i] != '\0')
			{

				if (details[i] == ' ' ||
					details[i] == '\t')
				{
					i++;
					continue;
				}

				time_string[j++] = details[i++];
				if (j == sizeof(time_string))
				{
					j--;
					break;
				}
			}

			time_string[j] = '\0';
			autokick_pname.ban_time = Q_atoi(time_string);
			break;
		}
	}

	if (!autokick_pname.ban && !autokick_pname.kick)
	{
		autokick_pname.kick = true;
	}

	if (autokick_pname.ban)
	{
		if (autokick_pname.ban_time == 0)
		{
			Msg("Ban permanent");
		}
		else
		{
			Msg("Ban %i minutes", autokick_pname.ban_time);
		}
	}
	else
	{
		Msg("Kick");
	}

	Msg("\n");

	autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;
}


//---------------------------------------------------------------------------------
// Purpose: Get the number of players attached to the server.
//---------------------------------------------------------------------------------
int CAdminPlugin::GetNumberOfActivePlayers(void)
{
	int	number_of_players = 0;
	player_t	player;

	for (int i = 1; i <= max_players; i ++)
	{
		player.index = i;
		if (FindPlayerByIndex(&player))
		{
			if (!player.is_bot)
			{
				if (!disconnected_player_list[i - 1].in_use)
				{
					number_of_players ++;
				}
			}
		}
	}

	return number_of_players;
}


//---------------------------------------------------------------------------------
// Purpose: Slay any players that are up for being slayed due to tk/tw
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerHurt(IGameEvent * event)
{
	player_t	victim;
	player_t	attacker;
	int tk_player_index = -1;
	char		weapon_name[128];

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);

	if (attacker.user_id > 0)
	{
		if (!FindPlayerByUserID(&attacker)) return;
	}

	if (!FindPlayerByUserID(&victim)) return;

	gpManiVictimStats->PlayerHurt(&victim, &attacker, event);
	if (!gpManiGameType->IsTeamPlayAllowed()) return;

	// World attacked
	if (attacker.user_id <= 0) return;

	if (!mp_friendlyfire) return;
	if (mp_friendlyfire->GetInt() == 0)	return;

	// Did attack happen to fellow team mate ?
	if (!IsOnSameTeam (&victim, &attacker))	return;

	if (gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		Q_strcpy(weapon_name, event->GetString("weapon","NULL"));
		if (FStrEq("smokegrenade", weapon_name)) return;
		if (FStrEq("flashbang", weapon_name)) return;
		if (FStrEq("zombie_claws_of_death", weapon_name)) return;
	}

	// Is option to show opposite team wound on ?
	if (mani_tk_show_opposite_team_wound.GetInt() == 1)
	{
		bool show_attack = true;
		int attacker_index = attacker.index - 1;

		// Test how soon this happened since the last one
		if (tw_spam_list[attacker_index].last_time + 0.40 > gpGlobals->curtime
			&& tw_spam_list[attacker_index].index == victim.index)
		{
			// This is spam, so set flag.
			show_attack = false;
		}
		else
		{	
			// Show the attack
			tw_spam_list[attacker_index].last_time = gpGlobals->curtime;
			tw_spam_list[attacker_index].index = victim.index;
		}

		// Not spam but update time anyway

		if (show_attack)
		{
			// Show player attack to opposite side 
			char	string_to_show[1024];
			bool	ct = false;
			bool	t = false;
			if (attacker.team == TEAM_A)
			{
				ct = true;
			}
			else if (attacker.team == TEAM_B)
			{
				t = true;
			}

			Q_snprintf (string_to_show, sizeof(string_to_show), "(%s) %s attacked a teammate", 
							Translate(gpManiGameType->GetTeamShortTranslation(attacker.team)), attacker.name);

			// Show to all spectators
			SayToTeam (ct, t, true, "%s", string_to_show);
		}
	}

	// Do out of spawn protection time processing
	if (gpGlobals->curtime > end_spawn_protection_time)
	{
		if (mani_tk_slap_on_team_wound.GetInt() == 1)
		{
			// If set then slap them
			ProcessSlapPlayer(&attacker, mani_tk_slap_on_team_wound_damage.GetInt(), true);
		}

		if (mani_tk_team_wound_reflect.GetInt() == 1)
		{
			// Handles reflection damage
			ProcessReflectDamagePlayer(&victim, &attacker, event);
		}

		return;
	}

	// Exit if tk protection not enabled
	if (mani_tk_protection.GetInt() == 0) return;

	// TK/TW occured (both on same team)
	// Slay them first before anything 

	// Handle tracking of this player
	// Find tkplayer list

	if (attacker.is_bot)
	{
		// Bots don't normally spawn attack
		SlayPlayer(&attacker, false, true, true);
		DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
		SayToAll (false, "Player %s has been slayed for spawn attacking", attacker.name);
		return;
	}

	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), &attacker))
		{
			tk_player_index = i;
			break;
		}
	}

	if (tk_player_index != -1)
	{
		// Player is already on list !!
		if (tk_player_list[tk_player_index].last_round_violation != round_number)
		{
			// If a new round then bump the violations
			if (victim.is_bot)
			{
				if (mani_tk_allow_bots_to_add_violations.GetInt() == 1)
				{
					tk_player_list[tk_player_index].violations_committed++;
				}
			}
			else
			{
				tk_player_list[tk_player_index].violations_committed++;
			}

			Q_strcpy(tk_player_list[tk_player_index].name, attacker.name);
			tk_player_list[tk_player_index].last_round_violation = round_number;
			tk_player_list[tk_player_index].rounds_to_miss += tk_player_list[tk_player_index].spawn_violations_committed;
			tk_player_list[tk_player_index].spawn_violations_committed ++;
			SlayPlayer(&attacker, true, true, true);
			DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
			SayToAll (false, "Player %s has been slayed for spawn attacking", attacker.name);
			// Check if ban required
			TKBanPlayer (&attacker, tk_player_index);
		}
	}
	else
	{
		// Add this player to list
		CreateNewTKPlayer(attacker.name, attacker.steam_id, attacker.user_id, 0, 0);

		// If a new round then bump the violations
		if (victim.is_bot)
		{
			if (mani_tk_allow_bots_to_add_violations.GetInt() == 1)
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 1;
			}
			else
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 0;
			}
		}
		else
		{
			tk_player_list[tk_player_list_size - 1].violations_committed = 1;
		}
			
		tk_player_list[tk_player_list_size - 1].spawn_violations_committed = 1;
		SlayPlayer(&attacker, true, true, true);
		DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team wounding at spawn user [%s] steam id [%s]\n", attacker.name, attacker.steam_id, victim.name, victim.steam_id);
		SayToAll (false, "Player %s has been slayed for spawn attacking", attacker.name);
		// Check if ban required
		TKBanPlayer (&attacker, tk_player_list_size - 1);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle Reflection of for attacker
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessReflectDamagePlayer
(
 player_t *victim, 
 player_t *attacker,
 IGameEvent *event
 )
{
	char	weapon_name[128];
	int		tk_player_index;

	if (!gpManiGameType->IsTeamPlayAllowed()) return;

	Q_strcpy(weapon_name, event->GetString("weapon","NULL"));

	if (FStrEq("smokegrenade", weapon_name)) return;
	if (FStrEq("flashbang", weapon_name)) return;

	// Get player details
	if (!victim->entity)
	{
		if (!FindPlayerByUserID(victim)) return;
	}

	if (!attacker->entity)
	{
		if (!FindPlayerByUserID(attacker)) return;
	}

	// World hurt victim
	if (attacker->user_id == 0) return;

	// Bot attacked so not deliberate
	if (attacker->is_bot) return;

	int damage_done = event->GetInt("dmg_health",0) + event->GetInt("dmg_armour");

	// Find attacker database
	tk_player_index = -1;
	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), attacker))
		{
			tk_player_index = i;
			break;
		}
	}

	if (tk_player_index == -1)
	{
		// Add this player to list
		CreateNewTKPlayer(attacker->name, attacker->steam_id, attacker->user_id, 0, 0);
		tk_player_index = tk_player_list_size - 1;
	}

	tk_player_list[tk_player_index].team_wounds ++;

	if (tk_player_list[tk_player_index].team_wounds <= mani_tk_team_wound_reflect_threshold.GetInt())
	{
		// No need to punish yet, threshold not broken
		return;
	}

	int	damage_to_inflict = (int) ((float) damage_done * tk_player_list[tk_player_index].team_wound_reflect_ratio);
	tk_player_list[tk_player_index].team_wound_reflect_ratio += mani_tk_team_wound_reflect_ratio_increase.GetFloat();

	// Hurt the player
	int sound_index = rand() % 3;

//	CBaseEntity *m_pCBaseEntity = attacker->entity->GetUnknown()->GetBaseEntity(); 

	int health = 0;
//	health = m_pCBaseEntity->GetHealth();
	health = Prop_GetVal(attacker->entity, MANI_PROP_HEALTH, 0);
	if (health <= 0) return;
		
	health -= ((damage_to_inflict >= 0) ? damage_to_inflict : damage_to_inflict * -1);
	if (health <= 0)
	{
		health = 0;
	}

	Prop_SetVal(attacker->entity, MANI_PROP_HEALTH, health);
//	m_pCBaseEntity->SetHealth(health);

	if (health <= 0)
	{
		// Player dead so slay them
		SlayPlayer(attacker, true, true, true);
	}

	if (esounds)
	{
		Vector pos = attacker->entity->GetCollideable()->GetCollisionOrigin();

		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();
		mrf.AddAllPlayers(max_players);
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			esounds->EmitSound((IRecipientFilter &)mrf, attacker->index - 1, CHAN_AUTO, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, &pos);
		}
		else
		{
			esounds->EmitSound((IRecipientFilter &)mrf, attacker->index - 1, CHAN_AUTO, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, &pos);
		}

	}
}

//---------------------------------------------------------------------------------
// Purpose: Slay any players that are up for being slayed due to tk/tw
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerDeath(IGameEvent * event)
{
	player_t	victim;
	player_t	attacker;
	bool	headshot;
	char	weapon_name[128];

	if (war_mode) return;

	victim.entity = NULL;
	attacker.entity = NULL;

	// Were they on the same team ?
	// Find user id and attacker id
	victim.user_id = event->GetInt("userid", -1);
	attacker.user_id = event->GetInt("attacker", -1);
	headshot = event->GetBool("headshot", false);
	Q_strcpy(weapon_name, event->GetString("weapon", ""));

	if (!FindPlayerByUserID(&victim)) return;

	punish_mode_list[victim.index - 1].no_clip = false;

	EffectsPlayerDeath(&victim);
	gpManiGhost->PlayerDeath(&victim);
	gpManiVictimStats->PlayerDeath(&victim, &attacker, headshot, weapon_name);

	if (mani_show_death_beams.GetInt() != 0)
	{
		ProcessDeathBeam(&attacker, &victim);
	}

	if (!FStrEq(weapon_name, "zombie_claws_of_death"))
	{
		// For zombie mod
		ProcessTKDeath(&attacker, &victim);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Processes a player on joining
//---------------------------------------------------------------------------------
bool CAdminPlugin::ProcessAutoKick(player_t *player)
{

	autokick_steam_t	autokick_steam_key;
	autokick_steam_t	*found_steam = NULL;
	autokick_ip_t		autokick_ip_key;
	autokick_ip_t		*found_ip = NULL;

	char	kick_cmd[512];
	char	ban_cmd[512];

	if (war_mode)
	{
		return true;
	}

	Q_strcpy (autokick_steam_key.steam_id, engine->GetPlayerNetworkIDString(player->entity));
	if (FStrEq(autokick_steam_key.steam_id, "BOT"))
	{
		// Is Bot
		return true;
	}

	int immunity_index = -1;
	if (IsImmune(player, &immunity_index))
	{
		if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_TAG])
		{
			return true;
		}
	}

	/* Check steam ID's first */
	if (autokick_steam_list_size != 0)
	{
		found_steam = (autokick_steam_t *) bsearch
						(
						&autokick_steam_key, 
						autokick_steam_list, 
						autokick_steam_list_size, 
						sizeof(autokick_steam_t), 
						sort_autokick_steam
						);

		if (found_steam != NULL)
		{
			// Matched steam id
			if (found_steam->kick)
			{
				player->user_id = engine->GetPlayerUserId(player->entity);
				PrintToClientConsole(player->entity, "You have been autokicked\n");
				Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player->user_id);
				LogCommand (NULL, "Kick (Bad Steam ID) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
				engine->ServerCommand(kick_cmd);				
				return false;
			}
		}
	}

	if (autokick_ip_list_size != 0)
	{
		Q_strcpy(autokick_ip_key.ip_address, player->ip_address);
		found_ip = (autokick_ip_t *) bsearch
						(
						&autokick_ip_key, 
						autokick_ip_list, 
						autokick_ip_list_size, 
						sizeof(autokick_ip_t), 
						sort_autokick_ip
						);

		if (found_ip != NULL)
		{
			// Matched ip address
			if (found_ip->kick)
			{
				player->user_id = engine->GetPlayerUserId(player->entity);
				PrintToClientConsole(player->entity, "You have been autokicked\n");
				Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player->user_id);
				LogCommand (NULL, "Kick (Bad IP Address) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
				engine->ServerCommand(kick_cmd);				
				return false;
			}
		}
	}

	if (player->player_info)
	{
		for (int i = 0; i < autokick_name_list_size; i++)
		{
			if (FStrEq(autokick_name_list[i].name, player->name))
			{
				// Matched name of player
				if (autokick_name_list[i].kick)
				{
					PrintToClientConsole(player->entity, "You have been autokicked\n");
					Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
					engine->ServerCommand(kick_cmd);				
					return false;
				}
				else if (autokick_name_list[i].ban && sv_lan->GetInt() != 1)
				{
					// Ban by user id
					PrintToClientConsole(player->entity, "You have been auto banned\n");
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", autokick_name_list[i].ban_time, player->user_id);
					LogCommand(NULL, "Banned (Bad Name) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);
					engine->ServerCommand("writeid\n");
					return false;
				}
			}
		}

		for (int i = 0; i < autokick_pname_list_size; i++)
		{
			if (NULL != Q_stristr(player->name, autokick_pname_list[i].pname))
			{
				// Matched name of player
				if (autokick_pname_list[i].kick)
				{
					PrintToClientConsole(player->entity, "You have been autokicked\n");
					Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s", player->name, player->steam_id, kick_cmd);
					engine->ServerCommand(kick_cmd);				
					return false;
				}
				else if (autokick_pname_list[i].ban && sv_lan->GetInt() != 1)
				{
					// Ban by user id
					PrintToClientConsole(player->entity, "You have been auto banned\n");
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", autokick_pname_list[i].ban_time, player->user_id);
					LogCommand(NULL, "Banned (Bad Name) [%s] [%s] %s", player->name, player->steam_id, ban_cmd);
					engine->ServerCommand(ban_cmd);
					engine->ServerCommand("writeid\n");
					return false;
				}
			}
		}

	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Processes a player on joining
//---------------------------------------------------------------------------------
bool CAdminPlugin::ProcessReserveSlotJoin(player_t *player)
{

	int			players_on_server = 0;
	bool		is_reserve_player = false;
	player_t	temp_player;
	int		 	players_to_kick = 0;
	int			admin_index;
	int			allowed_players;
	int			total_players;

	if (war_mode)
	{
		return true;
	}

	total_players = GetNumberOfActivePlayers() - 1;
//DirectLogCommand("[DEBUG] Total players on server [%i]\n", total_players);

	if (total_players < max_players - mani_reserve_slots_number_of_slots.GetInt())
	{
//DirectLogCommand("[DEBUG] No reserve slot action required\n");
		return true;
	}


	GetIPAddressFromPlayer(player);
	Q_strcpy (player->steam_id, engine->GetPlayerNetworkIDString(player->entity));
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(player->entity);
	if (playerinfo && playerinfo->IsConnected())
	{
		Q_strcpy(player->name, playerinfo->GetName());
	}
	else
	{
		Q_strcpy(player->name,"");
	}

//DirectLogCommand("[DEBUG] Index = [%i] IP Address [%s] Steam ID [%s]\n",
//									player->index, player->ip_address, player->steam_id);

//DirectLogCommand("[DEBUG] Processing player\n");

	if (FStrEq("BOT", player->steam_id))
	{
//DirectLogCommand("[DEBUG] Player joining is bot, ignoring\n");
		return true;
	}

	player->is_bot = false;


	if (IsPlayerInReserveList(player))
	{
//DirectLogCommand("[DEBUG] Player is on reserve slot list\n");
		is_reserve_player = true;
	}
	else if (mani_reserve_slots_include_admin.GetInt() == 1 && IsClientAdmin(player, &admin_index))
	{
//DirectLogCommand("[DEBUG] Player is admin\n");
		is_reserve_player = true;
	}

//DirectLogCommand("[DEBUG] Building players who can be kicked list\n");

/*	if (active_player_list_size != 0)
	{
		DirectLogCommand("[DEBUG] Players that can be kicked list\n");

		for (int i = 0; i < active_player_list_size; i++)
		{
			DirectLogCommand("[DEBUG] Name [%s] Steam [%s] Spectator [%s] Ping [%f] TimeConnected [%f]\n", 
										active_player_list[i].name,
										active_player_list[i].steam_id,
										(active_player_list[i].is_spectator) ? "YES":"NO",
										active_player_list[i].ping,
										active_player_list[i].time_connected
										);
		}
	}
	else
	{
		DirectLogCommand("[DEBUG] No players available for kicking\n");
	}
*/
	if (mani_reserve_slots_allow_slot_fill.GetInt() == 1)
	{
		BuildPlayerKickList(player, &players_on_server);
		// Allow reserve slots to fill first
		allowed_players = max_players - mani_reserve_slots_number_of_slots.GetInt();
		if (active_player_list_size >= allowed_players)
		{
			// standard players are exceeding slot allocation
			players_to_kick = active_player_list_size - allowed_players;
			for (int i = 0; i < players_to_kick; i++)
			{
				if (i == active_player_list_size)
				{
					break;
				}

				// Disconnect other players that got on (safe guard)
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", active_player_list[i].name);
				temp_player.entity = active_player_list[i].entity;
				DisconnectPlayer(&temp_player);
				disconnected_player_list[active_player_list[i].index - 1].in_use = true;
			}

			if (!is_reserve_player)
			{
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", player->steam_id);
				temp_player.entity = player->entity;
				DisconnectPlayer(&temp_player);
				disconnected_player_list[player->index - 1].in_use = true;
				return false;
			}
		}
	}
	else
	{
		// Keep reserve slots free at all times
		allowed_players = max_players - mani_reserve_slots_number_of_slots.GetInt();
		if (total_players >= allowed_players)
		{
			players_to_kick = total_players - allowed_players;
			if (is_reserve_player)
			{
				players_to_kick ++;
			}
			else
			{
				// Disconnect this player as they are not allowed on and redirect them
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", player->steam_id);
				temp_player.entity = player->entity;
				DisconnectPlayer(&temp_player);
				disconnected_player_list[player->index - 1].in_use = true;
			}

			if (players_to_kick != 0)
			{
				BuildPlayerKickList(player, &players_on_server);
			}

			for (int i = 0; i < players_to_kick; i++)
			{
				if (i == active_player_list_size)
				{
					break;
				}

				// Disconnect other players that got on (safe guard)
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", active_player_list[i].name);
				temp_player.entity = active_player_list[i].entity;
				DisconnectPlayer(&temp_player);
				disconnected_player_list[active_player_list[i].index - 1].in_use = true;
			}

			if (!is_reserve_player)
			{
				return false;
			}
		}
	}

	//DirectLogCommand("[DEBUG] Player [%s] is allowed to join\n", player->steam_id);

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Builds up a list of players that are 'kickable'
//---------------------------------------------------------------------------------
void CAdminPlugin::BuildPlayerKickList(player_t *player, int *players_on_server)
{
	player_t	temp_player;
	active_player_t active_player;
	int			admin_index;

	FreeList((void **) &active_player_list, &active_player_list_size);

	for (int i = 1; i <= max_players; i ++)
	{
		// Ignore players already booted by slot kicker
		if (disconnected_player_list[i - 1].in_use)
		{
			continue;
		}

		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() && pEntity != player->entity)
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				Q_strcpy(active_player.steam_id, playerinfo->GetNetworkIDString());
				if (FStrEq("BOT", active_player.steam_id))
				{
					continue;
				}

				INetChannelInfo *nci = engine->GetPlayerNetInfo(i);
				if (!nci)
				{
					continue;
				}

				active_player.entity = pEntity;

				active_player.ping = nci->GetAvgLatency(0);
				const char * szCmdRate = engine->GetClientConVarValue( i, "cl_cmdrate" );
				int nCmdRate = max( 20, Q_atoi( szCmdRate ) );
				active_player.ping -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

				// in GoldSrc we had a different, not fixed tickrate. so we have to adjust
				// Source pings by half a tick to match the old GoldSrc pings.
				active_player.ping -= TICKS_TO_TIME( 0.5f );
				active_player.ping = active_player.ping * 1000.0f; // as msecs
				active_player.ping = max( 5, active_player.ping ); // set bounds, dont show pings under 5 msecs

				active_player.time_connected = nci->GetTimeConnected();
				Q_strcpy(active_player.ip_address, nci->GetAddress());
				if (gpManiGameType->IsSpectatorAllowed() && playerinfo->GetTeamIndex () == gpManiGameType->GetSpectatorIndex())
				{
					active_player.is_spectator = true;
				}
				else
				{
					active_player.is_spectator = false;
				}
				active_player.user_id = playerinfo->GetUserID();
				Q_strcpy(active_player.name, playerinfo->GetName());

				*players_on_server = *players_on_server + 1;

				Q_strcpy(temp_player.steam_id, active_player.steam_id); 
				Q_strcpy(temp_player.ip_address, active_player.ip_address);
				Q_strcpy(temp_player.name, active_player.name);
				temp_player.is_bot = false;

				if (IsPlayerInReserveList(&temp_player))
				{
					continue;
				}

				active_player.index = i;
				temp_player.index = i;

				if (mani_reserve_slots_include_admin.GetInt() == 1)
				{
					if (IsClientAdmin(&temp_player, &admin_index))
					{
						continue;
					}
				}

				int immunity_index = -1;
				if (IsImmune(&temp_player, &immunity_index))
				{
					if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_RESERVE])
					{
						continue;
					}
				}

				AddToList((void **) &active_player_list, sizeof(active_player_t), &active_player_list_size);
				active_player_list[active_player_list_size - 1] = active_player;
			}
		}
	}

	if (mani_reserve_slots_kick_method.GetInt() == 0)
	{
		// Kick by highest ping
		qsort(active_player_list, active_player_list_size, sizeof(active_player_t), sort_active_players_by_ping);
	}
	else
	{
		// Kick by shortest connection time
		qsort(active_player_list, active_player_list_size, sizeof(active_player_t), sort_active_players_by_connect_time);
	}		
}

//---------------------------------------------------------------------------------
// Purpose: See if player is on reserve list
//---------------------------------------------------------------------------------
bool CAdminPlugin::IsPlayerInReserveList(player_t *player)
{

	reserve_slot_t	reserve_slot_key;

	// Do BSearch for steam ID
	Q_strcpy(reserve_slot_key.steam_id, player->steam_id);

	if (NULL == (reserve_slot_t *) bsearch
						(
						&reserve_slot_key, 
						reserve_slot_list, 
						reserve_slot_list_size, 
						sizeof(reserve_slot_t), 
						sort_reserve_slots_by_steam_id
						))
	{
		return false;
	}
	
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: See if player is on ping immunity list
//---------------------------------------------------------------------------------
bool CAdminPlugin::IsPlayerImmuneFromPingCheck(player_t *player)
{

	ping_immunity_t	ping_immunity_key;

	// Do BSearch for steam ID
	Q_strcpy(ping_immunity_key.steam_id, player->steam_id);

	if (NULL == (ping_immunity_t *) bsearch
						(
						&ping_immunity_key, 
						ping_immunity_list, 
						ping_immunity_list_size, 
						sizeof(ping_immunity_t), 
						sort_ping_immunity_by_steam_id
						))
	{
		return false;
	}
	
	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Check if weapon restricted for this weapon name
//---------------------------------------------------------------------------------
void CAdminPlugin::DisconnectPlayer(player_t *player)
{
	char	disconnect[512];

	if (FStrEq(mani_reserve_slots_redirect.GetString(), ""))
	{
		// No redirection required
		PrintToClientConsole( player->entity, "%s\n", mani_reserve_slots_kick_message.GetString());
		engine->ClientCommand( player->entity, "wait;wait;wait;wait;wait;wait;wait;disconnect\n");
	}
	else
	{
		// Redirection required
		PrintToClientConsole( player->entity, "%s\n", mani_reserve_slots_redirect_message.GetString());
		Q_snprintf(disconnect, sizeof (disconnect), "wait;wait;wait;wait;wait;wait;wait;connect %s\n", mani_reserve_slots_redirect.GetString());
		engine->ClientCommand( player->entity, disconnect);
	}

	return;
}


//---------------------------------------------------------------------------------
// Purpose: Run high ping checker processing
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessHighPingKick(void)
{
	// High ping kicker enabled
	if (next_ping_check > gpGlobals->curtime)
	{
		return;
	}

	next_ping_check = gpGlobals->curtime + 1.5;

	for (int i = 1; i <= max_players; i++)
	{
		// Discount players still connecting or downloading resources
		if (!check_ping_list[i - 1].in_use)
		{
			continue;
		}

		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				const char	*steam_id = playerinfo->GetNetworkIDString();

				if (FStrEq("BOT", steam_id))
				{
					average_ping_list[i - 1].in_use = false;
					continue;
				}

				INetChannelInfo *nci = engine->GetPlayerNetInfo(i);
				const char *ip_address = nci->GetAddress();

				float ping = nci->GetAvgLatency(0);
				const char * szCmdRate = engine->GetClientConVarValue( i, "cl_cmdrate" );
		
				int nCmdRate = max( 20, Q_atoi( szCmdRate ) );
				ping -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

				// in GoldSrc we had a different, not fixed tickrate. so we have to adjust
				// Source pings by half a tick to match the old GoldSrc pings.
				ping -= TICKS_TO_TIME( 0.5f );
				ping = ping * 1000.0f; // as msecs
				ping = clamp( ping, 5, 1000 ); // set bounds, dont show pings under 5 msecs

				if (!average_ping_list[i - 1].in_use)
				{
					// Not tracking player ping at the moment
					average_ping_list[i - 1].in_use = true;
					average_ping_list[i - 1].count = 1;
					average_ping_list[i - 1].ping = ping;
				}
				else
				{
					// Bump up average ping
					average_ping_list[i - 1].count += 1;
					average_ping_list[i - 1].ping += ping;
					if (average_ping_list[i - 1].count > mani_high_ping_kick_samples_required.GetInt())
					{
						if ((average_ping_list[i - 1].ping / average_ping_list[i - 1].count) > mani_high_ping_kick_ping_limit.GetInt())
						{
							player_t player;
							int	admin_index;
								
							Q_strcpy (player.steam_id, steam_id);
							Q_strcpy (player.ip_address, ip_address);
							Q_strcpy (player.name, playerinfo->GetName());

							player.is_bot = false;

							if (!(IsClientAdmin(&player, &admin_index) || 
								IsPlayerImmuneFromPingCheck(&player)))
							{
								player.index = i;
								if (FindPlayerByIndex(&player))
								{
									char kick_cmd[512];

									Q_snprintf( kick_cmd, sizeof(kick_cmd), 
												"kickid %i %s\n", 
												player.user_id,
												mani_high_ping_kick_message.GetString());

									PrintToClientConsole( pEntity, "%s\n", mani_high_ping_kick_message.GetString());
									DirectLogCommand("[MANI_ADMIN_PLUGIN] Kicked player [%s] steam id [%s] for exceeding ping limit\n", player.name, player.steam_id);
									Msg("Kicked player [%s] steam id [%s] for exceeding ping limit\n", player.name, player.steam_id);
									engine->ServerCommand(kick_cmd);
									average_ping_list[i - 1].in_use = false;
									SayToAll (false, "Player %s was autokicked for breaking the %ims ping limit on this server\n", 
																	player.name,
																	mani_high_ping_kick_ping_limit.GetInt()
																	);
								}
								else
								{
									average_ping_list[i - 1].count = 1;
									average_ping_list[i - 1].ping = ping;
								}
							}
							else
							{
								average_ping_list[i - 1].count = 1;
								average_ping_list[i - 1].ping = ping;
							}
						}
						else
						{
							// Reset count and current average
							average_ping_list[i - 1].count = 1;
							average_ping_list[i - 1].ping = ping;
						}
					}
				}

				// Next active player
				continue;
			}
		}
				
		average_ping_list[i - 1].in_use = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Forces certain cheat vars to default values every 1.5 seconds
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessCheatCVars(void)
{
	if (gpGlobals->curtime >= last_cheat_check_time + 10.0)
	{
		last_cheat_check_time = gpGlobals->curtime;
		ProcessCheatCVarCommands();
	}

	return;
}


//---------------------------------------------------------------------------------
// Purpose: Read the stats
//---------------------------------------------------------------------------------
bool CAdminPlugin::HookSayCommand(void)
{

	player_t player;
	const char	*say_string = engine->Cmd_Args();
	char	trimmed_say[2048];
	bool	team_say;
	int say_argc;

	if (engine->IsDedicatedServer() && m_iClientCommandIndex == -1) return true;

	player.index = m_iClientCommandIndex + 1;
	if (!FindPlayerByIndex(&player))
	{
		Msg("Did not find player\n");
		return true;
	}

	if (player.is_bot) return true;
	if (engine->Cmd_Argc() == 1) return true;
	if (Q_strlen(say_string) > 2047) return true;

	// Mute player output
	if (punish_mode_list[player.index - 1].muted && !war_mode)
	{
		return false;
	}

	ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);

/*	Msg("Trimmed Say [%s]\n", trimmed_say);
	Msg("No of args [%i]\n", say_argc);
	for (int i = 0; i < say_argc; i++)
	{
		Msg("Index [%i] [%s]\n", say_argv[i].index, say_argv[i].argv_string);
	}

	Msg("\n\n");
*/
	bool found_swear_word = false;

	if (!war_mode && !punish_mode_list[player.index - 1].gimped)
	{
		if (mani_filter_words_mode.GetInt() != 0)
		{
			// Process string for swear words if player not gimped
			char	upper_say[2048];

			// Copy in uppercase
			Q_strncpy(upper_say, trimmed_say, sizeof(upper_say));
			Q_strupr(upper_say);
	

			for (int i = 0; i < swear_list_size; i ++)
			{
				char	*str_index;
				swear_list[i].found = false;

				Q_strcpy (swear_list[i].filtered, upper_say);
				while (NULL != (str_index = Q_stristr(swear_list[i].filtered, swear_list[i].swear_word)))
				{
					for (int j = 0; j < swear_list[i].length; j++)
					{
						str_index[j] = '*';
					}

					swear_list[i].found = true;
				}
			}

			for (int i = 0; i < swear_list_size; i ++)
			{
				if (swear_list[i].found)
				{
					int str_length = Q_strlen(trimmed_say);
					for (int j = 0; j < str_length; j ++)
					{
						if (swear_list[i].filtered[j] == '*')
						{
							trimmed_say[j] = '*';
						}
					}

					found_swear_word = true;
				}
			}
		}
	}

	char *pcmd = say_argv[0].argv_string;
	char *pcmd1 = say_argv[1].argv_string;
	char *pcmd2 = say_argv[2].argv_string;
	char *pcmd3 = say_argv[3].argv_string;
	char *pcmd4 = say_argv[4].argv_string;
	char *pcmd5 = say_argv[5].argv_string;
	char *pcmd6 = say_argv[6].argv_string;
	char *pcmd7 = say_argv[7].argv_string;
	char *pcmd8 = say_argv[8].argv_string;
	char *pcmd9 = say_argv[9].argv_string;
	char *pcmd10 = say_argv[10].argv_string;
	char *pcmd11 = say_argv[11].argv_string;
	char ncmd[2048];

	team_say = true;
	if (FStrEq(engine->Cmd_Argv(0), "say"))
	{	
		team_say = false;
	}

	if (!CheckForReplacement(&player, pcmd))
	{
		// RCon or client command command executed
		return false;
	}

	// All say commands begin with an @ symbol
	if (pcmd[0] == '@')
	{
		Q_strcpy(ncmd, pcmd);
		if (mani_use_ma_in_say_command.GetInt() == 0)
		{
			if (Q_strlen(pcmd) > 3)
			{
				// If we don't have ma_ in the command
				if (toupper(pcmd[1] != 'M') &&
					toupper(pcmd[2] != 'A') &&
					pcmd[3] != '_')
				{
    				// Add it in
					Q_strcpy(ncmd, "@ma_");
					Q_strcat(ncmd, &(pcmd[1]));
				}
			}
		}

		if (FStrEq(pcmd, "@"))
		{
			if (!team_say) {ProcessMaSay(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
			else {ProcessMaChat(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
		}
		else if (FStrEq(pcmd, "@@")) {ProcessMaPSay(player.index, false, say_argc, pcmd, pcmd1, &(trimmed_say[say_argv[2].index])); return false;}
		else if (FStrEq(pcmd, "@@@")) {ProcessMaCSay(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
		else if (FStrEq(ncmd, "@ma_rcon")) {ProcessMaRCon(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
		else if (FStrEq(ncmd, "@ma_browse")) {ProcessMaBrowse(player.index, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
		else if (FStrEq(ncmd, "@ma_cexec")) {ProcessMaCExec(player.index, false, say_argc, pcmd, pcmd1, &(trimmed_say[say_argv[2].index])); return false;}
		else if (FStrEq(ncmd, "@ma_cexec_all")) {ProcessMaCExecAll(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index])); return false;}
		else if (FStrEq(ncmd, "@ma_cexec_t")) {ProcessMaCExecTeam(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index]), TEAM_A); return false;}
		else if (FStrEq(ncmd, "@ma_cexec_ct")) {ProcessMaCExecTeam(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index]), TEAM_B); return false;}
		else if (FStrEq(ncmd, "@ma_cexec_spec") && gpManiGameType->IsSpectatorAllowed()) {ProcessMaCExecTeam(player.index, false, say_argc, pcmd, &(trimmed_say[say_argv[1].index]), gpManiGameType->GetSpectatorIndex()); return false;}
		else if (FStrEq(ncmd, "@ma_slap")) {ProcessMaSlap (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_setadminflag")) {ProcessMaSetAdminFlag(player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_setskin")) {ProcessMaSetSkin (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_setcash")) {ProcessMaCash (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_SET_CASH); return false;}
		else if (FStrEq(ncmd, "@ma_givecash")) {ProcessMaCash (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_GIVE_CASH); return false;}
		else if (FStrEq(ncmd, "@ma_givecashp")) {ProcessMaCash (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_GIVE_CASH_PERCENT); return false;}
		else if (FStrEq(ncmd, "@ma_takecash")) {ProcessMaCash (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_TAKE_CASH); return false;}
		else if (FStrEq(ncmd, "@ma_takecashp")) {ProcessMaCash (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_TAKE_CASH_PERCENT); return false;}
		else if (FStrEq(ncmd, "@ma_sethealth")) {ProcessMaHealth (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_SET_HEALTH); return false;}
		else if (FStrEq(ncmd, "@ma_givehealth")) {ProcessMaHealth (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_GIVE_HEALTH); return false;}
		else if (FStrEq(ncmd, "@ma_givehealthp")) {ProcessMaHealth (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_GIVE_HEALTH_PERCENT); return false;}
		else if (FStrEq(ncmd, "@ma_takehealth")) {ProcessMaHealth (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_TAKE_HEALTH); return false;}
		else if (FStrEq(ncmd, "@ma_takehealthp")) {ProcessMaHealth (player.index, false, say_argc, pcmd, pcmd1, pcmd2, MANI_TAKE_HEALTH_PERCENT); return false;}
		else if (FStrEq(ncmd, "@ma_blind")) {ProcessMaBlind (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_freeze")) {ProcessMaFreeze (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_noclip")) {ProcessMaNoClip (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_burn")) {ProcessMaBurn (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_colour")) {ProcessMaColour (player.index, false, say_argc, pcmd, pcmd1, pcmd2, pcmd3, pcmd4, pcmd5); return false;}
		else if (FStrEq(ncmd, "@ma_color")) {ProcessMaColour (player.index, false, say_argc, pcmd, pcmd1, pcmd2, pcmd3, pcmd4, pcmd5); return false;}
		else if (FStrEq(ncmd, "@ma_render")) {ProcessMaRenderMode (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_renderfx")) {ProcessMaRenderFX (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_give")) {ProcessMaGive (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_giveammo")) {ProcessMaGiveAmmo (player.index, false, say_argc, pcmd, pcmd1, pcmd2, pcmd3, pcmd4, pcmd5); return false;}
		else if (FStrEq(ncmd, "@ma_drug")) {ProcessMaDrug (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_decal")) {ProcessMaDecal (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_gimp")) {ProcessMaGimp (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_timebomb")) {ProcessMaTimeBomb(player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_beacon")) {ProcessMaBeacon(player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_firebomb")) {ProcessMaFireBomb(player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_freezebomb")) {ProcessMaFreezeBomb(player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_mute")) {ProcessMaMute (player.index, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_teleport")) {ProcessMaTeleport (player.index, false, say_argc, pcmd, pcmd1, pcmd2, pcmd3, pcmd4); return false;}
		else if (FStrEq(ncmd, "@ma_position")) {ProcessMaPosition (player.index, false, say_argc, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_swapteam")) {ProcessMaSwapTeam (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_spec")) {ProcessMaSpec (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_balance")) {ProcessMaBalance (player.index, false, false); return false;}
		else if (FStrEq(ncmd, "@ma_dropc4")) {ProcessMaDropC4 (player.index, false); return false;}
		else if (FStrEq(ncmd, "@ma_saveloc")) {ProcessMaSaveLoc (player.index, false); return false;}
		else if (FStrEq(ncmd, "@ma_resetrank")) {ProcessMaResetPlayerRank (player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_map")) {ProcessMaMap(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_war")) {ProcessMaWar(player.index, false, say_argc, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_setnextmap")) {ProcessMaSetNextMap(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_voterandom")) {ProcessMaVoteRandom(player.index, false, true, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_voteextend")) {ProcessMaVoteExtend(player.index, false, say_argc, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_votercon")) {ProcessMaVoteRCon(player.index, false, true, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_vote")) {ProcessMaVote(player.index, false, true, say_argc, pcmd, pcmd1, pcmd2,
											pcmd3,pcmd4,pcmd5,pcmd6,pcmd7,pcmd8,pcmd9,pcmd10,pcmd11); return false;}
		else if (FStrEq(ncmd, "@ma_votequestion")) {ProcessMaVoteQuestion(player.index, false, true, false, say_argc, pcmd, pcmd1, pcmd2,
											pcmd3,pcmd4,pcmd5,pcmd6,pcmd7,pcmd8,pcmd9,pcmd10,pcmd11); return false;}
		else if (FStrEq(ncmd, "@ma_votecancel")) {ProcessMaVoteCancel(player.index, false, say_argc, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_play")) {ProcessMaPlaySound(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_restrict")) {ProcessMaRestrictWeapon(player.index, false, say_argc, pcmd, pcmd1, pcmd2, true); return false;}
		else if (FStrEq(ncmd, "@ma_knives")) {ProcessMaKnives(player.index, false, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_pistols")) {ProcessMaPistols(player.index, false, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_shotguns")) {ProcessMaShotguns(player.index, false, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_nosnipers")) {ProcessMaNoSnipers(player.index, false, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_unrestrict")) {ProcessMaRestrictWeapon(player.index, false, say_argc, pcmd, pcmd1, "0", false); return false;}
		else if (FStrEq(ncmd, "@ma_unrestrictall")) {ProcessMaUnRestrictAll(player.index, false, say_argc, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_kick" )) {ProcessMaKick(player.index, false, say_argc, pcmd,	pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_spray" )) {gpManiSprayRemove->ProcessMaSpray(player.index, false); return false;}
		else if (FStrEq(ncmd, "@ma_slay" )) {ProcessMaSlay(player.index, false, say_argc, pcmd,	pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_offset" )) {ProcessMaOffset(player.index, false, say_argc, pcmd,	pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_teamindex" )) {ProcessMaTeamIndex(player.index, false, say_argc, pcmd); return false;}
		else if (FStrEq(ncmd, "@ma_offsetscan" )) {ProcessMaOffsetScan(player.index, false, say_argc, pcmd,	pcmd1, pcmd2, pcmd3); return false;}
		else if (FStrEq(ncmd, "@ma_ban" )) {ProcessMaBan(player.index, false, false, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_banip" )) {ProcessMaBan(player.index, false, true, say_argc, pcmd, pcmd1, pcmd2); return false;}
		else if (FStrEq(ncmd, "@ma_unban" )) {ProcessMaUnBan(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_aban_name")) {ProcessMaAutoKickBanName(player.index, false, say_argc, pcmd, pcmd1, pcmd2, false); return false;}
		else if (FStrEq(ncmd, "@ma_aban_pname")) {ProcessMaAutoKickBanPName(player.index, false, say_argc, pcmd, pcmd1, pcmd2, false); return false;}
		else if (FStrEq(ncmd, "@ma_akick_name")) {ProcessMaAutoKickBanName(player.index, false, say_argc, pcmd, pcmd1, 0, true); return false;}
		else if (FStrEq(ncmd, "@ma_akick_pname")) {ProcessMaAutoKickBanPName(player.index, false, say_argc, pcmd, pcmd1, 0, true); return false;}
		else if (FStrEq(ncmd, "@ma_akick_steam")) {ProcessMaAutoKickSteam(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_akick_ip")) {ProcessMaAutoKickIP(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_unaban_name")) {ProcessMaUnAutoKickBanName(player.index, false, say_argc, pcmd, pcmd1, false); return false;}
		else if (FStrEq(ncmd, "@ma_unaban_pname")) {ProcessMaUnAutoKickBanPName(player.index, false, say_argc, pcmd, pcmd1, false); return false;}
		else if (FStrEq(ncmd, "@ma_unakick_name")) {ProcessMaUnAutoKickBanName(player.index, false, say_argc, pcmd, pcmd1, true); return false;}
		else if (FStrEq(ncmd, "@ma_unakick_pname")) {ProcessMaUnAutoKickBanPName(player.index, false, say_argc, pcmd, pcmd1, true); return false;}
		else if (FStrEq(ncmd, "@ma_unakick_steam")) {ProcessMaUnAutoKickSteam(player.index, false, say_argc, pcmd, pcmd1); return false;}
		else if (FStrEq(ncmd, "@ma_unakick_ip")) {ProcessMaUnAutoKickIP(player.index, false, say_argc, pcmd, pcmd1); return false;}
	}


	// Normal say command
	// Is swear word in there ?
	if (found_swear_word)
	{
		if (mani_filter_words_mode.GetInt() == 1)
		{
			SayToPlayer(&player, "%s", mani_filter_words_warning.GetString());
			return false;
		}

		// Mode 2 show filtered string
		char	client_cmd[2048];

		if (!team_say)
		{
			Q_snprintf(client_cmd, sizeof (client_cmd), "say %s\n", trimmed_say);
		}
		else
		{
			Q_snprintf(client_cmd, sizeof (client_cmd), "say_team %s\n", trimmed_say);
		}

		engine->ClientCommand(player.entity, client_cmd);
		return false;
	}

	if (!war_mode && punish_mode_list[player.index - 1].gimped)
	{
		if (gimp_phrase_list_size == 0) return true;

		// player is in gimped mode
		// Check if say string is in gimp word list
		for (int i = 0; i < gimp_phrase_list_size; i++)
		{
			if (FStrEq(gimp_phrase_list[i].phrase, trimmed_say)) return true;
		}

		// If here then player say hasn't been altered yet
		int gimp_phrase = rand() % gimp_phrase_list_size;
		engine->ClientCommand(player.entity, "say %s\n", gimp_phrase_list[gimp_phrase].phrase);
		return false;
	}

	// Check anti spam
	if (!war_mode && mani_chat_flood_time.GetFloat() > 0.1)
	{
		if (chat_flood[m_iClientCommandIndex] > gpGlobals->curtime)
		{
			SayToPlayer(&player, "%s", mani_chat_flood_message.GetString());
			chat_flood[m_iClientCommandIndex] = gpGlobals->curtime + mani_chat_flood_time.GetFloat() + 3.0;
			return false;
		}

		chat_flood[m_iClientCommandIndex] = gpGlobals->curtime + mani_chat_flood_time.GetFloat();
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: When level changed, check if we want to override it
//---------------------------------------------------------------------------------
bool CAdminPlugin::HookChangeLevelCommand(void)
{
	if (!IsCommandIssuedByServerAdmin()) return false;

	if (override_changelevel > 0)
	{
		char	changelevel_command[128];

		Q_snprintf(changelevel_command, sizeof(changelevel_command), "changelevel %s\n", forced_nextmap);
		engine->ServerCommand(changelevel_command);
		override_changelevel = 0;
		override_setnextmap = false;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_kick command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaKick
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	char kick_cmd[256];
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_kick", ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command) 
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}
		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_KICK))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to kick
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			int j = Q_strlen(target_player_list[i].name) - 1;

			while (j != -1)
			{
				if (target_player_list[i].name[j] == '\0') break;
				if (target_player_list[i].name[j] == ' ') break;
				j--;
			}

			j++;

			Q_snprintf( kick_cmd, sizeof(kick_cmd), "bot_kick \"%s\"\n", &(target_player_list[i].name[j]));
			LogCommand (player.entity, "bot_kick [%s]\n", target_player_list[i].name);
			engine->ServerCommand(kick_cmd);
			continue;
		}

		PrintToClientConsole(target_player_list[i].entity, "You have been kicked by Admin\n");
		Q_snprintf( kick_cmd, sizeof(kick_cmd), 
					"kickid %i You were kicked by Admin\n", 
					target_player_list[i].user_id);

		LogCommand (player.entity, "Kick (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].steam_id, kick_cmd);
		engine->ServerCommand(kick_cmd);
		AdminSayToAll(&player, mani_adminkick_anonymous.GetInt(), "kicked player %s", target_player_list[i].name ); 
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unban command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnBan
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_unban", ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <steam id or ip address>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <steam id or ip address>", command_string);
		}

		return PLUGIN_STOP;
	}


	// Try steam id next
	bool ban_by_ip = true;

	char	target_steam_id[2048];
	Q_strcpy(target_steam_id, target_string);
	if (Q_strlen(target_steam_id) > 6)
	{
		target_steam_id[6] = '\0';
		if (FStruEq(target_steam_id, "STEAM_"))
		{
		ban_by_ip = true;		
		}
	}

	char unban_cmd[128];
	if (!ban_by_ip)
	{
		Q_snprintf( unban_cmd, sizeof(unban_cmd), "removeid %s\n", target_string);
	}
	else
	{
		Q_snprintf( unban_cmd, sizeof(unban_cmd), "removeip %s\n", target_string);
	}

	LogCommand (player.entity, "%s", unban_cmd);

	engine->ServerCommand(unban_cmd);
	if (!ban_by_ip)
	{
		engine->ServerCommand("writeid\n");
	}
	else
	{
		engine->ServerCommand("writeip\n");
	}

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Unbanned [%s], no confirmation possible\n", target_string);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Unbanned [%s], no confirmation possible", target_string);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ban command (does both IP and User ID
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBan
(
 int index, 
 bool svr_command, 
 bool ban_by_ip,
 int argc, 
 char *command_string, 
 char *time_to_ban,
 char *target_string
)
{
	player_t player;
	int	admin_index;
	char ban_cmd[256];
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_ban", ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <time in minutes> <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <time in minutes> <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}

	if (sv_lan && sv_lan->GetInt() == 1 && !ban_by_ip)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Cannot ban by ID when on LAN or everyone gets banned !!\n");
		}
		else
		{
			SayToPlayer(&player, "Cannot ban by ID when on LAN or everyone gets banned !!\n");
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_BAN))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to ban
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (atoi(time_to_ban) == 0)
		{
			// Permanent ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by admin permanently !!\n");
		}
		else
		{
			// X minute ban
			PrintToClientConsole(target_player_list[i].entity, "You have been banned by Admin for %s minutes\n", time_to_ban);
		}

		if (!ban_by_ip)
		{
			Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %s %i kick\n", time_to_ban, target_player_list[i].user_id);
		}
		else
		{
			Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %s \"%s\"\n", time_to_ban, target_player_list[i].ip_address);
		}

		if (!ban_by_ip)
		{
			LogCommand (player.entity, "Banned (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].steam_id, ban_cmd);
			if (player.entity)
			{
				LogCommand (NULL, "Banned (By Admin [%s] [%s]) [%s] [%s] %s", 
								player.name, player.steam_id,
								target_player_list[i].name, target_player_list[i].steam_id, ban_cmd);
			}
		}
		else
		{
			LogCommand (player.entity, "Banned IP (By Admin) [%s] [%s] %s", target_player_list[i].name, target_player_list[i].ip_address, ban_cmd);
			if (player.entity)
			{
				LogCommand (NULL, "Banned IP (By Admin [%s] [%s]) [%s] [%s] %s", 
								player.name, player.steam_id,
								target_player_list[i].name, target_player_list[i].ip_address, ban_cmd);
			}
		}

		engine->ServerCommand(ban_cmd);
		if (!ban_by_ip)
		{
			engine->ServerCommand("writeid\n");
		}
		else
		{
			engine->ServerCommand("writeip\n");
		}

		AdminSayToAll(&player, mani_adminban_anonymous.GetInt(), "banned player %s", target_player_list[i].name ); 
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_slay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSlay
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_slay", ALLOW_SLAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_SLAY))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to slay
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		SlayPlayer(&(target_player_list[i]), false, true, true);
		LogCommand (player.entity, "slayed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminslay_anonymous.GetInt(), "slayed player %s", target_player_list[i].name ); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_offset (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaOffset
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (svr_command) return PLUGIN_CONTINUE;

	// Check if player is admin

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, "ma_offset", ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (argc < 2) 
	{
		SayToPlayer(&player, "Mani Admin Plugin: %s <offset number>", command_string);
		return PLUGIN_STOP;
	}

	int offset_number = Q_atoi(target_string);

	// Careful :)
	if (offset_number < 0) offset_number = 0;
	if (offset_number > 2000) offset_number = 2000;

	int *offset_ptr;
	offset_ptr = ((int *)player.entity->GetUnknown() + offset_number);

	LogCommand (player.entity, "Checked offset [%i] which is set to [%i]\n", offset_number, *offset_ptr);
	SayToPlayer(&player, "Offset [%i] = [%i]", offset_number, *offset_ptr);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_teamindex (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTeamIndex
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (svr_command) return PLUGIN_CONTINUE;

	// Check if player is admin

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, "ma_teamindex", ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

#ifdef __linux__
	SayToPlayer(&player, "Linux Server");
#else
	SayToPlayer(&player, "Windows Server");
#endif

	LogCommand (player.entity, "Current index is [%i]\n", player.team);
	SayToPlayer(&player, "Current index is [%i]", player.team);

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_offsetscan (debug) command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaOffsetScan
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *start_range,
 char *end_range
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (svr_command) return PLUGIN_CONTINUE;

	// Check if player is admin

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, "ma_offsetscan", ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (argc < 4) 
	{
		SayToPlayer(&player, "Mani Admin Plugin: %s <value to look for> <start offset> <end offset>", command_string);
		return PLUGIN_STOP;
	}

	int start_offset = Q_atoi(start_range);
	int end_offset = Q_atoi(end_range);

	if (start_offset > end_offset)
	{
		end_offset = Q_atoi(start_range);
		start_offset = Q_atoi(end_range);
	}

#ifdef __linux__
	SayToPlayer(&player, "Linux Server");
#else
	SayToPlayer(&player, "Windows Server");
#endif

	LogCommand (player.entity, "Checking offsets %i to %i\n", start_offset, end_offset);
	SayToPlayer(&player, "Checking offsets %i to %i", start_offset, end_offset);

	// Careful :)
	if (end_offset > 2000) end_offset = 2000;
	if (start_offset < 0) end_offset = 0;

	int	target_value = Q_atoi(target_string);

	bool found_match = false;
	for (int i = start_offset; i <= end_offset; i++)
	{
		int *offset_ptr;
		offset_ptr = ((int *)player.entity->GetUnknown() + i);

		if (*offset_ptr == target_value)
		{
			LogCommand (player.entity, "Offset [%i] = [%i]\n", i, *offset_ptr);
			SayToPlayer(&player, "Offset [%i] = [%i]", i, *offset_ptr);
			found_match = true;
		}
	}

	if (!found_match)
	{
		SayToPlayer(&player, "Did not find any matches");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_slay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSlap
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *damage_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!gpManiGameType->IsSlapAllowed()) return PLUGIN_CONTINUE;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_slap", ALLOW_SLAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <optional damage>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <optional damage>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_SLAP))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	int damage = 0;

	if (argc == 3)
	{
		damage = Q_atoi(damage_string);
		if (damage >= 100)
		{
			damage = 100;
		}
		else if (damage < 0)
		{
			damage = 0;
		}
	}
	else
	{
		damage = 0;
	}

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		ProcessSlapPlayer(&(target_player_list[i]), damage, false);

		if (last_slapped_player != target_player_list[i].index ||
		    last_slapped_time < gpGlobals->curtime - 3)
		{
			LogCommand (player.entity, "slapped user [%s] [%s] with %i damage\n", target_player_list[i].name, target_player_list[i].steam_id, damage);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminslap_anonymous.GetInt(), "slapped player %s with %i damage", target_player_list[i].name, damage ); 
			}
			last_slapped_player = target_player_list[i].index;
			last_slapped_time = gpGlobals->curtime;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setcash command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCash
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *amount,
 int mode /* set cash, take cash, give cash, take cash percent, give cash percent */
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	char	help_string[512];

	if (war_mode) return PLUGIN_CONTINUE;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_CASH, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		switch (mode)
		{
		case (MANI_SET_CASH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <set cash value>", command_string); break;
		case (MANI_GIVE_CASH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <cash to add to player>", command_string); break;
		case (MANI_GIVE_CASH_PERCENT) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <percent to give 0 - 9999999>", command_string); break;
		case (MANI_TAKE_CASH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <cash to remove from player>", command_string); break;
		case (MANI_TAKE_CASH_PERCENT) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <percent to take 0 - 100>", command_string); break;
		default : return PLUGIN_STOP;
		}

		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "%s\n", help_string);
		}
		else
		{
			SayToPlayer(&player, "%s", help_string);
		}

		return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	//int offset = gpManiGameType->GetCashOffset();

	// Convert to float and int
	float fAmount = Q_atof(amount);
	int iAmount = Q_atoi(amount);

	if (fAmount < 0.0) fAmount = 0.0;
	if (iAmount < 0) iAmount = 0;

	fAmount *= 0.01;

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is not on an active team\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is not on an active team", target_player_list[i].name);
			}

			continue;
		}

		int target_cash = Prop_GetVal(target_player_list[i].entity, MANI_PROP_ACCOUNT,0);
		//target_cash = ((int *)target_player_list[i].entity->GetUnknown() + offset);

		int new_cash = 0;

		switch (mode)
		{
			case (MANI_SET_CASH) :	new_cash = iAmount; break;
			case (MANI_GIVE_CASH) :	new_cash = iAmount + target_cash; break;
			case (MANI_GIVE_CASH_PERCENT) :	new_cash = (int) (((float) target_cash) * (fAmount + 1.0)); break;
			case (MANI_TAKE_CASH) :	new_cash = target_cash - iAmount; break;
			case (MANI_TAKE_CASH_PERCENT) :	new_cash = (int) (((float) target_cash) * fAmount); break;
			default : new_cash = target_cash;
		}
			
		if (new_cash < 0) new_cash = 0;
		else if (new_cash > 16000) new_cash = 16000;

		LogCommand (player.entity, "%s : Player [%s] [%s] had [%i] cash, now has [%i] cash\n", command_string, target_player_list[i].name, target_player_list[i].steam_id, target_cash, new_cash);

		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admincash_anonymous.GetInt(), "changed player %s cash reserves", target_player_list[i].name); 
		}

		Prop_SetVal(target_player_list[i].entity, MANI_PROP_ACCOUNT, new_cash);
		//*target_cash = new_cash;
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_sethealth command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaHealth
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *amount,
 int mode /* set cash, take cash, give cash, take cash percent, give cash percent */
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	char	help_string[512];

	if (war_mode) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_HEALTH, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		switch (mode)
		{
		case (MANI_SET_HEALTH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <set health value>", command_string); break;
		case (MANI_GIVE_HEALTH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <health to add to player>", command_string); break;
		case (MANI_GIVE_HEALTH_PERCENT) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <percent to give 0 - 9999999>", command_string); break;
		case (MANI_TAKE_HEALTH) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <health to remove from player>", command_string); break;
		case (MANI_TAKE_HEALTH_PERCENT) :	Q_snprintf(help_string, sizeof(help_string), "Mani Admin Plugin: %s <part of user name, user id or steam id> <percent to take 0 - 100>", command_string); break;
		default : return PLUGIN_STOP;
		}

		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "%s\n", help_string);
		}
		else
		{
			SayToPlayer(&player, "%s", help_string);
		}

		return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Convert to float and int
	float fAmount = Q_atof(amount);
	int iAmount = Q_atoi(amount);

	if (fAmount < 0.0) fAmount = 0.0;
	if (iAmount < 0) iAmount = 0;

	fAmount *= 0.01;

	// Found some players to slap
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is not on an active team\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is not on an active team", target_player_list[i].name);
			}

			continue;
		}

		int target_health;
		target_health = Prop_GetVal(target_player_list[i].entity, MANI_PROP_HEALTH, 0);
//		CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
//		target_health = m_pCBaseEntity->GetHealth();

		int new_health = 0;

		switch (mode)
		{
			case (MANI_SET_HEALTH) :	new_health = iAmount; break;
			case (MANI_GIVE_HEALTH) :	new_health = iAmount + target_health; break;
			case (MANI_GIVE_HEALTH_PERCENT) :	new_health = (int) (((float) target_health) * (fAmount + 1.0)); break;
			case (MANI_TAKE_HEALTH) :	new_health = target_health - iAmount; break;
			case (MANI_TAKE_HEALTH_PERCENT) :	new_health = (int) (((float) target_health) * fAmount); break;
			default : new_health = target_health;
		}
			
		if (new_health < 0) new_health = 0;
		else if (new_health > 999999) new_health = 999999;

		LogCommand (player.entity, "%s : Player [%s] [%s] had [%i] health, now has [%i] health\n", command_string, target_player_list[i].name, target_player_list[i].steam_id, target_health, new_health);

		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminhealth_anonymous.GetInt(), "changed player %s health to %i", target_player_list[i].name, new_health); 
		}

		if (new_health <= 0)
		{
			SlayPlayer(&(target_player_list[i]), false, false, false);
		}
		else
		{
			Prop_SetVal(target_player_list[i].entity, MANI_PROP_HEALTH, new_health);
//			m_pCBaseEntity->SetHealth(new_health);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_blind command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBlind
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *blind_amount_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_blind", ALLOW_BLIND, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <optional blindness amount>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <optional blindness amount>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_BLIND))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	int blind_amount = 0;

	if (argc == 3)
	{
		blind_amount = Q_atoi(blind_amount_string);
		if (blind_amount > 255)
		{
			blind_amount = 255;
		}
		else if (blind_amount < 0)
		{
			blind_amount = 255;
		}
	}
	else
	{
		blind_amount = 255;
	}

	// Found some players to blind
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		BlindPlayer(&(target_player_list[i]), blind_amount);
		LogCommand (player.entity, "%s user [%s] [%s]\n", (blind_amount == 0) ? "unblinded":"blinded", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminblind_anonymous.GetInt(), "%s player %s", (blind_amount == 0) ? "unblinded":"blinded", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_freeze command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFreeze
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_freeze", ALLOW_FREEZE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}
		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_FREEZE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].frozen == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].frozen)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{
			ProcessFreezePlayer(&(target_player_list[i]), true);
			LogCommand (player.entity, "froze user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfreeze_anonymous.GetInt(), "froze player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnFreezePlayer(&(target_player_list[i]));
			LogCommand (player.entity, "defrosted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfreeze_anonymous.GetInt(), "defrosted player %s", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_noclip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaNoClip
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_noclip", ALLOW_NO_CLIP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}
		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to freeze
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		ProcessNoClipPlayer(&(target_player_list[i]));

		if (punish_mode_list[target_player_list[i].index - 1].no_clip)
		{
			LogCommand (player.entity, "noclip user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			AdminSayToAll(&player, mani_adminnoclip_anonymous.GetInt(), "player %s is in no clip mode", target_player_list[i].name); 
		}
		else
		{
			LogCommand (player.entity, "un-noclip user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			AdminSayToAll(&player, mani_adminnoclip_anonymous.GetInt(), "player %s is mortal again", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_burn command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBurn
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsFireAllowed()) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_burn", ALLOW_BURN, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}
		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_BURN))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to burn
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		ProcessBurnPlayer(&(target_player_list[i]), mani_admin_burn_time.GetInt());
		LogCommand (player.entity, "burned user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminburn_anonymous.GetInt(), "burned player %s", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_drug command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDrug
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!gpManiGameType->IsDrugAllowed()) return PLUGIN_CONTINUE;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_DRUG, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_DRUG))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to drug
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (punish_mode_list[target_player_list[i].index - 1].drugged == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].drugged)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessDrugPlayer(&(target_player_list[i]), true);
			LogCommand (player.entity, "drugged user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admindrug_anonymous.GetInt(), "drugged player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnDrugPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-drugged user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admindrug_anonymous.GetInt(), "un-drugged player %s", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_decal command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDecal
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *decal_name
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!gpManiGameType->GetAdvancedEffectsAllowed()) return PLUGIN_STOP;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (svr_command) return PLUGIN_CONTINUE;

	// Check if player is admin
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, command_string, ALLOW_RCON, war_mode, &admin_index)) return PLUGIN_STOP;

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <decal name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <decal name>", command_string);
		}

		return PLUGIN_STOP;
	}

	CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();

	Vector eyepos = CBaseEntity_EyePosition(pPlayer);
	QAngle angles = CBaseEntity_EyeAngles(pPlayer);

	// If from server break out here
	if (svr_command) return PLUGIN_STOP;

	// Get position of entity in eye sight
	Vector vecEnd;
	Vector forward;
	Vector pos;

	AngleVectors( angles, &forward);

	trace_t tr;

	vecEnd = eyepos + (forward * 1024.0f);
	MANI_TraceLineWorldProps  ( eyepos, vecEnd, MASK_SOLID_BRUSHONLY, COLLISION_GROUP_NONE, &tr );
	pos = tr.endpos;

	if (tr.fraction != 1.0)
	{
		SayToPlayer(&player, "Target entity Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	}
	else
	{
		SayToPlayer(&player, "No target entity");
		return PLUGIN_STOP;
	}

	int	   decal_index = gpManiCustomEffects->GetDecal(decal_name);

	MRecipientFilter filter;
	Vector position;

	if (decal_index == -1)
	{
		SayToPlayer(&player, "Invalid Decal Index for [%s]", decal_name);
		return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "\"x\" \"%.5f\"\n", pos.x);
	OutputToConsole(player.entity, svr_command, "\"y\" \"%.5f\"\n", pos.y);
	OutputToConsole(player.entity, svr_command, "\"z\" \"%.5f\"\n", pos.z);

	filter.AddAllPlayers(max_players);
	temp_ents->BSPDecal((IRecipientFilter &) filter, 0, &pos, 0, decal_index);	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_give command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGive
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *item_name
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!sv_cheats) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_GIVE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <item name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <item name>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_GIVE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(target_player_list[i].entity), item_name);

		LogCommand (player.entity, "gave user [%s] [%s] item [%s]\n", target_player_list[i].name, target_player_list[i].steam_id, item_name);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admingive_anonymous.GetInt(), "gave player %s item %s", target_player_list[i].name, item_name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_giveammo command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGiveAmmo
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *weapon_slot_str,
 char *primary_fire_str,
 char *amount_str,
 char *noise_str
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!sv_cheats) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_GIVE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 5) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <weapon slot> <primary fire ammo 0 = alt, 1 = primary> <amount> <optional suppress sound 0 = no, 1 = yes>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <weapon slot> <primary fire ammo 0 = alt, 1 = primary> <amount> <optional suppress sound 0 = no, 1 = yes>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_GIVE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}


	bool	suppress_noise = false;

	if (argc == 6)
	{
		if (FStrEq(noise_str,"1"))
		{
			suppress_noise = true;
		}
	}

	int	weapon_slot = Q_atoi(weapon_slot_str);
	int	amount = Q_atoi(amount_str);
	bool primary_fire = false;

	if (FStrEq(primary_fire_str,"1")) primary_fire = true;
	if (amount < 0) amount = 0; else if (amount > 1000) amount = 1000;
	if (weapon_slot < 0) weapon_slot = 0; else if (weapon_slot > 20) weapon_slot = 20;


	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		CBaseEntity *pPlayer = target_player_list[i].entity->GetUnknown()->GetBaseEntity();
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, weapon_slot);
		if (!pWeapon) continue;

		int	ammo_index;

		if (primary_fire)
		{
			ammo_index = CBaseCombatWeapon_GetPrimaryAmmoType(pWeapon);
		}
		else
		{
			ammo_index = CBaseCombatWeapon_GetSecondaryAmmoType(pWeapon);
		}

		CBaseCombatCharacter_GiveAmmo(pCombat, amount, ammo_index, suppress_noise);

		LogCommand (player.entity, "gave user [%s] [%s] ammo\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admingive_anonymous.GetInt(), "gave player %s ammo", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_colour and ma_color command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaColour
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *red_str,
 char *green_str,
 char *blue_str,
 char *alpha_str
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	int	red, green, blue, alpha;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!sv_cheats) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 6) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <red 0-255> <green 0-255> <blue 0-255> <alpha 0-255>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <green 0-255> <blue 0-255> <alpha 0-255>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	red = Q_atoi(red_str);
	green = Q_atoi(green_str);
	blue = Q_atoi(blue_str);
	alpha = Q_atoi(alpha_str);

	if (red > 255) red = 255; else if (red < 0) red = 0;
	if (green > 255) green = 255; else if (green < 0) green = 0;
	if (blue > 255) blue = 255; else if (blue < 0) blue = 0;
	if (alpha > 255) alpha = 255; else if (alpha < 0) red = 0;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		ProcessSetColour(target_player_list[i].entity, red, green, blue, alpha);
		
		LogCommand (player.entity, "set user color [%s] [%s] to [%i] [%i] [%i] [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, red, blue, green, alpha);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admincolor_anonymous.GetInt(), "set player %s color", target_player_list[i].name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rendermode command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRenderMode
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *render_mode_str
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	int	render_mode;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!sv_cheats) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <render mode 0 onwards>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <render mode 0 onwards>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	render_mode = Q_atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->SetRenderMode((RenderMode_t) render_mode);
		Prop_SetVal(target_player_list[i].entity, MANI_PROP_RENDER_MODE, render_mode);
		LogCommand (player.entity, "set user rendermode [%s] [%s] to [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, render_mode);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admincolor_anonymous.GetInt(), "set player %s to render mode %i", target_player_list[i].name,  render_mode); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_renderfx command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRenderFX
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *render_mode_str
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	int	render_mode;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!sv_cheats) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_COLOUR, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <render fx 0 onwards>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <render fx 0 onwards>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_COLOUR))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	render_mode = Q_atoi(render_mode_str);

	if (render_mode < 0) render_mode = 0;
	if (render_mode > 100) render_mode = 100;

	// Found some players to give items to
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		//CBaseEntity *m_pCBaseEntity = target_player_list[i].entity->GetUnknown()->GetBaseEntity(); 
		//m_pCBaseEntity->m_nRenderFX = render_mode;
		
		Prop_SetVal(target_player_list[i].entity, MANI_PROP_RENDER_FX, render_mode);
		LogCommand (player.entity, "set user renderfx [%s] [%s] to [%i]\n", target_player_list[i].name, target_player_list[i].steam_id, render_mode);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_admincolor_anonymous.GetInt(), "set player %s to renderfx %i", target_player_list[i].name,  render_mode); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_gimp command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaGimp
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_GIMP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_GIMP))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].gimped)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessGimpPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "gimped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admingimp_anonymous.GetInt(), "gimped player %s", target_player_list[i].name); 
				SayToAll(false, "%s", mani_gimp_transform_message.GetString());
			}
		}
		else
		{
			ProcessUnGimpPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-gimped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admingimp_anonymous.GetInt(), "un-gimped player %s", target_player_list[i].name); 
				SayToAll(false, "%s", mani_gimp_untransform_message.GetString());
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_timebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTimeBomb
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_TIMEBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_TIMEBOMB))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].time_bomb == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].time_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnTimeBombPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-timebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admintimebomb_anonymous.GetInt(), "player %s is no longer a time bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessTimeBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player.entity, "timebomb user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_admintimebomb_anonymous.GetInt(), "player %s is now a time bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_firebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFireBomb
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_FIREBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_FIREBOMB))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to fire bomb
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].fire_bomb == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].fire_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnFireBombPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-firebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfirebomb_anonymous.GetInt(), "player %s is no longer a fire bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessFireBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player.entity, "timebomb user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfirebomb_anonymous.GetInt(), "player %s is now a fire bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_freezebomb command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaFreezeBomb
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_FREEZEBOMB, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_FREEZEBOMB))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].freeze_bomb == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].freeze_bomb)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnFreezeBombPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-freezebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfreezebomb_anonymous.GetInt(), "player %s is no longer a freeze bomb", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessFreezeBombPlayer(&(target_player_list[i]), false, true);
			LogCommand (player.entity, "un-freezebombed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminfreezebomb_anonymous.GetInt(), "player %s is now a freeze bomb", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_beacon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBeacon
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_BEACON, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_BEACON))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to turn into beacons
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_dead) continue;

		if (punish_mode_list[target_player_list[i].index - 1].beacon == MANI_TK_ENFORCED)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is under a TK punishment\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].beacon)
			{
				do_action = 1;
			}
		}

		if (!do_action)
		{	
			ProcessUnBeaconPlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-beaconed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminbeacon_anonymous.GetInt(), "player %s is no longer a beacon", target_player_list[i].name); 
			}
		}
		else
		{	
			ProcessBeaconPlayer(&(target_player_list[i]), true);
			LogCommand (player.entity, "beaconed user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminbeacon_anonymous.GetInt(), "player %s is now a beacon", target_player_list[i].name); 
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_mute command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaMute
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *toggle
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_MUTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_MUTE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to gimp
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		int	do_action = 0;

		if (argc == 3)
		{
			do_action = Q_atoi(toggle);
		}
		else
		{
			if (!punish_mode_list[target_player_list[i].index - 1].muted)
			{
				do_action = 1;
			}
		}

		if (do_action)
		{	
			ProcessMutePlayer(&(target_player_list[i]));
			LogCommand (player.entity, "muted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminmute_anonymous.GetInt(), "muted player %s", target_player_list[i].name); 
			}
		}
		else
		{
			ProcessUnMutePlayer(&(target_player_list[i]));
			LogCommand (player.entity, "un-muted user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(&player, mani_adminmute_anonymous.GetInt(), "un-muted player %s", target_player_list[i].name);
			}
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_teleport command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTeleport
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *x_coords,
 char *y_coords,
 char *z_coords
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_CONTINUE;

	// Server can't run this as a saved location
	if (svr_command && argc == 2)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <x> <y> <z> for target location\n", command_string);
		return PLUGIN_CONTINUE;
	}

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc != 2 && argc != 5) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <x> <y> <z> for target location\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> for saved location", command_string);
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <x> <y> <z> for target location", command_string);
		}

		return PLUGIN_STOP;
	}
	
	if (argc == 2 && !CanTeleport(&player))
	{
		// Can't teleport until ma_saveloc is issued first
		SayToPlayer(&player, "You can't teleport a player until you have saved a location using ma_saveloc");
		return PLUGIN_STOP;
	}

	Vector origin;
	Vector *origin2 = NULL;

	if (argc == 5)
	{
		origin.x = Q_atof(x_coords);
		origin.y = Q_atof(y_coords);
		origin.z = Q_atof(z_coords);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_TELEPORT))
	{
		if (!svr_command)
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}
		else
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}

		return PLUGIN_STOP;
	}

	if (argc == 2)
	{
		player_settings_t *player_settings;
	
		player_settings = FindPlayerSettings(&player);
		if (!player_settings) return PLUGIN_STOP;
		if (player_settings->teleport_coords_list_size == 0) return PLUGIN_STOP;

		for (int i = 0; i < player_settings->teleport_coords_list_size; i ++)
		{
			if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
			{
				origin2 = &(player_settings->teleport_coords_list[i].coords);
				break;
			}
		}

		if (!origin2) return PLUGIN_STOP;
		origin = *origin2;
	}

	// Found some players to teleport
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player_ptr;
		
		target_player_ptr = (player_t *) &(target_player_list[i]);

		if (target_player_ptr->is_dead)
		{
			if (!svr_command)
			{
				SayToPlayer(&player, "Player %s is dead, cannot perform command", target_player_ptr->name);
			}
			else
			{
				OutputToConsole(player.entity, svr_command, "Player %s is dead, cannot perform command\n", target_player_ptr->name);
			}

			continue;
		}
	
		ProcessTeleportPlayer(target_player_ptr, &origin);
		// Stack them
		origin.z += 70;

		LogCommand (player.entity, "teleported user [%s] [%s]\n", target_player_ptr->name, target_player_ptr->steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminteleport_anonymous.GetInt(), "teleported player %s", target_player_ptr->name); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_position command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaPosition
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	// Server can't run this 
	if (svr_command) return PLUGIN_CONTINUE;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	// Check if player is admin

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, command_string, ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;

	CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();

	Vector pos = player.player_info->GetAbsOrigin();
	Vector eyepos = CBaseEntity_EyePosition(pPlayer);
	QAngle angles = CBaseEntity_EyeAngles(pPlayer);

	SayToPlayer(&player, "Absolute Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	SayToPlayer(&player, "Eye Position XYZ = %.5f %.5f %.5f", eyepos.x, eyepos.y, eyepos.z);
	SayToPlayer(&player, "Eye Angles XYZ = %.5f %.5f %.5f", angles.x, angles.y, angles.z);

	// If from server break out here
	if (svr_command) return PLUGIN_STOP;

	// Get position of entity in eye sight
	Vector vecEnd;
	Vector forward;

//	pos.z += 24;

	AngleVectors( angles, &forward);

//	forward[2] = 0.0;
//	VectorNormalize( forward );

//	VectorMA( pos, 50.0, forward, pos );
//	VectorMA( pos, 1024.0, forward, vecEnd );

	trace_t tr;

	vecEnd = eyepos + (forward * 1024.0f);
	MANI_TraceLineWorldProps  ( eyepos, vecEnd, MASK_SOLID_BRUSHONLY, COLLISION_GROUP_NONE, &tr );
	pos = tr.endpos;
	if (tr.fraction != 1.0)
	{
		SayToPlayer(&player, "Target entity Position XYZ = %.5f %.5f %.5f", pos.x, pos.y, pos.z);
	}
	else
	{
		SayToPlayer(&player, "No target entity");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSwapTeam
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s This only works on team play games\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s This only works on team play games", command_string);
		}

		return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_SWAP))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is not on a team yet\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is not on a team yet\n", target_player_list[i].name);
			}

			continue;
		}

		// Swap player over
		target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));

		LogCommand (player.entity, "team swapped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								target_player_list[i].name, 
								Translate(gpManiGameType->GetTeamShortTranslation(gpManiGameType->GetOpposingTeam(target_player_list[i].team)))); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_spec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSpec
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsSpectatorAllowed())
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s This only works on games with spectator capability\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s This only works on games with spectator capability", command_string);
		}

		return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id>", command_string);
		}

		return PLUGIN_STOP;
	}
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_SWAP))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is not on a team yet\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is not on a team yet\n", target_player_list[i].name);
			}

			continue;
		}

		// Swap player over
		target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());

		LogCommand (player.entity, "moved the following player to spectator [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(&player, mani_adminswap_anonymous.GetInt(), "moved %s to be a spectator", target_player_list[i].name);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_balance command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBalance
(
 int index, 
 bool svr_command, 
 bool mute_action
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;


	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_balance", ALLOW_SWAP, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		if (!mute_action)
		{
			if (svr_command)
			{
                OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: This only works on team play games\n");
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: This only works on team play games");
			}
		}

		return PLUGIN_STOP;
	}

	if (mani_autobalance_mode.GetInt() == 0)
	{
		// Swap regardless if player is dead or alive
		ProcessMaBalancePlayerType(&player, svr_command, mute_action, true, true);
	}
	else if (mani_autobalance_mode.GetInt() == 1)
	{
		// Swap dead first, followed by Alive players if needed
		if (!ProcessMaBalancePlayerType(&player, svr_command, mute_action, true, false))
		{
			// Requirea check of alive people too
			ProcessMaBalancePlayerType(&player, svr_command, mute_action, false, false);
		}
	}
	else
	{
		// Dead only
		ProcessMaBalancePlayerType(&player, svr_command, mute_action, true, false);
	}	
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_dropc4 command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaDropC4
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_dropc4", ALLOW_DROPC4, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		if (svr_command)
		{
            OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: This only works on CS Source\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: This only works on CS Source");
		}

		return PLUGIN_STOP;
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t	bomb_player;

		bomb_player.index = i;
		if (!FindPlayerByIndex(&bomb_player)) continue;
		if (bomb_player.player_info->IsHLTV()) continue;

		CBaseEntity *pPlayer = bomb_player.entity->GetUnknown()->GetBaseEntity();

		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 4);

		if (pWeapon)
		{
			if (FStrEq("weapon_c4", CBaseCombatWeapon_GetName(pWeapon)))
			{
				CBasePlayer *pBase = (CBasePlayer *) pPlayer;
				CBasePlayer_WeaponDrop(pBase, pWeapon);

				if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
				{
					AdminSayToAll(&player, mani_admindropc4_anonymous.GetInt(), "forced player %s to drop the C4", bomb_player.name); 
				}

				LogCommand (player.entity, "forced c4 drop on player [%s] [%s]\n", bomb_player.name, bomb_player.steam_id);

				break;
			}
		}
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
bool	CAdminPlugin::ProcessMaBalancePlayerType
(
 player_t	*player,
 bool svr_command, 
 bool mute_action,
 bool dead_only,
 bool dont_care
)
{
	bool return_status = true;
	player_t target_player;
	int	t_count = 0;
	int	ct_count = 0;

	for (int i = 1; i <= max_players; i ++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team == TEAM_B) ct_count ++;
		else if (target_player.team == TEAM_A) t_count ++;
	}

	int team_difference = abs(ct_count - t_count);

	if (team_difference <= mp_limitteams->GetInt())
	{
		// No point balancing
		if (!mute_action)
		{
			if (svr_command)
			{
				OutputToConsole(player->entity, svr_command, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings\n");
			}
			else
			{
                SayToPlayer(player, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
			}
		}

		return true;
	}

	int team_to_swap = (ct_count > t_count) ? TEAM_B:TEAM_A;
	int number_to_swap = (team_difference / 2);
	if (number_to_swap == 0)
	{
		// No point balancing
		if (!mute_action)
		{
			if (svr_command)
			{
				OutputToConsole(player->entity, svr_command, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings\n");
			}
			else
			{
				SayToPlayer(player, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
			}
		}
		return true;
	}

	player_t *temp_player_list = NULL;
	int	temp_player_list_size = 0;

	for (int i = 1; i <= max_players; i++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team != team_to_swap) continue;
		if (!dont_care)
		{
			if (target_player.is_dead != dead_only) continue;
		}

		int immunity_index = -1;
		if (IsImmune(&target_player, &immunity_index))
		{
			if (immunity_list[immunity_index].flags[IMMUNITY_ALLOW_BALANCE])
			{
				continue;
			}
		}

		// Player is a candidate
		AddToList((void **) &temp_player_list, sizeof(player_t), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = target_player;
	}

	if (temp_player_list_size == 0) return false;
	if (temp_player_list_size < number_to_swap)
	{
		// Not enough players for this type
		number_to_swap = temp_player_list_size;
		return_status = false;
	}

	int player_to_swap;

	while (number_to_swap != 0)
	{
		for (;;)
		{
			// Get player to swap
			player_to_swap = rand() % temp_player_list_size;
			if (player_to_swap >= temp_player_list_size) continue; // Safety
			if (temp_player_list[player_to_swap].team == team_to_swap) break;
		}

		// Swap player over
		if (mute_action)
		{
			// Do it without slay
//			CBaseEntity *pPlayer = temp_player_list[player_to_swap].entity->GetUnknown()->GetBaseEntity();
//			pPlayer->m_iTeamNum = ((team_to_swap == TEAM_A) ? TEAM_B:TEAM_A);
			temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(team_to_swap));
		}
		else
		{
			temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(team_to_swap));
		}

		temp_player_list[player_to_swap].team = gpManiGameType->GetOpposingTeam(team_to_swap);

		number_to_swap --;

		LogCommand (player->entity, "team balanced user [%s] [%s]\n", temp_player_list[player_to_swap].name, temp_player_list[player_to_swap].steam_id);

		if (!mute_action)
		{
			if (!svr_command || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(player, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								temp_player_list[player_to_swap].name, 
								Translate(gpManiGameType->GetTeamShortTranslation(temp_player_list[player_to_swap].team)));
			}
		}
	}

	FreeList((void **) &temp_player_list, &temp_player_list_size);

	return return_status;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_psay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaPSay
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *say_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_PSAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	// Found some players to talk to
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}
			else
			{
				SayToPlayer(&player, "Player %s is a bot, cannot perform command\n", target_player_list[i].name);
			}

			continue;
		}

		if (!svr_command)
		{
			if (mani_adminsay_anonymous.GetInt() == 0)
			{
				SayToPlayer(target_player, "(ADMIN) %s to (%s) : %s", player.name, target_player->name, say_string);
				SayToPlayer(&player, "(ADMIN) %s to (%s) : %s", player.name, target_player->name, say_string);
			}
			else
			{
				SayToPlayer(target_player, "(ADMIN) to (%s) : %s", target_player->name, say_string);
				SayToPlayer(&player, "(ADMIN) to (%s) : %s", target_player->name, say_string);
			}	

			LogCommand(player.entity, "%s %s (ADMIN) %s to (%s) : %s\n", command_string, target_string, player.name, target_player->name, say_string);
		}
		else
		{
			SayToPlayer(target_player, "%s", say_string);
			LogCommand(NULL, "%s %s (CONSOLE) to (%s) : %s\n", command_string, target_string, target_player->name, say_string);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_msay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaMSay
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *time_display,
 char *target_string,
 char *say_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;
	int	time_to_display;
	msay_t	*lines_list = NULL;
	int		lines_list_size = 0;
	char	temp_line[2048];

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (mani_use_amx_style_menu.GetInt() == 0 || !gpManiGameType->IsAMXMenuAllowed()) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_PSAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 3) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <time to display> <part of user name, user id or steam id> <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <time to display> <part of user name, user id or steam id> <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	time_to_display = Q_atoi(time_display);
	if (time_to_display < 0) time_to_display = 0;
	else if (time_to_display > 100) time_to_display = 100;

	if (time_to_display == 0) time_to_display = -1;

	// Build up lines to display
	Q_strcpy(temp_line,"");
	int	j = 0;
	int	i = 0;

	while (say_string[i] != '\0')
	{
		if (say_string[i] == '\\' && say_string[i + 1] != '\0')
		{
			switch (say_string[i + 1])
			{
			case 'n':
				{
					AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
					temp_line[j] = '\0';
					Q_strcat(temp_line,"\n");
					Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
					Q_strcpy(temp_line,"");
					j = -1;
					i++;
					break;
				}
			case '\\': temp_line[j] = '\\'; i++; break;
			default : temp_line[j] = say_string[i];break;
			}
		}
		else
		{
			temp_line[j] = say_string[i];
		}
		
		i ++;
		j ++;
	}

	temp_line[j] = '\0';
	if (temp_line[0] != '\0')
	{
		AddToList((void **) &lines_list, sizeof(msay_t), &lines_list_size);
		Q_strcpy(lines_list[lines_list_size - 1].line_string, temp_line);
		Msg("Temp Line [%s]\n", temp_line);
	}

	// Found some players to talk to
	for (i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);

		if (target_player_list[i].is_bot) continue;
		if (!svr_command)
		{
			SayToPlayer(target_player, "(CONSOLE) to (%s) : %s", target_player->name, say_string);
			LogCommand(NULL, "%s %s (CONSOLE) to (%s) : %s\n", command_string, target_string, target_player->name, say_string);
		}
		else
		{
		menu_confirm[target_player->index - 1].in_use = false;

		for (j = 0; j < lines_list_size; j ++)
			{
				if (j == lines_list_size - 1)
				{
					DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, true);
				}
				else
				{
					DrawMenu(target_player->index, time_to_display, 10, false, false, false, lines_list[j].line_string, false);
				}
			}
		}
	}

	FreeList((void **) &lines_list, &lines_list_size);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_say command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSay
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player))
		{
			return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		if (!IsClientAdmin(&player, &admin_index))
		{
			if (!war_mode)
			{
				SayToAdmin (&player, "%s", say_string);
			}
			return PLUGIN_STOP;
		}

		if (!admin_list[admin_index].flags[ALLOW_SAY])
		{
			if (!war_mode)
			{
				SayToAdmin (&player, "%s", say_string);
			}
			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(say_string, substitute_text, &col);

	LogCommand (player.entity, "(ALL) %s %s\n", command_string, substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1 && !war_mode)
	{
		ClientMsg(&col, 10, false, 2, "%s", substitute_text);
	}

	if (mani_adminsay_chat_area.GetInt() == 1 || war_mode)
	{
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			AdminSayToAllColoured(&player, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
		}
		else
		{	
			AdminSayToAll(&player, mani_adminsay_anonymous.GetInt(), "%s", substitute_text);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_csay command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCSay
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_SAY, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	LogCommand (player.entity, "%s %s\n", command_string, say_string); 

	if (!svr_command)
	{
		AdminCSayToAll(&player, mani_adminsay_anonymous.GetInt(), "%s", say_string);
	}
	else
	{
		CSayToAll("%s", say_string);
	}
			
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_chat command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaChat
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (war_mode) return PLUGIN_CONTINUE;

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player))
		{
			return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		if (!IsClientAdmin(&player, &admin_index))
		{
			if (!war_mode)
			{
				if (mani_allow_chat_to_admin.GetInt() == 1)
				{
					SayToAdmin (&player, "%s", say_string);
				}
				else
				{
					SayToPlayer (&player, "You are not allowed to chat directly to admin !!");
				}
			}
			return PLUGIN_STOP;
		}

		// Player is Admin
		if (!admin_list[admin_index].flags[ALLOW_CHAT] || war_mode)
		{
			if (!war_mode)
			{
				SayToAdmin (&player, "%s", say_string);
			}
			return PLUGIN_STOP;
		}
	}

	char	substitute_text[512];
	Color	col(255,255,255,255);

	ParseColourStrings(say_string, substitute_text, &col);

	LogCommand (player.entity, "(CHAT) %s %s\n", command_string, substitute_text); 

	if (mani_adminsay_top_left.GetInt() == 1)
	{
		ClientMsg(&col, 15, true, 2, "%s", substitute_text);
	}

	if (mani_adminsay_chat_area.GetInt() == 1)
	{
		AdminSayToAdmin(&player, "%s", substitute_text);
	}
					
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rcon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRCon
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_RCON, false, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <rcon command>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <rcon command>", command_string);
		}

		return PLUGIN_STOP;
	}

	char	rcon_cmd[2048];

	LogCommand (player.entity, "%s %s\n", command_string, say_string); 
	Q_snprintf( rcon_cmd, sizeof(rcon_cmd), "%s\n", say_string);
	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Executed RCON %s\n", rcon_cmd);
	}
	else
	{
		SayToPlayer(&player, "Executed RCON %s", rcon_cmd);
	}

	engine->ServerCommand(rcon_cmd);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_browse command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaBrowse
(
 int index, 
 int argc, 
 char *command_string, 
 char *url_string
)
{
	player_t player;
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	
	if (argc < 2) 
	{
		SayToPlayer(&player, "Mani Admin Plugin: %s <url address>", command_string);
		return PLUGIN_STOP;
	}

	MRecipientFilter mrf;
	mrf.AddPlayer(player.index);
	DrawURL(&mrf, "Browser", url_string);

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExec
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *target_string,
 char *say_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_CEXEC, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <part of user name, user id or steam id> <message>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <part of user name, user id or steam id> <message>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_ALLOW_CEXEC))
	{
		if (svr_command)
		{
            OutputToConsole(player.entity, svr_command, "Did not find player %s\n", target_string);
		}
		else
		{
			SayToPlayer(&player, "Did not find player %s", target_string);
		}

		return PLUGIN_STOP;
	}

	char	client_cmd[2048];

	Q_snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player.entity, "%s \"%s\" %s\n", command_string, target_string, say_string); 

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Ran %s\n", client_cmd);
	}
	else
	{
		SayToPlayer(&player, "Ran %s", client_cmd);
	}
			
			
	// Found some players to run the command on
	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *target_player = &(target_player_list[i]);
		if (target_player->is_bot) continue;
		engine->ClientCommand(target_player->entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_all command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecAll
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_CEXEC, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <client command>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <client command>", command_string);
		}

		return PLUGIN_STOP;
	}

	char	client_cmd[2048];
	player_t	client_player;

	Q_snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player.entity, "%s %s\n", command_string, say_string); 
	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Ran %s\n", client_cmd);
	}
	else
	{
		SayToPlayer(&player, "Ran %s", client_cmd);
	}
			
	for (int i = 1; i <= max_players; i++)
	{
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_cexec_* command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaCExecTeam
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *say_string,
 int team
)
{
	player_t player;
	int	admin_index;

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_CEXEC, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <client command>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <client command>", command_string);
		}

		return PLUGIN_STOP;
	}

	char	client_cmd[2048];
	player_t	client_player;

	Q_snprintf(client_cmd, sizeof (client_cmd), "%s\n", say_string);
	LogCommand (player.entity, "%s %s\n", command_string, say_string); 
	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Ran %s\n", client_cmd);
	}
	else
	{
        SayToPlayer(&player, "Ran %s", client_cmd);
	}

	for (int i = 1; i <= max_players; i++)
	{
		client_player.index = i;
		if (!FindPlayerByIndex(&client_player))	continue;
		if (client_player.is_bot) continue;
		if (client_player.team != team) continue;
		engine->ClientCommand(client_player.entity, client_cmd);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_users command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUsers
(
 int index, 
 bool svr_command,
 int argc, 
 char *command_string, 
 char *target_players
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
	}

	char	target_string[512];

	if (argc < 2) 
	{
		Q_strcpy(target_string, "#ALL");
	}
	else
	{
		Q_strcpy(target_string, target_players);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find any players\n");
		}
		else
		{
			SayToPlayer(&player, "Did not find any players");
		}

		return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command,"Current User List\n\n");
	OutputToConsole(player.entity, svr_command,"Names = number of times a player has changed name\n");
	OutputToConsole(player.entity, svr_command,"A Ghost Name                Steam ID             IP Address       UserID\n");
	OutputToConsole(player.entity, svr_command,"------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		bool	is_admin;
		int		admin_index;

		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			average_ping_list[server_player->index - 1].in_use = false;
			continue;
		}

		if (IsClientAdmin(server_player, &admin_index))
		{
			is_admin = true;
		}
		else
		{
			is_admin = false;
		}

		OutputToConsole(player.entity, svr_command,"%s %s %-19s %-20s %-16s %-7i\n",
						(is_admin) ? "*":" ",
						(gpManiGhost->IsGhosting(server_player)) ? " YES ":"     ",
						server_player->name,
						server_player->steam_id,
						server_player->ip_address,
						server_player->user_id);

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_rates command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaRates
(
 int index, 
 bool svr_command,
 int argc, 
 char *command_string, 
 char *target_players
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_MA_RATES, false, &admin_index)) return PLUGIN_STOP;
	}

	char	target_string[512];

	if (argc < 2) 
	{
		Q_strcpy(target_string, "#ALL");
	}
	else
	{
		Q_strcpy(target_string, target_players);
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, target_string, IMMUNITY_DONT_CARE))
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Did not find any players\n");
		}
		else
		{
			SayToPlayer(&player, "Did not find any players");
		}

		return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command,"Current User List with rates\n\n");
	OutputToConsole(player.entity, svr_command,"Name                Steam ID             IP Address       UserID  rate    cmd    update\n");
	OutputToConsole(player.entity, svr_command,"---------------------------------------------------------------------------------------\n");

	for (int i = 0; i < target_player_list_size; i++)
	{
		player_t *server_player = &(target_player_list[i]);

		if (server_player->is_bot)
		{
			average_ping_list[server_player->index - 1].in_use = false;
			continue;
		}

		const char * szCmdRate = engine->GetClientConVarValue( server_player->index, "cl_cmdrate" );
		const char * szUpdateRate = engine->GetClientConVarValue( server_player->index, "cl_updaterate" );
		const char * szRate = engine->GetClientConVarValue( server_player->index, "rate" );
		int nCmdRate = Q_atoi( szCmdRate );
		int nUpdateRate = Q_atoi( szUpdateRate );
		int nRate = Q_atoi( szRate );

		
		OutputToConsole(player.entity, svr_command,"%s %-17s %-20s %-16s %-7i %-7i %-6i %-6i\n",
					(nCmdRate < 20) ? "*":" ",
					server_player->name,
					server_player->steam_id,
					server_player->ip_address,
					server_player->user_id,
					nRate,
					nCmdRate,
					nUpdateRate);

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_config command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaConfig
(
 int index, 
 bool svr_command,
 int argc,
 char *filter
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_config", ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current Plugin server var settings\n\n");

    ConCommandBase *pPtr = cvar->GetCommands();
    while (pPtr)
	{
		if (!pPtr->IsCommand())
		{
			const char *name = pPtr->GetName();

			if (NULL != Q_stristr(name, "mani_"))
			{
				if (argc == 2)
				{
					if (NULL != Q_stristr(name, filter))
					{
						// Found mani cvar filtered
						ConVar *mani_var = cvar->FindVar(name);
						OutputToConsole(player.entity, svr_command, "%s %s\n", name, mani_var->GetString());
					}
				}
				else
				{
					// Found mani cvar
					ConVar *mani_var = cvar->FindVar(name);
					OutputToConsole(player.entity, svr_command, "%s %s\n", name, mani_var->GetString());
				}
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_help command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaHelp
(
 int index, 
 bool svr_command,
 int argc,
 char	*filter
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_help", ADMIN_DONT_CARE, false, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current Plugin server console commands\n\n");

    ConCommandBase *pPtr = cvar->GetCommands();
    while (pPtr)
	{
		if (pPtr->IsCommand())
		{
			const char *name = pPtr->GetName();

			if (NULL != Q_stristr(name, "ma_"))
			{
				if (argc == 2)
				{
					if (NULL != Q_stristr(name, filter))
					{
						OutputToConsole(player.entity, svr_command, "%s\n", name);
					}
				}
				else
				{
					OutputToConsole(player.entity, svr_command, "%s\n", name);
				}
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}

	OutputToConsole(player.entity, svr_command, "nextmap (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "damage (console)\n");
	OutputToConsole(player.entity, svr_command, "damage (console)\n");
	OutputToConsole(player.entity, svr_command, "listmaps (console only))\n");
	OutputToConsole(player.entity, svr_command, "votemap (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "votekick (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "voteban (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "nominate (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "rockthevote (chat and console)\n");
	OutputToConsole(player.entity, svr_command, "timeleft (chat only)\n");
	OutputToConsole(player.entity, svr_command, "thetime (chat only)\n");
	OutputToConsole(player.entity, svr_command, "ff (chat only)\n");
	OutputToConsole(player.entity, svr_command, "rank (chat only)\n");
	OutputToConsole(player.entity, svr_command, "statsme (chat only)\n");
	OutputToConsole(player.entity, svr_command, "top (chat only)\n");

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_saveloc
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSaveLoc
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!gpManiGameType->IsTeleportAllowed()) return PLUGIN_STOP;

	if (svr_command) return PLUGIN_STOP;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	// Check if player is admin
	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!IsAdminAllowed(&player, "ma_saveloc", ALLOW_TELEPORT, war_mode, &admin_index)) return PLUGIN_STOP;

	ProcessSaveLocation(&player);

	SayToPlayer(&player, "Current location saved, any players will be teleported here");

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanShowName
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_ashowname", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current Names on the autokick/ban list\n\n");
	OutputToConsole(player.entity, svr_command, "Name                           Kick   Ban    Ban Time\n");

	char	name[512];
	char	ban_time[20];

	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (!autokick_name_list[i].ban && !autokick_name_list[i].kick)
		{
			continue;
		}

		Q_strcpy(ban_time,"");
		if (autokick_name_list[i].ban)
		{
			if (autokick_name_list[i].ban_time == 0)
			{
				Q_strcpy (ban_time, "Permanent");
			}
			else
			{
				Q_snprintf(ban_time, sizeof(ban_time), "%i minute%s", 
											autokick_name_list[i].ban_time, 
											(autokick_name_list[i].ban_time == 1) ? "":"s");
			}
		}

		Q_snprintf(name, sizeof(name), "\"%s\"", autokick_name_list[i].name);
		OutputToConsole(player.entity, svr_command, "%-30s %-6s %-6s %s\n", 
					name,
					(autokick_name_list[i].kick) ? "YES":"NO",
					(autokick_name_list[i].ban) ? "YES":"NO",
					ban_time
					);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanShowPName
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_ashowpname", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current partial names on the autokick/ban list\n\n");
	OutputToConsole(player.entity, svr_command, "Partial Name                   Kick   Ban    Ban Time\n");

	char	name[512];
	char	ban_time[20];

	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (!autokick_pname_list[i].ban && !autokick_pname_list[i].kick)
		{
			continue;
		}

		Q_strcpy(ban_time,"");
		if (autokick_pname_list[i].ban)
		{
			if (autokick_pname_list[i].ban_time == 0)
			{
				Q_strcpy (ban_time, "Permanent");
			}
			else
			{
				Q_snprintf(ban_time, sizeof(ban_time), "%i minute%s", 
											autokick_pname_list[i].ban_time, 
											(autokick_pname_list[i].ban_time == 1) ? "":"s");
			}
		}

		Q_snprintf(name, sizeof(name), "\"%s\"", autokick_pname_list[i].pname);
		OutputToConsole(player.entity, svr_command, "%-30s %-6s %-6s %s\n", 
					name,
					(autokick_pname_list[i].kick) ? "YES":"NO",
					(autokick_pname_list[i].ban) ? "YES":"NO",
					ban_time
					);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanShowSteam
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_ashowsteam", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current steam ids on the autokick/ban list\n\n");
	OutputToConsole(player.entity, svr_command, "Steam ID\n");

	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (!autokick_steam_list[i].kick)
		{
			continue;
		}

		OutputToConsole(player.entity, svr_command, "%s\n", autokick_steam_list[i].steam_id);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_ipcommand
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanShowIP
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode) return PLUGIN_STOP;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_ashowip", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	OutputToConsole(player.entity, svr_command, "Current IP addresses on the autokick/ban list\n\n");
	OutputToConsole(player.entity, svr_command, "IP Address\n");

	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (!autokick_ip_list[i].kick)
		{
			continue;
		}

		OutputToConsole(player.entity, svr_command, "%s\n", autokick_ip_list[i].ip_address);
	}

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_war
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaWar
(
 int index, 
 bool svr_command,
 int argc, 
 char *option
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_war", ALLOW_WAR, false, &admin_index)) return PLUGIN_STOP;
	}

	if (argc == 1)
	{
		if (mani_war_mode.GetInt() == 1)
		{
			mani_war_mode.SetValue(0);
			AdminSayToAll(&player, 1, "Disabled War Mode"); 
			LogCommand (player.entity, "Disable war mode\n");
		}
		else
		{
			AdminSayToAll(&player, 1, "Enabled War Mode"); 
			LogCommand (player.entity, "Enable war mode\n");
			mani_war_mode.SetValue(1);
		}	

		return PLUGIN_STOP;
	}

	int	option_val = Q_atoi(option);

	if (option_val == 0)
	{
		// War mode off please
		mani_war_mode.SetValue(0);
		AdminSayToAll(&player, 1, "Disabled War Mode"); 
		LogCommand (player.entity, "Disable war mode\n");
	}
	else if (option_val == 1)
	{
		// War mode on please
		AdminSayToAll(&player, 1, "Enabled War Mode"); 
		LogCommand (player.entity, "Enable war mode\n");
		mani_war_mode.SetValue(1);
	}
	
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_settings command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaSettings
(
 int index
)
{
	player_t player;
	player_settings_t *player_settings;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	
	player_settings = FindPlayerSettings(&player);
	if (!player_settings) return PLUGIN_STOP;

	OutputToConsole(player.entity, false, "Your current settings are\n\n");
	OutputToConsole(player.entity, false,		"Display Damage Stats    (%s)\n", (player_settings->damage_stats) ? "On":"Off");
	if (mani_quake_sounds.GetInt() == 1)
	{
		OutputToConsole(player.entity, false,	"Quake Style Sounds      (%s)\n", (player_settings->quake_sounds) ? "On":"Off");
		OutputToConsole(player.entity, false,	"Server Sounds           (%s)\n", (player_settings->server_sounds) ? "On":"Off");
	}

	if (player_settings->teleport_coords_list_size != 0)
	{
		// Dump maps stored for teleport
		OutputToConsole(player.entity, false, "Current maps you have teleport locations saved on :-\n");
		for (int i = 0; i < player_settings->teleport_coords_list_size; i++)
		{
			OutputToConsole(player.entity, false, "[%s] ", player_settings->teleport_coords_list[i].map_name);
		}

		OutputToConsole(player.entity, false, "\n");
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_admins command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAdmins
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_admins", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	player_t	client_player;
	bool		online;

	OutputToConsole(player.entity, svr_command, "Current Admins setup, a '*' means they are online\n\n");

	for (int i = 0; i < admin_list_size; i++)
	{
		online = false;

		Q_strcpy(client_player.steam_id, admin_list[i].steam_id);
		// Is admin playing ?
		if (FindPlayerBySteamID(&client_player))
		{
			online = true;
		}

		OutputToConsole(player.entity, svr_command, "%s %s ", (online) ? "*":" ", admin_list[i].steam_id);
		OutputToConsole(player.entity, svr_command, "%s ", admin_list[i].ip_address);
		OutputToConsole(player.entity, svr_command, "%s ", admin_list[i].name);
		if (admin_list[i].group_id && !FStrEq(admin_list[i].group_id,""))
		{
			OutputToConsole(player.entity, svr_command, "GROUP [%s] ", admin_list[i].group_id);
		}

		for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
		{
			if (admin_list[i].flags[j])
			{
				OutputToConsole(player.entity, svr_command,  "%s ", admin_flag_list[j].flag_desc);
			}
		}

		OutputToConsole(player.entity, svr_command, "\n");
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_admingroups command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAdminGroups
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_admingroups", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (admin_group_list_size == 0)
	{
		OutputToConsole(player.entity, svr_command, "You have no admin groups setup at the moment\n");
		return PLUGIN_STOP;
	}
	
	OutputToConsole(player.entity, svr_command, "Current Admins Groups\n");

	for (int i = 0; i < admin_group_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "GROUP [%s] ", admin_group_list[i].group_id);
		OutputToConsole(player.entity, svr_command, "\n");

		for (int j = 0; j < MAX_ADMIN_FLAGS; j ++)
		{
			if (admin_group_list[i].flags[j])
			{
				OutputToConsole(player.entity, svr_command,  "%s ", admin_flag_list[j].flag_desc);
			}
		}

		OutputToConsole(player.entity, svr_command, "\n");
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_immunity command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaImmunity
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_immunity", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	player_t	client_player;
	bool		online;

	OutputToConsole(player.entity, svr_command, "Current immunity users setup, a '*' means they are online\n\n");

	for (int i = 0; i < immunity_list_size; i++)
	{
		online = false;

		Q_strcpy(client_player.steam_id, immunity_list[i].steam_id);
		// Is admin playing ?
		if (FindPlayerBySteamID(&client_player))
		{
			online = true;
		}

		OutputToConsole(player.entity, svr_command, "%s %s ", (online) ? "*":" ", immunity_list[i].steam_id);
		OutputToConsole(player.entity, svr_command, "%s ", immunity_list[i].ip_address);
		OutputToConsole(player.entity, svr_command, "%s ", immunity_list[i].name);
		if (immunity_list[i].group_id && !FStrEq(immunity_list[i].group_id,""))
		{
			OutputToConsole(player.entity, svr_command, "GROUP [%s] ", immunity_list[i].group_id);
		}

		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
		{
			if (immunity_list[i].flags[j])
			{
				OutputToConsole(player.entity, svr_command,  "%s ", immunity_flag_list[j].flag_desc);
			}
		}

		OutputToConsole(player.entity, svr_command, "\n");
	}	

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_immunitygroups command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaImmunityGroups
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_STOP;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, "ma_immunitygroups", ADMIN_DONT_CARE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (immunity_group_list_size == 0)
	{
		OutputToConsole(player.entity, svr_command, "You have no immunity groups setup at the moment\n");
		return PLUGIN_STOP;
	}
	
	OutputToConsole(player.entity, svr_command, "Current Immunity Groups\n");

	for (int i = 0; i < immunity_group_list_size; i++)
	{
		OutputToConsole(player.entity, svr_command, "GROUP [%s] ", immunity_group_list[i].group_id);
		OutputToConsole(player.entity, svr_command, "\n");

		for (int j = 0; j < MAX_IMMUNITY_FLAGS; j ++)
		{
			if (immunity_group_list[i].flags[j])
			{
				OutputToConsole(player.entity, svr_command,  "%s ", immunity_flag_list[j].flag_desc);
			}
		}

		OutputToConsole(player.entity, svr_command, "\n");
	}	

	return PLUGIN_STOP;
}



//---------------------------------------------------------------------------------
// Purpose: Process the timeleft command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaTimeLeft
(
 int index, 
 bool svr_command
)
{
	player_t player;
	char	time_string[256]="";
	char	fraglimit_string[256]="";
	char	winlimit_string[256]="";
	char	maxrounds_string[256]="";
	char	final_string[1024];
	int	minutes_portion = 0;
	int 	seconds_portion = 0;
	int	time_left;
	bool	no_timelimit = true;
	bool	follow_string = false;
	bool	last_round = false;

	player.entity = NULL;
	if (war_mode) return PLUGIN_STOP;
	if (!svr_command)
	{
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	}

	if (mp_timelimit)
	{
		if (mp_timelimit->GetInt() != 0)
		{
			time_left = (int) ((mp_timelimit->GetFloat() * 60) - (gpGlobals->curtime - timeleft_offset));
			if (time_left < 0)
			{
				time_left = 0;
			}

			if (time_left != 0)
			{
				minutes_portion = time_left / 60;
				seconds_portion = time_left - (minutes_portion * 60);
			}
		
			if (gpManiGameType->IsGameType(MANI_GAME_CSS) && time_left == 0)
			{
				last_round = true;
			}
			else
			{
				Q_snprintf(time_string, sizeof(time_string), "Timeleft %i:%02i", minutes_portion, seconds_portion, gpGlobals->curtime);	
			}

			follow_string = true;
			no_timelimit = false;
		}
		else 
		{
			Q_snprintf(time_string, sizeof(time_string), "No timelimit for map");
		}
	}

	if (mp_fraglimit && mp_fraglimit->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(fraglimit_string, sizeof(fraglimit_string), ", or change map after player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
		}
		else
		{
			Q_snprintf(fraglimit_string, sizeof(fraglimit_string), "Map will change after a player reaches %i frag%s", mp_fraglimit->GetInt(), (mp_fraglimit->GetInt() == 1) ? "":"s");
			follow_string = true;
		}

	}

	if (mp_winlimit && mp_winlimit->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(winlimit_string, sizeof(winlimit_string), ", or change map after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
		}
		else
		{
			Q_snprintf(winlimit_string, sizeof(winlimit_string), "Map will change after a team wins %i round%s", mp_winlimit->GetInt(), (mp_winlimit->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (mp_maxrounds && mp_maxrounds->GetInt() != 0)
	{
		if (follow_string)
		{
			Q_snprintf(maxrounds_string, sizeof(maxrounds_string), ", or change map after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
		}
		else
		{
			Q_snprintf(maxrounds_string, sizeof(maxrounds_string), "Map will change after %i round%s", mp_maxrounds->GetInt(), (mp_maxrounds->GetInt() == 1)? "":"s");
			follow_string = true;
		}
	}

	if (!last_round)
	{
		if (no_timelimit && follow_string)
		{
			Q_snprintf(final_string, sizeof(final_string), "%s%s%s", fraglimit_string, winlimit_string, maxrounds_string);
		}
		else
		{
			Q_snprintf(final_string, sizeof(final_string), "%s%s%s%s", time_string, fraglimit_string, winlimit_string, maxrounds_string);
		}
	}
	else
	{
		Q_snprintf(final_string, sizeof(final_string), "This is the last round !!");
	}

	if (!svr_command)
	{		
		if (mani_timeleft_player_only.GetInt() == 1)
		{
			SayToPlayer(&player,"%s", final_string);
		}
		else
		{
			SayToAll(false,"%s", final_string);
		}	
	}
	else
	{
		OutputToConsole(NULL, svr_command, "%s\n", final_string);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autoban_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanName
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_name,
 char *ban_time_string,
 bool kick
)
{
	player_t player;
	int	admin_index;
	autokick_name_t	autokick_name;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

		if (kick)
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
		}
		else
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (kick)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player name>\n", command_string);
			}
			else
			{
                SayToPlayer(&player, "Mani Admin Plugin: %s <player name>", command_string);
			}
		}
		else
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player name> <optional time in minutes>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <player name> <optional time in minutes>", command_string);
			}
		}
		return PLUGIN_STOP;
	}

	int ban_time = 0;

	if (argc == 3)
	{
		ban_time = Q_atoi(ban_time_string);
		if (ban_time < 0)
		{
			ban_time = 0;
		}
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(player_name, autokick_name_list[i].name))
		{
			if (kick)
			{
				autokick_name_list[i].ban = false;
				autokick_name_list[i].ban_time = 0;
				autokick_name_list[i].kick = true;
			}
			else
			{
				autokick_name_list[i].ban = true;
				autokick_name_list[i].ban_time = ban_time;
				autokick_name_list[i].kick = false;
			}

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Player [%s] updated\n", player_name);
				LogCommand (NULL, "Updated player [%s] to autokick_name.txt\n", player_name);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Player [%s] updated", player_name);
				LogCommand (player.entity, "Updated player [%s] to autokick_name.txt\n", player_name);
			}

			WriteNameList("autokick_name.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_name.name, player_name);
	if (kick)
	{
		autokick_name.ban = false;
		autokick_name.ban_time = 0;
		autokick_name.kick = true;
	}
	else
	{
		autokick_name.ban = true;
		autokick_name.ban_time = ban_time;
		autokick_name.kick = false;
	}

	AddToList((void **) &autokick_name_list, sizeof(autokick_name_t), &autokick_name_list_size);
	autokick_name_list[autokick_name_list_size - 1] = autokick_name;

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Player [%s] added\n", player_name);
		LogCommand (NULL, "Added player [%s] to autokick_name.txt\n", player_name);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Player [%s] added", player_name);
		LogCommand (player.entity, "Added player [%s] to autokick_name.txt\n", player_name);
	}

	WriteNameList("autokick_name.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autoban_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickBanPName
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_pname,
 char *ban_time_string,
 bool kick
)
{
	player_t player;
	int	admin_index;
	autokick_pname_t	autokick_pname;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

		if (kick)
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
		}
		else
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (kick)
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <partial name>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <partial name>", command_string);
			}
		}
		else
		{
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <partial name> <optional time in minutes>\n", command_string);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: %s <partial name> <optional time in minutes>", command_string);
			}
		}

		return PLUGIN_STOP;
	}


	int ban_time = 0;

	if (argc == 3)
	{
		ban_time = Q_atoi(ban_time_string);
		if (ban_time < 0)
		{
			ban_time = 0;
		}
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (FStrEq(player_pname, autokick_pname_list[i].pname))
		{
			if (kick)
			{
				autokick_pname_list[i].ban = false;
				autokick_pname_list[i].ban_time = 0;
				autokick_pname_list[i].kick = true;
			}
			else
			{
				autokick_pname_list[i].ban = true;
				autokick_pname_list[i].ban_time = ban_time;
				autokick_pname_list[i].kick = false;
			}
			
			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Partial name [%s] updated\n", player_pname);
				LogCommand (NULL, "Updated name [%s] to autokick_pname.txt\n", player_pname);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Partial name [%s] updated", player_pname);
				LogCommand (player.entity, "Updated player [%s] to autokick_pname.txt\n", player_pname);
			}

			WritePNameList("autokick_pname.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_pname.pname, player_pname);
	if (kick)
	{
		autokick_pname.ban = false;
		autokick_pname.ban_time = 0;
		autokick_pname.kick = true;
	}
	else
	{
		autokick_pname.ban = true;
		autokick_pname.ban_time = ban_time;
		autokick_pname.kick = false;
	}


	AddToList((void **) &autokick_pname_list, sizeof(autokick_pname_t), &autokick_pname_list_size);
	autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;


	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Partial name [%s] added\n", player_pname);
		LogCommand (NULL, "Added player [%s] to autokick_pname.txt\n", player_pname);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Partial name [%s] added", player_pname);
		LogCommand (player.entity, "Added player [%s] to autokick_pname.txt\n", player_pname);
	}

	WritePNameList("autokick_pname.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickSteam
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_steam_id
)
{
	player_t player;
	int	admin_index;
	autokick_steam_t	autokick_steam;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <steam id>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if steam id is already in list
	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (FStrEq(player_steam_id, autokick_steam_list[i].steam_id))
		{
			autokick_steam_list[i].kick = true;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Steam ID [%s] updated\n", player_steam_id);
				LogCommand (NULL, "Updated steam [%s] to autokick_steam.txt\n", player_steam_id);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Steam ID [%s] updated", player_steam_id);
				LogCommand (player.entity, "Updated steam [%s] to autokick_steam.txt\n", player_steam_id);
			}

			WriteSteamList("autokick_steam.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_steam.steam_id, player_steam_id);
	autokick_steam.kick = true;

	AddToList((void **) &autokick_steam_list, sizeof(autokick_steam_t), &autokick_steam_list_size);
	autokick_steam_list[autokick_steam_list_size - 1] = autokick_steam;

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Steam ID [%s] added\n", player_steam_id);
		LogCommand (NULL, "Added steam id [%s] to autokick_steam.txt\n", player_steam_id);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Steam ID [%s] added", player_steam_id);
		LogCommand (player.entity, "Added steam id [%s] to autokick_steam.txt\n", player_steam_id);
	}

	qsort(autokick_steam_list, autokick_steam_list_size, sizeof(autokick_steam_t), sort_autokick_steam); 
	WriteSteamList("autokick_steam.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_ip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaAutoKickIP
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_ip_address
)
{
	player_t player;
	int	admin_index;
	autokick_ip_t	autokick_ip;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <ip address>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <ip address>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if ip is already in list
	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (FStrEq(player_ip_address, autokick_ip_list[i].ip_address))
		{
			autokick_ip_list[i].kick = true;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: IP address [%s] updated\n", player_ip_address);
				LogCommand (NULL, "Updated ip address [%s] to autokick_ip.txt\n", player_ip_address);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: IP address [%s] updated", player_ip_address);
				LogCommand (player.entity, "Updated ip address [%s] to autokick_ip.txt\n", player_ip_address);
			}

			WriteIPList("autokick_ip.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_ip.ip_address, player_ip_address);
	autokick_ip.kick = true;

	AddToList((void **) &autokick_ip_list, sizeof(autokick_ip_t), &autokick_ip_list_size);
	autokick_ip_list[autokick_ip_list_size - 1] = autokick_ip;

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: IP address [%s] added\n", player_ip_address);
		LogCommand (NULL, "Added ip address [%s] to autokick_ip.txt\n", player_ip_address);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: IP address [%s] added", player_ip_address);
		LogCommand (player.entity, "Added ip address [%s] to autokick_ip.txt\n", player_ip_address);
	}

	qsort(autokick_ip_list, autokick_ip_list_size, sizeof(autokick_ip_t), sort_autokick_ip); 

	WriteIPList("autokick_ip.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unautoban_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnAutoKickBanName
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_name,
 bool kick
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

		if (kick)
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
		}
		else
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
		}
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <player name>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(player_name, autokick_name_list[i].name))
		{
			autokick_name_list[i].ban = false;
			autokick_name_list[i].ban_time = 0;
			autokick_name_list[i].kick = false;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Player [%s] updated\n", player_name);
				LogCommand (NULL, "Updated player [%s] to autokick_name.txt\n", player_name);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Player [%s] updated", player_name);
				LogCommand (player.entity, "Updated player [%s] to autokick_name.txt\n", player_name);
			}

			WriteNameList("autokick_name.txt");
			return PLUGIN_STOP;
		}
	}

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Player [%s] not found\n", player_name);
		LogCommand (NULL, "Player [%s] not found\n", player_name);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Player [%s] not found", player_name);
		LogCommand (player.entity, "Player [%s] not found\n", player_name);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unautoban_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnAutoKickBanPName
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_pname,
 bool kick
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;

		if (kick)
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
		}
		else
		{
			if (!IsAdminAllowed(&player, command_string, ALLOW_BAN, war_mode, &admin_index)) return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <player partial name>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <player partial name>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (FStrEq(player_pname, autokick_pname_list[i].pname))
		{
			autokick_pname_list[i].ban = false;
			autokick_pname_list[i].ban_time = 0;
			autokick_pname_list[i].kick = false;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Partial name [%s] updated\n", player_pname);
				LogCommand (NULL, "Updated partial name [%s] to autokick_name.txt\n", player_pname);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Partial name [%s] updated", player_pname);
				LogCommand (player.entity, "Updated partial name [%s] to autokick_name.txt\n", player_pname);
			}

			WritePNameList("autokick_pname.txt");
			return PLUGIN_STOP;
		}
	}

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Partial name [%s] not found\n", player_pname);
		LogCommand (NULL, "Partial name [%s] not found\n", player_pname);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Partial name [%s] not found", player_pname);
		LogCommand (player.entity, "Partial name [%s] not found\n", player_pname);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unautoban_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnAutoKickSteam
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_steam_id
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <steam id>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <steam id", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if steam id is already in list
	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (FStrEq(player_steam_id, autokick_steam_list[i].steam_id))
		{
			autokick_steam_list[i].kick = false;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Steam ID [%s] updated\n", player_steam_id);
				LogCommand (NULL, "Updated steam id [%s] to autokick_steam.txt\n", player_steam_id);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: Steam ID [%s] updated", player_steam_id);
				LogCommand (player.entity, "Updated steam id [%s] to autokick_steam.txt\n", player_steam_id);
			}

			WriteSteamList("autokick_steam.txt");
			return PLUGIN_STOP;
		}
	}

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Steam ID [%s] not found\n", player_steam_id);
		LogCommand (NULL, "Steam ID [%s] not found\n", player_steam_id);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: Steam ID [%s] not found", player_steam_id);
		LogCommand (player.entity, "Steam ID [%s] not found\n", player_steam_id);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unautoban_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaUnAutoKickIP
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string, 
 char *player_ip_address
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_KICK, war_mode, &admin_index)) return PLUGIN_STOP;
	}
	
	if (argc < 2) 
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <ip address>\n", command_string);
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: %s <ip address>", command_string);
		}

		return PLUGIN_STOP;
	}

	// Check if ip is already in list
	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (FStrEq(player_ip_address, autokick_ip_list[i].ip_address))
		{
			autokick_ip_list[i].kick = false;

			if (svr_command)
			{
				OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: ip address [%s] updated\n", player_ip_address);
				LogCommand (NULL, "Updated ip address [%s] to autokick_ip.txt\n", player_ip_address);
			}
			else
			{
				SayToPlayer(&player, "Mani Admin Plugin: ip address [%s] updated", player_ip_address);
				LogCommand (player.entity, "Updated ip address [%s] to autokick_ip.txt\n", player_ip_address);
			}

			WriteIPList("autokick_ip.txt");
			return PLUGIN_STOP;
		}
	}

	if (svr_command)
	{
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: IP address [%s] not found\n", player_ip_address);
		LogCommand (NULL, "IP address [%s] not found\n", player_ip_address);
	}
	else
	{
		SayToPlayer(&player, "Mani Admin Plugin: IP address [%s] not found", player_ip_address);
		LogCommand (player.entity, "IP address [%s] not found\n", player_ip_address);
	}

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Write name list to file
//---------------------------------------------------------------------------------
void	CAdminPlugin::WriteNameList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	if (filesystem->FileExists( base_filename))
	{
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete %s\n", filename_string);
		}
	}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		// Write file in human readable text format
		for (int i = 0; i < autokick_name_list_size; i ++)
		{
			char ban_string[128];

			if (!autokick_name_list[i].ban && !autokick_name_list[i].kick) continue;
			Q_snprintf(ban_string , sizeof(ban_string), "b %i\n", autokick_name_list[i].ban_time);

			char	temp_string[512];
			int		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "\"%s\" %s", autokick_name_list[i].name, (autokick_name_list[i].kick)? "k\n":ban_string);											

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				Msg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write pname list to file
//---------------------------------------------------------------------------------
void	CAdminPlugin::WritePNameList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	if (filesystem->FileExists( base_filename))
	{
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete %s\n", filename_string);
		}
	}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		// Write file in human readable text format
		for (int i = 0; i < autokick_pname_list_size; i ++)
		{
			char ban_string[128];
			if (!autokick_pname_list[i].ban && !autokick_pname_list[i].kick) continue;

			Q_snprintf(ban_string , sizeof(ban_string), "b %i\n", autokick_pname_list[i].ban_time);

			char	temp_string[512];
			int		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "\"%s\" %s", autokick_pname_list[i].pname, (autokick_pname_list[i].kick)? "k\n": ban_string);											

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				Msg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write ip list to file
//---------------------------------------------------------------------------------
void	CAdminPlugin::WriteIPList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	if (filesystem->FileExists( base_filename))
	{
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete %s\n", filename_string);
		}
	}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		// Write file in human readable text format
		for (int i = 0; i < autokick_ip_list_size; i ++)
		{
			if (!autokick_ip_list[i].kick) continue;

			char	temp_string[512];
			int		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "%s k\n", autokick_ip_list[i].ip_address);

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)											
			{
				Msg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write steam list to file
//---------------------------------------------------------------------------------
void	CAdminPlugin::WriteSteamList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	if (filesystem->FileExists( base_filename))
	{
		filesystem->RemoveFile(base_filename);
		if (filesystem->FileExists( base_filename))
		{
			Msg("Failed to delete %s\n", filename_string);
		}
	}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		// Write file in human readable text format
		for (int i = 0; i < autokick_steam_list_size; i ++)
		{
			if (!autokick_steam_list[i].kick) continue;

			char	temp_string[512];
			int		temp_length = Q_snprintf(temp_string, sizeof(temp_string), "%s k\n", autokick_steam_list[i].steam_id);

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)											
			{
				Msg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}



//---------------------------------------------------------------------------------
// Purpose: Process the ma_votecancel command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVoteCancel
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_CANCEL_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: No system voting to cancel !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: No system voting to cancel !!");
		}

		return PLUGIN_STOP;
	}

	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "cancelled current vote"); 
	system_vote.vote_in_progress = false;
	if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
	{
		system_vote.map_decided = true;
	}

	for (int i = 0; i < max_players; i++)
	{
		voter_list[i].allowed_to_vote = false;
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_voterandom command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVoteRandom
(
 int index, 
 bool svr_command, 
 bool say_command,
 int argc, 
 char *command_string, 
 char *delay_type_string,
 char *number_of_maps_string
)
{
	player_t player;
	int	admin_index = -1;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_RANDOM_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (say_command) SayToPlayer(&player, "Mani Admin Plugin: %s <delay type \"now\", \"end\"%s> <number of maps>", command_string, (!gpManiGameType->IsGameType(MANI_GAME_CSS)) ? "":", \"round\"");
		if (say_command) SayToPlayer(&player, "See console version of command for examples on how to use %s", command_string);
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <delay type \"now\", \"end\"%s> <number of maps>\n", command_string, (!gpManiGameType->IsGameType(MANI_GAME_CSS)) ? "":", \"round\"");
		OutputToConsole(player.entity, svr_command, "For example, <%s now 5> will run a vote for 5 random maps that when completed will change map in 5 seconds\n", command_string);
		OutputToConsole(player.entity, svr_command, "<%s end 10> will run a vote for 10 random maps that when completed will change map at the end of the game\n", command_string);
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))	OutputToConsole(player.entity, svr_command, "<%s round 7> will run a vote for 7 random maps that when completed will change map at the end of the round\n", command_string);

		return PLUGIN_STOP;
	}


	if (system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Cannot run as system vote is already in progress !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		}

		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	int	number_of_maps;

	if (argc < 3)
	{
		// Only number of maps passed through
		number_of_maps = Q_atoi(delay_type_string);
	}
	else
	{
		// Delay type and number of maps passed through
		if (FStrEq(delay_type_string,"end"))
		{
			delay_type = VOTE_END_OF_MAP_DELAY;
		}
		else if (FStrEq(delay_type_string,"now"))
		{
			delay_type = VOTE_NO_DELAY;
		}
		else if (FStrEq(delay_type_string,"round") && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			delay_type = VOTE_END_OF_ROUND_DELAY;
		}

		number_of_maps = Q_atoi(number_of_maps_string);
	}

	if (number_of_maps == 0)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: You must have one or more maps !!");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: You must have one or more maps !!");
		}

		return PLUGIN_STOP;
	}

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_RANDOM_MAP;
	if (svr_command)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player.index;
	}

	system_vote.vote_confirmation = false;
	if (!svr_command && admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	BuildRandomMapVote(number_of_maps);
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title,Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
	}

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(player.entity, "Started a random map vote\n");
	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "started a random map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_vote command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVote
(
 int index, 
 bool svr_command, 
 bool say_command,
 int argc, 
 char *command_string, 
 char *delay_type_string,
 char *map1, char *map2, char *map3, char *map4, char *map5, char *map6, char *map7, char *map8, char *map9, char *map10
)
{
	player_t player;
	int	admin_index = -1;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (argc < 2) 
	{
		if (say_command) SayToPlayer(&player, "Mani Admin Plugin: %s <delay type \"now\", \"end\"%s> <number of maps>", command_string, (!gpManiGameType->IsGameType(MANI_GAME_CSS)) ? "":", \"round\"");
		if (say_command) SayToPlayer(&player, "See console version of command for examples on how to use %s", command_string);

		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <delay type \"now\", \"end\"%s> <map names>\n", command_string, (!gpManiGameType->IsGameType(MANI_GAME_CSS)) ? "":", \"round\"");
		OutputToConsole(player.entity, svr_command, "For example, <%s now de_dust> will run a Yes/No vote for de_dust completed will change map in 5 seconds\n", command_string);
		OutputToConsole(player.entity, svr_command, "<%s end de_dust de_dust2> will run a vote for de_dust and de_dust2 that when completed will change map at the end of the game\n", command_string);
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))	OutputToConsole(player.entity, svr_command, "<%s round de_aztec de_cbble> will run a vote for de_aztec and de_cbble that when completed will change map at the end of the round\n", command_string);

		return PLUGIN_STOP;
	}


	if (system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Cannot run as system vote is already in progress !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		}

		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	bool use_delay_string_as_first_map = false;

	if (FStrEq(delay_type_string,"end") ||
		(FStrEq(delay_type_string,"round") && gpManiGameType->IsGameType(MANI_GAME_CSS)) ||
		FStrEq(delay_type_string,"now"))
	{
		// Delay type parameter passed in
		if (FStrEq(delay_type_string,"end"))
		{
			delay_type = VOTE_END_OF_MAP_DELAY;
		}
		else if (FStrEq(delay_type_string,"now"))
		{
			delay_type = VOTE_NO_DELAY;
		}
		else if (FStrEq(delay_type_string,"round") && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			delay_type = VOTE_END_OF_ROUND_DELAY;
		}
	}
	else
	{
		use_delay_string_as_first_map = true;
	}

	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	if (mani_vote_allow_extend.GetInt() == 1 &&
		((use_delay_string_as_first_map && argc > 2) ||
		 (!use_delay_string_as_first_map && argc > 3)))
		
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change && (winlimit_change || maxrounds_change))
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
		}
		else if (timelimit_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
		}
		else 
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
		}

		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	if (!use_delay_string_as_first_map && argc < 3)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: You must have one or more maps !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: You must have one or more maps !!");
		}

		return PLUGIN_STOP;
	}

	if (use_delay_string_as_first_map)
	{
		if (!AddMapToVote(&player, svr_command, say_command, delay_type_string)) return PLUGIN_STOP;
	}

	int argc_count = 2;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map1)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map2)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map3)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map4)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map5)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map6)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map7)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map8)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map9)) return PLUGIN_STOP;
	if (argc > argc_count ++) if (!AddMapToVote(&player, svr_command, say_command, map10)) return PLUGIN_STOP;

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	if (svr_command)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player.index;
	}

	system_vote.vote_confirmation = false;
	if (!svr_command && admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
	}

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(player.entity, "Started a random map vote\n");
	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "started a map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_voteextend command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVoteExtend
(
 int index, 
 bool svr_command, 
 int argc, 
 char *command_string
)
{
	player_t player;
	int	admin_index = -1;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin
		player.index = index;
		if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
		if (!IsAdminAllowed(&player, command_string, ALLOW_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Cannot run as system vote is already in progress !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		}

		return PLUGIN_STOP;
	}

	if (!mp_timelimit && !mp_winlimit && !mp_maxrounds)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Server does not support a timelimit, win limit or max rounds !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: does not support a timelimit or win limit!!");
		}

		return PLUGIN_STOP;
	}

	bool timelimit_change = false;
	bool winlimit_change = false;
	bool maxrounds_change = false;

	if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
	if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
	if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

	if (!timelimit_change && !winlimit_change && !maxrounds_change)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: This server has an infinite time limit, win limit and max rounds limit!!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: This server has an infinite time limit, win limit and max rounds limit !!");
		}

		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	if (timelimit_change && winlimit_change && maxrounds_change)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
	}
	else if (timelimit_change)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
	}
	else 
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
	}

	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	if (svr_command)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player.index;
	}

	system_vote.vote_confirmation = false;
	if (!svr_command && admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();

	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_EXTEND_MAP_TITLE), mani_vote_extend_time.GetInt());

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(player.entity, "Started an extend map vote\n");
	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "started an extend map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process Adding a map to the vote option list
//---------------------------------------------------------------------------------
bool	CAdminPlugin::AddMapToVote
(
 player_t *player, 
 bool svr_command, 
 bool say_command, 
 char *map_name
)
{
	vote_option_t vote_option;
	map_t	*m_list = NULL;
	int		m_list_size = 0;

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	bool valid_map = false;

	for (int i = 0; i < m_list_size; i ++)
	{
		if (FStrEq(m_list[i].map_name, map_name))
		{
			valid_map = true;
			break;
		}
	}

	if (!valid_map)
	{
		if (svr_command)
		{
			OutputToConsole(player->entity, svr_command, "Mani Admin Plugin: Map [%s] is invalid !!\n", map_name);
		}
		else
		{
			SayToPlayer(player, "Mani Admin Plugin: Map [%s] is invalid !!", map_name);
		}

		return false;
	}
	
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", map_name);
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", map_name);
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_votequestion command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVoteQuestion
(
 int index, 
 bool svr_command, 
 bool say_command,
 bool menu_command,
 int argc, 
 char *command_string, 
 char *question,
 char *answer1, char *answer2, char *answer3, char *answer4, char *answer5, char *answer6, char *answer7, char *answer8, char *answer9, char *answer10
)
{
	player_t player;
	int	admin_index = -1;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player))
		{
			return PLUGIN_STOP;
		}

		if (!IsClientAdmin(&player, &admin_index))
		{
			SayToPlayer(&player, "Mani Admin Plugin: You are not authorised to use admin commands");
			return PLUGIN_STOP;
		}

		if ((!menu_command && !admin_list[admin_index].flags[ALLOW_QUESTION_VOTE]) ||
		    (menu_command && !admin_list[admin_index].flags[ALLOW_MENU_QUESTION_VOTE])  || war_mode)
		{
			SayToPlayer(&player, "Mani Admin Plugin: You are not authorised to start a question vote");
			return PLUGIN_STOP;
		}
	}

	if (argc < 2) 
	{
		if (say_command) SayToPlayer(&player, "Mani Admin Plugin: %s <question> <answer1, answer2, answer3,...>", command_string);
		if (say_command) SayToPlayer(&player, "See console version of command for examples on how to use %s", command_string);

		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <question> <answer1, answer2, answer3,...>\n", command_string);
		OutputToConsole(player.entity, svr_command, "For example, <%s \"Are we tired of this map ?\"> will run a Yes/No vote against that question\n", command_string);
		OutputToConsole(player.entity, svr_command, "<%s \"What do you think of this map\" \"Love it\" \"Its okay\" \"Hate it\"> will run a vote for that question and 3 answers\n", command_string);

		return PLUGIN_STOP;
	}


	if (system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Cannot run as system vote is already in progress !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		}

		return PLUGIN_STOP;
	}

	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	int argc_count = 2;
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer1);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer2);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer3);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer4);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer5);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer6);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer7);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer8);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer9);
	if (argc > argc_count ++) AddQuestionToVote(&player, svr_command, say_command, answer10);

	if (vote_option_list_size == 0)
	{
		vote_option_t vote_option;
	
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_YES));
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , Translate(M_MENU_YES));
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	system_vote.delay_action = 0;
	system_vote.vote_type = VOTE_QUESTION;
	if (svr_command)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player.index;
	}

	system_vote.vote_confirmation = false;
	if (!svr_command && admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();
	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", question);

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(player.entity, "Started a question vote\n");
	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "started a question vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process Adding a question to the vote option list
//---------------------------------------------------------------------------------
bool	CAdminPlugin::AddQuestionToVote(player_t *player, bool svr_command, bool say_command, char *answer)
{
	vote_option_t vote_option;
	
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", answer);
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", answer);
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_votercon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	CAdminPlugin::ProcessMaVoteRCon
(
 int index, 
 bool svr_command, 
 bool say_command,
 bool menu_command,
 int argc, 
 char *command_string, 
 char *question, 
 char *rcon_command
)
{
	player_t player;
	int	admin_index = -1;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	player.entity = NULL;

	if (!svr_command)
	{
		// Check if player is admin

		player.index = index;
		if (!FindPlayerByIndex(&player))
		{
			return PLUGIN_STOP;
		}

		if (!IsClientAdmin(&player, &admin_index))
		{
			SayToPlayer(&player, "Mani Admin Plugin: You are not authorised to use admin commands");
			return PLUGIN_STOP;
		}

		if ((!menu_command && !admin_list[admin_index].flags[ALLOW_RCON_VOTE]) || 
		    (menu_command && !admin_list[admin_index].flags[ALLOW_MENU_RCON_VOTE]) || war_mode)
		{
			SayToPlayer(&player, "Mani Admin Plugin: You are not authorised to start a rcon vote");
			return PLUGIN_STOP;
		}
	}

	if (argc < 3) 
	{
		if (say_command) SayToPlayer(&player, "Mani Admin Plugin: %s <question> <rcon command>", command_string);
		if (say_command) SayToPlayer(&player, "See console version of command for example on how to use %s", command_string);
		OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: %s <question> <rcon command>\n", command_string);
		OutputToConsole(player.entity, svr_command, "For example, <%s \"Turn on All Talk ?\" \"sv_alltalk 1\"> will run a Yes/No vote to turn on all talk\n", command_string);
		return PLUGIN_STOP;
	}


	if (system_vote.vote_in_progress)
	{
		if (svr_command)
		{
			OutputToConsole(player.entity, svr_command, "Mani Admin Plugin: Cannot run as system vote is already in progress !!\n");
		}
		else
		{
			SayToPlayer(&player, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		}

		return PLUGIN_STOP;
	}

	vote_option_t	vote_option;

	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", rcon_command);
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", rcon_command);
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	system_vote.delay_action = 0;
	system_vote.vote_type = VOTE_RCON;
	if (svr_command)
	{
		system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player.index;
	}

	system_vote.vote_confirmation = false;
	if (!svr_command && admin_list[admin_index].flags[ALLOW_ACCEPT_VOTE])
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();
	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", question);

	StartSystemVote();
	system_vote.vote_in_progress = true;
	LogCommand(player.entity, "Started a RCON vote\n");
	AdminSayToAll(&player, mani_adminvote_anonymous.GetInt(), "started a RCON vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_resetrank command
//---------------------------------------------------------------------------------
void	CAdminPlugin::ParseSayString
(
 const char *say_string, 
 char *trimmed_string_out,
 int  *say_argc
)
{
	char trimmed_string[2048];
	int i;
	int j;
	char terminate_char;
	bool found_quotes;
	int say_length;

	*say_argc = 0;

	for (i = 0; i < MAX_SAY_ARGC; i++)
	{
		// Reset strings for safety
		Q_strcpy(say_argv[i].argv_string,"");
		say_argv[i].index = 0;
	}

	if (!say_string) return;

	say_length = Q_strlen(say_string);
	if (say_length == 0)
	{
		return;
	}

	if (say_length == 1)
	{
		// Only one character in string
		Q_strcpy(trimmed_string, say_string);
		Q_strcpy(say_argv[0].argv_string, say_string);
		say_argv[0].index = 0;
		*say_argc = *say_argc + 1;
		return;
	}

	// Check if quotes are needed to be removed
	if (say_string[0] == '\"' && say_string[Q_strlen(say_string) - 1] == '\"')
	{
		Q_snprintf(trimmed_string, sizeof(trimmed_string), "%s", &(say_string[1]));
		trimmed_string[Q_strlen(trimmed_string) - 1] = '\0';
	}
	else
	{
		Q_snprintf(trimmed_string, sizeof(trimmed_string), "%s", say_string);
	}

	Q_strcpy(trimmed_string_out, trimmed_string);

	// Extract tokens
	i = 0;
	
	while (*say_argc != MAX_SAY_ARGC)
	{
		// Find first non white space
		while (trimmed_string[i] == ' ' && trimmed_string[i] != '\0') i++;

		if (trimmed_string[i] == '\0')	return;

		say_argv[*say_argc].index = i;

		found_quotes = false;
		if (trimmed_string[i] == '\"')
		{
			// Use quote to terminate string
			found_quotes = true;
			terminate_char = '\"';
			i++;
		}
		else
		{
			// Use next space to terminate string
			terminate_char = ' ';
		}

		if (trimmed_string[i] == '\0')	return;

		j = 0;

		while (trimmed_string[i] != terminate_char && trimmed_string[i] != '\0')
		{
			// Copy char
			say_argv[*say_argc].argv_string[j] = trimmed_string[i];
			j++;
			i++;
		}

		say_argv[*say_argc].argv_string[j] = '\0';
		*say_argc = *say_argc + 1;
		if (trimmed_string[i] == '\0') return;
		if (found_quotes) i++;
		if (trimmed_string[i] == '\0') return;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Turn off the overview map function
//---------------------------------------------------------------------------------
void	CAdminPlugin::TurnOffOverviewMap(void)
{

	bool	player_found = false;
	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	for (int i = 1; i <= max_players; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->IsHLTV())
				{
					continue;
				}

				if (playerinfo->IsDead())
				{
					// Force overview mode to 0 for dead clients
					player_found = true;
					mrf.AddPlayer(i);
				}
			}
		}
	}

	if (player_found)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, vgui_message_index );
		msg_buffer->WriteString("overview");
		msg_buffer->WriteByte(0); // Turn off 'overview' map
		msg_buffer->WriteByte(0); // Count of 0
		engine->MessageEnd();

		msg_buffer = engine->UserMessageBegin( &mrf, vgui_message_index );
		msg_buffer->WriteString("specmenu");
		msg_buffer->WriteByte(0); // Turn off 'overview' map
		msg_buffer->WriteByte(0); // Count of 0
		engine->MessageEnd();
	}

	return;
}



//*******************************************************************************
//
// Voting engine
//
//*******************************************************************************
//*******************************************************************************
// Start the system vote by showing menu to players
//*******************************************************************************
void	CAdminPlugin::StartSystemVote (void)
{
	player_t player;

	system_vote.votes_required = 0;
	system_vote.max_votes = 0;

	// Search for players and give them voting options
	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		voter_list[player.index - 1].allowed_to_vote = false;

		if (!FindPlayerByIndex(&player)) continue; // No Player
		if (player.is_bot) continue;  // Bots don't vote

		voter_list[player.index - 1].allowed_to_vote = true;
		voter_list[player.index - 1].voted = false;
		
		if (mani_vote_dont_show_if_alive.GetInt() == 1 && !player.is_dead)
		{
			// If option set, don't show vote to alive players but tell them a vote has started
			SayToPlayer(&player, "Vote has started, type 'vote' to make your choice");
		}
		else
		{
			// Force client to run menu command
			engine->ClientCommand(player.entity, "mani_votemenu\n");
		}

		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_VOTESTART);
		system_vote.votes_required ++;
		system_vote.max_votes ++;
	}

	system_vote.waiting_decision = false;
	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
}

//*******************************************************************************
// Count Votes
//*******************************************************************************
void	CAdminPlugin::ProcessVotes (void)
{
	int number_of_votes = 0;
	int highest_votes = -1;
	int	highest_index = 0;
	int votes_needed;

	player_t admin;

	// Get number of votes cast
	for (int i = 0; i < vote_option_list_size; i ++)
	{
		// Mark the option with the highest number of votes
		if (highest_votes < vote_option_list[i].votes_cast)
		{
			highest_votes = vote_option_list[i].votes_cast;
			highest_index = i;
		}

		number_of_votes +=  vote_option_list[i].votes_cast;
	}

	if (mani_vote_show_vote_mode.GetInt() != 0)
	{
		SayToAll(true, "Results are in, %i vote%s cast", number_of_votes, (number_of_votes == 1) ? " was":"s were");
	}

	// We must have at least 1 vote
	if (number_of_votes == 0)
	{
		system_vote.vote_in_progress = false;
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			// Stop another random vote happening
			system_vote.map_decided = true;
		}

		SayToAll(true, "Vote failed, nobody voted");
		return;
	}

	float vote_percentage = 0.0;

	switch (system_vote.vote_type)
	{
		case VOTE_RANDOM_END_OF_MAP: vote_percentage = mani_vote_end_of_map_percent_required.GetFloat();break;
		case VOTE_RANDOM_MAP: vote_percentage = mani_vote_random_map_percent_required.GetFloat();break;
		case VOTE_MAP: vote_percentage = mani_vote_map_percent_required.GetFloat();break;
		case VOTE_QUESTION: vote_percentage = mani_vote_question_percent_required.GetFloat();break;
		case VOTE_RCON: vote_percentage = mani_vote_rcon_percent_required.GetFloat();break;
		case VOTE_EXTEND_MAP: vote_percentage = mani_vote_extend_percent_required.GetFloat();break;
		default:break;
	}

	votes_needed = (int) ((float) system_vote.max_votes * (vote_percentage * 0.01));

	// Do some safety rounding
	if (votes_needed == 0) votes_needed = 1;
	if (votes_needed > number_of_votes) votes_needed = system_vote.max_votes;

	int	player_vote_count = 0;

	for (int i = 0; i < max_players; i++)
	{
		if (voter_list[i].voted)
		{
			player_vote_count ++;
		}
	}

	// Stop more players from voting
	for (int i = 0; i < max_players; i++)
	{
		voter_list[i].allowed_to_vote = false;
	}

	if (player_vote_count == 0 || player_vote_count < votes_needed)
	{
		SayToAll (true, "Voting failed, not enough players voted");
		system_vote.vote_in_progress = false;
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			// Stop another random vote happening
			system_vote.map_decided = true;
		}

		return;
	}

	system_vote.winner_index = highest_index;

	// Yes we do
	// Do we need need an admin to confirm results (exclude 'No' wins) ?
	if (system_vote.vote_confirmation && !(vote_option_list[1].null_command && highest_index == 1) && system_vote.vote_type != VOTE_QUESTION)
	{
		admin.index = system_vote.vote_starter;
		// We must find the admin if they are still there
		if (FindPlayerByIndex(&admin))
		{
			char	result_text[512];

			// Yes we do, show them the menu
			FreeMenu();

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_YES));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_vote_accept");

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_NO));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_vote_refuse");

			Q_snprintf(result_text, sizeof (result_text), Translate(M_ACCEPT_VOTE_MENU_TITLE), vote_option_list[highest_index].vote_name);
			DrawStandardMenu(&admin, Translate(M_ACCEPT_VOTE_MENU_ESCAPE), result_text, false);
			system_vote.waiting_decision = true;
			system_vote.waiting_decision_time = gpGlobals->curtime + 30.0;
			return;
		}
	}

	// Process the win
	ProcessVoteWin (highest_index);
	system_vote.vote_in_progress = false;

}

//*******************************************************************************
// Process an admins confirmation selection
//*******************************************************************************
void	CAdminPlugin::ProcessVoteConfirmation (player_t *player, bool accept)
{
	if (!system_vote.vote_confirmation) return;
	if (!system_vote.vote_in_progress) return;
	if (system_vote.vote_starter == -1)
	{
		// Non player started vote
		ProcessVoteWin (system_vote.winner_index);
		system_vote.vote_in_progress = false;
		return;
	}

	// If vote starter matches this player slot
	if (system_vote.vote_starter == player->index)
	{
		if (accept)
		{
			AdminSayToAll(player, mani_adminvote_anonymous.GetInt(), "accepted vote"); 
			ProcessVoteWin (system_vote.winner_index);
		}
		else
		{
			AdminSayToAll(player, mani_adminvote_anonymous.GetInt(), "refused vote"); 
			ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_VOTEEND);
		}
	}

	system_vote.vote_in_progress = false;
}

//*******************************************************************************
// Process a Vote Win, i.e change map, run rcon command, show result
//*******************************************************************************
void	CAdminPlugin::ProcessVoteWin (int win_index)
{
	switch (system_vote.vote_type)
	{
		case VOTE_RANDOM_END_OF_MAP: ProcessMapWin(win_index);break;
		case VOTE_RANDOM_MAP: ProcessMapWin(win_index);break;
		case VOTE_MAP: ProcessMapWin(win_index);break;
		case VOTE_QUESTION: ProcessQuestionWin(win_index);break;
		case VOTE_RCON: ProcessRConWin(win_index);break;
		case VOTE_EXTEND_MAP: ProcessExtendWin(win_index);break;
		case VOTE_ROCK_THE_VOTE: ProcessRockTheVoteWin(win_index);break;
		default:break;
	}

	ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_VOTEEND);

}

//*******************************************************************************
// Finalise a Random Map Win
//*******************************************************************************
void	CAdminPlugin::ProcessMapWin (int win_index)
{
	SayToAll (true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (true, "Next map will still be %s", next_map);
		system_vote.map_decided = true;
		return;
	}

	// If extend allowed and win index = 0 (extend)
	if (FStrEq(vote_option_list[win_index].vote_command,"mani_extend_map"))
	{
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP) system_vote.number_of_extends ++;
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "System vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		return;
	}

	Q_strcpy(forced_nextmap,vote_option_list[win_index].vote_command);
	Q_strcpy(next_map, vote_option_list[win_index].vote_command);
	mani_nextmap.SetValue(next_map);

	LogCommand (NULL, "System vote set nextmap to %s\n", vote_option_list[win_index].vote_command);
	override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
	override_setnextmap = true;
	if (system_vote.delay_action == VOTE_NO_DELAY)
	{
		SayToAll(true, "Map will change to %s in 5 seconds", vote_option_list[win_index].vote_command);
		trigger_changemap = true;
		trigger_changemap_time = gpGlobals->curtime + 5.0;
	}
	else if (system_vote.delay_action == VOTE_END_OF_ROUND_DELAY)
	{
		SayToAll(true, "Map will change to %s at the end of the round", vote_option_list[win_index].vote_command);
		if (mp_timelimit)
		{
			mp_timelimit->SetValue(1);
		}
	}
	else
	{
		SayToAll(true, "Next Map will be %s", vote_option_list[win_index].vote_command);
	}

	system_vote.map_decided = true;
}

//*******************************************************************************
// Finalise an RCon vote
//*******************************************************************************
void	CAdminPlugin::ProcessRConWin (int win_index)
{
	SayToAll (true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (true, "Command will not be executed");
		return;
	}
	else
	{
		char	server_cmd[512];
		Q_snprintf(server_cmd, sizeof(server_cmd), "%s\n", vote_option_list[win_index].vote_command);
		SayToAll(true, "Executing RCON command voted for");
		LogCommand (NULL, "System vote ran rcon command %s\n", vote_option_list[win_index].vote_command);
		engine->ServerCommand(server_cmd);
	}
}


//*******************************************************************************
// Finalise a Question win
//*******************************************************************************
void	CAdminPlugin::ProcessQuestionWin (int win_index)
{
	SayToAll (true, Translate(M_SYSTEM_VOTE_MENU_THE_QUESTION_WAS), system_vote.vote_title);
	SayToAll (true, Translate(M_SYSTEM_VOTE_MENU_THE_ANSWER_IS), vote_option_list[win_index].vote_name);
}

//*******************************************************************************
// Finalise an Extend win
//*******************************************************************************
void	CAdminPlugin::ProcessExtendWin (int win_index)
{
	SayToAll (true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (true, "Map will not be extended");
		return;
	}

	// If extend allowed and win index = 0 (extend)
	if (FStrEq(vote_option_list[win_index].vote_command,"mani_extend_map"))
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "System vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		return;
	}
}


//*******************************************************************************
// Check to see if there is only one vote option, and if so, make it a Yes/No
// vote
//*******************************************************************************
bool	CAdminPlugin::IsYesNoVote (void)
{
	vote_option_t vote_option;

	if (vote_option_list_size > 1) return false;

	// Need to make sure that vote options include 'No'
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_NO));
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "");
	vote_option.votes_cast = 0;
	vote_option.null_command = true;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;
	Q_snprintf(vote_option_list[0].vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_YES));

	return true;
}


//*******************************************************************************
// Build Random map vote
//*******************************************************************************
void	CAdminPlugin::BuildRandomMapVote (int max_maps)
{
	last_map_t *last_maps;
	int	maps_to_skip;
	vote_option_t vote_option;
	map_t	*m_list = NULL;
	int		m_list_size = 0;
	int		map_index;
	int		exclude;
	map_t	select_map;
	map_t	*select_list = NULL;
	map_t	*temp_ptr = NULL;
	int		select_list_size = 0;
	map_t	temp_map;

	select_list = NULL;
	select_list_size = 0;

	// Get list of maps already played and exclude them from being voted !!
	last_maps = GetLastMapsPlayed(&maps_to_skip, mani_vote_dont_show_last_maps.GetInt());
	
	// Pick the right map list
	if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 0)
	{
		m_list = map_in_cycle_list;
		m_list_size = map_in_cycle_list_size;
	}
	else if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 1)
	{
		m_list = votemap_list;
		m_list_size = votemap_list_size;
	}
	else
	{
		m_list = map_list;
		m_list_size = map_list_size;
	}

	// Exclude maps already played by pretending they are already selected.
	exclude = 0;
	for (int i = 0; i < m_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < maps_to_skip; j ++)
		{
			if (FStrEq(last_maps[j].map_name, m_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", m_list[i].map_name);
			AddToList((void **) &select_list, sizeof(map_t), &select_list_size);
			select_list[select_list_size - 1] = select_map;
		}
	}

	if (max_maps > select_list_size)
	{
		// Restrict max maps
		max_maps = select_list_size;
	}

	// Add Extend map ?
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	bool allow_extend = false;

	if (mani_vote_allow_extend.GetInt() == 1 && 
		max_maps > 1)
	{
		allow_extend = true;
	}

	if (allow_extend)
	{
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			if (mani_vote_max_extends.GetInt() != 0)
			{
				if (system_vote.number_of_extends >= mani_vote_max_extends.GetInt())
				{
					allow_extend = false;
				}
			}
		}
	}

	if (allow_extend)
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change && winlimit_change && maxrounds_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
		}
		else if (timelimit_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
		}
		else
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
		}

		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	// Generate a bit more randomness
	srand( (unsigned)time(NULL));
	for (int i = 0; i < max_maps; i ++)
	{
		map_index = rand() % select_list_size;

		// Add map to vote options list
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", select_list[map_index].map_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", select_list[map_index].map_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;

		if (map_index != select_list_size - 1)
		{
			temp_map = select_list[select_list_size - 1];
			select_list[select_list_size - 1] = select_list[map_index];
			select_list[map_index] = temp_map;
		}

		if (select_list_size != 1)
		{
			// Shrink array by 1
			temp_ptr = (map_t *) realloc(select_list, (select_list_size - 1) * sizeof(map_t));
			select_list = temp_ptr;
			select_list_size --;
		}
		else
		{
			free(select_list);
			select_list = NULL;
			select_list_size = 0;
			break;
		}
	}

	if (select_list != NULL)
	{
		free(select_list);
	}

	// Map list built
}



//---------------------------------------------------------------------------------
// Purpose: Generic function to handle any system voting
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuSystemVotemap( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	// Check if system vote still allowed 

	if (!system_vote.vote_in_progress)
	{
		SayToPlayer(player, "Voting not allowed at this time !!");
		return;
	}

	if (voter_list[player->index - 1].voted)
	{
		SayToPlayer(player, "You have already voted !!");
		return;
	}

	if (!voter_list[player->index - 1].allowed_to_vote)
	{
		SayToPlayer(player, "You are too late to join this vote !!");
		return;
	}

	if (argc - argv_offset == 2)
	{
		ProcessPlayerVoted(player, Q_atoi(engine->Cmd_Argv(1 + argv_offset)));
		return;
	}

	// Show votes to player
	FreeMenu();
	CreateList ((void **) &menu_list, sizeof(menu_t), vote_option_list_size, &menu_list_size);

	for( int i = 0; i < vote_option_list_size; i++ )
	{
		Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", vote_option_list[i].vote_name);
		Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "mani_votemenu %i", i);
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	// Show menu to player
	DrawSubMenu (player, 
				Translate(M_SYSTEM_VOTE_MENU_ESCAPE), 
				system_vote.vote_title, 
				next_index,
				"mani_votemenu",
				"",
				false,
				(int) (system_vote.end_vote_time - gpGlobals->curtime));
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Generic function to handle any system voting
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessPlayerVoted( player_t *player, int vote_index)
{
	voter_list[player->index - 1].voted = true;
	voter_list[player->index - 1].allowed_to_vote = false;
	voter_list[player->index - 1].vote_option_index = vote_index;
	vote_option_list[vote_index].votes_cast ++;

	switch(mani_vote_show_vote_mode.GetInt())
	{
		case (0): SayToPlayer (player, "You voted for %s", vote_option_list[vote_index].vote_name); break;
		case (1): SayToAll (true, "Player %s voted", player->name); break;
		case (2): SayToAll (true, "Vote for %s", vote_option_list[vote_index].vote_name); break;
		case (3): SayToAll(true, "%s voted %s", player->name, vote_option_list[vote_index].vote_name); break;
		default : break;
	}

	system_vote.votes_required --;
	if (system_vote.votes_required <= 0)
	{
		// Everyone has voted, force timeout in game frame code !!
		system_vote.end_vote_time = 0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Build up a map list for user started votes (run once a map)
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessBuildUserVoteMaps(void)
{
	last_map_t *last_maps = NULL;
	int	maps_to_skip;
	map_t	*m_list = NULL;
	int		m_list_size = 0;
	map_t	select_map;

	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);

	// Get list of maps already played and exclude them from being voted !!
	last_maps = GetLastMapsPlayed(&maps_to_skip, mani_vote_dont_show_last_maps.GetInt());

	if (maps_to_skip != 0)
	{
		Msg("Maps Not Included for voting !!\n");
		for (int i = 0; i < maps_to_skip; i ++)
		{
			Msg("%s ", last_maps[i].map_name);
		}

		Msg("\n");
	}

	m_list = votemap_list;
	m_list_size = votemap_list_size;

	// Exclude maps already played
	for (int i = 0; i < m_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < maps_to_skip; j ++)
		{
			if (FStrEq(last_maps[j].map_name, m_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", m_list[i].map_name);
			AddToList((void **) &user_vote_map_list, sizeof(map_t), &user_vote_map_list_size);
			user_vote_map_list[user_vote_map_list_size - 1] = select_map;
		}
	}

	Msg("Maps available for user vote\n");
	for (int i = 0; i < user_vote_map_list_size; i ++)
	{
		Msg("%s ", user_vote_map_list[i].map_name);
	}
	Msg("\n");	
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowCurrentUserMapVotes( player_t *player, int votes_required )
{

	OutputToConsole(player->entity, false, "\n");
	OutputToConsole(player->entity, false, "%s\n", mani_version);
	OutputToConsole(player->entity, false, "\nVotes required for map change is %i\n\n", votes_required);
	OutputToConsole(player->entity, false, "ID  Map Name            Votes\n");
	OutputToConsole(player->entity, false, "-----------------------------\n");

	int votes_found;

	if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
		system_vote.number_of_extends < mani_vote_max_extends.GetInt())
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change || winlimit_change || maxrounds_change)
		{
			// Extension of map allowed
			votes_found = 0;
			for (int j = 0; j < max_players; j++)
			{
				// Count votes for extension index 0
				if (user_vote_list[j].map_index == 0) votes_found ++;
			}

			OutputToConsole(player->entity, false, "%-4i%-20s%i\n", 0, "Extend Map", votes_found);
		}
	}

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		votes_found = 0;
		for (int j = 0; j < max_players; j++)
		{
			if (user_vote_list[j].map_index == i + 1)
			{
				votes_found ++;
			}
		}

		OutputToConsole(player->entity, false, "%-4i%-20s%i\n", i + 1, user_vote_map_list[i].map_name, votes_found);
	}

	OutputToConsole(player->entity, false, "\nTo vote for a map, type votemap <id> or votemap <map name>\n");
	OutputToConsole(player->entity, false, "e.g votemap 3, votemap de_dust2\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMaUserVoteMap(player_t *player, int argc, const char *map_id)
{
	int votes_required;

	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserMapVotes(player, votes_required);
		return;
	}

	if (!CanWeUserVoteMapYet(player)) return;
	if (!CanWeUserVoteMapAgainYet(player)) return;

	int		map_index = -1;

	// Deal with extend if it's available
	if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
		system_vote.number_of_extends < mani_vote_max_extends.GetInt())
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change || winlimit_change || maxrounds_change)
		{
			if (FStrEq(map_id,"0") || FStrEq(map_id, "Extend Map"))
			{
				map_index = 0;
			}
		}
	}

	if (map_index == -1)
	{
		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			// Try and match by name
			if (FStrEq(map_id, user_vote_map_list[i].map_name))
			{
				map_index = i + 1;
				break;
			}
		}

		if (map_index == -1)
		{
			// Map name not found, try map number instead
			map_index = Q_atoi (map_id);
			if (map_index < 1 || map_index > user_vote_map_list_size)
			{
				map_index = -1;
			}
		}
	}

	// Map failed validation so tell user
	if (map_index == -1)
	{
		OutputToConsole(player->entity, false, "%s is an invalid map to vote for, type 'votemap' to see the maps available\n", map_id);
		SayToPlayer(player, "%s is an invalid map to vote for, type 'votemap' to see the maps available", map_id);
		return;
	}

	// Check if player has voted already and change the vote

	bool found_vote = false;
	for (int i = 0; i < max_players; i++)
	{
		if (user_vote_list[i].map_index != -1)
		{
			found_vote = true;
			break;
		}
	}

	if (!found_vote)
	{
		// First vote, inform world
		SayToAll(false, "Voting started, type votemap in console or say 'votemap' in game for options");
	}

	user_vote_list[player->index - 1].map_index = map_index;
	user_vote_list[player->index - 1].map_vote_timestamp = gpGlobals->curtime;

	int counted_votes = 0;

	for (int i = 0; i < max_players; i ++)
	{
		if (user_vote_list[i].map_index == map_index)
		{
			counted_votes ++;
		}
	}

	int votes_left = votes_required - counted_votes;

	SayToAll(false, "Player %s voted %s %s, %i more vote%s required", 
								player->name, 
								(map_index == 0) ? "to":"for",
								(map_index == 0) ? "Extend Map":user_vote_map_list[map_index - 1].map_name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	OutputToConsole(player->entity, false, "Player %s voted %s %s, %i more vote%s required\n", 
								player->name, 
								(map_index == 0) ? "to":"for",
								(map_index == 0) ? "Extend Map":user_vote_map_list[map_index - 1].map_name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the change of map
		ProcessUserVoteMapWin(map_index);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessUserVoteMapWin(int map_index)
{
	if (map_index == 0)
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		system_vote.number_of_extends ++;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "User vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
			map_start_time = gpGlobals->curtime + (mani_vote_extend_time.GetInt() * 60);
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "User vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "User vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}
	}
	else
	{
		Q_strcpy(forced_nextmap,user_vote_map_list[map_index - 1].map_name);
		Q_strcpy(next_map, user_vote_map_list[map_index - 1].map_name);
		mani_nextmap.SetValue(next_map);
		LogCommand (NULL, "User vote set nextmap to %s\n", user_vote_map_list[map_index - 1].map_name);

		override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
		override_setnextmap = true;
		SayToAll(true, "Map will change to %s in 5 seconds", user_vote_map_list[map_index - 1].map_name);
		trigger_changemap = true;
		system_vote.map_decided = true;
		trigger_changemap_time = gpGlobals->curtime + 5.0;
		map_start_time = gpGlobals->curtime;
	}

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes
		user_vote_list[i].map_index = -1;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteMapYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_map_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteMapAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].map_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Gets User votes required
//---------------------------------------------------------------------------------
int CAdminPlugin::GetVotesRequiredForUserVote
( 
 bool player_leaving, 
 float percentage, 
 int minimum_votes
 )
{
	int number_of_players;
	int votes_required;

	number_of_players = GetNumberOfActivePlayers () - ((player_leaving) ? 1:0);

	votes_required = (int) ((float) number_of_players * (percentage * 0.01));

	// Kludge the votes required if unreasonable	
	if (votes_required <= 0) 
	{
		votes_required = 1;
	}
	else if (votes_required > number_of_players) 
	{
		votes_required = number_of_players;
	}

	if (votes_required < minimum_votes) votes_required = minimum_votes;

	return votes_required;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Map menu draw 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuUserVoteMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	int votes_required;


	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *map_id = engine->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteMap (player, 2, map_id);
		return;
	}
	else
	{
		int votes_found;
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteMapYet(player)) return;
		if (!CanWeUserVoteMapAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
			system_vote.number_of_extends < mani_vote_max_extends.GetInt())
		{
			bool timelimit_change = false;
			bool winlimit_change = false;
			bool maxrounds_change = false;

			if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
			if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
			if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

			if (timelimit_change || winlimit_change || maxrounds_change)
			{
				// Extension of map allowed
				votes_found = 0;
				for (int j = 0; j < max_players; j++)
				{
					// Count votes for extension index 0
					if (user_vote_list[j].map_index == 0) votes_found ++;
				}

				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Extend Map [%i]", votes_found);							
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotemapmenu 0");
			}
		}

		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			votes_found = 0;
			for (int j = 0; j < max_players; j++)
			{
				if (user_vote_list[j].map_index == i + 1)
				{
					votes_found ++;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", votes_found, user_vote_map_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotemapmenu %i", i + 1);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), Translate(M_USER_VOTE_MAP_MENU_TITLE), votes_required);							

		// Draw menu list
		DrawSubMenu (player, Translate(M_USER_VOTE_MAP_MENU_ESCAPE), votes_required_text, next_index, "mani_uservotemapmenu","", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Rock the Vote code
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// Purpose: Shows maps you can nominate
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowCurrentRockTheVoteMaps( player_t *player)
{

	OutputToConsole(player->entity, false, "\n");
	OutputToConsole(player->entity, false, "%s\n", mani_version);
	OutputToConsole(player->entity, false, "\nMaps available for nomination\n");
	OutputToConsole(player->entity, false, "ID  Map Name\n");
	OutputToConsole(player->entity, false, "-----------------------------\n");

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		OutputToConsole(player->entity, false, "%-4i%-20s\n", i + 1, user_vote_map_list[i].map_name);
	}

	OutputToConsole(player->entity, false, "\nTo nominate a map, type nominate <id> or nominate <map name>\n");
	OutputToConsole(player->entity, false, "e.g nominate 3, nominate de_dust2\n\n");

	return;
}

//*******************************************************************************
// Finalise a Random Map Win
//*******************************************************************************
void	CAdminPlugin::ProcessRockTheVoteWin (int win_index)
{
	SayToAll (true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (true, "Next map will still be %s", next_map);
		system_vote.map_decided = true;
		return;
	}

	Q_strcpy(forced_nextmap,vote_option_list[win_index].vote_command);
	Q_strcpy(next_map, vote_option_list[win_index].vote_command);
	mani_nextmap.SetValue(next_map);
	LogCommand (NULL, "System vote set nextmap to %s\n", vote_option_list[win_index].vote_command);
	override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
	override_setnextmap = true;
	SayToAll(true, "Map will change to %s in 5 seconds", vote_option_list[win_index].vote_command);
	trigger_changemap = true;
	trigger_changemap_time = gpGlobals->curtime + 5.0;

	system_vote.map_decided = true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle player saying 'nominate'
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessRockTheVoteNominateMap(player_t *player)
{
	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(player, "Your rockthevote command has already been registered, you can't nominate a map");
		return;
	}

	// Run nominate menu
	engine->ClientCommand(player->entity, "mani_rtvnominate\n");
	
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeRockTheVoteYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_time_before_rock_the_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "Rock the Vote is not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can nominate again 
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeNominateAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].nominate_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "You cannot nominate again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//*******************************************************************************
// Build Rock the vote map
//*******************************************************************************
void	CAdminPlugin::BuildRockTheVoteMapVote (void)
{
	vote_option_t vote_option;
	int	map_index;
	int	exclude;
	map_t	select_map;
	map_t	*select_list;
	map_t	*temp_ptr;
	int	select_list_size;
	map_t	temp_map;
	int	max_maps;
	vote_option_t *nominate_list = NULL;
	vote_option_t *temp_nominate_ptr;
	int		nominate_list_size = 0;

	select_list = NULL;
	select_list_size = 0;

	// Get maps nominated by users
	for (int i = 0; i < max_players; i++ )
	{
		bool	found_map;

		found_map = false;

		if (user_vote_list[i].nominated_map == -1) continue;

		for (int j = 0; j < nominate_list_size; j++)
		{
			if (FStrEq(nominate_list[j].vote_name, user_vote_map_list[user_vote_list[i].nominated_map].map_name)) 
			{
				nominate_list[j].votes_cast ++;
				found_map = true;
				break;
			}
		}

		if (!found_map)
		{
			AddToList((void **) &nominate_list, sizeof(vote_option_t), &nominate_list_size);
			Q_strcpy(nominate_list[nominate_list_size - 1].vote_name,  user_vote_map_list[user_vote_list[i].nominated_map].map_name);
			nominate_list[nominate_list_size - 1].votes_cast = 1;
		}
	}

	// Sort into winning order
	qsort(nominate_list, nominate_list_size, sizeof(vote_option_t), sort_nominations_by_votes_cast); 

	for (int i = 0; i < nominate_list_size; i++)
	{
		Msg("Nominations [%s] Votes [%i]\n", nominate_list[i].vote_name, nominate_list[i].votes_cast);
	}

	if (mani_vote_rock_the_vote_number_of_nominations.GetInt() < nominate_list_size)
	{
		// Prune the list down
		temp_nominate_ptr = (vote_option_t *) realloc(nominate_list, 
					(mani_vote_rock_the_vote_number_of_nominations.GetInt() * sizeof(vote_option_t)));
		nominate_list = temp_nominate_ptr;
		nominate_list_size = mani_vote_rock_the_vote_number_of_nominations.GetInt();
	}


	// Exclude maps already played by pretending they are already selected.
	exclude = 0;
	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < nominate_list_size; j ++)
		{
			if (FStrEq(nominate_list[j].vote_name, user_vote_map_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", user_vote_map_list[i].map_name);
			AddToList((void **) &select_list, sizeof(map_t), &select_list_size);
			select_list[select_list_size - 1] = select_map;
		}
	}

	// Calculate number of random maps to add
	max_maps = mani_vote_rock_the_vote_number_of_maps.GetInt() - nominate_list_size;
	if (max_maps < 0) max_maps = 0;

	if (max_maps > select_list_size)
	{
		// Restrict max maps
		max_maps = select_list_size;
	}

	// Add Extend map ?
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	// Add nominated maps to vote options list
	for (int i = 0; i < nominate_list_size; i++)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", nominate_list[i].vote_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", nominate_list[i].vote_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	// Generate a bit more randomness
	srand( (unsigned)time(NULL));
	for (int i = 0; i < max_maps; i ++)
	{
		map_index = rand() % select_list_size;

		// Add map to vote options list
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", select_list[map_index].map_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", select_list[map_index].map_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;

		if (map_index != select_list_size - 1)
		{
			temp_map = select_list[select_list_size - 1];
			select_list[select_list_size - 1] = select_list[map_index];
			select_list[map_index] = temp_map;
		}

		if (select_list_size != 1)
		{
			// Shrink array by 1
			temp_ptr = (map_t *) realloc(select_list, (select_list_size - 1) * sizeof(map_t));
			select_list = temp_ptr;
			select_list_size --;
		}
		else
		{
			free(select_list);
			select_list = NULL;
			select_list_size = 0;
			break;
		}
	}

	if (select_list != NULL)
	{
		free(select_list);
	}

	// Map list built
}

//---------------------------------------------------------------------------------
// Purpose: Start Rock the Vote proceedings
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessStartRockTheVote(void)
{
	Msg("Triggering Rock The Vote!!\n");
	system_vote.delay_action = VOTE_NO_DELAY;
	system_vote.vote_type = VOTE_ROCK_THE_VOTE;
	system_vote.vote_starter = -1;
	system_vote.vote_confirmation = false;
	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	BuildRockTheVoteMapVote ();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(M_SYSTEM_VOTE_MENU_VOTE_NEXT_MAP_TITLE));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), Translate(M_SYSTEM_VOTE_MENU_WILL_NEXT_MAP_TITLE), vote_option_list[0].vote_command);
	}

	StartSystemVote();
	system_vote.vote_in_progress = true;
	system_vote.start_rock_the_vote = false;
	system_vote.no_more_rock_the_vote = true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Map menu draw 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuRockTheVoteNominateMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();

	if (!CanWeNominateAgainYet(player))
	{
		return;
	}

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *map_id = engine->Cmd_Argv(1 + argv_offset);
		ProcessMaRockTheVoteNominateMap (player, 2, map_id);
		return;
	}
	else
	{
		if (mani_vote_rock_the_vote_number_of_nominations.GetInt() == 0)
		{
			SayToPlayer(player, "Nominations are not allowed on this server for rock the vote");
			return;
		}

		if (system_vote.map_decided)
		{
			SayToPlayer(player, "Map has already been decided, rock the vote not available");
			return;
		}

		if (system_vote.no_more_rock_the_vote) 
		{
			SayToPlayer(player, "No more 'rockthevote' commands are allowed on this map");
			return;
		}

		// Check if already typed rock the vote
		if (user_vote_list[player->index - 1].rock_the_vote)
		{
			SayToPlayer(player, "Your rockthevote command has already been registered, you can't nominate another map");
			return;
		}

		if (!CanWeNominateAgainYet(player))
		{
			return;
		}

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", user_vote_map_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_rtvnominate %i", i + 1);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		// Draw menu list
		DrawSubMenu (player, "Rock The Vote\nChoose a map to nominated then type rockthevote in game", "Press Esc to nominate a map", next_index, "mani_rtvnominate","", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process nominations for rock the vote
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMaRockTheVoteNominateMap(player_t *player, int argc, const char *map_id)
{
	int map_index;

	if (mani_voting.GetInt() == 0) return;
	if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;
	if (mani_vote_rock_the_vote_number_of_nominations.GetInt() == 0)
	{
		SayToPlayer(player, "Nominations are not allowed on this server for rock the vote");
		return;
	}

	if (system_vote.map_decided)
	{
		SayToPlayer(player, "Map has already been decided, rock the vote not available");
		return;
	}

	if (system_vote.no_more_rock_the_vote) 
	{
		SayToPlayer(player, "No more 'rockthevote' commands are allowed on this map");
		return;
	}

	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(player, "Your rockthevote command has already been registered, you can't nominate another map");
		return;
	}


	if (argc == 1)
	{
		ShowCurrentRockTheVoteMaps(player);
		return;
	}

	if (!CanWeNominateAgainYet(player))
	{
		return;
	}

	map_index = -1;

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		// Try and match by name
		if (FStrEq(map_id, user_vote_map_list[i].map_name))
		{
			map_index = i;
			break;
		}
	}

	if (map_index == -1)
	{
		// Map name not found, try map number instead
		map_index = Q_atoi (map_id);
		if (map_index < 1 || map_index > user_vote_map_list_size)
		{
			map_index = -1;
		}
		else
		{
			map_index --;
		}
	}

	// Map failed validation so tell user
	if (map_index == -1)
	{
		SayToPlayer(player, "%s is an invalid map to vote for, type 'nominate' to see the maps available", map_id);
		return;
	}

	user_vote_list[player->index - 1].nominated_map = map_index;
	user_vote_list[player->index - 1].nominate_timestamp = gpGlobals->curtime;

	SayToAll(false, "Player %s nominated map %s for rock the vote", 
								player->name, 
								user_vote_map_list[map_index].map_name);

}


//---------------------------------------------------------------------------------
// Purpose: Process players who type 'rockthevote'
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMaRockTheVote(player_t *player)
{

	if (mani_voting.GetInt() == 0) return;
	if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;
	if (system_vote.map_decided) 
	{
		SayToPlayer(player, "Map has already been decided, rock the vote not available");
		return;
	}

	if (system_vote.no_more_rock_the_vote) 
	{
		SayToPlayer(player, "No more 'rockthevote' commands are allowed on this map");
		return;
	}

	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(player, "Your rockthevote command has already been registered");
		return;
	}

	if (!CanWeRockTheVoteYet(player))
	{
		return;
	}


	int counted_votes = 0;

	user_vote_list[player->index - 1].rock_the_vote = true;

	for (int i = 0; i < max_players; i ++)
	{
		if (user_vote_list[i].rock_the_vote)
		{
			counted_votes ++;
		}
	}

	int votes_required;

	votes_required = GetVotesRequiredForUserVote(false, mani_vote_rock_the_vote_threshold_percent.GetFloat(), mani_vote_rock_the_vote_threshold_minimum.GetInt());
	if (votes_required > counted_votes)
	{
		if (counted_votes == 1)
		{
			SayToAll(false, "Player %s has started rock the vote, type 'rockthevote' to join in, %i votes required", player->name, votes_required - counted_votes);
			if (mani_vote_rock_the_vote_number_of_maps.GetInt() != 0)
			{
				SayToAll(false, "Type 'nominate' to bring up a nomination menu for your favourite map");
			}
		}
		else
		{
			SayToAll(false, "Player %s has entered rock the vote, %i votes required to trigger map vote", player->name, votes_required - counted_votes);
		}

		return;
	}

	// Threshold reached
	system_vote.start_rock_the_vote = true;
	if (system_vote.vote_in_progress)
	{
		SayToAll(false, "Rock the vote will start after the current system vote has ended");
	}

	for (int i = 0; i < max_players; i ++)
	{
		user_vote_list[i].rock_the_vote = true;
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Vote kick code 
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowCurrentUserKickVotes( player_t *player, int votes_required )
{

	OutputToConsole(player->entity, false, "\n");
	OutputToConsole(player->entity, false, "%s\n", mani_version);
	OutputToConsole(player->entity, false, "\nVotes required for user kick is %i\n\n", votes_required);
	OutputToConsole(player->entity, false, "ID   Name                     Votes\n");
	OutputToConsole(player->entity, false, "-----------------------------------\n");

	for (int i = 1; i <= max_players; i++)
	{
		player_t	server_player;

		// Don't show votes against player who ran it
		if (player->index == i) continue;

		server_player.index = i;
		if (!FindPlayerByIndex(&server_player)) continue;
		if (server_player.is_bot) continue;

		OutputToConsole(player->entity, false, "%-5i%-26s%i\n", server_player.user_id, server_player.name, user_vote_list[i-1].kick_votes);
	}

	OutputToConsole(player->entity, false, "\nTo vote to kick a player, type votekick <id> or votekick <player name or part of their name>\n");
	OutputToConsole(player->entity, false, "e.g votekick 3, votekick Mani\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMaUserVoteKick(player_t *player, int argc, const char *kick_id)
{
	int votes_required;
	int admin_index = -1;

	if (mani_vote_user_vote_kick_mode.GetInt() == 0)
	{
		bool found_admin = false;

		for (int i = 1; i < max_players; i ++)
		{
			player_t	admin_player;

			admin_player.index = i;
			if (!FindPlayerByIndex(&admin_player)) continue;
			if (admin_player.is_bot) continue;

			if (!IsClientAdmin(&admin_player, &admin_index)) continue;
			if (admin_list[admin_index].flags[ALLOW_KICK])
			{
				found_admin = true;
				break;
			}
		}

		if (found_admin)
		{
			SayToPlayer(player, "You can not vote kick when admin is on the server");
			return;
		}
	}
		
	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserKickVotes(player, votes_required);
		return;
	}

	if (!CanWeUserVoteKickYet(player)) return;
	if (!CanWeUserVoteKickAgainYet(player)) return;

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player, (char *) kick_id, IMMUNITY_ALLOW_KICK))
	{
		SayToPlayer(player, "Did not find player %s", kick_id);
		return;
	}

	player_t *target_player = &(target_player_list[0]);

	if (target_player->index == player->index)
	{
		SayToPlayer(player, "Voting yourself is not a good idea !!");
		return;
	}

	if (target_player->is_bot)
	{
		SayToPlayer(player, "Player %s is a bot, cannot perform command\n", target_player->name);
		return;
	}

	admin_index = -1;
	IsClientAdmin(target_player, &admin_index);

	if (user_vote_list[player->index - 1].kick_id != -1 && admin_index == -1)
	{
		// Player already voted, alter previous vote if not admin
		player_t change_player;

		change_player.user_id = user_vote_list[player->index - 1].kick_id;
		if (FindPlayerByUserID(&change_player))
		{
			user_vote_list[change_player.index - 1].kick_votes --;
		}
	}

	if (admin_index == -1)
	{
		// Add votes if not admin
		user_vote_list[target_player->index - 1].kick_votes ++;
	}

	user_vote_list[player->index - 1].kick_id = target_player->user_id;
	user_vote_list[player->index - 1].kick_vote_timestamp = gpGlobals->curtime;

	int votes_left = votes_required - user_vote_list[target_player->index - 1].kick_votes;

	SayToAll(false, "Player %s voted to kick %s, %i more vote%s required", 
								player->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	OutputToConsole(player->entity, false, "Player %s voted to kick %s, %i more vote%s required\n", 
								player->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the kick of the player
		ProcessUserVoteKickWin(target_player);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process a user vote kick win
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessUserVoteKickWin(player_t *player)
{
	char	kick_cmd[256];

	PrintToClientConsole(player->entity, "You have been kicked by vote\n");
	Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were vote kicked\n", player->user_id);
	LogCommand (NULL, "User vote kick using %s", kick_cmd);
	engine->ServerCommand(kick_cmd);
	SayToAll(true, "Player %s has been kicked by user vote", player->name);

	user_vote_list[player->index - 1].kick_votes = 0;
	user_vote_list[player->index - 1].kick_id = -1;

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes for that player
		if (user_vote_list[i].kick_id == player->user_id) 
		{
			user_vote_list[i].kick_id = -1;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteKickYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_kick_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteKickAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].kick_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Kick menu draw 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuUserVoteKick( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	int votes_required;

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *user_id = engine->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteKick (player, 2, user_id);
		return;
	}
	else
	{
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteKickYet(player)) return;
		if (!CanWeUserVoteKickAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 1; i <= max_players; i++)
		{
			player_t server_player;

			if (player->index == i) continue;

			server_player.index = i;
			if (!FindPlayerByIndex(&server_player)) continue;
			if (server_player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", user_vote_list[i-1].kick_votes, server_player.name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotekickmenu %i", server_player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, server_player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), Translate(M_USER_VOTE_KICK_MENU_TITLE), votes_required);							

		// Draw menu list
		DrawSubMenu (player, Translate(M_USER_VOTE_KICK_MENU_ESCAPE), votes_required_text, next_index, "mani_uservotekickmenu", "", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Vote ban code (practically a copy of the vote kick code :/
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void CAdminPlugin::ShowCurrentUserBanVotes( player_t *player, int votes_required )
{

	OutputToConsole(player->entity, false, "\n");
	OutputToConsole(player->entity, false, "%s\n", mani_version);
	OutputToConsole(player->entity, false, "\nVotes required for user ban is %i\n\n", votes_required);
	OutputToConsole(player->entity, false, "ID   Name                     Votes\n");
	OutputToConsole(player->entity, false, "-----------------------------------\n");

	for (int i = 1; i <= max_players; i++)
	{
		player_t	server_player;

		// Don't show votes against player who ran it
		if (player->index == i) continue;

		server_player.index = i;
		if (!FindPlayerByIndex(&server_player)) continue;
		if (server_player.is_bot) continue;

		OutputToConsole(player->entity, false, "%-5i%-26s%i\n", server_player.user_id, server_player.name, user_vote_list[i-1].ban_votes);
	}

	OutputToConsole(player->entity, false, "\nTo vote to ban a player, type voteban <id> or voteban <player name or part of their name>\n");
	OutputToConsole(player->entity, false, "e.g voteban 3, voteban Mani\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMaUserVoteBan(player_t *player, int argc, const char *ban_id)
{
	int votes_required;
	int admin_index = -1;

	if (mani_vote_user_vote_ban_mode.GetInt() == 0)
	{
		bool found_admin = false;

		for (int i = 1; i < max_players; i ++)
		{
			player_t	admin_player;

			admin_player.index = i;
			if (!FindPlayerByIndex(&admin_player)) continue;
			if (admin_player.is_bot) continue;

			if (!IsClientAdmin(&admin_player, &admin_index)) continue;
			if (admin_list[admin_index].flags[ALLOW_BAN])
			{
				found_admin = true;
				break;
			}
		}

		if (found_admin)
		{
			SayToPlayer(player, "You can not vote ban when admin is on the server");
			return;
		}
	}
		
	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserBanVotes(player, votes_required);
		return;
	}

	if (!CanWeUserVoteBanYet(player)) return;
	if (!CanWeUserVoteBanAgainYet(player)) return;

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player, (char *) ban_id, IMMUNITY_ALLOW_BAN))
	{
		SayToPlayer(player, "Did not find player %s", ban_id);
		return;
	}

	player_t *target_player = &(target_player_list[0]);

	if (target_player->index == player->index)
	{
		SayToPlayer(player, "Voting yourself is not a good idea !!");
		return;
	}

	if (target_player->is_bot)
	{
		SayToPlayer(player, "Player %s is a bot, cannot perform command\n", target_player->name);
		return;
	}

	admin_index = -1;
	IsClientAdmin(target_player, &admin_index);

	if (user_vote_list[player->index - 1].ban_id != -1 && admin_index == -1)
	{
		// Player already voted, alter previous vote if not admin
		player_t change_player;

		change_player.user_id = user_vote_list[player->index - 1].ban_id;
		if (FindPlayerByUserID(&change_player))
		{
			user_vote_list[change_player.index - 1].ban_votes --;
		}
	}

	if (admin_index == -1)
	{
		// Add votes if not admin
		user_vote_list[target_player->index - 1].ban_votes ++;
	}

	user_vote_list[player->index - 1].ban_id = target_player->user_id;
	user_vote_list[player->index - 1].ban_vote_timestamp = gpGlobals->curtime;

	int votes_left = votes_required - user_vote_list[target_player->index - 1].ban_votes;

	SayToAll(false, "Player %s voted to ban %s, %i more vote%s required", 
								player->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	OutputToConsole(player->entity, false, "Player %s voted to ban %s, %i more vote%s required\n", 
								player->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the ban of the player
		ProcessUserVoteBanWin(target_player);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process a user vote ban win
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessUserVoteBanWin(player_t *player)
{
	char	ban_cmd[256];

	if (mani_vote_user_vote_ban_type.GetInt() == 0 && sv_lan->GetInt() != 1)
	{
		// Ban by user id
		Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_vote_user_vote_ban_time.GetInt(), 
										player->user_id);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeid\n");
	}
	else if (mani_vote_user_vote_ban_type.GetInt() == 1)
	{
		// Ban by user ip address
		Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_vote_user_vote_ban_time.GetInt(), 
										player->ip_address);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeip\n");
	}
	else if (mani_vote_user_vote_ban_type.GetInt() == 2)
	{
		// Ban by user id and ip address
		if (sv_lan->GetInt() == 0)
		{
			Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
								mani_vote_user_vote_ban_time.GetInt(), 
								player->user_id);
			LogCommand(NULL, "User vote banned using %s", ban_cmd);
			engine->ServerCommand(ban_cmd);
			engine->ServerCommand("writeid\n");
		}

		Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
								mani_vote_user_vote_ban_time.GetInt(), 
								player->ip_address);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeip\n");
	}

	PrintToClientConsole(player->entity, "You have been banned by vote\n");
	SayToAll(true, "Player %s has been banned by user vote", player->name);

	user_vote_list[player->index - 1].ban_votes = 0;
	user_vote_list[player->index - 1].ban_id = -1;

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes for that player
		if (user_vote_list[i].ban_id == player->user_id) 
		{
			user_vote_list[i].ban_id = -1;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteBanYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_ban_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool CAdminPlugin::CanWeUserVoteBanAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].ban_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(player, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Ban menu draw 
//---------------------------------------------------------------------------------
void CAdminPlugin::ProcessMenuUserVoteBan( player_t *player, int next_index, int argv_offset )
{
	const int argc = engine->Cmd_Argc();
	int votes_required;

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *user_id = engine->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteBan (player, 2, user_id);
		return;
	}
	else
	{
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteBanYet(player)) return;
		if (!CanWeUserVoteBanAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 1; i <= max_players; i++)
		{
			player_t server_player;

			if (player->index == i) continue;

			server_player.index = i;
			if (!FindPlayerByIndex(&server_player)) continue;
			if (server_player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", user_vote_list[i-1].ban_votes, server_player.name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotebanmenu %i", server_player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, server_player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), Translate(M_USER_VOTE_BAN_MENU_TITLE), votes_required);							

		// Draw menu list
		DrawSubMenu (player, Translate(M_USER_VOTE_BAN_MENU_ESCAPE), votes_required_text, next_index, "mani_uservotebanmenu", "", false,-1);
	}

	return;
}

//*******************************************************************************
//
// New Console Commands
//
//*******************************************************************************
CON_COMMAND(ma_kick, "Kicks a user (ma_kick <partial user name, user id or steam id)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaKick
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1) // Command option
					);
	return;
}

CON_COMMAND(ma_ban, "Bans a user (ma_ban <time in minutes> <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBan
					(
					0, // Client index
					true,  // Sever console command type
					false, // By IP
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option minutes
					engine->Cmd_Argv(2) // Command option player
					);
	return;
}

CON_COMMAND(ma_banip, "Bans a users IP address (ma_banip <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBan
					(
					0, // Client index
					true,  // Sever console command type
					true, // By IP
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option minutes
					engine->Cmd_Argv(2) // Command option player
					);
	return;
}

CON_COMMAND(ma_unban, "Unbans a user (steam id or ip address)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnBan
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1) // Command option
					);
	return;
}

CON_COMMAND(ma_slay, "Slays a user (ma_slay <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaSlay
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1) // Command option
					);
	return;
}

CON_COMMAND(ma_slap, "Slaps a user (ma_slap <partial user name, user id or steam id> <damage amount>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaSlap
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Amount of damage
					);
	return;
}

CON_COMMAND(ma_setcash, "Sets a players cash (ma_setcash <player name | partial name | user id | steam id> <cash amount>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCash
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_SET_CASH
					);
	return;
}

CON_COMMAND(ma_givecash, "Gives a players cash (ma_givecash <player name | partial name | user id | steam id> <cash amount to give>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCash
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_GIVE_CASH
					);
	return;
}

CON_COMMAND(ma_givecashp, "Sets a players cash (ma_givecashp <player name | partial name | user id | steam id> <cash percent to add>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCash
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_GIVE_CASH_PERCENT
					);
	return;
}

CON_COMMAND(ma_takecash, "Takes a players cash (ma_takecash <player name | partial name | user id | steam id> <cash amount to take>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCash
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_TAKE_CASH
					);
	return;
}

CON_COMMAND(ma_takecashp, "Sets a players cash (ma_takecashp <player name | partial name | user id | steam id> <cash percent to take>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCash
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_TAKE_CASH_PERCENT
					);
	return;
}

// Set Health

CON_COMMAND(ma_sethealth, "Sets a players health (ma_sethealth <player name | partial name | user id | steam id> <cash amount>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHealth
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_SET_HEALTH
					);
	return;
}

CON_COMMAND(ma_givehealth, "Gives a players cash (ma_givehealth <player name | partial name | user id | steam id> <health amount to give>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHealth
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_GIVE_HEALTH
					);
	return;
}

CON_COMMAND(ma_givehealthp, "Sets a players health (ma_givehealthp <player name | partial name | user id | steam id> <health percent to add>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHealth
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_GIVE_HEALTH_PERCENT
					);
	return;
}

CON_COMMAND(ma_takehealth, "Takes a players cash (ma_takehealth <player name | partial name | user id | steam id> <health amount to take>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHealth
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_TAKE_HEALTH
					);
	return;
}

CON_COMMAND(ma_takehealthp, "Sets a players health (ma_takehealthp <player name | partial name | user id | steam id> <health percent to take>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHealth
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target player(s)
					engine->Cmd_Argv(2), // Amount of cash
					MANI_TAKE_HEALTH_PERCENT
					);
	return;
}



CON_COMMAND(ma_blind, "Blinds a user (ma_blind <partial user name, user id or steam id> <blind amount>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBlind
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Amount of blindness
					);
	return;
}

CON_COMMAND(ma_freeze, "Freezes a user (ma_freeze <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaFreeze
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_noclip, "Gives a player immortality and no clip (ma_clip <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaNoClip
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1) // Command option
					);
	return;
}
CON_COMMAND(ma_burn, "Burns a user (ma_burn <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBurn
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1) // Command option
					);
	return;
}

CON_COMMAND(ma_drug, "Drugs a user (ma_drug <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaDrug
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_give, "Gives a user an item (ma_give <partial user name, user id or steam id> <item name>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaGive
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_giveammo, "Gives a user ammo (ma_giveammo <partial user name, user id or steam id> <weapon slot> <primary fire ammo 0 = alt, 1 = primary> <amount> <optional suppress sound 0 = no, 1 = yes>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaGiveAmmo
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2), // Command option
					engine->Cmd_Argv(3), // Command option
					engine->Cmd_Argv(4), // Command option
					engine->Cmd_Argv(5) // Command option
					);
	return;
}

CON_COMMAND(ma_colour, "Sets a players colour (ma_colour <partial user name, user id or steam id> <red 0-255> <green 0-255> <blue 0-255> <alpha 0-255>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaColour
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target Player
					engine->Cmd_Argv(2), // Command red
					engine->Cmd_Argv(3), // Command green
					engine->Cmd_Argv(4), // Command blue
					engine->Cmd_Argv(5) // Command alpha
					);
	return;
}

CON_COMMAND(ma_render, "Sets a players render mode (ma_render <partial user name, user id or steam id> <render mode, 0 onwards)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaRenderMode
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target Player
					engine->Cmd_Argv(2) // Render mode
					);
	return;
}

CON_COMMAND(ma_renderfx, "Sets a players render fx (ma_render <partial user name, user id or steam id> <render fx, 0 onwards)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaRenderFX
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target Player
					engine->Cmd_Argv(2) // Render mode
					);
	return;
}

CON_COMMAND(ma_color, "Sets a players color (ma_color <partial user name, user id or steam id> <red 0-255> <green 0-255> <blue 0-255> <alpha 0-255>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaColour
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Target Player
					engine->Cmd_Argv(2), // Command red
					engine->Cmd_Argv(3), // Command green
					engine->Cmd_Argv(4), // Command blue
					engine->Cmd_Argv(5) // Command alpha
					);
	return;
}

CON_COMMAND(ma_gimp, "Gimps a user (ma_gimp <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaGimp
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_timebomb, "Turns a player into a time bomb (ma_timebomb <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaTimeBomb
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_firebomb, "Turns a player into a fire bomb (ma_firebomb <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaFireBomb
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}
CON_COMMAND(ma_freezebomb, "Turns a player into a freeze bomb (ma_freezebomb <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaFreezeBomb
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_beacon, "Turns a player into a beacon (ma_beacon <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBeacon
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_mute, "Mutes a user (ma_mute <partial user name, user id or steam id>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaMute
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1), // Command option
					engine->Cmd_Argv(2) // Command option
					);
	return;
}

CON_COMMAND(ma_teleport, "Teleports a player (ma_teleport <partial user name, user id or steam id> <x> <y> <z>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaTeleport
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command argument
					engine->Cmd_Argv(1),
					engine->Cmd_Argv(2),
					engine->Cmd_Argv(3),
					engine->Cmd_Argv(4)
					);
	return;
}

CON_COMMAND(ma_saveloc, "Dummy command for ma_saveloc")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg("You can't run ma_saveloc from the server console !\n");
	return;
}

CON_COMMAND(ma_psay, "ma_psay (<partial user name, user id or steam id> <message>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaPSay
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					say_argv[0].argv_string, // The player target string
					&(trimmed_say[say_argv[1].index]) // The actual say string
					);
	return;
}

CON_COMMAND(ma_msay, "ma_msay (<time 0 = permanent> <partial user name, user id or steam id> <message>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaMSay
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					say_argv[0].argv_string, // The player target string
					say_argv[1].argv_string, // The player target string
					&(trimmed_say[say_argv[2].index]) // The actual say string
					);
	return;
}

CON_COMMAND(ma_say, "ma_say <message>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaSay
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					&(trimmed_say[say_argv[0].index]) // The actual say string
					);
	return;
}

CON_COMMAND(ma_csay, "ma_csay <message>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;

	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaCSay
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					&(trimmed_say[say_argv[0].index]) // The actual say string
					);
	return;
}

CON_COMMAND(ma_chat, "ma_chat <message>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;

	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaChat
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					&(trimmed_say[say_argv[0].index]) // The actual say string
					);
	return;
}
//***************************************************************************************
// Autoban using ban command
CON_COMMAND(ma_aban_name, "Adds a user to the autoban list using name, ma_aban_name \"name\" <ban time>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					engine->Cmd_Argv(2), // Ban time
					false // Ban only
					);
	return;
}

CON_COMMAND(ma_aban_pname, "Adds a user to the autoban list using partial name, ma_aban_pname \"name\" <ban time>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanPName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					engine->Cmd_Argv(2), // Ban time
					false // Ban only
					);
	return;
}

// Autokick by kick
CON_COMMAND(ma_akick_name, "Adds a user to the autokick list using name, ma_akick_name \"name\"")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					0, // Ban Time default to 0
					true // Kick only
					);
	return;
}

CON_COMMAND(ma_akick_pname, "Adds a user to the autokick list using partial name, ma_akick_pname \"name\"")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanPName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					0, // Ban Time default to 0
					true // Kick only
					);
	return;
}

CON_COMMAND(ma_akick_steam, "Adds a user to the autokick list using steam id, ma_akick_steam <steam_id>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickSteam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // steam id
					);
	return;
}

CON_COMMAND(ma_akick_ip, "Adds a user to the autokick list using ip address, ma_akick_ip <ip address>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickIP
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // ip address
					);
	return;
}

//***************************************************************************************
// UnAutoban using ban command
CON_COMMAND(ma_unaban_name, "Removes a user from the autoban list using name, ma_unaban_name \"name\" <ban time>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickBanName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					false // Ban only
					);
	return;
}

CON_COMMAND(ma_unaban_pname, "Removes a user from the autoban list using partial name, ma_unaban_pname \"name\" <ban time>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickBanPName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					false // Ban only
					);
	return;
}

// Autokick by kick
CON_COMMAND(ma_unakick_name, "Removes a user from the autokick list using name, ma_unakick_name \"name\"")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickBanName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					true // Kick only
					);
	return;
}

CON_COMMAND(ma_unakick_pname, "Removes a user from the autokick list using partial name, ma_unakick_pname \"name\"")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickBanPName
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Player name
					true // Kick only
					);
	return;
}

CON_COMMAND(ma_unakick_steam, "Removes a user from the autoban list using steam id, ma_unakick_steam <steam_id>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickSteam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // steam id
					);
	return;
}

CON_COMMAND(ma_unakick_ip, "Removes a user from the autoban list using ip address, ma_unakick_ip <ip address>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUnAutoKickIP
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // ip address
					);
	return;
}



CON_COMMAND(ma_ashow_name, "Shows autoban/kick info for IP addresses, ma_ashow_name")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanShowName(0, true);
	return;
}

CON_COMMAND(ma_ashow_pname, "Shows autoban/kick info for partial names, ma_ashow_pname")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanShowPName(0, true);
	return;
}

CON_COMMAND(ma_ashow_steam, "Shows autoban/kick info for steam ids, ma_ashow_steam")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanShowSteam(0, true);
	return;
}

CON_COMMAND(ma_ashow_ip, "Shows autoban/kick info for IP addresses, ma_ashow_ip")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAutoKickBanShowIP(0, true);
	return;
}

/**************************************************************/
/* Client command execute list */

CON_COMMAND(ma_cexec_all, "Runs a client command on all players")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCExecAll
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					(char *) engine->Cmd_Args() // command to execute
					);
	return;
}

CON_COMMAND(ma_cexec_ct, "Runs a client command on all counter terrorist players")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCExecTeam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					(char *) engine->Cmd_Args(), // command to execute
					TEAM_B
					);
	return;
}
CON_COMMAND(ma_cexec_t, "Runs a client command on all terrorist players")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaCExecTeam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					(char *) engine->Cmd_Args(), // command to execute
					TEAM_A
					);
	return;
}

CON_COMMAND(ma_cexec_spec, "Runs a client command on all spectator players")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	if (!gpManiGameType->IsSpectatorAllowed()) return;

	g_ManiAdminPlugin.ProcessMaCExecTeam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					(char *) engine->Cmd_Args(), // command to execute
					gpManiGameType->GetSpectatorIndex()
					);
	return;
}

CON_COMMAND(ma_cexec, "ma_cexec (<partial user name, user id or steam id> <client command>)")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	char	trimmed_say[2048];
	int say_argc;

	if (ProcessPluginPaused()) return;

	g_ManiAdminPlugin.ParseSayString(engine->Cmd_Args(), trimmed_say, &say_argc);
	g_ManiAdminPlugin.ProcessMaCExec
					(
					0, 
					true, 
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // The command executed
					say_argv[0].argv_string, // The player target string
					&(trimmed_say[say_argv[1].index]) // The actual client string to execute
					);
	return;
}
//***************************************************************************************

CON_COMMAND(ma_help, "Prints all the mani admin plugin consol commands")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaHelp(0, true, engine->Cmd_Argc(), engine->Cmd_Argv(1));
	return;
}

CON_COMMAND(ma_users, "Prints user details")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaUsers
					(
					0,	
					true,
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // target players
					);
	return;
}

CON_COMMAND(ma_rates, "Prints user details with their rates")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaRates
					(
					0,	
					true,
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // target players
					);

	return;
}

CON_COMMAND(ma_showsounds, "Prints all sounds in soundlist.txt")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaShowSounds(0, true);
	return;
}

CON_COMMAND(ma_voterandom, "Starts a random votemap")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVoteRandom
					(
					0, // Client index
					true,  // Sever console command type
					false, // Say string command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // delay type
					engine->Cmd_Argv(2)  // Number of maps
					);
	return;
}

CON_COMMAND(ma_voteextend, "Starts an extend vote")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVoteExtend
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0)
					);
	return;
}

CON_COMMAND(ma_votercon, "Starts a rcon vote")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVoteRCon
					(
					0, // Client index
					true,  // Sever console command type
					false, // Say string command type
					false, // From menu selection
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // question
					engine->Cmd_Argv(2)  // rcon command
					);
	return;
}

CON_COMMAND(ma_vote, "Starts a map vote")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVote
					(
					0, // Client index
					true,  // Sever console command type
					false, // Say string command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // delay type
					engine->Cmd_Argv(2),  // Map 1
					engine->Cmd_Argv(3),  // Map 2
					engine->Cmd_Argv(4),  // Map 3
					engine->Cmd_Argv(5),  // Map 4
					engine->Cmd_Argv(6),  // Map 5
					engine->Cmd_Argv(7),  // Map 6
					engine->Cmd_Argv(8),  // Map 7
					engine->Cmd_Argv(9),  // Map 8
					engine->Cmd_Argv(10),  // Map 9
					engine->Cmd_Argv(11)  // Map 10
					);
	return;
}

CON_COMMAND(ma_votequestion, "Starts a question vote")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVoteQuestion
					(
					0, // Client index
					true,  // Sever console command type
					false, // Say string command type
					false, // From menu
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1), // Question
					engine->Cmd_Argv(2),  // Answer 1
					engine->Cmd_Argv(3),  // Answer 2
					engine->Cmd_Argv(4),  // Answer 3
					engine->Cmd_Argv(5),  // Answer 4
					engine->Cmd_Argv(6),  // Answer 5
					engine->Cmd_Argv(7),  // Answer 6
					engine->Cmd_Argv(8),  // Answer 7
					engine->Cmd_Argv(9),  // Answer 8
					engine->Cmd_Argv(10),  // Answer 9
					engine->Cmd_Argv(11)  // Answer 10
					);
	return;
}

CON_COMMAND(ma_votecancel, "Cancels current vote")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaVoteCancel
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0) // Command String
					);
	return;
}

CON_COMMAND(ma_play, "Plays a sound, ma_play <sound index or partial sound name>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	ProcessMaPlaySound
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // sound name
					);
	return;
}



CON_COMMAND(ma_swapteam, "Swaps a player to the opposite team, ma_swapteam <partial user name, user id or steam id>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaSwapTeam
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // player name
					);
	return;
}

CON_COMMAND(ma_spec, "Moves a player to be spectator, ma_spec <partial user name, user id or steam id>")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaSpec
					(
					0, // Client index
					true,  // Sever console command type
					engine->Cmd_Argc(), // Number of arguments
					engine->Cmd_Argv(0), // Command
					engine->Cmd_Argv(1) // player name
					);
	return;
}


CON_COMMAND(ma_balance, "Balances teams taking into account mp_limitteams")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaBalance(0,true, false);
	return;
}

CON_COMMAND(ma_dropc4, "In CSS this forces the c4 carrier to drop it")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaDropC4(0,true);
	return;
}

CON_COMMAND(ma_rcon, "Dummy ma_rcon command as it can't be run from server")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg("You have server access you don't need ma_rcon !!\n");
	return;
}

CON_COMMAND(ma_explode, "Dummy ma_explode command as it can't be run from server")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg("You can't run this from the server console !!\n");
	return;
}



CON_COMMAND(ma_war, "Enables/Disables war mode")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaWar(0,true, engine->Cmd_Argc(), engine->Cmd_Argv(1));
	return;
}

CON_COMMAND(ma_settings, "Dummy ma_settings command as it can't be run from server")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg("You can't run this from the server console !!\n");
	return;
}

CON_COMMAND(ma_config, "Prints the current configuration")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaConfig(0,true, engine->Cmd_Argc(), engine->Cmd_Argv(1));
	return;
}

CON_COMMAND(ma_admins, "Prints the admins and their rights for this server")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAdmins(0, true);
	return;
}

CON_COMMAND(ma_admingroups, "Prints the admins groups and their settings")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaAdminGroups(0, true);
	return;
}

CON_COMMAND(ma_immunity, "Prints the immunity users and their rights for this server")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaImmunity(0, true);
	return;
}

CON_COMMAND(ma_immunitygroups, "Prints the immunity groups and their settings")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaImmunityGroups(0, true);
	return;
}

CON_COMMAND(maniadminversion, "Prints the version of the plugin")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg( "%s\n", mani_version );
	return;
}

CON_COMMAND(ma_version, "Prints the version of the plugin")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	Msg( "%s\n", mani_version );
	Msg( "Server Tickrate %i\n", server_tickrate);

	return;
}

CON_COMMAND(ma_game, "Prints the game type in use")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	const char *game_type = serverdll->GetGameDescription();

	Msg( "Game Type = [%s]\n", game_type );

	return;
}

CON_COMMAND(ma_timeleft, "Prints timeleft information")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
	g_ManiAdminPlugin.ProcessMaTimeLeft(0, true);
	return;
}

static void WarModeChanged ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (atoi(var->GetString()) == 0)
		{
			war_mode = false;
		}
		else
		{
			war_mode = true;
		}
	}
}

static void ManiAdminPluginVersion ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		mani_admin_plugin_version.SetValue(PLUGIN_CORE_VERSION);
	}
}

static void ManiTickrate ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		mani_tickrate.SetValue(server_tickrate);
	}
}

static void ManiCSStackingNumLevels ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		if (cs_stacking_num_levels)
		{
			cs_stacking_num_levels->m_fMaxVal = var->GetFloat();
			cs_stacking_num_levels->SetValue(var->GetInt());
		}
	}
}

static void ManiUnlimitedGrenades ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()) && 
		gpManiGameType->IsGameType(MANI_GAME_CSS) && 
		sv_cheats &&
		!war_mode)
	{
		if (var->GetInt() == 1)
		{
			SayToAll(false, "Unlimited grenades enabled !!");
			for (int i = 1; i <= max_players; i++)
			{
				player_t player;

				player.index = i;
				if (!FindPlayerByIndex(&player)) continue;
				if (player.is_dead) continue;
				if (player.team == gpManiGameType->GetSpectatorIndex()) continue;

				if (sv_cheats->GetInt() != 0)
				{
					helpers->ClientCommand(player.entity,"give weapon_hegrenade\n");
				}
				else
				{
					sv_cheats->SetValue(1);
					helpers->ClientCommand(player.entity,"give weapon_hegrenade\n");
					sv_cheats->SetValue(0);
				}
			}
		}
		else
		{
			SayToAll(false, "Unlimited grenades disabled");
		}
	}
}

static void HighPingKick ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
		{
			average_ping_list[i].in_use = false;
		}
	}

	next_ping_check = 0.0;
}

static void HighPingKickSamples ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
		{
			average_ping_list[i].in_use = false;
		}
	}

	next_ping_check = 0.0;
}

static void ManiStatsBySteamID ( ConVar *var, char const *pOldString )
{
	player_t	player;

	// Reset player settings
	ResetActivePlayers();
	// Reload players that are on server for change of mode
	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		// Don't add player if steam id is not confirmed
		if (var->GetInt() == 1 && FStrEq(player.steam_id, "STEAM_ID_PENDING")) continue;
		
		if (var->GetInt() == 1)
		{
			AddPlayerToRankList(&player);
		}
		else
		{
			AddPlayerNameToRankList(&player);
		}
	}
}

static int sort_reserve_slots_by_steam_id ( const void *m1,  const void *m2) 
{
	struct reserve_slot_t *mi1 = (struct reserve_slot_t *) m1;
	struct reserve_slot_t *mi2 = (struct reserve_slot_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_ping_immunity_by_steam_id ( const void *m1,  const void *m2) 
{
	struct ping_immunity_t *mi1 = (struct ping_immunity_t *) m1;
	struct ping_immunity_t *mi2 = (struct ping_immunity_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_active_players_by_ping ( const void *m1,  const void *m2) 
{
	struct active_player_t *mi1 = (struct active_player_t *) m1;
	struct active_player_t *mi2 = (struct active_player_t *) m2;

	if (mi1->is_spectator == true && mi2->is_spectator == false)
	{
		return -1;
	}

	if (mi1->is_spectator == false && mi2->is_spectator == true)
	{
		return 1;
	}

	if (mi1->ping > mi2->ping)
	{
		return -1;
	}
	else if (mi1->ping < mi2->ping)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);

}

static int sort_active_players_by_connect_time ( const void *m1,  const void *m2) 
{
	struct active_player_t *mi1 = (struct active_player_t *) m1;
	struct active_player_t *mi2 = (struct active_player_t *) m2;

	if (mi1->is_spectator == true && mi2->is_spectator == false)
	{
		return -1;
	}

	if (mi1->is_spectator == false && mi2->is_spectator == true)
	{
		return 1;
	}

	if (mi1->time_connected < mi2->time_connected)
	{
		return -1;
	}
	else if (mi1->time_connected > mi2->time_connected)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);

}

// Functions for autokick 
static int sort_autokick_steam ( const void *m1,  const void *m2) 
{
	struct autokick_steam_t *mi1 = (struct autokick_steam_t *) m1;
	struct autokick_steam_t *mi2 = (struct autokick_steam_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_autokick_ip ( const void *m1,  const void *m2) 
{
	struct autokick_ip_t *mi1 = (struct autokick_ip_t *) m1;
	struct autokick_ip_t *mi2 = (struct autokick_ip_t *) m2;
	return strcmp(mi1->ip_address, mi2->ip_address);
}


static int sort_nominations_by_votes_cast ( const void *m1,  const void *m2) 
{
	struct vote_option_t *mi1 = (struct vote_option_t *) m1;
	struct vote_option_t *mi2 = (struct vote_option_t *) m2;
	return (mi2->votes_cast - mi1->votes_cast);
}

//**************************************************************************************************
// Special hook for say commands
//**************************************************************************************************

class CSayHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLSayCommand;
public:
   CSayHook() : ConCommand("say", NULL, "say messages", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         Msg("Didn't find say command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLSayCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say
      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookSayCommand()) return;
      // Forward to gamedll
      m_pGameDLLSayCommand->Dispatch();
   }
};

CSayHook g_SayHook;

//**************************************************************************************************
// Special hook for say commands
//**************************************************************************************************

class CSayTeamHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLSayCommand;
public:
   CSayTeamHook() : ConCommand("say_team", NULL, "say messages to teammates", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say_team") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         Msg("Didn't find say_team command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLSayCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookSayCommand()) return;
      // Forward to gamedll
      m_pGameDLLSayCommand->Dispatch();
   }
};

CSayTeamHook g_SayTeamHook;

//**************************************************************************************************
// Special hook for autobuy commands
//**************************************************************************************************

class CAutobuyHook : public ConCommand
{
   // This will hold the pointer original gamedll autobuy command
   ConCommand *m_pGameDLLAutobuyCommand;
public:
   CAutobuyHook() : ConCommand("autobuy", NULL, "autobuy", FCVAR_GAMEDLL), m_pGameDLLAutobuyCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll autobuy command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "autobuy") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }

      if (!pPtr)
	  {
         Msg("Didn't find autobuy command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLAutobuyCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the autobuy

      if(!ProcessPluginPaused() && !HookAutobuyCommand()) return;
      // Forward to gamedll
      m_pGameDLLAutobuyCommand->Dispatch();
   }
};

CAutobuyHook g_AutobuyHook;

//**************************************************************************************************
// Special hook for rebuy commands
//**************************************************************************************************
class CRebuyHook : public ConCommand
{
   // This will hold the pointer original gamedll rebuy command
   ConCommand *m_pGameDLLRebuyCommand;
public:
   CRebuyHook() : ConCommand("rebuy", NULL, "rebuy", FCVAR_GAMEDLL), m_pGameDLLRebuyCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll rebuy command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "rebuy") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }

      if (!pPtr)
	  {
         Msg("Didn't find rebuy command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLRebuyCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !HookRebuyCommand()) return;
      // Forward to gamedll
      m_pGameDLLRebuyCommand->Dispatch();

	  if(!ProcessPluginPaused()) PostProcessRebuyCommand();
   }
};

CRebuyHook g_RebuyHook;



class CChangeLevelHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLChangeLevelCommand;
public:
   CChangeLevelHook() : ConCommand("changelevel", NULL, "changelevel", FCVAR_GAMEDLL), m_pGameDLLChangeLevelCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "changelevel") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         Msg("Didn't find changelevel command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLChangeLevelCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
      // Do the normal stuff, return if you want to override the say

      if(!ProcessPluginPaused() && !g_ManiAdminPlugin.HookChangeLevelCommand()) return;
      // Forward to gamedll
      m_pGameDLLChangeLevelCommand->Dispatch();
   }
};

CChangeLevelHook g_ChangeLevelHook;

class CRespawnEntitiesHook : public ConCommand
{
   // This will hold the pointer original gamedll say command
   ConCommand *m_pGameDLLRespawnEntitiesCommand;
public:
   CRespawnEntitiesHook() : ConCommand("respawn_entities", NULL, "exploit fixed by mani", FCVAR_GAMEDLL), m_pGameDLLRespawnEntitiesCommand(NULL)
   { }

   // Override Init
   void Init()
   {
      // Try to find the gamedll say command
      ConCommandBase *pPtr = cvar->GetCommands();
      while (pPtr)
      {
         if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "respawn_entities") == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
      if (!pPtr)
	  {
         Msg("Didn't find respawn_entities command ptr!!!!\n");
		 return;
	  }

      m_pGameDLLRespawnEntitiesCommand = (ConCommand *) pPtr;

      // Call base class' init function
      ConCommand::Init();
   }

   void Dispatch()
   {
	   return;
   }
};

CRespawnEntitiesHook g_RespawnEntitiesHook;
