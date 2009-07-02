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



#ifndef MANI_GAMETYPE_H
#define MANI_GAMETYPE_H

// Define known game types so far
#define MANI_GAME_UNKNOWN (0)
#define MANI_GAME_CSS_STR "Counter-Strike: Source"
#define MANI_GAME_CSS (1)
#define MANI_GAME_DM_STR "Deathmatch"
#define MANI_GAME_DM (2)
#define MANI_GAME_TEAM_DM_STR "Team Deathmatch"
#define MANI_GAME_TEAM_DM (3)
#define MANI_GAME_CTF_STR "Half-Life 2 CTF"
#define MANI_GAME_CTF (4)
#define MANI_GAME_HIDDEN_STR "Hidden : Source"
#define MANI_GAME_HIDDEN (5)
#define MANI_GAME_GARRYS_MOD_STR "Garry's Mod"
#define MANI_GAME_GARRYS_MOD (6)
#define MANI_GAME_DOD_STR1 "Day Of Defeat"
#define MANI_GAME_DOD_STR2 "Day Of Defeat: Source"
#define MANI_GAME_DOD (7)

// VFunc defs
#define MANI_VFUNC_EYE_ANGLES (0)
#define MANI_VFUNC_TELEPORT (1)
#define MANI_VFUNC_SET_MODEL_INDEX (2)
#define MANI_VFUNC_EYE_POSITION (3)
#define MANI_VFUNC_MY_COMBAT_CHARACTER (4)
#define MANI_VFUNC_IGNITE (5)
#define MANI_VFUNC_REMOVE_PLAYER_ITEM (6)
#define MANI_VFUNC_GET_WEAPON_SLOT (7)
#define MANI_VFUNC_GIVE_AMMO (8)
#define MANI_VFUNC_WEAPON_DROP (9)
#define MANI_VFUNC_GET_PRIMARY_AMMO_TYPE (10)
#define MANI_VFUNC_GET_SECONDARY_AMMO_TYPE (11)
#define MANI_VFUNC_WEAPON_GET_NAME (12)
//#define MANI_VFUNC_GET_TEAM_NUMBER (13)
//#define MANI_VFUNC_GET_TEAM_NAME (14)
#define MANI_VFUNC_GET_VELOCITY (15)
#define MANI_VFUNC_WEAPON_SWITCH (16)
#define MANI_VFUNC_USER_CMDS (17)
#define MANI_VFUNC_GIVE_ITEM (18)

//Property defs
#define MANI_PROP_HEALTH		(0)
#define MANI_PROP_RENDER_MODE	(1)
#define MANI_PROP_RENDER_FX		(2)
#define MANI_PROP_COLOUR		(3)
#define MANI_PROP_ACCOUNT		(4)
#define MANI_PROP_MOVE_TYPE		(5)
//#define MANI_PROP_DEATHS		(6)
#define MANI_PROP_ARMOR			(7)
//#define MANI_PROP_SCORE			(8)
#define MANI_PROP_MODEL_INDEX	(9)
#define MANI_PROP_VEC_ORIGIN	(10)
#define MANI_PROP_ANG_ROTATION	(11)

class ManiGameType
{

public:
	ManiGameType();
	~ManiGameType();

	void		Init(void);
	const char	*GetGameType(void);
	bool		IsGameType(const char *game_str);
	bool		IsGameType(int game_index) {return ((game_index == game_type_index) ? true:false);}
	bool		GetAdvancedEffectsAllowed(void);
	void		SetAdvancedEffectsAllowed(bool allowed);
	int			GetAdvancedEffectsVFuncOffset(void);
	int			GetAdvancedEffectsCodeOffset(void);
	const char	*GetLinuxBin(void);

	bool		IsTeamPlayAllowed(void);

	bool		IsSpectatorAllowed(void);
	int			GetSpectatorIndex(void);

	bool		IsVoiceAllowed(void);
	int			GetVoiceOffset(void);
	bool		IsSprayHookAllowed(void);
	int			GetSprayHookOffset(void);
	bool		IsSpawnPointHookAllowed(void);
	int			GetSpawnPointHookOffset(void);

	bool		IsSetColourAllowed(void);
	int			GetAlphaRenderMode(void);

	bool		IsAMXMenuAllowed(void);

	bool		IsSlapAllowed(void);
	bool		IsTeleportAllowed(void);
	bool		IsFireAllowed(void);
	bool		IsDrugAllowed(void);
	bool		IsDeathBeamAllowed(void);

	// Dystopia requires this
	bool		IsBrowseAllowed(void);

	bool		IsAdvertDecalAllowed(void);

	int			GetMaxMessages(void);
//	bool		IsClientSuicide(void);
	bool		IsKillsAllowed(void);
	int			GetKillsOffset(void);

	bool		IsDeathsAllowed(void);
	int			GetDeathsOffset(void);
	int			DebugOn() { return debug_log; }

	bool		IsGravityAllowed(void);
	int			GetGravityOffset(void);
	
	int			GetIndexFromGroup(const char *group_id);
	bool		IsValidActiveTeam(int	index);
	char		*GetTeamSpawnPointClassName(int index);
	char		*GetTeamLogName(int index);
	int			GetTeamShortTranslation(int index);
	int			GetTeamTranslation(int index);
	int			GetOpposingTeam(int index);

	const char *GetAdminSkinDir(int index);
	const char *GetReservedSkinDir(int index);
	const char *GetPublicSkinDir(int index);
	int			GetVFuncIndex(int index);
	int         GetPropIndex(int  index);
	bool		CanUseProp(int index);

private:
	void		GetProps(KeyValues *kv_ptr);
	void		GetVFuncs(KeyValues *kv_ptr);
	void		DefaultValues(void);
	bool		FindBaseKey(KeyValues *kv, KeyValues *base_key_ptr);

	struct team_class_t
	{
		int		team_index;
		char	spawnpoint_class_name[128];
		int		team_translation_index;
		int		team_short_translation_index;
		char	group[32];
		char	admin_skin_dir[64];
		char	reserved_skin_dir[64];
		char	public_skin_dir[64];
		char	log_name[64];
	};

	team_class_t	team_class_list[10];

	char		game_type[256];
	int			game_type_index;
	
	int			advanced_effects;
	int			advanced_effects_vfunc_offset;
	int			advanced_effects_code_offset;
	char		linux_game_bin[256];

	int			hl1_menu_compatible;

	int			team_play;
	int			spectator_index;
	int			spectator_allowed;
	char		spectator_group[32];

	int			kills_allowed;
	int			kills_offset;

	int			deaths_allowed;
	int			deaths_offset;

	int			gravity_allowed;
	int			gravity_offset;

	int			voice_allowed;
	int			voice_offset;

	int			spray_hook_allowed;
	int			spray_hook_offset;

	int			spawn_point_allowed;
	int			spawn_point_offset;

	int			max_messages;
	int			set_colour;
	int			alpha_render_mode;

	int			debug_log;

	// Dod requires this
	int			slap_allowed;
	int			teleport_allowed;
	int			drug_allowed;
	int			fire_allowed;
	int			death_beam_allowed;
	int			advert_decal_allowed;
//	int			client_suicide;
	// Dystopia requires this
	int			browse_allowed;

	int			vfunc_index[200];
	int			prop_index[200];
};

extern	ManiGameType *gpManiGameType;

#endif
