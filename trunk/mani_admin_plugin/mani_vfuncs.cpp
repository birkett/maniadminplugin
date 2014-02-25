//
// Mani Admin Plugin
//
// Copyright (c) 2010-2013 Giles Millward (Mani). All rights reserved.
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
//#ifndef __linux__
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#endif
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_gametype.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "cbaseentity.h"
#include "mani_vfuncs.h"

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	CGlobalVars *gpGlobals;
extern	IVoiceServer *voiceserver;
extern	ITempEntsSystem *temp_ents;
extern	int	max_players;

#ifdef __linux__
#include "mani_linuxutils.h"

static	void	CheckVFunc(SymbolMap *sym_map_ptr, DWORD *class_ptr, char *class_name, char *class_function, char *gametype_ptr, int vfunc_type);
static	int	FindVFunc(SymbolMap *sym_map_ptr, DWORD *class_ptr, char *class_name, char *class_function, char *mangled_ptr);
#endif

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

CBaseEntity *EdictToCBE(edict_t *pEdict)
{
	return (CBaseEntity *) pEdict->GetUnknown()->GetBaseEntity();
}

class ManiEmptyClass {};

#ifdef __linux__
#define VFUNC_OS_DEP void *addr;	} u; 	u.addr = func;
#else
#define VFUNC_OS_DEP struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

#define VFUNC_SETUP_PTR(_vfunc_index)  \
{ \
	void **this_ptr = *(void ***)&pThisPtr; \
	void **vtable = *(void ***)pThisPtr; \
	void *func = vtable[gpManiGameType->GetVFuncIndex(_vfunc_index)]

//********************************************************************
// CBaseEntity

#define VFUNC_CALL0(_vfunc_index, _return_type, _class_type, _func_name ) \
	_return_type _func_name(_class_type *pThisPtr) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();} 

#define VFUNC_CALL1(_vfunc_index, _return_type, _class_type, _func_name, _param1) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1);} 

#define VFUNC_CALL2(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2);} 

#define VFUNC_CALL3(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3);} 

#define VFUNC_CALL4(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4);} 

#define VFUNC_CALL5(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5);} 

#define VFUNC_CALL6(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6);} 

#define VFUNC_CALL7(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7);} 

#define VFUNC_CALL8(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7, _param8 p8) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7, p8);} 

#define VFUNC_CALL9(_vfunc_index, _return_type, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8, _param9) \
	_return_type _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7, _param8 p8, _param9) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { _return_type (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8, _param9); \
	VFUNC_OS_DEP \
 	return (_return_type) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7, p8, p9);} 

#define VFUNC_CALL0_void(_vfunc_index, _class_type, _func_name ) \
	void _func_name(_class_type *pThisPtr) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();} 

#define VFUNC_CALL1_void(_vfunc_index, _class_type, _func_name, _param1) \
	void _func_name(_class_type *pThisPtr, _param1 p1) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1);} 

#define VFUNC_CALL2_void(_vfunc_index, _class_type, _func_name, _param1, _param2) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2);} 

#define VFUNC_CALL3_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3);} 

#define VFUNC_CALL4_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4);} 

#define VFUNC_CALL5_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5);} 

#define VFUNC_CALL6_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6);} 

#define VFUNC_CALL7_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7);} 

#define VFUNC_CALL8_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7, _param8 p8) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7, p8);} 

#define VFUNC_CALL9_void(_vfunc_index, _class_type, _func_name, _param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8, _param9) \
	void _func_name(_class_type *pThisPtr, _param1 p1, _param2 p2, _param3 p3, _param4 p4, _param5 p5, _param6 p6, _param7 p7, _param8 p8, _param9) \
	VFUNC_SETUP_PTR(_vfunc_index); \
	union { void (ManiEmptyClass::*mfpnew)(_param1, _param2, _param3, _param4, _param5, _param6, _param7, _param8, _param9); \
	VFUNC_OS_DEP \
 	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(p1, p2, p3, p4, p5, p6, p7, p8, p9);} 


