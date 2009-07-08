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
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_commands.h"
#include "mani_help.h"
#include "mani_sounds.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IEngineSound *esounds; // sound
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

action_sound_t	action_sound_list[MANI_MAX_ACTION_SOUNDS]= 
				{
					{"", "joinserver", false},
					{"", "votestart", false},
					{"", "voteend", false},
					{"", "roundstart", false},
					{"", "roundend", false},
					{"", "restrictedweapon", false}
				};

static	sound_t			*sound_list = NULL;
static	int				sound_list_size = 0;
int						sounds_played[MANI_MAX_PLAYERS];
CONVAR_CALLBACK_PROTO(SoundsAutoDownload);
static void	SetupSoundAutoDownloads(void);
static void	SetupActionAutoDownloads(void);

ConVar mani_sounds_auto_download ("mani_sounds_auto_download", "0", 0, "0 = Don't auto download files to client, 1 = automatically download files to client", true, 0, true, 1, CONVAR_CALLBACK_REF(SoundsAutoDownload)); 

//---------------------------------------------------------------------------------
// Purpose: Free the sounds used
//---------------------------------------------------------------------------------
void	FreeSounds(void)
{
	FreeList((void **) &sound_list, &sound_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the sounds used
//---------------------------------------------------------------------------------
void	LoadSounds(void)
{
	FileHandle_t file_handle;
	char	sound_id[512];
	char	sound_name[512];
	char	alias_command[512];

	char	base_filename[256];

	if (!esounds) return;

	FreeList((void **) &sound_list, &sound_list_size);

	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/soundlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename, "rt", NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load soundlist.txt\n");
	}
	else
	{
//		MMsg("Sound list\n");
		while (filesystem->ReadLine (sound_id, 512, file_handle) != NULL)
		{
			if (!ParseAliasLine(sound_id, alias_command, true, false))
			{
				// String is empty after parsing
				continue;
			}

			char	exists_string[512];

			// Check file exists on server
			snprintf(exists_string, sizeof(exists_string), "./sound/%s", sound_id);
			if (!filesystem->FileExists(exists_string)) continue;

			AddToList((void **) &sound_list, sizeof(sound_t), &sound_list_size);
			Q_strcpy(sound_list[sound_list_size-1].sound_name, sound_id);
			Q_strcpy(sound_list[sound_list_size-1].alias, alias_command);
			if (esounds)
			{
				// esounds->PrecacheSound(sound_id, true);
			}

//			MMsg("Alias [%s] Sound File [%s]\n", alias_command, sound_id); 
		}

		filesystem->Close(file_handle);
	}

	SetupSoundAutoDownloads();

	// Reset list
	for (int i = 0; i < MANI_MAX_ACTION_SOUNDS; i++)
	{
		action_sound_list[i].in_use = false;
		Q_strcpy(action_sound_list[i].sound_file,"");
	}

	//Get action sound list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/actionsoundlist.txt", mani_path.GetString());
	file_handle = filesystem->Open(base_filename, "rt", NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load actionsoundlist.txt\n");
	}
	else
	{
//		MMsg("Action Sound list\n");
		while (filesystem->ReadLine (sound_id, sizeof(sound_id), file_handle) != NULL)
		{
			if (!ParseAliasLine(sound_id, sound_name, true, false))
			{
				// String is empty after parsing
				continue;
			}

			bool found_id = false;

			for (int i = 0; i < MANI_MAX_ACTION_SOUNDS; i++)
			{
				if (FStrEq(sound_name, action_sound_list[i].alias))
				{
					char	exists_string[512];

					// Check file exists on server
					snprintf(exists_string, sizeof(exists_string), "./sound/%s", sound_id);
					if (filesystem->FileExists(exists_string))
					{
						Q_strcpy(action_sound_list[i].sound_file, sound_id);
						action_sound_list[i].in_use = true;
						found_id = true;
						if (esounds)
						{
							// esounds->PrecacheSound(sound_id, true);
						}
						break;
					}
				}
			}

			if (!found_id)
			{
//				MMsg("WARNING Action Sound Name [%s] for sound file [%s] is not valid !!\n",
//								sound_name,
//								sound_id);
			}
			else
			{
//				MMsg("Loaded Action Sound Name [%s] for file [%s]\n", 
//								sound_name,
//								sound_id);
			}
		}

		filesystem->Close(file_handle);
	}

	SetupActionAutoDownloads();
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_play command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaPlaySound(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	bool unlimited_play = false;

	if (player_ptr)
	{
		// Check if player is admin

		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))
		{
			if (mani_sounds_per_round.GetInt() == 0)
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to use admin commands");
				return PLUGIN_STOP;
			}
		}
		else
		{
			// Definately admin but can they play sounds ?
			if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PLAYSOUND) || war_mode)
			{
				if (mani_sounds_per_round.GetInt() == 0)
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to play sounds");
					return PLUGIN_STOP;
				}
			}
			else
			{
				// Is admin and allowed to play to their hearts content
				unlimited_play = true;
			}
		}
	}
	else
	{
		unlimited_play = true;
	}

	if (gpCmd->Cmd_Argc() < 2) return (gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type));

	if (player_ptr && !unlimited_play)
	{
		// Check to see if player has reached their limit
		if (sounds_played[player_ptr->index - 1] < mani_sounds_per_round.GetInt())
		{
			sounds_played[player_ptr->index - 1] ++;
		}
		else
		{
			SayToPlayer(ORANGE_CHAT, player_ptr, "You can't play any more sounds this round");
			return PLUGIN_STOP;
		}
	}

	int sound_index;
	char play_sound[512];

	// See if we can find a match by index or partial match on name
	sound_index = atoi(gpCmd->Cmd_Argv(1));
	if (sound_index < 1 || sound_index > sound_list_size)
	{
		bool found_match = false;
		for (int i = 0; i < sound_list_size; i ++)
		{
			if (NULL != Q_stristr(sound_list[i].alias, gpCmd->Cmd_Argv(1)))
			{
				sound_index = i;
				found_match = true;
				break;
			}
		}

		if (!found_match)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Did not find sound requested");
			return PLUGIN_STOP;
		}
	}
	else
	{
		sound_index --;
	}

	for (int i = 1; i <= max_players; i++ )
	{
		player_t target_player;

		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (target_player.is_bot) continue;

		player_settings_t	*player_settings;
					
		player_settings = FindPlayerSettings(&target_player);
		if (!player_settings) continue;

		// This player doesn't want to hear sounds
		if (player_settings->server_sounds == 0) continue;
	  
		if (!unlimited_play && mani_sounds_filter_if_dead.GetInt() == 1)
		{
			// Is player who triggered it dead
			if (player_ptr->is_dead)
			{
				if (!target_player.is_dead)
				{
					// Don't play to alive players
					continue;
				}
			}
		}

		snprintf(play_sound, sizeof(play_sound), "play \"%s\"\n",sound_list[sound_index].sound_name);
		engine->ClientCommand(target_player.entity, play_sound);
		//UTIL_EmitSoundSingle(&target_player, sound_list[sound_index].sound_name);
	}	

	if(!unlimited_play)
	{
		// Not admin so output message
		if (mani_sounds_filter_if_dead.GetInt() == 1)
		{
			if (player_ptr->is_dead)
			{
				SayToDead(ORANGE_CHAT, "Player %s played sound %s", player_ptr->name, sound_list[sound_index].alias);
			}
			else
			{
				SayToAll(ORANGE_CHAT, false, "Player %s played sound %s", player_ptr->name, sound_list[sound_index].alias);
			}
		}
		else
		{
			SayToAll(ORANGE_CHAT, false, "Player %s played sound %s", player_ptr->name, sound_list[sound_index].alias);
		}			

		DirectLogCommand("[MANI_ADMIN_PLUGIN] Player [%s] Steam ID [%s] played sound [%s]\n", 
							player_ptr->name, 
							player_ptr->steam_id, 
							sound_list[sound_index].alias);
	}
	else
	{
		LogCommand(player_ptr, "played sound %s\n", sound_list[sound_index].alias);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_showsounds
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaShowSounds(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	OutputHelpText(ORANGE_CHAT, player_ptr, "Current Sounds in list");

	for (int i = 0; i < sound_list_size; i++)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%-3i %s", i + 1,	sound_list[i].alias);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int PlaySoundItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int index;
	if (this->params.GetParam("index", &index))
	{
		char play_sound[512];

		for (int i = 1; i <= max_players; i++ )
		{
			player_t player;

			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;

			player_settings_t	*player_settings;			
			player_settings = FindPlayerSettings(&player);
			if (!player_settings) continue;
	
			// This player doesn't want to hear sounds
			if (!player_settings->server_sounds) continue;

			snprintf(play_sound, sizeof(play_sound), "play \"%s\"\n",sound_list[index].sound_name);
			engine->ClientCommand(player.entity, play_sound);
			//UTIL_EmitSoundSingle(&player, sound_list[index].sound_name);
		}
	}

	return REPOP_MENU;
}

bool PlaySoundPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 540));
	this->SetTitle("%s", Translate(player_ptr, 541));

	// Setup sound list
	for( int i = 0; i < sound_list_size; i++ )
	{
		MenuItem *ptr = new PlaySoundItem();
		ptr->SetDisplayText("%s", sound_list[i].alias);
		ptr->params.AddParam("index", i);
		this->AddItem(ptr);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ProcessPlayActionSound( player_t *target_player, int sound_id)
{
	char	play_sound[512];
	player_settings_t	*player_settings;

	if (!action_sound_list[sound_id].in_use) return;

	if (!target_player)
	{
		for (int i = 1; i <= max_players; i++ )
		{
			player_t player;

			player.index = i;

			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;

			player_settings = FindPlayerSettings(&player);
			if (!player_settings) continue;
		
			// This player doesn't want to hear sounds
			if (!player_settings->server_sounds) continue;

			snprintf(play_sound, sizeof(play_sound), "play \"%s\"\n",action_sound_list[sound_id].sound_file);
			engine->ClientCommand(player.entity, play_sound);
			//UTIL_EmitSoundSingle(&player, action_sound_list[sound_id].sound_file);
		}
	}
	else
	{
		player_settings = FindPlayerSettings(target_player);
		if (!player_settings) return;
		
		// This player doesn't want to hear sounds
		if (!player_settings->server_sounds) return;

		snprintf(play_sound, sizeof(play_sound), "play \"%s\"\n",action_sound_list[sound_id].sound_file);
		engine->ClientCommand(target_player->entity, play_sound);
		//UTIL_EmitSoundSingle(target_player, action_sound_list[sound_id].sound_file);

	}
}
//---------------------------------------------------------------------------------
// Purpose: Setup auto res of sound files
//---------------------------------------------------------------------------------
static
void	SetupSoundAutoDownloads(void)
{

	if (mani_sounds_auto_download.GetInt() == 0) return;

	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);

	for (int i = 0; i < sound_list_size; i++)
	{
		// Set up .res downloadables
		if (pDownloadablesTable) 
		{
			char	res_string[512];
			snprintf(res_string, sizeof(res_string), "sound/%s", sound_list[i].sound_name);
#ifdef ORANGE
			pDownloadablesTable->AddString(true, res_string, sizeof(res_string));
#else
			pDownloadablesTable->AddString(res_string, sizeof(res_string));
#endif		
		} 
	}

	engine->LockNetworkStringTables(save);
}

