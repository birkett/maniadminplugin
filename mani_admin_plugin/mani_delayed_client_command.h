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

#ifndef MANI_DELAYED_CLIENT_COMMAND_H
#define MANI_DELAYED_CLIENT_COMMAND_H


#include <string.h>
#include "utlvector.h"
#include "edict.h"

struct client_command_t {

	client_command_t() {
		entity = NULL;
		command_time = 0.0f;
		memset(command, 0, sizeof(command));
	}

	edict_t *entity;
	float command_time;
	char command[1024];
};
extern CUtlVector<client_command_t> delayed_client_command_list;

class ManiDelayedClientCommand {
public:
	void Init();
	void AddPlayer ( edict_t *entity, float command_time = 0.0f, const char *command = NULL );
	void GameFrame ();
	void PlayerCommand ( edict_t *entity, const char *command = NULL );
};

extern ManiDelayedClientCommand *gpManiDelayedClientCommand;

#endif // MANI_DELAYED_CLIENT_COMMAND_H
