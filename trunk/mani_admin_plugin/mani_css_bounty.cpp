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
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_effects.h"
#include "mani_vfuncs.h"
#include "mani_vars.h"
#include "mani_css_bounty.h" 
#include "mani_warmuptimer.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

ConVar mani_css_bounty ("mani_css_bounty", "0", 0, "0 = disable css bounty, 1 = enable css bounty", true, 0, true, 1);
ConVar mani_css_bounty_kill_streak ("mani_css_bounty_kill_streak", "5", 0, "Kill streak required before bounty is started", true, 1, true, 100);
ConVar mani_css_bounty_start_cash ("mani_css_bounty_start_cash", "1000", 0, "Start bounty cash amount", true, 0, true, 16000);
ConVar mani_css_bounty_survive_round_cash ("mani_css_bounty_survive_round_cash", "500", 0, "Amount of cash given if you survive the round after a bounty has started", true, 0, true, 16000);
ConVar mani_css_bounty_kill_cash ("mani_css_bounty_kill_cash", "250", 0, "Amount of cash given for each kill after a bounty has started", true, 0, true, 16000);

ConVar mani_css_bounty_ct_red ("mani_css_bounty_ct_red", "255", 0, "Red component for CT with bounty", true, 0, true, 255);
ConVar mani_css_bounty_ct_green ("mani_css_bounty_ct_green", "255", 0, "Green component for CT with bounty", true, 0, true, 255);
ConVar mani_css_bounty_ct_blue ("mani_css_bounty_ct_blue", "255", 0, "Blue component for CT with bounty", true, 0, true, 255);
ConVar mani_css_bounty_ct_alpha ("mani_css_bounty_ct_alpha", "255", 0, "Alpha component for CT with bounty", true, 0, true, 255);

ConVar mani_css_bounty_t_red ("mani_css_bounty_t_red", "255", 0, "Red component for T with bounty", true, 0, true, 255);
ConVar mani_css_bounty_t_green ("mani_css_bounty_t_green", "255", 0, "Green component for T with bounty", true, 0, true, 255);
ConVar mani_css_bounty_t_blue ("mani_css_bounty_t_blue", "255", 0, "Blue component for T with bounty", true, 0, true, 255);
ConVar mani_css_bounty_t_alpha ("mani_css_bounty_t_alpha", "255", 0, "Alpha component for T with bounty", true, 0, true, 255);

static int sort_by_bounty ( const void *m1,  const void *m2);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiCSSBounty::ManiCSSBounty()
{
	this->Reset();
	gpManiCSSBounty = this;
}