// virtual const QAngle &EyeAngles( void );
VFUNC_CALL0(MANI_VFUNC_EYE_ANGLES, const QAngle &, CBaseEntity, CBaseEntity_EyeAngles)

// virtual void	Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
#if defined ( GAME_CSGO )
VFUNC_CALL4_void(MANI_VFUNC_TELEPORT, CBaseEntity, CBaseEntity_Teleport, const Vector *, const QAngle *,  const Vector *, bool)
#else
VFUNC_CALL3_void(MANI_VFUNC_TELEPORT, CBaseEntity, CBaseEntity_Teleport, const Vector *, const QAngle *, const Vector *)
#endif

// virtual Vector	EyePosition( void );
VFUNC_CALL0(MANI_VFUNC_EYE_POSITION, Vector, CBaseEntity, CBaseEntity_EyePosition)

//virtual void	GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity = NULL);
VFUNC_CALL2_void(MANI_VFUNC_GET_VELOCITY, CBaseEntity, CBaseEntity_GetVelocity, Vector *, AngularImpulse *)

// virtual CBaseCombatCharacter *MyCombatCharacterPointer( void );
VFUNC_CALL0(MANI_VFUNC_MY_COMBAT_CHARACTER, CBaseCombatCharacter *, CBaseEntity, CBaseEntity_MyCombatCharacterPointer)

// virtual void SetModelIndex( int index );
VFUNC_CALL1_void(MANI_VFUNC_SET_MODEL_INDEX, CBaseEntity, CBaseEntity_SetModelIndex, short)

// virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
VFUNC_CALL4_void(MANI_VFUNC_IGNITE, CBasePlayer, CBasePlayer_Ignite, float, bool, float, bool)

// virtual bool	RemovePlayerItem( CBaseCombatWeapon *pItem )
VFUNC_CALL1(MANI_VFUNC_REMOVE_PLAYER_ITEM, bool, CBasePlayer, CBasePlayer_RemovePlayerItem, CBaseCombatWeapon *)

// virtual void	Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
VFUNC_CALL3_void(MANI_VFUNC_WEAPON_DROP, CBasePlayer, CBasePlayer_WeaponDrop, CBaseCombatWeapon *, const Vector *, const Vector *)

// virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 );
#if defined ( GAME_CSGO )
VFUNC_CALL3(MANI_VFUNC_GIVE_ITEM, CBaseEntity *, CBasePlayer, CBasePlayer_GiveNamedItem, const char *, int, bool)
#else
VFUNC_CALL2(MANI_VFUNC_GIVE_ITEM, CBaseEntity *, CBasePlayer, CBasePlayer_GiveNamedItem, const char *, int)
#endif

// virtual CBaseCombatWeapon *Weapon_GetSlot( int slot ) const;
VFUNC_CALL1(MANI_VFUNC_GET_WEAPON_SLOT, CBaseCombatWeapon *, CBaseCombatCharacter, CBaseCombatCharacter_Weapon_GetSlot, int)

// virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
VFUNC_CALL2_void(MANI_VFUNC_WEAPON_SWITCH, CBaseCombatCharacter, CBaseCombatCharacter_Weapon_Switch, CBaseCombatWeapon *, int)

// virtual int GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
VFUNC_CALL3_void(MANI_VFUNC_GIVE_AMMO, CBaseCombatCharacter, CBaseCombatCharacter_GiveAmmo, int, int, bool)

// virtual int	GetPrimaryAmmoType( void );
VFUNC_CALL0(MANI_VFUNC_GET_PRIMARY_AMMO_TYPE, int, CBaseCombatWeapon, CBaseCombatWeapon_GetPrimaryAmmoType)

// virtual int	GetSecondaryAmmoType( void );
VFUNC_CALL0(MANI_VFUNC_GET_SECONDARY_AMMO_TYPE, int, CBaseCombatWeapon, CBaseCombatWeapon_GetSecondaryAmmoType)

// virtual char const *GetName( void );
VFUNC_CALL0(MANI_VFUNC_WEAPON_GET_NAME, const char *, CBaseCombatWeapon, CBaseCombatWeapon_GetName)

