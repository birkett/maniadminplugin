//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "Color.h"
#include "eiface.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "bitbuf.h"
#include "mani_main.h"
#include "mani_player.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_maps.h"
#include "mani_gametype.h"
#include "mani_output.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerPluginHelpers *helpers;
extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	IFileSystem	*filesystem;
extern	ICvar *g_pCVar;
extern	bool war_mode;
extern	int	max_players;
extern	bf_write *msg_buffer;
extern	int	text_message_index;
extern	int hintMsg_message_index;

#define SIGLEN			8
#define ENGINE486_SIG	"\x55\x89\xE5\x53\x83\xEC\x14\xBB"
#define ENGINE486_OFFS	40
#define ENGINE686_SIG	"\x53\x83\xEC\x08\xBB\x01\x00\x00"
#define ENGINE686_OFFS	50
#define	ENGINEAMD_SIG	"\x53\x51\xBB\x01\x00\x00\x00\x51"
#define	ENGINEAMD_OFFS	47
#define ENGINEW32_SIG	"\xA1\x2A\x2A\x2A\x2A\x56\xBE\x01"
#define ENGINEW32_OFFS	38
#define IA32_CALL		0xE8

static	int	map_count = -1;
static	char mani_log_filename[512]="temp.log";

typedef void (*CONPRINTF_FUNC)(const char *, ...);
CONPRINTF_FUNC MMsg = Msg;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

CONVAR_CALLBACK_PROTO(ManiLogMode);
static void WriteToManiLog ( char *log_string, char *steam_id);
static bool vcmp(void *_addr1, void *_addr2, size_t len);


ConVar mani_log_directory ("mani_log_directory", "mani_logs", 0, "This defines the directory to store admin logs in"); 
ConVar mani_log_mode ("mani_log_mode", "0", 0, "0 = to main valve log file, 1 = per map in mani_log_directory, 2 = log to one big file in mani_log_directory, 3 = per steam id in mani_log_directory", true, 0, true, 3, CONVAR_CALLBACK_REF(ManiLogMode)); 

