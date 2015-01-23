//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
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
#include "Color.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#if defined ( GAME_CSGO )
#include <cstrike15_usermessage_helpers.h>
#else
#include "bitbuf.h"
#endif
#include "engine/IEngineSound.h"
#include "mani_main.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_commands.h"
#include "mani_output.h"
#include "mani_sounds.h"
#include "mani_menu.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	IEngineSound *esounds; // sound
#if !defined ( GAME_CSGO )
extern	bf_write *msg_buffer;
#endif
extern	int	menu_message_index;
extern	CGlobalVars *gpGlobals;
extern	int	max_players;

extern char *menu_select_exit_sound;
extern char *menu_select_sound;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ConVar mani_old_style_menu_behaviour ("mani_old_style_menu_behaviour", "0", 0, "0 = New Style menu behaviour where menu options remain open, 1 = Menus close after selecting an option", true, 0, true, 1);
ConVar mani_menu_force_text_input_via_esc ("mani_menu_force_text_input_via_esc", "0", 0, "0 = Use the chat line for text input, 1 = Use the Escape dialog box for text input", true, 0, true, 1);

//---------------------------------------------------------------------------------
// Purpose: Draw raw menu via user message
//---------------------------------------------------------------------------------
void DrawMenu
(
int	player_index,
int	time,
int range,
bool back,
bool more,
bool cancel,
char *menu_string,
bool final
)
{
// With the protobuf update of csgo, the ShowMenu message doesn't support partial sending of menu info.
// We need to send the menu all at once.
#if defined ( GAME_CSGO )
	static char player_menus[64][512] = {};

	int buffer_len = 0;
	buffer_len = strlen(player_menus[player_index]);

	snprintf(player_menus[player_index]+buffer_len, 512-buffer_len, "%s", menu_string);

	// If this menu isn't finished yet, don't send anything to the client.
	if(!final)
		return;
#endif

	unsigned int		keys=0;
	char				menu_string_clamped[512];

	for (int i = 1; i <= range; i++)
	{
		keys |= (1 << (i-1));
	}

#if defined ( GAME_CSGO )
	if (back) keys |= (1<<6); // Key 7
	if (more) keys |= (1<<7); // Key 8
	if (cancel)	keys |= (1<<8); // Key 9
#else
	if (back) keys |= (1<<7); // Key 8
	if (more) keys |= (1<<8); // Key 9
	if (cancel)	keys |= (1<<9); // Key 0
#endif
	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	mrf.AddPlayer(player_index);

#if defined ( GAME_CSGO )
	CCSUsrMsg_ShowMenu *msg = (CCSUsrMsg_ShowMenu *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_ShowMenu)->New(); // Show Menu message
#else
	msg_buffer = engine->UserMessageBegin( &mrf, menu_message_index ); // Show Menu message
#endif
	if (final)
	{
#if defined ( GAME_CSGO )
		msg->set_bits_valid_slots( keys ); // Key bits
#else
 		msg_buffer->WriteShort( keys ); // Key bits
#endif
#if defined ( GAME_ORANGE )
		if ( !g_menu_mgr.GetMenuShowing ( player_index - 1) ) {
			g_menu_mgr.ResetMenuShowing ( player_index - 1 , false );
		}
#endif
	}
	else
	{
#if defined ( GAME_CSGO )
		msg->set_bits_valid_slots( (1<<9) ); // Key bits
#else
		msg_buffer->WriteShort( (1<<9) ); // Key bits
#endif
		
	}

#if defined ( GAME_CSGO )
		msg->set_display_time( (time == -1)? 0 : time ); // // Time for menu 0 = permanent
#else
		msg_buffer->WriteChar( time ); // Time for menu -1 = permanent
#endif
	
#if !defined ( GAME_CSGO )
	if (final)
	{
		msg_buffer->WriteByte( 0 ); // Draw immediately
	}
	else
	{
		msg_buffer->WriteByte( 1 ); // Draw later more info required
	}
#endif	


	// Ensure string is not more than 512 bytes
#if defined ( GAME_CSGO )
	msg->set_menu_string(player_menus[player_index]);
	engine->SendUserMessage(mrf, CS_UM_ShowMenu, *msg);
	delete msg;
	player_menus[player_index][0] = 0;
	buffer_len = 0;
#else	
	snprintf(menu_string_clamped, sizeof(menu_string_clamped), "%s", menu_string);
	msg_buffer->WriteString (menu_string_clamped);	
	engine->MessageEnd();

#endif	
}

