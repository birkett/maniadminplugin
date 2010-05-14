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

#define MKSIG(name) name##_Sig, name##_SigBytes
// A
//#define CCSPlayer_RoundRespawn_Sig "\x56\x8B\xF1\x8B\x06\xFF\x90\x60\x04\x00\x00\x8B\x86\x2A\x0D\x00"
#define CCSPlayer_RoundRespawn_Sig "\x56\x8B\xF1\x8B\x06\xFF\x90\x2A\x04\x00\x00\x8B\x86\x2A\x0D\x00"
#define CCSPlayer_RoundRespawn_SigBytes 16
#define CCSPlayer_RoundRespawn_Linux "_ZN9CCSPlayer12RoundRespawnEv"

// B
#ifdef ORANGE
#define UTIL_Remove_Sig "\x8B\x44\x24\x04\x85\xC0\x74\x2A\x83\xC0\x0C\x89\x44\x24\x04\xE9\x7C\xFF\xFF\xFF"
#define UTIL_Remove_SigBytes 20
#else
#define UTIL_Remove_Sig "\x8B\x44\x24\x04\x85\xC0\x74\x2A\x05\x2A\x2A\x00\x00\x89\x44\x24\x04\xE9\x2A\xFF\xFF\xFF"
#define UTIL_Remove_SigBytes 20
#endif
#define UTIL_Remove_Linux "_Z11UTIL_RemoveP11CBaseEntity"

// Win32 function is IsThereABomb (look for weapon_c4 and planted_c4
#define CEntList_Sig			"\x53\x68\x2A\x2A\x2A\x2A\x6A\x00\xB9\x2A\x2A\x2A\x2A\x32\xDB"
#define CEntList_SigBytes		15
#define CEntList_Linux			"g_pEntityList"

// Find CBasePlayer_UpdateClientData, then look for mov ecx, dword_xxxxxxxx (there are at least two of them)
// Around ResetHUD string
#define CBasePlayer_UpdateClientData_Sig "\x83\xEC\x38\x53\x56\x8B\xF1\x57\x8D\x4C"
#define CBasePlayer_UpdateClientData_SigBytes 10
// Direct linux lookup for gamerules

#define CBaseCombatCharacter_SwitchToNextBestWeapon_Linux "_ZN20CBaseCombatCharacter22SwitchToNextBestWeaponEP17CBaseCombatWeapon"
#define CGameRules_gGameRules_Linux "g_pGameRules"
#define CGameRules_gGameRules	0x6D

//bytes into the function
//gEntList
#define CEntList_gEntList			0x9

// E
#ifdef ORANGE
#define CGlobalEntityList_FindEntityByClassname_Sig "\x53\x55\x56\x8B\xF1\x8B\x4C\x24\x2A\x85\xC9\x57\x74\x2A\x8B\x01\x8B\x50\x08\xFF\xD2"
#define CGlobalEntityList_FindEntityByClassname_SigBytes 21
#else
#define CGlobalEntityList_FindEntityByClassname_Sig "\x53\x55\x56\x8B\xF1\x8B\x4C\x24\x2A\x85\xC9\x57\x74\x2A\x8B\x01\xFF\x2A\x2A\x8B\x08"
#define CGlobalEntityList_FindEntityByClassname_SigBytes 21
#endif
#define CGlobalEntityList_FindEntityByClassname_Linux "_ZN17CGlobalEntityList21FindEntityByClassnameEP11CBaseEntityPKc"

// Look for CCSPlayer::SwitchTeam( %d )
#define CCSPlayer_SwitchTeam_Sig "\x83\xEC\x2A\x56\x57\x8B\x7C\x24\x2A\x57\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"
#define CCSPlayer_SwitchTeam_SigBytes 20
#define CCSPlayer_SwitchTeam_Linux "_ZN9CCSPlayer10SwitchTeamEi"

// Look for CCSPlayerSpawn, 3rd call down is SetModelClass
//#define CCSPlayer_SetModelFromClass_Sig "\x51\x56\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xF8\x02\x75\x2A\x8B\x86"
#define CCSPlayer_SetModelFromClass_Sig "\x51\x53\x56\x57\x8B\xF9\xE8\x2A\x2A\x2A\x2A\x83\xF8\x02"
#define CCSPlayer_SetModelFromClass_SigBytes 14
#define CCSPlayer_SetModelFromClass_Linux "_ZN9CCSPlayer17SetModelFromClassEv"

#define GetFileWeaponInfoFromHandle_Sig "\x66\x8B\x44\x24\x04\x66\x3B\x05\x2A\x2A\x2A\x2A\x73\x2A\x66\x3D\xFF\xFF"
#define GetFileWeaponInfoFromHandle_SigBytes 18
#define GetFileWeaponInfoFromHandle_Linux "_Z27GetFileWeaponInfoFromHandlet"

//#define CCSWeaponInfo_GetWeaponPrice_Sig "\x85\xC0\x0F\x8E\x2A\x2A\x2A\x2A\x83\xBF\x2A\x2A\x2A\x2A\x08\x55\x0F\x85"
#define CCSWeaponInfo_GetWeaponPrice_Sig   "\xF7\xD8\x50\x8B\xCE\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x56\x51\xE8"
#define CCSWeaponInfo_GetWeaponPrice_Offset -4
#define CCSWeaponInfo_GetWeaponPrice_SigBytes 17
#define CCSWeaponInfo_GetWeaponPrice_Linux "_ZN13CCSWeaponInfo14GetWeaponPriceEv"

