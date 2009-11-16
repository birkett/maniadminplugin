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
#include "mani_callback_sourcemm.h"
#include "mani_sourcehook.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_util.h"
#include "mani_sounds.h"
#include "mani_gametype.h"
#include "mani_weapon.h"
#include "mani_vfuncs.h"
#include "mani_commands.h"
#include "mani_team.h"
#include "mani_vars.h"
#include "mani_help.h"
#include "mani_warmuptimer.h"
#include "mani_maps.h"
#include "mani_sigscan.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameEnts *serverents;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;

ConVar mani_weapon_restrict_refund_on_spawn ("mani_weapon_restrict_refund_on_spawn", "0", 0, "0 = Money not refunded if weapon removed at spawn, 1 = money refunded if weapon removed at spawn", true, 0, true, 1); 
ConVar mani_weapon_restrict_prevent_pickup ("mani_weapon_restrict_prevent_pickup", "0", 0, "0 = restricted weapons can be picked up, 1 = restricted weapons cannot be picked up", true, 0, true, 1); 

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//---------------------------------------------------------------------------------
// Purpose: Setup Weapon class
//---------------------------------------------------------------------------------
MWeapon::MWeapon(const char *weapon_name, int translation_id, int weapon_index)
{
	strcpy(this->weapon_name, weapon_name);
	this->index = weapon_index;
	this->translation_id = translation_id;
	this->team_limit = 0;
	this->restricted = false;
	this->round_ratio = 0;
	//Msg("Setup: Weapon Name [%s]\n", weapon_name);
}

//---------------------------------------------------------------------------------
// Purpose: Checks if player can buy this weapon or not
//---------------------------------------------------------------------------------
bool	MWeapon::CanBuy(player_t *player_ptr, int offset, int &reason, int &limit, int &ratio)
{
	//	Msg("[%s] [%s] [%i] [%i] [%s]\n", weapon_name, display_name, team_limit, round_ratio, (restricted == true) ? "YES":"NO");
	if (!restricted || war_mode) return true;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return true;
	if (translation_id == 0) return true;

	// Weapons are restricted, check if limit is 0
	if (round_ratio != 0)
	{
		if (player_ptr->team == 2)
		{
			if (gpManiTeam->GetTeamScore(2) - gpManiTeam->GetTeamScore(3) >= round_ratio)
			{
				reason = WEAPON_RATIO;
				ratio = round_ratio;
				return false;
			}
		}
		else
		{
			if (gpManiTeam->GetTeamScore(3) - gpManiTeam->GetTeamScore(2) >= round_ratio)
			{
				reason = WEAPON_RATIO;
				ratio = round_ratio;
				return false;
			}
		}

		if (team_limit == 0)
		{
			return true;
		}
	}

	if (team_limit == 0 && round_ratio == 0) 
	{
		reason = WEAPON_RESTRICT;
		return false;
	}

	count[2] = 0;
	count[3] = 0;

	int compare = team_limit + offset;
	// Count up number of weapons that players have
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.player_info->IsHLTV()) continue;

		CBaseEntity *pPlayer = EdictToCBE(player.entity);
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		if (!pCombat) continue;
		for (int j = 0; j < 20; j ++)
		{
			CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_GetWeapon(pCombat, j);
			if (!pWeapon) continue;

			if (strcmp(weapon_name, CBaseCombatWeapon_GetName(pWeapon)) == 0)
			{
				count[player.team] ++;
			}
		}
	}

	if (count[player_ptr->team] >= compare)
	{
		reason = WEAPON_LIMIT;
		limit = team_limit;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Free the weapons used
//---------------------------------------------------------------------------------
ManiWeaponMgr::ManiWeaponMgr()
{
	for (int i = 0; i < 29; i ++)
	{
		weapons[i] = NULL;
	}

	this->alias_list.clear();
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		hooked[i] = false;
		ignore_hook[i] = false;
	}

	gpManiWeaponMgr = this;
}

ManiWeaponMgr::~ManiWeaponMgr()
{
	this->CleanUp();
}

void ManiWeaponMgr::Load()
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	this->CleanUp();
	this->SetupWeapons();
	this->LoadRestrictions();
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		hooked[i] = false;
		ignore_hook[i] = false;
		next_message[i] = 0.0;
	}

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player))
		{
			continue;
		}

		if (player.is_bot) continue;

		g_ManiSMMHooks.HookWeapon_CanUse((CBasePlayer *) EdictToCBE(player.entity));
		hooked[i] = true;
		ignore_hook[i] = false;
	}
}