//---------------------------------------------------------------------------------
// Purpose: Menu Item methods 
//---------------------------------------------------------------------------------
MenuItem::MenuItem ()
{
}

MenuItem::~MenuItem()
{
}

void MenuItem::SetDisplayText(const char *text, ...)
{
	char temp_str[256];
	va_list		argptr;

	va_start ( argptr, text );
	vsnprintf( temp_str, sizeof(temp_str), text, argptr );
	va_end   ( argptr );
	display_text.Set(temp_str);
}

void MenuItem::SetHiddenText(const char *text, ...)
{
	char temp_str[256];
	va_list		argptr;

	va_start ( argptr, text );
	vsnprintf( temp_str, sizeof(temp_str), text, argptr );
	va_end   ( argptr );
	hidden_text.Set(temp_str);
}

void MenuItem::AddPreText(const char *text, ...)
{
	char temp_str[256];
	va_list		argptr;

	va_start ( argptr, text );
	vsnprintf( temp_str, sizeof(temp_str), text, argptr );
	va_end   ( argptr );
	this->pre_text.push_back(BasicStr(temp_str));
}

void MenuItem::AddPostText(const char *text, ...)
{
	char temp_str[256];
	va_list		argptr;

	va_start ( argptr, text );
	vsnprintf( temp_str, sizeof(temp_str), text, argptr );
	va_end   ( argptr );
	this->post_text.push_back(BasicStr(temp_str));
}

bool MenuItem::operator<(const MenuItem &right) const 
{
	return (this->display_text < right.display_text);
}

//---------------------------------------------------------------------------------
// Purpose: Renders a built up menu 
//---------------------------------------------------------------------------------
MenuPage::MenuPage()
{
	current_index = 0;
	input_object = false;
	hook_chat = false;
	timeout = -1;
}

MenuPage::~MenuPage() 
{
	current_index = 0;
	for (int i = 0; i != (int) menu_items.size(); i++) 
		delete menu_items[i];
}

void MenuPage::SetInputObject(bool enabled) 
{
	input_object = enabled;
}

bool MenuPage::IsChatHooked(void)
{
	return hook_chat;
}

void MenuPage::SetTitle(const char *menu_title, ...)
{
	char temp_str[512];
	va_list		argptr;

	va_start ( argptr, menu_title );
	vsnprintf( temp_str, sizeof(temp_str), menu_title, argptr );
	va_end   ( argptr );
	title.Set(temp_str);
}

void MenuPage::SetEscLink(const char *esc_link_text, ...)
{
	char temp_str[512];
	va_list		argptr;

	va_start ( argptr, esc_link_text );
	vsnprintf( temp_str, sizeof(temp_str), esc_link_text, argptr );
	va_end   ( argptr );
	esc_link.Set(temp_str);
}

static bool DisplaySort(MenuItem* start, MenuItem* end)
{
	return (strcmp(start->display_text.str, end->display_text.str) < 0);
}

static bool HiddenSort(MenuItem* start, MenuItem* end)
{
	return (strcmp(start->hidden_text.str, end->hidden_text.str) < 0);
}

void MenuPage::Kill() 
{
	for (int i = 0; i != (int) menu_items.size(); i++) 
		delete menu_items[i];
}

int	 MenuPage::Size() 
{
	return (int) menu_items.size();
}

void MenuPage::SetTimeout(int time) 
{
	timeout = time;
}

void MenuPage::AddItem(MenuItem *ptr)
{
	menu_items.push_back(ptr);
}

void MenuPage::SortDisplay() 
{
	if (mani_sort_menus.GetInt() == 0) return;
	std::sort(menu_items.begin(), menu_items.end(), DisplaySort);
}

