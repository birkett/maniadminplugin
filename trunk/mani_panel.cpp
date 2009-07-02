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
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "bitbuf.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_language.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_parser.h"
#include "mani_output.h"
#include "mani_client.h"
#include "mani_menu.h"
#include "mani_commands.h"
#include "mani_gametype.h"
#include "mani_panel.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	INetworkStringTableContainer *networkstringtable;
extern	INetworkStringTable *g_pStringTableManiScreen;
extern	IFileSystem	*filesystem;

extern	int	max_players;
extern	bf_write *msg_buffer;
extern	int	vgui_message_index;
extern	char *mani_version;
extern	bool war_mode;

static	web_shorcut_t	*web_shortcut_list = NULL;
static	int				 web_shortcut_list_size = 0;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

//---------------------------------------------------------------------------------
// Purpose: Setup panels
//---------------------------------------------------------------------------------
void	InitPanels(void)
{

	return;
//	MMsg("Number of Tables [%i]\n", networkstringtable->GetNumTables());

	g_pStringTableManiScreen = networkstringtable->FindTable("InfoPanel");
	if (!g_pStringTableManiScreen)
	{
//		MMsg("Did not find InfoPanel\n");
	}
	else
	{
		g_pStringTableManiScreen->AddString( MANI_WEB_STATS_PANEL, 5, "INIT");
//		MMsg("Added 1 new network string\n");
	}
}

//---------------------------------------------------------------------------------
// Purpose: Load the cron tabs into memory
//---------------------------------------------------------------------------------
void	LoadWebShortcuts(void)
{
	FileHandle_t file_handle;
	char	web_shortcut_string[512];
	char	url_string[512];
	char	base_filename[256];

	web_shorcut_t	web_shortcut;

	FreeWebShortcuts();

//	MMsg("********** LOADING WEB SHORTCUTS ***********\n");

	//Get rcon list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/webshortcutlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load webshortcutlist.txt\n");
	}
	else
	{
		while (filesystem->ReadLine (url_string, sizeof(url_string), file_handle) != NULL)
		{
			if (!ParseAliasLine(url_string, web_shortcut_string, false, true))
			{
				// String is empty after parsing
				continue;
			}

			Q_strcpy(web_shortcut.shortcut, web_shortcut_string);
			Q_strcpy(web_shortcut.url_string, url_string);
			AddToList((void **) &web_shortcut_list, sizeof(web_shorcut_t), &web_shortcut_list_size);
			web_shortcut_list[web_shortcut_list_size - 1] = web_shortcut;

//			MMsg("command [%s] url [%s]\n", web_shortcut_string, url_string);
		}

		filesystem->Close(file_handle);
	}

//	MMsg("********** WEB SHORTCUTS LOADED ***********\n");

}

//---------------------------------------------------------------------------------
// Purpose: Free web shortcuts from memory
//---------------------------------------------------------------------------------
void FreeWebShortcuts(void)
{
	FreeList((void **) &web_shortcut_list, &web_shortcut_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Process short cuts entered via player say
//---------------------------------------------------------------------------------
bool ProcessWebShortcuts(edict_t *pEntity, const char *say_string)
{
	for (int i = 0; i < web_shortcut_list_size; i++)
	{
		if (FStrEq(say_string, web_shortcut_list[i].shortcut))
		{
			char	substitute_url[2048];

			player_t player;

			player.entity = pEntity;
			if (!FindPlayerByEntity(&player)) return false;

			ParseSubstituteStrings(&player, web_shortcut_list[i].url_string, substitute_url);

			MRecipientFilter mrf;
			mrf.AddPlayer(player.index);
			DrawURL(&mrf, mani_version, substitute_url);
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_favourites command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaFavourites(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsBrowseAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr && player_ptr->is_bot) return PLUGIN_STOP;

	if (web_shortcut_list_size != 0)
	{
		if (command_type == M_SAY || command_type == M_TSAY)
		{
			SayToPlayer(ORANGE_CHAT, player_ptr, "Web Keywords");
		}
		else
		{
			OutputToConsole(player_ptr, "Web Keywords\n");
			OutputToConsole(player_ptr, "------------\n");
		}
	}

	for (int i = 0; i < web_shortcut_list_size; i++)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", web_shortcut_list[i].shortcut); 
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
int FavouritesItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int	index;
	if (this->params.GetParam("index", &index))
	{
		ProcessWebShortcuts (player_ptr->entity, web_shortcut_list[index].shortcut);
	}

	return CLOSE_MENU;
}

bool FavouritesPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 770));
	this->SetTitle("%s", Translate(player_ptr, 771));
	
	for (int i = 0; i < web_shortcut_list_size; i++)
	{
		MenuItem *ptr = new FavouritesItem;
		ptr->params.AddParam("index", i);
		ptr->SetDisplayText("%s", web_shortcut_list[i].shortcut);
		this->AddItem(ptr);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Show Panel to user
//---------------------------------------------------------------------------------
void	DrawPanel(MRecipientFilter *mrf, char *panel_title, char *network_string, char *message, int message_length)
{
	// Update network string first
	if (!g_pStringTableManiScreen) return;

	int index = g_pStringTableManiScreen->FindStringIndex(network_string);

	if (index == -1) return;
	
	g_pStringTableManiScreen->SetStringUserData(index, message_length + 1, message);

	msg_buffer = engine->UserMessageBegin(mrf, vgui_message_index);
   
	msg_buffer->WriteString("info"); // menu name
	msg_buffer->WriteByte(1);
	msg_buffer->WriteByte(3);

	msg_buffer->WriteString("title");
	msg_buffer->WriteString(panel_title);

	msg_buffer->WriteString("type");
	msg_buffer->WriteString("1");

	msg_buffer->WriteString("Msg");
	msg_buffer->WriteString(network_string);

	engine->MessageEnd();

}

//---------------------------------------------------------------------------------
// Purpose: Show Web Stats to user
//---------------------------------------------------------------------------------
void	DrawMOTD(MRecipientFilter *mrf)
{
	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	msg_buffer = engine->UserMessageBegin(mrf, vgui_message_index);
   
	msg_buffer->WriteString("info"); // menu name
	msg_buffer->WriteByte(1);
	msg_buffer->WriteByte(3);

	msg_buffer->WriteString("title");
	msg_buffer->WriteString(title);

	msg_buffer->WriteString("type");
	msg_buffer->WriteString("1");

	msg_buffer->WriteString("Msg");
	msg_buffer->WriteString("motd");

	engine->MessageEnd();

}

//---------------------------------------------------------------------------------
// Purpose: Show URL Panel to user
//---------------------------------------------------------------------------------
void	DrawURL(MRecipientFilter *mrf, char *title, const char *url)
{
	msg_buffer = engine->UserMessageBegin(mrf, vgui_message_index);
   
	msg_buffer->WriteString("info"); // menu name
	msg_buffer->WriteByte(1);
	msg_buffer->WriteByte(3);

	msg_buffer->WriteString("title");
	msg_buffer->WriteString(title);

	msg_buffer->WriteString("type");
	msg_buffer->WriteString("2");  // URL

	msg_buffer->WriteString("Msg");
	msg_buffer->WriteString(url);

	engine->MessageEnd();

}

SCON_COMMAND(ma_favourites, 2175, MaFavourites, false);
