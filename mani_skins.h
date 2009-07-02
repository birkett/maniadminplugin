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



#ifndef MANI_SKINS_H
#define MANI_SKINS_H

#define MANI_ADMIN_T_SKIN (0)
#define MANI_ADMIN_CT_SKIN (1)
#define MANI_T_SKIN (2)
#define MANI_CT_SKIN (3)
#define MANI_MISC_SKIN (4)
#define MANI_RESERVE_T_SKIN (5)
#define MANI_RESERVE_CT_SKIN (6)

struct	skin_resource_t
{
	char	resource[256];
};

struct	skin_t
{
	int		skin_type;
	char	skin_name[20];
	char	precache_name[256];
	int		model_index;
	skin_resource_t	*resource_list;
	int		resource_list_size;
};

struct	current_skin_t
{
	int		team_id;
};

struct	action_model_sound_t
{
	char	sound_file[256];
	float	sound_length;
	int		play_once;
	int		primary_sound;
	int		sequence;
	int		auto_download;
};

struct	action_model_weapon_t
{
	int		weapon_index;
};

struct	action_model_t
{
	char	skin_name[20];
	int		gravity;
	int		random_sound;
	int		remove_all_weapons;
	int		allow_weapon_pickup;
	int		red;
	int		green;
	int		blue;
	int		alpha;
	float	intermission_min;
	float	intermission_max;
	int		sound_list_size;
	action_model_sound_t	*sound_list;
	int		weapon_list_size;
	action_model_weapon_t	*weapon_list;
};


extern  current_skin_t	current_skin_list[];
extern	void	LoadSkins(void);
extern	void	FreeSkins(void);
extern	void	FreeActionModels(void);
extern	skin_t	*skin_list;
extern	int		skin_list_size;
extern	void SkinTeamJoin( player_t	*player_ptr);
extern	void ForceSkinType( player_t	*player_ptr );
extern	PLUGIN_RESULT	ProcessMaSetSkin(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
extern	void ProcessMenuSetSkin (player_t *admin, int next_index, int argv_offset );
extern	void ProcessMenuSkinOptions (player_t *admin, int next_index, int argv_offset );
extern  void SkinPlayerDisconnect (player_t	*player_ptr);
extern	void SkinResetTeamID(void);
extern	void ProcessJoinSkinChoiceMenu(   player_t *player_ptr,   int next_index,   int argv_offset,   char *menu_command );


#endif
