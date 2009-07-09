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
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
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
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_util.h"
#include "mani_warmuptimer.h"
#include "mani_team.h"
#include "mani_teamkill.h"
#include "mani_commands.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;
extern	float		end_spawn_protection_time;
extern	int			round_number;

// Required to ensure when user executes
// a forgive/slay command via the menu that
// it is legitimate
struct	tk_confirm_t
{
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	name[MAX_PLAYER_NAME_LENGTH];
	int		user_id;
	bool	confirmed;
};

tk_player_t	*tk_player_list = NULL;
static	tk_bot_t tk_bot_list[MANI_MAX_PUNISHMENTS];
//static	tk_confirm_t tk_confirm_list[MANI_MAX_PLAYERS];

int	tk_player_list_size = 0;

static	void	ProcessPlayerNotOnServer(int	punishment,  player_t *victim_ptr,  player_t *attacker_ptr, bool	bot_calling );
static	void	ProcessDelayedPunishment( int	punishment,  player_t	*victim_ptr,  player_t	*attacker_ptr, char *say_string, bool	bot_calling );
static	void	ProcessImmediatePunishment( int	punishment,  player_t *victim_ptr,  player_t *attacker_ptr, char *say_string, char *log_string, bool	bot_calling );
static	void	ProcessTKPunishment( int punishment, player_t *attacker_ptr, player_t *victim_ptr,  char *say_string, char *log_string, bool just_spawned);
static	bool	GetTKPunishSayString( int	punishment,  player_t	*attacker_ptr,  player_t	*victim_ptr,  char	*output_string,  char *log_string, bool	future_tense );
static	bool	GetTKAutoPunishSayString( int	punishment,  player_t	*attacker_ptr, char	*output_string, char	*log_string );
static	bool	NeedToIncrementViolations( player_t	*victim_ptr,  int punishment );
static	void	ProcessTKNoForgiveMode( player_t *attacker_ptr,  player_t *victim_ptr );
static	int		GetRandomTKPunishmentAgainstBot(void);
static	int		GetRandomTKPunishmentAgainstHuman(void);
static	bool	IsMenuOptionAllowed( int punishment, bool is_bot);

