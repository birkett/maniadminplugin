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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h"
#if defined ( GAME_CSGO )
#include <cstrike15_usermessage_helpers.h>
#else 
#include "bitbuf.h"
#endif
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_mainclass.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_sounds.h"
#include "mani_maps.h"
#include "mani_gametype.h"
#include "mani_vars.h"
#include "mani_effects.h"
#include "mani_vfuncs.h"
#include "cbaseentity.h"
#include "beam_flags.h"
#include "mani_delayed_client_command.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;
extern	IEffects *effects;
extern	IEngineSound *esounds;
extern	IServerPluginHelpers *helpers;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IServerGameEnts	*serverents;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;
#if !defined ( GAME_CSGO )
extern	bf_write *msg_buffer;
#endif
extern	int	fade_message_index;
extern	int	text_message_index;
// Sprite indexes;
extern	int	tp_beam_index;
extern	int	plasmabeam_index;
extern	int	lgtning_index;
extern	int	explosion_index;
extern	int	orangelight_index;
extern	int	bluelight_index;
extern	int	purplelaser_index;

extern	ConVar	*mp_freezetime;

// Slay sound
char *slay_sound_name="weapons/hegrenade/explode3.wav";
char *hl2mp_slay_sound_name="weapons/explode3.wav";

// Time Bomb sounds
char *countdown_beep="buttons/button17.wav";
char *final_beep="weapons/cguard/charging.wav";
char *beacon_sound="buttons/blip1.wav";

punish_mode_t	punish_mode_list[MANI_MAX_PLAYERS];
static	float	drugged_z_angle[DRUG_STEPS];

struct time_bomb_t
{
	int		index;
	edict_t	*entity_ptr;
};

static	time_bomb_t	*time_bomb_list = NULL;
static	int	time_bomb_list_size = 0;

effect_sound_t slap_sound_name[3];
effect_sound_t hl2mp_slap_sound_name[3];

struct punish_global_t
{
	bool	drugged;
	bool	frozen;
	bool	time_bomb;
	bool	fire_bomb;
	bool	freeze_bomb;
	bool	beacon;
};

static	punish_global_t	punish_global;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

static	void	ProcessBombCount(player_t *bomb_target_ptr,int mode,const char	*fmt, ...);
static	void	CheckDruggedGlobal(void);
static	void	CheckFrozenGlobal(void);
static	void	CheckTimeBombGlobal(void);
static	void	CheckFireBombGlobal(void);
static	void	CheckFreezeBombGlobal(void);
static	void	CheckBeaconGlobal(void);
static	void	CheckAllGlobal(void);
static	void	ProcessDruggedFrame(void);
static	void	ProcessFrozenFrame(void);
static	void	ProcessTimeBombFrame(void);
static	void	ProcessFireBombFrame(void);
static	void	ProcessFreezeBombFrame(void);
static	void	ProcessBeaconFrame(void);

//---------------------------------------------------------------------------------
// Purpose: Initialise the effects list
//---------------------------------------------------------------------------------
void	InitEffects(void)
{

	Q_strcpy(slap_sound_name[0].sound_name,"player/damage1.wav");
	Q_strcpy(slap_sound_name[1].sound_name,"player/damage2.wav");
	Q_strcpy(slap_sound_name[2].sound_name,"player/damage3.wav");

	Q_strcpy(hl2mp_slap_sound_name[0].sound_name,"player/pl_fallpain1.wav");
	Q_strcpy(hl2mp_slap_sound_name[1].sound_name,"player/pl_fallpain3.wav");
	Q_strcpy(hl2mp_slap_sound_name[2].sound_name,"player/pl_pain5.wav");

	// Reset the lists
	for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
	{
		EffectsClientDisconnect(i, false);
	}

	// Setup drugged z angles
	float radian_angle = 3.141592653589 / 180.0;
	float angle_step = 360.0 / DRUG_STEPS;

	for (int i = 0; i < DRUG_STEPS; i ++)
	{
		drugged_z_angle[i] = sin(i * (angle_step * radian_angle)) * 48;
	}

}

//---------------------------------------------------------------------------------
// Purpose: Initialise the effects list
//---------------------------------------------------------------------------------
void	EffectsClientDisconnect(int	index, bool spawn_flag)
{
	punish_mode_list[index].drugged = 0;
	punish_mode_list[index].frozen = 0;
	punish_mode_list[index].next_drug_update_time = -999;
	punish_mode_list[index].next_frozen_update_time = -999;

	if (!spawn_flag)
	{
		punish_mode_list[index].muted = 0;
	}

	punish_mode_list[index].no_clip = 0;
	punish_mode_list[index].time_bomb = 0;
	punish_mode_list[index].fire_bomb = 0;
	punish_mode_list[index].freeze_bomb = 0;
	punish_mode_list[index].beacon = 0;
	punish_mode_list[index].next_time_bomb_update_time = -999;
	punish_mode_list[index].next_fire_bomb_update_time = -999;
	punish_mode_list[index].next_freeze_bomb_update_time = -999;
	punish_mode_list[index].next_beacon_update_time = -999;
	punish_mode_list[index].time_bomb_beeps_remaining = mani_tk_time_bomb_seconds.GetInt();
	punish_mode_list[index].fire_bomb_beeps_remaining = mani_tk_fire_bomb_seconds.GetInt();
	punish_mode_list[index].freeze_bomb_beeps_remaining = mani_tk_freeze_bomb_seconds.GetInt();
	punish_mode_list[index].flame_index = 0;
	CheckAllGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Initialise the effects list
//---------------------------------------------------------------------------------
void	EffectsRoundStart(void)
{
	float freeze_time = 0;

	if (mp_freezetime)
	{
		freeze_time = mp_freezetime->GetFloat();
	}

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		punish_mode_list[i].next_time_bomb_update_time = gpGlobals->curtime + freeze_time;
		punish_mode_list[i].time_bomb_beeps_remaining = mani_tk_time_bomb_seconds.GetInt();
		punish_mode_list[i].next_fire_bomb_update_time = gpGlobals->curtime + freeze_time;
		punish_mode_list[i].fire_bomb_beeps_remaining = mani_tk_fire_bomb_seconds.GetInt();
		punish_mode_list[i].next_freeze_bomb_update_time = gpGlobals->curtime + freeze_time;
		punish_mode_list[i].freeze_bomb_beeps_remaining = mani_tk_freeze_bomb_seconds.GetInt();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Initialise the effects list
//---------------------------------------------------------------------------------
void	EffectsPlayerDeath(player_t *player_ptr)
{
	int	index = player_ptr->index - 1;

	if (punish_mode_list[index].drugged)
	{
		// Undrug player if drugged at time of death
		ProcessUnDrugPlayer(player_ptr);
	}

	if (punish_mode_list[index].time_bomb)
	{
		punish_mode_list[index].time_bomb = 0;
		punish_mode_list[index].next_time_bomb_update_time = -999;
		CheckTimeBombGlobal();
	}

	if (punish_mode_list[index].fire_bomb)
	{
		punish_mode_list[index].fire_bomb = 0;
		punish_mode_list[index].next_fire_bomb_update_time = -999;
		CheckFireBombGlobal();
	}

	if (punish_mode_list[index].freeze_bomb)
	{
		punish_mode_list[index].freeze_bomb = 0;
		punish_mode_list[index].next_freeze_bomb_update_time = -999;
		CheckFreezeBombGlobal();
	}

	if (punish_mode_list[index].frozen)
	{
		punish_mode_list[index].frozen = 0;
		punish_mode_list[index].next_frozen_update_time = -999;
		CheckFrozenGlobal();
	}

	if (punish_mode_list[index].beacon)
	{
		punish_mode_list[index].beacon = 0;
		punish_mode_list[index].next_beacon_update_time = -999;
		CheckBeaconGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in game punishments
//---------------------------------------------------------------------------------
void ProcessInGamePunishments(void)
{
	ProcessDruggedFrame();
	ProcessFrozenFrame();
	ProcessTimeBombFrame();
	ProcessFireBombFrame();
	ProcessFreezeBombFrame();
	ProcessBeaconFrame();
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Drugged users
//---------------------------------------------------------------------------------
static
void ProcessDruggedFrame(void)
{
	// Update any drugged users
	if (punish_global.drugged)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].drugged == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_drug_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].drugged = 0;
				continue;
			}
			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].drugged = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].drugged = 0;
				continue;
			}

			CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
			if (m_pCBaseEntity)
			{
				QAngle angle = CBaseEntity_EyeAngles(m_pCBaseEntity);
				//QAngle angle = m_pCBaseEntity->EyeAngles();
				angle.z = drugged_z_angle[rand() % DRUG_STEPS];
				CBaseEntity_Teleport(m_pCBaseEntity, NULL, &angle, NULL);
			}

			punish_mode_list[i].next_drug_update_time = gpGlobals->curtime + DRUG_STEP_TIME;
		}
		CheckDruggedGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Frozen users
