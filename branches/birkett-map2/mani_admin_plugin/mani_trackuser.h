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



#ifndef MANI_TRACK_USER_H
#define MANI_TRACK_USER_H

class ManiTrackUser
{

public:
	ManiTrackUser();
	~ManiTrackUser();

	void		ClientActive(edict_t *pEntity);
	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	int			GetIndex(int user_id) 
	{ 
		return (int) hash_table[user_id]; 
	}

private:

	// Hash table of user ids
	char hash_table[65536];
};

extern	ManiTrackUser *gpManiTrackUser;

#endif