// virtual int CommitSuicide( void );
VFUNC_CALL0_void(MANI_VFUNC_COMMIT_SUICIDE, CBasePlayer, CBasePlayer_CommitSuicide)

// virtual bool			SetObserverTarget(CBaseEntity * target);
VFUNC_CALL1_void(MANI_VFUNC_SET_OBSERVER_TARGET, CBasePlayer, CBasePlayer_SetObserverTarget, CBaseEntity *)

// virtual const char*		GetClassName() const = 0;
VFUNC_CALL0(MANI_VFUNC_GET_CLASS_NAME, const char *, IServerNetworkable, IServerNetworkable_GetClassName)


datamap_t *CBaseEntity_GetDataDescMap(CBaseEntity *pThisPtr)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[gpManiGameType->GetVFuncIndex(MANI_VFUNC_MAP)]; 

	union {datamap_t *(ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (datamap_t *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
}



//********************************************************************
// Find offset functions

#ifdef __linux__
CON_COMMAND(ma_vfuncs, "Debug Tool")
{
#ifndef GAME_ORANGE
	const CCommand args;
#endif
        player_t player;


        player.entity = NULL;

        if (!IsCommandIssuedByServerAdmin()) return;
        if (ProcessPluginPaused()) return;

        if (args.ArgC() < 4)
        {
                MMsg("Need more args :)\n");
                return;
        }

	SymbolMap *sym_map_ptr = new SymbolMap();
	if (!sym_map_ptr->GetLib(gpManiGameType->GetLinuxBin()))
	{
		MMsg("Failed to get library [%s]\n", gpManiGameType->GetLinuxBin());
		delete sym_map_ptr;
		return;
	}

	bool	found_player = false;

	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		found_player = true;
	}

	if (!found_player)
	{
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			found_player = true;
		}
	}

	// Whoever issued the commmand is authorised to do it.
	if (!found_player)
	{
		MMsg("Need a target player to work the magic\n");
		delete sym_map_ptr;
		return;
	}


        CBaseEntity *pPlayer = player.entity->GetUnknown()->GetBaseEntity();

        FileHandle_t file_handle;
        char    base_filename[512];
        DWORD   *type_ptr;

        //Write to disk
        if (FStrEq("CBE", args.Arg(2)))
        {
       		snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbe.out", mani_path.GetString());
	        type_ptr = (DWORD *) pPlayer;
        }
        else if (FStrEq("VOICE", args.Arg(2)))
        {
	        snprintf(base_filename, sizeof (base_filename), "./cfg/%s/voice.out", mani_path.GetString());
	        type_ptr = (DWORD *) voiceserver;
        }
	else if (FStrEq("TE", args.Arg(2)))
        {
	        snprintf(base_filename, sizeof (base_filename), "./cfg/%s/te.out", mani_path.GetString());
	        type_ptr = (DWORD *) temp_ents;
        }
	else if (FStrEq("CBCC", args.Arg(2)))
        {
	        snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbcc.out", mani_path.GetString());
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
	        if (pCombat)
	        {
		        type_ptr = (DWORD *)pCombat;
	        }
	        else
	        {
		        MMsg("Failed to get Combat Character\n");
			delete sym_map_ptr;
		        return;
	        }
        }
        else if (FStrEq("CBCW", args.Arg(2)))
        {
	        snprintf(base_filename, sizeof (base_filename), "./cfg/%s/cbcw.out", mani_path.GetString());
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
	        if (!pCombat)
	        {
		        MMsg("Failed to get combat character\n");
			delete sym_map_ptr;
		        return;
	        }

        	// Get pistol type
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
	        if (!pWeapon)
	        {
		        MMsg("Failed to get weapon info\n");
			delete sym_map_ptr;
		        return;
	        }

	        type_ptr = (DWORD *) pWeapon;
        }
        else
        {
	        MMsg("Invalid 3rd arg\n");
		delete sym_map_ptr;
	        return;
        }

        file_handle = filesystem->Open (base_filename,"wt", NULL);
        if (file_handle == NULL)
        {
	        MMsg("Failed to open file [%s] for writing\n", base_filename);
		delete sym_map_ptr;
	        return;
        }

        DWORD   *class_address = type_ptr;
	DWORD	*vtable_address = (DWORD *) *class_address;

	void **vtable = (void **) vtable_address;

        for (int i = 2; i < atoi(args.Arg(3)); i++)
        {
        	void *FuncPtr = (void *) vtable[i];
                symbol_t *symbol_ptr = sym_map_ptr->GetAddr((void *)FuncPtr);
                if (symbol_ptr)
                {
			if (strncmp(symbol_ptr->mangled_name, "_ZTI", 4) == 0)
			{
				break;
			}

		        char temp_string[2048];
		        int temp_length = snprintf(temp_string, sizeof(temp_string), "%03i  %s\n", i, symbol_ptr->mangled_name);

		        if (filesystem->Write((void *) temp_string, temp_length, file_handle) == 0)
		        {
			        MMsg("Failed to write data !!\n");
			        filesystem->Close(file_handle);
				delete sym_map_ptr;
			        return;
		        }

		        MMsg("%03i  %s\n", i, symbol_ptr->mangled_name);
                }
        }

        filesystem->Close(file_handle);
	delete sym_map_ptr;
}

