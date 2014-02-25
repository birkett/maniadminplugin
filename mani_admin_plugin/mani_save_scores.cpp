//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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
#include "mani_vfuncs.h"
#include "mani_util.h"
#include "mani_vars.h"
#include "mani_save_scores.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

ConVar mani_save_scores ("mani_save_scores", "0", 0, "0 = disabled, 1 = scores are saved when players disconnect and reconnect", true, 0, true, 1);
ConVar mani_save_scores_tracking_time ("mani_save_scores_tracking_time", "5", 0, "Time in minutes before player is removed from tracking list, set to 0 for no limit", true, 0, true, 60);
ConVar mani_save_scores_css_cash ("mani_save_scores_css_cash", "0", 0, "1 = Save players cash, 0 = Do not save players cash", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiSaveScores::ManiSaveScores()
{
	// Init
	save_scores_list.clear();
	gpManiSaveScores = this;
}

ManiSaveScores::~ManiSaveScores()
{
	// Cleanup
	this->ResetScores();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiSaveScores::Load(void)
{
	this->ResetScores();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin UnLoaded
//---------------------------------------------------------------------------------
void	ManiSaveScores::Unload(void)
{
	this->ResetScores();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiSaveScores::LevelInit(void)
{
	this->ResetScores();
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiSaveScores::NetworkIDValidated(player_t *player_ptr)
{
	if (war_mode) return;
	if (mani_save_scores.GetInt() == 0) return;
	if (IsLAN()) return;
	if (player_ptr->is_bot) return;

	// Linear search (we need the index for removal
	std::vector<save_scores_t>::iterator i;
	for (i = save_scores_list.begin(); i != save_scores_list.end(); ++i)
	{
		if (strcmp(i->steam_id, player_ptr->steam_id) == 0)
		{
			time_t current_time;
			time(&current_time);

			// Set kills/deaths
			if (mani_save_scores_tracking_time.GetInt() == 0 ||
				i->disconnection_time > current_time)
			{
				// Still within time limit for re-connection so set kills/deaths
				CBaseEntity *pCBE = EdictToCBE(player_ptr->entity);

				if (Map_CanUseMap(pCBE, MANI_VAR_FRAGS))
				{
					int frags = Map_GetVal(pCBE, MANI_VAR_FRAGS, 0);
					frags += i->kills;
					Map_SetVal(pCBE, MANI_VAR_FRAGS, frags);
				}

				if (Map_CanUseMap(pCBE, MANI_VAR_DEATHS))
				{
					int deaths = Map_GetVal(pCBE, MANI_VAR_DEATHS, 0);
					deaths += i->deaths;
					Map_SetVal(pCBE, MANI_VAR_DEATHS, deaths);
				}

				if ((gpManiGameType->IsGameType(MANI_GAME_CSS) || gpManiGameType->IsGameType(MANI_GAME_CSGO)) &&
					mani_save_scores_css_cash.GetInt() == 1)
				{
					save_cash_list[player_ptr->index - 1].cash = i->cash;
					save_cash_list[player_ptr->index - 1].trigger = true;
				}

				SayToPlayer(LIGHT_GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3100));
			}

			save_scores_list.erase(i);
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiSaveScores::ClientDisconnect(player_t	*player_ptr)
{
	save_cash_list[player_ptr->index - 1].trigger = false;
	save_cash_list[player_ptr->index - 1].cash = 0;

	if (war_mode) return;
	if (mani_save_scores.GetInt() == 0) return;
	if (IsLAN()) return;
	if (player_ptr->is_bot) return;

	int frag_count = 0;
	int death_count = 0;
	int	cash = 0;

	CBaseEntity *pCBE = EdictToCBE(player_ptr->entity);

	// Need to save the players score
	if (Map_CanUseMap(pCBE, MANI_VAR_FRAGS))
	{
		frag_count = Map_GetVal(pCBE, MANI_VAR_FRAGS, 0);
	}

	if (Map_CanUseMap(pCBE, MANI_VAR_DEATHS))
	{
		death_count = Map_GetVal(pCBE, MANI_VAR_DEATHS, 0);
	}

	if ((gpManiGameType->IsGameType(MANI_GAME_CSS) || gpManiGameType->IsGameType(MANI_GAME_CSGO)) &&
		mani_save_scores_css_cash.GetInt() == 1)
	{
		cash = Prop_GetVal(player_ptr->entity, MANI_PROP_ACCOUNT, 0);
	}

	save_scores_t save_scores;
	Q_strcpy(save_scores.steam_id, player_ptr->steam_id);
	time_t current_time;
	time(&current_time);

	save_scores.kills = frag_count;
	save_scores.deaths = death_count;
	save_scores.cash = cash;
	save_scores.disconnection_time = current_time + (mani_save_scores_tracking_time.GetInt() * 60);

	save_scores_list.push_back(save_scores);
}

//---------------------------------------------------------------------------------
// Purpose: Player Spawned
//---------------------------------------------------------------------------------
void ManiSaveScores::PlayerSpawn(player_t *player_ptr)
{
//MMsg("PLAYER %s SPAWNED, SPAWN_COUNT [%i]\n", player_ptr->name, spawn_count[player_ptr->index - 1]);

	if (war_mode) return; 
	if (mani_save_scores.GetInt() == 0 || mani_save_scores_css_cash.GetInt() == 0) return;
	if (IsLAN()) return;
	if ((!gpManiGameType->IsGameType(MANI_GAME_CSS)) && (!gpManiGameType->IsGameType(MANI_GAME_CSGO))) return;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;
	if (player_ptr->is_bot) return;

	if (!save_cash_list[player_ptr->index - 1].trigger) return;
	save_cash_list[player_ptr->index - 1].trigger = false;

	// Set cash to highest of the two values
	int new_cash = save_cash_list[player_ptr->index - 1].cash;
	int current_cash = Prop_GetVal(player_ptr->entity, MANI_PROP_ACCOUNT, 0);
	if (current_cash < new_cash)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3101, "%i", new_cash));
		Prop_SetVal(player_ptr->entity, MANI_PROP_ACCOUNT, new_cash); 
	}
}

//---------------------------------------------------------------------------------
// Purpose: Clear down the scores
//---------------------------------------------------------------------------------
void ManiSaveScores::ResetScores(void)
{
	save_scores_list.clear();
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		save_cash_list[i].cash = 0;
		save_cash_list[i].trigger = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Game Commencing
//---------------------------------------------------------------------------------
void	ManiSaveScores::GameCommencing(void)
{
	this->ResetScores();
}

ManiSaveScores	g_ManiSaveScores;
ManiSaveScores	*gpManiSaveScores;
