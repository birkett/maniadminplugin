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
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_sourcehook.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_maps.h"
#include "mani_automap.h" 
#include "mani_vote.h"
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	time_t	g_RealTime;

CONVAR_CALLBACK_PROTO(ManiAutoMapMapList);
CONVAR_CALLBACK_PROTO(ManiAutoMapTimer);

ConVar mani_automap ("mani_automap", "0", 0, "0 = disabled, 1 = enabled", true, 0, true, 1);
ConVar mani_automap_map_list ("mani_automap_map_list", "", 0, "Setup your maps that will used seperated by a colon, e.g. de_dust:de_aztec:cs_office", CONVAR_CALLBACK_REF(ManiAutoMapMapList));
ConVar mani_automap_player_threshold ("mani_automap_player_threshold", "0", 0, "Player limit before an automap change will not take place", true, 0, true, 64);
ConVar mani_automap_include_bots ("mani_automap_include_bots", "0", 0, "0 = disabled, 1 = include bots as part of player count", true, 0, true, 1);
ConVar mani_automap_timer ("mani_automap_timer", "240", 0, "Time in seconds before map will be changed once player threshold reached", true, 60, true, 86400, CONVAR_CALLBACK_REF(ManiAutoMapTimer));
ConVar mani_automap_set_nextmap ("mani_automap_set_nextmap", "0", 0, "0 = Disabled, 1 = Once map changed set next map to be same as the changed map", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiAutoMap::ManiAutoMap()
{
	gpManiAutoMap = this;
	automap_list = NULL;
	automap_list_size = 0;
	set_next_map = false;
	ignore_this_map = false;
}

ManiAutoMap::~ManiAutoMap()
{
	// Cleanup
	FreeList((void **) &automap_list, &automap_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiAutoMap::Load(void)
{
	this->ResetTimeout(240);
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiAutoMap::Unload(void)
{
	set_next_map = false;
	FreeList((void **) &automap_list, &automap_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiAutoMap::LevelInit(void)
{
	this->ResetTimeout(mani_automap_timer.GetInt());
	ignore_this_map = false;

	if (set_next_map)
	{
		int map_choice = this->ChooseMap();

		Q_strcpy(forced_nextmap, automap_list[map_choice].map_name);
		Q_strcpy(next_map, automap_list[map_choice].map_name);
		mani_nextmap.SetValue(automap_list[map_choice].map_name);
		override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
		override_setnextmap = true;

		// Make sure end of map vote doesn't try and override it
		gpManiVote->SysSetMapDecided(true);

		set_next_map = false;
		ignore_this_map = true;
		SetChangeLevelReason("Automap set nextmap");
		LogCommand (NULL, "Autochange set nextmap %s while server idle\n", automap_list[map_choice].map_name);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process Game Frame
//---------------------------------------------------------------------------------
void ManiAutoMap::GameFrame(void)
{
	if (war_mode || 
		mani_automap.GetInt() == 0 || 
		ignore_this_map || 
		automap_list_size == 0) return;

	if (g_RealTime < trigger_time) return;

	trigger_time += 15;

	int	players = 0;
	bool include_bots = mani_automap_include_bots.GetBool();
	int	threshold = mani_automap_player_threshold.GetInt();

	// Count players
	for (int i = 1; i <= max_players; i++)
	{
		// Faster than FindPlayerByIndex()
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				if (playerinfo->IsHLTV()) continue;
				if (!include_bots && strcmp(playerinfo->GetNetworkIDString(), "BOT") == 0) continue;

				players ++;
				if (players > threshold)
				{
					// Broken threshold so just ignore, but reset the timeout
					this->ResetTimeout(mani_automap_timer.GetInt());
					return;
				}
			}
		}
	}

	// Need to change map
	if (mani_automap_set_nextmap.GetInt() != 0)
	{
		set_next_map = true;
	}
	else
	{
		set_next_map = false;
	}

	int map_choice = this->ChooseMap();

	override_changelevel = 0;
	override_setnextmap = false;
	ignore_this_map = true;
	LogCommand (NULL, "Autochange to map %s while server idle\n", automap_list[map_choice].map_name);
	SetChangeLevelReason("Automap changed map");

 	char	changelevel_command[128];
	snprintf(changelevel_command, sizeof(changelevel_command), "changelevel %s\n", automap_list[map_choice].map_name);
	engine->ServerCommand(changelevel_command);
}

//---------------------------------------------------------------------------------
// Purpose: Choose map
//---------------------------------------------------------------------------------
int		ManiAutoMap::ChooseMap(void)
{
	// Choose a map
	int map_choice = 0;

	if (automap_list_size > 1)
	{
		map_choice = rand() % automap_list_size;
	}

	return map_choice;
}

//---------------------------------------------------------------------------------
// Purpose: Allow external classes to set the override
//---------------------------------------------------------------------------------
void	ManiAutoMap::SetMapOverride(bool enable_override)
{
	set_next_map = enable_override;
}

//---------------------------------------------------------------------------------
// Purpose: Initialise our private vars
//---------------------------------------------------------------------------------
void	ManiAutoMap::ResetTimeout(int seconds)
{
	time_t	current_time;

	time(&current_time);

	trigger_time = current_time + seconds;
}

//---------------------------------------------------------------------------------
// Purpose: Setup the map list to choose from
//---------------------------------------------------------------------------------
void	ManiAutoMap::SetupMapList(void)
{
	FreeList((void **) &automap_list, &automap_list_size);
	ignore_this_map = true;

	const char *map_string = mani_automap_map_list.GetString();

	if (FStrEq(map_string,"")) return;

	ignore_this_map = false;

	int i = 0;
	int j = 0;
	char	map_name[64] = "";

	for (;;)
	{
		if (map_string[i] == ':' || map_string[i] == '\0')
		{
			map_name[j] = '\0';
			if (i != 0)
			{
				if (engine->IsMapValid(map_name))
				{
					AddToList((void **) &automap_list, sizeof(automap_id_t), &automap_list_size);
					Q_strcpy(automap_list[automap_list_size - 1].map_name, map_name);
					if (FStrEq(map_name, current_map))
					{
						ignore_this_map = true;
					}
				}

				j = 0;
				if (map_string[i] == '\0')
				{
					break;
				}
				else
				{
					i++;
					j = 0;
					continue;
				}
			}
			else
			{
				break;
			}
		}

		map_name[j] = map_string[i];
		j++;
		i++;
	}
}

CONVAR_CALLBACK_FN(ManiAutoMapMapList)
{
	gpManiAutoMap->SetupMapList();
}

CONVAR_CALLBACK_FN(ManiAutoMapTimer)
{
	if (!FStrEq(pOldString, mani_automap_timer.GetString()))
	{
		gpManiAutoMap->ResetTimeout(mani_automap_timer.GetInt());
	}
}

ManiAutoMap	g_ManiAutoMap;
ManiAutoMap	*gpManiAutoMap;