CON_COMMAND(ma_getvfunc, "Debug Tool")
{
#ifndef GAME_ORANGE
	const CCommand args;
#endif

	player_t player;

	player.entity = NULL;

	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	if (args.ArgC() < 3)
	{
		MMsg("Need more args :)\n");
		return;
	}

	SymbolMap *sym_map_ptr = new SymbolMap();
	if (!sym_map_ptr->GetLib(gpManiGameType->GetLinuxBin()))
	{
		MMsg("Failed to get library [%s]\n", gpManiGameType->GetLinuxBin());
		delete sym_map_ptr;
		return;
	}

	bool	found_player = false;

	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		found_player = true;
	}

	if (!found_player)
	{
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			found_player = true;
		}
	}

	// Whoever issued the commmand is authorised to do it.
	if (!found_player)
	{
		MMsg("Need a target player to work the magic\n");
		delete sym_map_ptr;
		return;
	}

	player_t *target_ptr = &player;

	CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();
	DWORD *type_ptr;

	//Write to disk
	if (FStrEq("CBE", args.Arg(1)))
	{
		type_ptr = (DWORD *) pPlayer;
	}
	else if (FStrEq("VOICE", args.Arg(1)))
	{
		type_ptr = (DWORD *) voiceserver;
	}
	else if (FStrEq("TE", args.Arg(1)))
	{
		type_ptr = (DWORD *) temp_ents;
	}
	else if (FStrEq("CBCC", args.Arg(1)))
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		//		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
		if (pCombat)
		{
			type_ptr = (DWORD *)pCombat;
		}
		else
		{
			MMsg("Failed to get Combat Character\n");
			return;
		}
	}
	else if (FStrEq("CBCW", args.Arg(1)))
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		//		CBaseCombatCharacter *pCombat = pPlayer->MyCombatCharacterPointer();
		if (!pCombat)
		{
			MMsg("Failed to get combat character\n");
			delete sym_map_ptr;
			return;
		}

		// Get pistol type
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
		if (!pWeapon)
		{
			MMsg("Failed to get weapon info\n");
			delete sym_map_ptr;
			return;
		}

		type_ptr = (DWORD *) pWeapon;
	}
	else
	{
		MMsg("Invalid 2nd arg\n");
		delete sym_map_ptr;
		return;
	}

	char mangled_name[256];

	int index;

	if (args.ArgC() < 4)
	{
		index = FindVFunc(sym_map_ptr, type_ptr, args.Arg(2), NULL, mangled_name);
	}
	else
	{
		index = FindVFunc(sym_map_ptr, type_ptr, args.Arg(2), args.Arg(3), mangled_name);
	}

	if (index == -1)
	{
		MMsg("Did not find index :(\n");
		delete sym_map_ptr;
		return;
	}

	MMsg("Found Index [%i] [0x%x] [%s]\n", index, index, mangled_name);
	delete sym_map_ptr;
}

