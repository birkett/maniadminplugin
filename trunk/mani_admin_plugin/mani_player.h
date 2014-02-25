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



#ifndef MANI_PLAYER_H
#define MANI_PLAYER_H

struct player_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	ip_address[128];
	char	name[MAX_PLAYER_NAME_LENGTH];
	char	password[128];
	int		user_id;
	int		team;
	int		health;
	int		index;
	edict_t	*entity;
	bool	is_bot;
	bool	is_dead;
	IPlayerInfo *player_info;
};

struct	teleport_coords_t
{
	Vector	coords;
	char	map_name[128];
};

struct player_settings_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	name[MAX_PLAYER_NAME_LENGTH];
	char	damage_stats;
	char	damage_stats_timeout;
	char	show_destruction;
	char	quake_sounds;
	char	server_sounds;
	char	show_vote_results_progress;
	time_t	last_connected;
	char	admin_t_model[20];
	char	admin_ct_model[20];
	char	immunity_t_model[20];
	char	immunity_ct_model[20];
	char	t_model[20];
	char	ct_model[20];
	char	language[20];
	int		show_death_beam;
	unsigned int		bit_array[8]; // 8 * 32 = 256 flags to play with (provided we have 32 bits per integer)
	int		teleport_coords_list_size;
	teleport_coords_t *teleport_coords_list;
};

struct	active_settings_t
{
	player_settings_t *ptr;
	bool	active;
};

extern	player_t	*target_player_list;
extern  int			target_player_list_size;

extern	void	SetPluginPausedStatus(bool status);
extern	bool	ProcessPluginPaused(void);

extern	bool	FindTargetPlayers(player_t *requesting_player, const char *target_string, char *immunity_flag);
extern	bool	FindPlayerBySteamID(player_t *player_ptr);
extern	bool	FindPlayerByUserID(player_t *player_ptr);
extern	bool	FindPlayerByEntity(player_t *player_ptr);
extern	bool	FindPlayerByIndex(player_t *player_ptr);
extern	bool	FindPlayerByIPAddress(player_t *player_ptr);
extern	void	GetIPAddressFromPlayer(player_t *player);

extern	void	InitPlayerSettingsLists (void);
extern	void	PlayerJoinedInitSettings(player_t *player);

extern	player_settings_t *FindStoredPlayerSettings (player_t *player);
extern	player_settings_t *FindPlayerSettings (player_t *player);

extern	bool	FindPlayerFlag (player_settings_t *player_settings_ptr, int flag_index);
extern	void	SetPlayerFlag (player_settings_t *player_settings_ptr, int flag_index, bool flag_value);

extern	void	PlayerSettingsDisconnect (player_t *player);

extern	void	AddWaitingPlayerSettings(void);
extern	void	ResetActivePlayers(void);
extern	void	ProcessPlayerSettings(void);
extern	void	FreePlayerSettings(void);
extern	void	FreePlayerNameSettings(void);

extern	PLUGIN_RESULT	ProcessMaDamage( int index);
extern	PLUGIN_RESULT	ProcessMaDamageTimeout( int index);
extern	PLUGIN_RESULT	ProcessMaSounds( int index);
extern	PLUGIN_RESULT	ProcessMaQuake( int index);
extern	PLUGIN_RESULT	ProcessMaDeathBeam ( int index);
extern	PLUGIN_RESULT	ProcessMaDestruction( int index);
extern	PLUGIN_RESULT	ProcessMaVoteProgress( int index);
extern	int				GetNumberOfActivePlayers( bool include_bots );
extern  void UTIL_KickPlayer( player_t *player_ptr,  char *short_reason,  char *long_reason,  char *log_reason );
extern  bool UTIL_DropC4(edict_t *pEntity);
extern	void UTIL_EmitSoundSingle(player_t *player_ptr, const char *sound_id);

#include "mani_menu.h"

MENUALL_DEC(PlayerSettings);
MENUALL_DEC(SkinChoice);

#if !defined GAME_ORANGE
class IVEngineServer;
extern	IVEngineServer	*engine;
class CCommand
{
public:
	CCommand () {};
		
	const char *ArgS() const
	{
		return engine->Cmd_Args();
	}
	int ArgC() const
	{
		return engine->Cmd_Argc();
	}

	const char *Arg(int index) const
	{
		return engine->Cmd_Argv(index);
	}
};

#endif

#endif
