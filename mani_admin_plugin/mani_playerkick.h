//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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

#ifndef MANI_PLAYERKICK_H
#define MANI_PLAYERKICK_H


#include <string.h>
#include "utlvector.h"
#include "edict.h"

struct player_kick_t {

	player_kick_t() {
		player_index = 0;
		kick_time = 0.0f;
		memset(reason, 0, sizeof(reason));
	}

	int	player_index;
	float kick_time;
	char reason[1024];
};
extern CUtlVector<player_kick_t> kick_list;

class ManiPlayerKick {
public:
	void Init();
	void AddPlayer ( int player_index, float kick_time = 0.0f, const char *reason = NULL );
	void GameFrame ();
	void KickPlayer ( int player_index, const char *reason = NULL );
};

extern ManiPlayerKick *gpManiPlayerKick;

#endif // MANI_PLAYERKICK_H