void ManiWeaponMgr::Unload()
{
	this->LevelShutdown();
}

void ManiWeaponMgr::LevelInit()
{
	this->CleanUp();
	this->SetupWeapons();
	this->LoadRestrictions();
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		hooked[i] = false;
		ignore_hook[i] = false;
		next_message[i] = 0.0;
	}
}

void ManiWeaponMgr::CleanUp()
{
	for (int i = 0; i < 29; i ++)
	{
		if (weapons[i])
		{
			delete weapons[i];
		}

		weapons[i] = NULL;
	}

	this->alias_list.clear();
}

void ManiWeaponMgr::AddWeapon(const char *search_name, int translation_id)
{	
	int weapon_index = this->FindWeaponIndex(search_name);
	if (weapon_index != -1)
	{
		this->weapons[weapon_index] = new MWeapon(search_name, translation_id, weapon_index);
	}
}

void ManiWeaponMgr::AddWeapon(const char *search_name, int translation_id, const char *alias1)
{	
	int weapon_index = this->FindWeaponIndex(search_name);
	if (weapon_index != -1)
	{
		this->weapons[weapon_index] = new MWeapon(search_name, translation_id, weapon_index);
		this->alias_list[alias1] = this->weapons[weapon_index];
	}
}

void ManiWeaponMgr::AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2)
{	
	int weapon_index = this->FindWeaponIndex(search_name);
	if (weapon_index != -1)
	{
		this->weapons[weapon_index] = new MWeapon(search_name, translation_id, weapon_index);
		this->alias_list[alias1] = this->weapons[weapon_index];
		this->alias_list[alias2] = this->weapons[weapon_index];
	}
}

void ManiWeaponMgr::AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2, const char *alias3)
{	
	int weapon_index = this->FindWeaponIndex(search_name);
	if (weapon_index != -1)
	{
		this->weapons[weapon_index] = new MWeapon(search_name, translation_id, weapon_index);
		this->alias_list[alias1] = this->weapons[weapon_index];
		this->alias_list[alias2] = this->weapons[weapon_index];
		this->alias_list[alias3] = this->weapons[weapon_index];
	}
}

void ManiWeaponMgr::AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2, const char *alias3, const char *alias4)
{	
	int weapon_index = this->FindWeaponIndex(search_name);
	if (weapon_index != -1)
	{
		this->weapons[weapon_index] = new MWeapon(search_name, translation_id, weapon_index);
		this->alias_list[alias1] = this->weapons[weapon_index];
		this->alias_list[alias2] = this->weapons[weapon_index];
		this->alias_list[alias3] = this->weapons[weapon_index];
		this->alias_list[alias4] = this->weapons[weapon_index];
	}
}

void ManiWeaponMgr::SetupWeapons()
{
	this->CleanUp();

	this->AddWeapon("weapon_xm1014", 3000, "xm1014", "autoshotgun", "weapon_xm1014");
	this->AddWeapon("weapon_usp", 3001, "usp", "km45", "weapon_usp");
	this->AddWeapon("weapon_ump45", 3002, "ump45", "weapon_ump45");
	this->AddWeapon("weapon_tmp", 3003, "tmp", "mp", "weapon_tmp");
	this->AddWeapon("weapon_smokegrenade", 3004, "smokegrenade", "sgren", "weapon_smokegrenade");
	this->AddWeapon("weapon_sg552", 3005, "sg552", "krieg552", "weapon_sg552");
	this->AddWeapon("weapon_sg550", 3006, "sg550", "krieg550", "weapon_sg550");
	this->AddWeapon("weapon_scout", 3007, "scout", "weapon_scout");
	this->AddWeapon("weapon_p90", 3008, "p90", "c90", "weapon_p90");
	this->AddWeapon("weapon_p228", 3009, "p228", "228compact", "weapon_p228");
	this->AddWeapon("weapon_mp5navy", 3010, "mp5navy", "mp5", "smg", "weapon_mp5navy");
	this->AddWeapon("weapon_mac10", 3011, "mac10", "weapon_mac10");
	this->AddWeapon("weapon_m4a1", 3012, "m4a1", "weapon_m4a1");
	this->AddWeapon("weapon_m3", 3013, "m3", "12gauge", "weapon_m3");
	this->AddWeapon("weapon_m249", 3014, "m249", "weapon_m249");
	this->AddWeapon("weapon_knife", 0);
	this->AddWeapon("weapon_hegrenade", 3015, "hegrenade", "hegren", "weapon_hegrenade");
	this->AddWeapon("weapon_glock", 3016, "glock", "9x19mm", "weapon_glock");
	this->AddWeapon("weapon_galil", 3017, "galil", "defender", "weapon_galil");
	this->AddWeapon("weapon_g3sg1", 3018, "g3sg1", "d3au1", "weapon_g3sg1");
	this->AddWeapon("weapon_flashbang", 3019, "flashbang", "flash", "weapon_flashbang");
	this->AddWeapon("weapon_fiveseven", 3020, "fiveseven", "fn57", "weapon_fiveseven");
	this->AddWeapon("weapon_famas", 3021, "famas", "clarion", "weapon_famas");
	this->AddWeapon("weapon_elite", 3022, "elite", "weapon_elite");
	this->AddWeapon("weapon_deagle", 3023, "deagle", "nighthawk", "weapon_deagle");
	this->AddWeapon("weapon_c4", 0);
	this->AddWeapon("weapon_awp", 3024, "awp", "magnum", "weapon_awp");
	this->AddWeapon("weapon_aug", 3025, "aug", "bullpup", "weapon_aug");
	this->AddWeapon("weapon_ak47", 3026, "ak47", "cv47", "weapon_ak47");
}

