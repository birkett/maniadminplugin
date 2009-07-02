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



#ifndef MANI_OBSERVER_TRACK_H
#define MANI_OBSERVER_TRACK_H

#include "mani_player.h"
#include "cbaseentity.h"

class ManiObserverTrack
{
public:
	MENUFRIEND_DEC(ObservePlayer);

	ManiObserverTrack();
	~ManiObserverTrack();

	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t	*player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		PlayerSpawn(player_t *player_ptr);
	PLUGIN_RESULT	ProcessMaObserve ( player_t *player_ptr, const char	*command_name, const int help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaEndObserve ( player_t *player_ptr, const char	*command_name, const int help_id, const int	command_type);
	void		PlayerDeath(player_t *player_ptr);

private:

	void		CleanUp();
	int			observer_id[MANI_MAX_PLAYERS];
	char		observer_steam[MANI_MAX_PLAYERS][MAX_NETWORKID_LENGTH];

};

MENUALL_DEC(ObservePlayer);

extern	ManiObserverTrack *gpManiObserverTrack;

#endif
