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
//#ifndef __linux__
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#endif
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_gametype.h"
#include "mani_player.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_convar.h"
#include "cbaseentity.h"
#include "mani_vfuncs.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	CGlobalVars *gpGlobals;
extern	IVoiceServer *voiceserver;
extern	ITempEntsSystem *temp_ents;

#ifdef __linux__
static	void	CheckVFunc(DWORD *class_ptr, char *class_name, char *class_function, char *gametype_ptr);
static	int		FindVFunc(DWORD *class_ptr, char *class_name, char *class_function, char *mangled_ptr);
#endif

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

class ManiEmptyClass {};

CBaseEntity *EdictToCBE(edict_t *pEdict)
{
	return (CBaseEntity *) pEdict->GetUnknown()->GetBaseEntity();
}

//********************************************************************
// CBaseEntity

QAngle &CBaseEntity_EyeAngles(CBaseEntity *pThisPtr)
{
// virtual const QAngle &EyeAngles( void );
//	DWORD *FuncPtr = (DWORD *)(*(DWORD *)((*((DWORD *) pThisPtr)) + 0x1b0));

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_EYE_ANGLES)]; 

	union {QAngle &(ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (QAngle &) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
}

void CBaseEntity_Teleport(CBaseEntity *pThisPtr, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)
{
//	DWORD *FuncPtr = (DWORD *)(*(DWORD *)((*((DWORD *) pCBE)) + 0x170));
//  pCBE->Teleport(newPosition, newAngles, newVelocity);

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_TELEPORT)]; 

	union {void (ManiEmptyClass::*mfpnew)(const Vector *, const QAngle *, const Vector *);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(newPosition, newAngles, newVelocity);
}

Vector CBaseEntity_EyePosition (CBaseEntity *pThisPtr)
{
//	pVector = pThisPtr->EyePosition();

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_EYE_POSITION)]; 

	union {Vector (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (Vector) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();

}


void CBaseEntity_GetVelocity (CBaseEntity *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity)
{
//	virtual void	GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity = NULL);

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_GET_VELOCITY)]; 

	union {void (ManiEmptyClass::*mfpnew)(Vector *, AngularImpulse *);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(vVelocity, vAngVelocity);
}



CBaseCombatCharacter *CBaseEntity_MyCombatCharacterPointer(CBaseEntity *pThisPtr)
{
//	pCombat = pThisPtr->MyCombatCharacterPointer();
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_MY_COMBAT_CHARACTER)]; 

	union {CBaseCombatCharacter * (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseCombatCharacter *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();}

void CBaseEntity_SetGravity(CBaseEntity *pThisPtr, float newGravity)
{
	pThisPtr->SetGravity(newGravity);
}

void CBaseEntity_SetModelIndex(CBaseEntity *pThisPtr, int iIndex)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_SET_MODEL_INDEX)]; 

	union {void (ManiEmptyClass::*mfpnew)(int);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(iIndex);
}

//********************************************************************
// CBasePlayer 

void CBasePlayer_Ignite(CBasePlayer *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_IGNITE)]; 

	union {void (ManiEmptyClass::*mfpnew)(float , bool , float , bool );
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);
}

bool CBasePlayer_RemovePlayerItem(CBasePlayer *pThisPtr, CBaseCombatWeapon *pWeapon)
{
//	pThisPtr->RemovePlayerItem(pWeapon);
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_REMOVE_PLAYER_ITEM)]; 

	union {bool (ManiEmptyClass::*mfpnew)(CBaseCombatWeapon *);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (bool) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(pWeapon);

}

