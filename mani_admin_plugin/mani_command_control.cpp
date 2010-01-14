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

#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mani_mainclass.h"
#include "mani_commands.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_command_control.h"

// needed by Keeper
extern	CGlobalVars *gpGlobals;
extern	int	max_players;

extern ConVar mani_command_flood_time;
extern ConVar mani_command_flood_total;
//birkett - added chat spamming punishment
extern ConVar mani_command_flood_punish;
extern ConVar mani_command_flood_violation_count;
extern ConVar mani_command_flood_punish_ban_time;
extern IVEngineServer* engine;

CCommandControl::CCommandControl () {
	for ( int i = 0; i < max_players; i++ ) {
		player_command_times[i].index = i + 1;
		player_command_times[i].times.clear();
	}
}

CCommandControl::~CCommandControl() {
	for ( int i = 0; i < max_players; i++ ) {
		player_command_times[i].index = i + 1;
		player_command_times[i].times.clear();
	}
}

void CCommandControl::CommandsIssuedOverTime( int player_index, int duration ) {
	float now = gpGlobals->curtime;
	bool changes = true;
	
	while ( changes ) {
		changes = false;
		for ( std::vector<float>::iterator i = player_command_times[player_index].times.begin();
			i != player_command_times[player_index].times.end();
			++i ) {
				if ( ( *i + duration ) < now ) {
					player_command_times[player_index].times.erase(i);
					changes = true;
					break;
				}
		}
	}
}

bool CCommandControl::ClientCommand(player_t *player_ptr) {
	if ( !mani_command_flood_time.GetBool() )
		return true;

	int time_to_check = mani_command_flood_time.GetInt();

	int player_index = player_ptr->index - 1;
	if ( player_index < 0 || player_index >= max_players )
		return false;

	player_command_times[player_index].times.push_back(gpGlobals->curtime);
	CommandsIssuedOverTime ( player_index, time_to_check );

	if ( (int) (player_command_times[player_index].times.size() - 1)  >= mani_command_flood_total.GetInt() ) {
		//add one to the players violation count
		player_command_times[player_index].violation_count += 1;

		if( mani_command_flood_punish.GetInt() == 1 )
		{
			//kick punishment
			if ( player_command_times[player_index].violation_count >= mani_command_flood_violation_count.GetInt() )
			{	
				//kick the player
				char kick_cmd[256];
				snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i %s\n", player_ptr->user_id, "Kicked due to command spam");
				engine->ServerCommand(kick_cmd);
				engine->ServerExecute();
			}
		}
		else if ( mani_command_flood_punish.GetInt() == 2 )
		{
			//ban punishment
			if ( player_command_times[player_index].violation_count >= mani_command_flood_violation_count.GetInt() )
			{	
				//ban the player
				LogCommand (NULL,"Ban (Command Spam) [%s] [%s]", player_ptr->name, player_ptr->steam_id);
				gpManiAdminPlugin->AddBan(player_ptr, player_ptr->steam_id, "MAP", 
					mani_command_flood_punish_ban_time.GetInt(), "Banned (Command spam)", "Banned (Command spam)");
				gpManiAdminPlugin->WriteBans();
			}
		}
		return false;
	}

	return true;
}

void CCommandControl::ClientActive ( player_t *player_ptr ) {
	int player_index = player_ptr->index - 1;
	if ( player_index < 0 || player_index >= max_players )
		return;

	player_command_times[player_index].times.clear();
	player_command_times[player_index].violation_count = 0;
}

void CCommandControl::ClientDisconnect(player_t *player_ptr) {
	int player_index = player_ptr->index - 1;
	if ( player_index < 0 || player_index >= max_players )
		return;

	player_command_times[player_index].times.clear();
	player_command_times[player_index].violation_count = 0;
}
CCommandControl g_command_control;