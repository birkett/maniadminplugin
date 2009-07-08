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



#ifndef MANI_SPAWNPOINTS_H
#define MANI_SPAWNPOINTS_H

class ManiSpawnPoints
{

public:
	ManiSpawnPoints();
	~ManiSpawnPoints();

	void		Spawn(player_t *player_ptr);
	void		Load(char	*map_name);
	void		LevelInit(char	*map_name);
	bool		AddSpawnPoints(char **pReplaceEnts, const char *pMapEntities);
	void		LevelShutdown(void);

private:

	struct		spawn_vector_t
	{
		float	vx,vy,vz;
		float	ax,ay,az;
	};

	struct		spawn_team_t
	{
		spawn_vector_t	*spawn_list;
		int				spawn_list_size;
		int				last_spawn_index;
	};

	void		CleanUp(void);
	void		LoadData(char	*map_name);
	void		GetCoordList(KeyValues *kv_ptr, int team_number);
	bool		IsToClose(player_t *player_ptr);
	bool		DecodeString(char *input_string, spawn_vector_t *coord, int coord_index);

	// Handle up to 10 teams
	spawn_team_t	spawn_team[10];

	char		*replacement_entities;

};

extern	ManiSpawnPoints *gpManiSpawnPoints;

#endif
