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
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_maps.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_output.h"
#include "mani_language.h"
#include "mani_customeffects.h"
#include "mani_vfuncs.h"
#include "mani_sigscan.h"
#include "mani_warmuptimer.h"
#include "mani_gametype.h"
#include "mani_vars.h"
#include "mani_weapon.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IGameEventManager2 *gameeventmanager;

extern	bool war_mode;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern  ConVar	*mp_friendlyfire; 

CONVAR_CALLBACK_PROTO(ManiWarmupTimerCVar);
CONVAR_CALLBACK_PROTO(ManiWarmupItem1);
CONVAR_CALLBACK_PROTO(ManiWarmupItem2);
CONVAR_CALLBACK_PROTO(ManiWarmupItem3);
CONVAR_CALLBACK_PROTO(ManiWarmupItem4);
CONVAR_CALLBACK_PROTO(ManiWarmupItem5);

ConVar mani_warmup_timer_show_countdown ("mani_warmup_timer_show_countdown", "1", 0, "1 = enable center say countdown, 0 = disable", true, 0, true, 1);
ConVar mani_warmup_timer_knives_only ("mani_warmup_timer_knives_only", "0", 0, "1 = enable knives only mode, 0 = all weapons allowed", true, 0, true, 1);
ConVar mani_warmup_timer_knives_respawn ("mani_warmup_timer_knives_respawn", "0", 0, "1 = enable respawn in knife mode, 0 = no respawn", true, 0, true, 1);
ConVar mani_warmup_timer ("mani_warmup_timer", "0", 0, "Time in seconds at the start of a map before performing mp_restartgame (0 = off)", true, 0, true, 360, CONVAR_CALLBACK_REF(ManiWarmupTimerCVar));
ConVar mani_warmup_timer_ignore_tk ("mani_warmup_timer_ignore_tk", "0", 0, "0 = tk punishment still allowed, 1 = no tk punishments", true, 0, true, 1);
ConVar mani_warmup_timer_disable_ff ("mani_warmup_timer_disable_ff", "0", 0, "0 = Do not disable friendly fire during warmup, 1 = If friendly fire was turned on, the plugin will disable it during the warmup round", true, 0, true, 1);
ConVar mani_warmup_timer_knives_only_ignore_fyi_aim_maps ("mani_warmup_timer_knives_only_ignore_fyi_aim_maps", "0", 0, "0 = knive mode still allowed on fy/aim maps, 1 = no knive mode for fy_/aim_ maps", true, 0, true, 1);
ConVar mani_warmup_timer_unlimited_grenades ("mani_warmup_timer_unlimited_grenades", "0", 0, "1 = enable unlimited he grenades, 0 = disable unlimited he's", true, 0, true, 1);
ConVar mani_warmup_timer_spawn_item_1 ("mani_warmup_timer_spawn_item_1", "item_assaultsuit", 0, "Item to spawn with in warmup mode", CONVAR_CALLBACK_REF(ManiWarmupItem1));
ConVar mani_warmup_timer_spawn_item_2 ("mani_warmup_timer_spawn_item_2", "", 0, "Item to spawn with in warmup mode", CONVAR_CALLBACK_REF(ManiWarmupItem2));
ConVar mani_warmup_timer_spawn_item_3 ("mani_warmup_timer_spawn_item_3", "", 0, "Item to spawn with in warmup mode", CONVAR_CALLBACK_REF(ManiWarmupItem3));
ConVar mani_warmup_timer_spawn_item_4 ("mani_warmup_timer_spawn_item_4", "", 0, "Item to spawn with in warmup mode", CONVAR_CALLBACK_REF(ManiWarmupItem4));
ConVar mani_warmup_timer_spawn_item_5 ("mani_warmup_timer_spawn_item_5", "", 0, "Item to spawn with in warmup mode", CONVAR_CALLBACK_REF(ManiWarmupItem5));
ConVar mani_warmup_in_progress ("mani_warmup_in_progress", "0", 0, "Used by LDuke VIP mod to detect when warmup mode in operation", true, 0, true, 1);
ConVar mani_warmup_infinite_ammo ("mani_warmup_infinite_ammo", "0", 0, "Infinite ammo, 0 = disabled, 1 = enabled", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiGameType
//class ManiGameType
//{

ManiWarmupTimer::ManiWarmupTimer()
{
	// Init
	check_timer = false;
	next_check = -999.0;
	gpManiWarmupTimer = this;

	for (int i = 0; i < 5; i ++)
	{
		item_name[i][0] = '\0';
	}
}

ManiWarmupTimer::~ManiWarmupTimer()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::LevelInit(void)
{
	friendly_fire = false;
	if (mani_warmup_timer.GetInt() == 0)
	{
		check_timer = false;
		fire_restart = false;
		mani_warmup_in_progress.SetValue(0);
	}
	else
	{
		check_timer = true;
		fire_restart = true;
		next_check = -999.0;
		mani_warmup_in_progress.SetValue(1);
	}

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		respawn_list[i].needs_respawn = false;
	}

	for (int i = 0; i < 5; i ++)
	{
		item_name[i][0] = '\0';
	}

	SetRandomItem(&mani_warmup_timer_spawn_item_1, 0);
	SetRandomItem(&mani_warmup_timer_spawn_item_2, 1);
	SetRandomItem(&mani_warmup_timer_spawn_item_3, 2);
	SetRandomItem(&mani_warmup_timer_spawn_item_4, 3);
	SetRandomItem(&mani_warmup_timer_spawn_item_5, 4);
}

