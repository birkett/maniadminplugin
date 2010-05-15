// Sigscan code developed by Bailopan from www.sourcemm.net

#ifndef MANI_SIGSCAN_H
#define MANI_SIGSCAN_H

#include "mani_player.h"

class CCSWeaponInfo
{
public:
	unsigned char dummy1[6];
	char weapon_name[80];
	unsigned char dummy2[1974];
	int	dynamic_price;
	int	standard_price;
	int previous_price;
};

class CGameRules;
class CBaseCombatCharacter;
class CBaseCombatWeapon;
class CBaseEntityList;

/*19:52.12    ( +c0ldfyr3 ) haha
  19:53.31    ( +c0ldfyr3 ) CBasePlayer_SetFOV
  19:55.10    ( +c0ldfyr3 ) sig was hard enough to find
  19:55.12    ( +c0ldfyr3 ) #define SetFOV_Sig									"\x53\x8B\x5C\x24\x08\x85\xDB\x57\x8B\xF9\x75\x07\x5F\x32\xC0\x5B"
  19:55.12    ( +c0ldfyr3 ) 	#define SetFOV_SigBytes								16
  19:55.25    ( +c0ldfyr3 ) hasnt changed in any updates lately
  19:55.49    ( +c0ldfyr3 ) bool CBasePlayer_SetFOV( CBasePlayer *pPlayer, CBaseEntity *pRequester, int FOV, float zoomRate )
*/

// A
#ifdef ORANGE
// We don't use the following commented out sigs but they could be useful for tracking down RoundRespawn again with IDA Pro
// and its ability to rename unknown functions with labels.
// CCSPlayer_PlayerClass is very short in length
//           mov     eax, [ecx+15FCh]
//           retn
// CBaseEntity_GetTeamNumber is very short in length
//           mov     eax, [ecx+1B8h]
//           retn
// #define CCSGameRules_GiveC4_Sig "81 EC ? ? ? ? 53 55 83 C8 FF 56 57 89 ? ? ? 89 ? ? ? A1 ? ? ? ? 33 ED 33 DB BF 01 00 00 00"
// #define CCSPlayer_CheckTKPunishment "B0 01 38 81 ? ? ? ? 75 ? 8B 15 ? ? ? ? 83 7A ? ? 74 ? 88 81 ? ? ? ? 8B 01 8B 90"
// #define UTIL_PlayerByIndex "8B 44 24 04 56 33 F6 85 C0 7E ? 8B 0D ? ? ? ? 3B 41 ? 7F ? 8B 0D ? ? ? ? 8B 11 50 8B 42"
// #define CCSGameRules_CleanUpMap "83 EC 08 80 B9 ? ? ? ? ? 0F 85 ? ? ? ? 56 6A 00 B9 ? ? ? ? E8 ? ? ? ? 8B F0 85 F6"
// #define CCSPlayer_ObserverRoundRespawn_Sig "56 57 8B F1 E8 ? ? ? ? 80 BE ? ? ? ? ? 8D BE ? ? ? ? 74 ? 57 8B CE E8"

#define CCSPlayer_RoundRespawn_Sig "56 8B F1 8B 06 8B 90 ? ? ? ? FF D2 8B 86 ? ? ? ? 85 C0 74 ? 8B 50 ? 85 D2 74 ? 8B 48"
#else
#define CCSPlayer_RoundRespawn_Sig "56 8B F1 8B 06 FF 90 ? 04 00 00 8B 86 ? 0D 00"
#endif
#define CCSPlayer_RoundRespawn_Linux "_ZN9CCSPlayer12RoundRespawnEv"

// B
#ifdef ORANGE
#define UTIL_Remove_Sig "8B 44 24 04 85 C0 74 ? 83 C0 0C 89 44 24 04 E9 7C FF FF FF"
#else
#define UTIL_Remove_Sig "8B 44 24 04 85 C0 74 ? 05 ? ? 00 00 89 44 24 04 E9 ? FF FF FF"
#endif
#define UTIL_Remove_Linux "_Z11UTIL_RemoveP11CBaseEntity"

