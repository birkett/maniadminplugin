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
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_skins.h"
#include "mani_maps.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_language.h"
#include "mani_customeffects.h"
#include "mani_team.h"
#include "mani_commands.h"
#include "mani_chattrigger.h"
#include "mani_gametype.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IGameEventManager2 *gameeventmanager;

extern	bool war_mode;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;

static int sort_chat_triggers ( const void *m1,  const void *m2);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiChatTriggers::ManiChatTriggers()
{
	// Init
	chat_trigger_list_size = 0;
	chat_trigger_list = NULL;
	gpManiChatTriggers = this;
}

ManiChatTriggers::~ManiChatTriggers()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Free up any lists
//---------------------------------------------------------------------------------
void ManiChatTriggers::CleanUp(void)
{
	// Cleanup
	if (chat_trigger_list_size != 0)
	{
		free(chat_trigger_list);
		chat_trigger_list_size = 0;
		chat_trigger_list = NULL;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Server loaded plugin
//---------------------------------------------------------------------------------
void		ManiChatTriggers::Load(void)
{
	this->CleanUp();
	this->LoadData();
}

//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiChatTriggers::LevelInit(void)
{
	this->CleanUp();
	this->LoadData();
}

//---------------------------------------------------------------------------------
// Purpose: Player has said something
//---------------------------------------------------------------------------------
bool		ManiChatTriggers::PlayerSay
(
 player_t *player_ptr, 
 const char *chat_string, 
 bool teamonly,
 bool from_event
 )
{
	chat_trigger_t *chat_trigger_ptr;

	if (ProcessPluginPaused()) return true;
	if (war_mode) return true;
	if (chat_trigger_list_size == 0) return true;

	if (!this->FindString(chat_string, &chat_trigger_ptr)) return true;

	switch (chat_trigger_ptr->trigger_type)
	{
		case MANI_CT_IGNORE: return (this->ProcessIgnore(player_ptr, chat_string, teamonly, from_event)); // Np processing required, just block the chat :)
		case MANI_CT_IGNORE_X: return (this->ProcessIgnoreX(player_ptr, chat_trigger_ptr, chat_string, teamonly, from_event)); // Np processing required, just block the chat :)
		default : return true;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Player has said something
//---------------------------------------------------------------------------------
bool		ManiChatTriggers::ProcessIgnore
(
 player_t *player_ptr, 
 const	char *chat_string, 
 bool teamonly, 
 bool from_event
 )
{
	// This came from an event so just return false so it doesn't get parsed by player say event
	if (from_event) return false;

	// Just log the chat in format HLstatsX can recognise

	// Get CTeam pointer so we can use the proper team name in the log message
	// This will probably blow up on some mod when the CTeam header is changed :/
	// Yup this blows up on hl2mp so I've switched to using the gametypes.txt file
	// instead.

	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return false;

	char *team_name = gpManiGameType->GetTeamLogName(player_ptr->team);

	if ( teamonly )
	{
		UTILLogPrintf("\"%s<%i><%s><%s>\" say_team \"%s\"\n", player_ptr->name, player_ptr->user_id, player_ptr->steam_id, team_name , chat_string );
	}
	else
	{
		UTILLogPrintf( "\"%s<%i><%s><%s>\" say \"%s\"\n", player_ptr->name, player_ptr->user_id, player_ptr->steam_id, team_name, chat_string );
	}

	// Fire the event so other plugins can see it
	IGameEvent * event = gameeventmanager->CreateEvent( "player_say" );
	if ( event )	// will be null if there are no listeners!
	{
		event->SetInt("userid", player_ptr->user_id );
		event->SetString("text", chat_string );
		event->SetInt("priority", 1 );	// HLTV event priority, not transmitted
		gameeventmanager->FireEvent( event );
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Player has said something we only want to ignore it x times
//---------------------------------------------------------------------------------
bool		ManiChatTriggers::ProcessIgnoreX
(
 player_t *player_ptr, 
 chat_trigger_t *chat_trigger_ptr,
 const	char *chat_string, 
 bool teamonly, 
 bool from_event
 )
{
	// This came from an event so just return true
	if (from_event) return true;

	// Check if we should let this one through or not based on this trigger count
	if (chat_trigger_ptr->ignore_count > 0)
	{
		if (chat_trigger_ptr->current_count == chat_trigger_ptr->ignore_count)
		{
			chat_trigger_ptr->current_count = 0;
			return true;
		}
	}

	chat_trigger_ptr->current_count ++;

	// Get CTeam pointer so we can use the proper team name in the log message
	// This will probably blow up on some mod when the CTeam header is changed :/
	// Yup this blows up on hl2mp so I've switched to using the gametypes.txt file
	// instead.

	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return false;

	char *team_name = gpManiGameType->GetTeamLogName(player_ptr->team);

	// Just log the chat in format HLstatsX can recognise

	if ( teamonly )
	{
		UTILLogPrintf( "\"%s<%i><%s><%s>\" say_team \"%s\"\n", player_ptr->name, player_ptr->user_id, player_ptr->steam_id, team_name , chat_string );
	}
	else
	{
		UTILLogPrintf( "\"%s<%i><%s><%s>\" say \"%s\"\n", player_ptr->name, player_ptr->user_id, player_ptr->steam_id, team_name, chat_string );
	}

	// Fire the event so other plugins can see it
	IGameEvent * event = gameeventmanager->CreateEvent( "player_say" );
	if ( event )	// will be null if there are no listeners!
	{
		event->SetInt("userid", player_ptr->user_id );
		event->SetString("text", chat_string );
		event->SetInt("priority", 1 );	// HLTV event priority, not transmitted
		gameeventmanager->FireEvent( event );
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Do a binary search for the string
//---------------------------------------------------------------------------------
bool		ManiChatTriggers::FindString(const char *chat_string, chat_trigger_t **chat_trigger_ptr)
{
	chat_trigger_t	chat_trigger_key;

	// Do BSearch for chat string
	Q_strcpy(chat_trigger_key.say_command, chat_string);

	*chat_trigger_ptr = (chat_trigger_t *) bsearch
						(
						&chat_trigger_key, 
						chat_trigger_list, 
						chat_trigger_list_size, 
						sizeof(chat_trigger_t), 
						sort_chat_triggers
						);

	if (*chat_trigger_ptr == NULL)
	{
		return false;
	}
	
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiChatTriggers::LoadData(void)
{
	char	core_filename[256];

//	MMsg("*********** Loading chattriggers.txt ************\n");

	KeyValues *kv_ptr = new KeyValues("chattriggers.txt");

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/chattriggers.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
//		MMsg("Failed to load chattriggers.txt\n");
		kv_ptr->deleteThis();
		return;
	}

	KeyValues *base_key_ptr;

	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	// Find our map entry
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), MANI_CT_IGNORE_STRING))
		{
			// Get 'Ignore' options
			this->ProcessLoadIgnore(base_key_ptr);
		}
		else if (FStrEq(base_key_ptr->GetName(), MANI_CT_IGNORE_X_STRING))
		{
			// Get 'Ignore X' options
			this->ProcessLoadIgnoreX(base_key_ptr);
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	kv_ptr->deleteThis();

	qsort(chat_trigger_list, chat_trigger_list_size, sizeof(chat_trigger_t), sort_chat_triggers); 

//	MMsg("*********** chattriggers.txt loaded ************\n");
}

//---------------------------------------------------------------------------------
// Purpose: Read in Ignore Types
//---------------------------------------------------------------------------------
void ManiChatTriggers::ProcessLoadIgnore(KeyValues *kv_parent_ptr)
{
	KeyValues *kv_ignore_ptr;
	chat_trigger_t	chat_trigger;

	kv_ignore_ptr = kv_parent_ptr->GetFirstValue();
	if (!kv_ignore_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&chat_trigger, 0, sizeof(chat_trigger));
		chat_trigger.trigger_type = MANI_CT_IGNORE;

		Q_strcpy(chat_trigger.say_command, kv_ignore_ptr->GetString());
		if (chat_trigger.say_command && !FStrEq(chat_trigger.say_command, ""))
		{
			AddToList((void **) &(chat_trigger_list), sizeof(chat_trigger_t), &(chat_trigger_list_size));
			chat_trigger_list[chat_trigger_list_size - 1] = chat_trigger;
		}

		kv_ignore_ptr = kv_ignore_ptr->GetNextValue();
		if (!kv_ignore_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Read in Ignore X Types
//---------------------------------------------------------------------------------
void ManiChatTriggers::ProcessLoadIgnoreX(KeyValues *kv_parent_ptr)
{
	KeyValues *kv_ignore_ptr;
	chat_trigger_t	chat_trigger;

	kv_ignore_ptr = kv_parent_ptr->GetFirstValue();
	if (!kv_ignore_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&chat_trigger, 0, sizeof(chat_trigger));
		chat_trigger.trigger_type = MANI_CT_IGNORE_X;

		Q_strcpy(chat_trigger.say_command, kv_ignore_ptr->GetName());
		chat_trigger.ignore_count = atoi(kv_ignore_ptr->GetString());

		if (chat_trigger.say_command && !FStrEq(chat_trigger.say_command, ""))
		{
			AddToList((void **) &(chat_trigger_list), sizeof(chat_trigger_t), &(chat_trigger_list_size));
			chat_trigger_list[chat_trigger_list_size - 1] = chat_trigger;
		}

		kv_ignore_ptr = kv_ignore_ptr->GetNextValue();
		if (!kv_ignore_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_chattriggers command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiChatTriggers::ProcessMaChatTriggers(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}
	
	if (chat_trigger_list_size == 0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "No chat triggers installed !!");
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() == 1)
	{
		// Just list them all
		for (int i = 0; i < chat_trigger_list_size; i++)
		{
			this->DumpTriggerData(player_ptr, &(chat_trigger_list[i]));
		}
	}
	else
	{
		chat_trigger_t *chat_trigger_ptr;

		if (!this->FindString(gpCmd->Cmd_Argv(1), &chat_trigger_ptr))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", gpCmd->Cmd_Argv(1)));
			return PLUGIN_STOP;
		}

		this->DumpTriggerData(player_ptr, chat_trigger_ptr);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Output trigger data
//---------------------------------------------------------------------------------
void	ManiChatTriggers::DumpTriggerData
(
 player_t *player_ptr,  
 chat_trigger_t *chat_trigger_ptr
 )
{
	char	temp_string[256];

	switch (chat_trigger_ptr->trigger_type)
	{
		case MANI_CT_IGNORE: snprintf(temp_string, sizeof(temp_string), "%s", MANI_CT_IGNORE_STRING); break;
		case MANI_CT_IGNORE_X: snprintf(temp_string, sizeof(temp_string), "%s Limit = %i Current = %i", MANI_CT_IGNORE_X_STRING, chat_trigger_ptr->ignore_count, chat_trigger_ptr->current_count); break;
		default:snprintf(temp_string, sizeof(temp_string), "UNKNOWN"); break;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s\t%s", chat_trigger_ptr->say_command, temp_string);
}

// Command that shows the current chat triggers to the server console
SCON_COMMAND(ma_chattriggers, 2159, MaChatTriggers, false);

// QSort/BSearch function
static int sort_chat_triggers ( const void *m1,  const void *m2) 
{
	struct chat_trigger_t *mi1 = (struct chat_trigger_t *) m1;
	struct chat_trigger_t *mi2 = (struct chat_trigger_t *) m2;

	return (Q_stricmp(mi1->say_command, mi2->say_command));
}

ManiChatTriggers	g_ManiChatTriggers;
ManiChatTriggers	*gpManiChatTriggers;
