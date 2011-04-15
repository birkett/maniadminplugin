//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_VOICE_H
#define MANI_VOICE_H

extern	bool	ProcessDeadAllTalk
(
 int	receiver_index,
 int	sender_index,
 bool	*new_listen
);

extern bool ProcessMuteTalk
(
 int	receiver_index,
 int	sender_index,
 bool	*new_listen
);

extern ConVar mani_dead_alltalk;

#endif
