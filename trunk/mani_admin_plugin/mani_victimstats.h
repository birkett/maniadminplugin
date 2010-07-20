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



#ifndef MANI_VICTIMSTATS_H
#define MANI_VICTIMSTATS_H

#include "mani_menu.h"

#define MANI_MAX_HITGROUPS (11)

class ShowMenuStatsFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *victim_ptr, player_t *attacker_ptr, int timeout);
};

class ManiVictimStats
{
	friend class ShowMenuStatsFreePage;
public:
	ManiVictimStats();
	~ManiVictimStats();

	void		ClientActive(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists, bool headshot,  char *weapon_name);
	void		DODSPlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists, int  weapon);
	void		PlayerHurt( player_t *victim_ptr,  player_t *attacker_ptr,  IGameEvent * event);
	void		PlayerSpawn(player_t *player_ptr);
	void		RoundStart(void);
	void		RoundEnd(void);

private:

	struct damage_t
	{
		int	armor_taken;
		int	health_taken;
		int shots_taken;
		int	armor_inflicted;
		int	health_inflicted;
		int shots_inflicted;
		bool killed;
		char weapon_name[128];
		bool headshot;
		char name[MAX_PLAYER_NAME_LENGTH];
		float	last_hit_time;
		bool shown_stats;
		float	distance;
		int	hit_groups_taken[MANI_MAX_HITGROUPS];
		int	hit_groups_inflicted[MANI_MAX_HITGROUPS];
	};

	damage_t		damage_list[MANI_MAX_PLAYERS][MANI_MAX_PLAYERS];

	void	AddHitGroup(int hits, char *final_string, char *bodypart);
	void	ShowStats(player_t *victim_ptr, player_t *attacker_ptr);
	void	ShowChatStats(player_t *victim_ptr, player_t *attacker_ptr, int mode);
};

extern	ManiVictimStats *gpManiVictimStats;

#endif
