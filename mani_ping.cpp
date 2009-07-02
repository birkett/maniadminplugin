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
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_ping.h" 
#include "mani_vfuncs.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

static int sort_ping_immunity_by_steam_id ( const void *m1,  const void *m2);

static void HighPingKick ( ConVar *var, char const *pOldString );
static void HighPingKickSamples ( ConVar *var, char const *pOldString );

ConVar mani_high_ping_kick ("mani_high_ping_kick", "0", 0, "This defines whether the high ping kicker is enabled or not", true, 0, true, 1, HighPingKick); 
ConVar mani_high_ping_kick_samples_required ("mani_high_ping_kick_samples_required", "60", 0, "This defines the amount of samples required before the player is kicked", true, 0, true, 10000, HighPingKickSamples); 
ConVar mani_high_ping_kick_ping_limit ("mani_high_ping_kick_ping_limit", "400", 0, "This defines the ping limit before a player is kicked", true, 0, true, 99999999); 
ConVar mani_high_ping_kick_message ("mani_high_ping_kick_message", "Your ping is too high", 0, "This defines the message given to the player in their console on disconnect"); 

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiPing::ManiPing()
{
	ping_immunity_list = NULL;
	ping_immunity_list_size = 0;
	gpManiPing = this;
}

ManiPing::~ManiPing()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiPing::Load(void)
{
	this->LoadImmunityList();

	for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
	{
		this->ResetPlayer(i);
	}

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		
		if (player.is_bot) continue;

		int dummy;
		if (!this->IsPlayerImmune(&player) && !gpManiClient->IsPotentialAdmin(&player, &dummy))
		{
			ping_list[player.index - 1].check_ping = true;
		}
	}

	next_check = 0.0;
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiPing::Unload(void)
{
	FreeList((void **) &ping_immunity_list, &ping_immunity_list_size);

	for (int i = 0; i < MANI_MAX_PLAYERS; i ++)
	{
		this->ResetPlayer(i);
	}

	next_check = 0.0;
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiPing::LevelInit(void)
{
	this->LoadImmunityList();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		this->ResetPlayer(i);
	}
	
	next_check = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiPing::NetworkIDValidated(player_t *player_ptr)
{
	if (war_mode) return;
	if (mani_high_ping_kick.GetInt() == 0) return;
	if (player_ptr->is_bot) return;

	int index = player_ptr->index - 1;

	this->ResetPlayer(player_ptr->index - 1);

	int dummy;
	if (!this->IsPlayerImmune(player_ptr) && !gpManiClient->IsPotentialAdmin(player_ptr, &dummy))
	{
		ping_list[player_ptr->index - 1].check_ping = true;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Player immune ?
//---------------------------------------------------------------------------------
bool	ManiPing::IsPlayerImmune(player_t *player_ptr)
{
	// Check if on immunity list
	ping_immunity_t	ping_immunity_key;

	// Do BSearch for steam ID
	Q_strcpy(ping_immunity_key.steam_id, player_ptr->steam_id);

	if (NULL != (ping_immunity_t *) bsearch
						(
						&ping_immunity_key, 
						ping_immunity_list, 
						ping_immunity_list_size, 
						sizeof(ping_immunity_t), 
						sort_ping_immunity_by_steam_id
						))
	{
		// player is on the list
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiPing::ClientDisconnect(player_t	*player_ptr)
{
	if (war_mode) return;
	if (mani_high_ping_kick.GetInt() == 0) return;

	this->ResetPlayer(player_ptr->index - 1);
}

//---------------------------------------------------------------------------------
// Purpose: Do timed afk if needed
//---------------------------------------------------------------------------------
void ManiPing::GameFrame(void)
{
	if (war_mode) return;
	if (mani_high_ping_kick.GetInt() == 0) return;

	// High ping kicker enabled
	if (next_check > gpGlobals->curtime)
	{
		return;
	}

	next_check = gpGlobals->curtime + 1.5;

	for (int i = 0; i < max_players; i++)
	{
		if (!ping_list[i].check_ping)
		{
			continue;
		}

		edict_t *pEntity = engine->PEntityOfEntIndex(i + 1);
		if(pEntity && !pEntity->IsFree() )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				const char	*steam_id = playerinfo->GetNetworkIDString();
				INetChannelInfo *nci = engine->GetPlayerNetInfo(i + 1);
				const char *ip_address = nci->GetAddress();

				float ping = nci->GetAvgLatency(0);
				const char * szCmdRate = engine->GetClientConVarValue( i + 1, "cl_cmdrate" );

				int nCmdRate = max( 20, Q_atoi( szCmdRate ) );
				ping -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

				// in GoldSrc we had a different, not fixed tickrate. so we have to adjust
				// Source pings by half a tick to match the old GoldSrc pings.
				ping -= TICKS_TO_TIME( 0.5f );
				ping = ping * 1000.0f; // as msecs
				ping = clamp( ping, 5, 1000 ); // set bounds, dont show pings under 5 msecs

				if (ping_list[i].count == 0)
				{
					// Init ping
					ping_list[i].average_ping = ping;
				}
				else
				{
					ping_list[i].average_ping += ping;
				}

				// Bump up average ping
				ping_list[i].count += 1;

				if (ping_list[i].count > mani_high_ping_kick_samples_required.GetInt())
				{
					if ((ping_list[i].average_ping / ping_list[i].count) > mani_high_ping_kick_ping_limit.GetInt())
					{
						player_t player;

						player.index = i + 1;
						if (FindPlayerByIndex(&player))
						{
							SayToAll (ORANGE_CHAT, false, "Player %s was autokicked for breaking the %ims ping limit on this server\n", 
											player.name,
											mani_high_ping_kick_ping_limit.GetInt()
											);

							char	log_reason[256];
							Q_snprintf(log_reason, sizeof(log_reason), "[MANI_ADMIN_PLUGIN] Kicked player [%s] steam id [%s] for exceeding ping limit\n", player.name, player.steam_id);

							UTIL_KickPlayer(&player, 
								(char *) mani_high_ping_kick_message.GetString(),
								(char *) mani_high_ping_kick_message.GetString(),
								log_reason);
						}

						ping_list[i].check_ping = false;
					}
					else
					{
						ping_list[i].count = 0;
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset a player on the ping list
//---------------------------------------------------------------------------------
void ManiPing::ResetPlayer(int index)
{
	ping_list[index].check_ping = false;
	ping_list[index].count = 0;
	ping_list[index].average_ping = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Load the player immunity list
//---------------------------------------------------------------------------------
void ManiPing::LoadImmunityList(void)
{
	FileHandle_t file_handle;
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	base_filename[256];

	FreeList((void **) &ping_immunity_list, &ping_immunity_list_size);

	//Get ping immunity player list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/pingimmunity.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL) return;

	while (filesystem->ReadLine (steam_id, sizeof(steam_id), file_handle) != NULL)
	{
		if (!ParseLine(steam_id, true, false))
		{
			// String is empty after parsing
			continue;
		}

		AddToList((void **) &ping_immunity_list, sizeof(ping_immunity_t), &ping_immunity_list_size);
		Q_strcpy(ping_immunity_list[ping_immunity_list_size - 1].steam_id, steam_id);
	}

	qsort(ping_immunity_list, ping_immunity_list_size, sizeof(ping_immunity_t), sort_ping_immunity_by_steam_id); 
	filesystem->Close(file_handle);

}

static void HighPingKick ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		gpManiPing->Load();
	}
}

static void HighPingKickSamples ( ConVar *var, char const *pOldString )
{
	if (!FStrEq(pOldString, var->GetString()))
	{
		gpManiPing->Load();
	}
}

static int sort_ping_immunity_by_steam_id ( const void *m1,  const void *m2) 
{
	struct ping_immunity_t *mi1 = (struct ping_immunity_t *) m1;
	struct ping_immunity_t *mi2 = (struct ping_immunity_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

ManiPing	g_ManiPing;
ManiPing	*gpManiPing;