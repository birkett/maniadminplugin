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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_afk.h" 
#include "mani_vfuncs.h"
#include "mani_hook.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

ConVar mani_afk_kicker_mode ("mani_afk_kicker_mode", "0", 0, "0 = kick to spectator first, 1 = kick straight off the server", true, 0, true, 1);
ConVar mani_afk_kicker_alive_rounds ("mani_afk_kicker_alive_rounds", "0", 0, "0 = disabled, > 0 = number of rounds before kick/move", true, 0, true, 20);
ConVar mani_afk_kicker_spectator_rounds ("mani_afk_kicker_spectator_rounds", "0", 0, "0 = disabled, > 0 = number of rounds before kick", true, 0, true, 20);
ConVar mani_afk_kicker_alive_timer ("mani_afk_kicker_alive_timer", "0", 0, "0 = disabled, > 0 = number of seconds before kick/move", true, 0, true, 1200);
ConVar mani_afk_kicker_spectator_timer ("mani_afk_kicker_spectator_timer", "0", 0, "0 = disabled, > 0 = number of seconds before kick", true, 0, true, 1200);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiAFK::ManiAFK()
{
	gpManiAFK = this;
}

ManiAFK::~ManiAFK()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiAFK::Load(void)
{
	for (int i = 1; i <= max_players; i++)
	{
		this->ResetPlayer(i);

		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player))
		{
			continue;
		}

		if (player.is_bot) continue;

		CBaseEntity *pCBE = EdictToCBE(player.entity);
		this->HookPlayer((CBasePlayer *) pCBE);
		afk_list[i - 1].hooked = true;
	}

	next_check = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiAFK::Unload(void)
{
	this->LevelShutdown();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiAFK::LevelInit(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayer(i);
	}
	
	next_check = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Game Commencing
//---------------------------------------------------------------------------------
void	ManiAFK::GameCommencing(void)
{
	time_t current_time;

	time(&current_time);

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		afk_list[i].last_active = current_time;
		afk_list[i].round_count = 0;
		afk_list[i].idle = true;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiAFK::NetworkIDValidated(player_t *player_ptr)
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	this->ResetPlayer(player_ptr->index - 1);

	CBaseEntity *pCBE = EdictToCBE(player_ptr->entity);
	this->HookPlayer((CBasePlayer *) pCBE);
	afk_list[player_ptr->index - 1].hooked = true;
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiAFK::ClientDisconnect(player_t	*player_ptr)
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;

	this->ResetPlayer(player_ptr->index - 1);
	CBaseEntity *pCBE = EdictToCBE(player_ptr->entity);
	this->UnHookPlayer((CBasePlayer *) pCBE);
	afk_list[player_ptr->index - 1].hooked = false;
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiAFK::ProcessUsercmds
(
 CBasePlayer *pPlayer,
 CUserCmd *cmds, 
 int numcmds
 )
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;

	if (!pPlayer) return;

	edict_t *pEdict = serverents->BaseEntityToEdict((CBasePlayer *) pPlayer);
	if (!pEdict) return;

	int index = engine->IndexOfEdict(pEdict);
	if (index < 1) return;

	if (cmds && numcmds && 
		(cmds->forwardmove || cmds->sidemove || cmds->upmove || cmds->mousedx || cmds->mousedy)) 
	{
		time_t current_time;

		time(&current_time);
		afk_list[index - 1].last_active = current_time;
		afk_list[index - 1].idle = false;
		afk_list[index - 1].round_count = 0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do timed afk if needed
//---------------------------------------------------------------------------------
void ManiAFK::GameFrame(void)
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;
	if (mani_afk_kicker_alive_timer.GetInt() == 0 &&
		mani_afk_kicker_spectator_timer.GetInt() == 0) return;

	time_t current_time;

	time(&current_time);

	if (current_time < next_check) return;

	next_check = current_time + 1;

	int max_timeout;
	if (mani_afk_kicker_alive_timer.GetInt() == 0)
	{
		max_timeout = mani_afk_kicker_spectator_timer.GetInt();
	}
	else if (mani_afk_kicker_spectator_timer.GetInt() == 0)
	{
		max_timeout = mani_afk_kicker_alive_timer.GetInt();
	}
	else
	{
		if (mani_afk_kicker_alive_timer.GetInt() > mani_afk_kicker_spectator_timer.GetInt())
		{
			max_timeout = mani_afk_kicker_spectator_timer.GetInt();
		}
		else
		{	
			max_timeout = mani_afk_kicker_alive_timer.GetInt();
		}
	}

	// Process our list
	for (int i = 0; i < max_players; i++)
	{
		if (!afk_list[i].hooked) continue;

		// Faster way of discriminating hopefully without getting player data
		// We are in game frame dont forget :)
		if (afk_list[i].last_active + max_timeout > current_time) continue;

		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			this->ResetPlayer(i);
			CBaseEntity *pCBE = EdictToCBE(player.entity);
			this->UnHookPlayer((CBasePlayer *) pCBE);
			afk_list[i].hooked = false;
			continue;
		}

		if (player.is_bot) continue;

		int client_index = -1;

		if (gpManiGameType->IsValidActiveTeam(player.team) &&
			mani_afk_kicker_alive_timer.GetInt() != 0)
		{

			// Player on active team (considered alive)
			if (afk_list[i].last_active + mani_afk_kicker_alive_timer.GetInt() <= current_time)
			{
				if (gpManiClient->IsImmune(&player, &client_index))
				{
					if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
					{
						this->ResetPlayer(i);
						afk_list[i].hooked = true;
						continue;
					}
				}

				// Exceeded timeout
				this->ResetPlayer(i);
				afk_list[i].hooked = true;
				if (mani_afk_kicker_mode.GetInt() == 0)
				{
					// Shift player to spectator
					player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
					SayToPlayer(&player, "You were moved to the Spectator team for being AFK");
				}
				else
				{
					// Need to kick player
					SayToPlayer(&player, "You have been kicked for being AFK");
					UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
						"You were automatically kicked for being AFK", 
						"Auto-AFK kicked");
				}
			}
		}
		else if (player.team == gpManiGameType->GetSpectatorIndex() &&
			mani_afk_kicker_spectator_timer.GetInt() != 0)
		{
			// Player is spectator
			// Player on active team (considered alive)
			if (afk_list[i].last_active + mani_afk_kicker_spectator_timer.GetInt() <= current_time)
			{
				if (gpManiClient->IsImmune(&player, &client_index))
				{
					if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
					{
						this->ResetPlayer(i);
						afk_list[i].hooked = true;
						continue;
					}
				}

				// Exceeded round count
				this->ResetPlayer(i);
				afk_list[i].hooked = true;
				// Need to kick player
				SayToPlayer(&player, "You have been kicked for being AFK");
				UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
					"You were automatically kicked for being AFK", 
					"Auto-AFK kicked");
			}
		}
		else
		{
			// Player is just joining
			if (mani_afk_kicker_spectator_timer.GetInt() != 0 &&
				afk_list[i].last_active + mani_afk_kicker_spectator_timer.GetInt() <= current_time)
			{
				if (gpManiClient->IsImmune(&player, &client_index))
				{
					if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
					{
						this->ResetPlayer(i);
						afk_list[i].hooked = true;
						continue;
					}
				}

				// Exceeded round count
				this->ResetPlayer(i);
				afk_list[i].hooked = true;

				// Need to kick player
				SayToPlayer(&player, "You have been kicked for being AFK");
				UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
					"You were automatically kicked for being AFK", 
					"Auto-AFK kicked");
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Unhook players on level shutdown
//---------------------------------------------------------------------------------
void ManiAFK::LevelShutdown(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (afk_list[i].hooked)
		{
			this->ResetPlayer(i);

			player_t	player;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player))
			{
				continue;
			}

			CBaseEntity *pCBE = EdictToCBE(player.entity);
			this->UnHookPlayer((CBasePlayer *) pCBE);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do kicks on round end
//---------------------------------------------------------------------------------
void ManiAFK::RoundEnd(void)
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;
	if (mani_afk_kicker_alive_rounds.GetInt() == 0 &&
		mani_afk_kicker_spectator_rounds.GetInt() == 0) return;

	for (int i = 0; i < max_players; i++)
	{
		if (!afk_list[i].hooked) continue;
		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			this->ResetPlayer(i);
			CBaseEntity *pCBE = EdictToCBE(player.entity);
			this->UnHookPlayer((CBasePlayer *) pCBE);
			afk_list[i].hooked = false;
			continue;
		}

		if (player.is_bot) continue;

		if (afk_list[i].idle)
		{
			int client_index = -1;

			afk_list[i].round_count++;
			if (gpManiGameType->IsValidActiveTeam(player.team) &&
				mani_afk_kicker_alive_rounds.GetInt() != 0)
			{

				// Player on active team (considered alive)
				if (afk_list[i].round_count > mani_afk_kicker_alive_rounds.GetInt())
				{
					if (gpManiClient->IsImmune(&player, &client_index))
					{
						if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
						{
							this->ResetPlayer(i);
							afk_list[i].hooked = true;
							continue;
						}
					}

					// Exceeded round count
					this->ResetPlayer(i);
					afk_list[i].hooked = true;
					if (mani_afk_kicker_mode.GetInt() == 0)
					{
						// Shift player to spectator
						player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
						SayToPlayer(&player, "You were moved to the Spectator team for being AFK");
					}
					else
					{
						// Need to kick player
						SayToPlayer(&player, "You have been kicked for being AFK");
						UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
												  "You were automatically kicked for being AFK", 
												  "Auto-AFK kicked");
					}
				}
			}
			else if (player.team == gpManiGameType->GetSpectatorIndex() &&
				mani_afk_kicker_spectator_rounds.GetInt() != 0)
			{
				// Player is spectator
				// Player on active team (considered alive)
				if (afk_list[i].round_count > mani_afk_kicker_spectator_rounds.GetInt())
				{
					if (gpManiClient->IsImmune(&player, &client_index))
					{
						if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
						{
							this->ResetPlayer(i);
							afk_list[i].hooked = true;
							continue;
						}
					}

					// Exceeded round count
					this->ResetPlayer(i);
					afk_list[i].hooked = true;
					// Need to kick player
					SayToPlayer(&player, "You have been kicked for being AFK");
					UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
											  "You were automatically kicked for being AFK", 
											  "Auto-AFK kicked");
				}
			}
			else
			{
				// Player is just joining
				if (afk_list[i].round_count > mani_afk_kicker_spectator_rounds.GetInt() &&
					mani_afk_kicker_spectator_rounds.GetInt() != 0)
				{
					if (gpManiClient->IsImmune(&player, &client_index))
					{
						if (gpManiClient->IsImmunityAllowed(client_index, IMMUNITY_ALLOW_AFK))
						{
							this->ResetPlayer(i);
							afk_list[i].hooked = true;
							continue;
						}
					}

					// Exceeded round count
					this->ResetPlayer(i);
					afk_list[i].hooked = true;

					// Need to kick player
					SayToPlayer(&player, "You have been kicked for being AFK");
					UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
											  "You were automatically kicked for being AFK", 
											  "Auto-AFK kicked");
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Hook a player
//---------------------------------------------------------------------------------
void ManiAFK::HookPlayer(CBasePlayer *pPlayer)
{
	int offset = gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS);

	// Not defined, forget it
	if (offset == -1) return;

#ifndef SOURCEMM
	// Win 32 hook
	HookProcessUsercmds(pPlayer, offset);
#else
	// SourceMM hook
	g_ManiSMMHooks.HookProcessUsercmds(pPlayer);
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Hook a player
//---------------------------------------------------------------------------------
void ManiAFK::UnHookPlayer(CBasePlayer *pPlayer)
{
	int offset = gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS);

	// Not defined, forget it
	if (offset == -1) return;

#ifdef SOURCEMM
	g_ManiSMMHooks.UnHookProcessUsercmds(pPlayer);
#else
	UnHookProcessUsercmds(pPlayer, offset);
#endif
}
//---------------------------------------------------------------------------------
// Purpose: Reset a player on the afk list
//---------------------------------------------------------------------------------
void ManiAFK::ResetPlayer(int index)
{
	time_t current_time;
	time(&current_time);

	afk_list[index].hooked = false;
	afk_list[index].idle = true;
	afk_list[index].last_active = current_time;
	afk_list[index].round_count = 0;
}

ManiAFK	g_ManiAFK;
ManiAFK	*gpManiAFK;