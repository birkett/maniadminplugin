// Sigscan code developed by Bailopan from www.sourcemm.net

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#if defined _WIN32 || defined WIN32
//   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #include <winnt.h>
   #include "mani_winutils.h"
#else
   #include <elf.h>
   #include <fcntl.h>
   #include <link.h>
   #include <sys/mman.h>
   #include <sys/stat.h>
   #include <demangle.h>
   #include "mani_linuxutils.h"
#endif
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
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_sigscan.h"
#include "mani_player.h"
#include "mani_vfuncs.h"
#include "cbaseentity.h"
#include "beam_flags.h"
#include "sourcehook.h"

extern	void *gamedll;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;
extern	bf_write *msg_buffer;
extern SourceHook::ISourceHook *g_SHPtr;

typedef void (*UTIL_Remove_)(CBaseEntity *);
typedef CCSWeaponInfo* (*CCSGetFileWeaponInfoHandle_)(unsigned short);

void *respawn_addr = NULL;
void *util_remove_addr = NULL;
void *ent_list_find_ent_by_classname = NULL;
void *console_echo_addr = NULL;
void *switch_team_addr = NULL;
void *set_model_from_class = NULL;
void *get_file_weapon_info_addr = NULL;
void *weapon_owns_this_type_addr = NULL;
void *connect_client_addr = NULL;
void *netsendpacket_addr = NULL;

CBaseEntityList *g_pEList = NULL;
CGameRules *g_pGRules = NULL;
UTIL_Remove_ UTILRemoveFunc;
CCSGetFileWeaponInfoHandle_ CCSGetFileWeaponInfoHandleFunc;

static void ShowSigInfo(void *ptr, char *sig_name);
#ifdef WIN32
static void *FindAddress(CWinSigUtil *sig_util_ptr, char *sig_name);
#else
static void *FindAddress(SymbolMap *sym_map_ptr, char *sig_name);
#endif

class ManiEmptyClass {};

