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



#ifndef MANI_IMMUNITY_H
#define MANI_IMMUNITY_H

#define MAX_IMMUNITY_FLAGS (25)

struct immunity_t
{
	char	steam_id[128];
	char	ip_address[128];
	char	name[128];
	char	password[128];
	char	group_id[128];
	bool	flags[MAX_IMMUNITY_FLAGS];
};

struct immunity_group_t
{
	char	group_id[128];
	bool	flags[MAX_IMMUNITY_FLAGS];
};

struct immunity_flag_t
{
	char	flag;
	char	flag_desc[64];
};


extern	immunity_t			*immunity_list;
extern	immunity_group_t	*immunity_group_list;
extern	int					immunity_list_size;
extern	int					immunity_group_list_size;
extern	immunity_flag_t		immunity_flag_list[MAX_IMMUNITY_FLAGS];


extern	bool	IsImmune( player_t *player, int *immunity_index);
extern	void	AddImmunity(char *immunity_details);
extern	void	AddImmunityGroup(char *immunity_details);
extern	void	LoadImmunity(void);
extern	void	FreeImmunity(void);


#endif