void	ManiWeaponMgr::PlayerSpawn(player_t *player_ptr) 
{
	this->RemoveWeapons(player_ptr, ((mani_weapon_restrict_refund_on_spawn.GetInt() == 0) ? false:true), true);
}

void	ManiWeaponMgr::AutoBuyReBuy()
{
	player_t player;
	player.index = con_command_index + 1;
	if (!FindPlayerByIndex(&player)) return;
	if (player.is_bot) return;
	this->RemoveWeapons(&player, true, false);
	ignore_hook[player.index - 1] = false;
}

void	ManiWeaponMgr::PreAutoBuyReBuy()
{
	ignore_hook[con_command_index] = true;
}

void	ManiWeaponMgr::RemoveWeapons(player_t *player_ptr, bool refund, bool show_refund)
{
	int reason, limit, ratio;

	if (war_mode) return;
	CBaseEntity *pPlayer = EdictToCBE(player_ptr->entity);
	CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
	CBasePlayer *pBase = (CBasePlayer*) pPlayer;
	if (!pCombat) return;

	bool knife_mode = gpManiWarmupTimer->KnivesOnly();
	// Check all weapons
	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if (weapons[i]->GetDisplayID() == 0) continue;
		if (!weapons[i]->CanBuy(player_ptr, 1, reason, limit, ratio) || 
			knife_mode)
		{
			for (int j = 0; j < 40; j++)
			{
				CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_GetWeapon(pCombat, j);
				if (!pWeapon) continue;
				if (strcmp(CBaseCombatWeapon_GetName(pWeapon), weapons[i]->GetWeaponName()) != 0)
				{
					continue;
				}

				CBasePlayer_RemovePlayerItem(pBase, pWeapon);
				if (!knife_mode)
				{
					ShowRestrictReason(player_ptr, weapons[i], reason, limit, ratio);
					ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
				}

				if (refund && !knife_mode)
				{
					CCSWeaponInfo *weapon_info = (CCSWeaponInfo *) CCSGetFileWeaponInfoFromHandle(i);
					if (weapon_info)
					{
						int cash = Prop_GetVal(player_ptr->entity, MANI_PROP_ACCOUNT,0);
						cash += weapon_info->dynamic_price;
						if (cash > 16000) cash = 16000;
						Prop_SetVal(player_ptr->entity, MANI_PROP_ACCOUNT, cash);
						if (show_refund)
						{
							OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 3042, "%i", weapon_info->dynamic_price));
						}
					}
				}

				// Switch to knife
				pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 2);
				if (pWeapon)
				{
					CBaseCombatCharacter_Weapon_Switch(pCombat, pWeapon, 0);
				}

				break;
			}
		}
	}
}