void MenuPage::SortHidden() 
{
	if (mani_sort_menus.GetInt() == 0) return;
	std::sort(menu_items.begin(), menu_items.end(), HiddenSort);
}

void	MenuPage::RenderPage(player_t *player_ptr, const int history_level)
{
	int current_page = 0;
	int number_of_pages = 0;
	bool need_page_count = false;
	char menu_string[256];

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		need_more = false;
		need_back = false;

		// Handle number of menu items being less than when
		// we last saw them (For instance, slaying a player
		// will reduce the number of entries on screen
		if (current_index >= (int) menu_items.size())
		{
			current_index = (int) menu_items.size() - 1;
			if (current_index < 0)
			{
				current_index = 0;
			}
		}

		// ShowMenu() style version using menu.cpp in client code
		int	range = 0;
		if ((this->menu_items.size() - current_index) > MAX_AMX_MENU - 1)
		{
			need_more = true;
		}

		if (this->menu_items.size() > (MAX_AMX_MENU - 1))
		{
			need_page_count = true;
			number_of_pages = ((this->menu_items.size() - 1) / (MAX_AMX_MENU - 1)) + 1;

			if (current_index < (MAX_AMX_MENU - 1))
			{
				current_page = 1;
			}
			else
			{
				current_page = (current_index / (MAX_AMX_MENU - 1)) + 1;
			}
		}
		// First do the title
		if (this->title.str)
		{
			if (need_page_count)
			{
				snprintf(menu_string, sizeof(menu_string), "%s\n%s\n", 
					this->title.str,
					Translate(player_ptr, 2942, "%i%i", current_page, number_of_pages));
			}
			else
			{
				snprintf(menu_string, sizeof(menu_string), "%s\n", this->title.str);
			}

			DrawMenu(player_ptr->index, this->timeout, range, false, false, true, menu_string, false);
		}

		int	 menu_option = 1;

		for (int i = current_index; i != (int) menu_items.size(); i++)
		{
			if (i - current_index >= MAX_AMX_MENU - 1)
			{
				break;
			}

			for (int j = 0; j != (int) menu_items[i]->pre_text.size(); j++)
			{
				snprintf(menu_string, sizeof(menu_string), "%s", menu_items[i]->pre_text[j].str);
				DrawMenu(player_ptr->index, this->timeout, range, false, false, true, menu_string, false);
			}

			snprintf(menu_string, sizeof(menu_string), "->%i %s\n", menu_option++, menu_items[i]->display_text.str);
			DrawMenu(player_ptr->index, this->timeout, range++, false, false, true, menu_string, false);

			for (int j = 0; j != (int) menu_items[i]->post_text.size(); j++)
			{
				snprintf(menu_string, sizeof(menu_string), "%s", menu_items[i]->post_text[j].str);
				DrawMenu(player_ptr->index, this->timeout, range - 1, false, false, true, menu_string, false);
			}		
		}

		if (current_index > 0 || history_level > 1)
		{
			need_back = true;
		}

		if (need_back)
		{
			snprintf( menu_string, sizeof(menu_string), Translate(player_ptr, M_MENU_BACK_AMX));
			DrawMenu(player_ptr->index, this->timeout, range, need_back, need_more, true, menu_string, false);
		}

		if (need_more)
		{
			snprintf( menu_string, sizeof(menu_string), Translate(player_ptr, M_MENU_MORE_AMX));
			DrawMenu(player_ptr->index, this->timeout, range, need_back, need_more, true, menu_string, false);
		}

		snprintf( menu_string, sizeof(menu_string), Translate(player_ptr, M_MENU_EXIT_AMX));
		DrawMenu(player_ptr->index, this->timeout, range, need_back, need_more, true, menu_string, true);
	}
	else
	{
		// VSP Escape style menus
		char num[10], msg[128], cmd[128];

		need_more = false;
		need_back = false;

		// Handle number of menu items being less than when
		// we last saw them (For instance, slaying a player
		// will reduce the number of entries on screen
		if (current_index >= (int) menu_items.size())
		{
			current_index = (int) menu_items.size() - 1;
			if (current_index < 0)
			{
				current_index = 0;
			}
		}

		if ((this->menu_items.size() - current_index) > MAX_ESCAPE_MENU - 1)
		{
			need_more = true;
		}

		if (this->menu_items.size() > (MAX_ESCAPE_MENU - 1))
		{
			need_page_count = true;
			number_of_pages = ((this->menu_items.size() - 1) / (MAX_ESCAPE_MENU - 1)) + 1;

			if (current_index < (MAX_ESCAPE_MENU - 1))
			{
				current_page = 1;
			}
			else
			{
				current_page = (current_index / (MAX_ESCAPE_MENU - 1)) + 1;
			}
		}

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", this->esc_link.str);
		kv->SetInt( "level", 0 );
		kv->SetColor( "color", Color( 255, 0, 0, 255 ));
		kv->SetInt( "time", 20 );
		if (need_page_count)
		{
			snprintf(menu_string, sizeof(menu_string), "%s : %s", 
				this->title.str,
				Translate(player_ptr, 2942, "%i%i", current_page, number_of_pages));
			kv->SetString( "msg", menu_string);
		}
		else
		{
			kv->SetString( "msg", this->title.str);
		}
		kv->SetString( "cmd", "ma_escselect 10");

		int	 menu_option = 1;

		for (int i = current_index; i != (int) menu_items.size(); i++)
		{
			if (i - current_index >= MAX_ESCAPE_MENU - 1)
			{
				break;
			}

			snprintf( num, sizeof(num), "%i", menu_option);
			snprintf( msg, sizeof(msg), "%s", menu_items[i]->display_text.str);
			snprintf( cmd, sizeof(cmd), "ma_escselect %i", menu_option++);
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		if (current_index > 0 || history_level > 1)
		{
			need_back = true;
		}

		if (need_back)
		{
			snprintf( num, sizeof(num), "8");
			snprintf( msg, sizeof(msg), "%s", Translate(player_ptr, M_MENU_BACK));
			snprintf( cmd, sizeof(cmd), "ma_escselect 8");
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		if (need_more)
		{
			snprintf( num, sizeof(num), "9");
			snprintf( msg, sizeof(msg), "%s", Translate(player_ptr, M_MENU_MORE));
			snprintf( cmd, sizeof(cmd), "ma_escselect 9");
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		helpers->CreateMessage( player_ptr->entity, DIALOG_MENU, kv, gpManiISPCCallback );
		kv->deleteThis();
	}
}

void	MenuPage::RenderInputObject(player_t *player_ptr)
{
	if (mani_use_amx_style_menu.GetInt() == 1 && 
		gpManiGameType->IsAMXMenuAllowed() &&
		mani_menu_force_text_input_via_esc.GetInt() == 0)
	{
		// Use chat line to get input
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 2682));
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", this->title.str);
		this->hook_chat = true;
	}
	else
	{
		// Use dialog box to get input
		KeyValues *kv = new KeyValues( "entry" );
		kv->SetString( "title", this->esc_link.str );
		kv->SetString( "msg", this->title.str );
		kv->SetString( "command", "ma_escinput" ); // anything they enter into the dialog turns into a say command
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		
		helpers->CreateMessage( player_ptr->entity, DIALOG_ENTRY, kv, gpManiISPCCallback );
		kv->deleteThis();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handles an option being selected
//---------------------------------------------------------------------------------
MenuTemporal::MenuTemporal()
{
	free_page = NULL;
}
MenuTemporal::~MenuTemporal() 
{
	this->Kill();
}

void MenuTemporal::Kill() 
{
	for(int i = 0; i != (int) menu_pages.size(); i++) 
		delete menu_pages[i]; 
	menu_pages.clear();
	if (free_page) 
	{
		delete free_page;
		free_page = NULL;
	}

	allow_repop = false;
}

void MenuTemporal::AddMenu(MenuPage *ptr) 
{
	menu_pages.push_back(ptr);
}

void MenuTemporal::OptionSelected(player_t *player_ptr, const int option)
{
	if (menu_pages.empty()) 
	{
		return;
	}
	MenuPage *menu_page_ptr = menu_pages[menu_pages.size() - 1];

	if (menu_page_ptr->input_object && option == 1337)
	{
		// Handle input object
		ProcessPlayMenuSound(player_ptr, menu_select_sound);
		switch (menu_page_ptr->menu_items[0]->MenuItemFired(player_ptr, menu_page_ptr))
		{
		case CLOSE_MENU:
			this->Kill();
#if defined ( GAME_ORANGE )
			g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case NEW_MENU:
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case PREVIOUS_MENU:
			// Check history level
			if (menu_pages.size() == 1)
			{
				// Get out of menu as it has closed
				this->Kill();
#if defined ( GAME_ORANGE )
				g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
				ProcessPlayMenuSound(player_ptr, menu_select_exit_sound);
			}
			else
			{
				delete menu_page_ptr;
				menu_pages.pop_back();
				menu_pages[menu_pages.size() - 1]->menu_items.clear();
				menu_pages[menu_pages.size() - 1]->PopulateMenuPage(player_ptr);
				menu_pages[menu_pages.size() - 1]->RenderPage(player_ptr, menu_pages.size());
				ProcessPlayMenuSound(player_ptr, menu_select_sound);
			}
			break;

		default:
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		}	

		return;
	}

	int item_delta = (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed()) ? MAX_AMX_MENU - 1:MAX_ESCAPE_MENU - 1;

	// Option has been selected
	if (option == OPTION_NEED_MORE && menu_page_ptr->need_more)
	{
		// Repopulate the menu items
		menu_page_ptr->menu_items.clear();
		menu_page_ptr->PopulateMenuPage(player_ptr);
		menu_page_ptr->current_index += item_delta;
		menu_page_ptr->RenderPage(player_ptr, menu_pages.size());
		ProcessPlayMenuSound(player_ptr, menu_select_sound);
		return;
	}

	if (option == OPTION_NEED_BACK && menu_page_ptr->need_back)
	{
		if (menu_page_ptr->current_index == 0)
		{
			// Check history level
			if (menu_pages.size() == 1)
			{
				// Get out of menu as it has closed
				this->Kill();
#if defined ( GAME_ORANGE )
				g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
				ProcessPlayMenuSound(player_ptr, menu_select_exit_sound);
				return;
			}
			else
			{
				delete menu_page_ptr;
				menu_pages.pop_back();

				// Skip input objects as they may well get us into
				// an infinite loop
				while (menu_pages[menu_pages.size() - 1]->input_object)
				{
					delete menu_pages[menu_pages.size() - 1];
					menu_pages.pop_back();
					if (menu_pages.empty())
					{
						this->Kill();
#if defined ( GAME_ORANGE )
						g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
						ProcessPlayMenuSound(player_ptr, menu_select_exit_sound);
						return;
					}
				}

				menu_pages[menu_pages.size() - 1]->menu_items.clear();
				menu_pages[menu_pages.size() - 1]->PopulateMenuPage(player_ptr);
				menu_pages[menu_pages.size() - 1]->RenderPage(player_ptr, menu_pages.size());
				ProcessPlayMenuSound(player_ptr, menu_select_sound);
				return;
			}
		}

		menu_page_ptr->menu_items.clear();
		menu_page_ptr->PopulateMenuPage(player_ptr);
		menu_page_ptr->current_index -= item_delta;
		if (menu_page_ptr->current_index < 0)
		{
			menu_page_ptr->current_index = 0;
		}

		menu_page_ptr->RenderPage(player_ptr, menu_pages.size());
		ProcessPlayMenuSound(player_ptr, menu_select_sound);
		return;
	}

	if (option == OPTION_EXIT)
	{
		// Exit pressed
		this->Kill();
#if defined ( GAME_ORANGE )
		g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
		ProcessPlayMenuSound(player_ptr, menu_select_exit_sound);
		return;
	}

	int menu_item_index = menu_page_ptr->current_index + (option - 1);
	if (menu_item_index >= (int) menu_page_ptr->menu_items.size() || menu_item_index < 0)
	{
		this->Kill();
#if defined ( GAME_ORANGE )
		g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
		return;
	}

	if (menu_page_ptr->menu_items[menu_item_index])
	{
		switch (menu_page_ptr->menu_items[menu_page_ptr->current_index + (option - 1)]->MenuItemFired(player_ptr, menu_page_ptr))
		{
		case REDRAW_MENU:
			menu_page_ptr->RenderPage(player_ptr, menu_pages.size());
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case REPOP_MENU:
			menu_page_ptr->menu_items.clear();
			menu_page_ptr->PopulateMenuPage(player_ptr);
			menu_page_ptr->RenderPage(player_ptr, menu_pages.size());
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case REPOP_MENU_WAIT1:
			g_menu_mgr.game_frame_repop[player_ptr->index - 1] = 1;
			//helpers->ClientCommand(player_ptr->entity, "ma_menurepop\n");
			allow_repop = true;
			break;
		case REPOP_MENU_WAIT2:
			g_menu_mgr.game_frame_repop[player_ptr->index - 1] = 2;
			//helpers->ClientCommand(player_ptr->entity, "wait;ma_menurepop\n");
			allow_repop = true;
			break;
		case REPOP_MENU_WAIT3:
			g_menu_mgr.game_frame_repop[player_ptr->index - 1] = 3;
			//helpers->ClientCommand(player_ptr->entity, "wait;wait;ma_menurepop\n");
			allow_repop = true;
			break;

		case CLOSE_MENU:
			this->Kill();
#if defined ( GAME_ORANGE )
			g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case NEW_MENU:
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		case PREVIOUS_MENU:
			if (menu_page_ptr->current_index == 0)
			{
				// Check history level
				if (menu_pages.size() == 1)
				{
					// Get out of menu as it has closed
					this->Kill();
#if defined ( GAME_ORANGE )
				g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
					ProcessPlayMenuSound(player_ptr, menu_select_exit_sound);
					return;
				}
				else
				{
					delete menu_page_ptr;
					menu_pages.pop_back();
					menu_pages[menu_pages.size() - 1]->menu_items.clear();
					menu_pages[menu_pages.size() - 1]->PopulateMenuPage(player_ptr);
					menu_pages[menu_pages.size() - 1]->RenderPage(player_ptr, menu_pages.size());
					ProcessPlayMenuSound(player_ptr, menu_select_sound);
					return;
				}
			}

			menu_page_ptr->menu_items.clear();
			menu_page_ptr->PopulateMenuPage(player_ptr);
			menu_page_ptr->current_index -= item_delta;
			if (menu_page_ptr->current_index < 0)
			{
				menu_page_ptr->current_index = 0;
			}

			menu_page_ptr->RenderPage(player_ptr, menu_pages.size());
			ProcessPlayMenuSound(player_ptr, menu_select_sound);			
			break;

		default:
			ProcessPlayMenuSound(player_ptr, menu_select_sound);
			break;
		}
	}
	else
	{
#if defined ( GAME_ORANGE )
		g_menu_mgr.ResetMenuShowing ( player_ptr->index - 1 );
#endif
		this->Kill();
	}

	return;
}

void MenuManager::Kill() 
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++) 
	{
		player_list[i].Kill();
		game_frame_repop[i] = 0;
	}
}

