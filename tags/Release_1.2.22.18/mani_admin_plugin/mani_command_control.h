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

#ifndef MANI_COMMAND_CONTROL_H
#define MANI_COMMAND_CONTROL_H

#include <vector>
#include <algorithm>
#include "engine/iserverplugin.h"

struct command_record_t {
	int	index;
	std::vector<float> times;
	int violation_count;
};

class CCommandControl
{
public:
	CCommandControl();
	~CCommandControl();

	bool			ClientCommand ( player_t *player_ptr );
	void			ClientDisconnect ( player_t *player_ptr );
	void			ClientActive	( player_t *player_ptr );
private:
	void CommandsIssuedOverTime ( int player_index, int duration );

	command_record_t player_command_times[64];
};

extern CCommandControl g_command_control;
#endif // MANI_COMMAND_CONTROL_H
