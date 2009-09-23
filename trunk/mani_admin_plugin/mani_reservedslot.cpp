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
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_gametype.h"
#include "mani_reservedslot.h"
#include "shareddefs.h"
#include "steamclientpublic.h"
#include "mani_detours.h"
#include "detour_macros.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern IFileSystem	*filesystem;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern void *connect_client_addr;
extern void *netsendpacket_addr;

static int sort_active_players_by_ping ( const void *m1,  const void *m2);
static int sort_active_players_by_connect_time ( const void *m1,  const void *m2);
static int sort_reserve_slots_by_steam_id ( const void *m1,  const void *m2);
static CDetour * ManiClientConnectDetour;
static CDetour * ManiNetSendPacketDetour;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

#define COPY_BYTE(target, source) \
	target = source[0]; \
	source++

#define COPY_STRING(target, source) \
	strcount = 0; \
	while (source[0] != 0) { \
		target[strcount++]=source[0]; \
		source++; \
	} \
	source++

#define COPY_SHORT(target, source) \
	memcpy (target, source, 2); \
	source+=2;

#define COPY_BOOL(target, source) \
	target = ( source[0] != 0); \
	source++

// we don't actually have to fill this data structure, but thought it might be easier
// to work with if we did when changes come about.
void FillINFOQuery ( const mem_t* data, A2S_INFO_t &info, mem_t **pPlayers, mem_t **pPassword ) {
	int strcount = 0;
	if ( (data[0]!=0xFF) && (data[1]!=0xFF) && (data[2]!=0xFF) && (data[3]!=0xFF) ) return;

	//byte	type;
	//byte	netversion;
	//char	server_name[256];
	//char	map[256];
	//char	gamedir[256];
	//char	gamedesc[256];
	//short	appid;
	//byte	players;
	//byte	maxplayers;
	//byte	bots;
	//char	dedicated;
	//char	os;
	//bool	passwordset;
	//bool	secure;
	//char	version[256];

	data+=4;
	COPY_BYTE(info.type, data);
	COPY_BYTE(info.netversion, data);
	COPY_STRING(info.server_name, data);
	COPY_STRING(info.map, data);
	COPY_STRING(info.gamedir, data);
	COPY_STRING(info.gamedesc, data);
	COPY_SHORT(&info.appid, data);
	*pPlayers = (mem_t *)data;
	COPY_BYTE(info.players, data);
	COPY_BYTE(info.maxplayers, data);
	COPY_BYTE(info.bots, data);
	COPY_BYTE(info.dedicated, data);
	COPY_BYTE(info.os, data);
	*pPassword = (mem_t *)data;
	COPY_BOOL(info.passwordset, data);
	COPY_BOOL(info.secure,data);
	COPY_STRING(info.version, data);
}

#if defined ( ORANGE )
DECL_MEMBER_DETOUR8_void(ConnectClientDetour, void *, int, int, int, const char *, const char *, const char*, int ) {
	MMsg("ConnectClient\n");
	return MEMBER_CALL(ConnectClientDetour)(p1,p2,p3,p4,p5,p6,p7,p8);
}
#else
DECL_MEMBER_DETOUR10_void(ConnectClientDetour, void *, int, int, int, const char *, const char *, const char*, int, char const*, int ) {
	MMsg("ConnectClient\n");
	return MEMBER_CALL(ConnectClientDetour)(p1,p2,p3,p4,p5,p6,p7,p8,p9,pA);
}
#endif