//---------------------------------------------------------------------------------
// Purpose: Round Started
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::RoundStart(void)
{
	if (war_mode) return;
	if (mani_warmup_timer.GetInt() == 0) return;
	if (!check_timer) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.player_info->IsHLTV()) continue;

		CBaseEntity *pPlayer = EdictToCBE(player.entity);
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBasePlayer *pBase = (CBasePlayer*) pPlayer;
		if (!pCombat) continue;

		for (int j = 0; j < 29; j++)
		{
			CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_OwnsThisType(pCombat, gpManiWeaponMgr->GetWeaponName(j), 0);
			if (!pWeapon) continue;

			if (strcmp(CBaseCombatWeapon_GetName(pWeapon), "weapon_c4") != 0)
			{
				continue;
			}

			CBasePlayer_RemovePlayerItem(pBase, pWeapon);

			// Switch to knife
			pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 2);
			if (pWeapon)
			{
				CBaseCombatCharacter_Weapon_Switch(pCombat, pWeapon, 0);
			}
		}
	}

	CBaseEntity *weaponc4 = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(NULL, "weapon_c4");
	if (weaponc4)
	{
		CCSUTILRemove(weaponc4);
	}

	CUtlVector<CBaseEntity*> hostages;
	CBaseEntity *hostage = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(NULL, "hostage_entity");

	while (hostage) 
	{
		hostages.AddToTail(hostage);
		hostage = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(hostage, "hostage_entity");
	}

	for (int x = 0; x < hostages.Count(); x++) 
	{
		CCSUTILRemove(hostages[x]);
	}
}	