CON_COMMAND(ma_autovfunc, "Debug Tool <player> <level>")
{
#ifndef GAME_ORANGE
	const CCommand args;
#endif
	player_t player;

	player.entity = NULL;

	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	if (args.ArgC() < 2)
	{
		MMsg("Need more args :)\n");
		return;
	}

	SymbolMap *sym_map_ptr = new SymbolMap();
	if (!sym_map_ptr->GetLib(gpManiGameType->GetLinuxBin()))
	{
		MMsg("Failed to get library [%s]\n", gpManiGameType->GetLinuxBin());
		delete sym_map_ptr;
		return;
	}

	bool	found_player = false;

	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		found_player = true;
	}

	if (!found_player)
	{
		for (int i = 1; i <= max_players; i++)
		{
			player.index = i;
			if (!FindPlayerByIndex(&player)) continue;
			found_player = true;
		}
	}

	// Whoever issued the commmand is authorised to do it.
	if (!found_player)
	{
		MMsg("Need a target player to work the magic\n");
		delete sym_map_ptr;
		return;
	}

	int level = atoi(args.Arg(1));

	player_t *target_ptr = &player;

	CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();
	CBasePlayer *pBase = (CBasePlayer *) pPlayer;

	DWORD *type_ptr;

	int index = -1;

	MMsg("Errors starting with 'Missing ...' are usually not a problem\n");
	MMsg("Any strings highlighted need correcting in gametypes.txt for this mod type\n");
	MMsg("This only shows vfunc indexes that are currently wrong in the gametypes.txt file\n");

	if (level > 0)
	{
		type_ptr = (DWORD *) pPlayer;

		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "EyeAngles", "eye_angles", MANI_VFUNC_EYE_ANGLES);
		CheckVFunc(sym_map_ptr, type_ptr, "CBaseEntity", "SetModelIndex", "set_model_index", MANI_VFUNC_SET_MODEL_INDEX);
		CheckVFunc(sym_map_ptr, type_ptr, "CBaseAnimating", "Teleport", "teleport", MANI_VFUNC_TELEPORT);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "EyePosition", "eye_position", MANI_VFUNC_EYE_POSITION);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "GiveNamedItem", "give_item", MANI_VFUNC_GIVE_ITEM);
		// Dods and CS inherit from CBasePlayer
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO))) CheckVFunc(sym_map_ptr, type_ptr, "CCSPlayer", "GiveNamedItem", "give_item", MANI_VFUNC_GIVE_ITEM);
		if (gpManiGameType->IsGameType(MANI_GAME_DOD)) CheckVFunc(sym_map_ptr, type_ptr, "CDODPlayer", "GiveNamedItem", "give_item", MANI_VFUNC_GIVE_ITEM);

		CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatCharacter", "MyCombatCharacterPointer", "my_combat_character", MANI_VFUNC_MY_COMBAT_CHARACTER);
		CheckVFunc(sym_map_ptr, type_ptr, "CBaseAnimating", "GetVelocity", "get_velocity", MANI_VFUNC_GET_VELOCITY);

		CheckVFunc(sym_map_ptr, type_ptr, "CBaseEntity", "GetDataDescMap", "map_desc", MANI_VFUNC_MAP);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "GetDataDescMap", "map_desc", MANI_VFUNC_MAP);
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO))) CheckVFunc(sym_map_ptr, type_ptr, "CCSPlayer", "GetDataDescMap", "map_desc", MANI_VFUNC_MAP);
		if (gpManiGameType->IsGameType(MANI_GAME_DOD)) CheckVFunc(sym_map_ptr, type_ptr, "CDODPlayer", "GetDataDescMap", "map_desc", MANI_VFUNC_MAP);
		if (gpManiGameType->IsGameType(MANI_GAME_TF)) CheckVFunc(sym_map_ptr, type_ptr, "CTFPlayer", "GetDataDescMap", "map_desc", MANI_VFUNC_MAP);

		type_ptr = (DWORD *) pBase;

		CheckVFunc(sym_map_ptr, type_ptr, "CBaseAnimating", "Ignite", "ignite", MANI_VFUNC_IGNITE);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "Weapon_Drop", "weapon_drop", MANI_VFUNC_WEAPON_DROP);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "ProcessUsercmds", "user_cmds", MANI_VFUNC_USER_CMDS);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "CommitSuicide", "commit_suicide", MANI_VFUNC_COMMIT_SUICIDE);
		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "SetObserverTarget", "set_observer_target", MANI_VFUNC_SET_OBSERVER_TARGET);

		if (gpManiGameType->IsGameType(MANI_GAME_TF)) 
		{
			CheckVFunc(sym_map_ptr, type_ptr, "CTFPlayer", "SetObserverTarget", "set_observer_target", MANI_VFUNC_SET_OBSERVER_TARGET);
		}

		CheckVFunc(sym_map_ptr, type_ptr, "CCSPlayer", "Weapon_CanUse", "canuse_weapon", MANI_VFUNC_WEAPON_CANUSE);
		if (gpManiGameType->IsGameType(MANI_GAME_DOD)) CheckVFunc(sym_map_ptr, type_ptr, "CDODPlayer", "CommitSuicide", "commit_suicide", MANI_VFUNC_COMMIT_SUICIDE);

		if (gpManiGameType->IsGameType(MANI_GAME_TF)) CheckVFunc(sym_map_ptr, type_ptr, "CBaseFlex", "Teleport", "teleport", MANI_VFUNC_TELEPORT);
	}

	if (level > 1)
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		type_ptr = (DWORD *) pCombat;

		CheckVFunc(sym_map_ptr, type_ptr, "CBasePlayer", "RemovePlayerItem", "remove_player_item", MANI_VFUNC_REMOVE_PLAYER_ITEM);
		CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatCharacter", "Weapon_GetSlot", "get_weapon_slot", MANI_VFUNC_GET_WEAPON_SLOT);
		if ((gpManiGameType->IsGameType(MANI_GAME_CSS)) || (gpManiGameType->IsGameType(MANI_GAME_CSGO))) CheckVFunc(sym_map_ptr, type_ptr, "CCSPlayer", "Weapon_Switch", "weapon_switch", MANI_VFUNC_WEAPON_SWITCH);
		CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatCharacter", "GiveAmmo", "give_ammo", MANI_VFUNC_GIVE_AMMO);
	}

	if (level > 2)
	{
		CBaseCombatCharacter *pCombat = CBaseEntity_MyCombatCharacterPointer(pPlayer);
		CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot(pCombat, 1);
		if (pWeapon)
		{

			type_ptr = (DWORD *) pWeapon;
			CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatWeapon", "GetPrimaryAmmoType", "get_primary_ammo_type", MANI_VFUNC_GET_PRIMARY_AMMO_TYPE);
			CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatWeapon", "GetSecondaryAmmoType", "get_secondary_ammo_type", MANI_VFUNC_GET_SECONDARY_AMMO_TYPE);
			CheckVFunc(sym_map_ptr, type_ptr, "CBaseCombatWeapon", "GetName", "weapon_get_name", MANI_VFUNC_WEAPON_GET_NAME);
		}
	}
	
	delete sym_map_ptr;
}



