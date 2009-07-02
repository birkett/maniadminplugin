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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_trackuser.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiTrackUser::ManiTrackUser()
{
	// Setup hash table for weapon search speed improvment
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}

	gpManiTrackUser = this;
}

ManiTrackUser::~ManiTrackUser()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiTrackUser::Load(void)
{
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t player; 

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		hash_table[player.user_id] = i;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Unloaded
//---------------------------------------------------------------------------------
void	ManiTrackUser::Unload(void)
{
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiTrackUser::LevelInit(void)
{
	for (int i = 0; i < 65536; i ++)
	{
		hash_table[i] = -1;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Client Active
//---------------------------------------------------------------------------------
void	ManiTrackUser::ClientActive(edict_t *pEntity)
{
	if(pEntity && !pEntity->IsFree() )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (playerinfo && playerinfo->IsConnected())
		{
			hash_table[playerinfo->GetUserID()] = engine->IndexOfEdict(pEntity);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiTrackUser::ClientDisconnect(player_t	*player_ptr)
{
	hash_table[player_ptr->user_id] = -1;
}


ManiTrackUser	g_ManiTrackUser;
ManiTrackUser	*gpManiTrackUser;