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
#include "inetchannelinfo.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_mainclass.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_menu.h"
#include "mani_util.h"
#include "mani_effects.h"
#include "mani_gametype.h"
#include "mani_commands.h"
#include "mani_sprayremove.h"
#include "KeyValues.h"
#include "cbaseentity.h"
#include "mani_playerkick.h"
#include "mani_handlebans.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IServerGameDLL	*serverdll;
extern	IPlayerInfoManager *playerinfomanager;
extern	ITempEntsSystem *temp_ents;

extern	CGlobalVars *gpGlobals;
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
		OutputHelpText(LIGHT_GREEN_CHAT, &player, "%s", mani_spray_tag_block_message.GetString());
		return false;
	}

	// Got player
	if (player.is_bot) return true;

	int	i = index - 1;

	spray_list[i].in_use = true;
	spray_list[i].user_id = player.user_id;
	Q_strcpy(spray_list[i].steam_id, player.steam_id);
	Q_strcpy(spray_list[i].name, player.name);
	Q_strcpy(spray_list[i].ip_address, player.ip_address);

	const char *password  = engine->GetClientConVarValue(player.index, "_password" );

	if (password)
	{
		Q_strcpy(spray_list[i].password, password);
	}
	else
	{
		Q_strcpy(spray_list[i].password, "");
	}

	spray_list[i].end_time = gpGlobals->curtime + mani_spray_tag_spray_duration.GetFloat();
	spray_list[i].position = *pos;
	check_list = true;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Remove sprays that have timed out
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
		if (!spray_list[i].in_use) continue;
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

			if (gpManiGameType->GetAdvancedEffectsAllowed())
			{
				temp_ents->GlowSprite((IRecipientFilter &) mrf, 0, &(spray_list[spray_index].position), spray_glow_index, 15, 0.8, 255);
			}
		}
	}

	return spray_index;
}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_spray
//---------------------------------------------------------------------------------
int SprayItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	if (war_mode) return CLOSE_MENU;
	if (mani_spray_tag.GetInt() == 0) return CLOSE_MENU;

	char	*menu_command;
	char	*player_string;
	this->params.GetParam("option", &menu_command);
	m_page_ptr->params.GetParam("player", &player_string);

	char	target[128];
	// Target could be user_id or steam id (depends on lan mode)
	Q_strcpy(target, player_string);

	if (FStrEq(menu_command, "warn"))
	{
		// Warn target player about offending spray
		if (!FindTargetPlayers(player_ptr, target, NULL))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target));
			return CLOSE_MENU;
		}

		for (int i = 0; i < target_player_list_size; i++)
		{
			player_t *target_player_ptr;
	
			target_player_ptr = (player_t *) &(target_player_list[i]);
			OutputHelpText(LIGHT_GREEN_CHAT, target_player_ptr, "%s", mani_spray_tag_warning_message.GetString());
			OutputHelpText(LIGHT_GREEN_CHAT, player_ptr, "%s", mani_spray_tag_warning_message.GetString());
			LogCommand (player_ptr, "Warned player [%s] [%s] for spray tag\n", target_player_ptr->name, target_player_ptr->steam_id);
			break;
		}
	}
	else if (FStrEq(menu_command, "slap") && 
		gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAY) &&
			gpManiGameType->IsTeleportAllowed())
	{
		// Warn target player about offending spray
		if (!FindTargetPlayers(player_ptr, target, IMMUNITY_SLAP))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target));
			return CLOSE_MENU;
		}

		for (int i = 0; i < target_player_list_size; i++)
		{
			player_t *target_player_ptr;
	
			target_player_ptr = (player_t *) &(target_player_list[i]);
			OutputHelpText(LIGHT_GREEN_CHAT, target_player_ptr, "%s", mani_spray_tag_warning_message.GetString());
			OutputHelpText(LIGHT_GREEN_CHAT, player_ptr, "%s", mani_spray_tag_warning_message.GetString());

			ProcessSlapPlayer(target_player_ptr, mani_spray_tag_slap_damage.GetInt(), false);
			LogCommand (player_ptr, "Slapped and warned player [%s] [%s] for spray tag\n", target_player_ptr->name, target_player_ptr->steam_id);
			break;
		}
	}
	else if (FStrEq (menu_command, "kick") && 
			gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_KICK))
	{
		// Kick player for offending spray
		if (!FindTargetPlayers(player_ptr, target, IMMUNITY_KICK))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target));
			return CLOSE_MENU;
		}

		for (int i = 0; i < target_player_list_size; i++)
		{
			player_t *target_player_ptr;
	
			target_player_ptr = (player_t *) &(target_player_list[i]);
			OutputHelpText(LIGHT_GREEN_CHAT, target_player_ptr, "%s", mani_spray_tag_kick_message.GetString());
			OutputHelpText(LIGHT_GREEN_CHAT, player_ptr, "%s", mani_spray_tag_kick_message.GetString());
			LogCommand (player_ptr, "Kicked player [%s] [%s] for spray tag\n", target_player_ptr->name, target_player_ptr->steam_id);
			gpManiPlayerKick->AddPlayer( target_player_ptr->index, 0.5f, "For spray tag" );
			break;
		}
	}
	else if (FStrEq (menu_command, "ban") && 
			gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN))
	{
		player_t	player;

		if (IsLAN())
		{
			// Lan mode
			OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot ban in Lan mode!");
			return CLOSE_MENU;
		}
		else
		{
			// Steam Mode
			Q_strcpy(player.steam_id, target);
			if (FindPlayerBySteamID(&player))
			{
				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is immune from being banned", player.name);
					return CLOSE_MENU;
				}

				// Player is on server
				OutputHelpText(LIGHT_GREEN_CHAT, &player, "%s", mani_spray_tag_ban_message.GetString());
				OutputHelpText(GREEN_CHAT, player_ptr, "%s", mani_spray_tag_ban_message.GetString());
				// Ban by user id
				LogCommand (NULL,"Ban (Spray Tag) [%s] [%s]\n", player.name, player.steam_id);
				gpManiHandleBans->AddBan ( &player, player.steam_id, "MAP", mani_spray_tag_ban_time.GetInt(), "Spray Tag Ban", "Spray Tag Ban" );
			}
			else
			{
				int found_spray = -1;

				// Find player details in spray list, we need to check immunity
				for (int i = 0; i < max_players; i++)
				{
					if (!gpManiSprayRemove->spray_list[i].in_use) continue;

					if (FStrEq(gpManiSprayRemove->spray_list[i].steam_id, player.steam_id))
					{
						found_spray = i;
						break;
					}
				}

				if (found_spray == -1)
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is not in the spray tag list", player.name);
					return CLOSE_MENU;
				}

				Q_strcpy(player.password, gpManiSprayRemove->spray_list[found_spray].password);
				Q_strcpy(player.name, gpManiSprayRemove->spray_list[found_spray].name);
				Q_strcpy(player.ip_address, gpManiSprayRemove->spray_list[found_spray].ip_address);

				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is immune from being banned", player.name);
					return CLOSE_MENU;
				}

				// Ban by steam id
				LogCommand (NULL,"Ban (Spray Tag) [%s] [%s]\n", player.name, player.steam_id);
				gpManiHandleBans->AddBan ( &player, player.steam_id, "MAP", mani_spray_tag_ban_time.GetInt(), "Spray Tag Ban", "Spray Tag Ban" );
			}
			gpManiHandleBans->WriteBans();
		}
	}
	else if (FStrEq (menu_command, "pban") && 
			gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN))
	{
		player_t	player;

		if (IsLAN())
		{
			// Lan mode
			OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot ban in Lan mode!");
			return CLOSE_MENU;

		}
		else
		{
			// Steam Mode
			Q_strcpy(player.steam_id, target);
			if (FindPlayerBySteamID(&player))
			{
				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is immune from being banned", player.name);
					return CLOSE_MENU;
				}

				// Player is on server
				OutputHelpText(LIGHT_GREEN_CHAT, &player, "%s", mani_spray_tag_perm_ban_message.GetString());
				OutputHelpText(LIGHT_GREEN_CHAT, player_ptr, "%s", mani_spray_tag_perm_ban_message.GetString());
				// Ban by user id
				LogCommand (NULL,"Ban (Spray Tag - permanent) [%s] [%s]\n", player.name, player.steam_id);
				gpManiHandleBans->AddBan ( &player, player.steam_id, "MAP", 0, "Permanent Spray Tag Ban", "Permanent Spray Tag Ban" );
			}
			else
			{
				int found_spray = -1;

				// Find player details in spray list, we need to check immunity
				for (int i = 0; i < max_players; i++)
				{
					if (!gpManiSprayRemove->spray_list[i].in_use) continue;

					if (FStrEq(gpManiSprayRemove->spray_list[i].steam_id, player.steam_id))
					{
						found_spray = i;
						break;
					}
				}

				if (found_spray == -1)
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is not in the spray tag list", player.name);
					return CLOSE_MENU;
				}

				Q_strcpy(player.password, gpManiSprayRemove->spray_list[found_spray].password);
				Q_strcpy(player.name, gpManiSprayRemove->spray_list[found_spray].name);
				Q_strcpy(player.ip_address, gpManiSprayRemove->spray_list[found_spray].ip_address);

				if (gpManiClient->HasAccess(player.index, IMMUNITY, IMMUNITY_BAN))
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Player [%s] is immune from being banned", player.name);
					return CLOSE_MENU;
				}

				// Ban by steam id
				LogCommand (NULL,"Ban (Spray Tag - permanent) [%s] [%s]\n", player.name, player.steam_id);
				gpManiHandleBans->AddBan ( &player, player.steam_id, "MAP", 0, "Permanent Spray Tag Ban", "Permanent Spray Tag Ban" );
			}
			gpManiHandleBans->WriteBans();
		}
	}

	return CLOSE_MENU;
}

