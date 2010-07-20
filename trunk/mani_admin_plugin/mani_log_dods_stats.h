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



#ifndef MANI_LOG_DODS_STATS_H
#define MANI_LOG_DODS_STATS_H

const int MANI_MAX_LOG_DODS_HITGROUPS = 11;
const int MANI_MAX_LOG_DODS_WEAPONS = 25;

class ManiLogDODSStats
{

public:
	ManiLogDODSStats();
	~ManiLogDODSStats();

	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t *player_ptr);
	void		Load(void);
	void		LevelInit(void);
	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists, int weapon_index );
	void		PlayerHurt( player_t *victim_ptr,  player_t *attacker_ptr,  IGameEvent * event);
	void		PlayerSpawn(player_t *player_ptr);
	void		PlayerFired(int index, int weapon_index);
	void		PointCaptured(const char *cappers, int cappers_length, const char *cp_name);
	void		CaptureBlocked(player_t *player_ptr, const char *cp_name);
	void		RoundEnd(void);

private:

	struct weapon_stats_t
	{
		bool dump;
		char weapon_name[128];
		int	total_shots_fired;
		int total_shots_hit;
		int	total_kills;
		int total_headshots;
		int total_team_kills;
		int total_damage;
		int total_deaths;
		float last_hit_time;
		int	hit_groups[MANI_MAX_LOG_DODS_HITGROUPS];
		bool last_hit_headshot;
	};

	struct player_info_t
	{
		char name[MAX_PLAYER_NAME_LENGTH];
		char steam_id[MAX_NETWORKID_LENGTH];
		int	 user_id;
		int	 team;
		weapon_stats_t weapon_stats_list[MANI_MAX_LOG_DODS_WEAPONS];
	};

	player_info_t	player_stats_list[MANI_MAX_PLAYERS];
	bool	level_ended;

	void	ResetPlayerStats(int index);
	void	ResetStats(void);
	void	UpdatePlayerIDInfo(player_t *player_ptr, bool reset_stats);
	void	DumpPlayerStats(int index);

	void	AddHitGroup(int hits, char *final_string, char *bodypart);
	void	InitStats(void);
};

extern	ManiLogDODSStats *gpManiLogDODSStats;

#endif
