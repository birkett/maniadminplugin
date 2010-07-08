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

const int MANI_MAX_PLAYERS = 64;

#define MAX_LAST_MAPS (20)
#define	MANI_MAX_TEAMS (5)
#define COMMAND_SPAM_DELAY 1.0f
// Team indexes for CS Source
// CT
#define TEAM_B (3)
// T
#define TEAM_A (2)

// Version information
const int	gametypes_min_version = 2;

#ifdef WIN32
#define snprintf _snprintf
#endif

#define COMMON_CORE "1.2V"
#define COMMON_VERSION "Mani Admin Plugin 2009 V" COMMON_CORE

#ifdef ORANGE
#ifdef SOURCEMM
#define PLUGIN_VERSION COMMON_VERSION " SMM Orange, www.mani-admin-plugin.com"
#define PLUGIN_CORE_VERSION COMMON_CORE " SMM"
#else
#define PLUGIN_VERSION COMMON_VERSION " VSP Orange, www.mani-admin-plugin.com"
#define PLUGIN_CORE_VERSION COMMON_CORE " VSP"
#endif
#else
#ifdef SOURCEMM
#define PLUGIN_VERSION COMMON_VERSION " SMM, www.mani-admin-plugin.com"
#define PLUGIN_CORE_VERSION COMMON_CORE " SMM"
#else
#define PLUGIN_VERSION COMMON_VERSION " VSP, www.mani-admin-plugin.com"
#define PLUGIN_CORE_VERSION COMMON_CORE " VSP"
#endif
#endif

#define PLUGIN_VERSION_ID "V" COMMON_CORE "\n"
#define PLUGIN_VERSION_ID2 "V" COMMON_CORE

#if defined ( WIN32 )
	#define PATH_SLASH '\\'
#else
	#define PATH_SLASH '/'
#endif

// Define vote types
enum
{
	VOTE_RANDOM_END_OF_MAP = 0,
	VOTE_RANDOM_MAP,
	VOTE_EXTEND_MAP,
	VOTE_MAP,
	VOTE_QUESTION,
	VOTE_RCON,
	VOTE_ROCK_THE_VOTE,
};

// Define delay types for voting
enum
{
	VOTE_NO_DELAY = 0,
	VOTE_END_OF_ROUND_DELAY,
	VOTE_END_OF_MAP_DELAY,
};

extern ConVar mani_stats_by_steam_id;

struct	rcon_t
{
	char	rcon_command[512];
	char	alias[512];
};

struct	cexec_t
{
	char	cexec_command[512];
	char	alias[512];
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
	char	steam_id[MAX_NETWORKID_LENGTH];
	bool	kick;
};

struct autokick_name_t
{
	char	name[MAX_PLAYER_NAME_LENGTH];
	bool	kick;
	bool	ban;
	int		ban_time;
};

struct autokick_pname_t
{
	char	pname[MAX_PLAYER_NAME_LENGTH];
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

struct kill_progress_t
{
	long	kills;
};

struct name_change_t
{
	char	name[MAX_PLAYER_NAME_LENGTH];
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

#ifdef ORANGE
#define CONVAR_CALLBACK_PROTO(_name) \
	void _name ( IConVar *pVar, char const *pOldString, float pOldFloat)

#define CONVAR_CALLBACK_REF(_name) \
	(FnChangeCallback_t) _name

#define CONVAR_CALLBACK_FN(_name) \
	void _name ( IConVar *pVar, char const *pOldString, float pOldFloat)
#else
#define CONVAR_CALLBACK_PROTO(_name) \
	static void _name ( ConVar *pVar, char const *pOldString)

#define CONVAR_CALLBACK_REF(_name) \
	_name

#define CONVAR_CALLBACK_FN(_name) \
	static void _name ( ConVar *pVar, char const *pOldString)
#endif

#endif