#define CCSWeaponInfo_GetDefaultPrice_Sig "\x89\x87\x2a\x2a\x2a\x2a\x83\xC6\x01\x83\xFE\x22\x7C\x2a\x5F\x5E\x5D\x5B"
#define CCSWeaponInfo_GetDefaultPrice_Offset -1
#define CCSWeaponInfo_GetDefaultPrice_SigBytes 18
#define CCSWeaponInfo_GetDefaultPrice_Linux "_ZN13CCSWeaponInfo15GetDefaultPriceEv"

#define CBaseCombatCharacter_Weapon_OwnsThisType_Sig "53\x8B\x5C\x24\x08\x55\x56\x8B\xE9\x57\x33\xFF\x8D\xB5\x2a\x2a\x2a\x2a\x8B\x0E\x83\xF9\xFF\x74"
#define CBaseCombatCharacter_Weapon_OwnsThisType_SigBytes 24
#define CBaseCombatCharacter_Weapon_OwnsThisType_Linux "_ZNK20CBaseCombatCharacter19Weapon_OwnsThisTypeEPKci"

// CBaseCombatCharacter_GetWeapon
// Find text 'Battery', skip next two calls one of the next 2 calls is to the function
#ifdef ORANGE
#define CBaseCombatCharacter_GetWeapon_Sig "\x8B\x44\x24\x04\x8B\x84\x81\x2a\x2a\x2a\x2a\x83\xF8\xFF\x74\x2a\x8B\x15\x2a\x2a\x2a\x2a\x8B\xC8\x81\x2a\x2a\x2a\x2a\x2a\xC1\xE1\x04\x8D\x4C\x11\x04\xC1\xE8\x0C\x39\x41\x04\x75\x2a\x8B\x01\xC2\x08\x00
#define CBaseCombatCharacter_GetWeapon_SigBytes 50
#else
#define CBaseCombatCharacter_GetWeapon_Sig "\x8B\x44\x24\x04\x8B\x84\x81\x2a\x2a\x2a\x2a\x83\xF8\xFF\x74\x2a\x8B\x15\x2a\x2a\x2a\x2a\x8B\xC8"
#define CBaseCombatCharacter_GetWeapon_SigBytes 24
#endif
#define CBaseCombatCharacter_GetWeapon_Linux "_ZNK20CBaseCombatCharacter9GetWeaponEi"

// K
// _ZN12CCSGameRules28GetBlackMarketPriceForWeaponEi
#ifdef ORANGE
#define CCSGameRules_GetBlackMarketPriceForWeapon_Sig "\x56\x8B\xF1\x83\xBE\x2a\x2a\x2a\x2a\x2a\x75\x2a\xE8\x2a\x2a\x2a\x2a\x8B\xB6\x2a\x2a\x2a\x2a\x85\xF6\x74\x2a\x8B\x44\x24\x08"
#define CCSGameRules_GetBlackMarketPriceForWeapon_SigBytes 31
#else
#define CCSGameRules_GetBlackMarketPriceForWeapon_Sig "\x56\x8B\xF1\x83\xBE\x2a\x2a\x2a\x2a\x2a\x75\x2a\xE8\x2a\x2a\x2a\x2a\x8B\x86\x2a\x2a\x2a\x2a\x8B"
#define CCSGameRules_GetBlackMarketPriceForWeapon_SigBytes 24
#endif
#define CCSGameRules_GetBlackMarketPriceForWeapon_Linux "_ZN12CCSGameRules28GetBlackMarketPriceForWeaponEi"

// Used in Reserve Slots - Thanks to *pRED
#if defined ( ORANGE )
	#define CBaseServer_ConnectClient_Sig "\x83\x2A\x2A\x56\x68\x2A\x2A\x2A\x2A\x8B\xF1\xFF\x15\x2A\x2A\x2A\x2A\x8B\x06\x8B\x50"
	#define CBaseServer_ConnectClient_SigBytes 21
	#define CBaseServer_ConnectClient_Linux "_ZN11CBaseServer13ConnectClientER8netadr_siiiPKcS3_S3_i"
#else
	#define CBaseServer_ConnectClient_Sig "\x56\x68\x2A\x2A\x2A\x2A\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x8B\x06\x83\xC4\x04\x8B\xCE"
	#define CBaseServer_ConnectClient_SigBytes 20
	#define CBaseServer_ConnectClient_Linux "_ZN11CBaseServer13ConnectClientER8netadr_siiiPKcS3_S3_iS3_i"
#endif

#if defined ( ORANGE )
	#define NET_SendPacket_Sig "\xB8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\x78\x2A\x2A\x53\x55\x8B"
	#define NET_SendPacket_SigBytes 22
	#define NET_SendPacket_Linux "_Z14NET_SendPacketP11INetChanneliRK8netadr_sPKhiP8bf_writeb"
#else
	#define NET_SendPacket_Sig "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x2a\x83\x78\x2A\x2A\x55\x8B\x6C\x2A\x2A\x56\x57\x8B\x7C"
	#define NET_SendPacket_SigBytes 21
	#define NET_SendPacket_Linux "_Z14NET_SendPacketP11INetChanneliRK8netadr_sPKhi"
#endif

extern bool	CCSRoundRespawn(CBaseEntity *pCBE);
extern bool	CCSUTILRemove(CBaseEntity *pCBE);

//extern void	CONSOLEEcho(char *,...);
extern CBaseEntity *CGlobalEntityList_FindEntityByClassname(CBaseEntity *pCBE, const char *ent_class);
extern bool CCSPlayer_SwitchTeam(CBaseEntity *pCBE, int team_index);
extern bool CCSPlayer_SetModelFromClass(CBaseEntity *pCBE);

extern CBaseCombatWeapon *CBaseCombatCharacter_GetWeapon(CBaseCombatCharacter *pCBCC, int weapon_number);

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