//---------------------------------------------------------------------------------
static
void ProcessFrozenFrame(void)
{
	// Update any frozen users
	if (punish_global.frozen)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].frozen == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_frozen_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].frozen = 0;
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].frozen = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].frozen = 0;
				continue;
			}

			Prop_SetVal(player.entity, MANI_PROP_MOVE_TYPE, MOVETYPE_NONE); 

			if (effects)
			{
				// Get player location, so we know where to play the sound.
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();
				Vector end = pos;

				end.z += 500;

				/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
				int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
				float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
				unsigned char noise, unsigned char red, unsigned char green,
				unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/


				effects->Beam(pos, end, plasmabeam_index, 0, 0, 10, 1.55, 50, 0, 0, 0, 255, 255, 255, 255, 32);
				punish_mode_list[i].next_frozen_update_time = gpGlobals->curtime + 1.5;
			}
		}

		CheckFrozenGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Time Bombed users
//---------------------------------------------------------------------------------
static
void ProcessTimeBombFrame(void)
{
	// Update any time bombed users
	if (punish_global.time_bomb)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].time_bomb == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_time_bomb_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].time_bomb = 0;
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].time_bomb = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].time_bomb = 0;
				continue;
			}

			ProcessBombCount(&player, mani_tk_time_bomb_blast_mode.GetInt(), "%i", punish_mode_list[i].time_bomb_beeps_remaining);

			if (punish_mode_list[i].time_bomb_beeps_remaining > 1)
			{

				// Just do normal beep
				int	next_colour = ((255.0 / mani_tk_time_bomb_seconds.GetFloat()) * (float) punish_mode_list[i].time_bomb_beeps_remaining);
				if (next_colour > 255) next_colour = 255;

				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )				
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, countdown_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, countdown_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				//CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
				ProcessSetColour(player.entity, 255, next_colour, next_colour, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_time_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_time_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											255,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											20,20,20,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

				}
				punish_mode_list[i].next_time_bomb_update_time = gpGlobals->curtime + 1.0;
				punish_mode_list[i].time_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].time_bomb_beeps_remaining == 1)
			{
				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, final_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, final_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				//CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity();
				ProcessSetColour(player.entity, 255, 0, 0, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_time_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_time_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											255,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											20,20,20,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

				}

				punish_mode_list[i].next_time_bomb_update_time = gpGlobals->curtime + 0.7;
				punish_mode_list[i].time_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].time_bomb_beeps_remaining == 0)
			{
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);

				// Blow everyone up
				if (mani_tk_time_bomb_blast_mode.GetInt() > 0)
				{
					for (int j = 1; j <= max_players; j++)
					{
						player_t	blast_player;

						// Don't include current player
						if (j == i + 1) continue;

						blast_player.index = j;

						if (!FindPlayerByIndex(&blast_player)) continue;
						if (blast_player.is_dead) continue;

						// Not on same team
						if (mani_tk_time_bomb_blast_mode.GetInt() == 1)
						{
							if (blast_player.team != player.team) continue;
						}
						else
						{
							if (!gpManiGameType->IsValidActiveTeam(blast_player.team)) continue;
						}

						Vector	v = blast_player.player_info->GetAbsOrigin() - player.player_info->GetAbsOrigin();

						if (v.Length() < mani_tk_time_bomb_blast_radius.GetInt())
						{
							// Add this player to the list
							AddToList((void **) &time_bomb_list, sizeof(time_bomb_t), &time_bomb_list_size);
							time_bomb_list[time_bomb_list_size - 1].index = j;
							time_bomb_list[time_bomb_list_size - 1].entity_ptr = blast_player.entity;
						}
					}

					Vector source_pos = player.entity->GetCollideable()->GetCollisionOrigin();
					source_pos.z += 50;

					if (time_bomb_list != 0)
					{
						if (mani_tk_time_bomb_show_beams.GetInt() > 0 && effects)
						{
							// Show the beams :)
							/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
							int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
							float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
							unsigned char noise, unsigned char red, unsigned char green,
							unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/

							for (int j = 0; j < time_bomb_list_size; j ++)
							{
								Vector victim_pos = time_bomb_list[j].entity_ptr->GetCollideable()->GetCollisionOrigin();

								victim_pos.z += 50;
								effects->Beam(victim_pos, source_pos, orangelight_index, 0, 0, 5, 1.0, 10, 10, 1, 0, 255, 255, 255, 255, 64);
							}
						}

						for (int j = 0; j < time_bomb_list_size; j ++)
						{
							player_t blast_player;

							blast_player.index = time_bomb_list[j].index;

							if (!FindPlayerByIndex(&blast_player)) continue;

							SlayPlayer(&blast_player, false, false, false);
						}
					}
				}

				SlayPlayer(&player, true, true, true);
				punish_mode_list[i].time_bomb = 0;
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);
				continue;
			}
		}

		CheckTimeBombGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Fire Bombed users
