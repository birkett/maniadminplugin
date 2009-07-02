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



#ifndef MANI_COMMANDS_H
#define MANI_COMMANDS_H

enum
{
	M_CCONSOLE				= 0,
	M_SAY,
	M_TSAY,
	M_SCONSOLE,
	M_MENU
};

// Define a new con command that will directly call the gpCmd-> function
#define SCON_COMMAND(_name, _help_id, _func, _war_mode_allowed) \
   static void _name(); \
   static ConCommand _name##_command( #_name, _name, "Use ma_help _name for help" ); \
   static void _name() \
{ \
	if (!IsCommandIssuedByServerAdmin() || ProcessPluginPaused()) return; \
	gpCmd->ExtractClientAndServerCommand(); \
	gpCmd->_func(NULL, #_name, _help_id, M_SCONSOLE, _war_mode_allowed); \
}

class ManiCommands;

struct	cmd_t
{
	char	*cmd_name;
	int		help_id;
	bool	server_command;
	bool	client_command;
	bool	say_command;
	bool	tsay_command;
	bool	war_mode_allowed;
	bool	menu_command;
	bool	admin_required;
	int (ManiCommands::*funcPtr)(player_t *, const char *, const int, const int, const bool);
};

class ManiCommands
{

public:
	ManiCommands();
	~ManiCommands();

	void		Load(void); // Run at plugin load
	void		Unload(void); // Run at plugin unload
	void		GetCommandModes(const int help_id,  bool *server_console, bool *client_console, bool *client_chat, bool *war_allowed);
	void		ShowAllCommands( player_t *player_ptr,  bool	admin_flag );
	void		SearchCommands( player_t *player_ptr,  bool	admin_flag, const char *pattern );
	void		RegisterCommand(const char *name, int help_id, bool admin_required, bool war_mode_allowed, bool server_command, bool client_command, bool say_command, bool tsay_command, int (ManiCommands::*funcPtr)(player_t *, const char *, int, int, bool));
	void		RegisterCommand(const char *name, bool war_mode_allowed, int (ManiCommands::*funcPtr)(player_t *, const char *, int, int, bool));
	int			HandleCommand(player_t *player_ptr, int command_type);
	int			Cmd_Argc(void) {return cmd_argc;}
	const char *Cmd_Argv(int i) {return cmd_argv[i];}
	const char *Cmd_Args(void) {return cmd_argv_cat[1];}
	const char *Cmd_Args(int i) {return cmd_argv_cat[i];}
	const char *Cmd_SayArg0(void) {return say_argv0;}
	void		ExtractSayCommand(bool team_say);
	void		ParseSayCommand(void);
	void		ParseEventSayCommand(const char *player_say);
	void		ExtractClientAndServerCommand(void);
	void		NewCmd(void);
	void		AddParam(char *fmt, ...);
	void		AddParam(int number);
	void		AddParam(float number);
	void		DumpCommands(void);

#define COM_PROTO(_func) \
	int _func(player_t *player_ptr, const char *name, const int help_id, int command_type, bool war_mode_allowed);

	COM_PROTO(MaSay)
	COM_PROTO(MaMSay)
	COM_PROTO(MaPSay)
	COM_PROTO(MaChat)
	COM_PROTO(MaCSay)
	COM_PROTO(MaSession)
	COM_PROTO(MaStatsMe)
	COM_PROTO(MaRCon)
	COM_PROTO(MaBrowse)
	COM_PROTO(MaCExec)
	COM_PROTO(MaSlap)
	COM_PROTO(MaSetAdminFlag)
	COM_PROTO(MaSetSkin)
	COM_PROTO(MaSetCash)
	COM_PROTO(MaGiveCash)
	COM_PROTO(MaGiveCashP)
	COM_PROTO(MaTakeCash)
	COM_PROTO(MaTakeCashP)
	COM_PROTO(MaSetHealth)
	COM_PROTO(MaGiveHealth)
	COM_PROTO(MaGiveHealthP)
	COM_PROTO(MaTakeHealth)
	COM_PROTO(MaTakeHealthP)
	COM_PROTO(MaBlind)
	COM_PROTO(MaFreeze)
	COM_PROTO(MaNoClip)
	COM_PROTO(MaBurn)
	COM_PROTO(MaGravity)
	COM_PROTO(MaColour)
	COM_PROTO(MaColourWeapon)
	COM_PROTO(MaRenderMode)
	COM_PROTO(MaRenderFX)
	COM_PROTO(MaGive)
	COM_PROTO(MaGiveAmmo)
	COM_PROTO(MaDrug)
	COM_PROTO(MaDecal)
	COM_PROTO(MaGimp)
	COM_PROTO(MaTimeBomb)
	COM_PROTO(MaBeacon)
	COM_PROTO(MaFireBomb)
	COM_PROTO(MaFreezeBomb)
	COM_PROTO(MaMute)
	COM_PROTO(MaTeleport)
	COM_PROTO(MaPosition)
	COM_PROTO(MaSwapTeam)
	COM_PROTO(MaSpec)
	COM_PROTO(MaBalance)
	COM_PROTO(MaDropC4)
	COM_PROTO(MaSaveLoc)
	COM_PROTO(MaResetRank)
	COM_PROTO(MaMap)
	COM_PROTO(MaSkipMap)
	COM_PROTO(MaNextMap)
	COM_PROTO(MaListMaps)
	COM_PROTO(MaMapList)
	COM_PROTO(MaMapCycle)
	COM_PROTO(MaVoteMapList)
	COM_PROTO(MaWar)
	COM_PROTO(MaSetNextMap)
	COM_PROTO(MaVoteRandom)
	COM_PROTO(MaVoteExtend)
	COM_PROTO(MaVoteRCon)
	COM_PROTO(MaVote)
	COM_PROTO(MaVoteQuestion)
	COM_PROTO(MaVoteCancel)
	COM_PROTO(MaPlaySound)
	COM_PROTO(MaShowRestrict)
	COM_PROTO(MaRestrict)
	COM_PROTO(MaKnives)
	COM_PROTO(MaPistols)
	COM_PROTO(MaShotguns)
	COM_PROTO(MaNoSnipers)
	COM_PROTO(MaUnRestrict)
	COM_PROTO(MaUnRestrictAll)
	COM_PROTO(MaTKList)
	COM_PROTO(MaKick)
	COM_PROTO(MaChatTriggers)
	COM_PROTO(MaSpray)
	COM_PROTO(MaSlay)
	COM_PROTO(MaOffset)
	COM_PROTO(MaOffsetScan)
	COM_PROTO(MaOffsetScanF)
	COM_PROTO(MaTeamIndex)
	COM_PROTO(MaClient)
	COM_PROTO(MaClientGroup)
	COM_PROTO(MaReloadClients)
	COM_PROTO(MaBan)
	COM_PROTO(MaBanIP)
	COM_PROTO(MaUnBan)
	COM_PROTO(MaFavourites)

	COM_PROTO(MaRates)
	COM_PROTO(MaUsers)
	COM_PROTO(MaShowSounds)
	COM_PROTO(MaConfig)
	COM_PROTO(MaTimeLeft)
	
	COM_PROTO(MaAutoBanName)
	COM_PROTO(MaAutoBanPName)
	COM_PROTO(MaAutoKickName)
	COM_PROTO(MaAutoKickPName)
	COM_PROTO(MaAutoKickSteam)
	COM_PROTO(MaAutoKickIP)
	COM_PROTO(MaUnAutoName)
	COM_PROTO(MaUnAutoPName)
	COM_PROTO(MaUnAutoSteam)
	COM_PROTO(MaUnAutoIP)

	COM_PROTO(MaAutoShowName)
	COM_PROTO(MaAutoShowPName)
	COM_PROTO(MaAutoShowSteam)
	COM_PROTO(MaAutoShowIP)

	COM_PROTO(MaRanks)
	COM_PROTO(MaPLRanks)
	COM_PROTO(MaHelp)

private:

	void		CleanUp(void);
	void		AddStringParam(char *string, int length);

	cmd_t		*cmd_list;
	int			cmd_list_size;

	int			cmd_argc; // Argument count
	char		*cmd_argv[80]; // Each argument 
	char		*cmd_argv_cat[80]; // Each argument with all other following arguments concatenated on the end
	char		*cmd_args; // Just the basic engine->Cmd_Args();
	char		say_argv0[2048];

	// 4k buffer for the arguments
	char		temp_string1[2048]; // Contains args similar to args
	int			current_marker1;
	char		temp_string2[2048]; // Contains args but each arg is split by nulls
	int			current_marker2;
};

extern	ManiCommands *gpCmd;

#endif