void MenuManager::Kill(player_t *player_ptr) 
{
	player_list[player_ptr->index - 1].Kill();
	game_frame_repop[player_ptr->index - 1] = 0;

#if defined ( GAME_ORANGE )
	ResetMenuShowing ( player_ptr->index - 1 );
#endif
}

void MenuManager::OptionSelected(player_t *player_ptr, const int option) 
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	if (mt_ptr->free_page)
	{
		if (!mt_ptr->free_page->OptionSelected(player_ptr, option))
		{
			delete mt_ptr->free_page;
			mt_ptr->free_page = NULL;
		}
		else
		{
			if (mt_ptr->free_page->timeout > 0)
			{
				time_t current_time;
				time(&current_time);
				mt_ptr->timeout_timestamp = current_time + mt_ptr->free_page->timeout;
			}
			else
			{
				mt_ptr->timeout_timestamp = 0;
			}
		}
	}
	else if (!mt_ptr->menu_pages.empty()) 
	{
		mt_ptr->OptionSelected(player_ptr, option);
	}
}

void MenuManager::RepopulatePage(player_t *player_ptr) 
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	if (mt_ptr->free_page != NULL) 
	{
		delete mt_ptr->free_page;
		mt_ptr->free_page = NULL;
		return;
	}

	if (mt_ptr->allow_repop && !mt_ptr->menu_pages.empty())
	{
		mt_ptr->allow_repop = false;
		MenuPage *menu_page_ptr = mt_ptr->menu_pages[mt_ptr->menu_pages.size() - 1];
		menu_page_ptr->menu_items.clear();
		menu_page_ptr->PopulateMenuPage(player_ptr);
		menu_page_ptr->RenderPage(player_ptr, mt_ptr->menu_pages.size());
		ProcessPlayMenuSound(player_ptr, menu_select_sound);
	}
}