//---------------------------------------------------------------------------------
static
void ProcessFireBombFrame(void)
{
	// Update any fire bombed users
	if (punish_global.fire_bomb)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].fire_bomb == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_fire_bomb_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].fire_bomb = 0;
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].fire_bomb = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].fire_bomb = 0;
				continue;
			}

			ProcessBombCount(&player, mani_tk_fire_bomb_blast_mode.GetInt(), "%i", punish_mode_list[i].fire_bomb_beeps_remaining);

			if (punish_mode_list[i].fire_bomb_beeps_remaining > 1)
			{
				// Just do normal beep
				int	next_colour = ((255.0 / mani_tk_fire_bomb_seconds.GetFloat()) * (float) punish_mode_list[i].fire_bomb_beeps_remaining);
				if (next_colour > 255) next_colour = 255;

				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, countdown_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, countdown_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				// CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
				ProcessSetColour(player.entity, next_colour, 255, next_colour, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_fire_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_fire_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											128,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											20,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

				}
				punish_mode_list[i].next_fire_bomb_update_time = gpGlobals->curtime + 1.0;
				punish_mode_list[i].fire_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].fire_bomb_beeps_remaining == 1)
			{
				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, final_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, final_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				//  *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
				ProcessSetColour(player.entity, 0, 255, 0, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_fire_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_fire_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											128,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											20,255,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags
				}

				punish_mode_list[i].next_fire_bomb_update_time = gpGlobals->curtime + 0.7;
				punish_mode_list[i].fire_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].fire_bomb_beeps_remaining == 0)
			{
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);

				// Set everyone on fire
				if (mani_tk_fire_bomb_blast_mode.GetInt() > 0)
				{
					for (int j = 1; j <= max_players; j++)
					{
						player_t	blast_player;

						// Don't include current player
						if (j == i + 1) continue;

						blast_player.index = j;

						if (!FindPlayerByIndex(&blast_player)) continue;
						if (blast_player.is_dead) continue;

						// Not on same team
						if (mani_tk_fire_bomb_blast_mode.GetInt() == 1)
						{
							if (blast_player.team != player.team) continue;
						}
						else
						{
							if (!gpManiGameType->IsValidActiveTeam(blast_player.team)) continue;
						}

						Vector	v = blast_player.player_info->GetAbsOrigin() - player.player_info->GetAbsOrigin();

						if (v.Length() < mani_tk_fire_bomb_blast_radius.GetInt())
						{
							// Add this player to the list
							AddToList((void **) &time_bomb_list, sizeof(time_bomb_t), &time_bomb_list_size);
							time_bomb_list[time_bomb_list_size - 1].index = j;
							time_bomb_list[time_bomb_list_size - 1].entity_ptr = blast_player.entity;
						}
					}

					Vector source_pos = player.entity->GetCollideable()->GetCollisionOrigin();
					source_pos.z += 50;

					if (time_bomb_list != 0)
					{
						if (mani_tk_fire_bomb_show_beams.GetInt() > 0 && effects)
						{
							// Show the beams :)
							/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
							int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
							float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
							unsigned char noise, unsigned char red, unsigned char green,
							unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/

							for (int j = 0; j < time_bomb_list_size; j ++)
							{
								Vector victim_pos = time_bomb_list[j].entity_ptr->GetCollideable()->GetCollisionOrigin();

								victim_pos.z += 50;
								effects->Beam(victim_pos, source_pos, orangelight_index, 0, 0, 5, 1.0, 10, 10, 1, 0, 128, 255, 255, 255, 64);
							}
						}

						for (int j = 0; j < time_bomb_list_size; j ++)
						{
							player_t blast_player;

							blast_player.index = time_bomb_list[j].index;

							if (!FindPlayerByIndex(&blast_player)) continue;

							ProcessBurnPlayer(&blast_player, mani_tk_fire_bomb_burn_time.GetInt());
						}
					}
				}


				ProcessBurnPlayer(&player, mani_tk_fire_bomb_burn_time.GetInt());
				punish_mode_list[i].fire_bomb = 0;
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);
				continue;
			}
		}

		CheckFireBombGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Freeze Bombed users
//---------------------------------------------------------------------------------
static
void ProcessFreezeBombFrame(void)
{
	// Update any freeze bombed users
	if (punish_global.freeze_bomb)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].freeze_bomb == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_freeze_bomb_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].freeze_bomb = 0;
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].freeze_bomb = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].freeze_bomb = 0;
				continue;
			}

			ProcessBombCount(&player, mani_tk_freeze_bomb_blast_mode.GetInt(), "%i", punish_mode_list[i].freeze_bomb_beeps_remaining);

			if (punish_mode_list[i].freeze_bomb_beeps_remaining > 1)
			{
				// Just do normal beep
				int	next_colour = ((255.0 / mani_tk_freeze_bomb_seconds.GetFloat()) * (float) punish_mode_list[i].freeze_bomb_beeps_remaining);
				if (next_colour > 255) next_colour = 255;

				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, countdown_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, countdown_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				// CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
				ProcessSetColour(player.entity, next_colour, next_colour, 255, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_freeze_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_freeze_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											255,128,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											255,20,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

				}
				punish_mode_list[i].next_freeze_bomb_update_time = gpGlobals->curtime + 1.0;
				punish_mode_list[i].freeze_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].freeze_bomb_beeps_remaining == 1)
			{
				// Just do extended beep
				Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

				if (esounds)
				{
					// Play the "final beep"  sound
					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, final_beep, 0.7,  0.5, 0, 0, 100, &pos);
#else
					esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, final_beep, 0.7,  0.5, 0, 100, 0, &pos);
