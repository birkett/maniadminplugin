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



#ifndef MANI_PING_H
#define MANI_PING_H

struct ping_immunity_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
};

class ManiPing
{
public:
	ManiPing();
	~ManiPing();

	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		GameFrame(void);
	void		SetNextCheck(float next_check_time) {next_check = next_check;}

private:

	void		LoadImmunityList(void);
	void		ResetPlayer(int index);
	bool		IsPlayerImmune(player_t *player_ptr);

	struct ping_player_t
	{
		player_t	player;
		int			current_ping;
		bool		in_use;
	};

	struct ping_t
	{
		bool	check_ping;
		float	average_ping;
		int		count;
	};

	ping_immunity_t	*ping_immunity_list;
	int				ping_immunity_list_size;

	ping_t	ping_list[MANI_MAX_PLAYERS];
	ping_player_t	ping_player_list[MANI_MAX_PLAYERS];
	float	next_check;
};

extern	ManiPing *gpManiPing;

#endif
