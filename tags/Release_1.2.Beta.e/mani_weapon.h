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
extern	void RemoveRestrictedWeapons(void);
extern	PLUGIN_RESULT	ProcessMaShowRestrict( int index,  bool svr_command);
extern	PLUGIN_RESULT	ProcessMaRestrictWeapon( int index,  bool svr_command,  int argc,  char *command_string,  char *weapon_name, char *limit_per_team, bool restrict);
extern	PLUGIN_RESULT	ProcessMaKnives( int index,  bool svr_command,  char *command_string);
extern	PLUGIN_RESULT	ProcessMaPistols( int index,  bool svr_command,  char *command_string);
extern	PLUGIN_RESULT	ProcessMaShotguns( int index,  bool svr_command,  char *command_string);
extern	PLUGIN_RESULT	ProcessMaNoSnipers( int index,  bool svr_command,  char *command_string);
extern	PLUGIN_RESULT	ProcessMaUnRestrictAll( int index,  bool svr_command,  int argc,  char *command_string);




#endif
