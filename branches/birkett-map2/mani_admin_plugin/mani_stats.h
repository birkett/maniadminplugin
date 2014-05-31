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

#ifndef MANI_STATS_H
#define MANI_STATS_H

const int  MANI_MAX_USER_DEF = 16;
const int  MANI_MAX_DODS_USER_DEF = 8;
const int  MANI_MAX_STATS_CSS_WEAPONS = 28;
const int  MANI_MAX_STATS_DODS_WEAPONS = 25;

#include "mani_menu.h"

#define MANI_STATS_ME_TIMEOUT (15)

enum
{
	CSS_SHOT_FIRED				= 0,
	CSS_SHOT_HIT,
	CSS_BOMB_PLANTED,
	CSS_BOMB_DEFUSED,
	CSS_HOSTAGE_RESCUED,
	CSS_HOSTAGE_FOLLOW,
	CSS_HOSTAGE_KILLED,
	CSS_BOMB_EXPLODED,
	CSS_BOMB_DROPPED,
	CSS_BOMB_DEFUSE_ATTEMPT,
	CSS_VIP_ESCAPED,
	CSS_VIP_KILLED,
	CSS_WON_AS_CT,
	CSS_LOST_AS_CT,
	CSS_WON_AS_T,
	CSS_LOST_AS_T
};

enum
{
	DODS_SHOT_FIRED				= 0,
	DODS_SHOT_HIT,
	DODS_POINT_CAPTURED,
	DODS_CAPTURE_BLOCKED,
	DODS_WON_AS_AXIS,
	DODS_LOST_AS_AXIS,
	DODS_WON_AS_ALLIES,
	DODS_LOST_AS_ALLIES
};

const int MANI_MAX_STATS_HITGROUPS = 8;

struct rank_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	name[MAX_PLAYER_NAME_LENGTH];
	unsigned char ip_address[4];
	int		kills;
	int		deaths;
	int		suicides;
	int		headshots;
	float	kd_ratio;
	time_t	last_connected;
	float	points_decay;
	int		rank;
	float	points;
	float	rank_points;
	int		team_kills;
	int		total_time_online;

	// Not available unless player_hurt event
	int		hit_groups[MANI_MAX_STATS_HITGROUPS];
	int		weapon_kills[MANI_MAX_STATS_CSS_WEAPONS];
	int		damage;

	// Extra fields
	int		user_def[MANI_MAX_USER_DEF];
};

struct session_t
{
	int		kills;
	int		deaths;
	int		suicides;
	int		headshots;
	int		team_kills;
	int		damage;
	float	start_points;

	// Extra fields
	int		user_def[MANI_MAX_USER_DEF];
};

struct weaponme_t
{
	char	weapon_name[128];
	int		kills;
	float	percent;
};

class ShowTopFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr);
	void	Redraw(player_t *player_ptr);
	bool	SetStartRank(int start_rank);
	void	SetBackMore(int list_size);
private:
	bool	back;
	bool	more;
	int		current_rank;
};

class StatsMeFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr, player_t *output_player_ptr);
	void	Redraw(player_t *player_ptr);
private:
	int		user_id;
};

class SessionFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr, player_t *output_player_ptr);
	void	Redraw(player_t *player_ptr);
private:
	int		user_id;
};

class HitBoxMeFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr);
	void	Redraw(player_t *player_ptr);
};

class WeaponMeFreePage : public FreePage
{
public:
	bool	OptionSelected(player_t *player_ptr, const int option);
	bool	Render(player_t *player_ptr);
	void	Redraw(player_t *player_ptr);
	int		page;
};

class ManiStats
{
	friend class ShowTopFreePage;
	friend class StatsMeFreePage;
	friend class SessionFreePage;
	friend class HitBoxMeFreePage;
	friend class WeaponMeFreePage;

public:
	ManiStats();
	~ManiStats();

	void		Load(void);
	void		LoadStats(void);
	void		Unload(void);
	void		LevelInit(const char *map_name);
	void		NetworkIDValidated(player_t *player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		GameFrame(void);
	void		LevelShutdown(void);
	void		CalculateStats(bool use_steam_id, bool round_end);
	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  char *weapon_name, bool attacker_exists, bool headshot);
	void		DODSPlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  int weapon_index, bool attacker_exists);
	void		PlayerHurt( player_t *victim_ptr,  player_t *attacker_ptr, IGameEvent * event);
	void		PlayerSpawn(player_t *player_ptr);
	void		CSSPlayerFired(int index, bool is_bot);
	void		DODSPlayerFired(player_t *player_ptr);
	void		DODSPointCaptured(const char *cappers, int cappers_length);
	void		DODSCaptureBlocked(player_t *player_ptr);
	void		BombPlanted(player_t *player_ptr);
	void		BombDropped(player_t *player_ptr);
	void		BombExploded(player_t *player_ptr);
	void		BombDefused(player_t *player_ptr);
	void		BombBeginDefuse(player_t *player_ptr);
	void		HostageRescued(player_t *player_ptr);
	void		HostageFollows(player_t *player_ptr);
	void		HostageKilled(player_t *player_ptr);
	void		VIPEscaped(player_t *player_ptr);
	void		VIPKilled(player_t *player_ptr);
	void		CSSRoundEnd(int winning_team, const char *message);
	void		DODSRoundEnd(int winning_team);
	void		ShowRank(player_t *player_ptr);
	void		ResetStats(void);
	PLUGIN_RESULT	ProcessMaRanks(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaPLRanks(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaResetRank(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaSession(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaStatsMe(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);

private:

	struct	active_player_t
	{
		bool	active;
		float	last_hit_time;
		int		last_user_id;
		rank_t	*rank_ptr;
	};

	active_player_t	active_player_list[MANI_MAX_PLAYERS];
	session_t	session[MANI_MAX_PLAYERS];

	rank_t	**rank_list;
	rank_t	**rank_name_list;
	rank_t	**rank_player_list;
	rank_t	**rank_player_name_list;
	rank_t	**rank_player_waiting_list;
	rank_t	**rank_player_name_waiting_list;

	int	rank_list_size;
	int	rank_name_list_size;
	int	rank_player_list_size;
	int	rank_player_name_list_size;
	int	rank_player_waiting_list_size;
	int	rank_player_name_waiting_list_size;

	time_t	last_stats_calculate_time;
	time_t	last_stats_write_time;


	rank_t		*FindStoredRank (player_t *player_ptr);
	bool		level_ended;
	
	// Hash table of user ids
	short		hash_table[65536];
	int			weapon_hash_table[255];

	ConVar		*weapon_weights[MANI_MAX_STATS_CSS_WEAPONS];

	void		FreeStats(void);
	void		FreeStats(bool use_steam_id);
	void		FreeActiveList(void);
	void		ReBuildStatsList(bool use_steam_id);
	void		ReadStats(bool use_steam_id);
	void		WriteStats(bool use_steam_id);
	void		SetPointsDeltas( rank_t	*a_player_ptr, rank_t	*v_player_ptr, bool team_kill, bool a_is_bot, bool v_is_bot, int a_index, int v_index, float weapon_weight, bool suicide );
	bool		MoreThanOnePlayer(void);
	char		*GetBar(float percent);
	int			GetCSSWeaponHashIndex(char *weapon_string);
	int			GetCSSWeaponIndex(char *weapon_string);
	void		GetIPList(char *ip_address, unsigned char *ip_list);
	void		AddTeamPoints( int team,   int points);
};

extern	ManiStats *gpManiStats;

#endif

