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
#include "mani_commands.h"
#include "mani_vfuncs.h"
#include "mani_vars.h"
#include "mani_css_betting.h" 
#include "mani_warmuptimer.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

ConVar mani_css_betting ("mani_css_betting", "0", 0, "0 = disable css betting, 1 = enable css betting", true, 0, true, 1);
ConVar mani_css_betting_dead_only ("mani_css_betting_dead_only", "0", 0, "0 = players can bet when alive or dead, 1 = players can only bet when dead", true, 0, true, 1);
ConVar mani_css_betting_pay_losing_bets ("mani_css_betting_pay_losing_bets", "0", 0, "0 = disable, > 1 = If one player is up against X or more players, they receive the losing bets placed if they win", true, 0, true, 32);
ConVar mani_css_betting_announce_one_v_one ("mani_css_betting_announce_one_v_one", "0", 0, "0 = disable, 1 = enable", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiCSSBetting::ManiCSSBetting()
{
	this->Reset();
	gpManiCSSBetting = this;
}

ManiCSSBetting::~ManiCSSBetting()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiCSSBetting::Load(void)
{
	this->Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiCSSBetting::LevelInit(void)
{
	this->Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiCSSBetting::ClientDisconnect(player_t	*player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (war_mode) return;
	if (mani_css_betting.GetInt() == 0) return;

	this->PlayerNotAlive();
	this->ResetPlayer(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiCSSBetting::PlayerDeath(void)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (war_mode) return;
	if (mani_css_betting.GetInt() == 0) return;
	this->PlayerNotAlive();
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiCSSBetting::PlayerNotAlive(void)
{
	int team_t_count;
	int team_ct_count;
	int	t_index;
	int ct_index;

	this->GetAlivePlayerCount(&team_t_count, &t_index, &team_ct_count, &ct_index);

	if (team_t_count == team_ct_count && team_t_count == 0) return;

	if (mani_css_betting_announce_one_v_one.GetInt() == 1)
	{
		if (team_t_count == 1 && team_ct_count == 1)
		{
			if (mani_css_betting_dead_only.GetInt() == 1)
			{
				SayToDead(LIGHT_GREEN_CHAT, "It is 1 vs 1, place your bets!");
			}
			else
			{
				SayToAll(LIGHT_GREEN_CHAT, false, "%s", Translate(NULL, 1300));
			}
		}
	}

	if (mani_css_betting_pay_losing_bets.GetInt() < 2)
	{
		return;
	}

	// Last player already found
	if (last_ct_index != -1 || last_t_index != -1)
	{
		return;
	}

	if (team_ct_count > 1 && team_t_count > 1)
	{
		return;
	}

	// Setup losing bet payout value for team with only 1 player left
	if (team_ct_count == 1)
	{
		if (team_t_count >= mani_css_betting_pay_losing_bets.GetInt())
		{
			// Store index and number of players the player is up against
			last_ct_index = ct_index;
			versus_1 = team_t_count;
			return;
		}
	}
	else if (team_t_count == 1)
	{
		if (team_ct_count >= mani_css_betting_pay_losing_bets.GetInt())
		{
			// Store index and number of players the player is up against
			last_t_index = t_index;
			versus_1 = team_ct_count;
			return;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Player places a bet
//---------------------------------------------------------------------------------
void ManiCSSBetting::PlayerBet(player_t	*player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (war_mode) return;
	if (mani_css_betting.GetInt() == 0) return;

	if (gpCmd->Cmd_Argc() == 1)
	{
		BetRulesFreePage *ptr = new BetRulesFreePage;
		g_menu_mgr.AddFreePage(player_ptr, ptr, 5, 30);
		if (!ptr->Render(player_ptr))
		{
			g_menu_mgr.Kill();
		}
		return;
	}

	if (gpCmd->Cmd_Argc() != 3)
	{
		return;
	}

	int index = player_ptr->index - 1;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1301));
		return;
	}

	if (!player_ptr->is_dead && mani_css_betting_dead_only.GetInt() == 1)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1302));
		return;
	}

	if (bet_list[index].amount_bet != 0)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1303));
		return;
	}

	// Work out team to win
	int team = 0;
	if (FStrEq(gpCmd->Cmd_Argv(1), "T"))
	{
		// Team T to win
		team = 2;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(1), "CT"))
	{
		// Team CT to win
		team = 3;
	}
	else
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1304, "%s", gpCmd->Cmd_Argv(1)));
		return;
	}

	int team_t_count;
	int team_ct_count;
	int	dummy;

	this->GetAlivePlayerCount(&team_t_count, &dummy, &team_ct_count, &dummy);

	if (team_ct_count == 0 || team_t_count == 0)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1305));
		return;
	}

	int account_value = Prop_GetVal(player_ptr->entity, MANI_PROP_ACCOUNT, 0);
	if (account_value == 0)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1306));
		return;
	}

	int bet_amount = 0;
	if (FStrEq(gpCmd->Cmd_Argv(2), "ALL"))
	{
		// Use all money available
		bet_amount = account_value;
	}
	else if (FStrEq(gpCmd->Cmd_Argv(2), "HALF"))
	{
		// Use all money available
		if (account_value == 1)
		{
			bet_amount = 1;
		}
		else
		{
			bet_amount = account_value / 2;
		}
	}
	else
	{
		// Get value
		bet_amount = atoi(gpCmd->Cmd_Argv(2));
	}

	if (bet_amount > account_value)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1307));
		return;
	}

	// Bet value too low or atoi chopped it to zero
	if (bet_amount <= 0)
	{
		SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1308, "%s", gpCmd->Cmd_Argv(2)));
		return;
	}

	// Okay we've got a good bet value, now calulate the odds
	float multiplier = 0.0;
	char odds_string[16];
	if (team == 2)
	{
		// Terrorist bet
		multiplier = (float) team_ct_count / (float) team_t_count;
		snprintf (odds_string, sizeof(odds_string), "%i-%i", team_ct_count, team_t_count);
	}
	else
	{
		// CT bet
		multiplier = (float) team_t_count / (float) team_ct_count;
		snprintf (odds_string, sizeof(odds_string), "%i-%i", team_t_count, team_ct_count);
	}

	if (team_ct_count == team_t_count)
	{
		snprintf (odds_string, sizeof(odds_string), "%s", Translate(player_ptr, 1309));
	}

	// Show odds to player plus the amount they stand to win
	bet_list[index].amount_bet = bet_amount;
	bet_list[index].win_payout = (int) ((float) bet_amount * (float) multiplier);
	bet_list[index].winning_team = team;

	Prop_SetVal(player_ptr->entity, MANI_PROP_ACCOUNT, (account_value - bet_amount));
	SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 1310, "%s%i%i", odds_string, bet_list[index].win_payout, bet_amount));
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Payout bet winnings and losses
//---------------------------------------------------------------------------------
void ManiCSSBetting::CSSRoundEnd(int winning_team)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;
	if (war_mode) return;
	if (mani_css_betting.GetInt() == 0) return;

	int lost_total = 0;

	for (int i = 0; i < max_players; i++)
	{
		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			this->ResetPlayer(i);
			continue;
		}

		if (bet_list[i].amount_bet == 0) continue;

		if (bet_list[i].winning_team == winning_team)
		{
			// Payout to player
			int new_value = Prop_GetVal(player.entity, MANI_PROP_ACCOUNT, 0);
			new_value += (bet_list[i].win_payout + bet_list[i].amount_bet);
			if (new_value > 16000)
			{
				new_value = 16000;
			}

			// Attacker must receive the bounty
			Prop_SetVal(player.entity, MANI_PROP_ACCOUNT, new_value);
			SayToPlayer(LIGHT_GREEN_CHAT, &player, "%s", Translate(&player, 1311, "%i%i", bet_list[i].win_payout, bet_list[i].amount_bet));
			this->ResetPlayer(i);
		}
		else
		{
			SayToPlayer(LIGHT_GREEN_CHAT, &player, "%s", Translate(&player, 1312, "%i", bet_list[i].amount_bet));
			lost_total += bet_list[i].amount_bet;
			this->ResetPlayer(i);
		}
	}

	if (mani_css_betting_pay_losing_bets.GetInt() > 1)
	{
		int last_player_win = -1;
		if (last_ct_index != -1 && winning_team == 3)
		{
			last_player_win = last_ct_index;
		}
		else if (last_t_index != -1 && winning_team == 2)
		{
			last_player_win = last_t_index;
		}

		if (last_player_win != -1)
		{
			player_t player;

			player.index = last_player_win;
			if (FindPlayerByIndex(&player))
			{
				// Give player the money
				int new_value = Prop_GetVal(player.entity, MANI_PROP_ACCOUNT, 0);
				new_value += lost_total;
				if (new_value > 16000)
				{
					new_value = 16000;
				}

				Prop_SetVal(player.entity, MANI_PROP_ACCOUNT, new_value);
				SayToPlayer(LIGHT_GREEN_CHAT, &player, "%s", Translate(&player, 1313, "%i%i", lost_total));
			}
		}
	}

	versus_1 = -1;
	last_ct_index = -1;
	last_t_index = -1;
}

