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



#ifndef MANI_CSS_BOUNTY_H
#define MANI_CSS_BOUNTY_H

#include "mani_menu.h"

struct top_bounty_t
{
	char	name[MAX_PLAYER_NAME_LENGTH];
	int		bounty;
};

class BountyFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr);
	void	Redraw(player_t *player_ptr);
};

class ManiCSSBounty
{
	friend class BountyFreePage;
public:
	ManiCSSBounty();
	~ManiCSSBounty();

	void		ClientActive(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void);
	void		LevelInit(void);
	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists);
	void		PlayerSpawn(player_t *player_ptr);
	void		CSSRoundEnd(const char *message);
private:

	struct bounty_t
	{
		int	current_bounty;
		int	kill_streak;
	};

	bounty_t	bounty_list[MANI_MAX_PLAYERS];

	void	Reset(void);
	void	SetPlayerColour(player_t *player_ptr);
};

extern	ManiCSSBounty *gpManiCSSBounty;

#endif
