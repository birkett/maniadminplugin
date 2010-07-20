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



#ifndef MANI_TEAMKILL_H
#define MANI_TEAMKILL_H

#include "mani_menu.h"

#define MANI_MAX_PUNISHMENTS (12)

#define MANI_TK_FORGIVE (0)
#define MANI_TK_SLAY (1)
#define MANI_TK_SLAP (2)
#define MANI_TK_BLIND (3)
#define MANI_TK_FREEZE (4)
#define MANI_TK_CASH (5)
#define MANI_TK_DRUG (6)
#define MANI_TK_BURN (7)
#define MANI_TK_TIME_BOMB (8)
#define MANI_TK_FIRE_BOMB (9)
#define MANI_TK_FREEZE_BOMB (10)
#define MANI_TK_BEACON (11)

struct tk_player_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	name[MAX_PLAYER_NAME_LENGTH];
	int		user_id; // Required for bots
	int		violations_committed;
	int		round_to_action_punishment; // CSS only 
	int		spawn_violations_committed;
	int		last_round_violation;
	int		rounds_to_miss;
	int		team_wounds;
	float	team_wound_reflect_ratio;
	int		punishment[MANI_MAX_PUNISHMENTS];
};

struct tk_bot_t
{
	bool	bot_can_give;
	bool	bot_can_take;
	ConVar	*punish_cvar_ptr;
};

extern	tk_player_t	*tk_player_list;
extern	int	tk_player_list_size;

extern	void	InitTKPunishments (void);
extern	void	FreeTKPunishments(void);
extern	void	ProcessTKSelectedPunishment(int	punishment, player_t *victim_ptr, const int attacker_user_id, char *attacker_steam_id, bool bot_calling);
extern	void	ProcessTKSpawnPunishment(player_t *player_ptr);

extern	bool	TKBanPlayer (player_t	*attacker, int ban_index);
extern	void	CreateNewTKPlayer(char *name, char *steam_id, int user_id, int violations, int rounds_to_miss);
extern	bool	IsTKPlayerMatch( tk_player_t *tk_player, player_t *player);
extern	bool	ProcessTKDeath( player_t *attacker_ptr, player_t *victim_ptr );
extern	PLUGIN_RESULT	ProcessMaTKList( player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);

MENUALL_DEC(TKPlayer);

#endif