void CBasePlayer_WeaponDrop(CBasePlayer *pThisPtr, CBaseCombatWeapon *pWeapon)
{
//	pThisPtr->Weapon_Drop(pWeapon);
//	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_DROP)]; 

	union {void (ManiEmptyClass::*mfpnew)(CBaseCombatWeapon *, const Vector *, const Vector *);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(pWeapon, NULL, NULL);
}

//********************************************************************
// CBaseCombatCharacter 

CBaseCombatWeapon *CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *pThisPtr, int slot)
{
//	pWeapon = pThisPtr->Weapon_GetSlot(slot);

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_GET_WEAPON_SLOT)]; 

	union {CBaseCombatWeapon * (ManiEmptyClass::*mfpnew)(int);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseCombatWeapon *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(slot);
}

void CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *pThisPtr, int amount, int ammo_index, bool suppress_noise)
{
//	pThisPtr->GiveAmmo(amount, ammo_index, suppress_noise);

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_GIVE_AMMO)]; 

	union {void (ManiEmptyClass::*mfpnew)(int, int, bool);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(amount, ammo_index, suppress_noise);
}

//********************************************************************
// CBaseCombatWeapon 

int CBaseCombatWeapon_GetPrimaryAmmoType(CBaseCombatWeapon *pThisPtr)
{
//	virtual int				GetPrimaryAmmoType( void )  const { return m_iPrimaryAmmoType; }

	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_GET_PRIMARY_AMMO_TYPE)]; 

	union {int (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
}

int CBaseCombatWeapon_GetSecondaryAmmoType(CBaseCombatWeapon *pThisPtr)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_GET_SECONDARY_AMMO_TYPE)]; 

	union {int (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
}

const char *CBaseCombatWeapon_GetName(CBaseCombatWeapon *pThisPtr)
{
//	pWeaponName = pThisPtr->GetName();
//  virtual char const		*GetName( void ) const;
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_WEAPON_GET_NAME)]; 

	union {char const		*(ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (char const *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
}

//********************************************************************
//********************************************************************
//********************************************************************
//********************************************************************
// Handle prop offset calls

int Prop_GetHealth(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_HEALTH)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetHealth(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_HEALTH)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************

bool Prop_SetRenderMode(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_RENDER_MODE)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************

bool Prop_SetRenderFX(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_RENDER_FX)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************

bool Prop_SetRenderColor(edict_t *pEntity, byte r, byte g, byte b, byte a)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_COLOUR)) != -1)
	{
		color32 *cptr = (color32 *)(pEntity->GetUnknown() + offset);
		cptr->r = r;
		cptr->g = g;
		cptr->b = b;
		cptr->a = a;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

bool Prop_SetRenderColor(edict_t *pEntity, byte r, byte g, byte b)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_COLOUR)) != -1)
	{
		color32 *cptr = (color32 *)(pEntity->GetUnknown() + offset);
		cptr->r = r;
		cptr->g = g;
		cptr->b = b;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************

int Prop_GetMoveType(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_MOVE_TYPE)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetMoveType(edict_t *pEntity, int	NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_MOVE_TYPE)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************

int Prop_GetAccount(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_ACCOUNT)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetAccount(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_ACCOUNT)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************
/*
int Prop_GetDeaths(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_DEATHS)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetDeaths(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_DEATHS)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}
*/
//***************

int Prop_GetArmor(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_ARMOR)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetArmor(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_ARMOR)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

//***************
/*
int Prop_GetScore(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_SCORE)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetScore(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_SCORE)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
                pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}
*/
//***************

int Prop_GetModelIndex(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_MODEL_INDEX)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		return *iptr;
	}

	return 0;	
}

bool Prop_SetModelIndex(edict_t *pEntity, int NewValue)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_MODEL_INDEX)) != -1)
	{
		int *iptr = (int *)(pEntity->GetUnknown() + offset);
		*iptr = NewValue;
        pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
		return true;
	}

	return false;	
}

Vector *Prop_GetVecOrigin(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_VEC_ORIGIN)) != -1)
	{
		Vector *vptr = (Vector *)(pEntity->GetUnknown() + offset);
		return vptr;
	}

	return NULL;
}

