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



#ifndef MANI_ADMIN_H
#define MANI_ADMIN_H

#define MAX_ADMIN_FLAGS (49)

struct admin_t
{
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	char	password[128];
	char	group_id[128];
	bool	flags[MAX_ADMIN_FLAGS];
};

struct admin_group_t
{
	char	group_id[128];
	bool	flags[MAX_ADMIN_FLAGS];
};

struct admin_flag_t
{
	char	flag;
	char	flag_desc[64];
};


extern	admin_t			*admin_list;
extern	admin_group_t	*admin_group_list;
extern	int				admin_list_size;
extern	int				admin_group_list_size;
extern	admin_flag_t	admin_flag_list[MAX_ADMIN_FLAGS];


extern	bool	IsClientAdmin( player_t *player, int *admin_index);
extern	void	AddAdmin(char *admin_details);
extern	void	AddAdminGroup(char *admin_details);
extern	void	LoadAdmins(void);
extern	void	FreeAdmins(void);
extern	bool	IsAdminAllowed(player_t *player, char *command, int admin_flag, bool check_war, int *admin_index);
extern	bool	IsCommandIssuedByServerAdmin( void );
extern  PLUGIN_RESULT	ProcessMaSetAdminFlag( int index,  bool svr_command,  int argc,  char *command_string,  char *target_string, char *flags);


#endif