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
#ifdef WIN32
#include <windows.h>
#endif
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_parser.h"
#include "mani_gametype.h"
#include "mani_vfuncs.h"
#include "mani_menu.h"
#include "mani_commands.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_sigscan.h"
#include "mani_language.h"
#include "mani_file.h"
#include "mani_help.h"
#include "mani_team.h"
#include "mani_vars.h"
#include "cbaseentity.h"
//#include "team_spawnpoint.h"


extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	int		con_command_index;
extern	bool	war_mode;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	ConVar	*mp_limitteams;

ConVar mani_swap_team_score ("mani_swap_team_score", "1", 0, "0 = Do not swap a teams score when swapping whole team, 1 = Swap the team score when swapping a team", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiTeam::ManiTeam()
{
	gpManiTeam = this;
	this->CleanUp();
	change_team = false;
	swap_team = false;
	change_team_time = 0.0;
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		pending_swap[i] = false;
	}
	delayed_swap = false;
}

ManiTeam::~ManiTeam()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiTeam::Init(int edict_count)
{
	int number_of_entries = 0;

	this->CleanUp();
	change_team = false;
	swap_team = false;
	change_team_time = 0.0;

	for (int i = 0; i < edict_count; i++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity)
		{
			if (!pEntity->m_pNetworkable)
			{
				continue;
			}

			const char *class_name = pEntity->GetClassName();
			if (!class_name)
			{
				continue;
			}
//  MMsg("Classname %s\n", class_name);

			if (Q_stristr(class_name, "team_") != NULL)
			{
				MMsg("Possible team classname [%s]\n", class_name);
			}

			if (Q_stristr(class_name, gpManiGameType->GetTeamManagerPattern()) != NULL)
			{
				CBaseEntity *pTeam = pEntity->GetUnknown()->GetBaseEntity();
				CTeam *team_ptr = (CTeam *) pTeam;

				int team_number = Prop_GetVal(pEntity, MANI_PROP_TEAM_NUMBER, 0);
				const char *team_name = Prop_GetVal(pEntity, MANI_PROP_TEAM_NAME, "");

				team_list[team_number].edict_ptr = pEntity;
				team_list[team_number].team_index = team_number;
				team_list[team_number].cteam_ptr = team_ptr;
				Q_strcpy(team_list[team_number].team_name, team_name);

				MMsg("Team index [%i] Name [%s]\n", team_number , team_name);
				number_of_entries++;
			}
		}
	}

	MMsg("Found [%i] team manager entities\n", number_of_entries);
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Un Loaded
//---------------------------------------------------------------------------------
void	ManiTeam::UnLoad(void)
{
	this->CleanUp();
	change_team = false;
	swap_team = false;
	change_team_time = 0.0;
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		pending_swap[i] = false;
	}
	delayed_swap = false;
}

//---------------------------------------------------------------------------------
// Purpose: Free up team manager
//---------------------------------------------------------------------------------
void ManiTeam::CleanUp(void)
{
	for (int i = 0; i < 20; i++)
	{
		team_list[i].cteam_ptr = NULL;
		team_list[i].edict_ptr = NULL;
		team_list[i].team_index = -1;
		Q_strcpy(team_list[i].team_name, "");
	}

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		pending_swap[i] = false;
	}

	delayed_swap = false;
}

