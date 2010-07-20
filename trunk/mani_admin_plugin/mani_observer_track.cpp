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
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_commands.h"
#include "mani_help.h"
#include "mani_memory.h"
#include "mani_util.h"
#include "mani_output.h"
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_warmuptimer.h"
#include "mani_observer_track.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	time_t	g_RealTime;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ConVar mani_sb_observe_mode ("mani_sb_observe_mode", "0", 0, "0 = sb_status is not executed when ma_observe is started, 1 = sb_status is run automatically when choosing a player to observe", true, 0, true, 1); 

ManiObserverTrack::ManiObserverTrack()
{
	gpManiObserverTrack = this;
	this->CleanUp();
}

ManiObserverTrack::~ManiObserverTrack()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiObserverTrack::Load(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiObserverTrack::Unload(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiObserverTrack::LevelInit(void)
{
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiObserverTrack::ClientDisconnect(player_t	*player_ptr)
{
	if (!gpManiGameType->IsSpectatorAllowed()) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return;
	observer_id[player_ptr->index - 1] = -1;
	for (int i = 0; i < max_players; i++)
	{
		if (observer_id[i] == player_ptr->index)
		{
			// Observed player is leaving server
			if (!IsLAN() && !player_ptr->is_bot)
			{
				strcpy(observer_steam[i], player_ptr->steam_id);
			}

			observer_id[i] = -1;
			if (war_mode) continue;

			player_t observer;
			observer.index = i + 1;
			if (!FindPlayerByIndex(&observer)) continue;
			OutputHelpText(GREEN_CHAT, &observer, "%s", Translate(&observer, 3115, "%s%s", player_ptr->name, player_ptr->steam_id));
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on connect
//---------------------------------------------------------------------------------
void ManiObserverTrack::NetworkIDValidated(player_t	*player_ptr)
{
	if (war_mode) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return;
	if (!gpManiGameType->IsSpectatorAllowed()) return;
	observer_id[player_ptr->index - 1] = -1;
	if (IsLAN() || player_ptr->is_bot) return;

	for (int i = 0; i < max_players; i++)
	{
		if (strcmp(observer_steam[i], player_ptr->steam_id) == 0)
		{
			// Observed player is leaving server
			if (!IsLAN() && !player_ptr->is_bot)
			{
				observer_id[i] = player_ptr->index;

				player_t observer;
				observer.index = i + 1;
				if (!FindPlayerByIndex(&observer)) continue;
				OutputHelpText(GREEN_CHAT, &observer, "%s", Translate(&observer, 3121, "%s%s", player_ptr->name, player_ptr->steam_id));
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do set observer mode on round start
//---------------------------------------------------------------------------------
void ManiObserverTrack::PlayerSpawn(player_t *player_ptr)
{
	if (war_mode) return;
	if (!gpManiGameType->IsSpectatorAllowed()) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return;

	for (int i = 0; i < max_players; i++)
	{
		if (observer_id[i] != player_ptr->index)
		{
			// Not observing
			continue;
		}

		player_t observer;
		observer.index = i + 1;
		if (!FindPlayerByIndex(&observer))
		{
			observer_id[i] = -1;
		}

		if (observer.team != gpManiGameType->GetSpectatorIndex())
		{
			continue;
		}

		if (!gpManiGameType->IsValidActiveTeam(player_ptr->team))
		{
			continue;
		}

		CBaseEntity *pTarget = EdictToCBE(player_ptr->entity);
		CBasePlayer *pBase = (CBasePlayer *) EdictToCBE(observer.entity);

		CBasePlayer_SetObserverTarget(pBase, pTarget);
		OutputHelpText(GREEN_CHAT, &observer, "%s", Translate(&observer, 3116, "%s%s", player_ptr->name, player_ptr->steam_id));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do set observer mode on player death
//---------------------------------------------------------------------------------
void ManiObserverTrack::PlayerDeath(player_t *player_ptr)
{
	if (war_mode) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return;
	if (!gpManiGameType->IsSpectatorAllowed()) return;
	if (observer_id[player_ptr->index - 1] == -1) return;

	player_t target;
	target.index = observer_id[player_ptr->index - 1];
	if (!FindPlayerByIndex(&target)) return;

	if (target.is_dead) 
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3117, "%s", target.name));
		return;
	}

	CBaseEntity *pTarget = EdictToCBE(target.entity);
	CBasePlayer *pBase = (CBasePlayer *) EdictToCBE(player_ptr->entity);

	CBasePlayer_SetObserverTarget(pBase, pTarget);
	OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3116, "%s%s", target.name, target.steam_id));
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_observe command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiObserverTrack::ProcessMaObserve
(
 player_t *player_ptr, 
 const char	*command_name, 
 const int	help_id, 
 const int	command_type
 )
{
	if (war_mode) return PLUGIN_CONTINUE;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return PLUGIN_CONTINUE;
	if (!gpManiGameType->IsSpectatorAllowed()) return PLUGIN_CONTINUE;
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	const char *target_string = gpCmd->Cmd_Argv(1);

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, target_string, NULL))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found a player to observe
	for (int i = 0; i < target_player_list_size; i++)
	{
		if (target_player_list[i].player_info->IsFakeClient())
		{
			continue;
		}

		observer_id[player_ptr->index - 1] = target_player_list[i].index;
		strcpy(observer_steam[player_ptr->index - 1], "");

		if (mani_sb_observe_mode.GetInt() == 1)
		{
			helpers->ClientCommand(player_ptr->entity, "sb_status");
		}

		LogCommand (player_ptr, "observing user [%s] [%s]\n", target_player_list[i].name, target_player_list[i].steam_id);
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3118, "%s%s", target_player_list[i].name, target_player_list[i].steam_id));

		if (!gpManiGameType->IsValidActiveTeam(target_player_list[i].team))
		{
			break;
		}

		if (!target_player_list[i].is_dead &&
			(player_ptr->is_dead || gpManiGameType->GetSpectatorIndex() == player_ptr->team))
		{
			CBaseEntity *pTarget = EdictToCBE(target_player_list[i].entity);
			CBasePlayer *pBase = (CBasePlayer *) EdictToCBE(player_ptr->entity);

			CBasePlayer_SetObserverTarget(pBase, pTarget);
			OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3116, "%s%s", target_player_list[i].name, target_player_list[i].steam_id));
		}

		break;
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_endobserve command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiObserverTrack::ProcessMaEndObserve
(
 player_t *player_ptr, 
 const char	*command_name, 
 const int	help_id, 
 const int	command_type
 )
{
	if (war_mode) return PLUGIN_CONTINUE;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_OBSERVER_TARGET) == -1) return PLUGIN_CONTINUE;
	if (!gpManiGameType->IsSpectatorAllowed()) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (observer_id[player_ptr->index - 1] == -1)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3119));
		strcpy(observer_steam[player_ptr->index - 1], "");
	}
	else
	{
		player_t target;
		target.index = observer_id[player_ptr->index - 1];
		if (FindPlayerByIndex(&target))
		{
			OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3120, "%s%s", target.name, target.steam_id));
		}

		observer_id[player_ptr->index - 1] = -1;
		strcpy(observer_steam[player_ptr->index - 1], "");
	}

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Clean Up
//---------------------------------------------------------------------------------
void ManiObserverTrack::CleanUp(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		observer_id[i] = -1;
		strcpy(observer_steam[i], "");
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle Swap Player draw and request
//---------------------------------------------------------------------------------
int ObservePlayerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *user_id;

	this->params.GetParam("user_id", &user_id);
	if (strcmp(user_id, "-1") == 0)
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_endobserve");
		gpManiObserverTrack->ProcessMaEndObserve(player_ptr, "ma_endobserve", 0, M_MENU);		
	}
	else
	{
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_observe");
		gpCmd->AddParam("%s", user_id);
		gpManiObserverTrack->ProcessMaObserve(player_ptr, "ma_observe", 0, M_MENU);
	}

	return RePopOption(REPOP_MENU);
}

bool ObservePlayerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 3110));
	this->SetTitle("%s", Translate(player_ptr, 3111));

	MenuItem *ptr = NULL;

	if (gpManiObserverTrack->observer_id[player_ptr->index - 1] != -1)
	{
		ptr = new ObservePlayerItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 3114));
		ptr->params.AddParamVar("user_id", "%i", -1);
		ptr->SetHiddenText(" ");
		this->AddItem(ptr);
	}

	for( int i = 1; i <= max_players; i++ )
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.player_info->IsFakeClient())
		{
			continue;
		}

		ptr = new ObservePlayerItem;

		if (gpManiObserverTrack->observer_id[player_ptr->index - 1] == player.index)
		{
			ptr->SetDisplayText("%s [%s] %i",
				Translate(player_ptr, 3112),
				player.name, 
				player.user_id);
		}
		else
		{
			ptr->SetDisplayText("[%s] %i",
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

ManiObserverTrack	g_ManiObserverTrack;
ManiObserverTrack	*gpManiObserverTrack;
