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
void *get_weapon_price_addr = NULL;
void *get_weapon_addr = NULL;
void *get_black_market_price_addr = NULL;
void *update_client_addr = NULL;
void *connect_client_addr = NULL;

CBaseEntityList *g_pEList = NULL;
CGameRules *g_pGRules = NULL;
UTIL_Remove_ UTILRemoveFunc;
CCSGetFileWeaponInfoHandle_ CCSGetFileWeaponInfoHandleFunc;

static void ShowSigInfo(void *ptr, char *sig_name);

#ifdef WIN32
static	void *FindSignature( unsigned char *pBaseAddress, size_t baseLength, unsigned char *pSignature, size_t sigLength);
static  bool GetDllMemInfo(void *pAddr, unsigned char **base_addr, size_t *base_len);
#else
static void	*FindAddress(char *address_name, bool gamebin = false);
#endif

class ManiEmptyClass {};

#ifdef WIN32
static
void *FindSignature( unsigned char *pBaseAddress, size_t baseLength, unsigned char *pSignature, size_t sigLength)
{
	unsigned char *pBasePtr = pBaseAddress;
	unsigned char *pEndPtr = pBaseAddress + baseLength;
	size_t i;
	while (pBasePtr < pEndPtr)
	{
		for (i=0; i<sigLength; i++)
		{
			if (pSignature[i] != '\x2A' && pSignature[i] != pBasePtr[i])
				break;
		}
		//iff i reached the end, we know we have a match!
		if (i == sigLength)
			return (void *)pBasePtr;
		pBasePtr += sizeof(unsigned char);  //search memory in an aligned manner
	}

	return NULL;
}
//Example usage:
//void *sigaddr = FindSignature(pBaseAddress, baseLength, MKSIG(TerminateRound));

static
bool GetDllMemInfo(void *pAddr, unsigned char **base_addr, size_t *base_len)
{
	MEMORY_BASIC_INFORMATION mem;

	if(!pAddr)
		return false; // GetDllMemInfo failed: !pAddr

	if(!VirtualQuery(pAddr, &mem, sizeof(mem)))
		return false;

	*base_addr = (unsigned char *)mem.AllocationBase;

	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)mem.AllocationBase;
	IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS*)((unsigned long)dos+(unsigned long)dos->e_lfanew);

	if(pe->Signature != IMAGE_NT_SIGNATURE) {
		*base_addr = 0;
		return false; // GetDllMemInfo failed: pe points to a bad location
	}

	*base_len = (size_t)pe->OptionalHeader.SizeOfImage; 
	return true;
}

#else
static
void	*FindAddress(char *address_name, bool gamebin)
{
	void	*handle;
	void	*var_address;

	if ( gamebin )
		handle = dlopen(gpManiGameType->GetLinuxBin(), RTLD_NOW);
	else
		handle = dlopen(gpManiGameType->GetLinuxEngine(), RTLD_NOW);

	if (handle == NULL)
	{
		MMsg("Failed to open server image, error [%s]\n", dlerror());
		return NULL;
	}
	else
	{ 
		var_address = dlsym(handle, address_name);
		if (var_address == NULL)
		{
			dlclose(handle);
			return (void *) NULL;
		}

		dlclose(handle);
	}

	return var_address;
}
#endif

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

int CCSWeaponInfo_GetWeaponPriceFunc(CCSWeaponInfo *weapon_info)
{
	if (!get_weapon_price_addr) return -1;

	void **this_ptr = *(void ***)&weapon_info;
	void *func = get_weapon_price_addr;

	union {int (ManiEmptyClass::*mfpnew)(CCSWeaponInfo *weapon_info);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_info);
}

int CCSGameRules_GetBlackMarketPriceForWeaponFunc(int weapon_id)
{
	if (!get_black_market_price_addr) return -1;
	if (!g_pGRules) return -1;

	void **this_ptr = *(void ***)g_pGRules;
	void *func = get_black_market_price_addr;

	union {int (ManiEmptyClass::*mfpnew)(int weapon_id);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (int) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_id);
}

CBaseCombatWeapon *CBaseCombatCharacter_GetWeapon(CBaseCombatCharacter *pCBCC, int weapon_number)
{
	if (!get_weapon_addr) return NULL;

	void **this_ptr = *(void ***)&pCBCC;
	void *func = get_weapon_addr;

	union {CBaseCombatWeapon *(ManiEmptyClass::*mfpnew)(int weapon_number);
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseCombatWeapon *) (reinterpret_cast<ManiEmptyClass*>(this_ptr)->*u.mfpnew)(weapon_number);
}



