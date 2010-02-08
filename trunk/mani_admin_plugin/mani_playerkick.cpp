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

#include "mani_playerkick.h"
#include "utlvector.h"
#include "inetchannel.h"
#include "iclient.h"
#include "edict.h"
#include "iplayerinfo.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "mani_player.h"

CUtlVector<player_kick_t> kick_list;

extern	CGlobalVars *gpGlobals;
extern  int max_players;
extern  IVEngineServer *engine;

#if defined ( WIN32 )
	#define snprintf _snprintf
#endif

void ManiPlayerKick::GameFrame() {
	if ( kick_list.Count() == 0 )
		return;

	for ( int i = 0; i < kick_list.Count(); i++ ) {
		if ( gpGlobals->curtime > kick_list.Element(i).kick_time ) {
			KickPlayer ( kick_list.Element(i).player_index, kick_list.Element(i).reason );
			kick_list.Remove(i);
		}
	}
}

void ManiPlayerKick::Init() {
	kick_list.RemoveAll();
	kick_list.EnsureCapacity(max_players);
}

void ManiPlayerKick::KickPlayer ( int player_index, const char *reason ) {
	player_t player;
	player.index = player_index;
	if ( !FindPlayerByIndex(&player) ) return;

	if ( player.is_bot ) {
		char kick_cmd[512];
		snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i\n", player.user_id);
		engine->ServerCommand(kick_cmd);
		engine->ServerExecute();
	} else {
		INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(player_index));
		IClient *pClient = static_cast<IClient *>(pNetChan->GetMsgHandler());
		if ( reason[0] == 0 )
			pClient->Disconnect("Kicked by Console");
		else
			pClient->Disconnect("%s", reason);
	}
}

void ManiPlayerKick::AddPlayer(int player_index, float kick_time, const char *reason) {
	player_kick_t temp;
	temp.kick_time = gpGlobals->curtime+kick_time;
	temp.player_index = player_index;
	if ( reason )
		strncpy ( temp.reason, reason, sizeof(temp.reason) );
	kick_list.AddToTail(temp);
}

ManiPlayerKick g_ManiPlayerKick;
ManiPlayerKick *gpManiPlayerKick = &g_ManiPlayerKick;