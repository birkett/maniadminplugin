//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_mainclass.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_gametype.h"
#include "mani_util.h"
#include "mani_commands.h"
#include "mani_help.h"
#include "mani_autokickban.h"
#include "shareddefs.h"
#include "mani_playerkick.h"
#include "mani_handlebans.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern IFileSystem	*filesystem;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

static int sort_autokick_steam ( const void *m1,  const void *m2);
static int sort_autokick_ip ( const void *m1,  const void *m2);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiAutoKickBan	g_ManiAutoKickBan;
ManiAutoKickBan	*gpManiAutoKickBan;

ManiAutoKickBan::ManiAutoKickBan()
{
	// Init
	autokick_ip_list = NULL;
	autokick_steam_list = NULL;
	autokick_name_list = NULL;
	autokick_pname_list = NULL;

	autokick_ip_list_size = 0;
	autokick_steam_list_size = 0;
	autokick_name_list_size = 0;
	autokick_pname_list_size = 0;

	gpManiAutoKickBan = this;
}

ManiAutoKickBan::~ManiAutoKickBan()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Init stuff for plugin load
//---------------------------------------------------------------------------------
void ManiAutoKickBan::CleanUp(void)
{
	FreeList ((void **) &autokick_ip_list, &autokick_ip_list_size);
	FreeList ((void **) &autokick_steam_list, &autokick_steam_list_size);
	FreeList ((void **) &autokick_name_list, &autokick_name_list_size);
	FreeList ((void **) &autokick_pname_list, &autokick_pname_list_size);	
}

//---------------------------------------------------------------------------------
// Purpose: Init stuff for plugin load
//---------------------------------------------------------------------------------
void ManiAutoKickBan::Load(void)
{


}