//---------------------------------------------------------------------------------
// Purpose: Slay any players that are up for being slayed due to tk/tw
//---------------------------------------------------------------------------------
bool	ManiTeam::IsOnSameTeam (player_t *victim, player_t *attacker)
{
	// Is suicide damage ?
	if (victim->user_id == attacker->user_id)
	{
		return false;
	}

	if (!victim->entity)
	{
		// Player not found yet
		if (!FindPlayerByUserID (victim))
		{
			return false;
		}
	}

	if (!attacker->entity)
	{
		// Player not found yet
		if (!FindPlayerByUserID (attacker))
		{
			return false;
		}
	}

	if (attacker->team != victim->team)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the game frame command
//---------------------------------------------------------------------------------
void	ManiTeam::GameFrame(void)
{
	if (war_mode) return;

	if ((change_team || swap_team || delayed_swap) && 
		change_team_time < gpGlobals->curtime && 
		gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		if (change_team)
		{
			change_team = false;
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_balance");
			gpCmd->AddParam("m"); // mute the function
			gpManiTeam->ProcessMaBalance(NULL, "ma_balance", 0, M_SCONSOLE);
		}
		
		if (swap_team)
		{
			swap_team = false;
			this->SwapWholeTeam();
			delayed_swap = false;
			for (int i = 0; i < MANI_MAX_PLAYERS; i++)
			{
				pending_swap[i] = false;
			}
		}

		if (delayed_swap)
		{
			this->ProcessDelayedSwap();
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the game frame command
//---------------------------------------------------------------------------------
void	ManiTeam::RoundEnd(void)
{
	if (war_mode) return;

	change_team_time = gpGlobals->curtime + 2.4;

	if (mani_autobalance_teams.GetInt() == 1 && !war_mode && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		change_team = true;	
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the game frame command
//---------------------------------------------------------------------------------
void	ManiTeam::TriggerSwapTeam(void)
{
	if (war_mode) return;
	swap_team = true;
	// Wait for round end to reset this to normal value (999999999 will give us 2 years)
	change_team_time = 99999999.0;
	SayToAll(LIGHT_GREEN_CHAT, true, "Teams will be swapped for the next round!");
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiTeam::ProcessMaSwapTeam(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: %s This only works on team play games", command_name);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SWAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on a team yet", target_player_list[i].name);
			continue;
		}

		// Swap player over
		if (gpManiGameType->IsGameType(MANI_GAME_CSS) && gpCmd->Cmd_Argc() == 2)
		{
			if (!CCSPlayer_SwitchTeam(EdictToCBE(target_player_list[i].entity),gpManiGameType->GetOpposingTeam(target_player_list[i].team)))
			{
				target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
			}
			else
			{
				UTIL_DropC4(target_player_list[i].entity);
				// If not dead then force model change
				if (!target_player_list[i].player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(target_player_list[i].entity));
				}
			}
		}
		else
		{
			target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(target_player_list[i].team));
		}

		LogCommand (player_ptr, "team swapped user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								target_player_list[i].name, 
								Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(gpManiGameType->GetOpposingTeam(target_player_list[i].team)))); 
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteamd command (Delayed swap)
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiTeam::ProcessMaSwapTeamD(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: %s This only works on CSS", command_name);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SWAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on a team yet", target_player_list[i].name);
			continue;
		}

		if (pending_swap[target_player_list[i].index - 1])
		{
			pending_swap[target_player_list[i].index - 1] = false;
			delayed_swap = false;
			for (int j = 0; j < max_players; j++)
			{
				if (pending_swap[j])
				{
					delayed_swap = false;
				}
			}

			LogCommand (player_ptr, "cancelled delayed team swap user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "Player %s will no longer be moved to team %s at end of round", 
					target_player_list[i].name, 
					Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(gpManiGameType->GetOpposingTeam(target_player_list[i].team)))); 
			}
		}
		else
		{
			pending_swap[target_player_list[i].index - 1] = true;
			delayed_swap = true;
			LogCommand (player_ptr, "delayed team swap user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "Player %s will be moved to team %s at end of round", 
					target_player_list[i].name, 
					Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(gpManiGameType->GetOpposingTeam(target_player_list[i].team)))); 
			}

			change_team_time = 99999999.0;
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process delayed swap
//---------------------------------------------------------------------------------
void	ManiTeam::ProcessDelayedSwap(void)
{
	for (int i = 0; i < max_players; i++)
	{
		if (pending_swap[i])
		{
			// Swap player
			pending_swap[i] = false;
			player_t player;

			player.index = i + 1;
			if (!FindPlayerByIndex(&player)) continue;
			if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

			if (!CCSPlayer_SwitchTeam(EdictToCBE(player.entity),gpManiGameType->GetOpposingTeam(player.team)))
			{
				player.player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(player.team));
			}
			else
			{
				UTIL_DropC4(player.entity);
				// If not dead then force model change
				if (!player.player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(player.entity));
				}
			}
		}
	}

	delayed_swap = false;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_spec command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiTeam::ProcessMaSpec(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	const char *target_string = gpCmd->Cmd_Argv(1);

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (!gpManiGameType->IsSpectatorAllowed())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: %s This only works on games with spectator capability", command_name);
		return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	
	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, IMMUNITY_SWAP))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found some players to swap to other team
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Player %s is not on a team yet", target_player_list[i].name);
			continue;
		}

		target_player_list[i].player_info->ChangeTeam(gpManiGameType->GetSpectatorIndex());

		LogCommand (player_ptr, "moved the following player to spectator [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
		{
			AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "moved %s to be a spectator", target_player_list[i].name);
		}
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_balance command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiTeam::ProcessMaBalance(player_t *player_ptr, const char *command_name, const int	help_id, const int	command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SWAP, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (!gpManiGameType->IsTeamPlayAllowed())
	{
		if (gpCmd->Cmd_Argc() == 1)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: This only works on team play games");
		}
		return PLUGIN_STOP;
	}

	bool mute_action = false;

	if (!player_ptr && gpCmd->Cmd_Argc() == 2)
	{
		mute_action = true;
	}

	if (mani_autobalance_mode.GetInt() == 0)
	{
		// Swap regardless if player is dead or alive
		ProcessMaBalancePlayerType(player_ptr, mute_action, true, true);
	}
	else if (mani_autobalance_mode.GetInt() == 1)
	{
		// Swap dead first, followed by Alive players if needed
		if (!ProcessMaBalancePlayerType(player_ptr, mute_action, true, false))
		{
			// Requirea check of alive people too
			ProcessMaBalancePlayerType(player_ptr, mute_action, false, false);
		}
	}
	else
	{
		// Dead only
		ProcessMaBalancePlayerType(player_ptr, mute_action, true, false);
	}	
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_swapteam command
//---------------------------------------------------------------------------------
bool	ManiTeam::ProcessMaBalancePlayerType
(
 player_t	*player_ptr,
 bool mute_action,
 bool dead_only,
 bool dont_care
)
{

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return true;

	bool return_status = true;
	player_t target_player;
	int	t_count = 0;
	int	ct_count = 0;

	for (int i = 1; i <= max_players; i ++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team == TEAM_B) ct_count ++;
		else if (target_player.team == TEAM_A) t_count ++;
	}

	int team_difference = abs(ct_count - t_count);

	if (team_difference <= mp_limitteams->GetInt())
	{
		// No point balancing
		if (!mute_action)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
		}

		return true;
	}

	int team_to_swap = (ct_count > t_count) ? TEAM_B:TEAM_A;
	int number_to_swap = (team_difference / 2);
	if (number_to_swap == 0)
	{
		// No point balancing
		if (!mute_action)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Teams are already balanced using mp_limitteams settings");
		}

		return true;
	}

	player_t *temp_player_list = NULL;
	int	temp_player_list_size = 0;

	for (int i = 1; i <= max_players; i++)
	{
		target_player.index = i;
		if (!FindPlayerByIndex(&target_player)) continue;
		if (!target_player.player_info->IsPlayer()) continue;
		if (target_player.team != team_to_swap) continue;
		if (!dont_care)
		{
			if (target_player.is_dead != dead_only) continue;
		}

		if (gpManiClient->HasAccess(target_player.index, IMMUNITY, IMMUNITY_BALANCE))
		{
			continue;
		}

		if (pending_swap[i - 1]) continue;

		// Player is a candidate
		AddToList((void **) &temp_player_list, sizeof(player_t), &temp_player_list_size);
		temp_player_list[temp_player_list_size - 1] = target_player;
	}

	if (temp_player_list_size == 0) return false;
	if (temp_player_list_size < number_to_swap)
	{
		// Not enough players for this type
		number_to_swap = temp_player_list_size;
		return_status = false;
	}

	int player_to_swap;

	while (number_to_swap != 0)
	{
		for (;;)
		{
			// Get player to swap
			player_to_swap = rand() % temp_player_list_size;
			if (player_to_swap >= temp_player_list_size) continue; // Safety
			if (temp_player_list[player_to_swap].team == team_to_swap) break;
		}

		// Swap player over
		if (gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			if (!CCSPlayer_SwitchTeam(EdictToCBE(temp_player_list[player_to_swap].entity),gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team)))
			{
				temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team));
			}
			else
			{
				UTIL_DropC4(temp_player_list[player_to_swap].entity);

				// If not dead then force model change
				if (!temp_player_list[player_to_swap].player_info->IsDead())
				{
					CCSPlayer_SetModelFromClass(EdictToCBE(temp_player_list[player_to_swap].entity));
				}
			}
		}
		else
		{
			temp_player_list[player_to_swap].player_info->ChangeTeam(gpManiGameType->GetOpposingTeam(temp_player_list[player_to_swap].team));
		}

		temp_player_list[player_to_swap].team = gpManiGameType->GetOpposingTeam(team_to_swap);

		number_to_swap --;

		LogCommand (player_ptr, "team balanced user [%s] [%s]\n", temp_player_list[player_to_swap].name, temp_player_list[player_to_swap].steam_id);

		if (!mute_action)
		{
			if (player_ptr || mani_mute_con_command_spam.GetInt() == 0)
			{
				AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminswap_anonymous.GetInt(), "swapped player %s to team %s", 
								temp_player_list[player_to_swap].name, 
								Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(temp_player_list[player_to_swap].team)));
			}
		}
	}

	FreeList((void **) &temp_player_list, &temp_player_list_size);

	return return_status;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Move of player to spectator side
