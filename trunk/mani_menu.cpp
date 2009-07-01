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
#include "Color.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "mani_main.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_menu.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern	IServerPluginCallbacks *gpManiAdminPlugin;
extern	IEngineSound *esounds; // sound
extern	bf_write *msg_buffer;
extern	int	menu_message_index;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

menu_t			*menu_list=NULL;
int				menu_list_size=0;
menu_confirm_t	menu_confirm[MANI_MAX_PLAYERS];


static	void	DrawSubMenuAmxStyle (player_t *player, char *title, char *message, int next_index, char *root_command, char *more_command, bool sub_menu_back, int time_to_show );
static	void	DrawSubMenuEscapeStyle (player_t *player, char *title, char *message, int next_index, char *root_command, char *more_command, bool sub_menu_back );
static	void	DrawStandardMenuAmxStyle( player_t *player, char *title, char *message, bool show_exit);
static	void	DrawStandardMenuEscapeStyle( player_t *player, char *title, char *message);
static	int		sort_menu_name ( const void *m1,  const void *m2);

//---------------------------------------------------------------------------------
// Purpose: Free the menu list
//---------------------------------------------------------------------------------

void	FreeMenu(void)
{
	FreeList ((void **) &menu_list, &menu_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void DrawSubMenu 
(
 player_t	*player, 
 char	*title, 
 char	*message, 
 int	next_index,
 char	*root_command,
 char	*more_command,
 bool	sub_menu_back,
 int	time_to_show
 )
{

	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawSubMenuAmxStyle
			(
			player,
			title,
			message,
			next_index,
			root_command,
			more_command,
			sub_menu_back,
			time_to_show
			);
	}
	else
	{
		DrawSubMenuEscapeStyle
			(
			player,
			title,
			message,
			next_index,
			root_command,
			more_command,
			sub_menu_back
			);
	}

}

//---------------------------------------------------------------------------------
// Purpose: Draw Standard Menu
//---------------------------------------------------------------------------------
void DrawStandardMenu
(
 player_t	*player,
 char		*title,
 char		*message,
 bool		show_exit
)
{
	if (mani_use_amx_style_menu.GetInt() == 1 && gpManiGameType->IsAMXMenuAllowed())
	{
		DrawStandardMenuAmxStyle
			(
			player,
			title,
			message,
			show_exit
			);
	}
	else
	{
		DrawStandardMenuEscapeStyle
			(
			player,
			title,
			message
			);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Valve style menus, not used anymore
//---------------------------------------------------------------------------------
void DrawStandardMenuEscapeStyle
(
 player_t *player, 
 char	*title, 
 char	*message
)
{
	char num[10], msg[128], cmd[128];

	KeyValues *kv = new KeyValues( "menu" );
	kv->SetString( "title", title );
	kv->SetInt( "level", 0 );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "time", 20 );
	kv->SetString( "msg", message );

	for( int i = 0; i < menu_list_size; i++ )
	{
		Q_snprintf( num, sizeof(num), "%i", i + 1);
		Q_snprintf( msg, sizeof(msg), "%s", menu_list[i].menu_text);
		Q_snprintf( cmd, sizeof(cmd), "%s", menu_list[i].menu_command);
		KeyValues *item1 = kv->FindKey( num, true );
		item1->SetString( "msg", msg );
		item1->SetString( "command", cmd );
	}

	helpers->CreateMessage( player->entity, DIALOG_MENU, kv, gpManiAdminPlugin );
	kv->deleteThis();
	FreeMenu();

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Draw Menus using the old amx style (in game)
//---------------------------------------------------------------------------------
void DrawStandardMenuAmxStyle
(
 player_t *player,
 char	*title, 
 char	*message,
 bool	show_exit
 )
{
	char	menu_string[256];
	int		index;
	int		range = 0;
	char cmd[128];

	index = player->index - 1;

	Q_strcpy (menu_string, message);
	Q_strcat (menu_string, "\n");
	DrawMenu (player->index, -1, range, false, false, show_exit, menu_string, false);

	for (int i = 0; i < 10; i ++)
	{
		menu_confirm[index].menu_select[i].in_use = false;
	}

	for( int i = 0; i < menu_list_size; i++ )
	{
		Q_snprintf( menu_string, sizeof(menu_string), "->%i %s\n", i + 1, menu_list[i].menu_text);
		Q_snprintf( cmd, sizeof(cmd), "%s", menu_list[i].menu_command);
		Q_strcpy(menu_confirm[index].menu_select[i].command,cmd);
		menu_confirm[index].menu_select[i].in_use = true;
		range ++;
		if (i == menu_list_size - 1 && !show_exit)
		{
			DrawMenu (player->index, -1, range, false, false, false, menu_string, true);
		}
		else
		{
			DrawMenu (player->index, -1, range, false, false, show_exit, menu_string, false);
		}
	}

	FreeMenu();

	if (show_exit)
	{
 		Q_strcpy(menu_string, Translate(M_MENU_EXIT_AMX));
		menu_confirm[index].menu_select[9].in_use = true;
		DrawMenu (player->index, -1, range, false, false, show_exit, menu_string, true);
	}
	
	menu_confirm[index].in_use = true;


	return;
}


//---------------------------------------------------------------------------------
// Purpose: Valve style menus, not used anymore
//---------------------------------------------------------------------------------
void DrawSubMenuEscapeStyle
(
 player_t *player, 
 char	*title, 
 char	*message, 
 int	next_index,
 char	*root_command,
 char	*more_command,
 bool	sub_menu_back
)
{
	bool	last_item;
	int		menu_index;
	bool	back;
	char num[10], msg[128], cmd[128];


	KeyValues *kv = new KeyValues( "menu" );
	kv->SetString( "title", title );
	kv->SetInt( "level", 0 );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "time", 20 );
	kv->SetString( "msg", message );
	last_item = false;

	menu_index = 0;

	if (next_index == 0)
	{
		back = false;
	}
	else
	{
		back = true;
	}

	for( int i = next_index; i < menu_list_size; i++ )
	{
		menu_index = (i - next_index);

		if (menu_index == MAX_ESCAPE_MENU - 1)
		{
			// Hit menu limit so we need to add a 'More options' button :/

			if (back)
			{
				Q_snprintf( num, sizeof(num), "7");
				Q_snprintf( msg, sizeof(msg), Translate(M_MENU_BACK));
				Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, ((next_index + 1) - MAX_ESCAPE_MENU < 0) ? 0:((next_index + 1) - MAX_ESCAPE_MENU) , more_command );
				KeyValues *item1 = kv->FindKey( num, true );
				item1->SetString( "msg", msg );
				item1->SetString( "command", cmd );
			}
			else
			{
				if (sub_menu_back)
				{
					Q_snprintf( num, sizeof(num), "7");
					Q_snprintf( msg, sizeof(msg), Translate(M_MENU_BACK));
					Q_snprintf( cmd, sizeof(cmd), "%s", root_command);
					KeyValues *item1 = kv->FindKey( num, true );
					item1->SetString( "msg", msg );
					item1->SetString( "command", cmd );
				}
			}

			Q_snprintf( num, sizeof(num), "8");
			Q_snprintf( msg, sizeof(msg), Translate(M_MENU_MORE));
			Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, i, more_command );
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
			last_item = true;
		}
		else
		{
			Q_snprintf( num, sizeof(num), "%i", menu_index + 1);
			Q_snprintf( msg, sizeof(msg), "%s", menu_list[i].menu_text);
			Q_snprintf( cmd, sizeof(cmd), "%s", menu_list[i].menu_command);
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		if (last_item)
		{
			break;
		}
	}

	FreeMenu();

	if (!last_item)
	{
		if (back)
		{
			Q_snprintf( num, sizeof(num), "7");
			Q_snprintf( msg, sizeof(msg), Translate(M_MENU_BACK));
			Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, ((next_index + 1) - MAX_ESCAPE_MENU < 0) ? 0:((next_index + 1) - MAX_ESCAPE_MENU) , more_command );
			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}
		else
		{
			if (sub_menu_back)
			{
				Q_snprintf( num, sizeof(num), "7");
				Q_snprintf( msg, sizeof(msg), Translate(M_MENU_BACK));
				Q_snprintf( cmd, sizeof(cmd), "%s", root_command);
				KeyValues *item1 = kv->FindKey( num, true );
				item1->SetString( "msg", msg );
				item1->SetString( "command", cmd );
			}			
		}
	}

	helpers->CreateMessage( player->entity, DIALOG_MENU, kv, gpManiAdminPlugin );
	kv->deleteThis();

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Draw Menus using the old amx style (in game)
//---------------------------------------------------------------------------------
void DrawSubMenuAmxStyle
(
 player_t *player, 
 char	*title, 
 char	*message, 
 int	next_index,
 char	*root_command,
 char	*more_command,
 bool	sub_menu_back,
 int	time_to_show
 )
{
	bool	last_item;
	char	menu_string[256];
	int		index;
	int		menu_index;
	int		range = 0;
	bool	back;
	bool	use_back_key = true;
	char cmd[128];


	if (!player->entity) return;

	index = player->index - 1;

	for (int i = 0; i < 10; i ++)
	{
		menu_confirm[index].menu_select[i].in_use = false;
	}

	Q_strcpy (menu_string, message);
	Q_strcat (menu_string, "\n");
	DrawMenu (player->index, time_to_show, range, false, false, true, menu_string, false);

	last_item = false;
	menu_index = 0;

	if (next_index == 0)
	{
		back = false;
	}
	else
	{
		back = true;
	}

	for( int i = next_index; i < menu_list_size; i++ )
	{
		menu_index = (i - next_index);

		if (menu_index == MAX_AMX_MENU - 1)
		{
			// Hit menu limit so we need to add a 'More options' button :/

			if (back)
			{
				Q_strcpy(menu_string,Translate(M_MENU_BACK_AMX));
				DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
				Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, ((next_index + 1) - MAX_AMX_MENU < 0) ? 0:((next_index + 1) - MAX_AMX_MENU) , more_command );
				Q_strcpy(menu_confirm[index].menu_select[7].command, cmd);
				menu_confirm[index].menu_select[7].in_use = true;
			}
			else
			{
				if (sub_menu_back)
				{
					Q_strcpy(menu_string,Translate(M_MENU_BACK_AMX));
					DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
					Q_snprintf( cmd, sizeof(cmd), "%s", root_command);
					Q_strcpy(menu_confirm[index].menu_select[7].command, cmd);
					menu_confirm[index].menu_select[7].in_use = true;
				}
				else
				{
					use_back_key = false;
				}
			}

			Q_snprintf( menu_string, sizeof(menu_string), Translate(M_MENU_MORE_AMX));
			DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
			Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, i, more_command );
			Q_strcpy(menu_confirm[index].menu_select[8].command, cmd);
			menu_confirm[index].menu_select[8].in_use = true;
			last_item = true;
		}
		else
		{
			Q_snprintf( menu_string, sizeof(menu_string), "->%i %s\n", menu_index + 1, menu_list[i].menu_text);
			Q_snprintf( cmd, sizeof(cmd), "%s", menu_list[i].menu_command);
			Q_strcpy(menu_confirm[index].menu_select[menu_index].command,cmd);
			menu_confirm[index].menu_select[menu_index].in_use = true;
			range ++;
			DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
		}

		if (last_item)
		{
			break;
		}
	}

	FreeMenu();

	if (!last_item)
	{
		if (back)
		{
			Q_strcpy(menu_string,Translate(M_MENU_BACK_AMX));
			DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
			Q_snprintf( cmd, sizeof(cmd), "%s more %i %s", root_command, ((next_index + 1) - MAX_AMX_MENU < 0) ? 0:((next_index + 1) - MAX_AMX_MENU) , more_command );
			Q_strcpy(menu_confirm[index].menu_select[7].command, cmd);
			menu_confirm[index].menu_select[7].in_use = true;
		}
		else
		{
			if (sub_menu_back)
			{
				Q_strcpy(menu_string,Translate(M_MENU_BACK_AMX));
				DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, false);
				Q_snprintf( cmd, sizeof(cmd), "%s", root_command);
				Q_strcpy(menu_confirm[index].menu_select[7].command, cmd);
				menu_confirm[index].menu_select[7].in_use = true;
			}			
			else
			{
				use_back_key = false;
			}
		}
	}

 	Q_strcpy(menu_string, Translate(M_MENU_EXIT_AMX));
	menu_confirm[index].menu_select[9].in_use = true;
	
	menu_confirm[index].in_use = true;

	DrawMenu (player->index, time_to_show, range, use_back_key, last_item, true, menu_string, true);

	return;
}

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
	unsigned int	keys=0;
	char			menu_string_clamped[512];

	for (int i = 1; i <= range; i++)
	{
		keys |= (1 << (i-1));
	}

	if (back) keys |= (1<<7); // Key 8
	if (more) keys |= (1<<8); // Key 9
	if (cancel)	keys |= (1<<9); // Key 0

	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();
	mrf.AddPlayer(player_index);

	msg_buffer = engine->UserMessageBegin( &mrf, menu_message_index ); // Show Menu message
	if (final)
	{
		msg_buffer->WriteShort( keys ); // Key bits
	}
	else
	{
		msg_buffer->WriteShort( (1<<9) ); // Key bits
	}

	msg_buffer->WriteChar( time ); // Time for menu -1 = permanent

	if (final)
	{
		msg_buffer->WriteByte( 0 ); // Draw immediately
	}
	else
	{
		msg_buffer->WriteByte( 1 ); // Draw later more info required
	}


	// Ensure string is not more than 512 bytes
	Q_snprintf(menu_string_clamped, sizeof(menu_string_clamped), "%s", menu_string);
	msg_buffer->WriteString (menu_string_clamped);	
	engine->MessageEnd();

}

//---------------------------------------------------------------------------------
// Purpose: Sort menu by sort name into alphabetical order
//---------------------------------------------------------------------------------
void SortMenu (void)
{
	if (mani_sort_menus.GetInt() == 0) return;

	qsort(menu_list, menu_list_size, sizeof(menu_t), sort_menu_name);
}

static int sort_menu_name ( const void *m1,  const void *m2) 
{
	struct menu_t *mi1 = (struct menu_t *) m1;
	struct menu_t *mi2 = (struct menu_t *) m2;
	return strcmp(mi1->sort_name, mi2->sort_name);
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