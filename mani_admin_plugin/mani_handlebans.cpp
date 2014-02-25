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

#include <stdio.h>
#include <time.h>

#include "mani_handlebans.h"
#include "icvar.h"
#include "const.h"
#include "filesystem.h"
#include "edict.h"
#include "iplayerinfo.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "igameevents.h"

#include "mani_main.h"
#include "mani_convar.h"
#include "mani_mainclass.h"
#include "mani_playerkick.h"
#include "mani_memory.h"
#include "mani_parser.h"

#define MAX_BANS_PER_TICK	(25)

extern	IVEngineServer	*engine;
extern	IFileSystem	*filesystem;
extern	int max_players;

int ban_list_size;
ban_settings_t *ban_list;

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}


void CManiHandleBans::WriteBans ( void ) {
	FileHandle_t file_handle;
	char base_filename[256];
	time_t now;
	time ( &now );

	if ( ban_list ) {
		snprintf(base_filename, sizeof (base_filename), "./cfg/%s/banlist.txt", mani_path.GetString());
		file_handle = filesystem->Open (base_filename,"wt",NULL);
		if (file_handle != NULL) {
			gpManiAdminPlugin->PrintHeader ( file_handle, "banlist.txt", "list of steam ids and IPs that are banned" );
			filesystem->FPrintf ( file_handle, "// This file contains the list of bans that\n" );
			filesystem->FPrintf ( file_handle, "// have been given via the ma_ban command.\n" );
			filesystem->FPrintf ( file_handle, "//\n" );
			filesystem->FPrintf ( file_handle, "//\n" );
			filesystem->FPrintf ( file_handle, "// The first entry is the STEAM_ID or the IP.\n" );
			filesystem->FPrintf ( file_handle, "// The second entry is the time the ban expires. 0 = permanent.\n" );
			filesystem->FPrintf ( file_handle, "// The third entry is the players name. ( quotes required )\n" );
			filesystem->FPrintf ( file_handle, "// The fourth entry is who executed the ban. ( quotes required )\n" );
			filesystem->FPrintf ( file_handle, "// The fifth entry ( optional ) is why the ban was given. ( quotes required )\n" );
			filesystem->FPrintf ( file_handle, "//\n" );
			filesystem->FPrintf ( file_handle, "// STEAM_0:0:000000 0 \"RoadRunner\" \"Wile E. Coyote\" \"obvious speedhack\"\n" );
			filesystem->FPrintf ( file_handle, "//\n" );

			for ( int index = 0; index < ban_list_size; index++ ) {
				if ( (ban_list[index].expire_time != 0) && (ban_list[index].expire_time <= now) ) continue;
				if ( ban_list[index].reason[0] != 0 )
					filesystem->FPrintf ( file_handle, "%s %i \"%s\" \"%s\" \"%s\"\n", ban_list[index].key_id, (int)ban_list[index].expire_time, ban_list[index].player_name, ban_list[index].ban_initiator, ban_list[index].reason );
				else
					filesystem->FPrintf ( file_handle, "%s %i \"%s\" \"%s\"\n", ban_list[index].key_id, (int)ban_list[index].expire_time, ban_list[index].player_name, ban_list[index].ban_initiator );
			}
			filesystem->Close(file_handle);
		}
	}
}

bool CManiHandleBans::AddBan ( ban_settings_t *ban ) {
	bool found = false;
	int index = 0;
	while ( !found ) {
		if ( found || (index == ban_list_size) ) break;
		found = FStrEq ( ban->key_id, ban_list[index++].key_id );
	}
	ban->byID = ((ban->key_id[0] == 'S') || (ban->key_id[0] == 's'));

	if ( !found ) {
		AddToList((void **) &ban_list, sizeof(ban_settings_t), &ban_list_size);
		ban_list[ban_list_size - 1] = *ban;
	} else { // update ban
		index--; // put it back to where it was found.
		Q_strcpy ( ban_list[index].ban_initiator, ban->ban_initiator );
		Q_strcpy ( ban_list[index].reason, ban->reason );
		Q_strcpy ( ban_list[index].player_name, ban->player_name );
		ban_list[index].expire_time = ban->expire_time;
	}

	return !found;
}