//---------------------------------------------------------------------------------
int SpecPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *user_id;

	this->params.GetParam("user_id", &user_id);
	gpCmd->NewCmd();
	gpCmd->AddParam("ma_spec");
	gpCmd->AddParam("%s", user_id);
	gpManiTeam->ProcessMaSpec(player_ptr, "ma_spec", 0, M_MENU);

	return RePopOption(REPOP_MENU_WAIT3);
}

bool SpecPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 790));
	this->SetTitle("%s", Translate(player_ptr, 791));

	MenuItem *ptr = NULL;

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

		if (!player.is_bot)
		{
			if (player_ptr->index != player.index && 
				gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_SWAP))
			{
				continue;
			}
		}

		ptr = new SpecPlayerItem;
		ptr->SetDisplayText("[%s] [%s] %i", 
								Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(player.team)),
								player.name, 
								player.user_id);
		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParamVar("user_id", "%i", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Swap Player draw and request
//---------------------------------------------------------------------------------
int SwapPlayerDItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *user_id;

	this->params.GetParam("user_id", &user_id);
	gpCmd->NewCmd();
	gpCmd->AddParam("ma_swapteam_d");
	gpCmd->AddParam("%s", user_id);
	gpManiTeam->ProcessMaSwapTeamD(player_ptr, "ma_swapteam_d", 0, M_MENU);

	return RePopOption(REPOP_MENU);
}

