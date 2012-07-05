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



#ifndef MANI_MP_RESTARTGAME_H
#define MANI_MP_RESTARTGAME_H

#include "mani_player.h"
#include "cbaseentity.h"

class ManiMPRestartGame
{
public:
	ManiMPRestartGame();
	~ManiMPRestartGame();

	void		Load(void);
	void		Unload(void);
	void		CVarChanged(ConVar *cvar_ptr);
	void		LevelInit();
	void		LevelShutdown();
	void		GameFrame();

private:

	void		PreFire();
	void		Fire();
	void		PostFire();

	bool		check_timers;
	float		pre_fire_timeout;
	bool		pre_fire_check;
	float		on_time_timeout;
	bool		on_time_check;
	float		post_fire_timeout;
	bool		post_fire_check;
	
};

extern	ManiMPRestartGame *gpManiMPRestartGame;

#endif
