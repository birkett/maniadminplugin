//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_DATABASE_H
#define MANI_DATABASE_H

class ManiDatabase
{

public:
	ManiDatabase();
	~ManiDatabase();

	bool	Init(void);
	int		GetServerID(void) {return server_id;}
	char	*GetServerGroupID(void) {return server_group_id;}
	char	*GetServerName(void) {return server_name;}
	char	*GetServerIPAddress(void) {return server_ip_address;}
	int		GetServerPort(void) {return server_port;}
	char	*GetModName(void) {return mod_name;}
	char	*GetRCONPassword(void) {return rcon_password;}

	bool	GetDBEnabled(void) {return db_enabled;}
	unsigned int	GetDBTimeout(void) {return db_timeout;}
	char	*GetDBHost(void) {return db_host;}
	char	*GetDBUser(void) {return db_user;}
	char	*GetDBPassword(void) {return db_password;}
	char	*GetDBName(void) {return db_name;}
	char	*GetDBTablePrefix(void) {return db_table_prefix;}
	char	*GetDBSocketPath(void) {return db_socket_path;}
	int		GetDBPort(void) {return db_port;}

	char	*GetDBTBClient(void) {return db_tb_client;}
	char	*GetDBTBSteam(void) {return db_tb_steam;}
	char	*GetDBTBNick(void) {return db_tb_nick;}
	char	*GetDBTBIP(void) {return db_tb_ip;}
	char	*GetDBTBFlag(void) {return db_tb_flag;}
	char	*GetDBTBServer(void) {return db_tb_server;}
	char	*GetDBTBGroup(void) {return db_tb_group;}
	char	*GetDBTBClientGroup(void) {return db_tb_client_group;}
	char	*GetDBTBClientFlag(void) {return db_tb_client_flag;}
	char	*GetDBTBClientLevel(void) {return db_tb_client_level;}
	char	*GetDBTBLevel(void) {return db_tb_level;}
	char	*GetDBTBClientServer(void) {return db_tb_client_server;}
	char	*GetDBTBVersion(void) {return db_tb_version;}
	int		GetDBLogLevel(void) {return db_log_level;}

private:

	bool	LoadDatabaseFile(void);

	bool	db_enabled;
	unsigned int		db_timeout;
	char	db_host[256];
	char	db_user[256];
	char	db_password[256];
	char	db_name[256];
	int		db_port;
	char	db_table_prefix[128];
	char	db_socket_path[256];

	// Table name information
	char	db_tb_client[64];
	char	db_tb_steam[64];
	char	db_tb_nick[64];
	char	db_tb_ip[64];
	char	db_tb_flag[64];
	char	db_tb_server[64];
	char	db_tb_group[64];
	char	db_tb_client_group[64];
	char	db_tb_client_flag[64];
	char	db_tb_client_level[64];
	char	db_tb_level[64];
	char	db_tb_client_server[64];
	char	db_tb_version[64];
	int		db_log_level;

	// Game server information (probably in wrong class really)
	int		server_id;
	char	server_group_id[32];
	char	server_name[128];
	char	server_ip_address[32];
	int		server_port;
	char	mod_name[64];
	char	rcon_password[64];
};

extern	ManiDatabase *gpManiDatabase;

#endif