//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::PlayerSpawn(player_t *player_ptr)
{
	if (war_mode) return;
	if (mani_warmup_timer.GetInt() == 0) return;
	if (!check_timer) return;
	if (!gpManiGameType->IsValidActiveTeam(player_ptr->team)) return;
	if (fire_restart == false) return;

	respawn_list[player_ptr->index - 1].needs_respawn = false;

	if (player_ptr->is_bot)
	{
		if (mani_warmup_timer_knives_only.GetInt() == 1)
		{
			// Set cash to zero and strip weapons
			Prop_SetVal(player_ptr->entity, MANI_PROP_ACCOUNT, 0);
			CBaseEntity *pPlayer = player_ptr->entity->GetUnknown()->GetBaseEntity();
			CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);

			CBaseCombatWeapon *pWeapon1 = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 0);
			CBaseCombatWeapon *pWeapon2 = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
			CBasePlayer *pBase = (CBasePlayer*) pPlayer;
			if (pWeapon1)
			{
				CBasePlayer_RemovePlayerItem(pBase, pWeapon1);
			}

			if (pWeapon2)
			{
				CBasePlayer_RemovePlayerItem(pBase, pWeapon2);
			}

			CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 2);
			if (pWeapon)
			{
				CBaseCombatCharacter_Weapon_Switch(pCombat, pWeapon, 0);
			}

			CBaseEntity *weaponc4 = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(NULL, "weapon_c4");
			if (weaponc4)
			{
				CCSUTILRemove(weaponc4);
			}
		}
	}

	// Unlimited grenades
	if (mani_warmup_timer_unlimited_grenades.GetInt() == 1 && gpManiGameType->IsGameType(MANI_GAME_CSS))
	{
		GiveItem(player_ptr->entity, "weapon_hegrenade");
	}

	for (int i = 0; i < 5; i ++)
	{
		if (item_name[i][0] != '\0')
		{
			// Ignore assault suit if not CSS
			if (i == 0 && 
				!gpManiGameType->IsGameType(MANI_GAME_CSS) &&
				strcmp(item_name[i], "item_assaultsuit") == 0)
			{
				continue;
			}

			GiveItem(player_ptr->entity, item_name[i]);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level has initialised
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::GiveItem(edict_t *pEntity, const char	*item_name)
{
	CBasePlayer_GiveNamedItem((CBasePlayer *) EdictToCBE(pEntity), item_name);
}

//---------------------------------------------------------------------------------
// Purpose: Game Frame has been run
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::GameFrame(void)
{

	if (war_mode) return;
	if (!check_timer) return;
	if (ProcessPluginPaused()) return;

	if (mp_friendlyfire && mp_friendlyfire->GetInt() != 0 && mani_warmup_timer_disable_ff.GetInt() == 1)
		{
		friendly_fire = true;
		mp_friendlyfire->SetValue(0);
		}

	if (gpGlobals->curtime > next_check)
	{
		if (mani_warmup_timer_show_countdown.GetInt())
		{
			int time_left = mani_warmup_timer.GetInt() - ((int) gpGlobals->curtime);
			CSayToAll("Warmup timer %i", time_left);
		}

		next_check = gpGlobals->curtime + 1.0;
		if (gpGlobals->curtime > mani_warmup_timer.GetFloat())
		{
			check_timer = false;
			mani_warmup_in_progress.SetValue(0);
			if (friendly_fire && mani_warmup_timer_disable_ff.GetInt() == 1)
			{
				// Reset Friendly Fire value
				mp_friendlyfire->SetValue(1);
			}
		}

		if (fire_restart && gpGlobals->curtime > mani_warmup_timer.GetFloat() - 1)
		{
			engine->ServerCommand("mp_restartgame 1\n");
			fire_restart = false;
		}

		if (gpManiGameType->IsGameType(MANI_GAME_CSS) &&
			mani_warmup_timer_knives_only.GetInt() != 0 && 
			mani_warmup_timer_knives_respawn.GetInt() == 1 &&
			mani_warmup_timer_unlimited_grenades.GetInt() == 0)
		{
			for (int i = 0; i < max_players; i++)
			{
				if (!respawn_list[i].needs_respawn) continue;

				if (respawn_list[i].time_to_respawn < gpGlobals->curtime)
				{
					respawn_list[i].needs_respawn = false;

					player_t player;
					player.index = i + 1;
					if (!FindPlayerByIndex(&player)) continue;

					if (player.team != 2 && player.team != 3) return;

					// Remove rag doll if there
					//				CBaseEntity *pRagDoll = Prop_GetRagDoll(player.entity);
					//				if (pRagDoll)
					//				{
					//					CCSUTILRemove(pRagDoll);
					//				}

					CUtlVector<CBaseEntity*> ragdolls;
					CBaseEntity *ragdoll = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(NULL, "cs_ragdoll");

					while (ragdoll) 
					{
						ragdolls.AddToTail(ragdoll);
						ragdoll = (CBaseEntity*)CGlobalEntityList_FindEntityByClassname(ragdoll, "cs_ragdoll");
					}

					for (int x = 0; x < ragdolls.Count(); x++) 
					{
						CCSUTILRemove(ragdolls[x]);
					}

					CBaseEntity *pCBE = EdictToCBE(player.entity);
					if (pCBE)
					{
						CCSRoundRespawn(pCBE);
					}
				}
			}
		}

		if (gpManiGameType->IsGameType(MANI_GAME_CSS) &&
			mani_warmup_infinite_ammo.GetInt() == 1)
		{
			this->GiveAllAmmo(); 
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Unlimited HE's ?
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::PlayerDeath(player_t *player_ptr)
{
	if (war_mode) return;
	if (!check_timer) return;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	if (mani_warmup_timer_knives_only.GetInt() == 0) return;
	if (mani_warmup_timer_knives_respawn.GetInt() == 0) return;
	if (mani_warmup_timer_unlimited_grenades.GetInt() == 1) return;
	if (mani_warmup_timer_knives_respawn.GetInt() == 0) return;

	if (player_ptr->team != 2 && player_ptr->team != 3) return;

	// Setup respawn of player
	respawn_list[player_ptr->index - 1].needs_respawn = true;
	respawn_list[player_ptr->index - 1].time_to_respawn = gpGlobals->curtime + 1.0;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Unlimited HE's ?
//---------------------------------------------------------------------------------
PLUGIN_RESULT		ManiWarmupTimer::JoinClass(edict_t *pEdict)
{
	if (war_mode) return PLUGIN_CONTINUE;
	if (!check_timer) return PLUGIN_CONTINUE;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return PLUGIN_CONTINUE;
	if (mani_warmup_timer_knives_only.GetInt() == 0) return PLUGIN_CONTINUE;
	if (mani_warmup_timer_unlimited_grenades.GetInt() == 1) return PLUGIN_CONTINUE;
	if (mani_warmup_timer_knives_respawn.GetInt() == 0) return PLUGIN_CONTINUE;

	player_t player;

	player.entity = pEdict; 
	if (!FindPlayerByEntity(&player)) return PLUGIN_CONTINUE;
	if (player.team != 2 && player.team != 3) return PLUGIN_CONTINUE;

	// Setup respawn of player
	respawn_list[player.index - 1].needs_respawn = true;
	respawn_list[player.index - 1].time_to_respawn = gpGlobals->curtime + 1.0;
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: Unlimited HE's ?
//---------------------------------------------------------------------------------
bool		ManiWarmupTimer::UnlimitedHE(void)
{
	if (!check_timer) return false;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;
	if (mani_warmup_timer_unlimited_grenades.GetInt() == 0) return false;

	// Unlimited HE mode
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Knives only ?
//---------------------------------------------------------------------------------
bool		ManiWarmupTimer::KnivesOnly(void)
{
	if (!check_timer) return false;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;
	if (mani_warmup_timer_knives_only.GetInt() == 0) return false;

	// Knife mode enabled, check if we need to see what type of map this
	// is

	if (mani_warmup_timer_knives_only_ignore_fyi_aim_maps.GetInt() == 1)
	{
		// Dont care about map type
		return true;
	}

	int	length = Q_strlen(current_map);

	if (length > 2)
	{
		if (current_map[2] == '_' &&
			(current_map[1] == 'y' || current_map[1] == 'Y') &&
			(current_map[0] == 'f' || current_map[0] == 'F'))
		{
			return false;
		}
	}
	
	if (length > 3)
	{
		if (current_map[3] == '_' &&
			(current_map[2] == 'm' || current_map[2] == 'M') &&
			(current_map[1] == 'i' || current_map[1] == 'I') &&
			(current_map[0] == 'a' || current_map[0] == 'A'))
		{
			return false;
		}

		if (current_map[3] == '_' &&
			(current_map[2] == 'p' || current_map[2] == 'A') &&
			(current_map[1] == 'w' || current_map[1] == 'W') &&
			(current_map[0] == 'a' || current_map[0] == 'P'))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Ignore TK punishments
//---------------------------------------------------------------------------------
bool		ManiWarmupTimer::IgnoreTK(void)
{
	if (!check_timer) return false;
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return false;

	return ((mani_warmup_timer_ignore_tk.GetInt() == 0) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Give everyone ammo
//---------------------------------------------------------------------------------
void		ManiWarmupTimer::GiveAllAmmo(void)
{
	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_dead) continue;
		if (player.player_info->IsHLTV()) continue;

		CBaseEntity *pPlayer = EdictToCBE(player.entity);
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 0);
		if (pWeapon)
		{
			int	ammo_index;
			ammo_index = CBaseCombatWeapon_GetPrimaryAmmoType(pWeapon);
			CBaseCombatCharacter_GiveAmmo(pCombat, 999, ammo_index, true);
			ammo_index = CBaseCombatWeapon_GetSecondaryAmmoType(pWeapon);
			CBaseCombatCharacter_GiveAmmo(pCombat, 999, ammo_index, true);
		}

		pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
		if (pWeapon)
		{
			int	ammo_index;
			ammo_index = CBaseCombatWeapon_GetPrimaryAmmoType(pWeapon);
			CBaseCombatCharacter_GiveAmmo(pCombat, 999, ammo_index, true);
			ammo_index = CBaseCombatWeapon_GetSecondaryAmmoType(pWeapon);
			CBaseCombatCharacter_GiveAmmo(pCombat, 999, ammo_index, true);
		}
	}
}


void	ManiWarmupTimer::SetRandomItem(ConVar *cvar_ptr, int item_number)
{
	item_t	*item_list = NULL;
	int	item_list_size = 0;

	const char *item_string = cvar_ptr->GetString();

	if (FStrEq(item_string,""))
	{
		item_name[item_number][0] = '\0';
		return;
	}

	int i = 0;
	int j = 0;
	char	tmp_item_name[80] = "";

	for (;;)
	{
		if (item_string[i] == ':' || item_string[i] == '\0')
		{
			tmp_item_name[j] = '\0';
			if (i != 0)
			{
				AddToList((void **) &item_list, sizeof(ManiWarmupTimer::item_t), &item_list_size);
				Q_strcpy(item_list[item_list_size - 1].item_name, tmp_item_name);

				j = 0;
				if (item_string[i] == '\0')
				{
					break;
				}
				else
				{
					i++;
					j = 0;
					continue;
				}
			}
			else
			{
				break;
			}
		}

		tmp_item_name[j] = item_string[i];
		j++;
		i++;
	}

	if (item_list_size == 0)
	{
		item_name[item_number][0] = '\0';
	}
	else if (item_list_size == 1)
	{
		strcpy(item_name[item_number], item_list[0].item_name);
	}
	else
	{
		int choice = rand() % item_list_size;
		strcpy(item_name[item_number], item_list[choice].item_name);
	}

	FreeList((void **) &item_list, &item_list_size);
	return;
}

ManiWarmupTimer	g_ManiWarmupTimer;
ManiWarmupTimer	*gpManiWarmupTimer;

CONVAR_CALLBACK_FN(ManiWarmupTimerCVar)
{
	gpManiWarmupTimer->LevelInit();
}

CONVAR_CALLBACK_FN(ManiWarmupItem1)
{
	gpManiWarmupTimer->SetRandomItem(&mani_warmup_timer_spawn_item_1, 0);
}

CONVAR_CALLBACK_FN(ManiWarmupItem2)
{
	gpManiWarmupTimer->SetRandomItem(&mani_warmup_timer_spawn_item_2, 1);
}

CONVAR_CALLBACK_FN(ManiWarmupItem3)
{
	gpManiWarmupTimer->SetRandomItem(&mani_warmup_timer_spawn_item_3, 2);
}

CONVAR_CALLBACK_FN(ManiWarmupItem4)
{
	gpManiWarmupTimer->SetRandomItem(&mani_warmup_timer_spawn_item_4, 3);
}

CONVAR_CALLBACK_FN(ManiWarmupItem5)
{
	gpManiWarmupTimer->SetRandomItem(&mani_warmup_timer_spawn_item_5, 4);
}