//---------------------------------------------------------------------------------
// Purpose: Say only to admin
//---------------------------------------------------------------------------------
void AdminSayToAdmin
(
 const int colour,
player_t	*player,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	final_string[2048];
	bool	found_admin = false;

	player_t recipient_player;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	if (player)
	{
		snprintf(final_string, sizeof (final_string), "(ADMIN ONLY) %s: %s", player->name, tempString);
	}
	else
	{
		snprintf(final_string, sizeof (final_string), "(ADMIN ONLY) CONSOLE: %s", tempString);
	}

	OutputToConsole(NULL, "%s\n", final_string);

	for (int i = 1; i <= max_players; i++)
	{
		recipient_player.index = i;
		if (!FindPlayerByIndex (&recipient_player))
		{
			continue;
		}

		if (recipient_player.is_bot)
		{
			continue;
		}

		if (gpManiClient->HasAccess(recipient_player.index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			// This is an admin player
			mrf.AddPlayer(i);
//			OutputToConsole(recipient_player.entity, "%s\n", final_string);
			found_admin = true;
		}
	}

	if (found_admin)
	{
		UTIL_SayText(colour, &mrf, final_string);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Say from player only to admin
//---------------------------------------------------------------------------------
void SayToAdmin
(
 const int colour,
player_t	*player,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	final_string[2048];
	bool	found_player = false;
	player_t recipient_player;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	snprintf(final_string, sizeof (final_string), "(TO ADMIN) %s: %s", player->name, tempString);
	OutputToConsole(NULL, "%s\n", final_string);

	for (int i = 1; i <= max_players; i++)
	{
		recipient_player.index = i;
		if (!FindPlayerByIndex (&recipient_player))
		{
			continue;
		}

		if (recipient_player.is_bot)
		{
			continue;
		}

		if (player->index == i)
		{
				mrf.AddPlayer(i);
				found_player = true;
//				OutputToConsole(recipient_player.entity, "%s\n", final_string);
		}
		else
		{
			if (gpManiClient->HasAccess(recipient_player.index, ADMIN, ADMIN_BASIC_ADMIN))
			{
				// This is an admin player
				mrf.AddPlayer(i);
				found_player = true;
//				OutputToConsole(recipient_player.entity, "%s\n", final_string);
			}
		}
	}

	if (found_player)
	{
		UTIL_SayText(colour, &mrf, final_string);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Say admin string to all
//---------------------------------------------------------------------------------
void AdminSayToAll
(
 const int colour,
player_t	*player,
int			anonymous,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	admin_final_string[2048];
	char	non_admin_final_string[2048];
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (!player)
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, "%s\n", admin_final_string);

	if (anonymous == 1)
	{
		MRecipientFilter mrfadmin;
		MRecipientFilter mrf;
		mrf.MakeReliable();
		mrfadmin.MakeReliable();

		for (int i = 1; i <= max_players; i++)
		{
			bool is_admin;

			is_admin = false;
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			is_admin = gpManiClient->HasAccess(server_player.index, ADMIN, ADMIN_BASIC_ADMIN);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
//				OutputToConsole(server_player.entity, "%s\n", admin_final_string);
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
//				OutputToConsole(server_player.entity, "%s\n", non_admin_final_string);
			}
		}

		if (found_player)
		{
			UTIL_SayText(colour, &mrf, non_admin_final_string);
		}

		if (found_admin)
		{
			UTIL_SayText(colour, &mrfadmin, admin_final_string);
		}
	}
	else
	{
		for (int i = 1; i <= max_players; i++)
		{
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			found_player = true;
//			OutputToConsole(server_player.entity, "%s\n", admin_final_string);
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

			UTIL_SayText(colour, &mrf, admin_final_string);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say admin string to all
//---------------------------------------------------------------------------------
void AdminCSayToAll
(
player_t	*player,
int			anonymous,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	admin_final_string[2048];
	char	non_admin_final_string[2048];
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (!player)
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, "%s\n", admin_final_string);

	if (anonymous == 1)
	{
		MRecipientFilter mrfadmin;
		MRecipientFilter mrf;
		mrf.MakeReliable();
		mrfadmin.MakeReliable();

		for (int i = 1; i <= max_players; i++)
		{
			bool is_admin;

			is_admin = false;
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			is_admin = gpManiClient->HasAccess(server_player.index, ADMIN, ADMIN_BASIC_ADMIN);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
				if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
					  gpManiGameType->IsGameType(MANI_GAME_CSGO)))
				{
					OutputToConsole(&server_player, "%s\n", admin_final_string);
				}
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
				if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
					gpManiGameType->IsGameType(MANI_GAME_CSGO)))
				{
					OutputToConsole(&server_player, "%s\n", non_admin_final_string);
				}
			}
		}

		if (found_player)
		{
#if defined ( GAME_CSGO )
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
#endif
			msg_buffer->WriteByte(4); // Center area
			msg_buffer->WriteString(non_admin_final_string);
			engine->MessageEnd();
		}

		if (found_admin)
		{
#if defined ( GAME_CSGO )
			msg_buffer = engine->UserMessageBegin( &mrfadmin, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
			msg_buffer = engine->UserMessageBegin( &mrfadmin, text_message_index ); // Show TextMsg type user message
#endif
			msg_buffer->WriteByte(4); // Center area
			msg_buffer->WriteString(admin_final_string);
			engine->MessageEnd();
		}
	}
	else
	{
		for (int i = 1; i <= max_players; i++)
		{
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			found_player = true;
			if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
				gpManiGameType->IsGameType(MANI_GAME_CSGO)))
			{
				OutputToConsole(&server_player, "%s\n", admin_final_string);
			}
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

#if defined ( GAME_CSGO )
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
#endif
			msg_buffer->WriteByte(4); // Center area
			msg_buffer->WriteString(admin_final_string);
			engine->MessageEnd();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say string to all players in center of screen
//---------------------------------------------------------------------------------
void CSayToAll
(
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.AddAllPlayers(max_players);

#if defined ( GAME_CSGO )
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
#endif
	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Say string to single player in center of screen
//---------------------------------------------------------------------------------
void CSayToPlayer
(
player_t *player_ptr,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.AddPlayer(player_ptr->index);

#if defined ( GAME_CSGO )
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
#endif
	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Say to all
//---------------------------------------------------------------------------------
void SayToAll(const int colour, bool echo, const char	*fmt, ...)
{
	va_list		argptr;
	char		tempString[1024];
	player_t	server_player;
	bool		found_player = false;

	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	if (echo) OutputToConsole(NULL, "%s\n", tempString);

	for (int i = 1; i <= max_players; i++)
		{
		server_player.index = i;
		if (!FindPlayerByIndex(&server_player))
		{
			continue;
		}

		if (server_player.is_bot)
		{
			continue;
		}

		found_player = true;
		mrf.AddPlayer(i);

		if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
			gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
			if (echo) OutputToConsole(&server_player, "%s\n", tempString);
		}
	}

	if (found_player)
	{
		UTIL_SayText(colour, &mrf, tempString);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say to Dead
//---------------------------------------------------------------------------------
void SayToDead(const int colour, const char	*fmt, ...)
{
	va_list		argptr;
	char		tempString[1024];
	player_t	recipient_player;
	bool		found_player = false;

	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	OutputToConsole(NULL, "%s\n", tempString);

	for (int i = 1; i <= max_players; i++)
	{
		recipient_player.index = i;
		if (!FindPlayerByIndex(&recipient_player))
		{
			continue;
		}

		if (recipient_player.is_bot)
		{
			continue;
		}

		// If player is spectator
		if (gpManiGameType->IsSpectatorAllowed() && 
			recipient_player.team == gpManiGameType->GetSpectatorIndex())
		{
			mrf.AddPlayer(i);
			found_player = true;
			if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
				gpManiGameType->IsGameType(MANI_GAME_CSGO)))
			{
				OutputToConsole(&recipient_player, "%s\n", tempString);
			}
			continue;
		}

		if (recipient_player.is_dead)
		{
			mrf.AddPlayer(i);
			found_player = true;
			if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
				gpManiGameType->IsGameType(MANI_GAME_CSGO)))
			{
				OutputToConsole(&recipient_player, "%s\n", tempString);
			}

			continue;
		}
	}

	if (found_player)
	{
		UTIL_SayText(colour, &mrf, tempString);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say to player
//---------------------------------------------------------------------------------
void SayToPlayer(const int colour, player_t *player, const char	*fmt, ...)
{
	va_list		argptr;
	char		tempString[1024];
	player_t	recipient_player;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();

	recipient_player.index = player->index;
	if (!FindPlayerByIndex(&recipient_player))
	{
		return;
	}

	if (recipient_player.is_bot)
	{
		return;
	}

	mrf.AddPlayer(player->index);
	if (!(gpManiGameType->IsGameType(MANI_GAME_CSS) ||
		 gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		OutputToConsole(player, "%s\n", tempString);
	}
//	OutputToConsole(NULL, "%s\n", tempString);

	UTIL_SayText(colour, &mrf, tempString);
}

//---------------------------------------------------------------------------------
// Purpose: Say admin string to all
//---------------------------------------------------------------------------------
void AdminHSayToAll
(
player_t	*player,
int			anonymous,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	admin_final_string[1024];
	char	non_admin_final_string[1024];
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (!player)
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, "%s\n", admin_final_string);

	SplitHintString(admin_final_string, 34);
	SplitHintString(non_admin_final_string, 34);

	if (anonymous == 1)
	{
		MRecipientFilter mrfadmin;
		MRecipientFilter mrf;
		mrf.MakeReliable();
		mrfadmin.MakeReliable();

		for (int i = 1; i <= max_players; i++)
		{
			bool is_admin;

			is_admin = false;
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			is_admin = gpManiClient->HasAccess(server_player.index, ADMIN, ADMIN_BASIC_ADMIN);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
//				OutputToConsole(server_player.entity, "%s\n", admin_final_string);
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
//				OutputToConsole(server_player.entity, "%s\n", non_admin_final_string);
			}
		}

		if (found_player)
		{
			UTIL_SayHint(&mrf, non_admin_final_string);
		}

		if (found_admin)
		{
			UTIL_SayHint(&mrfadmin, admin_final_string);
		}
	}
	else
	{
		for (int i = 1; i <= max_players; i++)
		{
			server_player.index = i;
			if (!FindPlayerByIndex(&server_player))
			{
				continue;
			}

			if (server_player.is_bot)
			{
				continue;
			}

			found_player = true;
//			OutputToConsole(server_player.entity, "%s\n", admin_final_string);
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

			UTIL_SayHint(&mrf, admin_final_string);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Use Hint type message to ouput text
//---------------------------------------------------------------------------------
void UTIL_SayHint(MRecipientFilter *mrf_ptr, char *text_ptr)
{
	char text_out[192];

	// Copy to restricted size
	snprintf(text_out, sizeof(text_out), "%s", text_ptr);
#if defined ( GAME_CSGO )
	msg_buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(mrf_ptr), hintMsg_message_index, "HintText" );
#else
	msg_buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(mrf_ptr), hintMsg_message_index);
	msg_buffer->WriteByte(1);
#endif
	msg_buffer->WriteString(text_out);
	engine->MessageEnd();	
}

//---------------------------------------------------------------------------------
// Purpose: Use Hint type message to ouput text
//---------------------------------------------------------------------------------
void SplitHintString(char *string, int width)
{
	int	length = Q_strlen(string);
	int	last_space = -1;
	int	wrap_count = 0;

	if (length < width)
	{
		return;
	}

	for (int i = 0; i < length; i++) 
	{
		if (string[i] == ' ')
		{
			last_space = i;
		}

		if (string[i] == '%' && string[i + 1] == 's')
		{
			string[i] = ' ';
		}

		if (string[i] == '\n')
		{
			wrap_count = -1;
		}

		if (wrap_count == width)
		{
			if (last_space != -1)
			{
				string[last_space] = '\n';
			}

			wrap_count = 0;
		}
		else
		{
			wrap_count ++;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say string to specific teams
//---------------------------------------------------------------------------------
void SayToTeam
(
 const int colour,
bool	ct_side,
bool	t_side,
bool	spectator,
const char	*fmt, ...
)
{
	va_list		argptr;
	char		tempString[1024];

	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	player_t recipient_player;

	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	for (int i = 1; i <= max_players; i++)
	{
		recipient_player.index = i;
		if (!FindPlayerByIndex (&recipient_player))
		{
			continue;
		}

		if (recipient_player.is_bot)
		{
			continue;
		}

		if (ct_side)
		{
			if (recipient_player.team == TEAM_B)
			{
				mrf.AddPlayer(i);
			}
		}
		
		if (t_side)
		{
			if (recipient_player.team == TEAM_A)
			{
				mrf.AddPlayer(i);
			}
		}
		
		if (gpManiGameType->IsSpectatorAllowed() && spectator)
		{
			if (recipient_player.team == gpManiGameType->GetSpectatorIndex())
			{
				mrf.AddPlayer(i);
			}
		}
	}

	OutputToConsole(NULL, "%s\n", tempString);
	UTIL_SayText(colour, &mrf, tempString);
}

//---------------------------------------------------------------------------------
// Purpose: Dump string to player console or server console depending on flag
//---------------------------------------------------------------------------------
void	OutputToConsole(player_t *player_ptr, char *fmt, ...)
{
	va_list		argptr;
	char		tempString[2048];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	if (!player_ptr)
	{
		MMsg("%s", tempString);
	}
	else
	{
		engine->ClientPrintf(player_ptr->entity, tempString);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Log who did what with timestamp to console
//---------------------------------------------------------------------------------
void DirectLogCommand(char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to server console
	engine->LogPrint( tempString );
}

//---------------------------------------------------------------------------------
// Purpose: Log who did what with timestamp to console
//---------------------------------------------------------------------------------
void LogCommand(player_t *player_ptr, char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024]="";
	char		tempString2[1024]="";
	char		user_details[128]="CONSOLE : ";
	char		steam_id[MAX_NETWORKID_LENGTH]="CONSOLE";

	if(player_ptr)
	{
		Q_strcpy (steam_id, player_ptr->steam_id);
		snprintf( user_details, sizeof(user_details), "[MANI_ADMIN_PLUGIN] Admin [%s] [%s] Executed : ", player_ptr->name, player_ptr->steam_id);
	}	

	va_start ( argptr, fmt );
	vsnprintf( tempString2, sizeof(tempString2), fmt, argptr );
	va_end   ( argptr );

	snprintf( tempString, sizeof(tempString), "%s %s", user_details, tempString2);  

	WriteToManiLog (tempString, steam_id);
	// Print to server console
}

//---------------------------------------------------------------------------------
// Purpose: Reset the log count for mode 1
//---------------------------------------------------------------------------------
void ResetLogCount(void)
{
	map_count = -1;
}

//---------------------------------------------------------------------------------
// Purpose: Write to log
//---------------------------------------------------------------------------------
static
void WriteToManiLog 
(
 char *log_string,
 char *steam_id
)
{
	int log_mode = mani_log_mode.GetInt();

	if (log_mode == 0)
	{
		// Log straight to Valves log
		engine->LogPrint( log_string );
		return;
	}
	else if (log_mode == 1 || log_mode == 2 || log_mode == 3)
	{
		FileHandle_t	filehandle = NULL;

		// Still print to log
		engine->LogPrint( log_string );

		if (log_mode == 1 || log_mode == 2)
		{
			// Log to custom log file (same filename format as Valves)
			filehandle = filesystem->Open(mani_log_filename, "at+");
			if (filehandle == NULL)
			{
					MMsg("Failed to open log file [%s] for writing.", mani_log_filename);
					engine->LogPrint( log_string );
					return;
			}

		}
		else if (log_mode == 3)
		{
			char	steam_filename[512];

			int	steam_length = Q_strlen(steam_id);

			for (int i = 0; i < steam_length; i ++)
			{
				// Having a : screws up filenaming
				if (steam_id[i] == ':')
				{
					steam_id[i] = '_';
				}
			}


			char full_path[512];
			snprintf(full_path, sizeof(full_path), "./cfg/%s/%s", 
									mani_path.GetString(),
									mani_log_directory.GetString()
									);

			filesystem->CreateDirHierarchy(full_path);

			snprintf(steam_filename, sizeof(steam_filename), "./cfg/%s/%s/%s.log", 
									mani_path.GetString(),
									mani_log_directory.GetString(),
									steam_id
									);

			// Log to custom log file (same filename format as Valves)
			filehandle = filesystem->Open(steam_filename, "at+");
			if (filehandle == NULL)
			{
					MMsg("Failed to open log file [%s] for writing.\nCheck to make sure %s directory exists\n", steam_filename, mani_log_directory.GetString());
					engine->LogPrint( log_string );
					return;
			}
		}

		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		time_now = localtime(&current_time);

		char	temp_string[4096];

		int	length = snprintf(temp_string, sizeof(temp_string), "M %02i/%02i/%04i - %02i:%02i:%02i: %s", 
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year + 1900,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec,
								log_string);

		filesystem->Write((void *) temp_string, length, filehandle);

		filesystem->Close(filehandle);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Update log mode
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(ManiLogMode)
{
	int	log_mode = atoi(mani_log_mode.GetString());

	// Setup log mode
	if (log_mode == 0) return;

	char full_path[512];
	snprintf(full_path, sizeof(full_path), "./cfg/%s/%s", 
		mani_path.GetString(),
		mani_log_directory.GetString()
		);

	filesystem->CreateDirHierarchy(full_path);

	if (log_mode == 1)
	{
		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		time_now = localtime(&current_time);

		// Construct filename
		if (map_count == -1)
		{
			char	filename[512];

			MMsg("Searching for old log file...\n");

			// Search for existing log files for todays date
			for (int i = 0; i < 1000; i++)
			{
				snprintf(filename, sizeof(filename), "./cfg/%s/%s/M%02i%02i%03i.log", 
									mani_path.GetString(),
									mani_log_directory.GetString(), 
									(int) time_now->tm_mon + 1, 
									(int) (time_now->tm_mday),
									i
									);

				if (!filesystem->FileExists(filename))
				{
					map_count = i;
					if (map_count == 1000)
					{
						map_count = 0;
					}

					break;
				}
			}

			if (map_count == -1) map_count = 0;
		}
		else
		{
			map_count ++;
			if (map_count == 1000)
			{
				map_count = 0;
			}
		}

		snprintf(mani_log_filename, sizeof(mani_log_filename), "./cfg/%s/%s/M%02i%02i%03i.log", 
									mani_path.GetString(),
									mani_log_directory.GetString(), 
									(int) time_now->tm_mon + 1, 
									(int) (time_now->tm_mday),
									map_count
									);

		FileHandle_t	filehandle;

		// Log to custom log file (same filename format as Valves)
		filehandle = filesystem->Open(mani_log_filename, "at+");
		if (filehandle == NULL)
		{
				MMsg("Failed to open log file [%s] for writing\n", mani_log_filename);
				return;
		}

		// Get round Valve breaking the FPrintf function
		char	temp_string[2048];

		int	length = snprintf(temp_string, sizeof(temp_string), "M %02i/%02i/%04i - %02i:%02i:%02i: Log file [%s] started for map [%s]\n", 
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec,
								mani_log_filename,
								current_map);

		filesystem->Write((void *) temp_string, length, filehandle);

		filesystem->Close(filehandle);

		return;
	}

	if (log_mode == 2)
	{
		// One big file
		snprintf(mani_log_filename, sizeof(mani_log_filename), "./cfg/%s/%s/adminlog.log", 
									mani_path.GetString(),
									mani_log_directory.GetString()
									);

	}


}

//---------------------------------------------------------------------------------
// Purpose: Update log mode
//---------------------------------------------------------------------------------
void WriteDebug 
(
 char *fmt,
 ...
)
{
	if (!gpManiGameType->DebugOn()) return;

	va_list		argptr;
	char		tempString[1024];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	char	filename[512];

	snprintf(filename, sizeof(filename), "./cfg/%s/%s/debug.log", 
									mani_path.GetString(),
									mani_log_directory.GetString()
									);

	// Log to custom log file (same filename format as Valves)
	FileHandle_t	filehandle;

	filehandle = filesystem->Open(filename, "at+");
	if (filehandle == NULL)
	{
		MMsg("Failed to open log file [%s] for writing\n", filename);
		return;
	}

	char		logString[1024];

	struct	tm	*time_now;
	time_t	current_time;

	time(&current_time);
	time_now = localtime(&current_time);

	int	length = snprintf(logString, sizeof(logString), "M %02i/%02i/%04i - %02i:%02i:%02i: %s", 
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year + 1900,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec,
								tempString);

	filesystem->Write((void *) logString, length, filehandle);
	filesystem->Close(filehandle);
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//---------------------------------------------------------------------------------
void ClientMsgSinglePlayer
(
 edict_t	*pEntity,
 const unsigned int seconds, 
 const int	level,
 const char* fmt, ...
 ) 
{
	tchar szBuf[256];
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vsnprintf(szBuf, sizeof(szBuf)-1, fmt, arg_ptr);
	va_end(arg_ptr);

	szBuf[sizeof(szBuf)-1] = 0;

	KeyValues *kv = new KeyValues("Msg");
	kv->SetString("title", szBuf);
	kv->SetString("Msg", "message");
	kv->SetColor("color", Color(255, 255, 255, 255)); // White
	kv->SetInt("level", level);
	kv->SetInt("time", seconds);
	helpers->CreateMessage(pEntity, DIALOG_MSG, kv, gpManiISPCCallback);
	kv->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//---------------------------------------------------------------------------------
void ClientMsg
(
 Color	*col,
 const unsigned int seconds, 
 const bool admin_only,
 const int	level,
 const char* fmt, ...
 ) 
{
	player_t	player;
	tchar szBuf[256];
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vsnprintf(szBuf, sizeof(szBuf)-1, fmt, arg_ptr);
	va_end(arg_ptr);
	Color	admin_only_colour(255, 0, 0, 255);

	szBuf[sizeof(szBuf)-1] = 0;

	for (int i = 1; i <= max_players; i++ )
	{
		player.index = i;
		if (!FindPlayerByIndex (&player))
		{
			continue;
		}

		if (player.is_bot)
		{
			continue;
		}

		if (admin_only)
		{
			if (gpManiClient->HasAccess(player.index, ADMIN, ADMIN_BASIC_ADMIN))
			{
				KeyValues *kv = new KeyValues("Msg");
				kv->SetString("title", szBuf);
				kv->SetString("Msg", "message");
				kv->SetColor("color", admin_only_colour); // Red
				kv->SetInt("level", level);
				kv->SetInt("time", seconds);
				helpers->CreateMessage(player.entity, DIALOG_MSG, kv, gpManiISPCCallback);
				kv->deleteThis();
			}
		}
		else
		{
			KeyValues *kv = new KeyValues("Msg");
			kv->SetString("title", szBuf);
			kv->SetString("Msg", "message");
			kv->SetColor("color", *col); 
			kv->SetInt("level", level);
			kv->SetInt("time", seconds);
			helpers->CreateMessage(player.entity, DIALOG_MSG, kv, gpManiISPCCallback);
			kv->deleteThis();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Dump text to a clients console
//---------------------------------------------------------------------------------
void PrintToClientConsole(edict_t *pEntity, char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to clients console
	engine->ClientPrintf(pEntity, tempString);
}

//---------------------------------------------------------------------------------
// Purpose: Output help text to clients
//---------------------------------------------------------------------------------
void OutputHelpText
(
 const int colour,
 player_t	*player_ptr,
 char		*fmt,
 ...
 )
{
	va_list		argptr;
	char		tempString[2048];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	if (!player_ptr)
	{
		OutputToConsole(player_ptr, "%s\n", tempString);
	}
	else
	{
		SayToPlayer(colour, player_ptr, "%s", tempString);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write Text to chat area
//---------------------------------------------------------------------------------
void UTIL_SayText(int colour, MRecipientFilter *mrf, const char *say_text)
{
#if defined ( GAME_CSGO )
	msg_buffer = engine->UserMessageBegin(mrf, text_message_index, "TextMsg" ); // Show TextMsg type user message
#else
	msg_buffer = engine->UserMessageBegin(mrf, text_message_index ); // Show TextMsg type user message
#endif

	msg_buffer->WriteByte(3); // Say area

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS)))
	{
		switch (colour)
		{
			case ORANGE_CHAT:break;
			case GREY_CHAT: msg_buffer->WriteByte(-1); break;
			case LIGHT_GREEN_CHAT: msg_buffer->WriteByte(3);break; // Light Green
			case GREEN_CHAT: msg_buffer->WriteByte(4);break; // Darker Green
			default :break;
		}
	}

	/*
	\x01 = White
\x02 = Dark Red
\x03 = White
\x04 = Dark Green
\x05 = Moss Green
\x06 = Lime Green
\x07/8/9 = Light Red
*/
	if ((gpManiGameType->IsGameType(MANI_GAME_CSGO)))
	{
		//switch (colour)
		//{
		//	case WHITE_TEXT: break;
		//	case WHITE2_TEXT:break;
		//	case RED_TEXT: msg_buffer->WriteByte(2); break;
		//	case DARK_GREEN_TEXT: msg_buffer->WriteByte(4);break; // Light Green
		//	case MOSS_GREEN_TEXT: msg_buffer->WriteByte(5);break; // Light Green
		//	case LIME_GREEN_TEXT: msg_buffer->WriteByte(6);break; // Light Green
		//	case LIGHT_RED_TEXT: msg_buffer->WriteByte(7);break; // Darker Green
		//	default :break;
		//}
	}

	msg_buffer->WriteString(say_text);
	engine->MessageEnd();
}

// Log to srcds core log
void UTILLogPrintf( char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024];
	
	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to server console
	engine->LogPrint( tempString );
}

bool UTIL_InterfaceMsg( void *ptr, char *interface_id, char *version)
{
	if (ptr)
	{
		MMsg("Interface %s @ %p\n", interface_id, ptr);
		return true;
	}

	MMsg("Interface %s version %s failed to load\n", interface_id, version);
	return false;
}

static 
bool vcmp(void *_addr1, void *_addr2, size_t len)
{
	unsigned char *addr1 = (unsigned char *)_addr1;
	unsigned char *addr2 = (unsigned char *)_addr2;

	for (size_t i=0; i<len; i++)
	{
		if (addr2[i] == '*')
			continue;
		if (addr1[i] != addr2[i])
			return false;
	}

	return true;
}

void FindConPrintf(void)
{
	ConCommandBase *pBase = NULL;
	unsigned char *ptr = NULL;

#ifdef GAME_ORANGE
	FnCommandCallback_t callback = NULL;
#else
	FnCommandCallback callback = NULL;
#endif

	int offs = 0;

#if !defined ( GAME_CSGO )
	pBase = g_pCVar->GetCommands();
	while (pBase)
	{
		if ( strcmp(pBase->GetName(), "echo") == 0 )
		{
			//callback = //*((FnCommandCallback *)((char *)pBase + offsetof(ConCommand, m_fnCommandCallback)));
			callback = ((ConCommand *)pBase)->m_fnCommandCallback;
			ptr = (unsigned char *)callback;
			if (vcmp(ptr, ENGINE486_SIG, SIGLEN))
			{
				offs = ENGINE486_OFFS;
			} else if (vcmp(ptr, ENGINE686_SIG, SIGLEN)) {
				offs = ENGINE686_OFFS;
			} else if (vcmp(ptr, ENGINEAMD_SIG, SIGLEN)) {
				offs = ENGINEAMD_OFFS;
			} else if (vcmp(ptr, ENGINEW32_SIG, SIGLEN)) {
				offs = ENGINEW32_OFFS;
			}
			if (!offs || ptr[offs-1] != IA32_CALL)
			{
				return;
			}
			//get the relative offset
			MMsg = *((CONPRINTF_FUNC *)(ptr + offs));
			//add the base offset, to the ip (which is the address+offset + 4 bytes for next instruction)
			MMsg = (CONPRINTF_FUNC)((unsigned long)MMsg + (unsigned long)(ptr + offs) + 4);
			Msg("Using conprintf\n");
			return;
		}
		pBase = const_cast<ConCommandBase *>(pBase->GetNext());
	}
#else
	pBase = g_pCVar->FindCommand("echo");
	callback = ((ConCommand *)pBase)->m_fnCommandCallback;
	ptr = (unsigned char *)callback;
	if (vcmp(ptr, ENGINE486_SIG, SIGLEN))
	{
		offs = ENGINE486_OFFS;
	} else if (vcmp(ptr, ENGINE686_SIG, SIGLEN)) {
		offs = ENGINE686_OFFS;
	} else if (vcmp(ptr, ENGINEAMD_SIG, SIGLEN)) {
		offs = ENGINEAMD_OFFS;
	} else if (vcmp(ptr, ENGINEW32_SIG, SIGLEN)) {
		offs = ENGINEW32_OFFS;
	}
	if (!offs || ptr[offs-1] != IA32_CALL)
	{
		return;
	}
	//get the relative offset
	MMsg = *((CONPRINTF_FUNC *)(ptr + offs));
	//add the base offset, to the ip (which is the address+offset + 4 bytes for next instruction)
	MMsg = (CONPRINTF_FUNC)((unsigned long)MMsg + (unsigned long)(ptr + offs) + 4);
	Msg("Using conprintf\n");
	return;

#endif

	Msg("Using Msg()\n");
	MMsg = (CONPRINTF_FUNC)Msg;
	return;

}

