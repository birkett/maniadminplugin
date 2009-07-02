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



#ifndef MANI_SAVE_SCORES_H
#define MANI_SAVE_SCORES_H

class ManiSaveScores
{

public:
	ManiSaveScores();
	~ManiSaveScores();

	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t *player_ptr);
	void		PlayerSpawn(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		GameCommencing(void);

private:

	struct save_scores_t
	{
		char	steam_id[MAX_NETWORKID_LENGTH];
		int		kills;
		int		deaths;
		int		cash;
		int		disconnection_time;
	};

	struct save_cash_t
	{
		int		cash;
		bool	trigger;
	};

	save_scores_t	*save_scores_list;
	int				save_scores_list_size;

	save_cash_t	 save_cash_list[MANI_MAX_PLAYERS];
	int			 spawn_count[MANI_MAX_PLAYERS];

	void	ResetScores(void);
};

extern	ManiSaveScores *gpManiSaveScores;

#endif