//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the quake style sounds
//---------------------------------------------------------------------------------
static
void	SetupActionAutoDownloads(void)
{
	if (mani_sounds_auto_download.GetInt() == 0) return;

	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);
	char	res_string[512];

	for (int i = 0; i < MANI_MAX_ACTION_SOUNDS; i++)
	{
		if (action_sound_list[i].in_use)
		{
 			// Set up .res downloadables
			if (pDownloadablesTable)
			{
				snprintf(res_string, sizeof(res_string), "sound/%s", action_sound_list[i].sound_file);
#ifdef ORANGE
				pDownloadablesTable->AddString(true, res_string, sizeof(res_string));
#else
				pDownloadablesTable->AddString(res_string, sizeof(res_string));
#endif
			}
		}
	}

	engine->LockNetworkStringTables(save);
}

//---------------------------------------------------------------------------------
// Purpose: Handle change of server sound download cvar
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(SoundsAutoDownload)
{
	if (!FStrEq(pOldString, mani_sounds_auto_download.GetString()))
	{
		if (atoi(mani_sounds_auto_download.GetString()) == 1)
		{
			SetupSoundAutoDownloads();
			SetupActionAutoDownloads();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ProcessPlayMenuSound( player_t *target_player_ptr, char *sound_name)
{
	if (!esounds) return;

	// Just do extended beep
	Vector pos = target_player_ptr->entity->GetCollideable()->GetCollisionOrigin();

	// Play the sound sound
	MRecipientFilter mrf; 
	mrf.MakeReliable();
	mrf.AddPlayer(target_player_ptr->index);
	esounds->EmitSound((IRecipientFilter &)mrf, target_player_ptr->index, CHAN_AUTO, sound_name, 0.7,  0.0, 0, 100, &pos);

}