// Win32 function is IsThereABomb (look for weapon_c4 and planted_c4
#define CEntList_Sig			"53 68 ? ? ? ? 6A 00 B9 ? ? ? ? 32 DB"
#define CEntList_Linux			"g_pEntityList"

// Find CBasePlayer_UpdateClientData, then look for mov ecx, dword_xxxxxxxx (there are at least two of them)
// Around ResetHUD string
#define CBasePlayer_UpdateClientData_Sig "83 EC 38 53 56 8B F1 57 8D 4C"
// Direct linux lookup for gamerules

#define CBaseCombatCharacter_SwitchToNextBestWeapon_Linux "_ZN20CBaseCombatCharacter22SwitchToNextBestWeaponEP17CBaseCombatWeapon"
#define CGameRules_gGameRules_Linux "g_pGameRules"
#define CGameRules_gGameRules	0x6D

//bytes into the function
//gEntList
#define CEntList_gEntList			0x9

// E
#ifdef ORANGE
#define CGlobalEntityList_FindEntityByClassname_Sig "53 55 56 8B F1 8B 4C 24 ? 85 C9 57 74 ? 8B 01 8B 50 08 FF D2"
#else
#define CGlobalEntityList_FindEntityByClassname_Sig "53 55 56 8B F1 8B 4C 24 ? 85 C9 57 74 ? 8B 01 FF ? ? 8B 08"
#endif
#define CGlobalEntityList_FindEntityByClassname_Linux "_ZN17CGlobalEntityList21FindEntityByClassnameEP11CBaseEntityPKc"

// Look for CCSPlayer::SwitchTeam( %d )
#define CCSPlayer_SwitchTeam_Sig "83 EC ? 56 57 8B 7C 24 ? 57 8B F1 E8 ? ? ? ? 83 C4 04"
#define CCSPlayer_SwitchTeam_Linux "_ZN9CCSPlayer10SwitchTeamEi"

// Look for CCSPlayerSpawn, 3rd call down is SetModelClass
#define CCSPlayer_SetModelFromClass_Sig "51 53 56 57 8B F9 E8 ? ? ? ? 83 F8 02"
#define CCSPlayer_SetModelFromClass_Linux "_ZN9CCSPlayer17SetModelFromClassEv"

#define GetFileWeaponInfoFromHandle_Sig "66 8B 44 24 04 66 3B 05 ? ? ? ? 73 ? 66 3D FF FF"
#define GetFileWeaponInfoFromHandle_Linux "_Z27GetFileWeaponInfoFromHandlet"

#define CCSWeaponInfo_GetWeaponPrice_Sig   "F7 D8 50 8B CE E8 ? ? ? ? ? ? ? ? 56 51 E8"
#define CCSWeaponInfo_GetWeaponPrice_Offset -4
#define CCSWeaponInfo_GetWeaponPrice_Linux "_ZN13CCSWeaponInfo14GetWeaponPriceEv"

#define CCSWeaponInfo_GetDefaultPrice_Sig "89 87 ? ? ? ? 83 C6 01 83 FE 22 7C ? 5F 5E 5D 5B"
#define CCSWeaponInfo_GetDefaultPrice_Offset -1
#define CCSWeaponInfo_GetDefaultPrice_Linux "_ZN13CCSWeaponInfo15GetDefaultPriceEv"

#define CBaseCombatCharacter_Weapon_OwnsThisType_Sig "53 8B 5C 24 08 55 56 8B E9 57 33 FF 8D B5 ? ? ? ? 8B 0E 83 F9 FF 74"
#define CBaseCombatCharacter_Weapon_OwnsThisType_Linux "_ZNK20CBaseCombatCharacter19Weapon_OwnsThisTypeEPKci"

