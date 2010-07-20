//
// Mani Admin Plugin
//
// Copyright © 2009-2010 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_GHOST_H
#define MANI_GHOST_H

class ManiGhost
{

public:
	ManiGhost();
	~ManiGhost();

	void		Init(void); // Run at level init
	void		ClientActive(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		PlayerDeath(player_t *player_ptr);
	void		RoundStart(void);
	bool		IsGhosting(player_t *player_ptr);

private:

	struct	current_ip_t
	{
		bool	in_use;
		bool	is_ghost;
		char	ip_address[128];
	};

	current_ip_t	current_ip_list[MANI_MAX_PLAYERS];
};

extern	ManiGhost *gpManiGhost;

#endif
