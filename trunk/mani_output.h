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
#include "mrecipientfilter.h"

#define MAX_SAY_ARGC (10)

typedef void (*CONPRINTF_FUNC)(const char *, ...);
extern CONPRINTF_FUNC MMsg;

enum
{
	ORANGE_CHAT				= 0,
	GREY_CHAT,
	LIGHT_GREEN_CHAT,
	GREEN_CHAT,
};

extern void	AdminSayToAdmin(const int colour, player_t *player,const char	*fmt, ...);
extern void	SayToAdmin(const int colour, player_t	*player,const char	*fmt, ...);
extern void	AdminSayToAll(const int colour, player_t *player,int anonymous, const char	*fmt, ...);
extern void	AdminHSayToAll(player_t *player,int anonymous, const char	*fmt, ...);
extern void	AdminCSayToAll(player_t *player,int anonymous, const char	*fmt, ...);
extern void CSayToAll(const char	*fmt, ...);
extern void CSayToPlayer(player_t *player_ptr,const char	*fmt, ...);
extern void	SayToAll(const int colour, bool echo, const char	*fmt, ...);
extern void	SayToDead(const int colour, const char *fmt, ...);
extern void	SayToPlayer(const int colour, player_t *player, const char *fmt, ...);
extern void	SayToTeam(const int colour, bool ct_side, bool t_side, bool spectator, const char *fmt, ...);
extern void	OutputToConsole(player_t *player_ptr, char *fmt, ...);
extern void	DirectLogCommand(char *fmt, ... );
extern void ResetLogCount(void);
extern void LogCommand(player_t *player_ptr, char *fmt, ... );
extern void ClientMsg(Color	*col, const unsigned int seconds, const bool admin_only, const int level, const char* fmt, ... ) ;
extern void ClientMsgSinglePlayer( edict_t *pEntity, const unsigned int seconds,  const int	level, const char* fmt, ... );
extern void	PrintToClientConsole(edict_t *pEntity, char *fmt, ... );
extern void OutputHelpText(const int colour, player_t *player_ptr, char *fmt, ...);
extern void	ParseSayString(const char *say_string, char *trimmed_string_out, int *say_argc);
extern void UTIL_LogPrintf( char *fmt, ... );
extern void UTIL_SayText(int colour, MRecipientFilter *mrf, const char *say_text);
extern void WriteDebug ( char *fmt, ...);
extern void UTIL_SayHint(MRecipientFilter *mrf_ptr, char *text_ptr);
extern void SplitHintString(char *string, int width);
extern bool UTIL_InterfaceMsg( void *ptr, char *interface_id, char *version);
extern void FindConPrintf(void);


#endif