bool SwapPlayerDPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 180));
	this->SetTitle("%s", Translate(player_ptr, 182));

	MenuItem *ptr = NULL;

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

		if (!player.is_bot)
		{
			if (player_ptr->index != player.index && 
				gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_SWAP))
			{
				continue;
			}
		}

		ptr = new SwapPlayerDItem;
		if (!gpManiTeam->pending_swap[i - 1])
		{
			ptr->SetDisplayText("[%s] [%s] %i", 
				Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(player.team)),
				player.name, 
				player.user_id);
		}
		else
		{
			ptr->SetDisplayText("[%s] %s [%s] %i", 
				Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(player.team)),
				Translate(player_ptr, 183),
				player.name, 
				player.user_id);
		}

		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParamVar("user_id", "%i", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle Swap Player draw and request
//---------------------------------------------------------------------------------
int SwapPlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *user_id;

	this->params.GetParam("user_id", &user_id);
	gpCmd->NewCmd();
	gpCmd->AddParam("ma_swapteam");
	gpCmd->AddParam("%s", user_id);
	gpManiTeam->ProcessMaSwapTeam(player_ptr, "ma_swapteam", 0, M_MENU);

	return RePopOption(REPOP_MENU_WAIT3);
}

bool SwapPlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 180));
	this->SetTitle("%s", Translate(player_ptr, 181));

	MenuItem *ptr = NULL;

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

		if (!player.is_bot)
		{
			if (player_ptr->index != player.index && 
				gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_SWAP))
			{
				continue;
			}
		}

		ptr = new SwapPlayerItem;
		ptr->SetDisplayText("[%s] [%s] %i", 
								Translate(player_ptr, gpManiGameType->GetTeamShortTranslation(player.team)),
								player.name, 
								player.user_id);
		ptr->SetHiddenText("%s", player.name);
		ptr->params.AddParamVar("user_id", "%i", player.user_id);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Get a teams score
//---------------------------------------------------------------------------------
int	ManiTeam::GetTeamScore(int team_index)
{
	return Prop_GetVal(team_list[team_index].edict_ptr, MANI_PROP_TEAM_SCORE, 0);
}

//---------------------------------------------------------------------------------
// Purpose: Set teams score
//---------------------------------------------------------------------------------
void	ManiTeam::SetTeamScore(int team_index, int new_score)
{
	Prop_SetVal(team_list[team_index].edict_ptr, MANI_PROP_TEAM_SCORE, new_score);
}

//---------------------------------------------------------------------------------
// Purpose: Swap the entire team !!!
//---------------------------------------------------------------------------------
void ManiTeam::SwapWholeTeam()
{
	// Get scores first
	int team_a_score = this->GetTeamScore(2);
	int	team_b_score = this->GetTeamScore(3);
    bool swap_allowed = true;

	for (int i = 1;i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;

		if (!gpManiGameType->IsValidActiveTeam(player.team)) continue;

		// Switch player to opposite team
		if (!CCSPlayer_SwitchTeam(EdictToCBE(player.entity),gpManiGameType->GetOpposingTeam(player.team)))
		{
			// Failed to swap player, probably broken sig
			swap_allowed = false;
			break;
		}
		else
		{
			UTIL_DropC4(player.entity);
		}
	}

	if (!swap_allowed) return;

	// Now switch the scores too
	if (mani_swap_team_score.GetInt() == 1)
	{
		this->SetTeamScore(2, team_b_score);
		this->SetTeamScore(3, team_a_score);
	}

	SayToAll(LIGHT_GREEN_CHAT, true, "Teams have been swapped!");
}

SCON_COMMAND(ma_balance, 2099, MaBalance, false);
SCON_COMMAND(ma_swapteam, 2095, MaSwapTeam, true);
SCON_COMMAND(ma_swapteam_d, 2237, MaSwapTeamD, true);
SCON_COMMAND(ma_spec, 2097, MaSpec, false);


ManiTeam	g_ManiTeam;
ManiTeam	*gpManiTeam;
