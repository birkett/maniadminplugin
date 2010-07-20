//
// Mani Admin Plugin
//
// Copyright � 2009-2010 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_SOUNDS_H
#define MANI_SOUNDS_H

#define MANI_MAX_ACTION_SOUNDS (6)

#define MANI_ACTION_SOUND_JOINSERVER	(0)
#define MANI_ACTION_SOUND_VOTESTART		(1)
#define MANI_ACTION_SOUND_VOTEEND		(2)
#define MANI_ACTION_SOUND_ROUNDSTART	(3)
#define MANI_ACTION_SOUND_ROUNDEND		(4)
#define MANI_ACTION_SOUND_RESTRICTWEAPON (5)

#include "mani_menu.h"

struct	sound_t
{
	char	sound_name[512];
	char	alias[512];
};

struct	action_sound_t
{
	char	sound_file[512];
	char	alias[512];
	bool	in_use;
};

extern	int		sounds_played[];
extern	void	LoadSounds(void);
extern	void	FreeSounds(void);
extern	PLUGIN_RESULT	ProcessMaShowSounds(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaPlaySound(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	void			ProcessPlayActionSound (player_t *target_player, int sound_id);
extern	void	ProcessPlayMenuSound( player_t *target_player_ptr, char *sound_name);

MENUALL_DEC(PlaySound);

#endif
