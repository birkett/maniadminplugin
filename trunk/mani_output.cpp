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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
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
#include "mani_maps.h"
#include "mani_gametype.h"
#include "mani_output.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerPluginHelpers *helpers;
extern	IServerPluginCallbacks *gpManiAdminPlugin;
extern	IFileSystem	*filesystem;
extern	bool war_mode;
extern	int	max_players;
extern	bf_write *msg_buffer;
extern	int	text_message_index;

static	int	map_count = -1;
static	char mani_log_filename[512]="temp.log";


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

static void ManiLogMode ( ConVar *var, char const *pOldString );
static void WriteToManiLog ( char *log_string, char *steam_id);

ConVar mani_log_directory ("mani_log_directory", "mani_logs", 0, "This defines the directory to store admin logs in"); 
ConVar mani_log_mode ("mani_log_mode", "0", 0, "0 = to main valve log file, 1 = per map in mani_log_directory, 2 = log to one big file in mani_log_directory, 3 = per steam id in mani_log_directory", true, 0, true, 3, ManiLogMode); 

//---------------------------------------------------------------------------------
// Purpose: Say only to admin
//---------------------------------------------------------------------------------
void AdminSayToAdmin
(
player_t	*player,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[1024];
	char	final_string[2048];
	bool	found_admin = false;
	int		admin_index;

	player_t recipient_player;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	if (player->entity != NULL)
	{
		Q_snprintf(final_string, sizeof (final_string), "(ADMIN ONLY) %s: %s", player->name, tempString);
	}
	else
	{
		Q_snprintf(final_string, sizeof (final_string), "(ADMIN ONLY) CONSOLE: %s", tempString);
	}

	OutputToConsole(NULL, true, "%s\n", final_string);

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

		if (gpManiClient->IsAdmin(&recipient_player, &admin_index))
		{
			// This is an admin player
			mrf.AddPlayer(i);
//			OutputToConsole(recipient_player.entity, false, "%s\n", final_string);
			found_admin = true;
		}
	}

	if (found_admin)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
		msg_buffer->WriteByte(3); // Say area
		msg_buffer->WriteString(final_string);
		engine->MessageEnd();
	}

}

//---------------------------------------------------------------------------------
// Purpose: Say from player only to admin
//---------------------------------------------------------------------------------
void SayToAdmin
(
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
	int		admin_index;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	Q_snprintf(final_string, sizeof (final_string), "(TO ADMIN) %s: %s", player->name, tempString);
	OutputToConsole(NULL, true, "%s\n", final_string);

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
//				OutputToConsole(recipient_player.entity, false, "%s\n", final_string);
		}
		else
		{
			if (gpManiClient->IsAdmin(&recipient_player, &admin_index))
			{
				// This is an admin player
				mrf.AddPlayer(i);
				found_player = true;
//				OutputToConsole(recipient_player.entity, false, "%s\n", final_string);
			}
		}
	}

	if (found_player)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
		msg_buffer->WriteByte(3); // Say area
		msg_buffer->WriteString(final_string);
		engine->MessageEnd();
	}

}

//---------------------------------------------------------------------------------
// Purpose: Say admin string to all
//---------------------------------------------------------------------------------
void AdminSayToAll
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
	int		admin_index;
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (player->entity == NULL)
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, true, "%s\n", admin_final_string);

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

			is_admin = gpManiClient->IsAdmin(&server_player, &admin_index);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
