//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_CSS_BETTING_H
#define MANI_CSS_BETTING_H

#include "mani_menu.h"

class BetRulesFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr);
	void	Redraw(player_t *player_ptr);
};

class ManiCSSBetting
{
	friend class BetRulesFreePage;
public:
	ManiCSSBetting();
	~ManiCSSBetting();

	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void);
	void		LevelInit(void);
	void		PlayerDeath(void);
	void		PlayerBet(player_t *player_ptr);
	void		CSSRoundEnd(int winning_team);
private:

	void		GetAlivePlayerCount(int *t_count, int *t_index, int *ct_count, int *ct_index);
	void		PlayerNotAlive(void);

	struct bet_t
	{
		int		amount_bet;
		int		win_payout;
		int		winning_team;
	};

	bet_t	bet_list[MANI_MAX_PLAYERS];

	int		last_ct_index;
	int		last_t_index;
	int		versus_1;

	void	Reset(void);
	void	ResetPlayer(int index);
};

extern	ManiCSSBetting *gpManiCSSBetting;

#endif
