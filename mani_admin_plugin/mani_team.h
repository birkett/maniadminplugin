//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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
#include "mani_player.h"
#include "mani_menu.h"

class ManiTeam
{
public:
	MENUFRIEND_DEC(SwapPlayerD);

	ManiTeam();
	~ManiTeam();

	void		UnLoad(void);
	bool		IsOnSameTeam (player_t *victim, player_t *attacker);
	edict_t		*FindTeam(int team_index) {return team_list[team_index].edict_ptr;};
	const char *FindTeamName (int team_index) {return team_list[team_index].team_name;};
	int			GetTeamScore(int team_index);
	void		SetTeamScore(int team_index, int new_score);
	void		Init(int edict_count);
	void		GameFrame(void);
	PLUGIN_RESULT	ProcessMaSwapTeam(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSwapTeamD(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSpec(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaBalance (player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	bool			ProcessMaBalancePlayerType ( player_t	*player_ptr, bool mute_action, bool dead_only, bool dont_care);

	void			RoundEnd(void);
	void			TriggerSwapTeam(void);
	void			SwapWholeTeam();

private:

	void		CleanUp(void);
	void		ProcessDelayedSwap(void);

	struct team_t
	{
		edict_t	*edict_ptr;
		CTeam	*cteam_ptr;
		int		team_index;
		char	team_name[32];
	};

	team_t	team_list[20];
	bool		change_team;
	float		change_team_time;
	bool		swap_team;
	bool		pending_swap[MANI_MAX_PLAYERS];
	bool		delayed_swap;
};

extern	ManiTeam *gpManiTeam;

MENUALL_DEC(SwapPlayer);
MENUALL_DEC(SwapPlayerD);
MENUALL_DEC(SpecPlayer);

#endif