void LoadSigScans(void)
{
	// Need to get engine hook here for clients connecting
#ifdef WIN32
	unsigned char *engine_base = 0;
	size_t engine_len = 0;

	bool engine_success = GetDllMemInfo(engine, &engine_base, &engine_len);

	if (engine_success) {
		MMsg("Found engine base %p and length %i [%p]\n", engine_base, engine_len, engine_base + engine_len);
		connect_client_addr = FindSignature(engine_base, engine_len, (unsigned char*) MKSIG(CBaseServer_ConnectClient));	
	}
#else
	connect_client_addr = FindAddress(CBaseServer_ConnectClient_Linux);	
#endif
	MMsg("Sigscan info\n"); 
	ShowSigInfo(connect_client_addr, "CBaseServer::ConnectClient");

	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
#ifdef WIN32
	// Windows
	unsigned char *base = 0;
	size_t len = 0;

	bool success = GetDllMemInfo(gamedll, &base, &len);

	if (success)
	{
		MMsg("Found base %p and length %i [%p]\n", base, len, base + len);

		respawn_addr = FindSignature(base, len, (unsigned char *) MKSIG(CCSPlayer_RoundRespawn));
		util_remove_addr = FindSignature(base, len, (unsigned char *) MKSIG(UTIL_Remove));
		void *is_there_a_bomb_addr = FindSignature(base, len, (unsigned char *) MKSIG(CEntList));
		if (is_there_a_bomb_addr)
		{
			g_pEList = *(CBaseEntityList **) ((unsigned long) is_there_a_bomb_addr + (unsigned long) CEntList_gEntList);
		}

		update_client_addr = FindSignature(base, len, (unsigned char *) MKSIG(CBasePlayer_UpdateClientData));
		if (update_client_addr)
		{
			g_pGRules = *(CGameRules **) ((unsigned long) update_client_addr + (unsigned long) CGameRules_gGameRules);
		}

		ent_list_find_ent_by_classname = FindSignature(base, len, (unsigned char *) MKSIG(CGlobalEntityList_FindEntityByClassname));
		switch_team_addr = FindSignature(base, len, (unsigned char *) MKSIG(CCSPlayer_SwitchTeam));
		set_model_from_class = FindSignature(base, len, (unsigned char *) MKSIG(CCSPlayer_SetModelFromClass));
		get_file_weapon_info_addr = FindSignature(base, len, (unsigned char *) MKSIG(GetFileWeaponInfoFromHandle));
		get_weapon_price_addr = FindSignature(base, len, (unsigned char *) MKSIG(CCSWeaponInfo_GetWeaponPrice));
		if (get_weapon_price_addr)
		{
			unsigned long offset = *(unsigned long *) ((unsigned long) get_weapon_price_addr + (unsigned long) (CCSWeaponInfo_GetWeaponPrice_Offset));
			get_weapon_price_addr = (void *) ((unsigned long) get_weapon_price_addr + (unsigned long) (offset));
		}
		get_weapon_addr = FindSignature(base, len, (unsigned char *) MKSIG(CBaseCombatCharacter_GetWeapon));
		get_black_market_price_addr = FindSignature(base, len, (unsigned char *) MKSIG(CCSGameRules_GetBlackMarketPriceForWeapon));
	}
	else
	{
		MMsg("Did not find base and length for gamedll\n");
	}

#else
	void *ptr;

	// Linux
	if (!gpManiGameType->IsGameType(MANI_GAME_CSS)) return;
	respawn_addr = FindAddress(CCSPlayer_RoundRespawn_Linux);
	util_remove_addr = FindAddress(UTIL_Remove_Linux);
	g_pEList = *(CBaseEntityList **) FindAddress(CEntList_Linux);
	ent_list_find_ent_by_classname = FindAddress(CGlobalEntityList_FindEntityByClassname_Linux);
	switch_team_addr = FindAddress(CCSPlayer_SwitchTeam_Linux);
	set_model_from_class = FindAddress(CCSPlayer_SetModelFromClass_Linux);

	ptr = FindAddress(CBaseCombatCharacter_SwitchToNextBestWeapon_Linux);
	if (ptr)
	{
		g_pGRules = *(CGameRules **) ((unsigned long) ptr + (unsigned long) 6);
	}

	get_file_weapon_info_addr = FindAddress(GetFileWeaponInfoFromHandle_Linux);
	get_weapon_price_addr = FindAddress(CCSWeaponInfo_GetWeaponPrice_Linux);
	get_weapon_addr = FindAddress(CBaseCombatCharacter_GetWeapon_Linux);
	get_black_market_price_addr = FindAddress(CCSGameRules_GetBlackMarketPriceForWeapon_Linux);


#endif
	MMsg("Sigscan info\n"); 
	ShowSigInfo(respawn_addr, "A");
	ShowSigInfo(util_remove_addr, "B");
	if (util_remove_addr != NULL)
	{
		UTILRemoveFunc = (UTIL_Remove_) util_remove_addr;
	}	

	ShowSigInfo(g_pEList, "C");
#ifdef WIN32
	ShowSigInfo(update_client_addr, "D");
#endif
	ShowSigInfo(g_pGRules, "D1");
	ShowSigInfo(ent_list_find_ent_by_classname, "E");
	ShowSigInfo(switch_team_addr, "F");
	ShowSigInfo(set_model_from_class, "G");
	ShowSigInfo(get_file_weapon_info_addr, "H");
	if (get_file_weapon_info_addr != NULL)
	{
		CCSGetFileWeaponInfoHandleFunc = (CCSGetFileWeaponInfoHandle_) get_file_weapon_info_addr;
	}

	ShowSigInfo(get_weapon_price_addr, "I");
	ShowSigInfo(get_weapon_addr, "J");
	ShowSigInfo(get_black_market_price_addr, "K");
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

