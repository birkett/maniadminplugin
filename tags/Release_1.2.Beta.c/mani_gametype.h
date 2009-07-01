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


class ManiGameType
{

public:
	ManiGameType();
	~ManiGameType();

	void		Init(void);
	const char	*GetGameType(void);
	bool		IsGameType(const char *game_str);
	bool		IsGameType(int game_index);
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

	bool		IsSetColourAllowed(void);
	int			GetAlphaRenderMode(void);

	bool		IsCashAllowed(void);
	int			GetCashOffset(void);
	bool		IsAMXMenuAllowed(void);

	// Dod requires next 5
	bool		IsSlapAllowed(void);
	bool		IsTeleportAllowed(void);
	bool		IsFireAllowed(void);
	bool		IsDrugAllowed(void);
	bool		IsDeathBeamAllowed(void);

	bool		IsAdvertDecalAllowed(void);

	int			GetMaxMessages(void);
//	bool		IsClientSuicide(void);
	bool		IsKillsAllowed(void);
	int			GetKillsOffset(void);
	
	int			GetIndexFromGroup(const char *group_id);
	bool		IsValidActiveTeam(int	index);
	int			GetTeamShortTranslation(int index);
	int			GetTeamTranslation(int index);
	int			GetOpposingTeam(int index);

	const char *GetAdminSkinDir(int index);
	const char *GetReservedSkinDir(int index);
	const char *GetPublicSkinDir(int index);


private:

	void		DefaultValues(void);
	bool		FindBaseKey(KeyValues *kv, KeyValues *base_key_ptr);

	struct team_class_t
	{
		int		team_index;
		int		team_translation_index;
		int		team_short_translation_index;
		char	group[32];
		char	admin_skin_dir[64];
		char	reserved_skin_dir[64];
		char	public_skin_dir[64];
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

	int			cash_allowed;
	int			cash_offset;

	int			kills_allowed;
	int			kills_offset;

	int			voice_allowed;
	int			voice_offset;

	int			max_messages;
	int			set_colour;
	int			alpha_render_mode;

	// Dod requires this
	int			slap_allowed;
	int			teleport_allowed;
	int			drug_allowed;
	int			fire_allowed;
	int			death_beam_allowed;
	int			advert_decal_allowed;
//	int			client_suicide;

};

extern	ManiGameType *gpManiGameType;

#endif
