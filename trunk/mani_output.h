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



#ifndef MANI_OUTPUT_H
#define MANI_OUTPUT_H

#include "mani_player.h"

extern void	AdminSayToAdmin(player_t *player,const char	*fmt, ...);
extern void	SayToAdmin(player_t	*player,const char	*fmt, ...);
extern void	AdminSayToAll(player_t *player,int anonymous, const char	*fmt, ...);
extern void	AdminSayToAllColoured(player_t *player,int anonymous, const char	*fmt, ...);
extern void	AdminCSayToAll(player_t *player,int anonymous, const char	*fmt, ...);
extern void CSayToAll(const char	*fmt, ...);
extern void	SayToAll(bool echo, const char	*fmt, ...);
extern void	SayToDead(const char *fmt, ...);
extern void	SayToPlayer(player_t *player, const char *fmt, ...);
extern void	SayToTeam(bool ct_side, bool t_side, bool spectator, const char *fmt, ...);
extern void	OutputToConsole(edict_t *pEntity, bool svr_command, char *fmt, ...);
extern void	DirectLogCommand(char *fmt, ... );
extern void ResetLogCount(void);
extern void LogCommand(edict_t *pEntity, char *fmt, ... );
extern void ClientMsg(Color	*col, const unsigned int seconds, const bool admin_only, const int level, const char* fmt, ... ) ;
extern void ClientMsgSinglePlayer( edict_t *pEntity, const unsigned int seconds,  const int	level, const char* fmt, ... );
extern void	PrintToClientConsole(edict_t *pEntity, char *fmt, ... );
extern void OutputHelpText( player_t	*player_ptr, bool		to_server_console, char		*fmt, ...);


#endif