bool CCSRoundRespawn(CBaseEntity *pThisPtr)
{
	if (!respawn_addr) return false;

	void **this_ptr = *(void ***)&pThisPtr;
	void *func = respawn_addr;

	union {void (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
	return true;
}

bool	CCSUTILRemove(CBaseEntity *pCBE)
{
	if (!util_remove_addr) return false;

	if (pCBE)
	{
		UTILRemoveFunc(pCBE);
		return true;
	}

	return false;
}

CCSWeaponInfo	*CCSGetFileWeaponInfoFromHandle(unsigned short handle_id)
{
	if (!get_file_weapon_info_addr) return false;
	return CCSGetFileWeaponInfoHandleFunc(handle_id);
}

CBaseEntity * CGlobalEntityList_FindEntityByClassname(CBaseEntity *pCBE, const char *ent_class)
{
//	pWeapon = pThisPtr->Weapon_GetSlot(slot);

	if (!g_pEList) return NULL;
	if (!ent_list_find_ent_by_classname) return NULL;

	void **this_ptr = *(void ***)&g_pEList;
	void *func = ent_list_find_ent_by_classname;

	union {CBaseEntity * (ManiEmptyClass::*mfpnew)(CBaseEntity *pCBE, const char *ent_class);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseEntity *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(pCBE, ent_class);
}

bool CCSPlayer_SwitchTeam(CBaseEntity *pCBE, int team_index)
{
//	pWeapon = pThisPtr->Weapon_GetSlot(slot);

	if (!switch_team_addr) return false;

	CBasePlayer *pCBP = (CBasePlayer *) pCBE;

	void **this_ptr = *(void ***)&pCBP;
	void *func = switch_team_addr;

	union {void (ManiEmptyClass::*mfpnew)(int team_index);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(team_index);
	return true;
}

bool CCSPlayer_SetModelFromClass(CBaseEntity *pCBE)
{
	if (!set_model_from_class) return false;

	CBasePlayer *pCBP = (CBasePlayer *) pCBE;

	void **this_ptr = *(void ***)&pCBP;
	void *func = set_model_from_class;

	union {void (ManiEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	(void) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)();
	return true;
}

//int CCSWeaponInfo_GetWeaponPriceFunc(CCSWeaponInfo *weapon_info)
//{
//	if (!get_weapon_price_addr) return -1;
//
//	void **this_ptr = *(void ***)&weapon_info;
//	void *func = get_weapon_price_addr;
//
//	union {int (ManiEmptyClass::*mfpnew)(CCSWeaponInfo *weapon_info);
//#ifndef __linux__
//        void *addr;	} u; 	u.addr = func;
//#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
//			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
//#endif
//
//	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_info);
//}

//int CCSGameRules_GetBlackMarketPriceForWeaponFunc(int weapon_id)
//{
//	if (!get_black_market_price_addr) return -1;
//	if (!g_pGRules) return -1;
//
//	void **this_ptr = *(void ***)g_pGRules;
//	void *func = get_black_market_price_addr;
//
//	union {int (ManiEmptyClass::*mfpnew)(int weapon_id);
//#ifndef __linux__
//        void *addr;	} u; 	u.addr = func;
//#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
//			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
//#endif
//
//	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_id);
//}

//CBaseCombatWeapon *CBaseCombatCharacter_GetWeapon(CBaseCombatCharacter *pCBCC, int weapon_number)
//{
//	if (!get_weapon_addr) return NULL;
//
//	void **this_ptr = *(void ***)&pCBCC;
//	void *func = get_weapon_addr;
//
//	union {CBaseCombatWeapon *(ManiEmptyClass::*mfpnew)(int weapon_number);
//#ifndef __linux__
//        void *addr;	} u; 	u.addr = func;
//#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
//			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
//#endif
//
//	return (CBaseCombatWeapon *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_number);
//}

CBaseCombatWeapon *CBaseCombatCharacter_Weapon_OwnsThisType(CBaseCombatCharacter *pCBCC, const char *weapon_name, int sub_type)
{
	if (!weapon_owns_this_type_addr) return NULL;

	void **this_ptr = *(void ***)&pCBCC;
	void *func = weapon_owns_this_type_addr;

	union {CBaseCombatWeapon *(ManiEmptyClass::*mfpnew)(const char *weapon_name, int sub_type);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseCombatWeapon *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_name, sub_type);
}


void LoadSigScans(void)
{
	bool libload_failed = false;
#ifdef WIN32
	CWinSigUtil *sig_util_ptr;
	sig_util_ptr = new CWinSigUtil;
	if (!sig_util_ptr->GetLib(engine))
	{
		MMsg("Failed to get base info for engine\n");
		libload_failed = true;
	}
#else 
	SymbolMap *linux_sym_ptr;
	linux_sym_ptr = new SymbolMap;
	if (!linux_sym_ptr->GetLib(gpManiGameType->GetLinuxEngine()))
	{
		MMsg("Failed to open [%s]\n", gpManiGameType->GetLinuxEngine());
		libload_failed = true;
	}
#endif

	if (!libload_failed)
	{
		connect_client_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CBaseServer_ConnectClient");
		netsendpacket_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "NET_SendPacket"); 
	}

#ifdef WIN32
	delete sig_util_ptr;
#else
	/* Call deconstructor to cleanup */
	delete linux_sym_ptr;
#endif

	if ((!gpManiGameType->IsGameType(MANI_GAME_CSS)) && (!gpManiGameType->IsGameType(MANI_GAME_CSGO))) return;
	libload_failed = false;

#ifdef WIN32
	sig_util_ptr = new CWinSigUtil;
	if (!sig_util_ptr->GetLib(gamedll))
	{
		MMsg("Failed to get base info for gamedll\n");
		libload_failed = true;
	}
#else 
	linux_sym_ptr = new SymbolMap;
	if (!linux_sym_ptr->GetLib(gpManiGameType->GetLinuxBin()))
	{
		MMsg("Failed to open [%s]\n", gpManiGameType->GetLinuxBin());
		libload_failed = true;
	}
#endif

	if (!libload_failed)
	{
		respawn_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CCSPlayer_RoundRespawn");
		util_remove_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "UTIL_Remove");
		g_pEList = (CBaseEntityList *) FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CEntList_gEntList");
		g_pGRules = (CGameRules *) FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CGameRules_gGameRules");
		ent_list_find_ent_by_classname = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CGlobalEntityList_FindEntityByClassname"); 
		switch_team_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CCSPlayer_SwitchTeam"); 
		set_model_from_class = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CCSPlayer_SetModelFromClass");
		get_file_weapon_info_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "GetFileWeaponInfoFromHandle");
		weapon_owns_this_type_addr = FIND_ADDRESS(sig_util_ptr, linux_sym_ptr, "CBaseCombatCharacter_Weapon_OwnsThisType");
	}

#ifdef WIN32
	delete sig_util_ptr;
#else
	/* Call deconstructor to cleanup */
	delete linux_sym_ptr;
#endif

	if (util_remove_addr != NULL)
	{
		UTILRemoveFunc = (UTIL_Remove_) util_remove_addr;
	}

	if (get_file_weapon_info_addr != NULL)
	{
		CCSGetFileWeaponInfoHandleFunc = (CCSGetFileWeaponInfoHandle_) get_file_weapon_info_addr;
	}
}

static
void ShowSigInfo(void *ptr, char *sig_name)
{
	if (ptr == NULL)
	{
		MMsg("%s Failed\n", sig_name);
	}
	else
	{
		MMsg("%s [%p]\n", sig_name, ptr);
	}
}

#ifdef WIN32
static void *FindAddress(CWinSigUtil *sig_util_ptr, char *sig_name)
#else
static void *FindAddress(SymbolMap *sym_map_ptr, char *sig_name)
#endif
{
	sigscan_t *sigscan_details = gpManiGameType->GetSigDetails(sig_name);
	if (sigscan_details == NULL)
	{
		MMsg("Failed to find sig [%s] in gametypes.txt\n", sig_name);
		return NULL;
	}

#ifdef WIN32
	int	sig_type = sigscan_details->win_sig_type;
	int	index = sigscan_details->win_index;

	void *ptr = sig_util_ptr->FindSignature((unsigned char *) sigscan_details->sigscan);
#else
	int	sig_type = sigscan_details->linux_sig_type;
	int	index = sigscan_details->linux_index;

	void *ptr = sym_map_ptr->FindAddress(sigscan_details->linux_symbol);
#endif

	if (ptr != NULL)
	{
		switch(sig_type)
		{
			case SIG_DIRECT:
				if (index != 0)
				{
					MMsg("  Initial [%p] Sig [%s]\n", ptr, sigscan_details->sig_name);
				}
				ptr = (void *) ((unsigned long) ptr + (unsigned long) index);
				break;
			case SIG_INDIRECT:
				MMsg("  Initial [%p] Sig [%s]\n", ptr, sigscan_details->sig_name);
				ptr = *(void **) ((unsigned long) ptr + (unsigned long) index);
				break;
			default : 
				ptr = NULL;
				break;
		}
	}

	if (ptr == NULL)
	{
		MMsg("Sig [%s] Failed!!\n", sigscan_details->sig_name);
	}
	else
	{
		MMsg("Final [%p] [%s]\n", ptr, sigscan_details->sig_name);
	}

	return ptr;
}