//				OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
//				OutputToConsole(server_player.entity, false, "%s\n", non_admin_final_string);
			}
		}

		if (found_player)
		{
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteString(non_admin_final_string);
			engine->MessageEnd();
		}

		if (found_admin)
		{
			msg_buffer = engine->UserMessageBegin( &mrfadmin, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
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
//			OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteString(admin_final_string);
			engine->MessageEnd();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say admin string to all using different colour
//---------------------------------------------------------------------------------
void AdminSayToAllColoured
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
	int		admin_index;
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (player->entity == NULL)
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, true, "%s\n", admin_final_string);

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

			is_admin = gpManiClient->IsAdmin(&server_player, &admin_index);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
				if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
				}
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
				if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					OutputToConsole(server_player.entity, false, "%s\n", non_admin_final_string);
				}
			}
		}

		if (found_player)
		{
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteByte(3); // Green
			msg_buffer->WriteString(non_admin_final_string);
			engine->MessageEnd();
		}

		if (found_admin)
		{
			msg_buffer = engine->UserMessageBegin( &mrfadmin, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteByte(3); // Green
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
			if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
			{
				OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
			}
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(3); // Say area
			msg_buffer->WriteByte(3); // Green
			msg_buffer->WriteString(admin_final_string);
			engine->MessageEnd();
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
	int		admin_index;
	bool	found_player = false;
	bool	found_admin = false;

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	player_t	server_player;

	if (player->entity == NULL)
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(CONSOLE) : %s", tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(CONSOLE) %s", tempString);
	}
	else
	{
		Q_snprintf(admin_final_string, sizeof (admin_final_string), "(ADMIN) %s: %s", player->name, tempString);
		Q_snprintf(non_admin_final_string, sizeof (non_admin_final_string), "(ADMIN) %s", tempString);
	}

	OutputToConsole(NULL, true, "%s\n", admin_final_string);

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

			is_admin = gpManiClient->IsAdmin(&server_player, &admin_index);
			if (is_admin)
			{
				found_admin = true;
				mrfadmin.AddPlayer(i);
				if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
				}
			}
			else
			{
				found_player = true;
				mrf.AddPlayer(i);
				if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					OutputToConsole(server_player.entity, false, "%s\n", non_admin_final_string);
				}
			}
		}

		if (found_player)
		{
			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
			msg_buffer->WriteByte(4); // Center area
			msg_buffer->WriteString(non_admin_final_string);
			engine->MessageEnd();
		}

		if (found_admin)
		{
			msg_buffer = engine->UserMessageBegin( &mrfadmin, text_message_index ); // Show TextMsg type user message
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
			if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
			{
				OutputToConsole(server_player.entity, false, "%s\n", admin_final_string);
			}
		}

		if (found_player)
		{
			MRecipientFilter mrf;
			mrf.MakeReliable();
			mrf.AddAllPlayers(max_players);

			msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.AddAllPlayers(max_players);

	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Say to all
//---------------------------------------------------------------------------------
void SayToAll(bool echo, const char	*fmt, ...)
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	if (echo) OutputToConsole(NULL, true, "%s\n", tempString);

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

		if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			if (echo) OutputToConsole(server_player.entity, false, "%s\n", tempString);
		}
	}

	if (found_player)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
		msg_buffer->WriteByte(3); // Say area
		msg_buffer->WriteString(tempString);
		engine->MessageEnd();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say to Dead
