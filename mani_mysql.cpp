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
bool ManiMySQL::Init(void)
{
	static	unsigned int timeout;
	
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
		Msg("Failed to init database\n");
		return false;
	}

	if (mysql_options(my_data, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &timeout))
	{
		Msg("mysql_options failed !!\n");
		Msg( "%s\n", mysql_error(my_data));
	}

//timer_id = ManiGetTimer();
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
		error_code = mysql_errno(my_data);
		Msg( "mysql_real_connect failed !\n") ;
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
//Msg("mysql_real_connect() time %f\n", ManiGetTimerDuration(timer_id));
		return false;
	}
	
//Msg("mysql_real_connect() time %f\n", ManiGetTimerDuration(timer_id));

//timer_id = ManiGetTimer();
	if ( mysql_select_db( my_data, gpManiDatabase->GetDBName()) != 0 ) 
	{
		error_code = mysql_errno(my_data);
		Msg( "Can't select the %s database !\n", gpManiDatabase->GetDBName() ) ;
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
//Msg("mysql_select_db() time %f\n", ManiGetTimerDuration(timer_id));
		return false;
	}
//Msg("mysql_select_db() time %f\n", ManiGetTimerDuration(timer_id));

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Execute a query
//---------------------------------------------------------------------------------
bool ManiMySQL::ExecuteQuery
(
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
	Q_vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

//timer_id = ManiGetTimer();

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		Msg( "sql [%s] failed\n", temp_string ) ;
		Msg( "error %i\n", mysql_errno(my_data));
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
//Msg("time %f mysql_query([%s])\n", ManiGetTimerDuration(timer_id), temp_string);
		return false;
	}

//Msg("time %f mysql_query([%s])\n", ManiGetTimerDuration(timer_id), temp_string);

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
	Q_vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

//timer_id = ManiGetTimer();

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		Msg( "sql [%s] failed\n", temp_string ) ;
		Msg( "error %i\n", mysql_errno(my_data));
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
//Msg("time %f mysql_query([%s])\n", ManiGetTimerDuration(timer_id), temp_string);
		return false;
	}
//Msg("time %f mysql_query([%s])\n", ManiGetTimerDuration(timer_id), temp_string);

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