bool CManiHandleBans::AddBan ( player_t *player, const char *key, const char *initiator, int ban_time, const char *prefix, const char *reason ) {
	ban_settings_t ban;
	time_t now;
	time ( &now );
	Q_memset ( &ban, 0, sizeof (ban_settings_t));

	if ( !player )
		return false;

	if ( !key || (key[0] == 0))
		return false;

	if ( !initiator || (initiator[0] == 0))
		return false; 

	if ( ban_time != 0 )
		ban.expire_time = (time_t)(now + ( ban_time * 60 ));

	Q_strcpy ( ban.key_id, key );
	Q_strcpy ( ban.ban_initiator, initiator );
	Q_strcpy ( ban.player_name, player->name );

	ban.byID = ( (key[0] == 'S') || (key[0] == 's') );
	if ( reason )
		Q_strcpy ( ban.reason, reason );
	else
		Q_strcpy ( ban.reason, prefix );

	char ban_cmd[512];
	Q_memset( &ban_cmd, 0, sizeof(ban_cmd) );
	
	if ( ban.byID ) { // ban by steamid
		snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i\n", ban_time, player->user_id );
		engine->ServerCommand(ban_cmd);
		if ( ban.reason ) {
			gpManiPlayerKick->AddPlayer ( player->index, 0.5f, ban.reason );
		} else {
			gpManiPlayerKick->AddPlayer ( player->index, 0.5f );
		}
	} else { // ban by IP
		char *localprefix = ( prefix != NULL ) ? prefix : "Banned IP (By Admin)";

		if ( reason ) {
			for ( int i = 1; i <= max_players; i++ ) {
				player_t plr;
				plr.index = i;
				if (!FindPlayerByIndex(&plr)) continue;
				if ( !plr.ip_address || (plr.ip_address[0] == 0)) continue;
				if ( !target_player_list[i-1].ip_address || (target_player_list[i-1].ip_address == 0)) continue;

				if ( FStrEq ( plr.ip_address, target_player_list[i-1].ip_address ) ) 
					UTIL_KickPlayer ( &plr, (char *)reason, localprefix, localprefix );
			}
		}

		snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", ban_time, player->ip_address);
		engine->ServerCommand(ban_cmd);
	}

	return AddBan ( &ban );
}

bool CManiHandleBans::RemoveBan( const char *key ) {
	bool found = false;
	int index = 0;
	while ( !found ) {
		if ( found || (index == ban_list_size) ) break;
		found = FStrEq ( key, ban_list[index++].key_id );
	}

	if ( found )
		ban_list[--index].expire_time = 1; // this ensures the ban will be removed on next write.

	return found;
}

void CManiHandleBans::ReadBans(void) {
	char	base_filename[256];
	FileHandle_t file_handle;
	char	ban_list_line[512];

	ban_settings_t banned_player;
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/banlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle != NULL) {
		while (filesystem->ReadLine (ban_list_line, sizeof(ban_list_line), file_handle) != NULL)
		{
			Q_memset ( &banned_player, 0, sizeof(ban_settings_t) );
			if (!ParseBanLine(ban_list_line, &banned_player, true, false))
			{
				// String is empty after parsing
				continue;
			}

			time_t now;
			time ( &now );
			int time_to_ban = 0;
			if ( banned_player.expire_time != 0 ) {
				time_to_ban = (int) (banned_player.expire_time - now)/60;
				if ( time_to_ban <= 0 ) continue;  // time has expired!
			}
			if ( time_to_ban >= 0 ) 
				gpManiHandleBans->AddBan ( &banned_player );
		}
		filesystem->Close(file_handle);
	}
}


void CManiHandleBans::LevelInit() {
	ReadBans();
	current_index = 0;
	starting_count = ban_list_size;
}

void CManiHandleBans::GameFrame() {
	int stopping_point = current_index + MAX_BANS_PER_TICK;
	if ( stopping_point > starting_count ) stopping_point = starting_count;

	for ( int i = current_index; i <stopping_point; i++ )
		SendBanToServer(i);

	current_index = stopping_point;
}

void CManiHandleBans::SendBanToServer(int ban_index) {
	char ban_cmd[512];
	ban_settings_t banned_player;
	if ( ban_index > ban_list_size ) return;

	banned_player = ban_list[ban_index];
	time_t now;
	time ( &now );

	int time_to_ban = 0;
	if ( banned_player.expire_time != 0 ) 
		time_to_ban = (int) (banned_player.expire_time - now)/60;

	if ( banned_player.byID )
		snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %s\n", time_to_ban, banned_player.key_id);
	else
		snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", time_to_ban, banned_player.key_id );
	engine->ServerCommand(ban_cmd);
}

CManiHandleBans g_ManiHandleBans;
CManiHandleBans *gpManiHandleBans = &g_ManiHandleBans;