#if defined ( ORANGE )
DECL_DETOUR7_void( NET_SendPacketDetour, void *, int, void *, const mem_t *, int, void *, bool) {
#else
DECL_DETOUR5_void( NET_SendPacketDetour, void *, int, void *, const mem_t *, int ) {
#endif
	char strIP[19]; // IPv6 capable!
	if ( p3 ) {
		int ip = *(int *)((const char *)p3 + 4);
		snprintf(strIP, sizeof(strIP), "%u.%u.%u.%u", ip & 0xFF, ( ip >> 8 ) & 0xFF, ( ip >> 16 ) & 0xFF, (ip >> 24) & 0xFF);
	}

	mem_t *pPassword = NULL;
	mem_t *pPlayers = NULL;
	A2S_INFO_t QueryData;
	memset (&QueryData, 0, sizeof(QueryData));

	FillINFOQuery( p4, QueryData, &pPlayers, &pPassword );
	if ( QueryData.netversion == 7 ) {
		//need to search valid IPs here ( last IP connected by admin flag )
		if ( pPlayers ) {
			//		if ( pPlayers[0] == pPlayers[1] )
			pPlayers[1]++;
		}

		if ( pPassword )
			pPassword[0]=0;
	}
	return NON_MEMBER_CALL(NET_SendPacketDetour)(p1,p2,p3,p4,p5);
}

ManiReservedSlot::ManiReservedSlot()
{
	// Init
	active_player_list_size = 0;
	active_player_list = NULL;
	reserve_slot_list = NULL;
	reserve_slot_list_size = 0;
	gpManiReservedSlot = this;
}

ManiReservedSlot::~ManiReservedSlot()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Init stuff for plugin load
//---------------------------------------------------------------------------------
void ManiReservedSlot::CleanUp(void)
{
	FreeList((void **) &reserve_slot_list, &reserve_slot_list_size);
	FreeList((void **) &active_player_list, &active_player_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Init stuff for plugin load
//---------------------------------------------------------------------------------
void ManiReservedSlot::Load(void)
{
	ManiClientConnectDetour = CDetourManager::CreateDetour( "ConnectClient", connect_client_addr, GET_MEMBER_CALLBACK(ConnectClientDetour), GET_MEMBER_TRAMPOLINE(ConnectClientDetour));
	if ( ManiClientConnectDetour )
		ManiClientConnectDetour->DetourFunction( );

	ASSIGN_ORIGINAL(NET_SendPacketDetour, netsendpacket_addr);

	ManiNetSendPacketDetour = CDetourManager::CreateDetour( "NETSendPacket", netsendpacket_addr, GET_NON_MEMBER_CALLBACK(NET_SendPacketDetour), GET_NON_MEMBER_TRAMPOLINE(NET_SendPacketDetour) );
	if ( ManiNetSendPacketDetour )
		ManiNetSendPacketDetour->DetourFunction( );
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Level load
//---------------------------------------------------------------------------------
void ManiReservedSlot::LevelInit(void)
{
	FileHandle_t file_handle;
	char	steam_id[MAX_NETWORKID_LENGTH];
	char	base_filename[256];

	this->CleanUp();
	//Get reserve player list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/reserveslots.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		//		MMsg("Failed to load reserveslots.txt\n");
	}
	else
	{
		//		MMsg("Reserve Slot list\n");
		while (filesystem->ReadLine (steam_id, sizeof(steam_id), file_handle) != NULL)
		{
			if (!ParseLine(steam_id, true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &reserve_slot_list, sizeof(reserve_slot_t), &reserve_slot_list_size);
			Q_strcpy(reserve_slot_list[reserve_slot_list_size - 1].steam_id, steam_id);
			//			MMsg("[%s]\n", steam_id);
		}

		qsort(reserve_slot_list, reserve_slot_list_size, sizeof(reserve_slot_t), sort_reserve_slots_by_steam_id);
		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on connect
//---------------------------------------------------------------------------------
bool ManiReservedSlot::NetworkIDValidated(player_t	*player_ptr)
{
	int			players_on_server = 0;
	bool		is_reserve_player = false;
	player_t	temp_player;
	int		 	players_to_kick = 0;
	int			allowed_players;
	int			total_players;

	if (war_mode)
	{
		return true;
	}

	total_players = GetNumberOfActivePlayers() - 1;
	// DirectLogCommand("[DEBUG] Total players on server [%i]\n", total_players);

	if (total_players < max_players - mani_reserve_slots_number_of_slots.GetInt())
	{
		// DirectLogCommand("[DEBUG] No reserve slot action required\n");
		return true;
	}


	GetIPAddressFromPlayer(player_ptr);
	Q_strcpy (player_ptr->steam_id, engine->GetPlayerNetworkIDString(player_ptr->entity));
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(player_ptr->entity);
	if (playerinfo && playerinfo->IsConnected())
	{
		Q_strcpy(player_ptr->name, playerinfo->GetName());
	}
	else
	{
		Q_strcpy(player_ptr->name,"");
	}

	// DirectLogCommand("[DEBUG] Index = [%i] IP Address [%s] Steam ID [%s]\n",
	//									player_ptr->index, player_ptr->ip_address, player_ptr->steam_id);

	// DirectLogCommand("[DEBUG] Processing player\n");

	if (FStrEq("BOT", player_ptr->steam_id))
	{
		// DirectLogCommand("[DEBUG] Player joining is bot, ignoring\n");
		return true;
	}

	player_ptr->is_bot = false;


	if (IsPlayerInReserveList(player_ptr))
	{
		// DirectLogCommand("[DEBUG] Player is on reserve slot list\n");
		is_reserve_player = true;
	}
	else if (mani_reserve_slots_include_admin.GetInt() == 1 &&
		gpManiClient->HasAccess(player_ptr->index, ADMIN, ADMIN_BASIC_ADMIN))

	{
		// DirectLogCommand("[DEBUG] Player is admin\n");
		is_reserve_player = true;
	}

	// DirectLogCommand("[DEBUG] Building players who can be kicked list\n");
	/*
	if (active_player_list_size != 0)
	{
	DirectLogCommand("[DEBUG] Players that can be kicked list\n");

	for (int i = 0; i < active_player_list_size; i++)
	{
	DirectLogCommand("[DEBUG] Name [%s] Steam [%s] Spectator [%s] Ping [%f] TimeConnected [%f]\n",
	active_player_list[i].name,
	active_player_list[i].steam_id,
	(active_player_list[i].is_spectator) ? "YES":"NO",
	active_player_list[i].ping,
	active_player_list[i].time_connected
	);
	}
	}
	else
	{
	DirectLogCommand("[DEBUG] No players available for kicking\n");
	}
	*/
	if (mani_reserve_slots_allow_slot_fill.GetInt() == 1)
	{
		BuildPlayerKickList(player_ptr, &players_on_server);
		// Allow reserve slots to fill first
		allowed_players = max_players - mani_reserve_slots_number_of_slots.GetInt();
		// DirectLogCommand("[DEBUG] Allowed players = [%i]\n", allowed_players);
		if (active_player_list_size >= allowed_players)
		{
			// standard players are exceeding slot allocation
			players_to_kick = active_player_list_size - allowed_players;
			for (int i = 0; i < players_to_kick; i++)
			{
				if (i == active_player_list_size)
				{
					break;
				}

				// Disconnect other players that got on (safe guard)
				// DirectLogCommand("[DEBUG] Kicking player [%s]\n", active_player_list[i].name);
				temp_player.index = active_player_list[i].index;
				FindPlayerByIndex(&temp_player);
				DisconnectPlayer(&temp_player);
			}

			if (!is_reserve_player)
			{
				// DirectLogCommand("[DEBUG] Kicking player [%s]\n", player_ptr->steam_id);
				DisconnectPlayer(player_ptr);
				return false;
			}
		}
	}
	else
	{
		// Keep reserve slots free at all times
		allowed_players = max_players - mani_reserve_slots_number_of_slots.GetInt();
		if (total_players >= allowed_players)
		{
			players_to_kick = total_players - allowed_players;
			if (is_reserve_player)
			{
				players_to_kick ++;
			}
			else
			{
				// Disconnect this player as they are not allowed on and redirect them
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", player_ptr->steam_id);
				DisconnectPlayer(player_ptr);
			}

			if (players_to_kick != 0)
			{
				BuildPlayerKickList(player_ptr, &players_on_server);
			}

			for (int i = 0; i < players_to_kick; i++)
			{
				if (i == active_player_list_size)
				{
					break;
				}

				// Disconnect other players that got on (safe guard)
				//DirectLogCommand("[DEBUG] Kicking player [%s]\n", active_player_list[i].name);
				temp_player.index = active_player_list[i].index;
				FindPlayerByIndex(&temp_player);
				DisconnectPlayer(&temp_player);
			}

			if (!is_reserve_player)
			{
				return false;
			}
		}
	}

	// DirectLogCommand("[DEBUG] Player [%s] is allowed to join\n", player_ptr->steam_id);

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Builds up a list of players that are 'kickable'
//---------------------------------------------------------------------------------
void ManiReservedSlot::BuildPlayerKickList(player_t *player_ptr, int *players_on_server)
{
	player_t	temp_player;
	active_player_t active_player;

	FreeList((void **) &active_player_list, &active_player_list_size);

	for (int i = 1; i <= max_players; i ++)
	{
		edict_t *pEntity = engine->PEntityOfEntIndex(i);
		if(pEntity && !pEntity->IsFree() && pEntity != player_ptr->entity)
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (playerinfo && playerinfo->IsConnected())
			{
				Q_strcpy(active_player.steam_id, playerinfo->GetNetworkIDString());
				if (FStrEq("BOT", active_player.steam_id))
				{
					continue;
				}

				INetChannelInfo *nci = engine->GetPlayerNetInfo(i);
				if (!nci)
				{
					continue;
				}

				active_player.entity = pEntity;

				active_player.ping = nci->GetAvgLatency(0);
				const char * szCmdRate = engine->GetClientConVarValue( i, "cl_cmdrate" );
				int nCmdRate = (20 > Q_atoi( szCmdRate )) ? 20 : Q_atoi(szCmdRate);
				active_player.ping -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

				// in GoldSrc we had a different, not fixed tickrate. so we have to adjust
				// Source pings by half a tick to match the old GoldSrc pings.
				active_player.ping -= TICKS_TO_TIME( 0.5f );
				active_player.ping = active_player.ping * 1000.0f; // as msecs
				active_player.ping = ((5 > active_player.ping) ? 5:active_player.ping); // set bounds, dont show pings under 5 msecs

				active_player.time_connected = nci->GetTimeConnected();
				Q_strcpy(active_player.ip_address, nci->GetAddress());
				if (gpManiGameType->IsSpectatorAllowed() &&
					playerinfo->GetTeamIndex () == gpManiGameType->GetSpectatorIndex())
				{
					active_player.is_spectator = true;
				}
				else
				{
					active_player.is_spectator = false;
				}
				active_player.user_id = playerinfo->GetUserID();
				Q_strcpy(active_player.name, playerinfo->GetName());

				*players_on_server = *players_on_server + 1;

				Q_strcpy(temp_player.steam_id, active_player.steam_id);
				Q_strcpy(temp_player.ip_address, active_player.ip_address);
				Q_strcpy(temp_player.name, active_player.name);
				temp_player.is_bot = false;

				if (IsPlayerInReserveList(&temp_player))
				{
					continue;
				}

				active_player.index = i;

				if (mani_reserve_slots_include_admin.GetInt() == 1 &&
					gpManiClient->HasAccess(active_player.index, ADMIN, ADMIN_BASIC_ADMIN))
				{
					continue;
				}

				if (gpManiClient->HasAccess(active_player.index, IMMUNITY, IMMUNITY_RESERVE))
				{
					continue;
				}

				AddToList((void **) &active_player_list, sizeof(active_player_t), &active_player_list_size);
				active_player_list[active_player_list_size - 1] = active_player;
			}
		}
	}

	if (mani_reserve_slots_kick_method.GetInt() == 0)
	{
		// Kick by highest ping
		qsort(active_player_list, active_player_list_size, sizeof(active_player_t), sort_active_players_by_ping);
	}
	else
	{
		// Kick by shortest connection time
		qsort(active_player_list, active_player_list_size, sizeof(active_player_t), sort_active_players_by_connect_time);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check if weapon restricted for this weapon name
//---------------------------------------------------------------------------------
void ManiReservedSlot::DisconnectPlayer(player_t *player_ptr)
{
//	char	disconnect[512];

//	if (FStrEq(mani_reserve_slots_redirect.GetString(), ""))
//	{
		// No redirection required
		UTIL_KickPlayer(player_ptr, (char *) mani_reserve_slots_kick_message.GetString(), (char *) mani_reserve_slots_kick_message.GetString(), (char *) mani_reserve_slots_kick_message.GetString());
		PrintToClientConsole( player_ptr->entity, "%s\n", mani_reserve_slots_kick_message.GetString());
		// engine->ClientCommand( player_ptr->entity, "wait;wait;wait;wait;wait;wait;wait;disconnect\n");
/*	}
	else
	{
		// Redirection required
		PrintToClientConsole( player_ptr->entity, "%s\n", mani_reserve_slots_redirect_message.GetString());
		Q_snprintf(disconnect, sizeof (disconnect), "wait;wait;wait;wait;wait;wait;wait;connect %s\n", mani_reserve_slots_redirect.GetString());
		engine->ClientCommand( player_ptr->entity, disconnect);
	}
*/
	return;
}

//---------------------------------------------------------------------------------
// Purpose: See if player is on reserve list
//---------------------------------------------------------------------------------
bool ManiReservedSlot::IsPlayerInReserveList(player_t *player_ptr)
{

	reserve_slot_t	reserve_slot_key;

	// Do BSearch for steam ID
	Q_strcpy(reserve_slot_key.steam_id, player_ptr->steam_id);

	if (NULL == (reserve_slot_t *) bsearch
		(
		&reserve_slot_key,
		reserve_slot_list,
		reserve_slot_list_size,
		sizeof(reserve_slot_t),
		sort_reserve_slots_by_steam_id
		))
	{
		return false;
	}

	return true;
}

static int sort_reserve_slots_by_steam_id ( const void *m1,  const void *m2)
{
	struct reserve_slot_t *mi1 = (struct reserve_slot_t *) m1;
	struct reserve_slot_t *mi2 = (struct reserve_slot_t *) m2;
	return strcmp(mi1->steam_id, mi2->steam_id);
}

static int sort_active_players_by_ping ( const void *m1,  const void *m2)
{
	struct active_player_t *mi1 = (struct active_player_t *) m1;
	struct active_player_t *mi2 = (struct active_player_t *) m2;

	if (mi1->is_spectator == true && mi2->is_spectator == false)
	{
		return -1;
	}

	if (mi1->is_spectator == false && mi2->is_spectator == true)
	{
		return 1;
	}

	if (mi1->ping > mi2->ping)
	{
		return -1;
	}
	else if (mi1->ping < mi2->ping)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);

}

static int sort_active_players_by_connect_time ( const void *m1,  const void *m2)
{
	struct active_player_t *mi1 = (struct active_player_t *) m1;
	struct active_player_t *mi2 = (struct active_player_t *) m2;

	if (mi1->is_spectator == true && mi2->is_spectator == false)
	{
		return -1;
	}

	if (mi1->is_spectator == false && mi2->is_spectator == true)
	{
		return 1;
	}

	if (mi1->time_connected < mi2->time_connected)
	{
		return -1;
	}
	else if (mi1->time_connected > mi2->time_connected)
	{
		return 1;
	}

	return strcmp(mi1->steam_id, mi2->steam_id);

}

ManiReservedSlot	g_ManiReservedSlot;
ManiReservedSlot	*gpManiReservedSlot;
