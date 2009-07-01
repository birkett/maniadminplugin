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



#ifndef SERVERPLUGIN_ADMIN_H
#define SERVERPLUGIN_ADMIN_H

// Hash defines
#define	MAX_WEAPON_ALIAS (10)
#define MAX_SAY_ARGC (10)

#define MANI_MAX_PLAYERS (64)
#define MAX_LAST_MAPS (20)
#define	MANI_MAX_TEAMS (5)

// Team indexes for CS Source
// CT
#define TEAM_B (3)
// T
#define TEAM_A (2)
//#define TEAM_SPEC (1)

// Version information
#define PLUGIN_VERSION "Mani Admin Plugin 2005 V1.2BetaB, www.mani-admin-plugin.com"
#define PLUGIN_CORE_VERSION "1.2BetaB"
#define PLUGIN_VERSION_ID "V1.2BetaB\n"

// Define vote types
#define VOTE_RANDOM_END_OF_MAP (0)
#define VOTE_RANDOM_MAP (1)
#define VOTE_EXTEND_MAP (2)
#define VOTE_MAP (3)
#define VOTE_QUESTION (4)
#define VOTE_RCON (5)
#define VOTE_ROCK_THE_VOTE (6)

// Define delay types for voting
#define VOTE_NO_DELAY (0)
#define VOTE_END_OF_ROUND_DELAY (1)
#define VOTE_END_OF_MAP_DELAY (2)

extern ConVar mani_stats_by_steam_id;

struct map_vote_t
{
	int	user_id;
	bool	slot_in_use;
	int	map_index;
	float	vote_time_stamp;
};

struct average_ping_t
{
	float	ping;
	int		count;
	bool	in_use;
};

struct	rcon_t
{
	char	rcon_command[512];
	char	alias[512];
};

struct	vote_rcon_t
{
	char	rcon_command[512];
	char	question[512];
	char	alias[512];
};

struct	vote_question_t
{
	char	question[512];
	char	alias[512];
};

struct	cexec_t
{
	char	cexec_command[512];
	char	alias[512];
};

struct reserve_slot_t
{
	char	steam_id[128];
};

struct ping_immunity_t
{
	char	steam_id[128];
};

struct active_player_t
{
	edict_t	*entity;
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	int		user_id;
	float	ping;
	float	time_connected;
	bool	is_spectator;
	int		index;
};

struct disconnected_player_t
{
	bool	in_use;
};

struct msay_t
{
	char	line_string[2048];
};

struct check_ping_t
{
	bool	in_use;
};

struct tw_spam_t
{
	int		index;
	float	last_time;
};

struct swear_t
{
	char	swear_word[128];
	int		length;
	bool	found;
	char	filtered[2048];
};

struct autokick_ip_t
{
	char	ip_address[32];
	bool	kick;
};

struct autokick_steam_t
{
	char	steam_id[64];
	bool	kick;
};

struct autokick_name_t
{
	char	name[128];
	bool	kick;
	bool	ban;
	int		ban_time;
};

struct autokick_pname_t
{
	char	pname[128];
	bool	kick;
	bool	ban;
	int		ban_time;
};

struct	cheat_cvar_t
{
	char	cheat_cvar[128];
};

struct	cheat_pinger_t
{
	char	control_string[128];
	bool	waiting_for_control;
	char	cvar_string[128];
	bool	waiting_for_cvar;
	bool	waiting_to_send;
	bool	connected;
	int		count;
};

struct say_argv_t
{
	char	argv_string[2048];
	int		index;
};

struct kill_progress_t
{
	long	kills;
};

struct gimp_t
{
	char	phrase[256];
};

struct vote_option_t
{
	char	vote_name[512];
	char	vote_command[512];
	bool	null_command;
	int		votes_cast;
};

struct system_vote_t
{
	bool	vote_in_progress;	// Flag if vote is in progress
	int		vote_starter;		// Player index who started vote
	bool	vote_confirmation;	// Flag if vote confirmation required
	int		vote_type;			// Type of vote in progress
	int		votes_required;		// Number of votes required for early result
	float	end_vote_time;		// end vote time when results are collated
	bool	waiting_decision;	// Used for Admin refuse/accept
	float	waiting_decision_time;	// Used for Admin refuse timeout
	int		delay_action;		// Delay action of vote
	char	vote_title[512];	// Name of vote title
	bool	map_decided;		// Stops end of map vote overriding an already selected map
	int		winner_index;		// Index of the winning vote option
	bool	start_rock_the_vote;	// Flag to start rock the vote
	bool	no_more_rock_the_vote;  // Flag to stop more rock the votes
	int		max_votes;			//	Max players who can vote
	int		number_of_extends;	// Max number of extends allowed
};

struct voter_t
{
	bool	allowed_to_vote;
	bool	voted;
	int		vote_option_index;
};

struct user_vote_t
{
	int		map_index;			// Map that user voted for
	float	map_vote_timestamp; 
	bool	rock_the_vote;		// User typed rock the vote
	float	nominate_timestamp;
	int		nominated_map;		// User nominated map index
	int		kick_id;			// User ID of kick target
	float	kick_vote_timestamp;
	int		kick_votes;			// Number of votes against player
	int		ban_id;				// User ID of player to ban
	float	ban_vote_timestamp;
	int		ban_votes;			// Number of votes against player
};

struct user_vote_map_t
{
	char	map_name[128];
};

struct lang_trans_t
{
	char	*translation;
};

struct name_change_t
{
	char	name[128];
	bool	in_use;
};

struct team_scores_t
{
	int	team_score[MANI_MAX_TEAMS];
};

enum {
MANI_SET_CASH,
MANI_GIVE_CASH,
MANI_GIVE_CASH_PERCENT,
MANI_TAKE_CASH,
MANI_TAKE_CASH_PERCENT};

enum {
MANI_SET_HEALTH,
MANI_GIVE_HEALTH,
MANI_GIVE_HEALTH_PERCENT,
MANI_TAKE_HEALTH,
MANI_TAKE_HEALTH_PERCENT};

#endif
