//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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

#include "mani_delayed_client_command.h"
#include "utlvector.h"
#include "inetchannel.h"
#include "iclient.h"
#include "edict.h"
#include "iplayerinfo.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "mani_player.h"

CUtlVector<client_command_t> delayed_client_command_list;

extern	CGlobalVars *gpGlobals;
extern  int max_players;
extern  IVEngineServer *engine;
extern	IServerPluginHelpers *helpers;

#if defined ( WIN32 )
	#define snprintf _snprintf
#endif

void ManiDelayedClientCommand::GameFrame() {
	if ( delayed_client_command_list.Count() == 0 )
		return;

	for ( int i = 0; i < delayed_client_command_list.Count(); i++ ) {
		if ( gpGlobals->curtime > delayed_client_command_list.Element(i).command_time ) {
			PlayerCommand ( delayed_client_command_list.Element(i).entity, delayed_client_command_list.Element(i).command);
			delayed_client_command_list.Remove(i);
		}
	}
}

void ManiDelayedClientCommand::Init() {
	delayed_client_command_list.RemoveAll();
	delayed_client_command_list.EnsureCapacity(max_players);
}

void ManiDelayedClientCommand::PlayerCommand ( edict_t *entity, const char *command ) {
	player_t player;
	player.entity = entity;
	if ( !FindPlayerByEntity(&player) ) return;

	if ( player.is_bot ) return;

		helpers->ClientCommand(entity, command);
}

void ManiDelayedClientCommand::AddPlayer(edict_t *entity, float command_time, const char *command) {
	client_command_t temp;
	temp.command_time = gpGlobals->curtime+command_time;
	temp.entity = entity;
	if ( command )
		strncpy ( temp.command, command, sizeof(temp.command) );
	delayed_client_command_list.AddToTail(temp);
}

ManiDelayedClientCommand g_ManiDelayedClientCommand;
ManiDelayedClientCommand *gpManiDelayedClientCommand = &g_ManiDelayedClientCommand;
