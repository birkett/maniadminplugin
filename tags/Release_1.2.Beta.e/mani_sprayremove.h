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



#ifndef MANI_SPRAYREMOVE_H
#define MANI_SPRAYREMOVE_H

class ManiSprayRemove
{
public:
	ManiSprayRemove();
	~ManiSprayRemove();

	void		Load(void);
	void		LevelInit(void);
	void		GameFrame(void);
	void		ClientDisconnect(player_t *player_ptr);
	bool		SprayFired(const Vector *pos, int	index);
	void		ManiSprayRemove::ProcessMaSprayMenu( player_t *admin_ptr, int admin_index, int next_index, int argv_offset, const char *menu_command);
	PLUGIN_RESULT	ManiSprayRemove::ProcessMaSpray( int index,  bool svr_command);

private:

	struct		spray_t
	{
		char	name[64];
		char	steam_id[64];
		int		user_id;
		bool	in_use;
		float	end_time;
		Vector	position;
	};

	spray_t		spray_list[MANI_MAX_PLAYERS];
	bool		check_list;
	float		game_timer;

	void		CleanUp(void);
	int			IsSprayValid(player_t	*player_ptr);

};

extern	ManiSprayRemove *gpManiSprayRemove;

#endif
