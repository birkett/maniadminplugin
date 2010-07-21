//
// Mani Admin Plugin
//
// Copyright © 2009-2010 Giles Millward (Mani). All rights reserved.
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
#include "mani_sounds.h"
#include "mani_team_join.h" 
#include "mani_client.h"
#include "mani_client_flags.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

static int sort_saved_team_by_steam_id ( const void *m1,  const void *m2);

ConVar mani_team_join_force_auto ("mani_team_join_force_auto", "0", 0, "0 = disabled, 1 = players are forced to select auto when joining", true, 0, true, 1);
ConVar mani_team_join_keep_same_team ("mani_team_join_keep_same_team", "1", 0, "Players re-joining on the same map will be assigned to the same team", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiTeamJoin::ManiTeamJoin()
{
	// Init
	saved_team_list = NULL;
	saved_team_list_size = 0;
	gpManiTeamJoin = this;
}

ManiTeamJoin::~ManiTeamJoin()
{
	// Cleanup
	this->ResetTeamList();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiTeamJoin::Load(void)
{
	this->ResetTeamList();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Unloaded
//---------------------------------------------------------------------------------
void	ManiTeamJoin::Unload(void)
{
	this->ResetTeamList();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiTeamJoin::LevelInit(void)
{
	this->ResetTeamList();
}

//---------------------------------------------------------------------------------
// Purpose: A 'player_team event happened
//---------------------------------------------------------------------------------
void ManiTeamJoin::PlayerTeamEvent(player_t *player_ptr)
{

	if (war_mode) return; 
	if (!mani_team_join_force_auto.GetBool() &&
		!mani_team_join_keep_same_team.GetBool()) return;
	if (!gpManiGameType->IsTeamPlayAllowed()) return;
	if (mani_team_join_keep_same_team.GetInt() == 0) return;
	if (player_ptr->is_bot) return;
	if (FStrEq(player_ptr->steam_id, "STEAM_ID_PENDING")) return;

	saved_team_t	*saved_team_record;

	if (!this->IsPlayerInTeamJoinList(player_ptr, &saved_team_record))
	{
		// Not found record so add it if an active team
		if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
		{
			// Not valid active team so ignore
			return;
		}

		saved_team_t saved_team;

		Q_strcpy(saved_team.steam_id, player_ptr->steam_id);
		saved_team.team_id = player_ptr->team;

		AddToList((void **) &saved_team_list, sizeof(saved_team_t), &saved_team_list_size);
		saved_team_list[saved_team_list_size - 1] = saved_team;

		qsort(saved_team_list, saved_team_list_size, sizeof(saved_team_t), sort_saved_team_by_steam_id); 
		return;
	}

	// Not found record so add it if an active team
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
	{
		// Not valid active team so ignore
		return;
	}

	saved_team_record->team_id = player_ptr->team;
}

//---------------------------------------------------------------------------------
// Purpose: A client jointeam client command was intercepted
//---------------------------------------------------------------------------------
PLUGIN_RESULT ManiTeamJoin::PlayerJoin(edict_t *pEntity, char *team_id)
{
	if (war_mode) return PLUGIN_CONTINUE; 
	if (!mani_team_join_force_auto.GetBool() &&
		!mani_team_join_keep_same_team.GetBool()) return PLUGIN_CONTINUE;
	if (!gpManiGameType->IsTeamPlayAllowed()) return PLUGIN_CONTINUE;

	int team_number = atoi(team_id);
	player_t player;
	player.entity = pEntity;
	if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
	if (player.is_bot) return PLUGIN_CONTINUE;

	if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AUTOJOIN))
	{
		return PLUGIN_CONTINUE;
	}

	if (mani_team_join_force_auto.GetBool() || 
		FStrEq(player.steam_id,"STEAM_ID_PENDING"))
	{
		// We only care about player using auto or spectator
		if (!gpManiGameType->IsValidActiveTeam(team_number))
		{
			return PLUGIN_CONTINUE;
		}

		// Player is trying to join an active team so force auto
		SayToPlayer(LIGHT_GREEN_CHAT, &player, "You must choose Auto-Assign");
		CSayToPlayer(&player, "You must choose Auto-Assign");
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
		engine->ClientCommand(player.entity, "chooseteam");
		return PLUGIN_STOP;
	}

	saved_team_t	*saved_team_record;

	// Find stored team id record for this player
	if (!this->IsPlayerInTeamJoinList(&player, &saved_team_record))
	{
		// Not found so just auto them

		// We only care about player using auto or spectator
		if (!gpManiGameType->IsValidActiveTeam(team_number))
		{
			return PLUGIN_CONTINUE;
		}

		// Player is trying to join an active team so force auto
		SayToPlayer(LIGHT_GREEN_CHAT, &player, "You must choose Auto-Assign");
		CSayToPlayer(&player, "You must choose Auto-Assign");
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
		engine->ClientCommand(player.entity, "chooseteam");
		return PLUGIN_STOP;
	}

	// Found stored team id for this player, check if trying to join
	// an active team or not
	if (team_number != 0 && !gpManiGameType->IsValidActiveTeam(team_number))
	{
		return PLUGIN_CONTINUE;
	}

	if (saved_team_record->team_id != team_number)
	{
		player.player_info->ChangeTeam(saved_team_record->team_id);
		SayToPlayer(LIGHT_GREEN_CHAT, &player, "Auto-forced to same team as before!");
		CSayToPlayer(&player, "Auto-forced to same team as before!");
		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_RESTRICTWEAPON);
		return PLUGIN_STOP;
	}

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: IsPlayerInTeamJoinList
//---------------------------------------------------------------------------------
bool ManiTeamJoin::IsPlayerInTeamJoinList(player_t *player_ptr, saved_team_t **saved_team_record)
{

	saved_team_t	saved_team_key;

	// Do BSearch for steam ID
	Q_strcpy(saved_team_key.steam_id, player_ptr->steam_id);

	*saved_team_record = (saved_team_t *) bsearch
						(
						&saved_team_key, 
						saved_team_list, 
						saved_team_list_size, 
						sizeof(saved_team_t), 
						sort_saved_team_by_steam_id
						);

	if (*saved_team_record == NULL)
	{
		return false;
	}
	
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Clear down the scores
//---------------------------------------------------------------------------------
void ManiTeamJoin::ResetTeamList(void)
{
	FreeList((void **) &saved_team_list, &saved_team_list_size);
}

static int sort_saved_team_by_steam_id ( const void *m1,  const void *m2) 
{
	struct saved_team_t *mi1 = (struct saved_team_t *) m1;
	struct saved_team_t *mi2 = (struct saved_team_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

ManiTeamJoin	g_ManiTeamJoin;
ManiTeamJoin	*gpManiTeamJoin;
