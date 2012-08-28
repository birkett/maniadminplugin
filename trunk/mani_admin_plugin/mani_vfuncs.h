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


#ifndef MANI_VFUNCS_H
#define MANI_VFUNCS_H

#include "cbaseentity.h"
//#ifndef __linux__
//#include <windows.h>
//#define WIN32_LEAN_AND_MEAN
//#endif

//extern	int				GetNumberOfActivePlayers(void );

extern CBaseEntity *EdictToCBE(edict_t *pEdict);

extern const QAngle &CBaseEntity_EyeAngles(CBaseEntity *pThisPtr);
#if defined ( GAME_CSGO )
extern void CBaseEntity_Teleport(CBaseEntity *pThisPtr, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity, bool UnkownBoolean = true);
extern CBaseEntity *CBasePlayer_GiveNamedItem(CBasePlayer *pThisPtr, const char *szName, int iSubType = 0, bool UnkownBoolean = true);
#else
extern void CBaseEntity_Teleport(CBaseEntity *pThisPtr, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
extern CBaseEntity *CBasePlayer_GiveNamedItem(CBasePlayer *pThisPtr, const char *szName, int iSubType = 0);
#endif
extern Vector CBaseEntity_EyePosition (CBaseEntity *pThisPtr);
extern void CBaseEntity_GetVelocity(CBaseEntity *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity = NULL);
extern CBaseCombatCharacter *CBaseEntity_MyCombatCharacterPointer(CBaseEntity *pThisPtr);
extern void CBaseEntity_SetModelIndex(CBaseEntity *pThisPtr, short iIndex);
extern void CBasePlayer_Ignite(CBasePlayer *pThisPtr, float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
extern bool CBasePlayer_RemovePlayerItem(CBasePlayer *pThisPtr, CBaseCombatWeapon *pItem);
extern void CBasePlayer_WeaponDrop(CBasePlayer *pThisPtr, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL);
extern int  CBaseCombatWeapon_GetPrimaryAmmoType(CBaseCombatWeapon *pThisPtr);
extern int  CBaseCombatWeapon_GetSecondaryAmmoType(CBaseCombatWeapon *pThisPtr);
extern const char *CBaseCombatWeapon_GetName(CBaseCombatWeapon *pThisPtr);
extern CBaseCombatWeapon *CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *pThisPtr, int slot);
extern void CBaseCombatCharacter_Weapon_Switch(CBaseCombatCharacter *pThisPtr, CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
extern void CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *pThisPtr, int iCount, int iAmmoIndex, bool bSuppressSound = false );

extern datamap_t *CBaseEntity_GetDataDescMap(CBaseEntity *pThisPtr);
extern void CBasePlayer_CommitSuicide(CBasePlayer *pThisPtr);
extern void CBasePlayer_SetObserverTarget(CBasePlayer *pThisPtr, CBaseEntity *pTarget);
extern const char *IServerNetworkable_GetClassName(IServerNetworkable *pThisPtr);

// Debug functions
extern void	VFunc_CallCBaseEntity(player_t *player_ptr);

#ifdef __linux__
extern void DumpVFuncClass(DWORD *class_ptr, int count);
#endif

#endif
