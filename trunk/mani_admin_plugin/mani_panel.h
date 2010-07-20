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



#ifndef MANI_PANEL_H
#define MANI_PANEL_H

#include "mani_menu.h"

#define MANI_TOP_PANEL ("mani_top")
#define MANI_STATSME_PANEL ("mani_statsme")
#define MANI_RULES_PANEL ("mani_rules")
#define MANI_WEB_STATS_PANEL ("mani_web_stats")

struct	web_shorcut_t
{
	char	shortcut[512];
	char	url_string[512];
};

extern void	LoadWebShortcuts(void);
extern void FreeWebShortcuts(void);
extern bool ProcessWebShortcuts(edict_t *pEntity, const char *say_string);
extern PLUGIN_RESULT ProcessMaFavourites(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern void	InitPanels(void);
extern void	DrawPanel(MRecipientFilter *mrf, char *panel_title, char *network_string, char *message, int message_length);
extern void	DrawMOTD(MRecipientFilter *mrf);
extern void	DrawURL(MRecipientFilter *mrf, char *title, const char *url);

MENUALL_DEC(Favourites);

#endif