bool SprayPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 1257));

	int spray_index = gpManiSprayRemove->IsSprayValid(player_ptr);

	if (spray_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, Translate(player_ptr, 1250));
		return false;
	}

	bool	lan_mode = false;

	if (IsLAN())
	{
		// Lan mode
		lan_mode = true;
		this->params.AddParamVar("player", "%i", gpManiSprayRemove->spray_list[spray_index].user_id);
	}
	else
	{
		// Steam Mode
		this->params.AddParam("player", gpManiSprayRemove->spray_list[spray_index].steam_id);
	}

	MenuItem *ptr = NULL;
	ptr = new SprayItem;
	ptr->SetDisplayText("%s", Translate(player_ptr, 1251));
	ptr->params.AddParam("option", "warn");
	this->AddItem(ptr);

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SLAP) && gpManiGameType->IsTeleportAllowed())
	{
		ptr = new SprayItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 1252));
		ptr->params.AddParam("option", "slap");
		this->AddItem(ptr);
	}

	if (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_KICK))
	{
		ptr = new SprayItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 1253));
		ptr->params.AddParam("option", "kick");
		this->AddItem(ptr);
	}

	// No bans in lan mode !!!
	if (!lan_mode && (gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BAN) ||
					  gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN)))
	{
		ptr = new SprayItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 1254, "%i", mani_spray_tag_ban_time.GetInt()));
		ptr->params.AddParam("option", "ban");
		this->AddItem(ptr);
	}

	// No bans in lan mode !!!
	if (!lan_mode && gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_PERM_BAN))
	{
		ptr = new SprayItem;
		ptr->SetDisplayText("%s", Translate(player_ptr, 1255));
		ptr->params.AddParam("option", "pban");
		this->AddItem(ptr);
	}

	this->SetTitle("%s", Translate(player_ptr, 1256, "%s%s%s", 
					gpManiSprayRemove->spray_list[spray_index].name, 
					gpManiSprayRemove->spray_list[spray_index].steam_id, 
					gpManiSprayRemove->spray_list[spray_index].ip_address));
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_spray command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiSprayRemove::ProcessMaSpray(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!player_ptr) return PLUGIN_CONTINUE;

	// Check if player is admin
	if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_SPRAY_TAG, war_mode)) return PLUGIN_BAD_ADMIN;

	MENUPAGE_CREATE_FIRST(SprayPage, player_ptr, 0, -1);
	return PLUGIN_STOP;
}


ManiSprayRemove	g_ManiSprayRemove;
ManiSprayRemove	*gpManiSprayRemove;