//---------------------------------------------------------------------------------
void SayToDead(const char	*fmt, ...)
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	OutputToConsole(NULL, true, "%s\n", tempString);

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
			if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
			{
				OutputToConsole(recipient_player.entity, false, "%s\n", tempString);
			}
			continue;
		}

		if (recipient_player.is_dead)
		{
			mrf.AddPlayer(i);
			found_player = true;
			if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
			{
				OutputToConsole(recipient_player.entity, false, "%s\n", tempString);
			}

			continue;
		}
	}

	if (found_player)
	{
		msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
		msg_buffer->WriteByte(3); // Say area
		msg_buffer->WriteString(tempString);
		engine->MessageEnd();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Say to player
//---------------------------------------------------------------------------------
void SayToPlayer(player_t *player, const char	*fmt, ...)
{
	va_list		argptr;
	char		tempString[1024];
	player_t	recipient_player;
	
	if (war_mode)
	{
		return;
	}

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
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
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		OutputToConsole(player->entity, false, "%s\n", tempString);
	}
//	OutputToConsole(NULL, true, "%s\n", tempString);

	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
	msg_buffer->WriteByte(3); // Say area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Say string to specific teams
//---------------------------------------------------------------------------------
void SayToTeam
(
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
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

	OutputToConsole(NULL, true, "%s\n", tempString);
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
	msg_buffer->WriteByte(3); // Say area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();

}

//---------------------------------------------------------------------------------
// Purpose: Dump string to player console or server console depending on flag
//---------------------------------------------------------------------------------
void	OutputToConsole(edict_t *pEntity, bool svr_command, char *fmt, ...)
{
	va_list		argptr;
	char		tempString[2048];

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	if (svr_command || pEntity == NULL)
	{
		Msg("%s", tempString);
	}
	else
	{
		engine->ClientPrintf(pEntity, tempString);
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to server console
	engine->LogPrint( tempString );
}

//---------------------------------------------------------------------------------
// Purpose: Log who did what with timestamp to console
//---------------------------------------------------------------------------------
void LogCommand(edict_t *pEntity, char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024]="";
	char		tempString2[1024]="";
	char		user_details[128]="CONSOLE : ";
	char		steam_id[128]="CONSOLE";

	if(pEntity != NULL && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			const char *user_name = playerinfo->GetName();
			Q_strcpy(steam_id, playerinfo->GetNetworkIDString());
			Q_snprintf( user_details, sizeof(user_details), "[MANI_ADMIN_PLUGIN] Admin [%s] [%s] Executed : ", user_name, steam_id);
		}
	}	

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString2, sizeof(tempString2), fmt, argptr );
	va_end   ( argptr );

	Q_snprintf( tempString, sizeof(tempString), "%s %s", user_details, tempString2);  

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
		FileHandle_t	filehandle;

		if (log_mode == 1 || log_mode == 2)
		{
			// Log to custom log file (same filename format as Valves)
			filehandle = filesystem->Open(mani_log_filename, "at+");
			if (filehandle == NULL)
			{
					Msg("Failed to open log file [%s] for writing\n", mani_log_filename);
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

			Q_snprintf(steam_filename, sizeof(steam_filename), "./cfg/%s/%s/%s.log", 
									mani_path.GetString(),
									mani_log_directory.GetString(),
									steam_id
									);

			// Log to custom log file (same filename format as Valves)
			filehandle = filesystem->Open(steam_filename, "at+");
			if (filehandle == NULL)
			{
					Msg("Failed to open log file [%s] for writing\n", steam_filename);
					engine->LogPrint( log_string );
					return;
			}
		}

		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		time_now = localtime(&current_time);

		char	temp_string[4096];

		int	length = Q_snprintf(temp_string, sizeof(temp_string), "M %02i/%02i/%04i - %02i:%02i:%02i: %s", 
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
static void ManiLogMode ( ConVar *var, char const *pOldString )
{
	int	log_mode = Q_atoi(var->GetString());

	// Setup log mode
	if (log_mode == 0) return;

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

			Msg("Searching for old log file...\n");

			// Search for existing log files for todays date
			for (int i = 0; i < 1000; i++)
			{
				Q_snprintf(filename, sizeof(filename), "./cfg/%s/%s/M%02i%02i%03i.log", 
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

		Q_snprintf(mani_log_filename, sizeof(mani_log_filename), "./cfg/%s/%s/M%02i%02i%03i.log", 
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
				Msg("Failed to open log file [%s] for writing\n", mani_log_filename);
				return;
		}

		// Get round Valve breaking the FPrintf function
		char	temp_string[2048];

		int	length = Q_snprintf(temp_string, sizeof(temp_string), "M %02i/%02i/%04i - %02i:%02i:%02i: Log file [%s] started for map [%s]\n", 
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
		Q_snprintf(mani_log_filename, sizeof(mani_log_filename), "./cfg/%s/%s/adminlog.log", 
									mani_path.GetString(),
									mani_log_directory.GetString()
									);

	}


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
	_vsntprintf(szBuf, sizeof(szBuf)-1, fmt, arg_ptr);
	va_end(arg_ptr);

	szBuf[sizeof(szBuf)-1] = 0;

	KeyValues *kv = new KeyValues("msg");
	kv->SetString("title", szBuf);
	kv->SetString("msg", "message");
	kv->SetColor("color", Color(255, 255, 255, 255)); // White
	kv->SetInt("level", level);
	kv->SetInt("time", seconds);
	helpers->CreateMessage(pEntity, DIALOG_MSG, kv, gpManiAdminPlugin);
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
	int			admin_index;
	tchar szBuf[256];
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	_vsntprintf(szBuf, sizeof(szBuf)-1, fmt, arg_ptr);
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
			if (gpManiClient->IsAdmin(&player, &admin_index))
			{
				KeyValues *kv = new KeyValues("msg");
				kv->SetString("title", szBuf);
				kv->SetString("msg", "message");
				kv->SetColor("color", admin_only_colour); // Red
				kv->SetInt("level", level);
				kv->SetInt("time", seconds);
				helpers->CreateMessage(player.entity, DIALOG_MSG, kv, gpManiAdminPlugin);
				kv->deleteThis();
			}
		}
		else
		{
			KeyValues *kv = new KeyValues("msg");
			kv->SetString("title", szBuf);
			kv->SetString("msg", "message");
			kv->SetColor("color", *col); 
			kv->SetInt("level", level);
			kv->SetInt("time", seconds);
			helpers->CreateMessage(player.entity, DIALOG_MSG, kv, gpManiAdminPlugin);
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
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to clients console
	engine->ClientPrintf(pEntity, tempString);
}

//---------------------------------------------------------------------------------
// Purpose: Output help text to clients
//---------------------------------------------------------------------------------
void OutputHelpText
(
 player_t	*player_ptr,
 bool		to_server_console,
 char		*fmt,
 ...
 )
{
	va_list		argptr;
	char		tempString[2048];

	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	if (to_server_console)
	{
		OutputToConsole(player_ptr->entity, to_server_console, "%s\n", tempString);
	}
	else
	{
		SayToPlayer(player_ptr, "%s\n", tempString);
	}
}