#endif
				}

				// CBaseEntity *m_pCBaseEntity = player.entity->GetUnknown()->GetBaseEntity(); 
				ProcessSetColour(player.entity, 255, 0, 0, 255 );

				if (gpManiGameType->GetAdvancedEffectsAllowed())
				{
					float	beep_radius = mani_tk_freeze_bomb_beep_radius.GetFloat();

					if (beep_radius < 1.0)
					{
						beep_radius = mani_tk_freeze_bomb_blast_radius.GetFloat();
					}

					pos.z += 30;

					MRecipientFilter mrf; // this is my class, I'll post it later.
					mrf.MakeReliable();
					mrf.AddAllPlayers(max_players);

					// Advanced effects allowed for ring point
					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											15, // Frame Rate
											0.2, // Life
											16, // Width
											0, // Spread,
											0, // Amplitude
											255,128,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags

					temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
											0.0, // Delay
											pos, // Origin
											16.0,  // Start radius
											beep_radius, // End radius
											bluelight_index,  // Model index
											0, // Halo index
											0, // Start Frame
											10, // Frame Rate
											0.4, // Life
											20, // Width
											0, // Spread,
											1, // Amplitude
											255,20,255,255, // Colour information
											5, // Rate
											0);
											//FBEAM_FADEOUT); //Flags
				}

				punish_mode_list[i].next_freeze_bomb_update_time = gpGlobals->curtime + 0.7;
				punish_mode_list[i].freeze_bomb_beeps_remaining --;
				continue;
			}

			if (punish_mode_list[i].freeze_bomb_beeps_remaining == 0)
			{
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);

				// Set everyone on freeze
				if (mani_tk_freeze_bomb_blast_mode.GetInt() > 0)
				{
					for (int j = 1; j <= max_players; j++)
					{
						player_t	blast_player;

						// Don't include current player
						if (j == i + 1) continue;

						blast_player.index = j;

						if (!FindPlayerByIndex(&blast_player)) continue;
						if (blast_player.is_dead) continue;

						// Not on same team
						if (mani_tk_freeze_bomb_blast_mode.GetInt() == 1)
						{
							if (blast_player.team != player.team) continue;
						}
						else
						{
							if (!gpManiGameType->IsValidActiveTeam(blast_player.team)) continue;
						}

						Vector	v = blast_player.player_info->GetAbsOrigin() - player.player_info->GetAbsOrigin();

						if (v.Length() < mani_tk_freeze_bomb_blast_radius.GetInt())
						{
							// Add this player to the list
							AddToList((void **) &time_bomb_list, sizeof(time_bomb_t), &time_bomb_list_size);
							time_bomb_list[time_bomb_list_size - 1].index = j;
							time_bomb_list[time_bomb_list_size - 1].entity_ptr = blast_player.entity;
						}
					}

					Vector source_pos = player.entity->GetCollideable()->GetCollisionOrigin();
					source_pos.z += 50;

					if (time_bomb_list != 0)
					{
						if (mani_tk_freeze_bomb_show_beams.GetInt() > 0 && effects)
						{
							// Show the beams :)
							/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
							int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
							float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
							unsigned char noise, unsigned char red, unsigned char green,
							unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/

							for (int j = 0; j < time_bomb_list_size; j ++)
							{
								Vector victim_pos = time_bomb_list[j].entity_ptr->GetCollideable()->GetCollisionOrigin();

								victim_pos.z += 50;
								effects->Beam(victim_pos, source_pos, orangelight_index, 0, 0, 5, 1.0, 10, 10, 1, 0, 128, 255, 255, 255, 64);
							}
						}

						for (int j = 0; j < time_bomb_list_size; j ++)
						{
							player_t blast_player;

							blast_player.index = time_bomb_list[j].index;

							if (!FindPlayerByIndex(&blast_player)) continue;

							ProcessFreezePlayer(&blast_player, (punish_mode_list[i].freeze_bomb == MANI_TK_ENFORCED) ? false:true);
						}
					}
				}


				ProcessFreezePlayer(&player, (punish_mode_list[i].freeze_bomb == MANI_TK_ENFORCED) ? false:true);
				punish_mode_list[i].freeze_bomb = 0;
				FreeList((void **) &time_bomb_list, &time_bomb_list_size);
				continue;
			}
		}

		CheckFreezeBombGlobal();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle in frame for Freeze Bombed users
//---------------------------------------------------------------------------------
static
void ProcessBeaconFrame(void)
{
	// Update any freeze bombed users
	if (punish_global.beacon)
	{
		for (int i = 0; i < max_players; i++)
		{
			player_t	player;

			if (punish_mode_list[i].beacon == 0) continue;
			if (gpGlobals->curtime < punish_mode_list[i].next_beacon_update_time) continue;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				punish_mode_list[i].beacon = 0;
				continue;
			}

			if (!gpManiGameType->IsValidActiveTeam(player.team))
			{
				punish_mode_list[i].beacon = 0;
				continue;
			}

			if (player.is_dead || player.health == 0)
			{
				punish_mode_list[i].beacon = 0;
				continue;
			}

			// Just do extended beep
			Vector pos = player.entity->GetCollideable()->GetCollisionOrigin();

			if (esounds)
			{
				// Play the "beacon"  sound
				MRecipientFilter mrf; // this is my class, I'll post it later.
				mrf.MakeReliable();
				mrf.AddAllPlayers(max_players);
#if defined ( GAME_CSGO )
				esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, NULL, 0, beacon_sound, 0.7,  0.5, 0, 0, 100, &pos);
#else
				esounds->EmitSound((IRecipientFilter &)mrf, player.index, CHAN_AUTO, beacon_sound, 0.7,  0.5, 0, 100, 0, &pos);
#endif
			}

			if (gpManiGameType->GetAdvancedEffectsAllowed())
			{
				float	beep_radius = mani_tk_beacon_radius.GetFloat();

				pos.z += 30;

				MRecipientFilter mrf; // this is my class, I'll post it later.
				mrf.MakeReliable();
				mrf.AddAllPlayers(max_players);

				// Advanced effects allowed for ring point
				temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
										0.0, // Delay
										pos, // Origin
										16.0,  // Start radius
										beep_radius, // End radius
										purplelaser_index,  // Model index
										0, // Halo index
										0, // Start Frame
										15, // Frame Rate
										0.2, // Life
										16, // Width
										0, // Spread,
										0, // Amplitude
										255,255,255,255, // Colour information
										4, // Rate
										0);
											//FBEAM_FADEOUT); //Flags
								temp_ents->BeamRingPoint((IRecipientFilter &)mrf, 
										0.0, // Delay
										pos, // Origin
										16.0,  // Start radius
										beep_radius, // End radius
										purplelaser_index,  // Model index
										0, // Halo index
										0, // Start Frame
										10, // Frame Rate
										0.4, // Life
										20, // Width
										0, // Spread,
										1, // Amplitude
										255,20,20,255, // Colour information
										4, // Rate
										0);

			}

			punish_mode_list[i].next_beacon_update_time = gpGlobals->curtime + 0.7;
			continue;
		}

		CheckBeaconGlobal();
	}
}
//---------------------------------------------------------------------------------
// Purpose: Say string to all players in center of screen
//---------------------------------------------------------------------------------
static
void ProcessBombCount
(
player_t	*bomb_target_ptr,
int	mode,
const char	*fmt, 
...
)
{
	va_list		argptr;
	char		tempString[128];

	va_start ( argptr, fmt );
	vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;
	mrf.MakeReliable();

	if (mode == 0)
	{
		// Just the player
		if (bomb_target_ptr->is_bot) return;
		mrf.AddPlayer(bomb_target_ptr->index);
	}
	else if (mode == 1)
	{
		// All players on same team
		for (int i = 1; i <= max_players; i++)
		{
			player_t player;

			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_dead) continue;
			if (player.player_info->IsHLTV()) continue;
			if (player.team != bomb_target_ptr->team) continue;
			if (FStrEq(player.player_info->GetNetworkIDString(),"BOT")) continue;

			mrf.AddPlayer(player.index);
		}
	}
	else
	{
		// All active players
		for (int i = 1; i <= max_players; i++)
		{
			player_t player;

			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			if (player.is_dead) continue;
			if (player.player_info->IsHLTV()) continue;
			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;
			if (FStrEq(player.player_info->GetNetworkIDString(),"BOT")) continue;

			mrf.AddPlayer(player.index);
		}
	}

#if defined ( GAME_CSGO )
	CCSUsrMsg_TextMsg *msg = (CCSUsrMsg_TextMsg *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_TextMsg)->New(); // Show TextMsg type user message
	msg->set_msg_dst(4);
	// Client tries to read all 5 'params' and will crash if less
	msg->add_params(tempString);
	msg->add_params("");
	msg->add_params("");
	msg->add_params("");
	msg->add_params("");
	engine->SendUserMessage(mrf, CS_UM_TextMsg, *msg);
	delete msg;
#else
	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index); // Show TextMsg type user message

	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(tempString);
	engine->MessageEnd();
#endif	
}

//---------------------------------------------------------------------------------
// Purpose: Blind a player
//---------------------------------------------------------------------------------