void MenuManager::AddMenu(player_t *player_ptr, MenuPage *ptr, int priority, int timeout) 
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	if (mt_ptr->free_page != NULL) 
	{
		delete mt_ptr->free_page;
		mt_ptr->free_page = NULL;
	}

	mt_ptr->menu_pages.push_back(ptr);

#if defined ( GAME_ORANGE )
	if ( timeout > 0 && timeout < 6 )
		timeout = 1;
	else 
		timeout -= 5;
#endif

	ptr->SetTimeout(timeout);
	if (timeout <= 0)
	{
		mt_ptr->timeout_timestamp = 0;
	}
	else
	{
		time_t current_time;
		time(&current_time);
		
#if defined ( GAME_ORANGE )
		int adjusted_time = ((int)(timeout/5))*5; // lock in the menu to 5 second intervals.
		mt_ptr->timeout_timestamp = current_time + adjusted_time;
#else
		mt_ptr->timeout_timestamp = current_time + timeout;
#endif
	}
}

void MenuManager::KillLast(player_t *player_ptr) 
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	if (mt_ptr->free_page != NULL) 
	{
		delete mt_ptr->free_page;
		mt_ptr->free_page = NULL;
	}

	if (!mt_ptr->menu_pages.empty())
	{
		std::vector<MenuPage *>::reverse_iterator i = mt_ptr->menu_pages.rbegin();
		delete *i;
		mt_ptr->menu_pages.pop_back();
	}
}


