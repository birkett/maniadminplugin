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



#ifndef MANI_PARSER_H
#define MANI_PARSER_H

#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "mani_player.h"

struct mani_colour_t
{
	char	*alias;
	float	red;
	float	green;
	float	blue;
};

extern bool	ParseLine(char *in, bool strip_comments, bool strip_start_comments);
extern bool	ParseAliasLine(char *in, char *alias, bool strip_comments, bool strip_start_comments);
extern bool	ParseAliasLine2(char *in, char *alias, char *question, bool strip_comments, bool strip_start_comments);
extern bool	ParseAliasLine3(char *in, char *alias, char *question, bool strip_comments, bool strip_start_comments);
extern bool ParseCommandReplace(char *in, char *alias, char *command_type, char *replacement);
extern void	ParseSubstituteStrings(player_t *player, const char *in_string, char *out_string);
extern void	ParseColourStrings( const char	*in_string, char	*out_string, Color	*out_colour);

#endif

