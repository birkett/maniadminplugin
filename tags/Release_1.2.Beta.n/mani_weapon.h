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



#ifndef MANI_WEAPON_H
#define MANI_WEAPON_H

// Define structure types
struct weapon_type_t
{
	char	weapon_alias[MAX_WEAPON_ALIAS][128];
	int		number_of_weapons;
	char	weapon_name[128];
	bool	restricted;
	int		limit_per_team;
	int		ct_count;
	int		t_count;
};

struct main_weapon_t
{
	char	name[128];
	char	core_name[128];
	int		cost;
	int	restrict_index;
};

extern	weapon_type_t	*weapon_list;
extern	int				weapon_list_size;

extern	void	FreeWeapons(void);
extern	void	LoadWeapons(const char *pMapName);
extern	PLUGIN_RESULT ProcessClientBuy(const char *pcmd, const char *pcmd2, const int pargc, player_t *player_ptr);
extern	void ProcessRestrictWeapon( player_t *admin, int next_index, int argv_offset );
extern	bool HookAutobuyCommand(void);
extern	bool HookRebuyCommand(void);
extern	void PostProcessRebuyCommand(void);
extern	void ResetWeaponCount(void);
extern	void RemoveRestrictedWeapons(player_t *player_ptr);
extern	PLUGIN_RESULT	ProcessMaShowRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaKnives(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaPistols(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaShotguns(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaNoSnipers(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaUnRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	PLUGIN_RESULT	ProcessMaUnRestrictAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);




#endif
