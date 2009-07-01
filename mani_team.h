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



#ifndef MANI_TEAM_H
#define MANI_TEAM_H

#include "cbaseentity.h"

struct team_t
{
	CTeam	*team_ptr;
};

extern  CGameRules *gamerules_ptr;
extern	void FreeTeamList(void);
extern	void SetupTeamList(int edict_count);
extern	void SwitchTeam(player_t	*player_ptr);
extern	void ReSpawnPlayer(CBasePlayer *player_ptr);
extern	bool IsOnSameTeam (player_t *victim, player_t *attacker);

#endif