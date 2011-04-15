//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_parser.h"
#include "mani_output.h"
#include "mani_adverts.h"

extern	IFileSystem	*filesystem;
extern	IServerPluginHelpers *helpers;
extern	IServerPluginCallbacks *gpManiISPCCallback;
extern	CGlobalVars *gpGlobals;
extern	int	max_players;

static	advert_t	*advert_list = NULL;
static	int			advert_list_size = 0;

static	float	next_ad_time = 20.0;
static	int		ad_index = 0;

static	void ShowAdvert(const char* advert_text);

//---------------------------------------------------------------------------------
// Purpose: Load adverts into memory
//---------------------------------------------------------------------------------
void	LoadAdverts(void)
{	
	FileHandle_t file_handle;
	char	base_filename[512];
	char	ad_text[512];

	FreeAdverts();

	//Get ad list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/adverts.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load adverts.txt\n");
		mani_adverts.SetValue( 0 );
	}
	else
	{
//		MMsg("Advert list\n");
		while (filesystem->ReadLine (ad_text, sizeof(ad_text), file_handle) != NULL)
		{
			if (!ParseLine(ad_text, false, true))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &advert_list, sizeof(advert_t), &advert_list_size);
			Q_strcpy(advert_list[advert_list_size - 1].advert_text, ad_text);
//			MMsg("[%s]\n", ad_text);
		}

		if (advert_list_size == 0)
		{
			mani_adverts.SetValue( 0 );
		}

		filesystem->Close(file_handle);
	}

	ad_index = 0;
	next_ad_time = 20.0;
}

//---------------------------------------------------------------------------------
// Purpose: Displays an advert on every client screen 
//---------------------------------------------------------------------------------
static
void ShowAdvert(const char* advert_text)
{
	char	substitute_text[512];
	char	substitute_text2[512];
	Color	col;

	if (mani_adverts_top_left.GetInt() == 0 &&
		mani_adverts_chat_area.GetInt() == 0 &&
		mani_adverts_bottom_area.GetInt() == 0)
	{
		return;
	}

	ParseSubstituteStrings((player_t *) NULL, advert_text, substitute_text);
	col.SetColor (mani_advert_col_red.GetInt(), mani_advert_col_green.GetInt(), mani_advert_col_blue.GetInt(), 255);
	ParseColourStrings(substitute_text, substitute_text2, &col);

	if (mani_adverts_top_left.GetInt() == 1)
	{	
		for (int i = 1; i <= max_players; i++ )
		{
			player_t player;

			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;
			if (mani_advert_dead_only.GetInt() == 1 && !player.is_dead) continue;

			KeyValues *kv = new KeyValues("Msg");
			kv->SetString("title", substitute_text2);
			kv->SetString("Msg", "advert");
			kv->SetColor("color", col); 
			kv->SetInt("level", 5);
			kv->SetInt("time", 10);
			helpers->CreateMessage(player.entity, DIALOG_MSG, kv, gpManiISPCCallback);
			kv->deleteThis();
		}
	}

	if (mani_adverts_chat_area.GetInt() == 1)
	{
		if (mani_advert_dead_only.GetInt() == 1)
		{
			SayToDead(GREEN_CHAT, "%s", substitute_text2);
		}
		else
		{
			SayToAll(GREEN_CHAT, true, "%s", substitute_text2);
		}
	}

	if (mani_adverts_bottom_area.GetInt() == 1)
	{
		SplitHintString(substitute_text2, 35);
		MRecipientFilter mrf;
		mrf.MakeReliable();

		bool player_found = false;

		for (int i = 1; i <= max_players; i ++)
		{
			player_t player;

			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_bot) continue;
			if (mani_advert_dead_only.GetInt() == 1 && !player.is_dead) continue;
			mrf.AddPlayer(i);
			player_found = true;
		}

		if (player_found)
		{
			UTIL_SayHint(&mrf, substitute_text2);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free adverts from memory
//---------------------------------------------------------------------------------
void FreeAdverts(void)
{
	FreeList((void **) &advert_list, &advert_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Process drawing of the adverts
//---------------------------------------------------------------------------------
void	ProcessAdverts(void)
{
	if (mani_adverts.GetInt() == 1)
	{
		if (advert_list_size != 0)
		{
			if (gpGlobals->curtime > next_ad_time)
			{
				ShowAdvert(advert_list[ad_index ++].advert_text);
				if (ad_index == advert_list_size) ad_index = 0;
				next_ad_time = gpGlobals->curtime + mani_time_between_adverts.GetFloat();
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Toggle adverts on and off
//---------------------------------------------------------------------------------
void ToggleAdverts(player_t *player_ptr)
{
	if (mani_adverts.GetInt() == 1)
	{
		mani_adverts.SetValue(0);
		SayToAll (GREEN_CHAT, true, "ADMIN %s disabled adverts", player_ptr->name);
		LogCommand (player_ptr, "Disable adverts\n");
	}
	else
	{
		mani_adverts.SetValue(1);
		next_ad_time = gpGlobals->curtime + 5.0;
		SayToAll (GREEN_CHAT, true, "ADMIN %s enabled adverts", player_ptr->name);
		LogCommand (player_ptr, "Enable adverts\n");
	}
}	