//---------------------------------------------------------------------------------
// Purpose: Show top 5 bounties
//---------------------------------------------------------------------------------
bool BetRulesFreePage::OptionSelected(player_t *player_ptr, const int option)
{
	return false;
}

bool BetRulesFreePage::Render(player_t *player_ptr)
{
	char	menu_string[512];

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1314));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1315));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1316));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1317));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1318));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1319));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);
	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, 1320));
	DrawMenu (player_ptr->index, 30, 0, false, false, true, menu_string, false);

	snprintf(menu_string, sizeof(menu_string), "%s", Translate(player_ptr, M_MENU_EXIT_AMX));
	DrawMenu (player_ptr->index, 30, 7, true, true, true, menu_string, true);
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Reset the bounty lists
//---------------------------------------------------------------------------------
void	ManiCSSBetting::Reset(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayer(i);
	}

	last_ct_index = -1;
	last_t_index = -1;
}

//---------------------------------------------------------------------------------
// Purpose: Reset the players betting info
//---------------------------------------------------------------------------------
void	ManiCSSBetting::ResetPlayer(int index)
{
	bet_list[index].amount_bet = 0;
	bet_list[index].win_payout = 0;
	bet_list[index].winning_team = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Reset the players betting info
//---------------------------------------------------------------------------------
void	ManiCSSBetting::GetAlivePlayerCount(int *t_count, int *t_index, int *ct_count, int *ct_index)
{
	*t_count = 0;
	*ct_count = 0;
	*t_index = -1;
	*ct_index = -1;

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;

		if (player.team == 2)
		{
			*t_count = *t_count + 1;
			*t_index = i;
			continue;
		}

		if (player.team == 3)
		{
			*ct_count = *ct_count + 1;
			*ct_index = i;
			continue;
		}
	}

	return;
}

ManiCSSBetting	g_ManiCSSBetting;
ManiCSSBetting	*gpManiCSSBetting;