QAngle *Prop_GetAngRotation(edict_t *pEntity)
{
	int offset;

	if ((offset = gpManiGameType->GetPropIndex(MANI_PROP_ANG_ROTATION)) != -1)
	{
		QAngle *vptr = (QAngle *)(pEntity->GetUnknown() + offset);
		return vptr;
	}

	return NULL;
}

//********************************************************************
// Find offset functions

void VFunc_CallCBaseEntity(player_t *player_ptr)
{

	CBaseEntity *pCBE;

	pCBE = EdictToCBE(player_ptr->entity);

	QAngle eye_angles = pCBE->EyeAngles();
	pCBE->Teleport(NULL, NULL, NULL);
	int health = pCBE->GetHealth();
	pCBE->SetHealth(100);
	Vector velocity;

	pCBE->GetVelocity(&velocity);
	pCBE->SetRenderColor(255, 255, 255, 255);
	pCBE->SetRenderColor(255, 255, 255);

	CBaseCombatCharacter *pCombat = pCBE->MyCombatCharacterPointer();
	pCBE->SetGravity(1.0);
	pCBE->SetModelIndex(-1);
	return;
}

#ifdef __linux__
CON_COMMAND(ma_vfuncs, "Debug Tool")
{

        player_t player;

        player.entity = NULL;

        if (!IsCommandIssuedByServerAdmin()) return;
        if (ProcessPluginPaused()) return;

        if (engine->Cmd_Argc() < 4)
        {
                Msg("Need more args :)\n");
                return;
        }

        // Whoever issued the commmand is authorised to do it.
        if (!FindTargetPlayers(&player, engine->Cmd_Argv(1), IMMUNITY_DONT_CARE))
        {
                return;
        }

        player_t *target_ptr = &(target_player_list[0]);


        CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();

        Dl_info d;

        void    *handle;
        void    *var_address;

        handle = dlopen(gpManiGameType->GetLinuxBin(), RTLD_NOW);

        if (handle == NULL)
        {
                Msg("Failed to open server image, error [%s]\n", dlerror());
                gpManiGameType->SetAdvancedEffectsAllowed(false);
        }
        else
        {
                FileHandle_t file_handle;
                char    base_filename[512];
                DWORD   *type_ptr;

                //Write to disk
                if (FStrEq("CBE", engine->Cmd_Argv(2)))
                {
                        Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbe.out", mani_path.GetString());
                        type_ptr = (DWORD *) pPlayer;
                }
                else if (FStrEq("VOICE", engine->Cmd_Argv(2)))
                {
                        Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/voice.out", mani_path.GetString());
                        type_ptr = (DWORD *) voiceserver;
                }
				else if (FStrEq("TE", engine->Cmd_Argv(2)))
                {
                        Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/te.out", mani_path.GetString());
                        type_ptr = (DWORD *) temp_ents;
                }
				else if (FStrEq("CBCC", engine->Cmd_Argv(2)))
                {
                        Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbcc.out", mani_path.GetString());
//                        CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
						CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
                        if (pCombat)
                        {
                                type_ptr = (DWORD *)pCombat;
                        }
                        else
                        {
                                Msg("Failed to get Combat Character\n");
								dlclose(handle);
                                return;
                        }
                }
                else if (FStrEq("CBCW", engine->Cmd_Argv(2)))
                {
                        Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbcw.out", mani_path.GetString());
						CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
//                        CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
                        if (!pCombat)
                        {
                                Msg("Failed to get combat character\n");
								dlclose(handle);
                                return;
                        }

                        // Get pistol type
//                        CBaseCombatWeapon *pWeapon = pCombat->Weapon_GetSlot(1);
						CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
                        if (!pWeapon)
                        {
                                Msg("Failed to get weapon info\n");
								dlclose(handle);
                                return;
                        }

                        type_ptr = (DWORD *) pWeapon;
                }
                else
                {
                        Msg("Invalid 3rd arg\n");
						dlclose(handle);
                        return;
                }

                file_handle = filesystem->Open (base_filename,"wt", NULL);
                if (file_handle == NULL)
                {
                        Msg ("Failed to open file [%s] for writing\n", base_filename);
						dlclose(handle);
                        return;
                }

                for (int i = 0; i < Q_atoi(engine->Cmd_Argv(3)); i++)
                {
                        DWORD *FuncPtr = (DWORD *)(*(DWORD *)((*((DWORD *) type_ptr)) + (i * 4)));

                        int status = dladdr(FuncPtr, &d);

                        if (status)
                        {
                                char temp_string[2048];
                                int temp_length = Q_snprintf(temp_string, sizeof(temp_string), "%s\n", d.dli_sname);

                                if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
                                {
                                        Msg ("Failed to write data !!\n");
                                        filesystem->Close(file_handle);
                                        return;
                                }

                                Msg("%s\n", d.dli_sname);
                        }
                        else
                        {
                                Msg("Failed offset [%i]\n", i);
                        }
                }

                filesystem->Close(file_handle);
                dlclose(handle);
        }
}

