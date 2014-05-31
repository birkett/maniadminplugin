//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_WEAPON_H
#define MANI_WEAPON_H

#include <map>
#include "cbaseentity.h"

#if defined ( GAME_CSGO )
#define MAX_WEAPONS_USED (42)
#else
#define MAX_WEAPONS_USED (29)
#endif
enum
{
	WEAPON_RESTRICT = 0,
	WEAPON_LIMIT,
	WEAPON_RATIO
};

class MWeapon
{
public:
	MWeapon(){};
	~MWeapon(){};
	MWeapon(const char *weapon_name, const int translation_id, int weapon_index);
	const char *GetWeaponName() {return weapon_name;}
	const int	GetDisplayID() {return translation_id;}
	void	SetTeamLimit(int limit) {team_limit = limit;}
	void	SetRestricted(bool enabled) {restricted = enabled;}
	void	SetRoundRatio(int ratio) {round_ratio = ratio;}
	bool	CanBuy(player_t *player_ptr, int offset, int &reason, int &limit, int &ratio);
	bool	IsRestricted() {return restricted;}
	int		GetTeamLimit(){return team_limit;}
	int		GetRoundRatio() {return round_ratio;}
	int		GetWeaponIndex() {return index;}

private:
	int		index;
	char	weapon_name[80];
	int		translation_id;
	bool	restricted;
	int		team_limit;
	int		round_ratio;
	int		count[4];
};

class ManiWeaponMgr
{
public:
	MENUFRIEND_DEC(RestrictWeapon);

	ManiWeaponMgr();
	~ManiWeaponMgr();
	void	Load();
	void	Unload();
	void	LevelInit();
	void	PlayerSpawn(player_t *player_ptr); 
	PLUGIN_RESULT	CanBuy(player_t *player_ptr, const char *alias_name);
	void	AutoBuyReBuy();
	void	PreAutoBuyReBuy();
	void	RestrictAll();
	void	UnRestrictAll();
	bool	SetWeaponRestriction(const char *weapon_name, bool restricted, int team_limit = 0);
	bool	SetWeaponRatio(const char *weapon_name, int ratio);
	void	RemoveWeapons(player_t *player_ptr, bool refund, bool show_refund);
	void	RoundStart();
	bool	CanPickUpWeapon(CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon);
	void	ClientActive(player_t *player_ptr);
	void	ClientDisconnect(player_t *player_ptr);
	void	LevelShutdown(void);
	const char *GetWeaponName(int weapon_index);


	PLUGIN_RESULT	ProcessMaShowRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaKnives(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaPistols(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaShotguns(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaNoSnipers(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUnRestrict(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaUnRestrictAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRestrictAll(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaRestrictRatio(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);

private:
	void	ShowRestrictReason(player_t *player_ptr, MWeapon *weapon, int reason, int limit, int ratio);
	void	CleanUp();
	void	SetupWeapons();
	void	LoadRestrictions();
	void	AddWeapon(const char *search_name, int translation_id);
	void	AddWeapon(const char *search_name, int translation_id, const char *alias1);
	void	AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2);
	void	AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2, const char *alias3);
	void	AddWeapon(const char *search_name, int translation_id, const char *alias1, const char *alias2, const char *alias3, const char *alias4);
	int		FindWeaponIndex(const char *search_name);

	std::map <BasicStr, MWeapon *> alias_list;
	MWeapon *weapons[MAX_WEAPONS_USED];
	bool	hooked[MANI_MAX_PLAYERS];
	bool	ignore_hook[MANI_MAX_PLAYERS];
	float	next_message[MANI_MAX_PLAYERS];
};

extern	ManiWeaponMgr *gpManiWeaponMgr;

extern	void ShowWeapons(void);

MENUALL_DEC(RestrictWeapon);


#endif
