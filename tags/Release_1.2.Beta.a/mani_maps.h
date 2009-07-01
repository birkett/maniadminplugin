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



#ifndef MANI_MAPS_H
#define MANI_MAPS_H

#define	MANI_MAX_CHANGELEVEL_TRIES (50)

struct	map_t
{
	char	map_name[128];
	bool	selected_for_vote;
};

struct	track_map_t
{
	char	map_name[128];
	bool	played;
};

struct last_map_t
{
	char	map_name[128];
	bool	selected;
};

extern map_t	*map_list;
extern map_t	*votemap_list;
extern map_t	*map_in_cycle_list;
extern map_t	*map_not_in_cycle_list;
extern int		map_list_size;
extern int		votemap_list_size;
extern int		map_in_cycle_list_size;
extern int		map_not_in_cycle_list_size;
extern char		next_map[128];
extern char		current_map[128];
extern int		override_changelevel;
extern bool		override_setnextmap;
extern char		forced_nextmap[128];

extern	void InitMaps(void);
extern	void LoadMaps(const char *map_being_loaded);
extern	void FreeMaps(void);
extern	last_map_t	*GetLastMapsPlayed (int *number_of_maps_found, int max_number_of_maps);
extern	PLUGIN_RESULT	ProcessMaMapList( int index, bool svr_command);
extern	PLUGIN_RESULT	ProcessMaMapCycle( int index, bool svr_command);
extern	PLUGIN_RESULT	ProcessMaListMaps( int index, bool svr_command);
extern	PLUGIN_RESULT	ProcessMaNextMap( int index, bool svr_command);
extern	PLUGIN_RESULT	ProcessMaMap( int index,  bool svr_command,  int argc,  char *command_string,  char *map_name);
extern	PLUGIN_RESULT	ProcessMaSetNextMap( int index,  bool svr_command,  int argc,  char *command_string,  char *map_name);
extern	PLUGIN_RESULT	ProcessMaVoteMapList( int index,  bool svr_command);

extern ConVar mani_nextmap;

#endif