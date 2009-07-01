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
#include "inetchannelinfo.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_menu.h"
#include "mani_gametype.h"
#include "mani_sprayremove.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerGameDLL	*serverdll;
extern	IPlayerInfoManager *playerinfomanager;
extern	ITempEntsSystem *temp_ents;

extern	CGlobalVars *gpGlobals;
extern	ConVar	*sv_lan;
extern	int	max_players;
extern	bool war_mode;
extern	int	purplelaser_index;
extern	int	spray_glow_index;


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiSprayRemove
//class ManiSprayRemove
//{

ManiSprayRemove::ManiSprayRemove()
{
	// Init
	this->CleanUp();
	gpManiSprayRemove = this;
}

ManiSprayRemove::~ManiSprayRemove()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Clean up
//---------------------------------------------------------------------------------
void ManiSprayRemove::CleanUp(void)
{
	// Cleanup
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		spray_list[i].user_id = -1;
		spray_list[i].in_use = false;
	}

	game_timer = -9999.0;
	check_list = false;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin loaded
//---------------------------------------------------------------------------------
void ManiSprayRemove::Load(void)
{
	this->CleanUp();
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Map Loaded
//---------------------------------------------------------------------------------
void ManiSprayRemove::LevelInit(void)
{
	this->CleanUp();
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Virtual Hook picked up a spray being fired
//---------------------------------------------------------------------------------
bool ManiSprayRemove::SprayFired(const Vector *pos, int index)
{
	if (war_mode) return true;
	if (mani_spray_tag.GetInt() == 0) return true;
	if (index > max_players) return true;

	player_t player;
	player.index = index;
	if (!FindPlayerByIndex(&player)) return true;

	// Block sprays
	if (mani_spray_tag_block_mode.GetInt() == 1)
	{
		if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			SayToPlayer(&player, "%s", mani_spray_tag_block_message.GetString());
		}
		else
		{
			SayToPlayerColoured(&player, "%s", mani_spray_tag_block_message.GetString());
		}

		return false;
	}

	// Got player
	if (player.is_bot) return true;

	int	i = index - 1;

	spray_list[i].in_use = true;
	spray_list[i].user_id = player.user_id;
	Q_strcpy(spray_list[i].steam_id, player.steam_id);
	Q_strcpy(spray_list[i].name, player.name);
	spray_list[i].end_time = gpGlobals->curtime + mani_spray_tag_spray_duration.GetFloat();
	spray_list[i].position = *pos;
	check_list = true;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Look for pending clients with STEAM_ID of STEAM_ID_PENDING
//---------------------------------------------------------------------------------
void ManiSprayRemove::GameFrame(void)
{

	if (war_mode) return;
	if (mani_spray_tag.GetInt() == 0) return;

	// Quick check
	if (!check_list) return;

	bool	found_spray = false;

	if (game_timer >= gpGlobals->curtime)
	{
		return;
	}

	game_timer = gpGlobals->curtime + 1.0;

	for (int i = 0; i < max_players; i ++)
	{
		if (spray_list[i].in_use)
		{
			if (spray_list[i].end_time < gpGlobals->curtime)
			{
				spray_list[i].user_id = -1;
				spray_list[i].in_use = false;
			}
			else
			{
				found_spray = true;
			}
		}
	}

	if (!found_spray)
	{
		check_list = false;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Client left....
//---------------------------------------------------------------------------------
void ManiSprayRemove::ClientDisconnect(player_t *player_ptr)
{
	// Dont change in_use flag as we still might want to ban player who left
	spray_list[player_ptr->index - 1].user_id = -1;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Check if player is close enough to ID spray
//---------------------------------------------------------------------------------
int ManiSprayRemove::IsSprayValid(player_t *player_ptr)
{

	int	spray_index = -1;

	float closest = mani_spray_tag_spray_distance_limit.GetFloat();
	float distance = 0.0;

	Vector p_pos = player_ptr->player_info->GetAbsOrigin();

	for (int i = 0; i < max_players; i++)
	{
//		if (player_ptr->index -1 == i) continue;
		Vector	v = p_pos - spray_list[i].position;
		distance = v.Length();

		if (distance <= mani_spray_tag_spray_distance_limit.GetFloat())
		{
			if (distance <= closest)
			{
				closest = distance;
				spray_index = i;
			}
		}
	}

	if (mani_spray_tag_spray_highlight.GetInt() == 0)
	{
		return spray_index;
	}

	// Show beam to spray position
	if (spray_index != -1 &&
        gpManiGameType->GetAdvancedEffectsAllowed())
	{
		if (mani_spray_tag_spray_highlight.GetInt() == 1 &&
			gpManiGameType->IsDeathBeamAllowed())
		{
			MRecipientFilter mrf; // this is my class, I'll post it later.
			mrf.MakeReliable();
			mrf.AddPlayer(player_ptr->index);

			Vector source = player_ptr->player_info->GetAbsOrigin();

			source.z += 50;

			temp_ents->BeamPoints((IRecipientFilter &)mrf,
							0, // Delay
							&source, // Start Vector
							&(spray_list[spray_index].position), // End Vector
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
		else if (mani_spray_tag_spray_highlight.GetInt() == 2 ||
			(mani_spray_tag_spray_highlight.GetInt() == 1 && 
			 !gpManiGameType->IsDeathBeamAllowed()))
		{
			MRecipientFilter mrf; // this is my class, I'll post it later.
			mrf.MakeReliable();
			mrf.AddPlayer(player_ptr->index);

			temp_ents->GlowSprite((IRecipientFilter &) mrf, 0, &(spray_list[spray_index].position), spray_glow_index, 15, 0.8, 255);
		}
	}

	return spray_index;
}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_spray
//---------------------------------------------------------------------------------
void	ManiSprayRemove::ProcessMaSprayMenu
( 
 player_t *admin_ptr, 
 int admin_index, 
 int next_index, 
 int argv_offset,
 const char *menu_command
 )
{
	const int argc = engine->Cmd_Argc();

	if (war_mode) return;
	if (mani_spray_tag.GetInt() == 0) return;

	if (argc - argv_offset == 3 && !FStrEq(menu_command, "spray"))
	{
		char	target[128];
		// Target could be user_id or steam id (depends on lan mode)
		Q_strcpy(target, engine->Cmd_Argv(2 + argv_offset));

		if (FStrEq(menu_command, "spraywarn"))
		{
			// Warn target player about offending spray
			if (!FindTargetPlayers(admin_ptr, target, IMMUNITY_DONT_CARE))
			{
				SayToPlayer(admin_ptr, "Did not find player %s", target);
				return;
			}

			for (int i = 0; i < target_player_list_size; i++)
			{
				player_t *target_player_ptr;
		
				target_player_ptr = (player_t *) &(target_player_list[i]);
				if (!gpManiGameType->IsGameType(MANI_GAME_CSS))
				{
					SayToPlayer(target_player_ptr, "%s", mani_spray_tag_warning_message.GetString());
				}
				else
				{
					SayToPlayerColoured(target_player_ptr, "%s", mani_spray_tag_warning_message.GetString());
				}

				LogCommand (admin_ptr->entity, "Warned player [%s] [%s] for spray tag", target_player_ptr->name, target_player_ptr->steam_id);
				break;
			}
		}
		else if (FStrEq (menu_command, "spraykick") && 
				gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK))
		{
			// Warn target player about offending spray
			if (!FindTargetPlayers(admin_ptr, target, IMMUNITY_DONT_CARE))
			{
				SayToPlayer(admin_ptr, "Did not find player %s", target);
				return;
			}

			for (int i = 0; i < target_player_list_size; i++)
			{
				player_t *target_player_ptr;
				char	kick_cmd[256];
		
				target_player_ptr = (player_t *) &(target_player_list[i]);
				SayToPlayer(target_player_ptr, "%s", mani_spray_tag_kick_message.GetString());
				LogCommand (admin_ptr->entity, "Kicked player [%s] [%s] for spray tag", target_player_ptr->name, target_player_ptr->steam_id);
				Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i See console for reason\n", target_player_ptr->user_id);
				engine->ServerCommand(kick_cmd);				
				break;
			}
		}
		else if (FStrEq (menu_command, "sprayban") && 
				gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN))
		{
			player_t	player;
			char	ban_cmd[256];

			if (sv_lan && sv_lan->GetInt())
			{
				// Lan mode
				player.user_id = Q_atoi(target);
				if (!FindPlayerByUserID(&player))
				{
					SayToPlayer(admin_ptr, "Did not find player %s", target);
					return;
				}

				SayToPlayer(&player, "%s", mani_spray_tag_ban_message.GetString());
				LogCommand (admin_ptr->entity, "Banned player [%s] [%s] for spray tag for %i minutes", 
								player.name, 
								player.steam_id, 
								mani_spray_tag_ban_time.GetInt());

				// Ban by user id
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_spray_tag_ban_time.GetInt(), 
										player.user_id);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
			}
			else
			{
				// Steam Mode
				Q_strcpy(player.steam_id, target);
				if (FindPlayerBySteamID(&player))
				{
					// Player is on server
					SayToPlayer(&player, "%s", mani_spray_tag_ban_message.GetString());
					// Ban by user id
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i\n", 
										mani_spray_tag_ban_time.GetInt(), 
										player.user_id);
					LogCommand (admin_ptr->entity, "Banned player [%s] [%s] for spray tag for %i minutes", 
									player.name, 
									player.steam_id, 
									mani_spray_tag_ban_time.GetInt());
				}
				else
				{
					// Ban by steam id
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i \"%s\"\n", 
										mani_spray_tag_ban_time.GetInt(), 
										player.steam_id);
					LogCommand (admin_ptr->entity, "Banned player [%s] for spray tag for %i minutes",  
									player.steam_id, 
									mani_spray_tag_ban_time.GetInt());
				}


				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
			}
		}
		else if (FStrEq (menu_command, "spraypermban") && 
				gpManiClient->IsAdminAllowed(admin_index, ALLOW_PERM_BAN))
		{

			player_t	player;
			char	ban_cmd[256];

			if (sv_lan && sv_lan->GetInt())
			{
				// Lan mode
				player.user_id = Q_atoi(target);
				if (!FindPlayerByUserID(&player))
				{
					SayToPlayer(admin_ptr, "Did not find player %s", target);
					return;
				}

				SayToPlayer(&player, "%s", mani_spray_tag_ban_message.GetString());
				LogCommand (admin_ptr->entity, "Banned player [%s] [%s] for spray tag for %i minutes", 
								player.name, 
								player.steam_id, 
								mani_spray_tag_ban_time.GetInt());

				// Ban by user id
				Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_spray_tag_ban_time.GetInt(), 
										player.user_id);
				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
			}
			else
			{
				// Steam Mode
				Q_strcpy(player.steam_id, target);
				if (FindPlayerBySteamID(&player))
				{
					// Player is on server
					SayToPlayer(&player, "%s", mani_spray_tag_perm_ban_message.GetString());
					// Ban by user id
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i\n", 
										0, 
										player.user_id);
					LogCommand (admin_ptr->entity, "Banned player [%s] [%s] for spray tag permanently", 
									player.name, 
									player.steam_id);
				}
				else
				{
					// Ban by steam id
					Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i \"%s\"\n", 
										0, 
										player.steam_id);
					LogCommand (admin_ptr->entity, "Banned player [%s] for spray tag for permanently",  
									player.steam_id);
				}

				engine->ServerCommand(ban_cmd);
				engine->ServerCommand("writeid\n");
			}
		}
		else
		{
			return;
		}

		return;
	}
	else
	{
		int spray_index = this->IsSprayValid(admin_ptr);

		if (spray_index == -1)
		{
			OutputHelpText(admin_ptr, false, "No spray tags within range of current position !!");
			return ;
		}

		// Setup player list
		FreeMenu();

		char	player_string[64];
		bool	lan_mode = false;

		if (sv_lan && sv_lan->GetInt())
		{
			// Lan mode
			lan_mode = true;
			Q_snprintf(player_string, sizeof(player_string), "%i", spray_list[spray_index].user_id);
		}
		else
		{
			// Steam Mode
			Q_snprintf(player_string, sizeof(player_string), "%s", spray_list[spray_index].steam_id);
		}

		AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
		Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Warn Player");
		Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin spraywarn \"%s\"", player_string);

		if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Kick Player");
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin spraykick \"%s\"", player_string);
		}

		// No bans in lan mode !!!
		if (!lan_mode && gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Ban Player for %i minutes", mani_spray_tag_ban_time.GetInt());
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin sprayban \"%s\"", player_string);
		}

		// No bans in lan mode !!!
		if (!lan_mode && gpManiClient->IsAdminAllowed(admin_index, ALLOW_PERM_BAN))
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Ban Player permanently");
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin spraypermban  \"%s\"", player_string);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		char	temp_title[1024];

		Q_snprintf (temp_title, sizeof (temp_title), 
					"Spray Tag Tracking\nPlayer [%s]\nSteam ID [%s]\n",
					spray_list[spray_index].name,
					spray_list[spray_index].steam_id);

										
		DrawSubMenu (admin_ptr, "Press Esc to view Spray Details", temp_title, next_index, "admin", "spray", true, -1);
		// Draw menu list
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_spray command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiSprayRemove::ProcessMaSpray
(
 int index, 
 bool svr_command
)
{
	player_t player;
	int	admin_index;
	player.entity = NULL;

	if (war_mode)
	{
		return PLUGIN_CONTINUE;
	}

	if (mani_spray_tag.GetInt() == 0) return PLUGIN_CONTINUE;

	// Check if player is admin

	player.index = index;
	if (!FindPlayerByIndex(&player)) return PLUGIN_STOP;
	if (!gpManiClient->IsAdminAllowed(&player, "ma_spray", ALLOW_SPRAY_TAG, war_mode, &admin_index)) return PLUGIN_STOP;
	
	// Cheat by getting the client to fire up the menu using raw commands
	engine->ClientCommand(player.entity, "admin spray\n");
	return PLUGIN_STOP;
}


ManiSprayRemove	g_ManiSprayRemove;
ManiSprayRemove	*gpManiSprayRemove;
