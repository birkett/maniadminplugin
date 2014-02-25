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



#ifndef MANI_TEAM_JOIN_H
#define MANI_TEAM_JOIN_H

struct saved_team_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	int		team_id;
};

class ManiTeamJoin
{

public:
	ManiTeamJoin();
	~ManiTeamJoin();

	void		PlayerTeamEvent(player_t *player_ptr);
	PLUGIN_RESULT	PlayerJoin(edict_t *pEntity, char *team_id);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);

private:

	bool ManiTeamJoin::IsPlayerInTeamJoinList(player_t *player_ptr, saved_team_t **saved_team_record);

	saved_team_t	*saved_team_list;
	int				saved_team_list_size;

	void	ResetTeamList(void);

};

extern	ManiTeamJoin *gpManiTeamJoin;

#endif