static	void CheckVFunc(SymbolMap *sym_map_ptr, DWORD *class_ptr, char *class_name, char *class_function, char *gametype_ptr, int vfunc_type)
{
	int index;
	char mangled_ptr[256]="";

	index = FindVFunc(sym_map_ptr, class_ptr, class_name, class_function, mangled_ptr);

	if (index != -1)
	{
		if (gpManiGameType->vfunc_index[vfunc_type] != index)
		{
			MMsg("\t\t\t\"%s\"\t\"%i\"\n", gametype_ptr, index);
		}
	}
	else
	{
		MMsg("Missing %s::%s (Probably not a problem)\n", class_name, class_function);
	}
}

static	int FindVFunc(SymbolMap *sym_map_ptr, DWORD *class_ptr, char *class_name, char *class_function, char *mangled_ptr)
{
        DWORD   *class_address = class_ptr;
	DWORD	*vtable_address = (DWORD *) *class_address;

	void **vtable = (void **) vtable_address;

        for (int i = 2; i < 1000; i++)
        {
        	void *FuncPtr = (void *) vtable[i];
                symbol_t *symbol_ptr = sym_map_ptr->GetAddr((void *)FuncPtr);
                if (symbol_ptr)
                {
			if (class_function)
			{
				if (Q_stristr(symbol_ptr->mangled_name, class_name) != NULL &&
					Q_stristr(symbol_ptr->mangled_name, class_function) != NULL)
				{
					// Found Match
					Q_strcpy(mangled_ptr, symbol_ptr->mangled_name);
					return i;
				}
			}
			else
			{
				// Just match one parameter
 				if (Q_stristr(symbol_ptr->mangled_name, class_name) != NULL)
				{
					// Found Match
					Q_strcpy(mangled_ptr, symbol_ptr->mangled_name);
					return i;
				}
			}

			if (strncmp(symbol_ptr->mangled_name, "_ZTI", 4) == 0)
			{
				break;
			}
                }
        }

	return -1;
}

