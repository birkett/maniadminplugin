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



#ifndef MANI_NETIDVALID_H
#define MANI_NETIDVALID_H

#define MANI_STEAM_PENDING ("STEAM_ID_PENDING")

#include <vector>

class ManiNetIDValid
{
public:
	ManiNetIDValid();
	~ManiNetIDValid();

	void		Load(void);
	void		LevelInit(void);
	void		ClientActive(player_t *player_ptr);
	void		GameFrame(void);
	void		ClientDisconnect(player_t *player_ptr);

private:
	void		CleanUp(void);
	void		NetworkIDValidated( player_t *player_ptr );
	bool		TimeoutKick(player_t *player_ptr, time_t timeout);

	struct net_id_t
	{
		int	player_index;
		time_t	timer;
	};

	std::vector	<net_id_t> net_id_list;

	float		timeout;

};

extern	ManiNetIDValid *gpManiNetIDValid;

#endif
