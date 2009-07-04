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



#ifndef MANI_ANTI_REJOIN_H
#define MANI_ANTI_REJOIN_H

#include "mani_player.h"
#include <map>

class ManiAntiRejoin
{
public:
	ManiAntiRejoin();
	~ManiAntiRejoin();

	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void) {ResetList();}
	void		Unload(void) {ResetList();}
	void		LevelInit(void) {ResetList();}
	void		PlayerSpawn(player_t *player_ptr);

private:

	void		ResetList() {rejoin_list.clear();}
	std::map <BasicStr, int> rejoin_list;	
};

extern	ManiAntiRejoin *gpManiAntiRejoin;

#endif
