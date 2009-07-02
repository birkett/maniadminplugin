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
extern void CBaseEntity_Teleport(CBaseEntity *pThisPtr, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
extern Vector CBaseEntity_EyePosition (CBaseEntity *pThisPtr);
extern void CBaseEntity_GetVelocity(CBaseEntity *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity = NULL);
extern CBaseCombatCharacter *CBaseEntity_MyCombatCharacterPointer(CBaseEntity *pThisPtr);
extern void CBaseEntity_SetModelIndex(CBaseEntity *pThisPtr, int iIndex);
extern void CBasePlayer_Ignite(CBasePlayer *pThisPtr, float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
extern bool CBasePlayer_RemovePlayerItem(CBasePlayer *pThisPtr, CBaseCombatWeapon *pItem);
extern void CBasePlayer_WeaponDrop(CBasePlayer *pThisPtr, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL);
extern CBaseEntity *CBasePlayer_GiveNamedItem(CBasePlayer *pThisPtr, const char *szName, int iSubType = 0 );
extern int  CBaseCombatWeapon_GetPrimaryAmmoType(CBaseCombatWeapon *pThisPtr);
extern int  CBaseCombatWeapon_GetSecondaryAmmoType(CBaseCombatWeapon *pThisPtr);
extern const char *CBaseCombatWeapon_GetName(CBaseCombatWeapon *pThisPtr);
extern CBaseCombatWeapon *CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *pThisPtr, int slot);
extern void CBaseCombatCharacter_Weapon_Switch(CBaseCombatCharacter *pThisPtr, CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
extern void CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *pThisPtr, int iCount, int iAmmoIndex, bool bSuppressSound = false );

extern datamap_t *CBaseEntity_GetDataDescMap(CBaseEntity *pThisPtr);

// Prop manager calls
extern	int	Prop_GetHealth(edict_t *pEntity);
extern	bool Prop_SetHealth(edict_t *pEntity, int NewValue);
extern	bool Prop_SetRenderMode(edict_t *pEntity, int NewValue);
extern	bool Prop_SetRenderFX(edict_t *pEntity, int NewValue);
extern	bool Prop_SetRenderColor(edict_t *pEntity, byte r, byte g, byte b, byte a);
extern	bool Prop_SetRenderColor(edict_t *pEntity, byte r, byte g, byte b);
extern	int Prop_GetMoveType(edict_t *pEntity);
extern	bool Prop_SetMoveType(edict_t *pEntity, int	NewValue);
extern	int Prop_GetAccount(edict_t *pEntity);
extern	bool Prop_SetAccount(edict_t *pEntity, int NewValue);
//extern	int Prop_GetDeaths(edict_t *pEntity);
//extern	bool Prop_SetDeaths(edict_t *pEntity, int NewValue);
extern	int Prop_GetArmor(edict_t *pEntity);
extern	bool Prop_SetArmor(edict_t *pEntity, int NewValue);
//extern	int Prop_GetScore(edict_t *pEntity);
//extern	bool Prop_SetScore(edict_t *pEntity, int NewValue);
extern	int Prop_GetModelIndex(edict_t *pEntity);
extern	bool Prop_SetModelIndex(edict_t *pEntity, int NewValue);
extern	Vector *Prop_GetVecOrigin(edict_t *pEntity);
extern	QAngle *Prop_GetAngRotation(edict_t *pEntity);

// Debug functions
extern void	VFunc_CallCBaseEntity(player_t *player_ptr);

#endif