void BlindPlayer( player_t *player, int blind_amount)
{
	MRecipientFilter mrf; 
	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	mrf.AddPlayer(player->index);

#if defined ( GAME_CSGO )
	CCSUsrMsg_Fade *msg = (CCSUsrMsg_Fade *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_Fade)->New();
	msg->set_duration(1536);
	msg->set_hold_time(1536);
	if (blind_amount == 0)
	{
		msg->set_flags(0x0001 | 0x0010); // Purge fade
	}
	else
	{
		msg->set_flags(0x0002 | 0x0008);
	}
	CMsgRGBA * rgba = msg->mutable_clr();
	rgba->set_r(0);
	rgba->set_g(0);
	rgba->set_b(0);
	rgba->set_a(blind_amount);
	engine->SendUserMessage(mrf, CS_UM_Fade, *msg);
	delete msg;
#else
	msg_buffer = engine->UserMessageBegin( &mrf, fade_message_index );


	msg_buffer->WriteShort(1536);
	msg_buffer->WriteShort(1536);
	if (blind_amount == 0)
	{
		msg_buffer->WriteShort(0x0001 | 0x0010); // Purge fade
	}
	else
	{
		msg_buffer->WriteShort(0x0002 | 0x0008);
	}

	msg_buffer->WriteByte(0);
	msg_buffer->WriteByte(0);
	msg_buffer->WriteByte(0);
	msg_buffer->WriteByte(blind_amount);
	engine->MessageEnd();
#endif	
}

//---------------------------------------------------------------------------------
// Purpose: Slay a player 
//---------------------------------------------------------------------------------

void	SlayPlayer
(
 player_t *player_ptr,
 bool	kill_as_suicide,
 bool	use_beam,
 bool	use_explosion
 )
{
	if (war_mode) return;

	Vector pos = player_ptr->player_info->GetAbsOrigin();

	if (esounds)
	{
		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();
		mrf.AddAllPlayers(max_players);
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, NULL, 0, slay_sound_name, 0.5,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, slay_sound_name, 0.5,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}
		else
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, NULL, 0, hl2mp_slay_sound_name, 0.6,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, hl2mp_slay_sound_name, 0.6,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}

	}

	if (effects)
	{
		// Generate some FXs
		int mag = 60;
		int trailLen = 4;
		pos.z+=12; // rise the effect for better results :)
		effects->Sparks(pos, mag, trailLen, NULL);

		// Get player location, so we know where to play the sound.
		Vector end = pos;
		end.z += 500;

		/*	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed) = 0;*/

		if (use_beam)
		{
			effects->Beam(pos, end, lgtning_index, 0, 0, 5, 1.0, 10, 10, 1, 0, 255, 255, 255, 255, 64);
		}
		
		if (use_explosion && gpManiGameType->GetAdvancedEffectsAllowed() && !gpManiGameType->IsGameType(MANI_GAME_CSGO))
		{
			MRecipientFilter f;
			f.AddAllPlayers(max_players);

			for (int j = 0; j < 4; j++)
			{
				temp_ents->Explosion( f, randomStr->RandomFloat( 0.0, 1.0 ), &pos, 
									explosion_index, randomStr->RandomInt( 4, 10 ), 
									randomStr->RandomInt( 8, 15 ),
                                    #if defined ( GAME_CSGO )
									( j < 2 ) ? TE_EXPLFLAG_NOSOUND : TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOFIREBALLSMOKE,
									#else									
									( j < 2 ) ? TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND : TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NODLIGHTS,
									#endif
									500, 0 );
			}	
		}
	}

//	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_COMMIT_SUICIDE) == -1)
/*	if (1)
	{*/
		if (player_ptr->is_bot)
		{
			char kill_cmd[128];
			int j = Q_strlen(player_ptr->name) - 1;

			while (j != -1)
			{
				if (player_ptr->name[j] == '\0') break;
				if (player_ptr->name[j] == ' ') break;
				j--;
			}

			j++;

			snprintf(kill_cmd, sizeof(kill_cmd), "bot_kill \"%s\"\n", &(player_ptr->name[j]));
			engine->ServerCommand(kill_cmd);
		}
		else
		{
			gpManiDelayedClientCommand->AddPlayer(player_ptr->entity, 0.1f, "kill");
			//helpers->ClientCommand(player_ptr->entity, "kill");
			//helpers->ClientCommand(player->entity, "kill\n");
		}
/*	}
	else
	{
		CBasePlayer *pPlayer = (CBasePlayer *) EdictToCBE(player_ptr->entity);
		if (pPlayer)
		{
			CBasePlayer_CommitSuicide(pPlayer);
		}
	}
/*	}
	else
	{
        CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity();
        CBasePlayer *m_pCBasePlayer = (CBasePlayer *) m_pCBaseEntity;
        if (!m_pCBasePlayer->IsAlive()) return;

		m_pCBasePlayer->Event_Killed( CTakeDamageInfo( m_pCBasePlayer, m_pCBasePlayer, 0, DMG_NEVERGIB ) );
		m_pCBasePlayer->Event_Dying();
	}
*/

	if (!kill_as_suicide)
	{
		CBaseEntity *pCBE = EdictToCBE(player_ptr->entity);
		if (Map_CanUseMap(pCBE, MANI_VAR_FRAGS))
		{
			int frags = Map_GetVal(pCBE, MANI_VAR_FRAGS, 0);
			frags += 1;
			Map_SetVal(pCBE, MANI_VAR_FRAGS, frags);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Slap a player
//---------------------------------------------------------------------------------
void	ProcessSlapPlayer
(
 player_t *player, 
 int damage,
 bool slap_angle
)
{

	// let's just slap for if mani_tk_team_wound_reflect is set to 1
	int sound_index = rand() % 3;
	if (!gpManiGameType->IsTeleportAllowed()) return;

	CBaseEntity *m_pCBaseEntity = player->entity->GetUnknown()->GetBaseEntity(); 

	Vector vVel;
	CBaseEntity_GetVelocity(m_pCBaseEntity, &vVel, NULL); 

	if (slap_angle)
	{
		vVel.x += ((rand() % 180) + 50) * (((rand() % 2) == 1) ? -1:1);
		vVel.y += ((rand() % 180) + 50) * (((rand() % 2) == 1) ? -1:1);
		QAngle angle = CBaseEntity_EyeAngles(m_pCBaseEntity);
		angle.x -= 20;
		CBaseEntity_Teleport(m_pCBaseEntity, NULL, &angle, &vVel);
	}
	else
	{
		vVel.x += ((rand() % 180) + 50) * (((rand() % 2) == 1) ? -1:1);
		vVel.y += ((rand() % 180) + 50) * (((rand() % 2) == 1) ? -1:1);
		vVel.z += rand() % 200 + 100;
		CBaseEntity_Teleport(m_pCBaseEntity, NULL, NULL, &vVel);
	}

	int health = 0;

	health = Prop_GetVal(player->entity, MANI_PROP_HEALTH, 0);
	// health = m_pCBaseEntity->GetHealth();
	if (health <= 0)
	{
		return;
	}
		
	health -= ((damage >= 0) ? damage : damage * -1);
	if (health <= 0)
	{
		health = 0;
	}

	//m_pCBaseEntity->SetHealth(health);
	Prop_SetVal(player->entity, MANI_PROP_HEALTH, health);

//	Vector vVel = m_pCBaseEntity->GetLocalVelocity();

	if (health <= 0)
	{
		// Player dead
		SlayPlayer(player, false, false, false);
	}

	if (esounds)
	{
		Vector pos = player->entity->GetCollideable()->GetCollisionOrigin();

		// Play the "death"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();
		mrf.AddAllPlayers(max_players);
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, NULL, 0, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}
		else
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, NULL, 0, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player->index, CHAN_AUTO, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}

	}
}

