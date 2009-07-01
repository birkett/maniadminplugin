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



#ifndef MANI_MENU_H
#define MANI_MENU_H

#define MAX_AMX_MENU (8)
#define MAX_ESCAPE_MENU (7)

struct menu_t
{
	char	sort_name[256];
	char	menu_text[128];
	char	menu_command[512];
};

struct menu_select_t
{
	char	command[512];
	bool	in_use;
};

struct menu_confirm_t
{
	bool	in_use;
	menu_select_t menu_select[10];
};

extern	menu_t		*menu_list;
extern	int			menu_list_size;
extern	menu_confirm_t	menu_confirm[MANI_MAX_PLAYERS];

extern	void	FreeMenu (void);
extern	void	DrawSubMenu (player_t *player, char *title, char *message, int next_index, char *root_command, char *more_command, bool sub_menu_back, int time_to_show );
extern	void	DrawStandardMenu( player_t	*player, char *title, char *message, bool show_exit);
extern	void	DrawMenu(int player_index, int time, int range, bool back, bool more, bool cancel, char *menu_string, bool final);
extern	void	SortMenu(void);
extern	void	ProcessPlayMenuSound( player_t *target_player_ptr, char *sound_name);

#endif