CON_COMMAND(ma_vfunc_dumpall, "Dump all vfuncs to file vfuncs_dumpall.out")
{
#ifndef GAME_ORANGE
	const CCommand args;
#endif
        FileHandle_t file_handle;
        char    base_filename[512];
        char    temp_string[2048];

        if (!IsCommandIssuedByServerAdmin()) return;
        if (ProcessPluginPaused()) return;

	SymbolMap *symbol_map = new SymbolMap();

	if (!symbol_map->GetLib(gpManiGameType->GetLinuxBin()))
	{
		MMsg("Failed to get library [%s]\n", gpManiGameType->GetLinuxBin());
		delete symbol_map;
		return;
	}

        //Write to disk
        snprintf(base_filename, sizeof (base_filename), 
			"./cfg/%s/vfuncs_dumpall.out", mani_path.GetString());

        file_handle = filesystem->Open (base_filename,"wt", NULL);
        if (file_handle == NULL)
        {
                MMsg("Failed to open file [%s] for writing\n", base_filename);
		delete symbol_map;
		return;
        }

        for ( int i = 0; i < symbol_map->GetMapSize(); i++)
        {
		symbol_t *symbol = symbol_map->GetAddr(i);

                if (strncmp(symbol->demangled_name, "vtable", 6) == 0)
                {
        		int temp_length = snprintf(temp_string, 
					sizeof(temp_string), 
					"\n*** Class [%s] [%s] ****\n", 
					symbol->mangled_name, 
					symbol->demangled_name);

//			MMsg("%s", temp_string);
	
        		if (filesystem->Write((void *) temp_string, 
						temp_length, file_handle) == 0)
        		{
        			MMsg("Failed to write data !!\n");
                		filesystem->Close(file_handle);
				delete symbol_map;
                		return;
        		}

			void **vtable = (void **) symbol->ptr;

		        int vtable_index = 2;
        		while (vtable_index < 1000)
        		{
		                void *FuncPtr = vtable[vtable_index];
		                if (FuncPtr == NULL)
		                {
		                        break;
		                }

				symbol_t *symbol_ptr = symbol_map->GetAddr(FuncPtr);
		                if (symbol_ptr != NULL)
		                {
					if (strncmp(symbol_ptr->mangled_name, "_ZTI", 4) == 0)
					{
						break;
					}

        				temp_length = snprintf(temp_string, 
						sizeof(temp_string), "  %03i [%s] [%s]\n", 
						vtable_index - 2, 
						symbol_ptr->mangled_name, 
						symbol_ptr->demangled_name);

//			MMsg("%s", temp_string);
		        		if (filesystem->Write((void *) temp_string, 
							temp_length, file_handle) == 0)
		        		{
		        			MMsg("Failed to write data !!\n");
		                		filesystem->Close(file_handle);
						delete symbol_map;
		                		return;
		        		}
		                }

		                vtable_index+=1;
		        }
                }

        }


        filesystem->Close(file_handle);
	delete symbol_map;
	Msg("Written details to [%s]\n", base_filename);
}

#endif




