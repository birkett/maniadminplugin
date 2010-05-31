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



#ifndef MANI_QUAKE_H
#define MANI_QUAKE_H

class ConVar;

enum {
MANI_QUAKE_SOUND_FIRSTBLOOD	= 0,
MANI_QUAKE_SOUND_HUMILIATION,
MANI_QUAKE_SOUND_MULTIKILL,	
MANI_QUAKE_SOUND_MONSTERKILL,
MANI_QUAKE_SOUND_ULTRAKILL,	
MANI_QUAKE_SOUND_GODLIKE,	
MANI_QUAKE_SOUND_HEADSHOT,	
MANI_QUAKE_SOUND_DOMINATING,
MANI_QUAKE_SOUND_HOLYSHIT,
MANI_QUAKE_SOUND_KILLINGSPREE,
MANI_QUAKE_SOUND_LUDICROUSKILL,
MANI_QUAKE_SOUND_PREPARE,
MANI_QUAKE_SOUND_RAMPAGE,
MANI_QUAKE_SOUND_UNSTOPPABLE,
MANI_QUAKE_SOUND_WICKEDSICK,
MANI_QUAKE_SOUND_TEAMKILLER,
MANI_MAX_QUAKE_SOUNDS
};

struct track_kill_t
{
	int	kill_streak;
	float last_kill;
};

struct	quake_sound_t
{
	char	sound_file[512];
	char	alias[512];
	bool	in_use;
};

class IGameEvent;

extern	void	LoadQuakeSounds(void);
extern	void	ResetQuakeDeaths(void);
extern	void	ProcessQuakeRoundStart(void);
extern	void	ProcessQuakeDeath(IGameEvent * event);

extern ConVar mani_quake_sounds;


#endif