void ManiWeaponMgr::RoundStart()
{
	if (war_mode) return;
	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if ( weapons[i]->GetDisplayID() == 0 ) continue;
		if ( !weapons[i]->IsRestricted() ) continue;

		int round_ratio = weapons[i]->GetRoundRatio();

		// Round ratio not in use
		if (round_ratio == 0) continue;

		// If ratio still too close then continue
		int score_diff = gpManiTeam->GetTeamScore(2) - gpManiTeam->GetTeamScore(3);

		if (abs(score_diff) < round_ratio)
		{
			continue;
		}

		SayToAll(GREEN_CHAT, false, "%s", 
			Translate(NULL, 3043, "%s%s%i", 
			(score_diff < 0) ? "CT":"T", Translate(NULL, weapons[i]->GetDisplayID()), 
			round_ratio));
	}
}

PLUGIN_RESULT	ManiWeaponMgr::CanBuy(player_t *player_ptr, const char *alias_name)
{
	int reason, limit, ratio;

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;
	if (war_mode) return PLUGIN_CONTINUE;
	if (gpManiWarmupTimer->KnivesOnly()) return PLUGIN_STOP;

	char lower_alias[32];

	int length = strlen(alias_name);
	length = ( length > 30 ) ? 30 : length; // cut off any attempts to do a weapon name > 30 characters

	for (int i = 0; i <= length; i++)
	{
		lower_alias[i] = tolower(alias_name[i]);
	}
	
	MWeapon *weapon = NULL;
	std::map <BasicStr, MWeapon *>::iterator itr;
	bool more_than_one_weapon = false;
	for (itr = alias_list.begin(); itr != alias_list.end(); ++itr)
	{
		if (V_stristr(itr->first.str, lower_alias) != NULL)
		{
			if (weapon == NULL)
			{
				// Found first weapon match
				weapon = itr->second;
			}
			else
			{
				// Another weapon alias matched, check if the MWeapon ptr
				// found matches the previous one
				if (weapon != itr->second)
				{
					// Two different weapons found in buy string
					more_than_one_weapon = true;
					break;
				}
			}
		}
	}

	if (weapon == NULL)
	{
		return PLUGIN_CONTINUE;
	}

    // If player tried to have multiple weapons in the buy string then stop
    // the transaction from happening
    if (more_than_one_weapon)
    {
        ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
        return PLUGIN_STOP;                
    }
    
	// Check if player has enough cash anyway
	CCSWeaponInfo *weapon_info = (CCSWeaponInfo *) CCSGetFileWeaponInfoFromHandle(weapon->GetWeaponIndex());
	if (weapon_info)
	{
		int cash = Prop_GetVal(player_ptr->entity, MANI_PROP_ACCOUNT,0);
		if (cash < weapon_info->dynamic_price)
		{
			// Not enough money so continue anyway
			return PLUGIN_CONTINUE;
		}
	}

	if (weapon->CanBuy(player_ptr, 0, reason, limit, ratio))
	{
		return PLUGIN_CONTINUE;
	}

	ProcessPlayActionSound(player_ptr, MANI_ACTION_SOUND_RESTRICTWEAPON);
	ShowRestrictReason(player_ptr, weapon, reason, limit, ratio);
	return PLUGIN_STOP;
} 


void	ManiWeaponMgr::LoadRestrictions()
{
	FileHandle_t file_handle;
	char	weapon_name[128];
	char	restrict_filename[256];

	snprintf(restrict_filename, sizeof(restrict_filename), "./cfg/%s/restrict/%s_restrict.txt", mani_path.GetString(), current_map);
	file_handle = filesystem->Open (restrict_filename,"rt",NULL);
	if (file_handle)
	{
		while (filesystem->ReadLine (weapon_name, sizeof(weapon_name), file_handle) != NULL)
		{
			if (!ParseLine(weapon_name, true, false))
			{
				// String is empty after parsing
				continue;
			}

			int length = strlen(weapon_name);
			for (int i = 0; i <= length; i++)
			{
				weapon_name[i] = tolower(weapon_name[i]);
			}

			MWeapon *weapon = alias_list[weapon_name];
			if (weapon == NULL)
			{
				MMsg("In file [%s], weapon [%s] is not valid\n", restrict_filename, weapon_name);
				continue;
			}

			weapon->SetRestricted(true);
			weapon->SetTeamLimit(0);
			weapon->SetRoundRatio(0);
		}

		filesystem->Close(file_handle);
	}

	//Get weapons restriction list for this map
	snprintf( restrict_filename, sizeof(restrict_filename), "./cfg/%s/default_weapon_restrict.txt", mani_path.GetString());
	file_handle = filesystem->Open (restrict_filename,"rt",NULL);
	if (file_handle == NULL)
	{
	}
	else
	{
		while (filesystem->ReadLine (weapon_name, sizeof(weapon_name), file_handle) != NULL)
		{
			if (!ParseLine(weapon_name, true, false))
			{
				// String is empty after parsing
				continue;
			}

			int length = strlen(weapon_name);
			for (int i = 0; i <= length; i++)
			{
				weapon_name[i] = tolower(weapon_name[i]);
			}

			MWeapon *weapon = alias_list[weapon_name];
			if (weapon == NULL)
			{
				MMsg("In file [%s], weapon [%s] is not valid\n", restrict_filename, weapon_name);
				continue;
			}

			weapon->SetRestricted(true);
			weapon->SetTeamLimit(0);
			weapon->SetRoundRatio(0);
		}

		filesystem->Close(file_handle);
	}
}

