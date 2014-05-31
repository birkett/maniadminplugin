//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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
	time_t	start_time;
	char	end_reason[128];
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
extern	void SetChangeLevelReason(const char *fmt, ...);
extern	PLUGIN_RESULT	ProcessMaMapList(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaMapHistory(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaMapCycle(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaListMaps(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaNextMap(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaMap(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaSkipMap(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaSetNextMap(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaVoteMapList(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);

extern ConVar mani_nextmap;

#endif
