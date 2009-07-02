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



#ifndef MANI_LOG_CSS_STATS_H
#define MANI_LOG_CSS_STATS_H

const int MANI_MAX_CSS_HITGROUPS = 11;
const int MANI_MAX_CSS_WEAPONS = 28;

class ManiLogCSSStats
{

public:
	ManiLogCSSStats();
	~ManiLogCSSStats();

	void		ClientActive(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		NetworkIDValidated(player_t *player_ptr);
	void		Load(void);
	void		LevelInit(void);
	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists, bool headshot,  char *weapon_name );
	void		PlayerHurt( player_t *victim_ptr,  player_t *attacker_ptr,  IGameEvent * event);
	void		PlayerSpawn(player_t *player_ptr);
	void		PlayerFired(int index, char *weapon_name, bool is_bot);
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
		int	hit_groups[MANI_MAX_CSS_HITGROUPS];
	};

	struct player_info_t
	{
		char name[128];
		char steam_id[128];
		int	 user_id;
		int	 team;
		weapon_stats_t weapon_stats_list[MANI_MAX_CSS_WEAPONS];
	};

	player_info_t	player_stats_list[MANI_MAX_PLAYERS];
	bool	level_ended;

	void	ResetPlayerStats(int index);
	void	ResetStats(void);
	void	UpdatePlayerIDInfo(player_t *player_ptr, bool reset_stats);
	void	DumpPlayerStats(int index);

	int		FindWeapon(char *weapon_string);
	void	AddHitGroup(int hits, char *final_string, char *bodypart);
	void	InitStats(void);
};

extern	ManiLogCSSStats *gpManiLogCSSStats;

#endif