//---------------------------------------------------------------------------------
// Purpose: Burn a player
//---------------------------------------------------------------------------------
void	ProcessBurnPlayer
(
 player_t *player, 
 int burn_time
)
{
	std::vector<int> flamelist;
	// first get alist of the entityflame entities that already exist
	// we do this to avoid sig scanning for the entity list in all the
	// different games
	int max = engine->GetEntityCount();
	for ( int index = 0; index < max; index++ ) {
#if defined ( GAME_CSGO )
		edict_t *pEdict = PEntityOfEntIndex( index );
#else
		edict_t *pEdict = engine->PEntityOfEntIndex( index );
#endif
		if ( !pEdict ) continue;
		if ( FStrEq ( "entityflame", pEdict->GetClassName() ) ) {
			flamelist.push_back(index);
		}
	}

	CBaseEntity *m_pCBaseEntity = player->entity->GetUnknown()->GetBaseEntity(); 
	CBasePlayer *pBase = (CBasePlayer*) m_pCBaseEntity;
	CBasePlayer_Ignite(pBase, burn_time, false, 12, false);
//	pBase->Ignite(burn_time ,false, 12, false);

	max = engine->GetEntityCount(); // should be one more :)
	for ( int index = 0; index < max; index++ ) {
#if defined ( GAME_CSGO )
		edict_t *pEdict = PEntityOfEntIndex( index );
#else
		edict_t *pEdict = engine->PEntityOfEntIndex( index );
#endif
		if ( !pEdict ) continue;
		if ( FStrEq ( "entityflame", pEdict->GetClassName() ) ) {
			if ( flamelist.size() == 0 ) {
				punish_mode_list[player->index - 1].flame_index = index;
				break;
			}
			for ( int fi = 0; fi< (int)flamelist.size(); fi ++ ) {
				if ( flamelist[fi] == index )
					break;

				punish_mode_list[player->index - 1].flame_index = index;
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Setup no clip on a player
//---------------------------------------------------------------------------------
void	ProcessNoClipPlayer(player_t *player)
{
	if (!sv_cheats) return;

	if (sv_cheats->GetInt() == 0)
	{
        sv_cheats->m_nFlags &= ~FCVAR_SPONLY;
        sv_cheats->m_nFlags &= ~FCVAR_NOTIFY;
		sv_cheats->SetValue(1);
		helpers->ClientCommand(player->entity, "noclip");
		sv_cheats->SetValue(0);
        sv_cheats->m_nFlags |= FCVAR_SPONLY;
        sv_cheats->m_nFlags |= FCVAR_NOTIFY;
	}
	else
	{
		helpers->ClientCommand(player->entity, "noclip");
	}

	if (punish_mode_list[player->index - 1].no_clip)
	{
		punish_mode_list[player->index - 1].no_clip = false;
	}
	else
	{
		punish_mode_list[player->index - 1].no_clip = true;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Freeze a player
//---------------------------------------------------------------------------------
void	ProcessFreezePlayer(player_t *player_ptr, bool admin_called)
{
	int index = player_ptr->index - 1;

	if (punish_mode_list[index].frozen) return;

	Prop_SetVal(player_ptr->entity, MANI_PROP_MOVE_TYPE, MOVETYPE_NONE); 
	ProcessSetColour(player_ptr->entity, 0, 128, 255, 135 );

	punish_mode_list[player_ptr->index].next_frozen_update_time = -999;

	if (admin_called)
	{
		punish_mode_list[index].frozen = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[index].frozen = MANI_TK_ENFORCED;
	}

	punish_global.frozen = true;

	if (esounds)
	{
		int sound_index = rand() % 3;
		Vector pos = player_ptr->entity->GetCollideable()->GetCollisionOrigin();

		// Play the "hurt"  sound
		MRecipientFilter mrf; // this is my class, I'll post it later.
		mrf.MakeReliable();
		mrf.AddAllPlayers(max_players);
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO)))
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, NULL, 0, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}
		else
		{
#if defined ( GAME_CSGO )
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, NULL, 0, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 0, 100, &pos);
#else
			esounds->EmitSound((IRecipientFilter &)mrf, player_ptr->index, CHAN_AUTO, hl2mp_slap_sound_name[sound_index].sound_name, 0.7,  ATTN_NORM, 0, 100, 0, &pos);
#endif
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Freeze a player
//---------------------------------------------------------------------------------
void	ProcessUnFreezePlayer(player_t *player_ptr)
{
	int index = player_ptr->index - 1;

	if (!punish_mode_list[index].frozen) return;

	Prop_SetVal(player_ptr->entity, MANI_PROP_MOVE_TYPE, MOVETYPE_WALK); 

	ProcessSetColour(player_ptr->entity, 255, 255, 255, 255 );

	punish_mode_list[index].frozen = 0;
	punish_mode_list[player_ptr->index].next_frozen_update_time = -999;
	CheckFrozenGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Teleports a player
//---------------------------------------------------------------------------------
void	ProcessTeleportPlayer(player_t *player_to_teleport, Vector *origin_ptr)
{
	if (!gpManiGameType->IsTeleportAllowed()) return;

	CBaseEntity *m_pCBaseEntity = player_to_teleport->entity->GetUnknown()->GetBaseEntity(); 

	Vector vVel;
	vVel.x = 0;
	vVel.y = 0;
	vVel.z = 0;

/*	MMsg("Teleporting [%s] to X = %f, Y = %f, Z = %f\n", 
				player_to_teleport->name,
				origin_ptr->x,
				origin_ptr->y,
				origin_ptr->z);
*/
	CBaseEntity_Teleport(m_pCBaseEntity, origin_ptr, NULL, &vVel);
}

//---------------------------------------------------------------------------------
// Purpose: Takes cash from a player and gives it to another
//---------------------------------------------------------------------------------
void	ProcessTakeCash (player_t *donator, player_t *receiver)
{
	if (!gpManiGameType->CanUseProp(MANI_PROP_ACCOUNT)) return;

	int cash_to_give = 0;

	int donators_cash = Prop_GetVal(donator->entity, MANI_PROP_ACCOUNT, 0);
	int receiver_cash = Prop_GetVal(receiver->entity, MANI_PROP_ACCOUNT, 0);

//	receiver_cash = ((int *)receiver->entity->GetUnknown() + offset);

	cash_to_give = donators_cash * (mani_tk_cash_percent.GetFloat() * 0.01);

	if (cash_to_give < 1)
	{
		return;
	}

	if (receiver_cash + cash_to_give > 16000)
	{
		Prop_SetVal(receiver->entity, MANI_PROP_ACCOUNT, 160000);
	}
	else
	{
		Prop_SetVal(receiver->entity, MANI_PROP_ACCOUNT, (receiver_cash + cash_to_give));
	}

	// Donators cash should never go negative
	Prop_SetVal(donator->entity, MANI_PROP_ACCOUNT, (donators_cash - cash_to_give));
	// *donators_cash = *donators_cash - cash_to_give;
	
}

//---------------------------------------------------------------------------------
// Purpose: Saves a players location
//---------------------------------------------------------------------------------
void	ProcessSaveLocation(player_t *player)
{
	player_settings_t *player_settings;

	player_settings = FindPlayerSettings(player);
	if (!player_settings) return;

	Vector current_position = player->player_info->GetAbsOrigin();
	
	bool found_map = false;
	for (int i = 0; i < player_settings->teleport_coords_list_size; i ++)
	{
		if (FStrEq(player_settings->teleport_coords_list[i].map_name, current_map))
		{
			found_map = true;
			player_settings->teleport_coords_list[i].coords = current_position;
			break;
		}
	}

	if (!found_map)
	{
		// Need to add a new map to the list
		AddToList((void **) &(player_settings->teleport_coords_list), sizeof(teleport_coords_t), &(player_settings->teleport_coords_list_size));
		Q_strcpy(player_settings->teleport_coords_list[player_settings->teleport_coords_list_size - 1].map_name, current_map);
		player_settings->teleport_coords_list[player_settings->teleport_coords_list_size - 1].coords = current_position;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Mute a player
//---------------------------------------------------------------------------------
void	ProcessMutePlayer(player_t *player, player_t *giver, int timetomute, bool byID, const char *reason)
{
	punish_mode_list[player->index - 1].muted = MANI_ADMIN_ENFORCED;
	const char *key = (byID) ? player->steam_id : player->ip_address;
	if ( !giver )
		gpManiAdminPlugin->AddMute( player, key, "CONSOLE", timetomute, "Muted", reason);
	else
		gpManiAdminPlugin->AddMute( player, key, giver->name, timetomute, "Muted", reason);

	gpManiAdminPlugin->WriteMutes();
}

//---------------------------------------------------------------------------------
// Purpose: Un-Mute a player
//---------------------------------------------------------------------------------
void	ProcessUnMutePlayer(player_t *player)
{
	punish_mode_list[player->index - 1].muted = 0;
	gpManiAdminPlugin->RemoveMute(player->steam_id);
	gpManiAdminPlugin->RemoveMute(player->ip_address);
	gpManiAdminPlugin->WriteMutes();
}
//---------------------------------------------------------------------------------
// Purpose: Drug a player
//---------------------------------------------------------------------------------
void	ProcessDrugPlayer(player_t *player, bool admin_called)
{
	if (admin_called)
	{
		punish_mode_list[player->index - 1].drugged = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[player->index - 1].drugged = MANI_TK_ENFORCED;
	}

	punish_mode_list[player->index - 1].next_drug_update_time = -999;
	punish_global.drugged = true;
}

//---------------------------------------------------------------------------------
// Purpose: Un-Drug a player
//---------------------------------------------------------------------------------
void    ProcessUnDrugPlayer(player_t *player)
{
	if (!gpManiGameType->IsTeleportAllowed()) return;

	punish_mode_list[player->index - 1].drugged = 0;
	punish_mode_list[player->index - 1].next_drug_update_time = -999;

	CBaseEntity *m_pCBaseEntity = player->entity->GetUnknown()->GetBaseEntity();
	if (m_pCBaseEntity)
	{
//        QAngle angle = m_pCBaseEntity->EyeAngles();
        QAngle angle = CBaseEntity_EyeAngles(m_pCBaseEntity);
		angle.z = 0;
		CBaseEntity_Teleport(m_pCBaseEntity, NULL, &angle, NULL);
	}

	CheckDruggedGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Time bomb a player
//---------------------------------------------------------------------------------
void	ProcessTimeBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called)
{
	if (punish_mode_list[player_ptr->index - 1].time_bomb)
	{
		return;
	}

	if (admin_called)
	{
		punish_mode_list[player_ptr->index - 1].time_bomb = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[player_ptr->index - 1].time_bomb = MANI_TK_ENFORCED;
	}

	punish_mode_list[player_ptr->index - 1].next_time_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].time_bomb_beeps_remaining = mani_tk_time_bomb_seconds.GetInt();
	punish_global.time_bomb = true;
}

//---------------------------------------------------------------------------------
// Purpose: Un Time bomb a player
//---------------------------------------------------------------------------------
void	ProcessUnTimeBombPlayer(player_t *player_ptr)
{
	if (!punish_mode_list[player_ptr->index - 1].time_bomb)
	{
		return;
	}

	// CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity(); 
	ProcessSetColour(player_ptr->entity, 255, 255, 255, 255 );

	punish_mode_list[player_ptr->index - 1].time_bomb = 0;
	punish_mode_list[player_ptr->index - 1].next_time_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].time_bomb_beeps_remaining = mani_tk_time_bomb_seconds.GetInt();

	CheckTimeBombGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Fire Bomb a player
//---------------------------------------------------------------------------------
void	ProcessFireBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called)
{
	if (punish_mode_list[player_ptr->index - 1].fire_bomb)
	{
		return;
	}

	if (admin_called)
	{
		punish_mode_list[player_ptr->index - 1].fire_bomb = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[player_ptr->index - 1].fire_bomb = MANI_TK_ENFORCED;
	}

	punish_mode_list[player_ptr->index - 1].next_fire_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].fire_bomb_beeps_remaining = mani_tk_fire_bomb_seconds.GetInt();
	punish_global.fire_bomb = true;
}

//---------------------------------------------------------------------------------
// Purpose: Un Fire bomb a player
//---------------------------------------------------------------------------------
void	ProcessUnFireBombPlayer(player_t *player_ptr)
{
	if (!punish_mode_list[player_ptr->index - 1].fire_bomb)
	{
		return;
	}

	// CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity(); 
	ProcessSetColour(player_ptr->entity, 255, 255, 255, 255 );

	punish_mode_list[player_ptr->index - 1].fire_bomb = 0;
	punish_mode_list[player_ptr->index - 1].next_fire_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].fire_bomb_beeps_remaining = mani_tk_fire_bomb_seconds.GetInt();

	CheckFireBombGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Freeze Bomb a player
//---------------------------------------------------------------------------------
void	ProcessFreezeBombPlayer(player_t *player_ptr, bool just_spawned, bool admin_called)
{
	if (punish_mode_list[player_ptr->index - 1].freeze_bomb)
	{
		return;
	}

	if (admin_called)
	{
		punish_mode_list[player_ptr->index - 1].freeze_bomb = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[player_ptr->index - 1].freeze_bomb = MANI_TK_ENFORCED;
	}
	punish_mode_list[player_ptr->index - 1].next_freeze_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].freeze_bomb_beeps_remaining = mani_tk_freeze_bomb_seconds.GetInt();
	punish_global.freeze_bomb = true;
}

//---------------------------------------------------------------------------------
// Purpose: Un Freeze bomb a player
//---------------------------------------------------------------------------------
void	ProcessUnFreezeBombPlayer(player_t *player_ptr)
{
	if (!punish_mode_list[player_ptr->index - 1].freeze_bomb)
	{
		return;
	}

	//CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity(); 
	ProcessSetColour(player_ptr->entity, 255, 255, 255, 255 );

	punish_mode_list[player_ptr->index - 1].freeze_bomb = 0;
	punish_mode_list[player_ptr->index - 1].next_freeze_bomb_update_time = -999;
	punish_mode_list[player_ptr->index - 1].freeze_bomb_beeps_remaining = mani_tk_freeze_bomb_seconds.GetInt();

	CheckFreezeBombGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Freeze Bomb a player
//---------------------------------------------------------------------------------
void	ProcessBeaconPlayer(player_t *player_ptr, bool admin_called)
{
	if (punish_mode_list[player_ptr->index - 1].beacon)
	{
		return;
	}

	if (admin_called)
	{
		punish_mode_list[player_ptr->index - 1].beacon = MANI_ADMIN_ENFORCED;
	}
	else
	{
		punish_mode_list[player_ptr->index - 1].beacon = MANI_TK_ENFORCED;
	}

	punish_mode_list[player_ptr->index - 1].next_beacon_update_time = -999;
	punish_global.beacon = true;
}

//---------------------------------------------------------------------------------
// Purpose: Un Freeze bomb a player
//---------------------------------------------------------------------------------
void	ProcessUnBeaconPlayer(player_t *player_ptr)
{
	if (!punish_mode_list[player_ptr->index - 1].beacon)
	{
		return;
	}

	//CBaseEntity *m_pCBaseEntity = player_ptr->entity->GetUnknown()->GetBaseEntity(); 
	ProcessSetColour(player_ptr->entity, 255, 255, 255, 255 );

	punish_mode_list[player_ptr->index - 1].beacon = 0;
	punish_mode_list[player_ptr->index - 1].next_freeze_bomb_update_time = -999;

	CheckBeaconGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Show Beams from one player to another
//---------------------------------------------------------------------------------
void	ProcessDeathBeam(player_t *attacker_ptr, player_t *victim_ptr)
{
	if (!gpManiGameType->GetAdvancedEffectsAllowed())
	{
		return;
	}

	if (!gpManiGameType->IsDeathBeamAllowed()) return;

	if (attacker_ptr->user_id <= 0) return;
	if (attacker_ptr->user_id == victim_ptr->user_id) return;
	if (victim_ptr->is_bot) return;

	if (attacker_ptr->entity == NULL)
	{
		if (!FindPlayerByUserID(attacker_ptr)) return;
	}

	player_settings_t	*player_settings;
	player_settings = FindPlayerSettings(victim_ptr);

	if (!player_settings) return;
	if (!player_settings->show_death_beam) return;

	MRecipientFilter mrf; // this is my class, I'll post it later.
	mrf.MakeReliable();
	mrf.AddPlayer(victim_ptr->index);

	CBaseEntity *pPlayer = attacker_ptr->entity->GetUnknown()->GetBaseEntity();

	Vector source = CBaseEntity_EyePosition(pPlayer);
	Vector dest = victim_ptr->player_info->GetAbsOrigin();

	temp_ents->BeamPoints((IRecipientFilter &)mrf,
							0, // Delay
							&source, // Start Vector
							&dest, // End Vector
							purplelaser_index, // model index
							0, // halo index
							0, // Frame start
							10, // Frame rate
							15.0, // Frame life
							7, // Width
							7, // End Width
							2, // Fade length
							0.1, // Noise amplitude
							255, 255, 255, 255,
							5);

}

//---------------------------------------------------------------------------------
// Purpose: Set Render colour for entity
//---------------------------------------------------------------------------------
void	ProcessSetColour(edict_t *pEntity, int r, int g, int b, int a)
{
	if (!gpManiGameType->IsSetColourAllowed()) return;

	if (a != 255 && gpManiGameType->CanUseProp(MANI_PROP_RENDER_MODE))
	{
		Prop_SetVal(pEntity, MANI_PROP_RENDER_MODE, gpManiGameType->GetAlphaRenderMode());
	}

	Prop_SetColor(pEntity, r,g,b,a);
}

//---------------------------------------------------------------------------------
// Purpose: Set Render colour for entity
//---------------------------------------------------------------------------------
void	ProcessSetWeaponColour(CBaseEntity *pCBaseEntity, int r, int g, int b, int a)
{
	if (!gpManiGameType->IsSetColourAllowed()) return;

	CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pCBaseEntity);

	if (!pCombat) return;

	for (int i = 0; i < 10; i++)
	{
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, i);

		if (pWeapon)
		{
			edict_t *pEntity = serverents->BaseEntityToEdict(pWeapon);
			if (a != 255)
			{
				Prop_SetVal(pEntity, MANI_PROP_RENDER_MODE, gpManiGameType->GetAlphaRenderMode());
			}

			Prop_SetColor(pEntity, r, g, b, a);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as drugged
//---------------------------------------------------------------------------------
static
void	CheckDruggedGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].drugged)
		{
			punish_global.drugged = true;
			return;
		}
	}

	punish_global.drugged = false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as frozne
//---------------------------------------------------------------------------------
static
void	CheckFrozenGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].frozen)
		{
			punish_global.frozen = true;
			return;
		}
	}

	punish_global.frozen = false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as being a time bomb
