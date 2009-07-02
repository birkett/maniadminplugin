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



#ifndef MANI_AFK_H
#define MANI_AFK_H

#include "mani_player.h"
#include "cbaseentity.h"

class ManiAFK
{
public:
	ManiAFK();
	~ManiAFK();

	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		GameCommencing(void);
	void		ProcessUsercmds(CBaseEntity *, CUserCmd *cmds, int numcmds);
	void		GameFrame(void);
	void		RoundEnd(void);
	void		LevelShutdown(void);
	void		NotAFK(int	index);

private:

	void		ResetPlayer(int index, bool check_player);

	struct afk_t
	{
		int	round_count;
		time_t	last_active;
		bool	check_player;
		bool	idle;
		bool	hooked;
	};

	afk_t	afk_list[MANI_MAX_PLAYERS];
	time_t	next_check;
	
};

extern	ManiAFK *gpManiAFK;

#endif