void	ManiWeaponMgr::RestrictAll()
{
	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if (weapons[i]->GetDisplayID() == 0) continue;
		weapons[i]->SetRestricted(true);
		weapons[i]->SetTeamLimit(0);
		weapons[i]->SetRoundRatio(0);
	}
}

void	ManiWeaponMgr::UnRestrictAll()
{
	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if (weapons[i]->GetDisplayID() == 0) continue;
		weapons[i]->SetRestricted(false);
		weapons[i]->SetTeamLimit(0);
		weapons[i]->SetRoundRatio(0);
	}
}

bool	ManiWeaponMgr::SetWeaponRestriction
(
 const char *weapon_name, 
 bool restricted, 
 int team_limit
 )
{

	char lower_alias[32];

	int length = strlen(weapon_name);
	if (length > 30) return true;

	for (int i = 0; i <= length; i++)
	{
		lower_alias[i] = tolower(weapon_name[i]);
	}

	MWeapon *weapon = alias_list[lower_alias];
	if (!weapon)
	{
		// No weapon found so check other names for weapons
		for (int i = 0; i < 29; i ++)
		{
			if (stricmp(weapons[i]->GetWeaponName(), weapon_name) == 0)
			{
				weapon = weapons[i];
				break;
			}

			if (stricmp(Translate(NULL, weapons[i]->GetDisplayID()), weapon_name) == 0)
			{
				weapon = weapons[i];
				break;
			}
		}

		if (!weapon)
		{
			return false;
		}
	}

	weapon->SetRestricted(restricted);
	weapon->SetTeamLimit(team_limit);
	weapon->SetRoundRatio(0);
	return true;
}

bool	ManiWeaponMgr::SetWeaponRatio
(
 const char *weapon_name, 
 int ratio
 )
{
	char lower_alias[32];

	int length = strlen(weapon_name);
	if (length > 30) return true;

	for (int i = 0; i <= length; i++)
	{
		lower_alias[i] = tolower(weapon_name[i]);
	}

	MWeapon *weapon = alias_list[lower_alias];
	if (!weapon)
	{
		// No weapon found so check other names for weapons
		for (int i = 0; i < 29; i ++)
		{
			if (strcmp(weapons[i]->GetWeaponName(), weapon_name) == 0)
			{
				weapon = weapons[i];
				break;
			}

			if (strcmp(Translate(NULL, weapons[i]->GetDisplayID()), weapon_name) == 0)
			{
				weapon = weapons[i];
				break;
			}
		}

		if (!weapon)
		{
			return false;
		}
	}

	weapon->SetRoundRatio(ratio);
	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Handle the restrict weapon menu
//---------------------------------------------------------------------------------
int RestrictWeaponItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	int i;
	if (this->params.GetParam("index", &i))
	{
		if (gpManiWeaponMgr->weapons[i]->IsRestricted())
		{
			if (gpManiWeaponMgr->weapons[i]->GetTeamLimit() >= 5)
			{
				gpCmd->NewCmd();
				gpCmd->AddParam("ma_unrestrict");
				gpCmd->AddParam("%s", gpManiWeaponMgr->weapons[i]->GetWeaponName());
				gpManiWeaponMgr->ProcessMaUnRestrict(player_ptr, "ma_unrestrict", 0, M_MENU);
				return REPOP_MENU;
			}
			else
			{
				int new_count = gpManiWeaponMgr->weapons[i]->GetTeamLimit() + 1;
				gpCmd->NewCmd();
				gpCmd->AddParam("ma_restrict");
				gpCmd->AddParam("%s", gpManiWeaponMgr->weapons[i]->GetWeaponName());
				gpCmd->AddParam("%i", new_count);
				gpManiWeaponMgr->ProcessMaRestrict(player_ptr, "ma_restrict", 0, M_MENU);
			}
		}
		else
		{
			gpCmd->NewCmd();
			gpCmd->AddParam("ma_restrict");
			gpCmd->AddParam("%s", gpManiWeaponMgr->weapons[i]->GetWeaponName());
			gpCmd->AddParam("0");
			gpManiWeaponMgr->ProcessMaRestrict(player_ptr, "ma_restrict", 0, M_MENU);
		}
	}

	return REPOP_MENU;
}

