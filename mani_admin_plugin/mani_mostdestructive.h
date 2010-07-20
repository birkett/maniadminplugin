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



#ifndef MANI_MOSTDESTUCTIVE_H
#define MANI_MOSTDESTUCTIVE_H

class ManiMostDestructive
{

public:
	ManiMostDestructive();
	~ManiMostDestructive();

	void		PlayerDeath( player_t *victim_ptr,  player_t *attacker_ptr,  bool attacker_exists);
	void		PlayerHurt( player_t *victim_ptr,  player_t *attacker_ptr,  IGameEvent * event);
	void		RoundStart(void);
	void		RoundEnd(void);

private:

	struct destructive_t
	{
		int	damage_done;
		int kills;
		char name[MAX_PLAYER_NAME_LENGTH];
	};

	destructive_t		destruction_list[MANI_MAX_PLAYERS];

};

extern	ManiMostDestructive *gpManiMostDestructive;

#endif
