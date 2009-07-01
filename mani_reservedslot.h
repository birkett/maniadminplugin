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



#ifndef MANI_RESERVEDSLOT_H
#define MANI_RESERVEDSLOT_H


struct reserve_slot_t
{
	char	steam_id[128];
};

struct active_player_t
{
	edict_t	*entity;
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	int		user_id;
	float	ping;
	float	time_connected;
	bool	is_spectator;
	int		index;
};

struct disconnected_player_t
{
	bool	in_use;
};

extern	disconnected_player_t	disconnected_player_list[MANI_MAX_PLAYERS];

class ManiReservedSlot
{

public:
	ManiReservedSlot();
	~ManiReservedSlot();

	void		Load(void); // Run at level init
	void		LevelInit(void); // Run at level init
	bool		NetworkIDValidated(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);

private:

	void		BuildPlayerKickList(player_t *player_ptr, int *players_on_server);
	void		DisconnectPlayer(player_t *player_ptr);
	bool		IsPlayerInReserveList(player_t *player_ptr);
	void		CleanUp(void);

	int	active_player_list_size;
	active_player_t	*active_player_list;
	reserve_slot_t	*reserve_slot_list;
	int	reserve_slot_list_size;
};

extern	ManiReservedSlot *gpManiReservedSlot;

#endif