ManiCSSBounty::~ManiCSSBounty()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiCSSBounty::Load(void)
{
	this->Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiCSSBounty::LevelInit(void)
{
	this->Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Client Active
//---------------------------------------------------------------------------------
void	ManiCSSBounty::ClientActive(player_t *player_ptr)
{
	bounty_list[player_ptr->index - 1].current_bounty = 0;
	bounty_list[player_ptr->index - 1].kill_streak = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiCSSBounty::ClientDisconnect(player_t	*player_ptr)
{
	bounty_list[player_ptr->index - 1].current_bounty = 0;
	bounty_list[player_ptr->index - 1].kill_streak = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiCSSBounty::PlayerDeath
(
 player_t *victim_ptr, 
 player_t *attacker_ptr, 
 bool attacker_exists
 )
{
	int victim_index;
	int attacker_index;

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (war_mode) return;
	if (mani_css_bounty.GetInt() == 0) return;

	if (attacker_ptr->user_id <= 0 || !attacker_exists)
	{
		// World attacked player (i.e. fell too far)
		return;
	}

	if (victim_ptr->team == attacker_ptr->team)
	{
		// Team kill or suicide
		return;
	}

	// Good clean kill
	attacker_index = attacker_ptr->index - 1;
	victim_index = victim_ptr->index - 1;

	// Handle giving attacker a bounty on their head
	bounty_list[attacker_index].kill_streak ++;
	if (bounty_list[attacker_index].kill_streak == mani_css_bounty_kill_streak.GetInt())
	{
		// Start Bounty
		bounty_list[attacker_index].current_bounty = mani_css_bounty_start_cash.GetInt();
		this->SetPlayerColour(attacker_ptr);
		SayToAll(LIGHT_GREEN_CHAT, true, "%s", Translate(NULL, 1340, "%s%i", attacker_ptr->name, bounty_list[attacker_index].current_bounty));
	}
	else if (bounty_list[attacker_index].kill_streak > mani_css_bounty_kill_streak.GetInt())
	{
		// Add new kill bonus
		bounty_list[attacker_index].current_bounty += mani_css_bounty_kill_cash.GetInt();
		this->SetPlayerColour(attacker_ptr);
	}

	if (bounty_list[victim_index].kill_streak >= mani_css_bounty_kill_streak.GetInt())
	{
		int new_value;

		new_value = Prop_GetVal(attacker_ptr->entity, MANI_PROP_ACCOUNT, 0);
		new_value += bounty_list[victim_index].current_bounty;
		if (new_value > 16000)
		{
			new_value = 16000;
		}
		// Attacker must receive the bounty
		Prop_SetVal(attacker_ptr->entity, MANI_PROP_ACCOUNT, new_value);
		SayToPlayer(LIGHT_GREEN_CHAT, attacker_ptr, "%s", Translate(attacker_ptr, 1341, "%i%s", bounty_list[victim_index].current_bounty, victim_ptr->name));
		SayToPlayer(LIGHT_GREEN_CHAT, victim_ptr, "%s", Translate(victim_ptr, 1342, "%s%i", attacker_ptr->name, bounty_list[victim_index].current_bounty));
	}

	// Reset victim bounty and kill streak
	bounty_list[victim_index].kill_streak = 0;
	bounty_list[victim_index].current_bounty = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Add bonus to those who survived and have bounty against them
//---------------------------------------------------------------------------------
void ManiCSSBounty::CSSRoundEnd(const char *message)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (war_mode) return;
	if (mani_css_bounty.GetInt() == 0) return;

	if (FStrEq(message, "#Game_Commencing"))
	{
		this->Reset();
		return;
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player))
		{
			bounty_list[i - 1].current_bounty = 0;
			bounty_list[i - 1].kill_streak = 0;
			continue;
		}

		if (player.is_dead) continue;
		if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

		if (bounty_list[i - 1].kill_streak >= mani_css_bounty_kill_streak.GetInt())
		{
			// Player has a bounty so add survival cash
			bounty_list[i - 1].current_bounty += mani_css_bounty_survive_round_cash.GetInt();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: PlayerSpawned, update their steam id and name in the stats
//---------------------------------------------------------------------------------
void ManiCSSBounty::PlayerSpawn(player_t *player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (war_mode) return;
	if (mani_css_bounty.GetInt() == 0) return;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;

	if (bounty_list[player_ptr->index - 1].kill_streak >= mani_css_bounty_kill_streak.GetInt())
	{
		this->SetPlayerColour(player_ptr);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Show top 5 bounties
//---------------------------------------------------------------------------------
bool BountyFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	return false;
}

bool BountyFreePage::Render(player_t *player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;
	if (war_mode) return false;
	if (mani_css_bounty.GetInt() == 0) return false;

	top_bounty_t	*top_bounty_list = NULL;
	int				top_bounty_list_size = 0;

	for (int i = 0; i < max_players; i++)
	{
		player_t player;

		if (gpManiCSSBounty->bounty_list[i].kill_streak >= mani_css_bounty_kill_streak.GetInt())
		{
			player.index = i + 1;
			if (!FindPlayerByIndex(&player)) continue;
			if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) continue;

			// Valid player with bounty so add bounty and name
			AddToList((void **) &top_bounty_list, sizeof (top_bounty_t), &top_bounty_list_size);
			top_bounty_list[top_bounty_list_size - 1].bounty = gpManiCSSBounty->bounty_list[i].current_bounty;
			Q_strcpy(top_bounty_list[top_bounty_list_size - 1].name, player.name);
		}
	}

	qsort(top_bounty_list, top_bounty_list_size, sizeof(top_bounty_t), sort_by_bounty);

	char	menu_string[512];

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1343));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1344, "%i", mani_css_bounty_kill_streak.GetInt()));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1345, "%i", mani_css_bounty_start_cash.GetInt()));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1346, "%i", mani_css_bounty_kill_cash.GetInt()));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1347, "%i", mani_css_bounty_survive_round_cash.GetInt()));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1348));
	DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);

	if (top_bounty_list_size == 0)
	{
		snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1349));
		DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
		snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, M_MENU_EXIT_AMX));
		DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 7, true, true, true, menu_string, true);
	}
	else
	{
		for (int i = 0; i < top_bounty_list_size && i < 5; i++)
		{
			snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1350, "%i%s", top_bounty_list[i].bounty, top_bounty_list[i].name));
			DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 0, false, false, true, menu_string, false);
		}

		snprintf(menu_string, sizeof(menu_string), Translate(player_ptr, M_MENU_EXIT_AMX));
		DrawMenu (player_ptr->index, mani_stats_top_display_time.GetInt(), 7, true, true, true, menu_string, true);
	}

	// Clean up memory
	FreeList((void **) &top_bounty_list, &top_bounty_list_size);
	return true;
}

void BountyFreePage::Redraw(player_t *player_ptr)
{
	this->Render(player_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Set a players colour to bounty defaults
//---------------------------------------------------------------------------------
void ManiCSSBounty::SetPlayerColour(player_t *player_ptr)
{
	int r,g,b,a;

	if (player_ptr->team == 3)
	{
		r = mani_css_bounty_ct_red.GetInt();
		g = mani_css_bounty_ct_green.GetInt();
		b = mani_css_bounty_ct_blue.GetInt();
		a = mani_css_bounty_ct_alpha.GetInt();
	}
	else
	{
		r = mani_css_bounty_t_red.GetInt();
		g = mani_css_bounty_t_green.GetInt();
		b = mani_css_bounty_t_blue.GetInt();
		a = mani_css_bounty_t_alpha.GetInt();
	}

	if (r == 255 && g == 255 && b == 255 && a == 255) return;

	ProcessSetColour(player_ptr->entity, r, g, b, a);
}

//---------------------------------------------------------------------------------
// Purpose: Reset the bounty lists
//---------------------------------------------------------------------------------
void	ManiCSSBounty::Reset(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		bounty_list[i].current_bounty = 0;
		bounty_list[i].kill_streak = 0;
	}
}

static int sort_by_bounty ( const void *m1,  const void *m2) 
{
	struct top_bounty_t *mi1 = (struct top_bounty_t *) m1;
	struct top_bounty_t *mi2 = (struct top_bounty_t *) m2;

	if (mi1->bounty > mi2->bounty)
	{
		return -1;
	}
	else if (mi1->bounty < mi2->bounty)
	{
		return 1;
	}

	return strcmp(mi1->name, mi2->name);
}

ManiCSSBounty	g_ManiCSSBounty;
ManiCSSBounty	*gpManiCSSBounty;