bool RestrictWeaponPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 530));
	this->SetTitle("%s", Translate(player_ptr, 531));

	// Setup weapon list
	for( int i = 0; i < 29; i++ )
	{
		if ( !gpManiWeaponMgr->weapons[i] ) continue;
		if (gpManiWeaponMgr->weapons[i]->GetDisplayID() == 0) continue;

		MenuItem *ptr = new RestrictWeaponItem();
		if (!gpManiWeaponMgr->weapons[i]->IsRestricted())
		{
			ptr->SetDisplayText("%s", Translate(player_ptr, gpManiWeaponMgr->weapons[i]->GetDisplayID()));	
		}
		else
		{
			ptr->SetDisplayText("* %s <%i>", 
				Translate(player_ptr, gpManiWeaponMgr->weapons[i]->GetDisplayID()), 
				gpManiWeaponMgr->weapons[i]->GetTeamLimit());
		}

		ptr->SetHiddenText("%s", Translate(player_ptr, gpManiWeaponMgr->weapons[i]->GetDisplayID()));
		ptr->params.AddParam("index", i);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;

}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_showrestrict
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaShowRestrict(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	OutputToConsole(player_ptr, "Current weapons and their restrictions\n\n");
	OutputToConsole(player_ptr, "Weapon Alias                  Restricted  Limit  Ratio\n");
	OutputToConsole(player_ptr, "------------------------------------------------------\n");

	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if (weapons[i]->GetDisplayID() == 0) continue;
		OutputToConsole(player_ptr, "%-29s %-11s %i      %i\n", 
					Translate(player_ptr, weapons[i]->GetDisplayID()), 
					(weapons[i]->IsRestricted()) ? Translate(player_ptr, M_MENU_YES):Translate(player_ptr, M_MENU_NO),
					weapons[i]->GetTeamLimit(),
					weapons[i]->GetRoundRatio());

	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_restrict command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaRestrict
(
 player_t *player_ptr, 
 const char	*command_name, 
 const int	help_id, 
 const int	command_type
 )
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	int limit = 0;
	if (gpCmd->Cmd_Argc() == 3)
	{
		// Validate negative
		limit = atoi(gpCmd->Cmd_Argv(2));
		if (limit < 0)
		{
			return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
		}

		if (!this->SetWeaponRestriction(gpCmd->Cmd_Argv(1), true, limit))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3044, "%s", gpCmd->Cmd_Argv(1)));
			return PLUGIN_STOP;
		}
	}
	else
	{
		if (!this->SetWeaponRestriction(gpCmd->Cmd_Argv(1), true))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3044, "%s", gpCmd->Cmd_Argv(1)));
			return PLUGIN_STOP;
		}
	}

	LogCommand (player_ptr, "restrict [%s]\n", gpCmd->Cmd_Argv(1));
	if (gpCmd->Cmd_Argc() == 2)
	{
		SayToAll(GREEN_CHAT, true, "%s", Translate(player_ptr, 3045, "%s", gpCmd->Cmd_Argv(1)));
	}
	else
	{
		SayToAll(GREEN_CHAT, true, "%s", Translate(player_ptr, 3040, "%s%i", gpCmd->Cmd_Argv(1), limit));
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_restrict_ratio command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaRestrictRatio
(
 player_t *player_ptr, 
 const char	*command_name, 
 const int	help_id, 
 const int	command_type
 )
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Validate negative
	int ratio = atoi(gpCmd->Cmd_Argv(2));
	if (ratio < 0)
	{
		return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}

	if (!this->SetWeaponRatio(gpCmd->Cmd_Argv(1), ratio))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3044, "%s", gpCmd->Cmd_Argv(1)));
		return PLUGIN_STOP;
	}

	LogCommand (player_ptr, "restrict ratio [%s] [%s]\n", gpCmd->Cmd_Argv(1), gpCmd->Cmd_Argv(2));
	SayToAll(GREEN_CHAT, true, "%s", Translate(player_ptr, 3054, "%s%i", gpCmd->Cmd_Argv(1), ratio));

	return PLUGIN_STOP;
}
//---------------------------------------------------------------------------------
// Purpose: Process the ma_unrestrict command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaUnRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// check if valid weapon name
	if (!this->SetWeaponRestriction(gpCmd->Cmd_Argv(1), false))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, 3044, "%s", gpCmd->Cmd_Argv(1)));
		return PLUGIN_STOP;
	}

	// Un-restrict weapon
	LogCommand (player_ptr, "un-restrict [%s]\n", gpCmd->Cmd_Argv(1));
	SayToAll(GREEN_CHAT, true, "%s", Translate(player_ptr, 3047, "%s", gpCmd->Cmd_Argv(1)));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unrestrictall command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaUnRestrictAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	// UnRestrict all the weapons
	this->UnRestrictAll();

	LogCommand (player_ptr, "unrestricted all weapons\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3048));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_unrestrictall command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaRestrictAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	// Restrict All weapons
	this->RestrictAll();

	LogCommand (player_ptr, "restricted all weapons\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3049));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_knives
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaKnives(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	this->RestrictAll();

	LogCommand (player_ptr, "Only knives can be used next round !!!\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3050));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_pistols
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaPistols(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	this->RestrictAll();
	this->SetWeaponRestriction("glock", false);
	this->SetWeaponRestriction("usp", false);
	this->SetWeaponRestriction("p228", false);
	this->SetWeaponRestriction("deagle", false);
	this->SetWeaponRestriction("elite", false);

	LogCommand (player_ptr, "Only pistols can be used next round !!!\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3051));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_shotguns
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaShotguns(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	this->RestrictAll();
	this->SetWeaponRestriction("m3", false);
	this->SetWeaponRestriction("xm1014", false);

	LogCommand (player_ptr, "Only shotguns can be used next round !!!\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3052));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_nosnipers
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiWeaponMgr::ProcessMaNoSnipers(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_RESTRICT_WEAPON, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	this->UnRestrictAll();
	this->SetWeaponRestriction("awp", true);
	this->SetWeaponRestriction("g3sg1", true);
	this->SetWeaponRestriction("sg550", true);
	this->SetWeaponRestriction("scout", true);

	LogCommand (player_ptr, "No sniper weapons next round !!!\n");
	SayToAll(GREEN_CHAT, true, "%s", Translate(NULL, 3053));

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Find weapon index from search name
//---------------------------------------------------------------------------------
int ManiWeaponMgr::FindWeaponIndex(const char *search_name)
{
	int start = 0;

	for ( start = 0; start < 29; start++ ) {
		CCSWeaponInfo *weapon_info = (CCSWeaponInfo *) CCSGetFileWeaponInfoFromHandle(start);
		if (weapon_info->weapon_name[0] != 0)
			break;
	}

	if ( start == 29 ) return -1;

	for (int i = start; i < start+29; i++)
	{
		CCSWeaponInfo *weapon_info = (CCSWeaponInfo *) CCSGetFileWeaponInfoFromHandle(i);
		if (weapon_info == NULL)
		{
			return -1;
		}

		if (strcmp(search_name, weapon_info->weapon_name) == 0) 
		{
			return i-start;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Check if player can pickup weapon or not
//---------------------------------------------------------------------------------
bool	ManiWeaponMgr::CanPickUpWeapon(CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon)
{
	if (war_mode) return true;
	if (mani_weapon_restrict_prevent_pickup.GetInt() == 0) return true;
	if (gpManiWarmupTimer->KnivesOnly()) return true;

	edict_t *pEdict = serverents->BaseEntityToEdict((CBasePlayer *) pPlayer);
	if (!pEdict) return true;

	int index = engine->IndexOfEdict(pEdict);
	if (index < 1 || index > max_players) return true;

	if (ignore_hook[index - 1])
	{
		return true;
	}

	const char *weapon_name = CBaseCombatWeapon_GetName(pWeapon);
	// Find index
	for (int i = 0; i < 29; i++)
	{
		if ( !weapons[i] ) break; // for DS tool and listen servers ... order of processing different
		if (strcmp(weapons[i]->GetWeaponName(), weapon_name) != 0) continue;
		if (weapons[i]->GetDisplayID() == 0) continue;

		player_t player;
		player.index = index;

		if (!FindPlayerByIndex(&player)) break;

		int reason, limit, ratio;
		if (!weapons[i]->CanBuy(&player, 0, reason, limit, ratio))
		{
			if (next_message[player.index - 1] < gpGlobals->curtime)
			{
				ShowRestrictReason(&player, weapons[i], reason, limit, ratio);
				next_message[player.index - 1] = gpGlobals->curtime + 1.2;
			}

			return false;
		}

		break;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Find weapon index from search name
//---------------------------------------------------------------------------------
void	ManiWeaponMgr::ShowRestrictReason(player_t *player_ptr, MWeapon *weapon, int reason, int limit, int ratio)
{
	switch(reason)
	{
	case WEAPON_RESTRICT:
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", 
			Translate(player_ptr, 3041, "%s", Translate(player_ptr, weapon->GetDisplayID())));
		break;
	case WEAPON_LIMIT:
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", 
			Translate(player_ptr, 3040, "%s%i", Translate(player_ptr, weapon->GetDisplayID()), limit));
		break;
	case WEAPON_RATIO:
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", 
			Translate(player_ptr, 3043, "%s%s%i", 
			(player_ptr->team == 3) ? "CT":"T", Translate(NULL, weapon->GetDisplayID()), 
			ratio));

	default:
		break;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiWeaponMgr::ClientActive(player_t *player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_CANUSE) == -1) return;
	if (player_ptr->is_bot) return;
	if (!hooked[player_ptr->index - 1])
	{
		g_ManiSMMHooks.HookWeapon_CanUse((CBasePlayer *) EdictToCBE(player_ptr->entity));
		hooked[player_ptr->index - 1] = true;
		ignore_hook[player_ptr->index - 1] = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiWeaponMgr::ClientDisconnect(player_t	*player_ptr)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_CANUSE) == -1) return;
	if (player_ptr->is_bot) return;
	if (hooked[player_ptr->index - 1])
	{
		g_ManiSMMHooks.UnHookWeapon_CanUse((CBasePlayer *) EdictToCBE(player_ptr->entity));
		hooked[player_ptr->index - 1] = false;
		ignore_hook[player_ptr->index - 1] = false;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level Shutdown
//---------------------------------------------------------------------------------
void	ManiWeaponMgr::LevelShutdown(void)
{
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		if (hooked[i])
		{
			player_t player;

			player.index = i + 1;
			if (FindPlayerByIndex(&player))
			{
				g_ManiSMMHooks.UnHookWeapon_CanUse((CBasePlayer *) EdictToCBE(player.entity));
			}

			hooked[i] = false;
			ignore_hook[i] = false;
			next_message[i] = 0.0;
		}
	}
}

SCON_COMMAND(ma_showrestrict, 2139, MaShowRestrict, false);
SCON_COMMAND(ma_restrict, 2141, MaRestrict, false);
SCON_COMMAND(ma_knives, 2143, MaKnives, false);
SCON_COMMAND(ma_pistols, 2145, MaPistols, false);
SCON_COMMAND(ma_shotguns, 2147, MaShotguns, false);
SCON_COMMAND(ma_nosnipers, 2149, MaNoSnipers, false);
SCON_COMMAND(ma_unrestrict, 2151, MaUnRestrict, false);
SCON_COMMAND(ma_unrestrictall, 2153, MaUnRestrictAll, false);
SCON_COMMAND(ma_restrictall, 2225, MaRestrictAll, false);
SCON_COMMAND(ma_restrictratio, 2229, MaRestrictRatio, false);

CON_COMMAND(ma_listweapons, "Debug Tool")
{
	for (int i = 0; i < 29; i++)
	{
		CCSWeaponInfo *weapon_info = (CCSWeaponInfo *) CCSGetFileWeaponInfoFromHandle(i);
		if (weapon_info == NULL)
		{
			MMsg("Sigscan failed for CCSGetFileWeaponInfoFromHandle\n");
			return;
		}

		MMsg("Weapon name [%s] Price [%i]\n", weapon_info->weapon_name, weapon_info->dynamic_price);
	}	
}

ManiWeaponMgr	g_ManiWeaponMgr;
ManiWeaponMgr	*gpManiWeaponMgr;
