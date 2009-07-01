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

#define MANI_MAX_QUAKE_SOUNDS (15)

#define MANI_QUAKE_SOUND_FIRSTBLOOD		(0)
#define MANI_QUAKE_SOUND_HUMILIATION	(1)
#define MANI_QUAKE_SOUND_MULTIKILL		(2)
#define MANI_QUAKE_SOUND_MONSTERKILL	(3)
#define MANI_QUAKE_SOUND_ULTRAKILL		(4)
#define MANI_QUAKE_SOUND_GODLIKE		(5)
#define MANI_QUAKE_SOUND_HEADSHOT		(6)
#define MANI_QUAKE_SOUND_DOMINATING		(7)
#define MANI_QUAKE_SOUND_HOLYSHIT		(8)
#define MANI_QUAKE_SOUND_KILLINGSPREE	(9)
#define MANI_QUAKE_SOUND_LUDICROUSKILL	(10)
#define MANI_QUAKE_SOUND_PREPARE		(11)
#define MANI_QUAKE_SOUND_RAMPAGE		(12)
#define MANI_QUAKE_SOUND_UNSTOPPABLE	(13)
#define MANI_QUAKE_SOUND_WICKEDSICK		(14)

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

extern	void	LoadQuakeSounds(void);
extern	void	ResetQuakeDeaths(void);
extern	void	ProcessQuakeRoundStart(void);
extern	void	ProcessQuakeDeath(IGameEvent * event);

#endif
