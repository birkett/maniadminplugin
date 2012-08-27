//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_sourcehook.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_warmuptimer.h"
#include "mani_afk.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	time_t	g_RealTime;

CONVAR_CALLBACK_PROTO(ManiAFKKicker);

ConVar mani_afk_kicker ("mani_afk_kicker", "0", 0, "0 = disabled, 1 = enabled", true, 0, true, 1, CONVAR_CALLBACK_REF(ManiAFKKicker));
ConVar mani_afk_kicker_mode ("mani_afk_kicker_mode", "0", 0, "0 = kick to spectator first, 1 = kick straight off the server", true, 0, true, 1);
ConVar mani_afk_kicker_alive_rounds ("mani_afk_kicker_alive_rounds", "0", 0, "0 = disabled, > 0 = number of rounds before kick/move", true, 0, true, 20);
ConVar mani_afk_kicker_spectator_rounds ("mani_afk_kicker_spectator_rounds", "0", 0, "0 = disabled, > 0 = number of rounds before kick", true, 0, true, 20);
ConVar mani_afk_kicker_alive_timer ("mani_afk_kicker_alive_timer", "0", 0, "0 = disabled, > 0 = number of seconds before kick/move", true, 0, true, 1200);
ConVar mani_afk_kicker_spectator_timer ("mani_afk_kicker_spectator_timer", "0", 0, "0 = disabled, > 0 = number of seconds before kick", true, 0, true, 1200);
ConVar mani_afk_kicker_immunity_to_spec_only ("mani_afk_kicker_immunity_to_spec_only", "0", 0, "0 = immune players are unaffected by AFK kicker, 1 = immune players are moved to spectator but not kicked", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiAFK::ManiAFK()
{
	gpManiAFK = this;
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayer(i, false);
		afk_list[i].hooked = false;
	}
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
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS) == -1) return;

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayer(i, false);

		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			continue;
		}

		if (player.is_bot) continue;

		if (!afk_list[i].hooked)
		{
			g_ManiSMMHooks.HookProcessUsercmds((CBasePlayer *) EdictToCBE(player.entity));
			afk_list[i].hooked = true;
		}

		afk_list[i].check_player = true;
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
		this->ResetPlayer(i, false);
		afk_list[i].hooked = false;
	}
	
	next_check = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiAFK::NetworkIDValidated(player_t *player_ptr)
{
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS) == -1) return;
	if (player_ptr->is_bot) return;
	this->ResetPlayer(player_ptr->index - 1, true);
	if (!afk_list[player_ptr->index - 1].hooked)
	{
		g_ManiSMMHooks.HookProcessUsercmds((CBasePlayer *) EdictToCBE(player_ptr->entity));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiAFK::ClientDisconnect(player_t	*player_ptr)
{
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS) == -1) return;

	if (afk_list[player_ptr->index - 1].hooked)
	{
		g_ManiSMMHooks.UnHookProcessUsercmds((CBasePlayer *) EdictToCBE(player_ptr->entity));
		afk_list[player_ptr->index - 1].hooked = false;
	}

	this->ResetPlayer(player_ptr->index - 1, false);
}