//---------------------------------------------------------------------------------
// Purpose: Level load
//---------------------------------------------------------------------------------
void ManiAutoKickBan::LevelInit(void)
{
	FileHandle_t file_handle;
	char	autokickban_id[512];
	char	base_filename[256];

	this->CleanUp();

	//Get autokickban IP list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_ip.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Did not load autokick_ip.txt\n");
	}
	else
	{
//		MMsg("autokickban IP list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, true, true)) continue;
			AddAutoKickIP(autokickban_id);
		}
		filesystem->Close(file_handle);
		qsort(autokick_ip_list, autokick_ip_list_size, sizeof(autokick_ip_t), sort_autokick_ip); 
	}

	//Get autokickban Steam list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_steam.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Did not load autokick_steam.txt\n");
	}
	else
	{
//		MMsg("autokickban Steam list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, true, true)) continue;
			AddAutoKickSteamID(autokickban_id);
		}
		filesystem->Close(file_handle);
		qsort(autokick_steam_list, autokick_steam_list_size, sizeof(autokick_steam_t), sort_autokick_steam); 
	}

	//Get autokickban Name list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_name.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Did not load autokick_name.txt\n");
	}
	else
	{
//		MMsg("autokickban Name list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, true, false)) continue;
			AddAutoKickName(autokickban_id);
		}
		filesystem->Close(file_handle);
	}

	//Get autokickban PName list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/autokick_pname.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Did not load autokick_pname.txt\n");
	}
	else
	{
//		MMsg("autokickban PName list\n");
		while (filesystem->ReadLine (autokickban_id, sizeof(autokickban_id), file_handle) != NULL)
		{
			if (!ParseLine(autokickban_id, true, false)) continue;
			AddAutoKickPName(autokickban_id);
		}
		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Client Disconnected
//---------------------------------------------------------------------------------
void ManiAutoKickBan::ClientDisconnect(player_t *player_ptr)
{
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on connect
//---------------------------------------------------------------------------------
bool ManiAutoKickBan::NetworkIDValidated(player_t	*player_ptr)
{
	autokick_steam_t	autokick_steam_key;
	autokick_steam_t	*found_steam = NULL;
	autokick_ip_t		autokick_ip_key;
	autokick_ip_t		*found_ip = NULL;

	char	kick_cmd[512];

	if (war_mode)
	{
		return true;
	}

	if (FStrEq(player_ptr->steam_id, "BOT"))
	{
		// Is Bot
		return true;
	}

	if (gpManiClient->HasAccess(player_ptr->index, IMMUNITY, IMMUNITY_TAG, false, true))
	{
		return true;
	}

	Q_strcpy (autokick_steam_key.steam_id, player_ptr->steam_id);

	/* Check steam ID's first */
	if (autokick_steam_list_size != 0)
	{
		found_steam = (autokick_steam_t *) bsearch
						(
						&autokick_steam_key, 
						autokick_steam_list, 
						autokick_steam_list_size, 
						sizeof(autokick_steam_t), 
						sort_autokick_steam
						);

		if (found_steam != NULL)
		{
			// Matched steam id
			if (found_steam->kick)
			{
				player_ptr->user_id = engine->GetPlayerUserId(player_ptr->entity);
				PrintToClientConsole(player_ptr->entity, "You have been autokicked\n");
				gpManiPlayerKick->AddPlayer( player_ptr->index, 0.5f, "You were autokicked" );
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player_ptr->user_id);
				LogCommand (NULL, "Kick (Bad Steam ID) [%s] [%s] %s\n", player_ptr->name, player_ptr->steam_id, kick_cmd);
				return false;
			}
		}
	}

	if (autokick_ip_list_size != 0)
	{
		Q_strcpy(autokick_ip_key.ip_address, player_ptr->ip_address);
		found_ip = (autokick_ip_t *) bsearch
						(
						&autokick_ip_key, 
						autokick_ip_list, 
						autokick_ip_list_size, 
						sizeof(autokick_ip_t), 
						sort_autokick_ip
						);

		if (found_ip != NULL)
		{
			// Matched ip address
			if (found_ip->kick)
			{
				player_ptr->user_id = engine->GetPlayerUserId(player_ptr->entity);
				PrintToClientConsole(player_ptr->entity, "You have been autokicked\n");
				gpManiPlayerKick->AddPlayer( player_ptr->index, 0.5f, "You were autokicked" );
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player_ptr->user_id);
				LogCommand (NULL, "Kick (Bad IP Address) [%s] [%s] %s\n", player_ptr->name, player_ptr->steam_id, kick_cmd);
				return false;
			}
		}
	}

	if (player_ptr->player_info)
	{
		for (int i = 0; i < autokick_name_list_size; i++)
		{
			if (FStrEq(autokick_name_list[i].name, player_ptr->name))
			{
				// Matched name of player
				if (autokick_name_list[i].kick)
				{
					PrintToClientConsole(player_ptr->entity, "You have been autokicked\n");
					gpManiPlayerKick->AddPlayer( player_ptr->index, 0.5f, "You were autokicked" );
					snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player_ptr->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s\n", player_ptr->name, player_ptr->steam_id, kick_cmd);
					return false;
				}
				else if (autokick_name_list[i].ban && !IsLAN())
				{
					// Ban by user id
					PrintToClientConsole(player_ptr->entity, "You have been auto banned\n");
					LogCommand (NULL,"Ban (Bad Name) [%s] [%s]\n", player_ptr->name, player_ptr->steam_id);
					gpManiHandleBans->AddBan ( player_ptr, player_ptr->steam_id, "MAP", autokick_name_list[i].ban_time, "Banned (Bad Name)", "Bad Name" );
					gpManiHandleBans->WriteBans();
					return false;
				}
			}
		}

		for (int i = 0; i < autokick_pname_list_size; i++)
		{
			if (NULL != Q_stristr(player_ptr->name, autokick_pname_list[i].pname))
			{
				// Matched name of player
				if (autokick_pname_list[i].kick)
				{
					PrintToClientConsole(player_ptr->entity, "You have been autokicked\n");
					gpManiPlayerKick->AddPlayer( player_ptr->index, 0.5f, "You were autokicked" );
					snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were autokicked\n", player_ptr->user_id);
					LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s\n", player_ptr->name, player_ptr->steam_id, kick_cmd);
					return false;
				}
				else if (autokick_pname_list[i].ban && !IsLAN())
				{
					// Ban by user id
					PrintToClientConsole(player_ptr->entity, "You have been auto banned\n");
					LogCommand (NULL,"Ban (Bad Name - partial) [%s] [%s]\n", player_ptr->name, player_ptr->steam_id);
					gpManiHandleBans->AddBan ( player_ptr, player_ptr->steam_id, "MAP", autokick_pname_list[i].ban_time, "Banned (Bad Name)", "Bad Name" );
					gpManiHandleBans->WriteBans();
					return false;
				}
			}
		}

	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autoban_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoBanName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	autokick_name_t	autokick_name;
	bool perm_ban = true;
	bool temp_ban = true;

	if (player_ptr)
	{
		// Check if player is admin
		perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
		temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);

		if (!(temp_ban || perm_ban)) return PLUGIN_BAD_ADMIN;
	}

	int argc = gpCmd->Cmd_Argc();

	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int ban_time = 0;

	if (argc == 3)
	{
		ban_time = atoi(gpCmd->Cmd_Argv(2));
		if (ban_time < 0)
		{
			ban_time = 0;
		}
	}

	// If if only temp ban if it's within limits
	if (!perm_ban)
	{
		if (ban_time == 0 || ban_time > mani_admin_temp_ban_time_limit.GetInt())
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 2581, "%i", mani_admin_temp_ban_time_limit.GetInt()));
			return PLUGIN_STOP;
		}
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_name_list[i].name))
		{
			autokick_name_list[i].ban = true;
			autokick_name_list[i].ban_time = ban_time;
			autokick_name_list[i].kick = false;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated player [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));
			WriteNameList("autokick_name.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_name.name, gpCmd->Cmd_Argv(1));
	autokick_name.ban = true;
	autokick_name.ban_time = ban_time;
	autokick_name.kick = false;

	AddToList((void **) &autokick_name_list, sizeof(autokick_name_t), &autokick_name_list_size);
	autokick_name_list[autokick_name_list_size - 1] = autokick_name;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added player [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));

	WriteNameList("autokick_name.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoKickName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	autokick_name_t	autokick_name;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	int argc = gpCmd->Cmd_Argc();

	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if name is already in list
	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_name_list[i].name))
		{
			autokick_name_list[i].ban = false;
			autokick_name_list[i].ban_time = 0;
			autokick_name_list[i].kick = true;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated player [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));

			WriteNameList("autokick_name.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_name.name, gpCmd->Cmd_Argv(1));

	autokick_name.ban = false;
	autokick_name.ban_time = 0;
	autokick_name.kick = true;

	AddToList((void **) &autokick_name_list, sizeof(autokick_name_t), &autokick_name_list_size);
	autokick_name_list[autokick_name_list_size - 1] = autokick_name;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added player [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));

	WriteNameList("autokick_name.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autoban_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoBanPName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	autokick_pname_t	autokick_pname;

	bool perm_ban = true;
	bool temp_ban = true;

	if (player_ptr)
	{
		// Check if player is admin
		perm_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode);
		temp_ban = gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN, war_mode);

		if (!(temp_ban || perm_ban)) return PLUGIN_BAD_ADMIN;
	}

	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int ban_time = 0;

	if (argc == 3)
	{
		ban_time = atoi(gpCmd->Cmd_Argv(2));
		if (ban_time < 0)
		{
			ban_time = 0;
		}
	}

	// If if only temp ban if it's within limits
	if (!perm_ban)
	{
		if (ban_time == 0 || ban_time > mani_admin_temp_ban_time_limit.GetInt())
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 2581, "%i", mani_admin_temp_ban_time_limit.GetInt()));
			return PLUGIN_STOP;
		}
	}

	// Check if name is already in list
	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_pname_list[i].pname))
		{
			autokick_pname_list[i].ban = true;
			autokick_pname_list[i].ban_time = ban_time;
			autokick_pname_list[i].kick = false;
			
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated player [%s] to autokick_pname.txt\n", gpCmd->Cmd_Argv(1));

			WritePNameList("autokick_pname.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_pname.pname, gpCmd->Cmd_Argv(1));
	autokick_pname.ban = true;
	autokick_pname.ban_time = ban_time;
	autokick_pname.kick = false;

	AddToList((void **) &autokick_pname_list, sizeof(autokick_pname_t), &autokick_pname_list_size);
	autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added player [%s] to autokick_pname.txt\n", gpCmd->Cmd_Argv(1));

	WritePNameList("autokick_pname.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoKickPName(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	autokick_pname_t	autokick_pname;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if name is already in list
	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_pname_list[i].pname))
		{
			autokick_pname_list[i].ban = false;
			autokick_pname_list[i].ban_time = 0;
			autokick_pname_list[i].kick = true;
			
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated player [%s] to autokick_pname.txt\n", gpCmd->Cmd_Argv(1));

			WritePNameList("autokick_pname.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_pname.pname, gpCmd->Cmd_Argv(1));
	autokick_pname.ban = false;
	autokick_pname.ban_time = 0;
	autokick_pname.kick = true;

	AddToList((void **) &autokick_pname_list, sizeof(autokick_pname_t), &autokick_pname_list_size);
	autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added player [%s] to autokick_pname.txt\n", gpCmd->Cmd_Argv(1));

	WritePNameList("autokick_pname.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoKickSteam(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	autokick_steam_t	autokick_steam;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if steam id is already in list
	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_steam_list[i].steam_id))
		{
			autokick_steam_list[i].kick = true;
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Steam ID [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated steam [%s] to autokick_steam.txt\n", gpCmd->Cmd_Argv(1));
			WriteSteamList("autokick_steam.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_steam.steam_id, gpCmd->Cmd_Argv(1));
	autokick_steam.kick = true;

	AddToList((void **) &autokick_steam_list, sizeof(autokick_steam_t), &autokick_steam_list_size);
	autokick_steam_list[autokick_steam_list_size - 1] = autokick_steam;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Steam ID [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added steam id [%s] to autokick_steam.txt\n", gpCmd->Cmd_Argv(1));

	qsort(autokick_steam_list, autokick_steam_list_size, sizeof(autokick_steam_t), sort_autokick_steam); 
	WriteSteamList("autokick_steam.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_autokick_ip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoKickIP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	autokick_ip_t	autokick_ip;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if ip is already in list
	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_ip_list[i].ip_address))
		{
			autokick_ip_list[i].kick = true;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: IP address [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated ip address [%s] to autokick_ip.txt\n", gpCmd->Cmd_Argv(1));

			WriteIPList("autokick_ip.txt");
			return PLUGIN_STOP;
		}
	}

	Q_strcpy(autokick_ip.ip_address, gpCmd->Cmd_Argv(1));
	autokick_ip.kick = true;

	AddToList((void **) &autokick_ip_list, sizeof(autokick_ip_t), &autokick_ip_list_size);
	autokick_ip_list[autokick_ip_list_size - 1] = autokick_ip;

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: IP address [%s] added", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Added ip address [%s] to autokick_ip.txt\n", gpCmd->Cmd_Argv(1));

	qsort(autokick_ip_list, autokick_ip_list_size, sizeof(autokick_ip_t), sort_autokick_ip); 

	WriteIPList("autokick_ip.txt");
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unauto_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaUnAutoName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if name is already in list
	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_name_list[i].name))
		{
			autokick_name_list[i].ban = false;
			autokick_name_list[i].ban_time = 0;
			autokick_name_list[i].kick = false;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated player [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));

			WriteNameList("autokick_name.txt");
			return PLUGIN_STOP;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Player [%s] not found", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Player [%s] not found\n", gpCmd->Cmd_Argv(1));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unauto_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaUnAutoPName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if name is already in list
	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_pname_list[i].pname))
		{
			autokick_pname_list[i].ban = false;
			autokick_pname_list[i].ban_time = 0;
			autokick_pname_list[i].kick = false;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated partial name [%s] to autokick_name.txt\n", gpCmd->Cmd_Argv(1));

			WritePNameList("autokick_pname.txt");
			return PLUGIN_STOP;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Partial name [%s] not found", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Partial name [%s] not found\n", gpCmd->Cmd_Argv(1));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unauto_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaUnAutoSteam(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if steam id is already in list
	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_steam_list[i].steam_id))
		{
			autokick_steam_list[i].kick = false;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Steam ID [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated steam id [%s] to autokick_steam.txt\n", gpCmd->Cmd_Argv(1));

			WriteSteamList("autokick_steam.txt");
			return PLUGIN_STOP;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Steam ID [%s] not found", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "Steam ID [%s] not found\n", gpCmd->Cmd_Argv(1));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unauto_ip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaUnAutoIP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	int argc = gpCmd->Cmd_Argc();
	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Check if ip is already in list
	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (FStrEq(gpCmd->Cmd_Argv(1), autokick_ip_list[i].ip_address))
		{
			autokick_ip_list[i].kick = false;

			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: ip address [%s] updated", gpCmd->Cmd_Argv(1));
			LogCommand (player_ptr, "Updated ip address [%s] to autokick_ip.txt\n", gpCmd->Cmd_Argv(1));

			WriteIPList("autokick_ip.txt");
			return PLUGIN_STOP;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: IP address [%s] not found", gpCmd->Cmd_Argv(1));
	LogCommand (player_ptr, "IP address [%s] not found\n", gpCmd->Cmd_Argv(1));

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Write name list to file
//---------------------------------------------------------------------------------
void	ManiAutoKickBan::WriteNameList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	//if (filesystem->FileExists( base_filename))
	//{
	//	filesystem->RemoveFile(base_filename);
	//	if (filesystem->FileExists( base_filename))
	//	{
	//		MMsg("Failed to delete %s\n", filename_string);
	//	}
	//}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		MMsg("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		gpManiAdminPlugin->PrintHeader ( file_handle, filename_string, "list of names that are to be kicked/banned" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// Put the name you wish to kick/ban in quotes.\n" );
		filesystem->FPrintf ( file_handle, "// Then whether to kick (k) or ban (b)\n" );
		filesystem->FPrintf ( file_handle, "// Lastly put the amount of time to ban (optional)\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// \"spek\" k\n" );
		filesystem->FPrintf ( file_handle, "// \"spek\" b 60\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		// Write file in human readable text format
		for (int i = 0; i < autokick_name_list_size; i ++)
		{
			char ban_string[128];

			if (!autokick_name_list[i].ban && !autokick_name_list[i].kick) continue;
			snprintf(ban_string , sizeof(ban_string), "b %i\n", autokick_name_list[i].ban_time);

			char	temp_string[512];
			int		temp_length = snprintf(temp_string, sizeof(temp_string), "\"%s\" %s", autokick_name_list[i].name, (autokick_name_list[i].kick)? "k\n":ban_string);											

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				MMsg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write pname list to file
//---------------------------------------------------------------------------------
void	ManiAutoKickBan::WritePNameList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	//if (filesystem->FileExists( base_filename))
	//{
	//	filesystem->RemoveFile(base_filename);
	//	if (filesystem->FileExists( base_filename))
	//	{
	//		MMsg("Failed to delete %s\n", filename_string);
	//	}
	//}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		MMsg("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		gpManiAdminPlugin->PrintHeader ( file_handle, filename_string, "list of name matches that are to be kicked/banned" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// Put the partial name you wish to kick/ban in quotes.\n" );
		filesystem->FPrintf ( file_handle, "// Then whether to kick (k) or ban (b)\n" );
		filesystem->FPrintf ( file_handle, "// Lastly put the amount of time to ban (optional)\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// \"spek\" k\n" );
		filesystem->FPrintf ( file_handle, "// \"spek\" b 60\n" );
		filesystem->FPrintf ( file_handle, "//\n" );		
		// Write file in human readable text format
		for (int i = 0; i < autokick_pname_list_size; i ++)
		{
			char ban_string[128];
			if (!autokick_pname_list[i].ban && !autokick_pname_list[i].kick) continue;

			snprintf(ban_string , sizeof(ban_string), "b %i\n", autokick_pname_list[i].ban_time);

			char	temp_string[512];
			int		temp_length = snprintf(temp_string, sizeof(temp_string), "\"%s\" %s", autokick_pname_list[i].pname, (autokick_pname_list[i].kick)? "k\n": ban_string);											

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
			{
				MMsg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write ip list to file
//---------------------------------------------------------------------------------
void	ManiAutoKickBan::WriteIPList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	//if (filesystem->FileExists( base_filename))
	//{
	//	filesystem->RemoveFile(base_filename);
	//	if (filesystem->FileExists( base_filename))
	//	{
	//		MMsg("Failed to delete %s\n", filename_string);
	//	}
	//}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		MMsg("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		gpManiAdminPlugin->PrintHeader ( file_handle, filename_string, "list of IPs that are to be kicked/banned" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// Put the IP you wish to kick/ban in quotes.\n" );
		filesystem->FPrintf ( file_handle, "// Then whether to kick (k) or ban (b)\n" );
		filesystem->FPrintf ( file_handle, "// Lastly put the amount of time to ban (optional)\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// \"192.168.0.2\" k\n" );
		filesystem->FPrintf ( file_handle, "// \"192.168.0.2\" b 60\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		// Write file in human readable text format
		for (int i = 0; i < autokick_ip_list_size; i ++)
		{
			if (!autokick_ip_list[i].kick) continue;

			char	temp_string[512];
			int		temp_length = snprintf(temp_string, sizeof(temp_string), "%s k\n", autokick_ip_list[i].ip_address);

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)											
			{
				MMsg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Write steam list to file
//---------------------------------------------------------------------------------
void	ManiAutoKickBan::WriteSteamList(char *filename_string)
{
	char	base_filename[1024];
	FileHandle_t file_handle;

	// Check if file exists, create a new one if it doesn't
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/%s", mani_path.GetString(), filename_string);

	//if (filesystem->FileExists( base_filename))
	//{
	//	filesystem->RemoveFile(base_filename);
	//	if (filesystem->FileExists( base_filename))
	//	{
	//		MMsg("Failed to delete %s\n", filename_string);
	//	}
	//}

	file_handle = filesystem->Open(base_filename,"wt",NULL);
	if (file_handle == NULL)
	{
		MMsg("Failed to open %s for writing\n", filename_string);
	}
	else
	{
		gpManiAdminPlugin->PrintHeader ( file_handle, filename_string, "list of steam ids that are to be kicked/banned" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// Put the steamid you wish to kick/ban in quotes.\n" );
		filesystem->FPrintf ( file_handle, "// Then whether to kick (k) or ban (b)\n" );
		filesystem->FPrintf ( file_handle, "// Lastly put the amount of time to ban (optional)\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		filesystem->FPrintf ( file_handle, "// \"STEAM_0:1:0000001\" k\n" );
		filesystem->FPrintf ( file_handle, "// \"STEAM_0:1:0000001\" b 60\n" );
		filesystem->FPrintf ( file_handle, "//\n" );
		// Write file in human readable text format
		for (int i = 0; i < autokick_steam_list_size; i ++)
		{
			if (!autokick_steam_list[i].kick) continue;

			char	temp_string[512];
			int		temp_length = snprintf(temp_string, sizeof(temp_string), "%s k\n", autokick_steam_list[i].steam_id);

			if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)											
			{
				MMsg("Failed to write to %s!!\n", filename_string);
				filesystem->Close(file_handle);
				break;
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick IP string
//---------------------------------------------------------------------------------

void ManiAutoKickBan::AddAutoKickIP(char *details)
{
	char	ip_address[128];
	autokick_ip_t	autokick_ip;

	if ( !details || (details[0]==0) )
		return;

	autokick_ip.kick = true; // they are in the list ... they are to be autokicked.
	Q_strcpy(autokick_ip.ip_address, "");
	Q_strcpy(ip_address, "");

	int i = 0;
	int j = 0;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			ip_address[j] = '\0';
			i--;
			break;
		}

		if ( details[i] == '\"' )
			i++;

		// If reached space or tab break out of loop
		if (details[i] == ' ' ||
			details[i] == '\t')
		{
			ip_address[j] = '\0';
			break;
		}

		ip_address[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_ip.ip_address, ip_address);

	//MMsg("%s ", ip_address);

	i++;

	//while (details[i] != '\0')
	//{
	//	switch (details[i])
	//	{
	//		case ('k') : autokick_ip.kick = true; break;
	//		default : break;
	//	}

	//	i++;

	//	if (autokick_ip.kick)
	//	{
	//		break;
	//	}
	//}

	if (!AddToList((void **) &autokick_ip_list, sizeof(autokick_ip_t), &autokick_ip_list_size))
		return;

	autokick_ip_list[autokick_ip_list_size - 1] = autokick_ip;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Steam ID string
//---------------------------------------------------------------------------------

void ManiAutoKickBan::AddAutoKickSteamID(char *details)
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	autokick_steam_t	autokick_steam;

	if ( !details || (details[0]==0) )
		return;

	// default admin to have absolute power
	autokick_steam.kick = true; // they are in the list ... they are to be autokicked.
	Q_strcpy(autokick_steam.steam_id, "");
	Q_strcpy(steam_id, "");

	int i = 0;
	int j = 0;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			steam_id[j] = '\0';
			i--;
			break;
		}

		if ( details[i] == '\"' )
			i++;

		// If reached space or tab break out of loop
		if (details[i] == ' ' ||
			details[i] == '\t')
		{
			steam_id[j] = '\0';
			break;
		}



		steam_id[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_steam.steam_id, steam_id);

	//MMsg("%s ", steam_id);

	//i++;

	//while (details[i] != '\0')
	//{
	//	switch (details[i])
	//	{
	//		case ('k') : autokick_steam.kick = true; break;
	//		default : break;
	//	}

	//	i++;

	//	if (autokick_steam.kick)
	//	{
	//		break;
	//	}
	//}

	if (!AddToList((void **) &autokick_steam_list, sizeof(autokick_steam_t), &autokick_steam_list_size))
		return;

	autokick_steam_list[autokick_steam_list_size - 1] = autokick_steam;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Name string
//---------------------------------------------------------------------------------

void ManiAutoKickBan::AddAutoKickName(char *details)
{
	char	name[MAX_PLAYER_NAME_LENGTH];
	autokick_name_t	autokick_name;

	if ( !details || (details[0]==0) )
		return;

	// default admin to have absolute power
	autokick_name.ban = false;
	autokick_name.ban_time = 0;
	autokick_name.kick = false;
	Q_strcpy(autokick_name.name, "");
	Q_strcpy(name, "");

	int i = 0;
	int j = 0;

	// Filter 
	while (details[i] != '\"' && details[i] != '\0')
		i++;


	if (details[i] == '\0')
	{
		return;
	}

	i++;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			name[j] = '\0';
			i--;
			break;
		}

		// If reached space or tab break out of loop
		if (details[i] == '\"')
		{
			name[j] = '\0';
			break;
		}

		name[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_name.name, name);

	//MMsg("%s ", name);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_name.kick = true; break;
			case ('b') : autokick_name.ban = true; break;
			default : break;
		}

		i++;

		if (autokick_name.ban)
		{
			// Need to get the time in minutes
			j = 0;
			char time_string[512];

			while(details[i] != '\0')
			{

				if (details[i] == ' ' ||
					details[i] == '\t')
				{
					i++;
					continue;
				}

				time_string[j++] = details[i++];
				if (j == sizeof(time_string))
				{
					j--;
					break;
				}
			}

			time_string[j] = '\0';
			autokick_name.ban_time = atoi(time_string);
			break;
		}
	}

	if (!autokick_name.ban && !autokick_name.kick)
	{
		autokick_name.kick = true;
	}

	if (autokick_name.ban)
	{
		if (autokick_name.ban_time == 0)
		{
			//MMsg("Ban permanent");
		}
		else
		{
			//MMsg("Ban %i minutes", autokick_name.ban_time);
		}
	}

	if (!AddToList((void **) &autokick_name_list, sizeof(autokick_name_t), &autokick_name_list_size))
		return;

	autokick_name_list[autokick_name_list_size - 1] = autokick_name;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Autokick Name string
//---------------------------------------------------------------------------------

void ManiAutoKickBan::AddAutoKickPName(char *details)
{
	char	name[MAX_PLAYER_NAME_LENGTH];
	autokick_pname_t	autokick_pname;

	// default admin to have absolute power
	autokick_pname.ban = false;
	autokick_pname.ban_time = 0;
	autokick_pname.kick = false;
	Q_strcpy(autokick_pname.pname, "");
	Q_strcpy(name, "");

	int i = 0;
	int j = 0;

	// Filter 
	while (details[i] != '\"' && details[i] != '\0')
	{
		i++;
	}

	if (details[i] == '\0')
	{
		return;
	}

	i++;

	for (;;)
	{
		if (details[i] == '\0')
		{
			// No more data
			name[j] = '\0';
			i--;
			break;
		}

		// If reached space or tab break out of loop
		if (details[i] == '\"')
		{
			name[j] = '\0';
			break;
		}

		name[j] = details[i];
		i++;
		j++;
	}

	Q_strcpy(autokick_pname.pname, name);

	//MMsg("%s ", name);

	i++;

	while (details[i] != '\0')
	{
		switch (details[i])
		{
			case ('k') : autokick_pname.kick = true; break;
			case ('b') : autokick_pname.ban = true; break;
			default : break;
		}

		i++;

		if (autokick_pname.ban)
		{
			// Need to get the time in minutes
			j = 0;
			char time_string[512]="0"; // default to perma

			while(details[i] != '\0')
			{

				if (details[i] == ' ' ||
					details[i] == '\t')
				{
					i++;
					continue;
				}

				time_string[j++] = details[i++];
				if (j == sizeof(time_string))
				{
					j--;
					break;
				}
			}

			time_string[j] = '\0';
			autokick_pname.ban_time = atoi(time_string);
			break;
		}
	}

	if (!autokick_pname.ban && !autokick_pname.kick)
	{
		autokick_pname.kick = true;
	}

	if (autokick_pname.ban)
	{
		if (autokick_pname.ban_time == 0)
		{
			//MMsg("Ban permanent");
		}
		else
		{
			//MMsg("Ban %i minutes", autokick_pname.ban_time);
		}
	}
	else
	{
		//MMsg("Kick");
	}

	if (!AddToList((void **) &autokick_pname_list, sizeof(autokick_pname_t), &autokick_pname_list_size))
		return;

	autokick_pname_list[autokick_pname_list_size - 1] = autokick_pname;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_name command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoShowName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current Names on the autokick/ban list\n\n");
	OutputToConsole(player_ptr, "Name                           Kick   Ban    Ban Time\n");

	char	name[512];
	char	ban_time[20];

	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (!autokick_name_list[i].ban && !autokick_name_list[i].kick)
		{
			continue;
		}

		Q_strcpy(ban_time,"");
		if (autokick_name_list[i].ban)
		{
			if (autokick_name_list[i].ban_time == 0)
			{
				Q_strcpy (ban_time, "Permanent");
			}
			else
			{
				snprintf(ban_time, sizeof(ban_time), "%i minute%s", 
											autokick_name_list[i].ban_time, 
											(autokick_name_list[i].ban_time == 1) ? "":"s");
			}
		}

		snprintf(name, sizeof(name), "\"%s\"", autokick_name_list[i].name);
		OutputToConsole(player_ptr, "%-30s %-6s %-6s %s\n", 
					name,
					(autokick_name_list[i].kick) ? "YES":"NO",
					(autokick_name_list[i].ban) ? "YES":"NO",
					ban_time
					);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_pname command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoShowPName(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current partial names on the autokick/ban list\n\n");
	OutputToConsole(player_ptr, "Partial Name                   Kick   Ban    Ban Time\n");

	char	name[512];
	char	ban_time[20];

	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (!autokick_pname_list[i].ban && !autokick_pname_list[i].kick)
		{
			continue;
		}

		Q_strcpy(ban_time,"");
		if (autokick_pname_list[i].ban)
		{
			if (autokick_pname_list[i].ban_time == 0)
			{
				Q_strcpy (ban_time, "Permanent");
			}
			else
			{
				snprintf(ban_time, sizeof(ban_time), "%i minute%s", 
											autokick_pname_list[i].ban_time, 
											(autokick_pname_list[i].ban_time == 1) ? "":"s");
			}
		}

		snprintf(name, sizeof(name), "\"%s\"", autokick_pname_list[i].pname);
		OutputToConsole(player_ptr, "%-30s %-6s %-6s %s\n", 
					name,
					(autokick_pname_list[i].kick) ? "YES":"NO",
					(autokick_pname_list[i].ban) ? "YES":"NO",
					ban_time
					);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_steam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoShowSteam(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current steam ids on the autokick/ban list\n\n");
	OutputToConsole(player_ptr, "Steam ID\n");

	for (int i = 0; i < autokick_steam_list_size; i++)
	{
		if (!autokick_steam_list[i].kick)
		{
			continue;
		}

		OutputToConsole(player_ptr, "%s\n", autokick_steam_list[i].steam_id);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_ashow_ip command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiAutoKickBan::ProcessMaAutoShowIP(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current IP addresses on the autokick/ban list\n\n");
	OutputToConsole(player_ptr, "IP Address\n");

	for (int i = 0; i < autokick_ip_list_size; i++)
	{
		if (!autokick_ip_list[i].kick)
		{
			continue;
		}

		OutputToConsole(player_ptr, "%s\n", autokick_ip_list[i].ip_address);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int AutoBanItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	int	time;
	if (!m_page_ptr->params.GetParam("time", &time)) return CLOSE_MENU;
	if (!this->params.GetParam("name", &name)) return CLOSE_MENU;

	gpCmd->NewCmd();
	gpCmd->AddParam("ma_ban");
	gpCmd->AddParam("%s", name);
	gpCmd->AddParam(time); 
	gpManiAutoKickBan->ProcessMaAutoBanName
			(
			player_ptr,
			"ma_aban_name",
			0,
			M_MENU
			);

	return RePopOption(REPOP_MENU_WAIT2);
}

bool AutoBanPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 510));
	this->SetTitle("%s", Translate(player_ptr, 511));

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue; 
		if (player.is_bot) continue;

		MenuItem *ptr = new AutoBanItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		ptr->params.AddParam("name", player.name);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int AutoKickItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *ban_type;
	if (!m_page_ptr->params.GetParam("ban_type", &ban_type)) return CLOSE_MENU;

	int user_id;
	if (!this->params.GetParam("user_id", &user_id)) return CLOSE_MENU;

	player_t player;
	player.user_id = user_id;
	if (!FindPlayerByUserID(&player)) return CLOSE_MENU;
	if (player.is_bot) return CLOSE_MENU;

	gpCmd->NewCmd();
	gpCmd->AddParam("emulate_console");
	if (strcmp("autokicksteam", ban_type) == 0)
	{
		gpCmd->AddParam("%s", player.steam_id);
		gpManiAutoKickBan->ProcessMaAutoKickSteam
				(
				player_ptr,
				"ma_akick_steam",
				0,
				M_MENU
				);
	}
	else if (strcmp("autokickip", ban_type) == 0)
	{
		gpCmd->AddParam("%s", player.ip_address);
		gpManiAutoKickBan->ProcessMaAutoKickIP
				(
				player_ptr,
				"ma_akick_ip",
				0,
				M_MENU
				);
	}
	else
	{
		gpCmd->AddParam("%s", player.name);
		gpManiAutoKickBan->ProcessMaAutoKickName
				(
				player_ptr,
				"ma_akick_name",
				0,
				M_MENU
				);
	}

	return RePopOption(REPOP_MENU_WAIT2);
}

bool AutoKickPage::PopulateMenuPage(player_t *player_ptr)
{

	char *ban_type;
	this->params.GetParam("ban_type", &ban_type);

	if (strcmp(ban_type, "autokicksteam") == 0)
	{
		this->SetEscLink("%s", Translate(player_ptr, 520));
		this->SetTitle("%s", Translate(player_ptr, 523));
	}
	else if (strcmp(ban_type, "autokickip") == 0)
	{
		this->SetEscLink("%s", Translate(player_ptr, 521));
		this->SetTitle("%s", Translate(player_ptr, 524));
	}
	else
	{
		this->SetEscLink("%s", Translate(player_ptr, 522));
		this->SetTitle("%s", Translate(player_ptr, 525));
	}

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue; 
		if (player.is_bot) continue;

		if (strcmp("autokicksteam", ban_type) == 0 ||
			strcmp("autokickip", ban_type) == 0)
		{
			if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
			{
				continue;
			}
		}

		MenuItem *ptr = new AutoKickItem();
		ptr->SetDisplayText("[%s] %i", player.name, player.user_id);
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetHiddenText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle a player changing name
//---------------------------------------------------------------------------------
void ManiAutoKickBan::ProcessChangeName(player_t *player, const char *new_name, char *old_name)
{

	char	kick_cmd[512];

	if (gpManiClient->HasAccess(player->index, IMMUNITY, IMMUNITY_TAG, false, true))
	{
		return;
	}

	for (int i = 0; i < autokick_name_list_size; i++)
	{
		if (FStrEq(new_name, autokick_name_list[i].name))
		{
			// Matched name of player
			if (autokick_name_list[i].kick)
			{
				PrintToClientConsole(player->entity, "You have been autokicked\n");
				gpManiPlayerKick->AddPlayer( player->index, 0.5f, "You were autokicked" );
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
				LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s\n", player->name, player->steam_id, kick_cmd);
				return;
			}
			else if (autokick_name_list[i].ban && !IsLAN())
			{
				// Ban by user id
				PrintToClientConsole(player->entity, "You have been auto banned\n");
				LogCommand (NULL,"Ban (Bad Name) [%s] [%s]\n", player->name, player->steam_id);
				gpManiHandleBans->AddBan ( player, player->steam_id, "MAP", autokick_name_list[i].ban_time, "Banned (Bad Name)", "Bad Name" );
				gpManiHandleBans->WriteBans();
				return;
			}
		}
	}	


	for (int i = 0; i < autokick_pname_list_size; i++)
	{
		if (NULL != Q_stristr(new_name, autokick_pname_list[i].pname))
		{
			// Matched name of player
			if (autokick_pname_list[i].kick)
			{
				PrintToClientConsole(player->entity, "You have been autokicked\n");
				gpManiPlayerKick->AddPlayer( player->index, 0.5f, "You were autokicked" );
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were auto kicked\n", player->user_id);
				LogCommand (NULL, "Kick (Bad Name) [%s] [%s] %s\n", player->name, player->steam_id, kick_cmd);
				return;
			}
			else if (autokick_pname_list[i].ban && !IsLAN())
			{
				// Ban by user id
				PrintToClientConsole(player->entity, "You have been auto banned\n");
				LogCommand (NULL,"Ban (Bad Name - partial) [%s] [%s]\n", player->name, player->steam_id);
				gpManiHandleBans->AddBan ( player, player->steam_id, "MAP", autokick_pname_list[i].ban_time, "Banned (Bad Name)", "Bad Name" );
				gpManiHandleBans->WriteBans();
				return;
			}
		}
	}	

}

//***************************************************************************************
// Autoban using ban command

SCON_COMMAND(ma_aban_name, 2187, MaAutoBanName, false);
SCON_COMMAND(ma_aban_pname, 2189, MaAutoBanPName, false);

SCON_COMMAND(ma_akick_name, 2191, MaAutoKickName, false);
SCON_COMMAND(ma_akick_pname, 2193, MaAutoKickPName, false);
SCON_COMMAND(ma_akick_steam, 2195, MaAutoKickSteam, false);
SCON_COMMAND(ma_akick_ip, 2197, MaAutoKickIP, false);


SCON_COMMAND(ma_unauto_name, 2199, MaUnAutoName, false);
SCON_COMMAND(ma_unauto_pname, 2201, MaUnAutoPName, false);
SCON_COMMAND(ma_unakick_steam, 2203, MaUnAutoSteam, false);
SCON_COMMAND(ma_unakick_ip, 2205, MaUnAutoIP, false);

SCON_COMMAND(ma_ashow_name, 2207, MaAutoShowName, false);
SCON_COMMAND(ma_ashow_pname, 2209, MaAutoShowPName, false);
SCON_COMMAND(ma_ashow_steam, 2211, MaAutoShowSteam, false);
SCON_COMMAND(ma_ashow_ip, 2213, MaAutoShowIP, false);


// Functions for autokick 
static int sort_autokick_steam ( const void *m1,  const void *m2) 
{
	struct autokick_steam_t *mi1 = (struct autokick_steam_t *) m1;
	struct autokick_steam_t *mi2 = (struct autokick_steam_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_autokick_ip ( const void *m1,  const void *m2) 
{
	struct autokick_ip_t *mi1 = (struct autokick_ip_t *) m1;
	struct autokick_ip_t *mi2 = (struct autokick_ip_t *) m2;
	return strcmp(mi1->ip_address, mi2->ip_address);
}


