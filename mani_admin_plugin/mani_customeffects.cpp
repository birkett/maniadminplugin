//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_sounds.h"
#include "mani_maps.h"
#include "mani_gametype.h"
#include "mani_customeffects.h"
#include "mani_downloads.h"
#include "mani_commands.h"
#include "mani_trackuser.h"
#include "cbaseentity.h"
#include "beam_flags.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;
extern	IEffects *effects;
extern	IEngineSound *esounds;
extern	IServerPluginHelpers *helpers;
extern	ITempEntsSystem *temp_ents;
extern	IUniformRandomStream *randomStr;
extern	IPlayerInfoManager *playerinfomanager;

extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;
extern	bf_write *msg_buffer;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

static	int sort_texture_list_by_name ( const void *m1,  const void *m2);

ManiCustomEffects::ManiCustomEffects()
{
	// Init
	gpManiCustomEffects = this;
	texture_list = NULL;
	texture_list_size = 0;
	decal_list = NULL;
	decal_list_size = 0;
}

ManiCustomEffects::~ManiCustomEffects()
{
	// Clean up 
	FreeList((void **) &texture_list, &texture_list_size);
	FreeList((void **) &decal_list, &decal_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Read in texure list and pre-cache them
//---------------------------------------------------------------------------------
void ManiCustomEffects::Init(void)
{
	char	core_filename[256];

	// Get our textures we want to pre-cache
	FreeList((void **) &texture_list, &texture_list_size);
	FreeList((void **) &decal_list, &decal_list_size);

	// Read the decallist.txt file
//	MMsg("*********** Loading decallist.txt ************\n");

	KeyValues *kv_ptr = new KeyValues("decallist.txt");

	for (;;)
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/decallist.txt", mani_path.GetString());
		if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
		{
//			MMsg("Failed to load decallist.txt\n");
			kv_ptr->deleteThis();
			break;
		}

		KeyValues *base_key_ptr;

		base_key_ptr = kv_ptr->GetFirstSubKey();
		if (!base_key_ptr)
		{
//			MMsg("Nothing found\n");
			kv_ptr->deleteThis();
			break;
		}

		texture_info_t texture_info;

		for (;;)
		{
			if (FStrEq(base_key_ptr->GetName(), "downloads"))
			{
				gpManiDownloads->AddDownloadsKeyValues(base_key_ptr);
			}
			else
			{
				Q_strcpy(texture_info.texture_name, base_key_ptr->GetName());
				Q_strcpy(texture_info.filename, base_key_ptr->GetString());

				if (!FStrEq(texture_info.filename,""))
				{
					texture_info.texture_index = engine->PrecacheDecal(texture_info.filename);
					AddToList((void **) &decal_list, sizeof(texture_info_t), &decal_list_size);
					decal_list[decal_list_size - 1] = texture_info;
//					MMsg("Added [%s => %s] using decal index %i\n", texture_info.texture_name, texture_info.filename, texture_info.texture_index);
				}
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				break;
			}
		}

		kv_ptr->deleteThis();

		qsort(decal_list, decal_list_size, sizeof(texture_info_t), sort_texture_list_by_name); 
//		MMsg("*********** decallist.txt loaded ************\n");
		break;
	}

	// Read the texturelist.txt file
//	MMsg("*********** Loading texturelist.txt ************\n");

	kv_ptr = new KeyValues("texturelist.txt");

	for (;;)
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/texturelist.txt", mani_path.GetString());
		if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
		{
//			MMsg("Failed to load texturelist.txt\n");
			kv_ptr->deleteThis();
			break;
		}

		KeyValues *base_key_ptr;

		base_key_ptr = kv_ptr->GetFirstSubKey();
		if (!base_key_ptr)
		{
//			MMsg("Nothing found\n");
			kv_ptr->deleteThis();
			break;
		}

		texture_info_t texture_info;

		for (;;)
		{
			if (FStrEq(base_key_ptr->GetName(), "downloads"))
			{
				gpManiDownloads->AddDownloadsKeyValues(base_key_ptr);
			}
			else
			{
				Q_strcpy(texture_info.texture_name, base_key_ptr->GetName());
				Q_strcpy(texture_info.filename, base_key_ptr->GetString());

				if (!FStrEq(texture_info.filename,""))
				{
					texture_info.texture_index = engine->PrecacheModel(texture_info.filename, true);
					AddToList((void **) &texture_list, sizeof(texture_info_t), &texture_list_size);
					texture_list[texture_list_size - 1] = texture_info;
//					MMsg("Added [%s => %s] using model index %i\n", texture_info.texture_name, texture_info.filename, texture_info.texture_index);
				}
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				break;
			}
		}

		kv_ptr->deleteThis();

		qsort(texture_list, texture_list_size, sizeof(texture_info_t), sort_texture_list_by_name); 
//		MMsg("*********** texturelist.txt loaded ************\n");
		break;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process incomming command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiCustomEffects::ProcessMaEffect(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	// Come from server console
	argc = gpCmd->Cmd_Argc();
	command_string = gpCmd->Cmd_Argv(0);

	if (!effects)
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s, unable to use effects system\n", command_string);
		return PLUGIN_STOP;
	}

	if (argc < 3)
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s, not enough arguments\n", command_string);
		return PLUGIN_STOP;
	}

	arg_type = atoi(gpCmd->Cmd_Argv(1));
	argv_index = 2;

	switch (arg_type)
	{
		case MANI_C_BEAM : Beam(); break;
		case MANI_C_SMOKE : Smoke(); break;
		case MANI_C_SPARKS : Sparks(); break;
		case MANI_C_TE_BEAM_ENT_POINT : TEBeamEntPoint(); break;
		case MANI_C_TE_BEAM_ENTS : TEBeamEnts(); break;
		case MANI_C_TE_BEAM_FOLLOW : TEBeamFollow(); break;
		case MANI_C_TE_BEAM_POINTS : TEBeamPoints(); break;
		case MANI_C_TE_BEAM_LASER : TEBeamLaser(); break;
		case MANI_C_TE_BEAM_RING : TEBeamRing (); break;
		case MANI_C_TE_BEAM_RING_POINT : TEBeamRingPoint (); break;
		case MANI_C_TE_DYNAMIC_LIGHT : TEDynamicLight (); break;
		case MANI_C_TE_EXPLOSION : TEExplosion (); break;
		case MANI_C_TE_GLOW_SPRITE : TEGlowSprite (); break;
		case MANI_C_TE_FUNNEL : TEFunnel (); break;
		case MANI_C_TE_METAL_SPARKS : TEMetalSparks (); break;
		case MANI_C_TE_SHOW_LINE : TEShowLine (); break;
		case MANI_C_TE_SMOKE : TESmoke (); break;
		case MANI_C_TE_SPARKS : TESparks (); break;
		case MANI_C_TE_SPRITE : TESprite (); break;
		case MANI_C_TE_BREAK_MODEL : TEBreakModel(); break;
		case MANI_C_TE_PHYSICS_PROP : TEPhysicsProp(); break;
		case MANI_C_TE_PROJECT_DECAL : TEProjectDecal(); break;
		case MANI_C_TE_BSP_DECAL : TEBSPDecal(); break;
		case MANI_C_TE_WORLD_DECAL : TEWorldDecal(); break;
		default : OutputToConsole(NULL, "Mani Admin Plugin: %s, unknown effect type [%i]\n", command_string, arg_type);
					break;
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect (All players see these)
//---------------------------------------------------------------------------------
void	ManiCustomEffects::Beam(void)
{
//	virtual void Beam( 
	Vector Start; 
	Vector End; 
	int nModelIndex; 
	int nHaloIndex;  
	unsigned char frameStart; 
	unsigned char frameRate;
	float flLife;
	unsigned char width;
	unsigned char endWidth;
	unsigned char fadeLength;
	unsigned char noise; 
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char brightness;
	unsigned char speed;

	// We are looking at 22 parameters for this !
	if (!EnoughParams(22)) return;

	GetXYZ(&Start);
	GetXYZ(&End);
	nModelIndex = GetModel();
	nHaloIndex = GetModel();
	frameStart = GetChar();
	frameRate = GetChar();
	flLife = GetFloat();
	width = GetChar();
	endWidth = GetChar();
	fadeLength = GetChar();
	noise = GetChar();
	red = GetChar();
	green = GetChar();
	blue = GetChar();
	brightness = GetChar();
	speed = GetChar();

	effects->Beam(Start, End, nModelIndex, nHaloIndex, frameStart, frameRate, flLife, width, endWidth, 
					fadeLength, noise, red, green, blue, brightness, speed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::Smoke(void)
{
//	virtual void Smoke( 
	Vector origin;
	int modelIndex; 
	float scale;
	float framerate;

	// We are looking at 8 parameters for this !
	if (!EnoughParams(8)) return;

	GetXYZ(&origin);
	modelIndex = GetModel();
	scale = GetFloat();
	framerate = GetFloat();

	effects->Smoke(origin, modelIndex, scale, framerate);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::Sparks(void)
{
	// virtual void Sparks( 
	Vector position;
	int nMagnitude = 1;
	int nTrailLength = 1;
//	const Vector *pvecDir = NULL ) = 0;

	// We are looking at 7 parameters for this !
	if (!EnoughParams(7)) return;

	GetXYZ(&position);
	nMagnitude = GetInt();
	nTrailLength = GetInt();

	effects->Sparks(position, nMagnitude, nTrailLength, NULL);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamEntPoint(void)
{
	// void TE_BeamEntPoint( 
	MRecipientFilter filter;
	float delay;
	int	nStartEntity;
	Vector start;
	int nEndEntity;
	Vector end;
	int modelindex;
	int haloindex;
	int startframe;
	int framerate;
	float life;
	float width;
	float endWidth;
	int fadeLength;
	float amplitude; 
	int r; 
	int g;
	int b;
	int a;
	int speed;

	if (!EffectAllowed()) return;
	// We are looking at 26 parameters for this !
	if (!EnoughParams(26)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	nStartEntity = GetInt();
	GetXYZ(&start);
	nEndEntity = GetInt();
	GetXYZ(&end);
	modelindex = GetModel();
	haloindex = GetModel();
	startframe = GetInt();
	framerate = GetInt();
	life = GetFloat();
	width = GetFloat();
	endWidth = GetFloat();
	fadeLength = GetInt();
	amplitude = GetFloat(); 
	r = GetInt(); 
	g = GetInt();
	b = GetInt(); 
	a = GetInt();
	speed = GetInt();

	temp_ents->BeamEntPoint((IRecipientFilter &) filter, delay, nStartEntity, &start,	nEndEntity, &end, modelindex, haloindex, 
							startframe, framerate, life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamEnts(void)
{
	// void TE_BeamEnts( 
	MRecipientFilter filter; 
	float delay;
	int	start; 
	int end; 
	int modelindex; 
	int haloindex; 
	int startframe; 
	int framerate;
	float life; 
	float width; 
	float endWidth; 
	int fadeLength; 
	float amplitude; 
	int r; 
	int g; 
	int b; 
	int a;
	int speed;

	if (!EffectAllowed()) return;
	// We are looking at 20 parameters for this !
	if (!EnoughParams(20)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	start = GetInt(); 
	end = GetInt();  
	modelindex = GetModel();  
	haloindex = GetModel();  
	startframe = GetInt();  
	framerate = GetInt(); 
	life = GetFloat();
	width = GetFloat();
	endWidth = GetFloat();
	fadeLength = GetInt();  
	amplitude = GetFloat();
	r = GetInt();  
	g = GetInt();  
	b = GetInt();  
	a = GetInt(); 
	speed = GetInt(); 

	temp_ents->BeamEnts((IRecipientFilter &) filter, delay, start, end, modelindex, haloindex, 
						startframe, framerate, life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamFollow(void)
{
	// void TE_BeamFollow( 
	
	MRecipientFilter filter;
	float delay;
	int iEntIndex;
	int modelIndex; 
	int haloIndex;
	float life;
	float width;
	float endWidth;
	float fadeLength;
	float r; 
	float g; 
	float b; 
	float a;

	if (!EffectAllowed()) return;
	// We are looking at 15 parameters for this !
	if (!EnoughParams(15)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	iEntIndex = GetInt();
	modelIndex = GetModel(); 
	haloIndex = GetModel(); 
	life = GetFloat();
	width = GetFloat();
	endWidth = GetFloat();
	fadeLength = GetFloat();
	r = GetFloat();
	g = GetFloat();
	b = GetFloat();
	a = GetFloat();

	temp_ents->BeamFollow((IRecipientFilter &) filter, delay, iEntIndex, modelIndex, haloIndex, life, width, endWidth, fadeLength, r, g, b, a);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamPoints(void)
{
	// void TE_BeamPoints( 
	MRecipientFilter filter;
	float delay;
	Vector start;
	Vector end;
	int modelindex;
	int haloindex;
	int startframe;
	int framerate;
	float life;
	float width; 
	float endWidth;
	int fadeLength; 
	float amplitude;
	int r;
	int g; 
	int b; 
	int a; 
	int speed;

	if (!EffectAllowed()) return;
	// We are looking at 22 parameters for this !
	if (!EnoughParams(22)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&start); 
	GetXYZ(&end); 
	modelindex = GetModel(); 
	haloindex = GetModel();  
	startframe = GetInt();
	framerate = GetInt();
	life = GetFloat();
	width = GetFloat();
	endWidth = GetFloat(); 
	fadeLength = GetInt(); 
	amplitude = GetFloat(); 
	r = GetInt();
	g = GetInt(); 
	b = GetInt(); 
	a = GetInt(); 
	speed = GetInt();

	temp_ents->BeamPoints((IRecipientFilter &) filter, delay, &start, &end, modelindex, haloindex, startframe, framerate, 
								life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamLaser(void)
{
	// void TE_BeamLaser( 
	MRecipientFilter filter;
	float delay;
	int	start;
	int end;
	int modelindex;
	int haloindex;
	int startframe;
	int framerate;
	float life;
	float width;
	float endWidth; 
	int fadeLength;
	float amplitude;
	int r; 
	int g; 
	int b; 
	int a;
	int speed;

	if (!EffectAllowed()) return;
	// We are looking at 20 parameters for this !
	if (!EnoughParams(20)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	start = GetInt();
	end = GetInt();
	modelindex = GetModel();
	haloindex = GetModel();
	startframe = GetInt();
	framerate = GetInt();
	life = GetFloat();
	width = GetFloat();
	endWidth = GetFloat(); 
	fadeLength = GetInt();
	amplitude = GetFloat();
	r = GetInt(); 
	g = GetInt(); 
	b = GetInt(); 
	a = GetInt();
	speed = GetInt();

	temp_ents->BeamLaser((IRecipientFilter &) filter, delay, start, end, modelindex, haloindex, startframe, framerate, life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamRing (void)
{
	// void TE_BeamRing( 
	MRecipientFilter filter;
	float delay;
	int	start; 
	int end;
	int modelindex; 
	int haloindex;
	int startframe; 
	int framerate;
	float life;
	float width; 
	int spread;
	float amplitude;
	int r; 
	int g; 
	int b; 
	int a;
	int speed;
	int flags; // default = 0

	if (!EffectAllowed()) return;
	// We are looking at 20 parameters for this !
	if (!EnoughParams(20)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	start = GetInt(); 
	end = GetInt();
	modelindex = GetModel();
	haloindex = GetModel();
	startframe = GetInt(); 
	framerate = GetInt();
	life = GetFloat();
	width = GetFloat();
	spread = GetInt();
	amplitude = GetFloat();
	r = GetInt(); 
	g = GetInt(); 
	b = GetInt(); 
	a = GetInt();
	speed = GetInt();
	flags = GetInt(); // default = 0

	temp_ents->BeamRing((IRecipientFilter &) filter, delay, start, end, modelindex, haloindex, startframe, framerate, life, width, spread, amplitude, r, g, b, a, speed, flags);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBeamRingPoint (void)
{
	// void TE_BeamRingPoint( 
	MRecipientFilter filter;
	float delay;
	Vector center;
	float start_radius; 
	float end_radius; 
	int modelindex; 
	int haloindex;
	int startframe;
	int framerate;
	float life;
	float width; 
	int spread;
	float amplitude;
	int r; 
	int g; 
	int b; 
	int a;
	int speed; 
	int flags;

	if (!EffectAllowed()) return;
	// We are looking at 23 parameters for this !
	if (!EnoughParams(23)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&center);
	start_radius = GetFloat(); 
	end_radius = GetFloat(); 
	modelindex = GetModel();
	haloindex = GetModel();
	startframe = GetInt();
	framerate = GetInt();
	life = GetFloat();
	width = GetFloat(); 
	spread = GetInt();
	amplitude = GetFloat();
	r = GetInt(); 
	g = GetInt(); 
	b = GetInt(); 
	a = GetInt();
	speed = GetInt(); 
	flags = GetInt();

	temp_ents->BeamRingPoint((IRecipientFilter &) filter, delay, center, start_radius, end_radius, modelindex, haloindex, 
						startframe, framerate, life, width, spread, amplitude, r, g, b, a, speed, flags);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEDynamicLight (void)
{
	// void TE_DynamicLight( 
	MRecipientFilter filter;
	float delay;
	Vector org;
	int r; 
	int g; 
	int b;
	int exponent; 
	float radius;
	float time;
	float decay;

	if (!EffectAllowed()) return;
	// We are looking at 14 parameters for this !
	if (!EnoughParams(14)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&org);
	r = GetInt(); 
	g = GetInt();  
	b = GetInt(); 
	exponent = GetInt();  
	radius = GetFloat();
	time = GetFloat();
	decay = GetFloat();

	temp_ents->DynamicLight((IRecipientFilter &) filter, delay, &org, r, g, b, exponent, radius, time, decay);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEExplosion (void)
{
	// void TE_Explosion( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int modelindex;
	float scale;
	int framerate;
	int flags;
	int radius; 
	int magnitude;

	if (!EffectAllowed()) return;
	// We are looking at 13 parameters for this !
	if (!EnoughParams(13)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	modelindex = GetModel();
	scale = GetFloat();
	framerate = GetInt(); 
	flags = GetInt();  
	radius = GetInt();  
	magnitude = GetInt(); 

	temp_ents->Explosion((IRecipientFilter &) filter, delay, &pos, modelindex, scale, framerate, flags, radius, magnitude);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEGlowSprite (void)
{
	// void TE_GlowSprite( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int modelindex;
	float life; 
	float size;
	int brightness;

	if (!EffectAllowed()) return;
	// We are looking at 11 parameters for this !
	if (!EnoughParams(11)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	modelindex = GetModel();
	life = GetFloat();
	size = GetFloat();
	brightness = GetInt();

	temp_ents->GlowSprite((IRecipientFilter &) filter, delay, &pos, modelindex, life, size, brightness);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEFunnel (void)
{
//	void TE_LargeFunnel( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int modelindex;
	int reversed;

	if (!EffectAllowed()) return;
	// We are looking at 9 parameters for this !
	if (!EnoughParams(9)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	modelindex = GetModel();
	reversed = GetInt();

	temp_ents->LargeFunnel((IRecipientFilter &) filter, delay, &pos, modelindex, reversed);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEMetalSparks (void)
{
	// void TE_MetalSparks( 
	MRecipientFilter filter; 
	float delay;
	Vector pos;
	Vector dir;

	if (!EffectAllowed()) return;
	// We are looking at 7 parameters for this !
	if (!EnoughParams(7)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	dir.z = 0.5;
	dir.x = 0.01;
	dir.y = 0.01;

	temp_ents->MetalSparks((IRecipientFilter &) filter, delay, &pos, &dir);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEShowLine (void)
{
	// void TE_ShowLine( 
	
	MRecipientFilter filter;
	float delay;
	Vector start;
	Vector end;

	if (!EffectAllowed()) return;
	// We are looking at 10 parameters for this !
	if (!EnoughParams(10)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&start);
	GetXYZ(&end);

	temp_ents->ShowLine((IRecipientFilter &) filter, delay, &start, &end);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TESmoke (void)
{
	// void TE_Smoke( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int modelindex;
	float scale;
	int framerate;

	if (!EffectAllowed()) return;
	// We are looking at 10 parameters for this !
	if (!EnoughParams(10)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	modelindex = GetModel();
	scale = GetFloat();
	framerate = GetInt();

	temp_ents->Smoke((IRecipientFilter &) filter, delay, &pos, modelindex, scale, framerate);  
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TESparks (void)
{
	// void TE_Sparks( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int nMagnitude;
	int nTrailLength;
//	Vector *pDir );

	if (!EffectAllowed()) return;
	// We are looking at 9 parameters for this !
	if (!EnoughParams(9)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	nMagnitude = GetInt();
	nTrailLength = GetInt();

	temp_ents->Sparks((IRecipientFilter &) filter, delay, &pos, nMagnitude, nTrailLength, NULL);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TESprite (void)
{
	// void TE_Sprite( 
	
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int modelindex;
	float size;
	int brightness;

	if (!EffectAllowed()) return;
	// We are looking at 10 parameters for this !
	if (!EnoughParams(10)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	modelindex = GetModel();
	size = GetFloat();
	brightness = GetInt();

	temp_ents->Sprite((IRecipientFilter &) filter, delay, &pos, modelindex, size, brightness);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBreakModel (void)
{
//	void TE_BreakModel( 

	// ma_rcon ma_effect 20 A 0 117.67635 -1136.33838 101.41106 0 0 0 16 16 16 0 0 10 spine 100 100 15 0

	MRecipientFilter filter;
	float delay;
	Vector pos; 
	QAngle angles; 
	Vector size; 
	Vector vel;
	int modelindex;
	int randomization;
	int count;
	float time; 
	int flags;

	if (!EffectAllowed()) return;
	// We are looking at 21 parameters for this !
	if (!EnoughParams(21)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	GetAngles(&angles); // Rotation
	GetXYZ(&size);
	GetXYZ(&vel);

	modelindex = GetModel();
	randomization = GetInt(); 
	count = GetInt();  
	time = GetFloat();  
	flags = GetInt(); 

	temp_ents->BreakModel((IRecipientFilter &) filter, delay, pos, angles, size, vel, modelindex, randomization, count, time, flags);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEPhysicsProp (void)
{
//	void TE_PhysicsProp( 

	// ma_rcon ma_rcon ma_effect 21 A 0 skull1 117.67635 -1136.33838 101.41106 0 0 0 0 0 10 0

	MRecipientFilter filter; 
	float delay;
	int modelindex; 
	Vector pos; 
	QAngle angles; 
	Vector vel; 
	int flags;
	int skin;
	int effects;

	if (!EffectAllowed()) return;
	// We are looking at 17 parameters for this !
	if (!EnoughParams(17)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	modelindex = GetModel();
	skin = GetInt();
	GetXYZ(&pos);
	GetAngles(&angles); // Rotation
	GetXYZ(&vel); 
	flags = GetInt(); 
	effects = GetInt();

/*	virtual void PhysicsProp( IRecipientFilter& filter, float delay, int modelindex, int skin, 
		const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects ) = 0; */

	temp_ents->PhysicsProp((IRecipientFilter &) filter, delay, modelindex, skin, pos, angles, vel, flags, effects);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEProjectDecal (void)
{
//	void TE_ProjectDecal( 
	MRecipientFilter filter; 
	float delay;
	Vector pos; 
	QAngle angles; 
	float distance; 
	int index;

	if (!EffectAllowed()) return;
	// We are looking at 12 parameters for this !
	if (!EnoughParams(12)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	GetAngles(&angles);
	distance = GetFloat(); 
	index = GetDecal();

	temp_ents->ProjectDecal((IRecipientFilter &) filter, delay, &pos, &angles, distance, index);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEBSPDecal (void)
{
//	void TE_BSPDecal( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int entity;
	int index;

	if (!EffectAllowed()) return;
	// We are looking at 9 parameters for this !
	if (!EnoughParams(9)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	entity = GetInt(); 
	index = GetDecal();

	temp_ents->BSPDecal((IRecipientFilter &) filter, delay, &pos, entity, index);
}

//---------------------------------------------------------------------------------
// Purpose: Process custom effect
//---------------------------------------------------------------------------------
void	ManiCustomEffects::TEWorldDecal (void)
{
//	void TE_WorldDecal( 
	MRecipientFilter filter;
	float delay;
	Vector pos;
	int index;

	if (!EffectAllowed()) return;
	// We are looking at 8 parameters for this !
	if (!EnoughParams(8)) return;

	SetupFilter(&filter);
	delay = GetFloat();
	GetXYZ(&pos);
	index = GetDecal();

	temp_ents->WorldDecal((IRecipientFilter &) filter, delay, &pos, index);
}

//---------------------------------------------------------------------------------
// Purpose: Check if Advanced effects are allowed
//---------------------------------------------------------------------------------
bool	ManiCustomEffects::EffectAllowed (void)
{
	if (!gpManiGameType->GetAdvancedEffectsAllowed())
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s, Advanced effect [%i] not allowed on this system\n", command_string, arg_type);		
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Setup Vector based on argv_index
//---------------------------------------------------------------------------------
void	ManiCustomEffects::GetXYZ (Vector *vec)
{
	vec->x = atof(gpCmd->Cmd_Argv(argv_index++));
	vec->y = atof(gpCmd->Cmd_Argv(argv_index++));
	vec->z = atof(gpCmd->Cmd_Argv(argv_index++));
}

//---------------------------------------------------------------------------------
// Purpose: Setup Angles based on argv_index
//---------------------------------------------------------------------------------
void	ManiCustomEffects::GetAngles (QAngle *angle)
{
	angle->x = atof(gpCmd->Cmd_Argv(argv_index++));
	angle->y = atof(gpCmd->Cmd_Argv(argv_index++));
	angle->z = atof(gpCmd->Cmd_Argv(argv_index++));
}

//---------------------------------------------------------------------------------
// Purpose: Setup float based on argv_index
//---------------------------------------------------------------------------------
float	ManiCustomEffects::GetFloat(void)
{
	return (atof(gpCmd->Cmd_Argv(argv_index++)));
}

//---------------------------------------------------------------------------------
// Purpose: Setup int based on argv_index
//---------------------------------------------------------------------------------
int	ManiCustomEffects::GetInt(void)
{
	return (atoi(gpCmd->Cmd_Argv(argv_index++)));
}

//---------------------------------------------------------------------------------
// Purpose: Setup Char based on argv_index
//---------------------------------------------------------------------------------
unsigned char	ManiCustomEffects::GetChar(void)
{
	return ((unsigned char) atoi(gpCmd->Cmd_Argv(argv_index++)));
}

//---------------------------------------------------------------------------------
// Purpose: Setup model index
//---------------------------------------------------------------------------------
int	ManiCustomEffects::GetModel(void)
{
	texture_info_t	texture_key;
	texture_info_t	*found_texture;

	const char	*texture_name = gpCmd->Cmd_Argv(argv_index ++);

	// Do BSearch for model
	Q_strcpy(texture_key.texture_name, texture_name);

	found_texture = (texture_info_t *) bsearch
						(
						&texture_key, 
						texture_list, 
						texture_list_size, 
						sizeof(texture_info_t), 
						sort_texture_list_by_name
						);

	if (!found_texture)
	{
		return 0;
	}

	return (found_texture->texture_index);
}

//---------------------------------------------------------------------------------
// Purpose: Setup model index
//---------------------------------------------------------------------------------
int	ManiCustomEffects::GetDecal(void)
{
	texture_info_t	texture_key;
	texture_info_t	*found_texture;

	const char	*texture_name = gpCmd->Cmd_Argv(argv_index ++);

	// Do BSearch for model
	Q_strcpy(texture_key.texture_name, texture_name);

	found_texture = (texture_info_t *) bsearch
						(
						&texture_key, 
						decal_list, 
						decal_list_size, 
						sizeof(texture_info_t), 
						sort_texture_list_by_name
						);

	if (!found_texture)
	{
		return -1;
	}
	
	return (found_texture->texture_index);
}

//---------------------------------------------------------------------------------
// Purpose: Setup model index by passed in name
//---------------------------------------------------------------------------------
int	ManiCustomEffects::GetDecal(char *name)
{
	texture_info_t	texture_key;
	texture_info_t	*found_texture;

	// Do BSearch for model
	Q_strcpy(texture_key.texture_name, name);

	found_texture = (texture_info_t *) bsearch
						(
						&texture_key, 
						decal_list, 
						decal_list_size, 
						sizeof(texture_info_t), 
						sort_texture_list_by_name
						);

	if (!found_texture)
	{
		return -1;
	}

	return (found_texture->texture_index);
}

//---------------------------------------------------------------------------------
// Purpose: Function to check parameters
//---------------------------------------------------------------------------------
bool	ManiCustomEffects::EnoughParams(int	min_params)
{
	if (gpCmd->Cmd_Argc() < min_params)
	{
		OutputToConsole(NULL, "Mani Admin Plugin: %s, not enough arguments, need %i minimum\n", command_string, min_params);
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Setup filter type
//---------------------------------------------------------------------------------
void	ManiCustomEffects::SetupFilter (MRecipientFilter *filter)
{
	bool	player_list[MANI_MAX_PLAYERS];

	const char	*filter_string = gpCmd->Cmd_Argv(argv_index ++);
	int		string_length;
	bool	add_all, add_team_a, add_team_b, add_spec, add_dead, exclude_dead;
	bool	spec_allowed = gpManiGameType->IsSpectatorAllowed();
	bool	team_play = gpManiGameType->IsTeamPlayAllowed();
	int		players_to_add = 0;

	add_all = add_team_a = add_team_b = add_spec = add_dead = exclude_dead = false;

	string_length = Q_strlen(filter_string);

	for (int i = 0; i < string_length; i++)
	{
		if (filter_string[i] == MANI_C_FILTER_ALL)
		{
			add_all = true;
		}
		else if (filter_string[i] == MANI_C_FILTER_TEAM_A)
		{
			add_team_a = true;
		}
		else if (filter_string[i] == MANI_C_FILTER_TEAM_B && team_play)
		{
			add_team_b = true;
		}
		else if (filter_string[i] == MANI_C_FILTER_SPEC && spec_allowed)
		{
			add_spec = true;
		}
		else if (filter_string[i] == MANI_C_FILTER_DEAD)
		{
			add_dead = true;
		}
		else if (filter_string[i] == MANI_C_FILTER_EXDEAD)
		{
			exclude_dead = true;
		}
		else if (filter_string[i] >= '0' &&  filter_string[i] <= '9')
		{
			// Handle number section
			players_to_add = atoi(&(filter_string[i]));
			break;
		}
		else
		{
			continue;
		}
	}

	for (int i = 0; i < max_players; i++)
	{
		player_t	player;
		
		player.index = i + 1;

		player_list[i] = false;
		if (!PlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		if (add_all) 
		{
			player_list[i] = true;
		}
		else
		{
			if (add_team_a)
			{
				if (team_play)
				{
					if (player.team == TEAM_A) player_list[i] = true;
				}
				else
				{
					if (player.team == 0) player_list[i] = true;
				}
			}

			if (add_team_b && player.team == TEAM_B)
			{
				player_list[i] = true;
			}

			if (add_spec && player.team == gpManiGameType->GetSpectatorIndex())
			{
				player_list[i] = true;
			}

			if (add_dead && player.is_dead)
			{
					player_list[i] = true;
			}
		}

		if (exclude_dead && player.is_dead)
		{
			player_list[i] = false;
		}
	}

	if (players_to_add)
	{
		// Some players to add to the list
		for (int i = 0; i < players_to_add; i++)
		{
			player_t	player;

			player.user_id = GetInt();
			if (!PlayerByUserID(&player)) continue;
			if (player.is_bot) continue;

			player_list[player.index - 1] = true;
		}
	}

	filter->MakeReliable();

	for (int i = 0; i < max_players; i++)
	{
		if (player_list[i])
		{
			filter->AddPlayer(i + 1);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Faster version of FindPlayerByUserID
//---------------------------------------------------------------------------------
bool ManiCustomEffects::PlayerByUserID(player_t *player_ptr)
{
	player_ptr->index = gpManiTrackUser->GetIndex(player_ptr->user_id);
	if (player_ptr->index == -1) return false;

	edict_t *pEntity = engine->PEntityOfEntIndex(player_ptr->index);
	if(pEntity && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			player_ptr->player_info = playerinfo;
			player_ptr->team = playerinfo->GetTeamIndex();
			player_ptr->is_dead = playerinfo->IsDead();
			player_ptr->entity = pEntity;
			Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());
			if (FStrEq(player_ptr->steam_id, "BOT"))
			{
				player_ptr->is_bot = true;
			}
			else
			{
				player_ptr->is_bot = false;
			}
			return true;

		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: PlayerByIndex - Faster version of FindPlayerByIndex
//---------------------------------------------------------------------------------
bool ManiCustomEffects::PlayerByIndex(player_t *player_ptr)
{

	edict_t *pEntity = engine->PEntityOfEntIndex(player_ptr->index);
	if(pEntity && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			player_ptr->player_info = playerinfo;
			player_ptr->team = playerinfo->GetTeamIndex();
			player_ptr->is_dead = playerinfo->IsDead();
			player_ptr->entity = pEntity;
			Q_strcpy(player_ptr->steam_id, playerinfo->GetNetworkIDString());
			if (FStrEq(player_ptr->steam_id, "BOT"))
			{
				player_ptr->is_bot = true;
			}
			else
			{
				player_ptr->is_bot = false;
			}

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Qsort function for texture list
//---------------------------------------------------------------------------------
static
int sort_texture_list_by_name ( const void *m1,  const void *m2) 
{
	struct texture_info_t *mi1 = (struct texture_info_t *) m1;
	struct texture_info_t *mi2 = (struct texture_info_t *) m2;
	return strcmp(mi1->texture_name, mi2->texture_name);
}

ManiCustomEffects	g_ManiCustomEffects;
ManiCustomEffects	*gpManiCustomEffects;

SCON_COMMAND(ma_effect, 2221, MaEffect, false);