//---------------------------------------------------------------------------------
// Purpose: Level Shutdown
//---------------------------------------------------------------------------------
void	ManiAFK::LevelShutdown(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		if (afk_list[i].hooked)
		{
			player_t player;

			player.index = i + 1;
			if (FindPlayerByIndex(&player))
			{
				g_ManiSMMHooks.UnHookProcessUsercmds((CBasePlayer *) EdictToCBE(player.entity));
			}
		}

		this->ResetPlayer(i, false);
		afk_list[i].hooked = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: External modules can call this to indicate a player is not AFK
//---------------------------------------------------------------------------------
void ManiAFK::NotAFK(int index)
{
	time_t current_time;

	time(&current_time);

	afk_list[index].idle = false;
	afk_list[index].last_active = current_time;
	afk_list[index].round_count = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Process a players death
//---------------------------------------------------------------------------------
void ManiAFK::ProcessUsercmds
(
 CBasePlayer *pPlayer, CUserCmd *cmds, int numcmds
 )
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;

	if (!pPlayer) return;
	edict_t *pEdict = serverents->BaseEntityToEdict((CBaseEntity *) pPlayer);
	if (!pEdict) return;

#if defined ( GAME_CSGO )
	int index = IndexOfEdict(pEdict);
#else
	int index = engine->IndexOfEdict(pEdict);
#endif
	if (index < 1 || index > max_players) return;

	if (cmds && numcmds && 
		(cmds->forwardmove || cmds->sidemove || cmds->upmove || cmds->mousedx || cmds->mousedy)) 
	{
		index --;
		time_t current_time;
		time(&current_time);
		afk_list[index].last_active = current_time;
		afk_list[index].idle = false;
		afk_list[index].round_count = 0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do timed afk if needed
//---------------------------------------------------------------------------------
void ManiAFK::GameFrame(void)
{
	if (war_mode) return;
	if (mani_afk_kicker.GetInt() == 0) return;
	if (mani_afk_kicker_alive_timer.GetInt() == 0 && mani_afk_kicker_spectator_timer.GetInt() == 0) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS) == -1) return;

	if (g_RealTime < next_check) return;

	next_check = g_RealTime + 1;

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
		if (!afk_list[i].check_player) continue;

		// Faster way of discriminating hopefully without getting player data
		// We are in game frame dont forget :)
		if (afk_list[i].last_active + max_timeout > g_RealTime) continue;

		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			this->ResetPlayer(i, false);
			continue;
		}

		if (player.is_bot) continue;

		if (gpManiGameType->IsValidActiveTeam(player.team) &&
			mani_afk_kicker_alive_timer.GetInt() != 0)
		{

			// Player on active team (considered alive)
			if (afk_list[i].last_active + mani_afk_kicker_alive_timer.GetInt() <= g_RealTime)
			{
				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
				{
					this->ResetPlayer(i, true);
					if (mani_afk_kicker_immunity_to_spec_only.GetInt() == 1)
					{
						// Shift player to spectator
						player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
						//engine->ClientCommand(player.entity, "cmd jointeam %i", gpManiGameType->GetSpectatorIndex());
						SayToPlayer(GREEN_CHAT, &player, "You were moved to the Spectator team for being AFK");
						LogCommand(NULL, "AFK-Kicker moved player [%s] [%s] to Spectator\n", player.name, player.steam_id);
						continue;
					}

					continue;
				}

				// Exceeded timeout
				this->ResetPlayer(i, true);
				if (mani_afk_kicker_mode.GetInt() == 0)
				{
					// Shift player to spectator
					player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
					//engine->ClientCommand(player.entity, "cmd jointeam %i", gpManiGameType->GetSpectatorIndex());
					SayToPlayer(GREEN_CHAT, &player, "You were moved to the Spectator team for being AFK");
					LogCommand(NULL, "AFK-Kicker moved player [%s] [%s] to Spectator\n", player.name, player.steam_id);
				}
				else
				{
					// Need to kick player
					SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
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
			if (afk_list[i].last_active + mani_afk_kicker_spectator_timer.GetInt() <= g_RealTime)
			{
				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
				{
					this->ResetPlayer(i, true);
					continue;
				}

				// Exceeded round count
				this->ResetPlayer(i, false);

				// Need to kick player
				SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
				UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
					"You were automatically kicked for being AFK", 
					"Auto-AFK kicked");
			}
		}
		else if (player.team == 0 && 
			gpManiGameType->IsTeamPlayAllowed() &&
			mani_afk_kicker_spectator_timer.GetInt() != 0 &&
			afk_list[i].last_active + mani_afk_kicker_spectator_timer.GetInt() <= g_RealTime)
		{
			if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
			{
				this->ResetPlayer(i, true);
				continue;
			}

			// Exceeded round count
			this->ResetPlayer(i, false);

			// Need to kick player
			SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
			UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
				"You were automatically kicked for being AFK", 
				"Auto-AFK kicked");
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
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_USER_CMDS) == -1) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	for (int i = 0; i < max_players; i++)
	{
		if (!afk_list[i].check_player) continue;
		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			this->ResetPlayer(i, false);
			continue;
		}

		if (player.is_bot) continue;

		if (afk_list[i].idle)
		{
			afk_list[i].round_count++;

			if (gpManiGameType->IsValidActiveTeam(player.team) &&
				mani_afk_kicker_alive_rounds.GetInt() != 0)
			{

				// Player on active team (considered alive)
				if (afk_list[i].round_count > mani_afk_kicker_alive_rounds.GetInt())
				{
					if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
					{
						this->ResetPlayer(i, true);
						if (mani_afk_kicker_immunity_to_spec_only.GetInt() == 1)
						{
							// Shift player to spectator
							player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
							//engine->ClientCommand(player.entity, "cmd jointeam %i", gpManiGameType->GetSpectatorIndex());
							SayToPlayer(GREEN_CHAT, &player, "You were moved to the Spectator team for being AFK");
							LogCommand(NULL, "AFK-Kicker moved player [%s] [%s] to Spectator\n", player.name, player.steam_id);
							continue;
						}

						continue;
					}

					// Exceeded round count
					this->ResetPlayer(i, true);
					if (mani_afk_kicker_mode.GetInt() == 0)
					{
						// Shift player to spectator
						player.player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());
						//engine->ClientCommand(player.entity, "cmd jointeam %i", gpManiGameType->GetSpectatorIndex());
						SayToPlayer(GREEN_CHAT, &player, "You were moved to the Spectator team for being AFK");
						LogCommand(NULL, "AFK-Kicker moved player [%s] [%s] to Spectator\n", player.name, player.steam_id);
					}
					else
					{
						// Need to kick player
						SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
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
					if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
					{
						this->ResetPlayer(i, true);
						continue;
					}

					// Exceeded round count
					this->ResetPlayer(i, false);

					// Need to kick player
					SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
					UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
											  "You were automatically kicked for being AFK", 
											  "Auto-AFK kicked");
				}
			}
			else if (player.team == 0 &&
				gpManiGameType->IsTeamPlayAllowed() &&
				mani_afk_kicker_spectator_rounds.GetInt() != 0 &&
				afk_list[i].round_count > mani_afk_kicker_spectator_rounds.GetInt())
			{
				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_AFK))
				{
					this->ResetPlayer(i, true);
					continue;
				}

				// Exceeded round count
				this->ResetPlayer(i, false);

				// Need to kick player
				SayToPlayer(GREEN_CHAT, &player, "You have been kicked for being AFK");
				UTIL_KickPlayer(&player, "Auto-kicked for being AFK", 
					"You were automatically kicked for being AFK", 
					"Auto-AFK kicked");
			}
		}
	}

	for (int i = 0; i < max_players; i++)
	{
		if (!afk_list[i].check_player) continue;
		afk_list[i].idle = true;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset a player on the afk list
//---------------------------------------------------------------------------------
void ManiAFK::ResetPlayer(int index, bool check_player)
{
	time_t current_time;
	time(&current_time);

	afk_list[index].check_player = check_player;
	afk_list[index].idle = true;
	afk_list[index].last_active = current_time;
	afk_list[index].round_count = 0;
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

CONVAR_CALLBACK_FN(ManiAFKKicker)
{
	if (!FStrEq(pOldString, mani_afk_kicker.GetString()))
	{
		gpManiAFK->Load();
	}
}

ManiAFK	g_ManiAFK;
ManiAFK	*gpManiAFK;
