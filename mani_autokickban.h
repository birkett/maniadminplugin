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



#ifndef MANI_AUTOKICKBAN_H
#define MANI_AUTOKICKBAN_H

class ManiAutoKickBan
{

public:
	ManiAutoKickBan();
	~ManiAutoKickBan();

	void		Load(void); // Run at level init
	void		LevelInit(void); // Run at level init
	bool		NetworkIDValidated(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		ProcessChangeName(player_t *player, const char *new_name, char *old_name);

	PLUGIN_RESULT	ProcessMaAutoKickBanName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_name, char *ban_time_string, bool kick);
	PLUGIN_RESULT	ProcessMaAutoKickBanPName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_pname, char *ban_time_string, bool kick);
	PLUGIN_RESULT	ProcessMaAutoKickSteam( int index,  bool svr_command,  int argc,  char *command_string,  char *player_steam_id);
	PLUGIN_RESULT	ProcessMaAutoKickIP( int index,  bool svr_command,  int argc,  char *command_string,  char *player_ip_address);
	PLUGIN_RESULT	ProcessMaUnAutoKickBanName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_name, bool kick);
	PLUGIN_RESULT	ProcessMaUnAutoKickBanPName( int index,  bool svr_command,  int argc,  char *command_string,  char *player_pname, bool kick);
	PLUGIN_RESULT	ProcessMaUnAutoKickSteam( int index,  bool svr_command,  int argc,  char *command_string,  char *player_steam_id);
	PLUGIN_RESULT	ProcessMaUnAutoKickIP( int index,  bool svr_command,  int argc,  char *command_string,  char *player_ip_address);
	PLUGIN_RESULT	ProcessMaAutoKickBanShowName( int index,  bool svr_command);
	PLUGIN_RESULT	ProcessMaAutoKickBanShowPName( int index,  bool svr_command);
	PLUGIN_RESULT	ProcessMaAutoKickBanShowSteam( int index,  bool svr_command);
	PLUGIN_RESULT	ProcessMaAutoKickBanShowIP( int index,  bool svr_command);
	void			ProcessAutoBanPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset );
	void			ProcessAutoKickPlayer( player_t *admin, const char *ban_command, int next_index, int argv_offset );

private:

	void		CleanUp(void);
	void			WriteNameList(char *filename_string);
	void			WritePNameList(char *filename_string);
	void			WriteIPList(char *filename_string);
	void			WriteSteamList(char *filename_string);
	void			AddAutoKickIP(char *details);
	void			AddAutoKickSteamID(char *details);
	void			AddAutoKickName(char *details);
	void			AddAutoKickPName(char *details);

	int	autokick_ip_list_size;
	int	autokick_steam_list_size;
	int	autokick_name_list_size;
	int	autokick_pname_list_size;

	autokick_ip_t		*autokick_ip_list;
	autokick_steam_t	*autokick_steam_list;
	autokick_name_t		*autokick_name_list;
	autokick_pname_t	*autokick_pname_list;
};

extern	ManiAutoKickBan *gpManiAutoKickBan;

#endif
