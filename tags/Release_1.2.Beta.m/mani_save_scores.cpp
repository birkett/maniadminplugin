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
#include "mani_vfuncs.h"
#include "mani_save_scores.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

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
	save_scores_list = NULL;
	save_scores_list_size = 0;
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
	if (sv_lan && sv_lan->GetInt() == 1) return;
	if (player_ptr->is_bot) return;

	// Linear search (we need the index for removal
	for (int i = 0; i < save_scores_list_size; i++)
	{
		if (FStrEq(save_scores_list[i].steam_id, player_ptr->steam_id))
		{
			time_t current_time;
			time(&current_time);

			// Set kills/deaths
			if (mani_save_scores_tracking_time.GetInt() == 0 ||
				save_scores_list[i].disconnection_time > current_time)
			{
				// Still within time limit for re-connection so set kills/deaths
				if (gpManiGameType->IsKillsAllowed())
				{
					int offset = gpManiGameType->GetKillsOffset();

					int *frags;
					frags = ((int *)player_ptr->entity->GetUnknown() + offset);
					*frags = *frags + save_scores_list[i].kills;
				}

				if (gpManiGameType->IsDeathsAllowed())
				{
					int offset = gpManiGameType->GetDeathsOffset();

					int *deaths;
					deaths = ((int *)player_ptr->entity->GetUnknown() + offset);
					*deaths = *deaths + save_scores_list[i].deaths;
				}

				if (gpManiGameType->IsGameType(MANI_GAME_CSS) &&
					mani_save_scores_css_cash.GetInt() == 1)
				{
					save_cash_list[player_ptr->index - 1].cash = save_scores_list[i].cash;
					save_cash_list[player_ptr->index - 1].trigger = true;
				}

				if (gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					SayToPlayerColoured(player_ptr, "Saved score reloaded");
				}
				else
				{	
					SayToPlayer(player_ptr, "Saved score reloaded");
				}
//MMsg("Restored player [%s] score to Kills [%i], Deaths [%i]\n", 
//	player_ptr->name, save_scores_list[i].kills, save_scores_list[i].deaths);
			}

			// Remove player from list
			RemoveIndexFromList((void **) &save_scores_list, sizeof(save_scores_t), &save_scores_list_size, i, 
								(void *) &(save_scores_list[i]), (void *) &(save_scores_list[save_scores_list_size - 1]));
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
	spawn_count[player_ptr->index - 1] = 0;

	if (war_mode) return;
	if (mani_save_scores.GetInt() == 0) return;
	if (sv_lan && sv_lan->GetInt() == 1) return;
	if (player_ptr->is_bot) return;

	int frag_count = 0;
	int death_count = 0;
	int	cash = 0;

	// Need to save the players score
	if (gpManiGameType->IsKillsAllowed())
	{
		int offset = gpManiGameType->GetKillsOffset();

		int *frags;
		frags = ((int *)player_ptr->entity->GetUnknown() + offset);
		frag_count = *frags;
	}

	// Need to save the players score
	if (gpManiGameType->IsDeathsAllowed())
	{
		int offset = gpManiGameType->GetDeathsOffset();

		int *deaths;
		deaths = ((int *)player_ptr->entity->GetUnknown() + offset);
		death_count = *deaths;
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS) &&
		mani_save_scores_css_cash.GetInt() == 1)
	{
		cash = Prop_GetAccount(player_ptr->entity);
	}

	save_scores_t save_scores;
	Q_strcpy(save_scores.steam_id, player_ptr->steam_id);
	time_t current_time;
	time(&current_time);

	save_scores.kills = frag_count;
	save_scores.deaths = death_count;
	save_scores.cash = cash;
	save_scores.disconnection_time = current_time + (mani_save_scores_tracking_time.GetInt() * 60);


//MMsg("Storing Player [%s], Kills [%i], Deaths [%i], Cash [%i], Current Time [%ld], Track Time [%ld]\n", 
//	player_ptr->name, save_scores.kills, save_scores.deaths, save_scores.cash, current_time, save_scores.disconnection_time);

	AddToList((void **) &save_scores_list, sizeof(save_scores_t), &save_scores_list_size);
	save_scores_list[save_scores_list_size - 1] = save_scores;

}

//---------------------------------------------------------------------------------
// Purpose: Player Spawned
//---------------------------------------------------------------------------------
void ManiSaveScores::PlayerSpawn(player_t *player_ptr)
{
//MMsg("PLAYER %s SPAWNED, SPAWN_COUNT [%i]\n", player_ptr->name, spawn_count[player_ptr->index - 1]);

	if (war_mode) return; 
	if (mani_save_scores.GetInt() == 0 || mani_save_scores_css_cash.GetInt() == 0) return;
	if (sv_lan && sv_lan->GetInt() == 1) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (player_ptr->is_bot) return;

	spawn_count[player_ptr->index - 1] ++;

	if (!save_cash_list[player_ptr->index - 1].trigger) return;
	if (spawn_count[player_ptr->index - 1] < 2) return;

	save_cash_list[player_ptr->index - 1].trigger = false;
	SayToPlayerColoured(player_ptr, "Cash restored to last known amount");
	Prop_SetAccount(player_ptr->entity, save_cash_list[player_ptr->index - 1].cash); 
}

//---------------------------------------------------------------------------------
// Purpose: Clear down the scores
//---------------------------------------------------------------------------------
void ManiSaveScores::ResetScores(void)
{
	FreeList((void **) &save_scores_list, &save_scores_list_size);
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		save_cash_list[i].cash = 0;
		save_cash_list[i].trigger = false;
		spawn_count[i] = 0;
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