CON_COMMAND(ma_getvfunc, "Debug Tool")
{

	player_t player;

	player.entity = NULL;

	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	if (engine->Cmd_Argc() < 4)
	{
		Msg("Need more args :)\n");
		return;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, engine->Cmd_Argv(1), IMMUNITY_DONT_CARE))
	{
		return;
	}

	player_t *target_ptr = &(target_player_list[0]);

	CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();
	DWORD *type_ptr;

	//Write to disk
	if (FStrEq("CBE", engine->Cmd_Argv(2)))
	{
		type_ptr = (DWORD *) pPlayer;
	}
	else if (FStrEq("VOICE", engine->Cmd_Argv(2)))
	{
		type_ptr = (DWORD *) voiceserver;
	}
	else if (FStrEq("TE", engine->Cmd_Argv(2)))
	{
		type_ptr = (DWORD *) temp_ents;
	}
	else if (FStrEq("CBCC", engine->Cmd_Argv(2)))
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		//		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
		if (pCombat)
		{
			type_ptr = (DWORD *)pCombat;
		}
		else
		{
			Msg("Failed to get Combat Character\n");
			return;
		}
	}
	else if (FStrEq("CBCW", engine->Cmd_Argv(2)))
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		//		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
		if (!pCombat)
		{
			Msg("Failed to get combat character\n");
			return;
		}

		// Get pistol type
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
		if (!pWeapon)
		{
			Msg("Failed to get weapon info\n");
			return;
		}

		type_ptr = (DWORD *) pWeapon;
	}
	else
	{
		Msg("Invalid 3rd arg\n");
		return;
	}

	char mangled_name[256];

	int index;

	if (engine->Cmd_Argc() < 5)
	{
		index = FindVFunc(type_ptr, engine->Cmd_Argv(3), NULL, mangled_name);
	}
	else
	{
		index = FindVFunc(type_ptr, engine->Cmd_Argv(3), engine->Cmd_Argv(4), mangled_name);
	}

	if (index == -1)
	{
		Msg("Did not find index :(\n");
		return;
	}

	Msg("Found Index [%i] [0x%x] [%s]\n", index, index, mangled_name);
}

