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



#ifndef MANI_AUTORR_H
#define MANI_AUTORR_H

class ManiWarmupTimer
{

public:
	ManiWarmupTimer();
	~ManiWarmupTimer();

	void		LevelInit(void);
	void		GameFrame(void);
	bool		KnivesOnly(void);
	bool		IgnoreTK(void);
	bool		UnlimitedHE(void);
	void		PlayerSpawn(player_t *player_ptr);
	void		RoundStart(void);
	void		PlayerDeath(player_t *player_ptr);
	PLUGIN_RESULT		JoinClass(edict_t	*pEdict);
	inline		bool InWarmupRound(void) {return check_timer;}
	void		SetRandomItem(ConVar *cvar_ptr, int item_number);

	struct		item_t
	{
		char	item_name[80];
	};

private:

	struct		respawn_t
	{
		bool	needs_respawn;
		float	time_to_respawn;
	};

	char		item_name[5][80];

	void		GiveItem(edict_t *pEntity, const char	*item_name);
	void		GiveAllAmmo(void);
	bool		check_timer;
	bool		fire_restart;
	float		next_check;
	respawn_t	respawn_list[MANI_MAX_PLAYERS];
	bool		friendly_fire;

};

extern	ManiWarmupTimer *gpManiWarmupTimer;

#endif
