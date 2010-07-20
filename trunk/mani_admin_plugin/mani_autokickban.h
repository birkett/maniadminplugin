//
// Mani Admin Plugin
//
// Copyright © 2009-2010 Giles Millward (Mani). All rights reserved.
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

#include "mani_menu.h"

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

	PLUGIN_RESULT	ProcessMaAutoBanName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoKickName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoBanPName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoKickPName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoKickSteam(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoKickIP(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaUnAutoName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUnAutoPName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUnAutoSteam(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaUnAutoIP(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoShowName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoShowPName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoShowSteam(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaAutoShowIP(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);

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

MENUALL_DEC(AutoKick);
MENUALL_DEC(AutoBan);

#endif
