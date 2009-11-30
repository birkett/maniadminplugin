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
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_gametype.h"
#include "mani_effects.h"
#include "mani_voice.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_alltalk;
extern int mute_list_size;
extern ban_settings_t *mute_list;
static	bool IsPlayerValid(player_t *player_ptr);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

CONVAR_CALLBACK_PROTO(ManiDeadAllTalk);

ConVar mani_dead_alltalk ("mani_dead_alltalk", "0", 0, "0 = Dont let dead players from each team talk, 1 = Let dead players from each team talk", true, 0, true, 1, CONVAR_CALLBACK_REF(ManiDeadAllTalk)); 


//---------------------------------------------------------------------------------
// Purpose: Process if the player can talk when dead or not
//---------------------------------------------------------------------------------
bool	ProcessDeadAllTalk
(
 int	receiver_index,
 int	sender_index,
 bool	*new_listen
)
{
//	MMsg("Receiver [%i] Sender [%i]\n", receiver_index, sender_index);
	if (!voiceserver) return false;
	if (war_mode) return false;

	if (!gpManiGameType->IsTeamPlayAllowed()) return false;
	if (mani_dead_alltalk.GetInt() == 0) return false;
	if (sv_alltalk && sv_alltalk->GetInt() == 1) return false;

	player_t player_receiver;
	player_t player_sender;

	player_sender.index = sender_index;
	player_receiver.index = receiver_index;

	if (!IsPlayerValid(&player_sender)) return false;
	if (!IsPlayerValid(&player_receiver)) return false;

	// Mute player output
	time_t now;
	time (&now);
	int j = -1;
	if ( mute_list_size != 0) {
		j = 0;
		for (;;) {
			if ( mute_list[j].byID ) {
				if ( FStrEq ( mute_list[j].key_id, player_sender.steam_id ) )
					break;
			} else {
				if ( FStrEq ( mute_list[j].key_id, player_sender.ip_address ) )
					break;
			}
			j++;
			if ( j > (mute_list_size - 1) ) {
				j=-1; //not muted
				break;
			}
		}
	}

	if ((j >= 0) && punish_mode_list[player_sender.index - 1].muted) {
		if ((mute_list[j].expire_time <= now) && (mute_list[j].expire_time != 0)) 
			punish_mode_list[player_sender.index - 1].muted = 0;
	}

	if (punish_mode_list[sender_index - 1].muted != 0)
	{
		*new_listen = false;
		return true;
	}


	if (gpManiGameType->IsSpectatorAllowed())
	{
		if (player_sender.team == gpManiGameType->GetSpectatorIndex()) return false;
		if (player_receiver.team == gpManiGameType->GetSpectatorIndex()) return false;
	}

	if (gpManiGameType->IsValidActiveTeam(player_sender.team) && gpManiGameType->IsValidActiveTeam(player_receiver.team))
	{
		if (player_sender.team != player_receiver.team)
		{
			*new_listen = true;
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Faster version of FindPlayerByIndex
//---------------------------------------------------------------------------------

static
bool IsPlayerValid(player_t *player_ptr)
{

	if (player_ptr->index < 1 || player_ptr->index > max_players)
	{
		return false;
	}

	edict_t *pEntity = engine->PEntityOfEntIndex(player_ptr->index);
	if(pEntity && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			if (playerinfo->IsHLTV()) return false;
			if (FStrEq(playerinfo->GetNetworkIDString(),"BOT")) return false;
			player_ptr->team = playerinfo->GetTeamIndex();
			if (playerinfo->IsDead()) return true;
			return false;
		}
	}

	return false;
}

CONVAR_CALLBACK_FN(ManiDeadAllTalk)
{
	if (!FStrEq(pOldString, mani_dead_alltalk.GetString()))
	{
		if (atoi(mani_dead_alltalk.GetString()) == 0)
		{
			SayToAll(ORANGE_CHAT, true, "DeadAllTalk mode off");
		}
		else
		{
			SayToAll(ORANGE_CHAT, true, "DeadAllTalk mode on");
		}
	}
}
