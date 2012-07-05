//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_MESSAGEMODE_H
#define MANI_MESSAGEMODE_H

#include "mani_player.h"
#include "cbaseentity.h"

class ManiMessageMode
{
public:
	ManiMessageMode();
	~ManiMessageMode();

	void		NetworkIDValidated(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	PLUGIN_RESULT	ProcessMaPMess(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaExit(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);

	PLUGIN_RESULT	ProcessMaPSay(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaMSay(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSay(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaCSay(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaChat(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
private:

	void		CleanUp(void);
	void		DisableMessageMode(int index);

	struct	mess_mode_t
	{
		bool	admin_psay;
		bool	admin_psay_list[MANI_MAX_PLAYERS];
	};

	mess_mode_t	mess_mode_list[MANI_MAX_PLAYERS];
};

extern	ManiMessageMode *gpManiMessageMode;

#endif
