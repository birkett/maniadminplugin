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



#ifndef MANI_EFFECTS_H
#define MANI_EFFECTS_H

#define	MANI_TK_ENFORCED (1)
#define	MANI_ADMIN_ENFORCED (2)
#define DRUG_STEPS (20)
#define DRUG_STEP_TIME (1.5)

#include "mani_sounds.h"

// Slay sound
extern	char *slay_sound_name;
extern	char *hl2mp_slay_sound_name;

extern	char *countdown_beep;
extern	char *final_beep;
extern	char *beacon_sound;

struct effect_sound_t
{
	char	sound_name[256];
};

// Slap sounds
extern effect_sound_t slap_sound_name[];
extern effect_sound_t hl2mp_slap_sound_name[];

struct punish_mode_t
{
	int		drugged; // 0 = none, 1 = TK ENFORCED, 2 = ADMIN ENFORCED
	float	next_drug_update_time;
	int		muted;
	int		frozen;
	float	next_frozen_update_time;
	int		no_clip;
	int		time_bomb;
	float	next_time_bomb_update_time;
	int		time_bomb_beeps_remaining;
	int		fire_bomb;
	float	next_fire_bomb_update_time;
	int		fire_bomb_beeps_remaining;
	int		freeze_bomb;
	float	next_freeze_bomb_update_time;
	int		freeze_bomb_beeps_remaining;
	int		beacon;
	float	next_beacon_update_time;
	int		flame_index;
};

extern	punish_mode_t	punish_mode_list[];

extern	void	InitEffects(void);
extern	void	EffectsClientDisconnect(int	index, bool spawn_flag);
extern	void	EffectsRoundStart(void);
extern	void	EffectsPlayerDeath(player_t *player_ptr);
extern	void	ProcessInGamePunishments(void);
extern	void	BlindPlayer( player_t *player, int blind_amount);
extern	void	SlayPlayer(player_t *player_ptr, bool kill_as_suicide, bool use_beam, bool	use_explosion);
extern	void	ProcessSlapPlayer(player_t *player, int damage, bool slap_angle);
extern	void	ProcessBurnPlayer( player_t *player,  int burn_time);
extern	void	ProcessNoClipPlayer( player_t *player);
extern	void	ProcessFreezePlayer(player_t *player_ptr, bool admin_called);
extern	void	ProcessUnFreezePlayer(player_t *player_ptr);
extern	void	ProcessDrugPlayer(player_t *player, bool admin_called);
extern	void	ProcessUnDrugPlayer(player_t *player);
extern	void	ProcessMutePlayer(player_t *player, player_t *giver, int timetomute, bool byID, const char *reason = NULL);
extern	void	ProcessUnMutePlayer(player_t *player);
extern	void	ProcessTeleportPlayer(player_t *player_to_teleport, Vector *origin_ptr);
extern	void	ProcessSaveLocation(player_t *player);
extern	void	ProcessTakeCash(player_t *donator, player_t *receiver);
extern	void	ProcessTimeBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called);
extern	void	ProcessUnTimeBombPlayer(player_t *player_ptr);
extern	void	ProcessFireBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called);
extern	void	ProcessUnFireBombPlayer(player_t *player_ptr);
extern	void	ProcessFreezeBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called);
extern	void	ProcessUnFreezeBombPlayer(player_t *player_ptr);
extern	void	ProcessBeaconPlayer(player_t *player_ptr, bool admin_called);
extern	void	ProcessUnBeaconPlayer(player_t *player_ptr);
extern	void	ProcessDeathBeam(player_t *attacker_ptr, player_t *victim_ptr);
extern	void	ProcessSetColour(edict_t *pEntity, int r, int g, int b, int a);
extern	void	ProcessSetWeaponColour(CBaseEntity *pCBaseEntity, int r, int g, int b, int a);
extern  void	MANI_TraceLineWorldProps( const Vector& vecAbsStart,  const Vector& vecAbsEnd,  unsigned int mask,  int collisionGroup, trace_t *ptr);

#endif
