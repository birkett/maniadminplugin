//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_CUSTOMEFFECTS_H
#define MANI_CUSTOMEFFECTS_H

#define MANI_C_BEAM					(1)
#define MANI_C_SMOKE				(2)
#define MANI_C_SPARKS				(3)
#define MANI_C_TE_BEAM_ENT_POINT	(4)
#define MANI_C_TE_BEAM_ENTS			(5)
#define MANI_C_TE_BEAM_FOLLOW		(6)
#define MANI_C_TE_BEAM_POINTS		(7)
#define MANI_C_TE_BEAM_LASER		(8)
#define MANI_C_TE_BEAM_RING			(9)
#define MANI_C_TE_BEAM_RING_POINT	(10)
#define MANI_C_TE_DYNAMIC_LIGHT		(11)
#define MANI_C_TE_EXPLOSION			(12)
#define MANI_C_TE_GLOW_SPRITE		(13)
#define MANI_C_TE_FUNNEL			(14)
#define MANI_C_TE_METAL_SPARKS		(15)
#define MANI_C_TE_SHOW_LINE			(16)
#define MANI_C_TE_SMOKE				(17)
#define MANI_C_TE_SPARKS			(18)
#define MANI_C_TE_SPRITE			(19)
#define MANI_C_TE_BREAK_MODEL		(20)
#define MANI_C_TE_PHYSICS_PROP		(21)
#define MANI_C_TE_PROJECT_DECAL		(22)
#define MANI_C_TE_BSP_DECAL			(23)
#define MANI_C_TE_WORLD_DECAL		(24)

#define MANI_C_FILTER_ALL			('A')
#define MANI_C_FILTER_TEAM_A		('T') // Terrorist in CSS
#define MANI_C_FILTER_TEAM_B		('C') // Counter Terrorist in CSS
#define MANI_C_FILTER_SPEC			('S')
#define MANI_C_FILTER_DEAD			('D')
#define MANI_C_FILTER_EXDEAD		('E')


struct	texture_info_t
{
	char	filename[256];
	char	texture_name[64];
	int		texture_index;
};

class ManiCustomEffects
{

public:
	ManiCustomEffects();
	~ManiCustomEffects();

	void		Init(void); // Run at level init and startup
	PLUGIN_RESULT	ProcessMaEffect(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	int			GetDecal(char *name);

private:
	void	Beam(void);
	void	Smoke(void);
	void	Sparks(void);
	void	TEBeamEntPoint(void);
	void	TEBeamEnts(void);
	void	TEBeamFollow(void);
	void	TEBeamPoints(void);
	void	TEBeamLaser(void);
	void	TEBeamRing (void);
	void	TEBeamRingPoint (void);
	void	TEDynamicLight (void);
	void	TEExplosion (void);
	void	TEGlowSprite (void);
	void	TEFunnel (void);
	void	TEMetalSparks (void);
	void	TEShowLine (void);
	void	TESmoke (void);
	void	TESparks (void);
	void	TESprite (void);
	void	TEBreakModel (void);
	void	TEPhysicsProp (void);
	void	TEProjectDecal (void);
	void	TEBSPDecal (void);
	void	TEWorldDecal (void);

	bool	EffectAllowed (void);
	void	GetXYZ (Vector *vec);
	void	GetAngles (QAngle *angle);
    inline	float	GetFloat (void);
	inline	int		GetInt (void);
	inline	unsigned char	GetChar(void);

	int		GetModel(void);
	int		GetDecal(void);
	bool	EnoughParams(int	min_params);
	void	SetupFilter (MRecipientFilter *filter);
	bool	PlayerByUserID(player_t *player_ptr);
	bool	PlayerByIndex(player_t *player_ptr);
	void	AddToDownloads(char *filename);

	texture_info_t	*texture_list;
	int				texture_list_size;

	texture_info_t	*decal_list;
	int				decal_list_size;

	int				argv_index;
	int				argc;
	int				arg_type;
	const char		*command_string;

};

extern	ManiCustomEffects *gpManiCustomEffects;

#endif