static	int	current_round;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//---------------------------------------------------------------------------------
// Purpose: Setup the punishments that bots can give and take
//---------------------------------------------------------------------------------
void	InitTKPunishments (void)
{
	tk_bot_list[MANI_TK_SLAY].bot_can_give = true;
	tk_bot_list[MANI_TK_SLAY].bot_can_take = true;
	tk_bot_list[MANI_TK_SLAY].punish_cvar_ptr = &mani_tk_allow_slay_option;

	tk_bot_list[MANI_TK_SLAP].bot_can_give = gpManiGameType->IsSlapAllowed();
	tk_bot_list[MANI_TK_SLAP].bot_can_take = gpManiGameType->IsSlapAllowed();;
	tk_bot_list[MANI_TK_SLAP].punish_cvar_ptr = &mani_tk_allow_slap_option;

	tk_bot_list[MANI_TK_BLIND].bot_can_give = true;
	tk_bot_list[MANI_TK_BLIND].bot_can_take = false;
	tk_bot_list[MANI_TK_BLIND].punish_cvar_ptr = &mani_tk_allow_blind_option;

	tk_bot_list[MANI_TK_FREEZE].bot_can_give = true;
	tk_bot_list[MANI_TK_FREEZE].bot_can_take = true;
	tk_bot_list[MANI_TK_FREEZE].punish_cvar_ptr = &mani_tk_allow_freeze_option;

	tk_bot_list[MANI_TK_CASH].bot_can_give = true;
	tk_bot_list[MANI_TK_CASH].bot_can_take = true;
	tk_bot_list[MANI_TK_CASH].punish_cvar_ptr = &mani_tk_allow_cash_option;

	tk_bot_list[MANI_TK_BURN].bot_can_give = gpManiGameType->IsFireAllowed();
	tk_bot_list[MANI_TK_BURN].bot_can_take = gpManiGameType->IsFireAllowed();
	tk_bot_list[MANI_TK_BURN].punish_cvar_ptr = &mani_tk_allow_burn_option;

	tk_bot_list[MANI_TK_FORGIVE].bot_can_give = true;
	tk_bot_list[MANI_TK_FORGIVE].bot_can_take = true;
	tk_bot_list[MANI_TK_FORGIVE].punish_cvar_ptr = &mani_tk_allow_forgive_option;

	tk_bot_list[MANI_TK_DRUG].bot_can_give = gpManiGameType->IsDrugAllowed();
	tk_bot_list[MANI_TK_DRUG].bot_can_take = false;
	tk_bot_list[MANI_TK_DRUG].punish_cvar_ptr = &mani_tk_allow_drugged_option;

	tk_bot_list[MANI_TK_TIME_BOMB].bot_can_give = true;
	tk_bot_list[MANI_TK_TIME_BOMB].bot_can_take = true;
	tk_bot_list[MANI_TK_TIME_BOMB].punish_cvar_ptr = &mani_tk_allow_time_bomb_option;

	tk_bot_list[MANI_TK_FIRE_BOMB].bot_can_give = gpManiGameType->IsFireAllowed();
	tk_bot_list[MANI_TK_FIRE_BOMB].bot_can_take = gpManiGameType->IsFireAllowed();
	tk_bot_list[MANI_TK_FIRE_BOMB].punish_cvar_ptr = &mani_tk_allow_fire_bomb_option;

	tk_bot_list[MANI_TK_FREEZE_BOMB].bot_can_give = true;
	tk_bot_list[MANI_TK_FREEZE_BOMB].bot_can_take = true;
	tk_bot_list[MANI_TK_FREEZE_BOMB].punish_cvar_ptr = &mani_tk_allow_freeze_bomb_option;

	if (gpManiGameType->GetAdvancedEffectsAllowed())
	{
		tk_bot_list[MANI_TK_BEACON].bot_can_give = true;
		tk_bot_list[MANI_TK_BEACON].bot_can_take = true;
		tk_bot_list[MANI_TK_BEACON].punish_cvar_ptr = &mani_tk_allow_beacon_option;
	}
	else
	{
		tk_bot_list[MANI_TK_BEACON].bot_can_give = false;
		tk_bot_list[MANI_TK_BEACON].bot_can_take = false;
		tk_bot_list[MANI_TK_BEACON].punish_cvar_ptr = &mani_tk_allow_beacon_option;
	}

	current_round = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Process Rounds End event
//---------------------------------------------------------------------------------
void	TKRoundStart (void)
{
	current_round ++;
}

//---------------------------------------------------------------------------------
// Purpose: Free the tk punishments list
//---------------------------------------------------------------------------------
void	FreeTKPunishments (void)
{
	FreeList((void **) &tk_player_list, &tk_player_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Handle a punishment to dish out at spawn time
//---------------------------------------------------------------------------------
void	ProcessTKSpawnPunishment
(
 player_t *player_ptr
 )
{
	char		log_string[512];
	char		output_string[512];

	int	tk_index = -1;
	int	tk_punishment = -1;

	if (war_mode) return;

	// Don't bother with spectators
	if (gpManiGameType->IsSpectatorAllowed() && gpManiGameType->GetSpectatorIndex() == player_ptr->team) return;
	if (FStrEq(player_ptr->steam_id, "STEAM_ID_PENDING")) return;

	// Get all players on list who need slaying
	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (tk_player_list[i].user_id == player_ptr->user_id)
		{
			// Found user id match
			tk_index = i;
			break;
		}

		if (player_ptr->is_bot) continue;
		if (IsLAN()) continue;

		// Only human players on web
		if (FStrEq(tk_player_list[i].steam_id, player_ptr->steam_id))
		{
			tk_index = i;
			break;
		}
	}

	if (tk_index == -1) return;

	for (int i = 0; i < MANI_MAX_PUNISHMENTS; i++)
	{
		if (tk_player_list[tk_index].punishment[i] > 0)
		{
			if (!GetTKAutoPunishSayString(i, player_ptr, output_string, log_string))
			{
				// Bad punishment so reset
				tk_player_list[tk_index].punishment[i] = 0;
				continue;
			}

			tk_punishment = i;
			break;
		}
	}

	// No punishment required
	if (tk_punishment == -1) return;
	ProcessTKPunishment(tk_punishment, player_ptr, player_ptr, output_string, log_string, true);

	// Reset punishments, special case for TK Slay
	for (int i = 0; i < MANI_MAX_PUNISHMENTS; i++)
	{
		if (i == MANI_TK_SLAY)
		{
			tk_player_list[tk_index].rounds_to_miss --;
			if (tk_player_list[tk_index].rounds_to_miss <= 0)
			{
				tk_player_list[tk_index].punishment[tk_punishment] = 0;
			}
		}
		else
		{
			tk_player_list[tk_index].punishment[tk_punishment] = 0;
		}
	}


}

//---------------------------------------------------------------------------------
// Purpose: Handle a selected punishment from the menu system, or a bot 
//          created punishment. Attacking player may be still alive, dead,
//          moved to observer team or left the server.
//---------------------------------------------------------------------------------
void	ProcessTKSelectedPunishment
(
 int	punishment, 
 player_t *victim_ptr, 
 const int	attacker_user_id, 
 char	*attacker_steam_id, 
 bool	bot_calling // Bot called for this punishment
 )
{
	player_t	attacker;
	char		log_string[512];
	char		output_string[512];

	if (war_mode) return;

	// Check user allowed to select punishment
	if (!IsMenuOptionAllowed(punishment, bot_calling))
	{
		return;
	}


	attacker.user_id = attacker_user_id;
	Q_strcpy (attacker.steam_id, attacker_steam_id);

	// Check if player found on server
	if (!FindPlayerByUserID(&attacker))
	{
		// Player is not on the server so we need a delayed
		// action
		ProcessPlayerNotOnServer(punishment, victim_ptr, &attacker, bot_calling);
		return;
	}


	// Check if attacking player is an observer or is dead
	if (attacker.player_info->IsObserver() || attacker.is_dead)
	{
		// Attacker is either dead or has moved to
		// the spectating team to avoid punishment
		if (GetTKPunishSayString(punishment, &attacker, victim_ptr, output_string, log_string, true))
		{
			// We can delay the punishment until player spawns
			ProcessDelayedPunishment(punishment, victim_ptr, &attacker, output_string, bot_calling);
			return;
		}
	}

	// If here then attacker is either alive and well on the server
	// or GetTKPunishment returned false meaning that the punishment
	// can be applied whatever state the player is in
	GetTKPunishSayString(punishment, &attacker, victim_ptr, output_string, log_string, false);
	ProcessImmediatePunishment (punishment, victim_ptr, &attacker, output_string, log_string, bot_calling);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle a punishment delivered to a player not on the server
//---------------------------------------------------------------------------------
static
void	ProcessPlayerNotOnServer
(
 int	punishment, 
 player_t *victim_ptr, 
 player_t *attacker_ptr,
 bool	bot_calling
 )
{
	// Bot should not leave a server
	if (FStrEq("BOT", attacker_ptr->steam_id)) return;
	if (bot_calling) return;

	// If we are in lan mode we can't track using steam id
	if (IsLAN()) return;

	// Could not find attacker on server, doesn't mean he
	// doesn't exist in the tk list !!
	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (FStrEq(tk_player_list[i].steam_id, attacker_ptr->steam_id))
		{
			if (punishment == MANI_TK_SLAY)
			{
				tk_player_list[i].rounds_to_miss ++;
			}

			// Set punishment delayed required
			tk_player_list[i].punishment[punishment] = 1;

			if (NeedToIncrementViolations(victim_ptr, punishment))
			{
				tk_player_list[i].violations_committed ++;
			}

			// Check if ban required
			TKBanPlayer (attacker_ptr, i);
			return;
		}
	}

	// Player not found in tk list so add them
	CreateNewTKPlayer("", attacker_ptr->steam_id, attacker_ptr->user_id, mani_tk_add_violation_without_forgive.GetInt(), 0);
	TKBanPlayer (attacker_ptr, tk_player_list_size - 1);
	
	if (punishment == MANI_TK_SLAY)
	{
		tk_player_list[tk_player_list_size - 1].rounds_to_miss ++;
	}

	// Set punishment delayed required
	tk_player_list[tk_player_list_size - 1].punishment[punishment] = 1;

	if (NeedToIncrementViolations(victim_ptr, punishment))
	{
		tk_player_list[tk_player_list_size - 1].violations_committed ++;
	}

	// Check if ban required
	TKBanPlayer (attacker_ptr, tk_player_list_size - 1);

	return;
}


//---------------------------------------------------------------------------------
// Purpose: Handle a punishment delivered to a player that should be delayed until
//          next spawn time
//---------------------------------------------------------------------------------
static
void	ProcessDelayedPunishment
(
 int	punishment, 
 player_t	*victim_ptr, 
 player_t	*attacker_ptr,
 char		*say_string,
 bool	bot_calling
 )
{

	if (attacker_ptr->is_bot && !tk_bot_list[punishment].bot_can_take) return;

	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), attacker_ptr))
		{
			if (punishment == MANI_TK_SLAY)
			{
				tk_player_list[i].rounds_to_miss ++;
			}

			// Set punishment delayed required
			tk_player_list[i].punishment[punishment] = 1;

			if (NeedToIncrementViolations(victim_ptr, punishment))
			{
				tk_player_list[i].violations_committed ++;
			}

			// Check if ban required
			if (!TKBanPlayer (attacker_ptr, i))
			{
				SayToAll(ORANGE_CHAT, true, say_string);
			}
			return;
		}
	}

	// Player not found in tk list so add them
	CreateNewTKPlayer(attacker_ptr->name, attacker_ptr->steam_id, attacker_ptr->user_id, 0, 0);
	if (NeedToIncrementViolations(victim_ptr, punishment))
	{
		tk_player_list[tk_player_list_size - 1].violations_committed ++;
	}	
	
	if (punishment == MANI_TK_SLAY)
	{
		tk_player_list[tk_player_list_size - 1].rounds_to_miss ++;
	}

	// Set punishment delayed required
	tk_player_list[tk_player_list_size - 1].punishment[punishment] = 1;
	// Check if ban required
	if (!TKBanPlayer (attacker_ptr, tk_player_list_size - 1))
	{
		SayToAll(ORANGE_CHAT, true, say_string);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle the punishment now
//---------------------------------------------------------------------------------
static
void	ProcessImmediatePunishment
(
 int	punishment, 
 player_t *victim_ptr, 
 player_t *attacker_ptr,
 char	*say_string,
 char	*log_string,
 bool	bot_calling
 )
{
	if (attacker_ptr->is_bot && !tk_bot_list[punishment].bot_can_take) return;

	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (attacker_ptr->is_bot)
		{
			if (tk_player_list[i].user_id == attacker_ptr->user_id)
			{
				// Dont care about ban for bot
				ProcessTKPunishment(punishment, attacker_ptr, victim_ptr, say_string, log_string, false);
				return;
			}
		}
		else if (IsTKPlayerMatch(&(tk_player_list[i]), attacker_ptr))
		{
			if (NeedToIncrementViolations(victim_ptr, punishment))
			{
				tk_player_list[i].violations_committed ++;
			}

			// Check if ban required
			if (TKBanPlayer (attacker_ptr, i)) return;

			ProcessTKPunishment(punishment, attacker_ptr, victim_ptr, say_string, log_string, false);
			return;
		}
	}

	// Player not found in tk list so add if attacker is not a bot
	CreateNewTKPlayer(attacker_ptr->name, attacker_ptr->steam_id, attacker_ptr->user_id, 1, 0);

	// Check if ban required
	if (TKBanPlayer (attacker_ptr, tk_player_list_size - 1))
	{
		return;
	}

	ProcessTKPunishment(punishment, attacker_ptr, victim_ptr, say_string, log_string, false);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle the immediate mode punishments
//---------------------------------------------------------------------------------
static
void		ProcessTKPunishment
(
 int		punishment, 
 player_t	*attacker_ptr, 
 player_t	*victim_ptr, 
 char		*say_string,
 char		*log_string,
 bool		just_spawned
 )
{
	if (punishment == MANI_TK_SLAY)
	{
		SlayPlayer(attacker_ptr, true, true, true);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_SLAP)
	{
		ProcessSlapPlayer(attacker_ptr, attacker_ptr->health - mani_tk_slap_to_damage.GetInt(), false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_BLIND)
	{
		BlindPlayer(attacker_ptr, mani_tk_blind_amount.GetInt());
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_FREEZE)
	{
		ProcessFreezePlayer(attacker_ptr, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_CASH)
	{
		ProcessTakeCash(attacker_ptr, victim_ptr);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_BURN)
	{
		ProcessBurnPlayer(attacker_ptr, mani_tk_burn_time.GetInt());
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_FORGIVE)
	{
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		return;
	}
	else if (punishment == MANI_TK_DRUG)
	{
		ProcessDrugPlayer(attacker_ptr, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_TIME_BOMB)
	{
		ProcessTimeBombPlayer(attacker_ptr, just_spawned, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_FIRE_BOMB)
	{
		ProcessFireBombPlayer(attacker_ptr, just_spawned, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_FREEZE_BOMB)
	{
		ProcessFreezeBombPlayer(attacker_ptr, just_spawned, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}
	else if (punishment == MANI_TK_BEACON)
	{
		ProcessBeaconPlayer(attacker_ptr, false);
		SayToAll (ORANGE_CHAT, true, "%s", say_string);
		DirectLogCommand("%s", log_string);
		return;
	}}

//---------------------------------------------------------------------------------
// Purpose: Get the string we need to show all players for the punishment
//---------------------------------------------------------------------------------
static
bool	NeedToIncrementViolations
(
 player_t	*victim_ptr, 
 int		punishment
 )
{
	if (victim_ptr->is_bot)
	{
		// This is a bot inflicting vengence
		if (mani_tk_add_violation_without_forgive.GetInt() == 0)
		{
			if (punishment == MANI_TK_FORGIVE)
			{
				return false;
			}
		}

		if (mani_tk_allow_bots_to_add_violations.GetInt() == 1)
		{
			return true;
		}
	}
	else
	{
		// Regular player
		// This is a bot inflicting vengence
		if (mani_tk_add_violation_without_forgive.GetInt() == 0)
		{
			if (punishment == MANI_TK_FORGIVE)
			{
				return false;
			}

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Get the string we need to show all players for the punishment
//---------------------------------------------------------------------------------
static
bool	GetTKPunishSayString
(
 int	punishment, 
 player_t	*attacker_ptr, 
 player_t	*victim_ptr, 
 char	*output_string, 
 char	*log_string,
 bool	future_tense
 )
{
	if (future_tense)
	{
		Q_strcpy(log_string,"");
		switch (punishment)
		{
		case MANI_TK_SLAY: snprintf(output_string, 512, "Player %s will be slayed for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_SLAP: snprintf(output_string, 512, "Player %s will be slapped for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_BLIND: snprintf(output_string, 512, "Player %s will be blinded for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_FREEZE: snprintf(output_string, 512, "Player %s will be frozen for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_CASH: return false; // No future tense needed
		case MANI_TK_BURN: snprintf(output_string, 512, "Player %s will be burned for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_DRUG: snprintf(output_string, 512, "Player %s will be drugged for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_TIME_BOMB: snprintf(output_string, 512, "Player %s will be turned into a time bomb for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_FIRE_BOMB: snprintf(output_string, 512, "Player %s will be turned into a fire bomb for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_FREEZE_BOMB: snprintf(output_string, 512, "Player %s will be turned into a freeze bomb for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_BEACON: snprintf(output_string, 512, "Player %s will be turned into a beacon for team killing %s", attacker_ptr->name, victim_ptr->name); break;
		case MANI_TK_FORGIVE: return false; // No future tense needed
		default: break;
		}
	}
	else
	{
		switch (punishment)
		{
		case MANI_TK_SLAY: snprintf(output_string, 512, "Player %s has been slayed for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection slayed user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_SLAP: snprintf(output_string, 512, "Player %s has been slapped for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection slapped user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_BLIND: snprintf(output_string, 512, "Player %s has been blinded for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection blinded user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_FREEZE: snprintf(output_string, 512, "Player %s has been frozen for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection froze user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_CASH: snprintf(output_string, 512, "Player %s has taken %i percent cash from %s", victim_ptr->name, mani_tk_cash_percent.GetInt(), attacker_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection took cash from user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_BURN: snprintf(output_string, 512, "Player %s has been burned for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection burned user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_DRUG: snprintf(output_string, 512, "Player %s has been drugged for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection drugged user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_TIME_BOMB: snprintf(output_string, 512, "Player %s has been turned into a time bomb for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection time bombed user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_FIRE_BOMB: snprintf(output_string, 512, "Player %s has been turned into a fire bomb for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection fire bombed user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_FREEZE_BOMB: snprintf(output_string, 512, "Player %s has been turned into a freeze bomb for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection freeze bombed user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_BEACON: snprintf(output_string, 512, "Player %s has been turned into a beacon for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection beaconed user [%s] steam id [%s] for team killing user [%s] steam id [%s]\n", attacker_ptr->name, attacker_ptr->steam_id, victim_ptr->name, victim_ptr->steam_id);
							break;

		case MANI_TK_FORGIVE: snprintf(output_string, 512, "Player %s has been forgiven for team killing %s", attacker_ptr->name, victim_ptr->name);
							snprintf(log_string, 512, "");
							break;

		default: break;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Get the string we need to show all players for the auto punishment
//---------------------------------------------------------------------------------
static
bool	GetTKAutoPunishSayString
(
 int	punishment, 
 player_t	*attacker_ptr,
 char	*output_string,
 char	*log_string
 )
{

	switch (punishment)
	{
	case MANI_TK_SLAY: snprintf(output_string, 512, "Player %s has been slayed for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto slayed user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_SLAP: snprintf(output_string, 512, "Player %s has been slapped for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto slapped user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_BLIND: snprintf(output_string, 512, "Player %s has been blinded for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto blinded user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_FREEZE: snprintf(output_string, 512, "Player %s has been frozen for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto froze user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_BURN: snprintf(output_string, 512, "Player %s has been burned for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto burned user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_DRUG: snprintf(output_string, 512, "Player %s has been drugged for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto drugged user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_TIME_BOMB: snprintf(output_string, 512, "Player %s has been turned into a time bomb for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto time bombed user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_FIRE_BOMB: snprintf(output_string, 512, "Player %s has been turned into a fire bomb for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto fire bombed user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_FREEZE_BOMB: snprintf(output_string, 512, "Player %s has been turned into a freeze bomb for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto freeze bombed user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	case MANI_TK_BEACON: snprintf(output_string, 512, "Player %s has been turned into a beacon for a previous team killing violation", attacker_ptr->name);
						snprintf(log_string, 512, "[MANI_ADMIN_PLUGIN] TK Protection auto beaconed user [%s] steam id [%s] for team killing\n", attacker_ptr->name, attacker_ptr->steam_id);
						break;

	default: return false;
			break;
	}

	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Check and ban player if tk offences are too high 
//---------------------------------------------------------------------------------
bool	TKBanPlayer (player_t	*attacker, int ban_index)
{
	player_t	bot;
	
	if (war_mode)
	{
		return false;
	}
	
	// Don't bother banning if on lan
	if (IsLAN()) return false;

	// Make sure index is within bounds
	if (tk_player_list_size >= ban_index && ban_index >= 0)
	{
		if (tk_player_list[ban_index].violations_committed >= mani_tk_offences_for_ban.GetInt() && mani_tk_offences_for_ban.GetInt() != 0)
		{
			// Ban required
			char	ban_cmd[128];

			Q_strcpy(bot.steam_id, attacker->steam_id);
			if (FindPlayerBySteamID(&bot))
			{
				if (bot.is_bot)
				{
					return false;
				}
			
				if (mani_tk_ban_time.GetInt() == 0)
				{
					// Permanent ban
					PrintToClientConsole(bot.entity, "You have been banned permanently for team killing !!\n"); 
				}
				else
				{
					// X minute ban
					PrintToClientConsole(bot.entity, "You have been banned for %i minutes for team killing !!\n", mani_tk_ban_time.GetInt());
				}
			}

			snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %s kick\n", mani_tk_ban_time.GetInt(), attacker->steam_id);

			engine->ServerCommand(ban_cmd);
			engine->ServerCommand("writeid\n");

			if (mani_tk_ban_time.GetInt() == 0)
			{
				DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection banned steam id [%s] permanently for team killing\n", attacker->steam_id);
			}
			else
			{
				DirectLogCommand("[MANI_ADMIN_PLUGIN] TK Protection banned steam id [%s] for %i minutes for team killing\n", attacker->steam_id, mani_tk_ban_time.GetInt());
			}

			tk_player_list[ban_index].violations_committed = 0;
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: CreateNewTKPlayer
//---------------------------------------------------------------------------------

void	CreateNewTKPlayer
(
 char	*name,
 char	*steam_id,
 int	user_id,
 int	violations,
 int	rounds_to_miss
)
{
	AddToList((void **) &tk_player_list, sizeof(tk_player_t), &tk_player_list_size);
	Q_strcpy(tk_player_list[tk_player_list_size - 1].steam_id, steam_id);
	Q_strcpy(tk_player_list[tk_player_list_size - 1].name, name);
	tk_player_list[tk_player_list_size - 1].user_id = user_id;
	tk_player_list[tk_player_list_size - 1].violations_committed = violations;
	tk_player_list[tk_player_list_size - 1].last_round_violation = round_number;
	tk_player_list[tk_player_list_size - 1].spawn_violations_committed = 0;
	tk_player_list[tk_player_list_size - 1].rounds_to_miss = rounds_to_miss;
	tk_player_list[tk_player_list_size - 1].team_wounds = 0;
	tk_player_list[tk_player_list_size - 1].team_wound_reflect_ratio = mani_tk_team_wound_reflect_ratio.GetFloat();

	for (int i = 0; i < MANI_MAX_PUNISHMENTS; i++)
	{
		tk_player_list[tk_player_list_size - 1].punishment[i] = 0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: IsTKPlayerMatch
//---------------------------------------------------------------------------------
bool	IsTKPlayerMatch
(
 tk_player_t	*tk_player,
 player_t		*player
)
{

	if (IsLAN())
	{
		// Lan mode
		if (tk_player->user_id == player->user_id)
		{
			return true;
		}
	}
	else
	{
		// Steam Mode
		if (FStrEq(tk_player->steam_id, player->steam_id))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Handle a player death event
//---------------------------------------------------------------------------------
bool ProcessTKDeath
(
 player_t	*attacker_ptr,
 player_t	*victim_ptr
 )
{
	if (mani_tk_protection.GetInt() == 0) return false;

	// Check TK happened outside spawn protection time
	if (gpGlobals->curtime < end_spawn_protection_time)	return false;

	// Killed by world ?
	if (attacker_ptr->user_id <= 0) return false;

	// Are players on same team ?
	if (!gpManiTeam->IsOnSameTeam (victim_ptr, attacker_ptr))	return false;

	if (mani_tk_forgive.GetInt() == 0)
	{
		ProcessTKNoForgiveMode(attacker_ptr, victim_ptr);
		return false;
	}

	int tk_player_index = -1;

	// Find tkplayer list and create tracking data
	if (!victim_ptr->is_bot && !attacker_ptr->is_bot)
	{
		for (int i = 0; i < tk_player_list_size; i++)
		{
			if (IsLAN())
			{
				if (attacker_ptr->user_id == tk_player_list[i].user_id)
				{
					tk_player_index = i;
					break;
				}
			}
			else if (FStrEq(attacker_ptr->steam_id, tk_player_list[i].steam_id))
			{
				tk_player_index = i;
				break;
			}
		}	

		// TK/TW occured (both on same team)
		// Handle tracking of this player

		if (tk_player_index != -1)
		{
			// If a new round then bump the violations
			tk_player_list[tk_player_index].last_round_violation = round_number;

			if (!attacker_ptr->is_bot)
			{
				if (mani_tk_add_violation_without_forgive.GetInt() == 1)
				{
					tk_player_list[tk_player_index].violations_committed++;
					Q_strcpy(tk_player_list[tk_player_index].name, attacker_ptr->name);
					// Check if ban required
					if (TKBanPlayer (attacker_ptr, tk_player_index))
					{
						return false;
					}
				}
			}
		}
		else
		{
			// Add this player to list
			CreateNewTKPlayer(attacker_ptr->name, attacker_ptr->steam_id, attacker_ptr->user_id, 0, 0);
			// Check if ban required

			if (!attacker_ptr->is_bot && mani_tk_add_violation_without_forgive.GetInt() == 1)
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 1;
				if(TKBanPlayer (attacker_ptr, tk_player_list_size -1))
				{
					return false;
				}
			}
			else
			{
				tk_player_list[tk_player_list_size - 1].violations_committed = 0;
			}
		}
	}

	// No point showing to menu to player
	if (victim_ptr->entity == NULL) return false;
	if (victim_ptr->is_bot && mani_tk_allow_bots_to_punish.GetInt() == 0)
	{
		// Dont't let bots punish players
		return false;
	}

	int	 bot_choice;

	// Handle a bot dishing out punishments
	if (victim_ptr->is_bot)
	{
		if (attacker_ptr->is_dead) return false;

		if (attacker_ptr->is_bot)
		{
			bot_choice = GetRandomTKPunishmentAgainstBot();
		}
		else
		{
			bot_choice = GetRandomTKPunishmentAgainstHuman();
		}

		char	log_string[512];
		char	output_string[512];
		GetTKPunishSayString(bot_choice, attacker_ptr, victim_ptr, output_string, log_string, false);
		ProcessTKPunishment(bot_choice, attacker_ptr, victim_ptr, output_string, log_string, false);
		return false;
	}

	// Force client to call command to show menu
	g_menu_mgr.Kill(victim_ptr);
	MENUPAGE_CREATE_PARAM4(TKPlayerPage, 
							victim_ptr, 
							AddParam("is_bot", attacker_ptr->is_bot), 
							AddParam("user_id", attacker_ptr->user_id), 
							AddParam("steam_id", attacker_ptr->steam_id), 
							AddParam("name", attacker_ptr->name),
							2,
							-1);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle draw tk punishment menu
//---------------------------------------------------------------------------------
int TKPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	bool	is_bot;
	int		user_id;
	char	*steam_id;
	int		punish;

	if (!m_page_ptr->params.GetParam("is_bot", &is_bot)) return CLOSE_MENU;
	if (!m_page_ptr->params.GetParam("user_id", &user_id)) return CLOSE_MENU;
	if (!m_page_ptr->params.GetParam("steam_id", &steam_id)) return CLOSE_MENU;
	if (!this->params.GetParam("punish", &punish)) return CLOSE_MENU;

	ProcessTKSelectedPunishment(punish, player_ptr, user_id, steam_id, is_bot);
	return CLOSE_MENU;
}

bool TKPlayerPage::PopulateMenuPage(player_t *player_ptr)
{

	bool	is_bot;
	char	*name;

	this->params.GetParam("is_bot", &is_bot);
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 620));
	this->SetTitle("%s", Translate(player_ptr, 631,"%s", name));

	// Some people don't want the forgive option
	if (IsMenuOptionAllowed(MANI_TK_FORGIVE, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 632, AddParam("punish", MANI_TK_FORGIVE));
	}

	// Some people don't want the slay option
	if (IsMenuOptionAllowed(MANI_TK_SLAY, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 633, AddParam("punish", MANI_TK_SLAY));
	}

	// Some people don't want the slap option
	if (IsMenuOptionAllowed(MANI_TK_SLAP, is_bot) && gpManiGameType->IsSlapAllowed())
	{
		MenuItem *ptr = new TKPlayerItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 634, "%i", mani_tk_slap_to_damage.GetInt()));
		ptr->params.AddParam("punish", MANI_TK_SLAP);
		this->AddItem(ptr);
	}

	// Some people don't want the beacon option
	if (IsMenuOptionAllowed(MANI_TK_BEACON, is_bot) && gpManiGameType->GetAdvancedEffectsAllowed())
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 643, AddParam("punish", MANI_TK_BEACON));
	}

	// Some people don't want the time bomb option
	if (IsMenuOptionAllowed(MANI_TK_TIME_BOMB, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 640, AddParam("punish", MANI_TK_TIME_BOMB));
	}

	// Some people don't want the fire bomb option
	if (IsMenuOptionAllowed(MANI_TK_FIRE_BOMB, is_bot) && gpManiGameType->IsFireAllowed())
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 641, AddParam("punish", MANI_TK_FIRE_BOMB));
	}

	// Some people don't want the freeze bomb option
	if (IsMenuOptionAllowed(MANI_TK_FREEZE_BOMB, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 642, AddParam("punish", MANI_TK_FREEZE_BOMB));
	}

	// Some people don't want the freeze option
	if (IsMenuOptionAllowed(MANI_TK_FREEZE, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 636, AddParam("punish", MANI_TK_FREEZE));
	}

	// Some people don't want the burn option
	if (IsMenuOptionAllowed(MANI_TK_BURN, is_bot)  && gpManiGameType->IsFireAllowed())
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 639, AddParam("punish", MANI_TK_BURN));
	}

	// Some people don't want the cash option
	if (IsMenuOptionAllowed(MANI_TK_CASH, is_bot) && gpManiGameType->CanUseProp(MANI_PROP_ACCOUNT))
	{
		MenuItem *ptr = new TKPlayerItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 637, "%i", mani_tk_cash_percent.GetInt()));
		ptr->params.AddParam("punish", MANI_TK_CASH);
		this->AddItem(ptr);
	}

	// Some people don't want the drug option
	if (IsMenuOptionAllowed(MANI_TK_DRUG, is_bot) && gpManiGameType->IsDrugAllowed())
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 638, AddParam("punish", MANI_TK_DRUG));
	}

	// Some people don't want the blind option
	if (IsMenuOptionAllowed(MANI_TK_BLIND, is_bot))
	{
		MENUOPTION_CREATE_PARAM(TKPlayerItem, 635, AddParam("punish", MANI_TK_BLIND));
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Fetch a punishment id for a bot v bot
//---------------------------------------------------------------------------------
static
bool	IsMenuOptionAllowed
(
 int		punishment,
 bool		is_bot
 )
{
	if (is_bot)
	{
		if (tk_bot_list[punishment].bot_can_take &&
			tk_bot_list[punishment].punish_cvar_ptr->GetInt() == 1)
		{
			return true;
		}
	}
	else
	{
		if (tk_bot_list[punishment].punish_cvar_ptr->GetInt() == 1)
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Fetch a punishment id for a bot v bot
//---------------------------------------------------------------------------------
static
int		GetRandomTKPunishmentAgainstBot(void)
{
	int			build_index = 0;
	static		int		punishment_list[MANI_MAX_PUNISHMENTS];

	bool	found_forgive = false;

	for (int i = 0; i < MANI_MAX_PUNISHMENTS; i++)
	{
		if (tk_bot_list[i].bot_can_give &&
			tk_bot_list[i].bot_can_take &&
			tk_bot_list[i].punish_cvar_ptr->GetInt() == 1)
		{
			if (i != MANI_TK_FORGIVE)
			{
				punishment_list[build_index ++] = i;
			}
			else
			{
				found_forgive = true;
			}
		}
	}

	if (build_index == 0)
	{
		return MANI_TK_FORGIVE;
	}

	int choice;
	if (found_forgive)
	{
		choice = rand() % (build_index * 2);

		if (choice >= build_index)
		{
			return MANI_TK_FORGIVE;
		}
		
		return punishment_list[choice];
	}

	choice = rand() % build_index;
	return punishment_list[build_index];
}

//---------------------------------------------------------------------------------
// Purpose: Fetch a punishment id for a bot v human
//---------------------------------------------------------------------------------
static
int		GetRandomTKPunishmentAgainstHuman(void)
{
	int			build_index = 0;
	static		int		punishment_list[MANI_MAX_PUNISHMENTS];

	bool	found_forgive = false;

	for (int i = 0; i < MANI_MAX_PUNISHMENTS; i++)
	{
		if (tk_bot_list[i].bot_can_give &&
			tk_bot_list[i].punish_cvar_ptr->GetInt() == 1)
		{
			if (i != MANI_TK_FORGIVE)
			{
				punishment_list[build_index ++] = i;
			}
			else
			{
				found_forgive = true;
			}
		}
	}

	if (build_index == 0)
	{
		return MANI_TK_FORGIVE;
	}

	int choice;
	if (found_forgive)
	{
		choice = rand() % (build_index * 2);

		if (choice >= build_index)
		{
			return MANI_TK_FORGIVE;
		}
		
		return punishment_list[choice];
	}

	choice = rand() % build_index;
	return punishment_list[build_index];

}

//---------------------------------------------------------------------------------
// Purpose: Handle the custom no forgive menu mode
//---------------------------------------------------------------------------------
static
void	ProcessTKNoForgiveMode
(
 player_t	*attacker_ptr, 
 player_t	*victim_ptr
 )
{
	char	log_string[512];
	char	output_string[512];

	// Get translation
	GetTKPunishSayString(MANI_TK_SLAY, attacker_ptr, victim_ptr, output_string, log_string, false);

	if (attacker_ptr->is_bot)
	{
		// Don't bother tracking bot
		SlayPlayer(attacker_ptr, false, true, true);
		DirectLogCommand("%s", log_string);
		SayToAll (ORANGE_CHAT, false, "%s", output_string);
		return;
	}

	// Not in forgive mode so slay instantly
	int tk_player_index = -1;

	// Find tkplayer list
	for (int i = 0; i < tk_player_list_size; i++)
	{
		if (IsTKPlayerMatch(&(tk_player_list[i]), attacker_ptr))
		{
			tk_player_index = i;
			break;
		}
	}	

	// TK/TW occured (both on same team)
	// Handle tracking of this player

	if (tk_player_index != -1)
	{
		// If a new round then bump the violations
		Q_strcpy(tk_player_list[tk_player_index].name, attacker_ptr->name);
		tk_player_list[tk_player_index].violations_committed++;
		tk_player_list[tk_player_index].last_round_violation = round_number;
		SlayPlayer(attacker_ptr, true, true, true);
		DirectLogCommand("%s", log_string);
		SayToAll (ORANGE_CHAT, false, "%s", output_string);
		// Check if ban required
		TKBanPlayer (attacker_ptr, tk_player_index);
	}
	else
	{
		// Add this player to list
		CreateNewTKPlayer(attacker_ptr->name, attacker_ptr->steam_id, attacker_ptr->user_id, 1, 0);
		SlayPlayer(attacker_ptr, true, true, true);
		DirectLogCommand("%s", log_string);
		SayToAll (ORANGE_CHAT, false, "%s", output_string);
		// Check if ban required
		TKBanPlayer (attacker_ptr, tk_player_list_size -1);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_tklist command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ProcessMaTKList(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_CONFIG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	OutputToConsole(player_ptr, "Current Players in TK Violation list\nViolations needed for ban [%i]\n", mani_tk_offences_for_ban.GetInt());
	OutputToConsole(player_ptr, "Steam ID             Name                 User ID  Violations Wounds\n");
	OutputToConsole(player_ptr, "--------------------------------------------------------------------\n");

	for (int i = 0; i < tk_player_list_size; i++)
	{
		OutputToConsole(player_ptr, "%-20s %-20s %-8i %-10i %i\n", 
					tk_player_list[i].steam_id,
					tk_player_list[i].name,
					tk_player_list[i].user_id,
					tk_player_list[i].violations_committed,
					tk_player_list[i].team_wounds);
	}

	return PLUGIN_STOP;
}

SCON_COMMAND(ma_tklist, 2155, MaTKList, false);
