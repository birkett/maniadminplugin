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



#ifndef MANI_DATABASE_H
#define MANI_DATABASE_H

class ManiDatabase
{

public:
	ManiDatabase();
	~ManiDatabase();

	bool	Init(void);
	int		GetServerID(void) {return server_id;}
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
	int		GetDBPort(void) {return db_port;}

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

	// Game server information (probably in wrong class really)
	int		server_id;
	char	server_name[128];
	char	server_ip_address[32];
	int		server_port;
	char	mod_name[64];
	char	rcon_password[64];
};

extern	ManiDatabase *gpManiDatabase;

#endif
