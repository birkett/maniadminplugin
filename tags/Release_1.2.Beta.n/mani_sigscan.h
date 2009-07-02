// Sigscan code developed by Bailopan from www.sourcemm.net

#ifndef MANI_SIGSCAN_H
#define MANI_SIGSCAN_H

#include "mani_player.h"

class CGameRules;

#define MKSIG(name) name##_Sig, name##_SigBytes

#define CCSPlayer_RoundRespawn_Sig "\x56\x8B\xF1\x8B\x06\xFF\x90\x60\x04\x00\x00\x8B\x86\x2A\x0D\x00"
#define CCSPlayer_RoundRespawn_SigBytes 16
#define CCSPlayer_RoundRespawn_Linux "_ZN9CCSPlayer12RoundRespawnEv"

#define UTIL_Remove_Sig "\x8B\x44\x24\x04\x85\xC0\x74\x2A\x05\x2A\x2A\x00\x00\x89\x44\x24\x04\xE9\x2A\xFF\xFF\xFF"
#define UTIL_Remove_SigBytes 22
#define UTIL_Remove_Linux "_Z11UTIL_RemoveP11CBaseEntity"

// Win32 function is IsThereABomb
#define CEntList_Sig			"\x53\x68\x2A\x2A\x2A\x2A\x6A\x00\xB9\x2A\x2A\x2A\x2A\x32\xDB"
#define CEntList_SigBytes		15
#define CEntList_Linux			"g_pEntityList"

// Find CBasePlayer_UpdateClientData, then look for mov ecx, dword_xxxxxxxx (there are at least two of them)
// Around ResetHUD string
#define CBasePlayer_UpdateClientData_Sig "\x83\xEC\x34\x53\x56\x8B\xF1\x57\x8D\x4C\x24\x20"
#define CBasePlayer_UpdateClientData_SigBytes 12
// Direct linux lookup for gamerules

#define CBaseCombatCharacter_SwitchToNextBestWeapon_Linux "_ZN20CBaseCombatCharacter22SwitchToNextBestWeaponEP17CBaseCombatWeapon"
#define CGameRules_gGameRules_Linux "g_pGameRules"
#define CGameRules_gGameRules	0x6D

//bytes into the function
//gEntList
#define CEntList_gEntList			0x9

#define CGlobalEntityList_FindEntityByClassname_Sig "\x53\x55\x56\x8B\xF1\x8B\x4C\x24\x2A\x85\xC9\x57\x74\x2A\x8B\x01\xFF\x2A\x2A\x8B\x08"
#define CGlobalEntityList_FindEntityByClassname_SigBytes 21
#define CGlobalEntityList_FindEntityByClassname_Linux "_ZN17CGlobalEntityList21FindEntityByClassnameEP11CBaseEntityPKc"

// Look for CCSPlayer::SwitchTeam( %d )
#define CCSPlayer_SwitchTeam_Sig "\x83\xEC\x2A\x56\x57\x8B\x7C\x24\x2A\x57\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"
#define CCSPlayer_SwitchTeam_SigBytes 20
#define CCSPlayer_SwitchTeam_Linux "_ZN9CCSPlayer10SwitchTeamEi"

// Look for class_ter/class_ct, the call xxxxxx above each one is CBaseEntityGetTeamNumber
// below both at the bottom is the call to _ZN9CCSPlayer23HandleCommand_JoinClass
// In _ZN9CCSPlayer23HandleCommand_JoinClass there are two calls to CBaseEntityGetTeamNumber, the next call is 
// to SetModelFromClass
#define CCSPlayer_SetModelFromClass_Sig "\x51\x56\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xF8\x02\x75\x2A\x8B\x86"
#define CCSPlayer_SetModelFromClass_SigBytes 16
#define CCSPlayer_SetModelFromClass_Linux "_ZN9CCSPlayer17SetModelFromClassEv"

extern bool	CCSRoundRespawn(CBaseEntity *pCBE);
extern bool	CCSUTILRemove(CBaseEntity *pCBE);

//extern void	CONSOLEEcho(char *,...);
extern CBaseEntity *CGlobalEntityList_FindEntityByClassname(CBaseEntity *pCBE, const char *ent_class);
extern bool CCSPlayer_SwitchTeam(CBaseEntity *pCBE, int team_index);
extern bool CCSPlayer_SetModelFromClass(CBaseEntity *pCBE);

extern CBaseEntityList *g_pEList;
extern CGameRules *g_pGRules;

extern void LoadSigScans(void);

#endif