void MenuManager::AddFreePage(player_t *player_ptr, FreePage *ptr, int priority, int timeout) 
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	mt_ptr->Kill();
	mt_ptr->free_page = ptr;

	ptr->SetTimeout(timeout);

#if defined ( GAME_ORANGE )
	if ( timeout > 0 && timeout < 6 )
		timeout = 1;
	else 
		timeout -= 5;
#endif

	if (timeout <= 0)
	{
		mt_ptr->timeout_timestamp = 0;
	}
	else
	{
		time_t current_time;
		time(&current_time);

#if defined ( GAME_ORANGE )
		int adjusted_time = ((int)(timeout/5))*5; // lock in the menu to 5 second intervals.
		mt_ptr->timeout_timestamp = current_time + adjusted_time;
#else
		mt_ptr->timeout_timestamp = current_time + timeout;
#endif
	}
}

bool MenuManager::CanAddMenu(player_t *player_ptr, int priority)
{
	// No way to handle close button on Escape style menus :(
	if (mani_use_amx_style_menu.GetInt() == 0 ||
		!gpManiGameType->IsAMXMenuAllowed())
	{
		return true;
	}

	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);

	if (mt_ptr->free_page == NULL && mt_ptr->menu_pages.empty()) return true;
	if (priority <= mt_ptr->priority) return true;

	// Priority greater than existing, so check timeout
	if (mt_ptr->timeout_timestamp == 0) return false;

	time_t current_time;
	time(&current_time);
	if (mt_ptr->timeout_timestamp < current_time)
	{
		// timeout expire so good to go
		return true;
	}

	// Forget it, calling function is not getting control
	return false;
}