//---------------------------------------------------------------------------------
static
void	CheckTimeBombGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].time_bomb)
		{
			punish_global.time_bomb = true;
			return;
		}
	}

	punish_global.time_bomb = false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as being a fire bomb
//---------------------------------------------------------------------------------
static
void	CheckFireBombGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].fire_bomb)
		{
			punish_global.fire_bomb = true;
			return;
		}
	}

	punish_global.fire_bomb = false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as being a freeze bomb
//---------------------------------------------------------------------------------
static
void	CheckFreezeBombGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].freeze_bomb)
		{
			punish_global.freeze_bomb = true;
			return;
		}
	}

	punish_global.freeze_bomb = false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if any players marked as being a beacon
//---------------------------------------------------------------------------------
static
void	CheckBeaconGlobal(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (punish_mode_list[i].beacon)
		{
			punish_global.beacon = true;
			return;
		}
	}

	punish_global.beacon = false;
}
//---------------------------------------------------------------------------------
// Purpose: Check if any modes are running
//---------------------------------------------------------------------------------
static
void	CheckAllGlobal(void)
{
	CheckDruggedGlobal();
	CheckFrozenGlobal();
	CheckTimeBombGlobal();
	CheckFireBombGlobal();
	CheckFreezeBombGlobal();
	CheckBeaconGlobal();
}

//---------------------------------------------------------------------------------
// Purpose: Trace Line to position
//---------------------------------------------------------------------------------
void	MANI_TraceLineWorldProps
(
 const Vector& vecAbsStart, 
 const Vector& vecAbsEnd, 
 unsigned int mask, 
 int collisionGroup, 
 trace_t *ptr
 )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterWorldAndPropsOnly traceFilter;
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

