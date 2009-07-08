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
#else
#include <pthread.h>
#include <errno.h>
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
#include "mani_timers.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_database.h"
#include "mani_mysql.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	CGlobalVars *gpGlobals;
extern	int	max_players;
extern	bool	war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiMySQL::ManiMySQL()
{
	// Init
	my_data = NULL;
	res_ptr = NULL;
	fd = NULL;
	row = NULL;
	strcpy(sql_server_version, "");
	major = minor = issue = 0;
}

ManiMySQL::~ManiMySQL()
{
	// Cleanup
	if (res_ptr) mysql_free_result( res_ptr ) ;
	if (my_data) mysql_close( my_data ) ;
	my_data = NULL;
	res_ptr = NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Create a connection to a database
//---------------------------------------------------------------------------------
bool ManiMySQL::Init(player_t *player_ptr)
{
	static	unsigned int timeout;
	bool	connection_failed = false;
	bool	tried_connection = false;
	
	timeout = gpManiDatabase->GetDBTimeout();

	if (res_ptr) 
	{
		mysql_free_result( res_ptr ) ;
		res_ptr = NULL;
	}

	if (my_data) 
	{
		mysql_close( my_data ) ;
		my_data = NULL;
	}

	if ((my_data = mysql_init((MYSQL*) 0)) == NULL) 
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Failed to init database!");
		return false;
	}

	if (mysql_options(my_data, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &timeout))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "mysql_options failed!");
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", mysql_error(my_data));
	}

	// Linux only !!
#ifdef __linux__
	if (FStrEq(gpManiDatabase->GetDBHost(), "localhost") ||
	    FStrEq(gpManiDatabase->GetDBHost(), "127.0.0.1"))
	{
		if (gpManiDatabase->GetDBSocketPath() &&
			!FStrEq(gpManiDatabase->GetDBSocketPath(), ""))
		{
			tried_connection = true;

			if (mysql_real_connect
				(
				my_data, 
				gpManiDatabase->GetDBHost(),
				gpManiDatabase->GetDBUser(), 
				gpManiDatabase->GetDBPassword(), 
				gpManiDatabase->GetDBName(), 
				gpManiDatabase->GetDBPort(),
				gpManiDatabase->GetDBSocketPath(), 
				0 ) == NULL)
			{
				connection_failed = true;
			}
		}
	}
#endif

	// Only if not tried connection yet !!
	if (!tried_connection)
	{
		if (mysql_real_connect
				(
				my_data, 
				gpManiDatabase->GetDBHost(),
				gpManiDatabase->GetDBUser(), 
				gpManiDatabase->GetDBPassword(), 
				gpManiDatabase->GetDBName(), 
				gpManiDatabase->GetDBPort(),
				NULL, 
				0 ) == NULL)
		{
			connection_failed = true;
		}
	}

	if (connection_failed)
	{
		error_code = mysql_errno(my_data);
		OutputHelpText(ORANGE_CHAT, player_ptr, "mysql_real_connect failed!") ;
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}
	

	if ( mysql_select_db( my_data, gpManiDatabase->GetDBName()) != 0 ) 
	{
		error_code = mysql_errno(my_data);
		OutputHelpText(ORANGE_CHAT, player_ptr, "Can't select the %s database!", gpManiDatabase->GetDBName() ) ;
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Execute a query
//---------------------------------------------------------------------------------
bool ManiMySQL::ExecuteQuery
(
 player_t *player_ptr,
 int	*row_count,
 char	*sql_query,
 ...
 )
{
	*row_count = 0;

	if (res_ptr)
	{
		mysql_free_result( res_ptr ) ;
		res_ptr = NULL;
	}

	va_list		argptr;
	char		temp_string[4096];

	va_start ( argptr, sql_query );
	vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		OutputHelpText(ORANGE_CHAT, player_ptr, "sql [%s] failed", temp_string ) ;
		OutputHelpText(ORANGE_CHAT, player_ptr, "error %i", mysql_errno(my_data));
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}

	res_ptr = mysql_store_result( my_data );
	if (res_ptr)
	{
		*row_count = (int) mysql_num_rows( res_ptr ); 
	}
	else
	{
		*row_count = -1;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Execute a query
//---------------------------------------------------------------------------------
bool ManiMySQL::ExecuteQuery
(
 player_t *player_ptr,
 char	*sql_query,
 ...
 )
{
	if (res_ptr)
	{
		mysql_free_result( res_ptr ) ;
		res_ptr = NULL;
	}

	va_list		argptr;
	char		temp_string[4096];

	va_start ( argptr, sql_query );
	vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		OutputHelpText(ORANGE_CHAT, player_ptr, "sql [%s] failed", temp_string ) ;
		OutputHelpText(ORANGE_CHAT, player_ptr, "error %i", mysql_errno(my_data));
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}

	res_ptr = mysql_store_result( my_data );
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Fetch row into row (char **)
//---------------------------------------------------------------------------------
bool ManiMySQL::FetchRow(void)
{
	row = mysql_fetch_row(res_ptr);
	if (row == NULL) return false;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Get row id (after auto increment insert
//---------------------------------------------------------------------------------
int	ManiMySQL::GetRowID(void)
{
	return (int) mysql_insert_id(my_data);
}

//---------------------------------------------------------------------------------
// Purpose: Get sql server version
//---------------------------------------------------------------------------------
char *ManiMySQL::GetServerVersion(void)
{
	if (strcmp(sql_server_version,"") == 0)
	{
		Q_strcpy(sql_server_version, mysql_get_server_info(my_data));
		if (strcmp(sql_server_version, "") != 0)
		{
			int i = 0;
			int j = 0;
			char buffer[32];

			while (sql_server_version[i] != '.')
			{
				buffer[j++] = sql_server_version[i++];
			}

			buffer[j] = '\0';
			i++;
			major = atoi(buffer);

			j = 0;
			while (sql_server_version[i] != '.')
			{
				buffer[j++] = sql_server_version[i++];
			}
				
			buffer[j] = '\0';
			i++;
			minor = atoi(buffer);

			j = 0;
			// We add isdigit here because sometimes they like to do things like
			// '5.0.24-standard'
			while (sql_server_version[i] != '\0' && isdigit(sql_server_version[i]) != 0)
			{
				buffer[j++] = sql_server_version[i++];
			}

			buffer[j] = '\0';
			issue = atoi(buffer);
		}
	}

	return sql_server_version;
}

//---------------------------------------------------------------------------------
// Purpose: Check if version higher than passed in version
//---------------------------------------------------------------------------------
bool	ManiMySQL::IsHigherVer(int maj, int min, int iss)
{
	if (major > maj) return true;
	if (major == maj && minor > min) return true;
	if (major == maj && minor == min && issue > iss) return true;

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if version higher than passed in version
//---------------------------------------------------------------------------------
bool	ManiMySQL::IsHigherVer(int maj, int min)
{
	if (major > maj) return true;
	if (major == maj && minor > min) return true;

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Check if version higher than passed in version
//---------------------------------------------------------------------------------
bool	ManiMySQL::IsHigherVer(int maj)
{
	if (major > maj) return true;

	return false;
}