CON_COMMAND(ma_autovfunc, "Debug Tool <player> <level>")
{

	player_t player;

	player.entity = NULL;

	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	if (engine->Cmd_Argc() < 3)
	{
		Msg("Need more args :)\n");
		return;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, engine->Cmd_Argv(1), IMMUNITY_DONT_CARE))
	{
		Msg("Need a target player to work the magic\n");
		return;
	}

	int level = Q_atoi(engine->Cmd_Argv(2));

	player_t *target_ptr = &(target_player_list[0]);

	CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();
	CBasePlayer *pBase = (CBasePlayer *) pPlayer;

	DWORD *type_ptr;

	int index = -1;

	Msg("\t\t\"vfuncs\"\n\t\t{\n");

	if (level > 0)
	{
		type_ptr = (DWORD *) pPlayer;

		CheckVFunc(type_ptr, "CBasePlayer", "EyeAngles", "eye_angles");
		CheckVFunc(type_ptr, "CBaseEntity", "SetModelIndex", "set_model_index");
		CheckVFunc(type_ptr, "CBaseAnimating", "Teleport", "teleport");
		CheckVFunc(type_ptr, "CBasePlayer", "EyePosition", "eye_position");
		CheckVFunc(type_ptr, "CBaseCombatCharacter", "MyCombatCharacterPointer", "my_combat_character");
		CheckVFunc(type_ptr, "CBaseAnimating", "GetVelocity", "get_velocity");

		type_ptr = (DWORD *) pBase;

		CheckVFunc(type_ptr, "CBaseAnimating", "Ignite", "ignite");
		CheckVFunc(type_ptr, "CBasePlayer", "Weapon_Drop", "weapon_drop");
	}

	if (level > 1)
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		type_ptr = (DWORD *) pCombat;

		CheckVFunc(type_ptr, "CBasePlayer", "RemovePlayerItem", "remove_player_item");
		CheckVFunc(type_ptr, "CBaseCombatCharacter", "Weapon_GetSlot", "get_weapon_slot");
		CheckVFunc(type_ptr, "CBaseCombatCharacter", "GiveAmmo", "give_ammo");
	}

	if (level > 2)
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
		if (pWeapon)
		{

			type_ptr = (DWORD *) pWeapon;
			CheckVFunc(type_ptr, "CBaseCombatWeapon", "GetPrimaryAmmoType", "get_primary_ammo_type");
			CheckVFunc(type_ptr, "CBaseCombatWeapon", "GetSecondaryAmmoType", "get_secondary_ammo_type");
			CheckVFunc(type_ptr, "CBaseCombatWeapon", "GetName", "weapon_get_name");
		}
	}

	Msg("\t\t}\n");

}

static	void CheckVFunc(DWORD *class_ptr, char *class_name, char *class_function, char *gametype_ptr)
{
	int index;
	char mangled_ptr[256]="";

	index = FindVFunc(class_ptr, class_name, class_function, mangled_ptr);

	if (index != -1)
	{
		Msg("\t\t\t\"%s\"\t\"%i\"\n", gametype_ptr, index);
	}
	else
	{
		Msg("Failed to find %s::%s () !!\n", class_name, class_function);
	}
}

static	int		FindVFunc(DWORD *class_ptr, char *class_name, char *class_function, char *mangled_ptr)
{
        void    *handle;
        void    *var_address;
        Dl_info d;

        handle = dlopen(gpManiGameType->GetLinuxBin(), RTLD_NOW);

        if (handle == NULL)
        {
                Msg("Failed to open server image, error [%s]\n", dlerror());
                gpManiGameType->SetAdvancedEffectsAllowed(false);
        }
        else
        {
                for (int i = 0; i < 1000; i++)
                {
                        DWORD *FuncPtr = (DWORD *)(*(DWORD *)((*((DWORD *) class_ptr)) + (i * 4)));

                        int status = dladdr(FuncPtr, &d);

                        if (status)
                        {
							if (class_function)
							{
								if (Q_stristr(d.dli_sname, class_name) != NULL &&
									Q_stristr(d.dli_sname, class_function) != NULL)
								{
									// Found Match
									Q_strcpy(mangled_ptr, d.dli_sname);
									dlclose(handle);
									return i;
								}
							}
							else
							{
								// Just match one parameter
 								if (Q_stristr(d.dli_sname, class_name) != NULL)
								{
									// Found Match
									Q_strcpy(mangled_ptr, d.dli_sname);
									dlclose(handle);
									return i;
								}
							}
                       }
                }

                dlclose(handle);
        }

		return -1;
}


#endif