const int MenuManager::GetHistorySize(player_t *player_ptr) 
{
	return (const int) player_list[player_ptr->index - 1].menu_pages.size();
}

void MenuManager::ClientActive(player_t *player_ptr) 
{
	this->Kill(player_ptr);
}

void MenuManager::ClientDisconnect(player_t *player_ptr) 
{
	this->Kill(player_ptr);
}
void MenuManager::Load() 
{
	this->Kill();
}
void MenuManager::Unload() 
{
	this->Kill();
}
void MenuManager::LevelInit() 
{
	this->Kill();
#if defined ( GAME_ORANGE )
	next_time_check = 0.0f;
#endif
}

// Check if chat is hooked
bool MenuManager::ChatHooked(player_t *player_ptr)
{
	MenuTemporal *mt_ptr = &(player_list[player_ptr->index - 1]);
	if (!mt_ptr->menu_pages.empty())
	{
		MenuPage *m_ptr = mt_ptr->menu_pages[mt_ptr->menu_pages.size() - 1];
		if (m_ptr->hook_chat)
		{
			char temp_string[2048];

			snprintf(temp_string, sizeof(temp_string), "ma_escinput %s", gpCmd->Cmd_Args(0));
			helpers->ClientCommand(player_ptr->entity, temp_string);
			m_ptr->hook_chat = false;
			return true;
		}
	}

	return false;
}

