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
#ifndef __linux__
#include <winsock.h>
#endif
#include <mysql.h>
#include <mysqld_error.h>
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
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_skins.h"
#include "mani_client.h"
#include "mani_database.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiDatabase::ManiDatabase()
{
	// Init
	gpManiDatabase = this;
}

ManiDatabase::~ManiDatabase()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Initialise the database fields
//---------------------------------------------------------------------------------
bool ManiDatabase::Init(void)
{

	LoadDatabaseFile();
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Load up database.txt
//---------------------------------------------------------------------------------
bool ManiDatabase::LoadDatabaseFile(void)
{
	char	core_filename[256];

	db_enabled = false;
	db_timeout = 20;
	Q_strcpy(db_host,"");
	Q_strcpy(db_user,"");
	Q_strcpy(db_password,"");
	Q_strcpy(db_name,"");
	db_port = 3306; // Default MYSQL port
	Q_strcpy(db_table_prefix,"map_");

	// Game server information (probably in wrong class really)
	server_id = 1;
	Q_strcpy(server_group_id,"Default");
	Q_strcpy(server_name,"");
	Q_strcpy(server_ip_address,"");
	server_port = 27015;
	Q_strcpy(mod_name,"");
	Q_strcpy(rcon_password,"");

	Q_strcpy(db_tb_client,"client");
	Q_strcpy(db_tb_steam,"steam");
	Q_strcpy(db_tb_nick,"nick");
	Q_strcpy(db_tb_ip,"ip");
	Q_strcpy(db_tb_flag,"flag");
	Q_strcpy(db_tb_server,"server");
	Q_strcpy(db_tb_group,"group");
	Q_strcpy(db_tb_client_group,"client_group");
	Q_strcpy(db_tb_client_flag,"client_flag");
	Q_strcpy(db_tb_client_level,"client_level");
	Q_strcpy(db_tb_level,"level");
	Q_strcpy(db_tb_client_server,"client_server");
	Q_strcpy(db_tb_version,"version");

//	MMsg("*********** Loading database.txt ************\n");
	// Read the database.txt file

	KeyValues *kv_ptr = new KeyValues("database.txt");

	Q_snprintf(core_filename, sizeof (core_filename), "./cfg/%s/database.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
//		MMsg("Failed to load database.txt, no database usage possible\n");
		kv_ptr->deleteThis();
		return false;
	}

	Q_strcpy(db_host, kv_ptr->GetString("db_host"));
	Q_strcpy(db_user, kv_ptr->GetString("db_user"));
	Q_strcpy(db_password, kv_ptr->GetString("db_password"));
	Q_strcpy(db_name, kv_ptr->GetString("db_name"));
	Q_strcpy(db_table_prefix, kv_ptr->GetString("db_table_prefix","map_"));
	Q_strcpy(db_socket_path, kv_ptr->GetString("db_socket_path"));
	db_port = kv_ptr->GetInt("db_port", 3306);
	db_timeout = kv_ptr->GetInt("db_timeout", 10);
	if (kv_ptr->GetInt("db_enabled", 0) == 1)
	{
		db_enabled = true;
	}

	Q_strcpy(db_tb_client, kv_ptr->GetString("db_table_client", "client"));
	Q_strcpy(db_tb_steam, kv_ptr->GetString("db_table_steam", "steam"));
	Q_strcpy(db_tb_nick, kv_ptr->GetString("db_table_nick", "nick"));
	Q_strcpy(db_tb_ip, kv_ptr->GetString("db_table_ip", "ip"));;
	Q_strcpy(db_tb_flag, kv_ptr->GetString("db_table_flag", "flag"));
	Q_strcpy(db_tb_server, kv_ptr->GetString("db_table_server", "server"));
	Q_strcpy(db_tb_client_group, kv_ptr->GetString("db_table_client_group", "client_group"));
	Q_strcpy(db_tb_client_flag, kv_ptr->GetString("db_table_client_flag", "client_flag"));
	Q_strcpy(db_tb_client_level, kv_ptr->GetString("db_table_client_level", "client_level"));
	Q_strcpy(db_tb_level, kv_ptr->GetString("db_table_level", "level"));
	Q_strcpy(db_tb_client_server, kv_ptr->GetString("db_table_client_server", "client_server"));
	Q_strcpy(db_tb_version, kv_ptr->GetString("db_table_version", "version"));


	Q_strcpy(server_name, kv_ptr->GetString("server_name"));
	Q_strcpy(server_ip_address, kv_ptr->GetString("server_ip_address"));
	Q_strcpy(mod_name, kv_ptr->GetString("mod_name"));
	Q_strcpy(rcon_password, kv_ptr->GetString("rcon_password"));
	Q_strcpy(server_group_id, kv_ptr->GetString("server_group_id", "Default"));
	server_id = kv_ptr->GetInt("server_id", 1);
	server_port = kv_ptr->GetInt("server_port", 27015);

	kv_ptr->deleteThis();

	if (db_enabled)
	{
//		MMsg("Using database connection for administration\n");
	}

//	MMsg("*********** database.txt loaded ************\n");

	return true;
}

ManiDatabase	g_ManiDatabase;
ManiDatabase	*gpManiDatabase;