// CBaseCombatCharacter_GetWeapon
// Find text 'Battery', skip next two calls one of the next 2 calls is to the function
#define CBaseCombatCharacter_GetWeapon_Sig "8B 44 24 04 8B 84 81 ? ? ? ? 83 F8 FF 74 ? 8B 15 ? ? ? ? 8B C8"
#define CBaseCombatCharacter_GetWeapon_Linux "_ZNK20CBaseCombatCharacter9GetWeaponEi"

// K
// _ZN12CCSGameRules28GetBlackMarketPriceForWeaponEi
#ifdef ORANGE
#define CCSGameRules_GetBlackMarketPriceForWeapon_Sig "56 8B F1 83 BE ? ? ? ? ? 75 ? E8 ? ? ? ? 8B B6 ? ? ? ? 85 F6 74 ? 8B 44 24 08"
#else
#define CCSGameRules_GetBlackMarketPriceForWeapon_Sig "56 8B F1 83 BE ? ? ? ? ? 75 ? E8 ? ? ? ? 8B 86 ? ? ? ? 8B"
#endif
#define CCSGameRules_GetBlackMarketPriceForWeapon_Linux "_ZN12CCSGameRules28GetBlackMarketPriceForWeaponEi"

// Used in Reserve Slots - Thanks to *pRED
#if defined ( ORANGE )
	#define CBaseServer_ConnectClient_Sig "83 ? ? 56 68 ? ? ? ? 8B F1 FF 15 ? ? ? ? 8B 06 8B 50"
	#define CBaseServer_ConnectClient_Linux "_ZN11CBaseServer13ConnectClientER8netadr_siiiPKcS3_S3_i"
#else
	#define CBaseServer_ConnectClient_Sig "56 68 ? ? ? ? 8B F1 E8 ? ? ? ? 8B 06 83 C4 04 8B CE"
	#define CBaseServer_ConnectClient_Linux "_ZN11CBaseServer13ConnectClientER8netadr_siiiPKcS3_S3_iS3_i"
#endif

#if defined ( ORANGE )
	#define NET_SendPacket_Sig "B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 78 ? ? 53 55 8B"
	#define NET_SendPacket_Linux "_Z14NET_SendPacketP11INetChanneliRK8netadr_sPKhiP8bf_writeb"
#else
	#define NET_SendPacket_Sig "A1 ? ? ? ? 83 EC ? 83 78 ? ? 55 8B 6C ? ? 56 57 8B 7C"
	#define NET_SendPacket_Linux "_Z14NET_SendPacketP11INetChanneliRK8netadr_sPKhi"
#endif

extern bool	CCSRoundRespawn(CBaseEntity *pCBE);
extern bool	CCSUTILRemove(CBaseEntity *pCBE);

//extern void	CONSOLEEcho(char *,...);
extern CBaseEntity *CGlobalEntityList_FindEntityByClassname(CBaseEntity *pCBE, const char *ent_class);
extern bool CCSPlayer_SwitchTeam(CBaseEntity *pCBE, int team_index);
extern bool CCSPlayer_SetModelFromClass(CBaseEntity *pCBE);

extern CBaseCombatWeapon *CBaseCombatCharacter_GetWeapon(CBaseCombatCharacter *pCBCC, int weapon_number);
extern CBaseCombatWeapon *CBaseCombatCharacter_Weapon_OwnsThisType(CBaseCombatCharacter *pCBCC, const char *weapon_name, int sub_type);

extern CCSWeaponInfo	*CCSGetFileWeaponInfoFromHandle(unsigned short handle_id);
extern CBaseEntityList *g_pEList;
extern CGameRules *g_pGRules;
extern int CCSWeaponInfo_GetWeaponPriceFunc(CCSWeaponInfo *weapon_info);

extern int CCSGameRules_GetBlackMarketPriceForWeaponFunc(int weapon_id);

extern void LoadSigScans(void);
#ifdef WIN32
extern void *FindSignature( unsigned char *pBaseAddress, size_t baseLength, unsigned char *pSignature, size_t sigLength);
extern bool GetDllMemInfo(void *pAddr, unsigned char **base_addr, size_t *base_len);
#endif

#endif