// I used to use ClientCommand 'wait;wait;ma_repopmenu' to re-fire menus but now I do this instead 
// after the Valve ClientCommand update
void MenuManager::GameFrame()
{
	player_t player;
	for (int i = 0; i < 64; i++)
	{
		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (player.player_info->IsHLTV()) continue;

		if (game_frame_repop[i] > 0)
		{
			game_frame_repop[i]--;
			if (game_frame_repop[i] == 0)
			{
				// Valid player to run re-pop on
				this->RepopulatePage(&player);
			}
		}
#if defined ( GAME_ORANGE )
		time_t current_timestamp;
		time ( &current_timestamp );

		if ( gpGlobals->curtime > next_time_check ) {
			if ( !menu_showing[i] ) continue;

			MenuTemporal *mt_ptr = &player_list[i];
			if ( mt_ptr->menu_pages.size() == 0 ) {
				ResetMenuShowing( player.index - 1, false );

				if ( !mt_ptr->free_page ) 
				{
					menu_showing[i] = false; // shut her down!
					continue;
				}
	
				if ( ( mt_ptr->timeout_timestamp != 0 ) && (current_timestamp >= mt_ptr->timeout_timestamp) ) {
					menu_showing[i] = false;
					mt_ptr->Kill();
				} else {
					mt_ptr->free_page->Redraw(&player);
				}
				continue;
			}
		
			MenuPage *menu_page_ptr = mt_ptr->menu_pages[mt_ptr->menu_pages.size() - 1];

			if ( ( mt_ptr->timeout_timestamp != 0 ) && (current_timestamp >= mt_ptr->timeout_timestamp) ) {
				menu_showing[i] = false;
				mt_ptr->Kill();
			} else {
				menu_page_ptr->RenderPage(&player, mt_ptr->menu_pages.size());
			}
		}
#endif
	}
#if defined ( GAME_ORANGE )
	if ( gpGlobals->curtime > next_time_check ) 
		next_time_check = gpGlobals->curtime + 1.0f;
#endif
}
#if defined ( GAME_ORANGE )
float MenuManager::GetExpirationTime(int player_index) {
	if ( player_index < 0 || player_index > max_players )
		return 0;

	return menu_expiration_time[player_index];
}

bool MenuManager::GetMenuShowing(int player_index) {
	if ( player_index < 0 || player_index >= max_players )
		return false;

	return menu_showing[player_index];
}

void MenuManager::ResetMenuShowing (int player_index, bool off) {
	if ( player_index < 0 || player_index >= max_players )
		return;

	menu_showing[player_index] = !off;
}
#endif
int	RePopOption(int return_option)
{
	return (((mani_old_style_menu_behaviour.GetInt() == 0) ? return_option:CLOSE_MENU));
}

MenuManager g_menu_mgr;
MenuPage *g_page_ptr;
