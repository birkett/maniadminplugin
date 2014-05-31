//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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
#include "inetchannelinfo.h"
#include "mani_convar.h"
#include "mani_output.h"
#include "mani_main.h"
#include "mani_file.h"
#include "mani_client.h"
#include "mani_client_sql.h"
#include "mani_client_util.h"
#include "mani_util.h"
#include "mani_keyvalues.h"

#undef strlen

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
#include "mani_timers.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_database.h"
#include "mani_client_sql.h"

//#define MANI_SHOW_LOCKS

extern	bool	war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

SQLManager::SQLManager(int thread_number, int sleep_time)
{
	// Init
	my_data = NULL;
	res_ptr = NULL;
	fd = NULL;
	row = NULL;
	first = NULL;
	last = NULL;
	thread_alive = false;
	kill_thread = false;
	accept_requests = false;
	this->thread_number = thread_number;
	this->sleep_time = sleep_time;
}

SQLManager::~SQLManager()
{
	// Cleanup
	this->Unload();
}

//---------------------------------------------------------------------------------
// Purpose: Stop our thread
//---------------------------------------------------------------------------------
void SQLManager::Unload(void)
{
	if (!thread_alive) return;

	this->KillThread();
	this->RemoveAllRequests();
	if (res_ptr) mysql_free_result( res_ptr ) ;
	if (my_data) mysql_close( my_data ) ;
	my_data = NULL;
	res_ptr = NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Stop our thread
//---------------------------------------------------------------------------------
void SQLManager::KillThread(void)
{
	if (!kill_thread)
	{
		kill_thread = true;
	}

#ifndef __linux__
	// Windows

	// Wait for thread to terminate
	WaitForSingleObject (thread_id, INFINITE);
	thread_alive = false;
	// Remove mutex
	mutex.Destroy();
#else
	pthread_join(thread_id, NULL);
	thread_alive = false;
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Fire up a new thread
//---------------------------------------------------------------------------------
bool SQLManager::Load(void)
{
	if (!gpManiDatabase->GetDBEnabled()) return false;

	kill_thread = false;

// Create a mutex with no initial owner.

	if (!mutex.Create())
	{
		return false;
	}

// moved this line from above the create -- the call to trylock was failing with EINVAL,
// meaning it wasn't initialized yet. -- Keeper
#ifndef __linux__
	thread_id = CreateThread( 
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		( LPTHREAD_START_ROUTINE ) SQLManager::CALLBACK_ThreadFunc,					 // thread function 
		(LPVOID)this,                // argument to thread function 
		0,                           // use default creation flags 
		NULL);                // returns the thread identifier 

	// Check the return value for success. 

	if (thread_id == NULL) 
	{
		MMsg("Create thread failed\n");
		CloseHandle(request_list_mutex);
		return false;
	}

#else
// Linux version
// Create a mutex with no initial owner.


	pthread_create (&thread_id, NULL, (void*(*)(void *))SQLManager::CALLBACK_ThreadFunc, this);
#endif

	thread_alive = true;
	accept_requests = true;
	MMsg("Create thread %i success\n", thread_number);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Windows hack to return static ptr
//---------------------------------------------------------------------------------
#ifndef __linux__
DWORD SQLManager::CALLBACK_ThreadFunc(LPVOID data)
{
	return static_cast<SQLManager*>(data)->RunThread(); 
}
#else
void *SQLManager::CALLBACK_ThreadFunc(void *data)
{
	return reinterpret_cast<SQLManager*>(data)->RunThread(); 
}
#endif
//---------------------------------------------------------------------------------
// Purpose: Thread start function
//---------------------------------------------------------------------------------
#ifndef __linux__
DWORD SQLManager::RunThread(void)
#else
void *SQLManager::RunThread(void)
#endif
{
	// Loop until a request comes in 
	for (;;)
	{
		// Check keep alive
		// Request ownership of mutex.
		if (mutex.Lock())
		{
			// Got lock on mutex
			SQLProcessBlock *request_ptr = this->CheckRequests();
			if (request_ptr == NULL)
			{
				// Give mutex back
				mutex.Unlock();
				MSleep(sleep_time);
			}
			else
			{
				mutex.Unlock();
				// Open database up for use
				while (!this->GetConnection())
				{
					MSleep(2000);
					if (kill_thread)
					{
						break;
					}
				}	
				
				request_ptr->ProcessBlock(this);
				mutex.LockWait();
				request_ptr->SetCompleted(true);
				// Give mutex back
				mutex.Unlock();
				// Close MySQL connection
				if (res_ptr) mysql_free_result( res_ptr ) ;
				if (my_data) mysql_close( my_data ) ;
				my_data = NULL;
				res_ptr = NULL;
			}
		}
		else
		{
			MSleep(sleep_time);
		}

		if (kill_thread)
		{
			break;
		}
	}

#ifndef __linux__
	ExitThread(0);
	return 0;
#else
	pthread_exit(NULL);
	reinterpret_cast<void*>(0);
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Check if there is a request to process
//---------------------------------------------------------------------------------
SQLProcessBlock *SQLManager::CheckRequests(void)
{
	if (first == NULL)
	{
		return NULL;
	}

	for (request_list_t *i = first; i != NULL; i = i->next_ptr)
	{
		if (!i->request->IsComplete())
		{
			return i->request;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Create a connection to a database
//---------------------------------------------------------------------------------
bool SQLManager::GetConnection(void)
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
		LogSQL("Failed to init database\n");
		return false;
	}

	if (mysql_options(my_data, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &timeout))
	{
		LogSQL("mysql_options failed !!\n");
		LogSQL( "%s\n", mysql_error(my_data));
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
		LogSQL( "mysql_real_connect failed !\n") ;
		LogSQL( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}
	

	if ( mysql_select_db( my_data, gpManiDatabase->GetDBName()) != 0 ) 
	{
		error_code = mysql_errno(my_data);
		LogSQL( "Can't select the %s database !\n", gpManiDatabase->GetDBName() ) ;
		LogSQL( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Remove a node from the system
//---------------------------------------------------------------------------------
void    SQLManager::RemoveRequestStruct(struct request_list_t *node_ptr)
{
	if (node_ptr->next_ptr == NULL)
	{
		// This node has no nodes after it
		if (node_ptr->prev_ptr == NULL)
		{
			// This node is also the only node
			last = NULL;
			first = NULL;
		}
		else
		{
			// Not the first but definately the last
			last = node_ptr->prev_ptr;
			last->next_ptr = NULL;
		}
	}
	else if (node_ptr->prev_ptr == NULL)
	{
		// First in the list
		if (node_ptr->next_ptr == NULL)
		{
			// Only node in the list
			last = NULL;
			first = NULL;
		}
		else
		{
			// More than one in the list but this is the first
			first = node_ptr->next_ptr;
			first->prev_ptr = NULL;
		}
	}
	else
	{
		// Nodes are either side of this one
		node_ptr->next_ptr->prev_ptr = node_ptr->prev_ptr;
		node_ptr->prev_ptr->next_ptr = node_ptr->next_ptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free all the list
//---------------------------------------------------------------------------------
void    SQLManager::RemoveAllRequests(void)
{
	request_list_t *current_ptr;
	request_list_t *next_ptr;

	if (first == NULL) return;

	current_ptr = first;

	while(current_ptr != NULL)
	{
		next_ptr = current_ptr->next_ptr;
		current_ptr->request;
		free(current_ptr);
		current_ptr = next_ptr;
	}

	first = NULL;
	last = NULL;

}

//---------------------------------------------------------------------------------
// Purpose: AddRequest
//---------------------------------------------------------------------------------
void	SQLManager::AddRequest
(
 SQLProcessBlock	*request
)
{
	request_list_t *request_list_ptr = NULL;

	request_list_ptr = (request_list_t *) malloc(sizeof(request_list_t));
	if (request_list_ptr == NULL)
	{
		request_list_ptr = NULL;
		return;
	}

	// Assign client process class to request
	request_list_ptr->request = request;

	request_list_ptr->next_ptr = NULL;
	request_list_ptr->prev_ptr = NULL;

	// Add it to the end of the linked list (need mutex for it)
	mutex.LockWait();

	// Add to the linked list
	if (first == NULL)
	{
		first = request_list_ptr;
		last = request_list_ptr;
		request_list_ptr->next_ptr = NULL;
		request_list_ptr->prev_ptr = NULL;
	}
	else
	{
		request_list_ptr->prev_ptr = last;
		last->next_ptr = request_list_ptr;
		last = request_list_ptr;
	}

	// Release Mutex
	mutex.Unlock();

	return;
}

//---------------------------------------------------------------------------------
// Purpose: GameFrame to parse results (main process calls this)
//---------------------------------------------------------------------------------
void	SQLManager::GameFrame(void)
{
	if (war_mode) return;
	if (first == NULL) return; // Nothing to check
	if (!accept_requests) return;

	// Get Mutex, quit if it can't get it
	if (thread_alive)
	{
		if (!mutex.Lock()) return;
	}

	for (request_list_t *i = first;i != NULL;)
	{
		request_list_t *next = i->next_ptr;
		if (i->request->IsComplete())
		{
			// Complete the request here
			this->ProcessRequestComplete(i->request);
			this->RemoveRequestStruct(i);
			delete i->request;
			free(i);
		}

		i = next;
	}

	// Release Mutex
	if (thread_alive)
	{
		mutex.Unlock();
	}
	else
	{
		// Thread is dead
		if (accept_requests && first == NULL)
		{
			// Stop more requests from being processed.
			accept_requests = false;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process a successful result
//---------------------------------------------------------------------------------
void	SQLManager::ProcessRequestComplete(SQLProcessBlock *request)
{
	int	dummy;

	if (request->out_params.GetParam("update_user_id", &dummy))
	{
		// Need to update client list with new user_id number
		char *name;
		int	 user_id;

		if (request->in_params.GetParam("user_id", &user_id) && 
			request->in_params.GetParam("name", &name))
		{
			gpManiClient->UpdateClientUserID(user_id, name);
		}

		return;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Execute a query
//---------------------------------------------------------------------------------
bool SQLManager::ExecuteQuery
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
	vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

	if (gpManiDatabase->GetDBLogLevel() > 1)
	{
		LogSQL("%s\n", temp_string);
	}

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		if (gpManiDatabase->GetDBLogLevel() > 0)
		{
			LogSQL( "sql [%s] failed\n", temp_string ) ;
			LogSQL( "error %i\n", mysql_errno(my_data));
			LogSQL( "%s\n", mysql_error(my_data));
		}		
		
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
bool SQLManager::ExecuteQuery
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
	vsnprintf( temp_string, sizeof(temp_string), sql_query, argptr );
	va_end   ( argptr );

	if (gpManiDatabase->GetDBLogLevel() > 1)
	{
		LogSQL("%s\n", temp_string);
	}

	if (mysql_query( my_data, temp_string ) != 0)
	{
		error_code = mysql_errno(my_data);
		if (gpManiDatabase->GetDBLogLevel() > 0)
		{
			LogSQL( "sql [%s] failed\n", temp_string ) ;
			LogSQL( "error %i\n", mysql_errno(my_data));
			LogSQL( "%s\n", mysql_error(my_data));
		}

		return false;
	}

	res_ptr = mysql_store_result( my_data );
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Fetch row into row (char **)
//---------------------------------------------------------------------------------
bool SQLManager::FetchRow(void)
{
	row = mysql_fetch_row(res_ptr);
	if (row == NULL) return false;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Get row id (after auto increment insert
//---------------------------------------------------------------------------------
int	SQLManager::GetRowID(void)
{
	return (int) mysql_insert_id(my_data);
}

//---------------------------------------------------------------------------------
// Purpose: Short logging code for sql
//---------------------------------------------------------------------------------
void	SQLManager::LogSQL(const char *sql, ...)
{
	char	filename[512];

	FILE *filehandle = NULL;
	ManiFile *mf = new ManiFile();

	snprintf(filename, sizeof(filename), "./mani_sql_thread%i.log", thread_number);
	filehandle = mf->Open(filename, "at+");
	if (filehandle == NULL)
	{
		delete mf;
		return;
	}

	va_list		argptr;
	char		temp_string1[4096];

	va_start ( argptr, sql );
	vsnprintf( temp_string1, sizeof(temp_string1), sql, argptr );
	va_end   ( argptr );

	struct	tm	*time_now;
	time_t	current_time;

	time(&current_time);
	time_now = localtime(&current_time);

	char	temp_string[4096];

	int	length = snprintf(temp_string, sizeof(temp_string), "M %02i/%02i/%04i - %02i:%02i:%02i: %s", 
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year + 1900,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec,
								temp_string1);

	mf->Write((void *) temp_string, length, filehandle);

	mf->Close(filehandle);
	delete mf;
}

//---------------------------------------------------------------------------------
// Purpose: Process adding a steam id for a client
//---------------------------------------------------------------------------------
void	SQLAddSteam::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *steam_id;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("steam_id", &steam_id);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s (user_id, steam_id) VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBSteam(),
			user_id,
			steam_id))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process adding an ip address for a client
//---------------------------------------------------------------------------------
void	SQLAddIPAddress::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *ip_address;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("ip_address", &ip_address);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s (user_id, ip_address) VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBIP(),
			user_id,
			ip_address))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process adding a nickname for a client
//---------------------------------------------------------------------------------
void	SQLAddNick::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *nick;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("nick", &nick);
 
	// Get clients for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s (user_id, nick) VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBNick(),
			user_id,
			nick))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process setting a name for a client
//---------------------------------------------------------------------------------
void	SQLSetName::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *old_name;
	char *new_name;

	this->in_params.GetParam("old_name", &old_name);
	this->in_params.GetParam("new_name", &new_name);
 
	// Get clients for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		old_name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("UPDATE %s%s SET name = '%s' WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClient(),
			new_name,
			user_id))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", new_name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process setting a password for a client
//---------------------------------------------------------------------------------
void	SQLSetPassword::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *password;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("password", &password);
 
	// Get clients for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("UPDATE %s%s SET password = '%s' WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClient(),
			password,
			user_id))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process setting an email address for a client
//---------------------------------------------------------------------------------
void	SQLSetEmailAddress::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *email;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("email", &email);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("UPDATE %s%s SET email = '%s' WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClient(),
			email,
			user_id))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process setting notes for a client
//---------------------------------------------------------------------------------
void	SQLSetNotes::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *notes;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("notes", &notes);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	if (row_count != 0)
	{
		manager->FetchRow();

		user_id = manager->GetInt(0);
		if (!manager->ExecuteQuery("UPDATE %s%s SET notes = '%s' WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClient(),
			notes,
			user_id))
		{
			return;
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process setting a level/class for a client
//---------------------------------------------------------------------------------
void	SQLSetLevel::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	int	 level_id;
	char *class_type;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("level_id", &level_id);
	this->in_params.GetParam("class_type", &class_type);
 
	// Get client for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		// Drop level id reference
		if (!manager->ExecuteQuery(&row_count,
			"DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND server_group_id = '%s' "
			"AND type = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientLevel(),
			user_id,
			gpManiDatabase->GetServerGroupID(),
			class_type))
		{
			return;
		}

		if (level_id > -1)
		{	
			if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s (user_id, level_id, type, server_group_id) VALUES (%i,%i,'%s','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				user_id,
				level_id,
				class_type,
				gpManiDatabase->GetServerGroupID()))
			{	
				return;
			}		
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process adding a group/class for a client
//---------------------------------------------------------------------------------
void	SQLAddGroup::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *group_id;
	char *class_type;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("group_id", &group_id);
	this->in_params.GetParam("class_type", &class_type);
 
	// Get client for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		// Check if group actually exists
		if (!manager->ExecuteQuery(&row_count,
			"SELECT 1 "
			"FROM %s%s "
			"WHERE group_id = '%s' "
			"AND server_group_id = '%s' "
			"AND type = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBGroup(),
			group_id,
			gpManiDatabase->GetServerGroupID(),
			class_type))
		{
			return;
		}

		if (row_count == 0)
		{
			return;
		}

		// Drop group id reference
		if (!manager->ExecuteQuery(&row_count,
			"DELETE FROM %s%s "
			"WHERE group_id = '%s' "
			"AND user_id = %i "
			"AND server_group_id = '%s' "
			"AND type = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientGroup(),
			group_id,
			user_id,
			gpManiDatabase->GetServerGroupID(),
			class_type))
		{
			return;
		}

		if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s (user_id, group_id, type, server_group_id) VALUES (%i,'%s','%s','%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientGroup(),
			user_id,
			group_id,
			class_type,
			gpManiDatabase->GetServerGroupID()))
		{	
			return;
		}		

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}


//---------------------------------------------------------------------------------
// Purpose: Process adding a global group
//---------------------------------------------------------------------------------
void	SQLAddGroupType::ProcessBlock(SQLManager *manager)
{
	char *flag_string;
	char *group_id;
	char *class_type;
	bool insert_required;

	this->in_params.GetParam("flag_string", &flag_string);
	this->in_params.GetParam("group_id", &group_id);
	this->in_params.GetParam("class_type", &class_type);
	this->in_params.GetParam("insert", &insert_required);
 
	if (insert_required)
	{
		// Drop group id reference
		if (!manager->ExecuteQuery(
			"INSERT IGNORE INTO %s%s (group_id, flag_string, type, server_group_id) VALUES ('%s', '%s', '%s', '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBGroup(),
			group_id,
			flag_string,
			class_type,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}
	}
	else
	{
		if (!manager->ExecuteQuery(
			"UPDATE %s%s "
			"SET flag_string = '%s' "
			"WHERE group_id = '%s' "
			"AND type = '%s' "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBGroup(),
			flag_string,
			group_id,
			class_type,
			gpManiDatabase->GetServerGroupID()))
		{	
			return;
		}		
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a global group
//---------------------------------------------------------------------------------
void	SQLRemoveGroupType::ProcessBlock(SQLManager *manager)
{
	char *group_id;
	char *class_type;

	this->in_params.GetParam("group_id", &group_id);
	this->in_params.GetParam("class_type", &class_type);
 
	// Drop group id reference
	if (!manager->ExecuteQuery(
		"DELETE FROM %s%s "
		"WHERE group_id = '%s' "
		"AND type = '%s' "
		"AND server_group_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBGroup(),
		group_id,
		class_type,
		gpManiDatabase->GetServerGroupID()))
	{
		return;
	}

	if (!manager->ExecuteQuery(
		"DELETE FROM %s%s "
		"WHERE group_id = '%s' "
		"AND type = '%s' "
		"AND server_group_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientGroup(),
		group_id,
		class_type,
		gpManiDatabase->GetServerGroupID()))
	{	
		return;
	}		
}

//---------------------------------------------------------------------------------
// Purpose: Process adding a global level/class
//---------------------------------------------------------------------------------
void	SQLAddLevelType::ProcessBlock(SQLManager *manager)
{
	char *flag_string;
	int	 level_id;
	char *class_type;
	bool insert_required;

	this->in_params.GetParam("flag_string", &flag_string);
	this->in_params.GetParam("level_id", &level_id);
	this->in_params.GetParam("class_type", &class_type);
	this->in_params.GetParam("insert", &insert_required);
 
	if (insert_required)
	{
		// Insert level id reference
		if (!manager->ExecuteQuery(
			"INSERT IGNORE INTO %s%s (level_id, type, flag_string, server_group_id) VALUES (%i, '%s', '%s', '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBLevel(),
			level_id,
			class_type,
			flag_string,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}
	}
	else
	{
		if (!manager->ExecuteQuery(
			"UPDATE %s%s "
			"SET flag_string = '%s' "
			"WHERE level_id = %i "
			"AND type = '%s' "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBLevel(),
			flag_string,
			level_id,
			class_type,
			gpManiDatabase->GetServerGroupID()))
		{	
			return;
		}		
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a global level
//---------------------------------------------------------------------------------
void	SQLRemoveLevelType::ProcessBlock(SQLManager *manager)
{
	int	 level_id;
	char *class_type;

	this->in_params.GetParam("level_id", &level_id);
	this->in_params.GetParam("class_type", &class_type);
 
	// Drop level id reference
	if (!manager->ExecuteQuery(
		"DELETE FROM %s%s "
		"WHERE level_id = %i "
		"AND type = '%s' "
		"AND server_group_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBLevel(),
		level_id,
		class_type,
		gpManiDatabase->GetServerGroupID()))
	{
		return;
	}

	if (!manager->ExecuteQuery(
		"DELETE FROM %s%s "
		"WHERE level_id = %i "
		"AND type = '%s' "
		"AND server_group_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientLevel(),
		level_id,
		class_type,
		gpManiDatabase->GetServerGroupID()))
	{	
		return;
	}		
}

//---------------------------------------------------------------------------------
// Purpose: Process settings individual flags for a client
//---------------------------------------------------------------------------------
void	SQLSetFlag::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *class_type;
	char *flag_string;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("flag_string", &flag_string);
	this->in_params.GetParam("class_type", &class_type);
 
	// Get client for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		// Remove flags
		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i	"
			"AND type =	'%s' "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientFlag(),
			user_id,
			class_type,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}

		if (flag_string && strcmp(flag_string, "") != 0)
		{
			// Add them
			if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s	(user_id, flag_string, type, server_group_id) VALUES (%i,'%s','%s','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientFlag(),
				user_id,
				flag_string,
				class_type,
				gpManiDatabase->GetServerGroupID()))
			{
				return;
			}
		}

		out_params.AddParam("user_id", user_id);
		out_params.AddParam("name", name);
		out_params.AddParam("update_user_id", 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a client
//---------------------------------------------------------------------------------
void	SQLRemoveClient::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;

	this->in_params.GetParam("name", &name);
 
	// Get client for this server
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientServer(),
			user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}

		if (!manager->ExecuteQuery(&row_count,
			"SELECT 1 FROM %s%s "
			"WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientServer(),
			user_id))
		{
			return;
		}

		if (row_count == 0)
		{
			// No client server records so delete unique client
			if (!manager->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				user_id))
			{
				return;
			}
		}				

		// Delete rest of the crap
		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientFlag(),
			user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientGroup(),
			user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND server_group_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientLevel(),
			user_id,
			gpManiDatabase->GetServerGroupID()))
		{
			return;
		}

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i ",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBSteam(),
			user_id))
		{
			return;
		}

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i ",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBIP(),
			user_id))
		{
			return;
		}

		if (!manager->ExecuteQuery("DELETE FROM %s%s"
			"WHERE user_id = %i ",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBNick(),
			user_id))
		{
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a client's steam id
//---------------------------------------------------------------------------------
void	SQLRemoveSteam::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *steam_id;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("steam_id", &steam_id);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND steam_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBSteam(),
			user_id,
			steam_id))
		{
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a client's ip address
//---------------------------------------------------------------------------------
void	SQLRemoveIPAddress::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *ip;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("ip", &ip);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND ip_address = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBIP(),
			user_id,
			ip))
		{
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a client's nick name 
//---------------------------------------------------------------------------------
void	SQLRemoveNick::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *nick;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("nick", &nick);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
			"WHERE user_id = %i "
			"AND nick = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBNick(),
			user_id,
			nick))
		{
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process removing a group from a client 
//---------------------------------------------------------------------------------
void	SQLRemoveGroup::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	int user_id;
	char *name;
	char *class_type;
	char *group_id;

	this->in_params.GetParam("name", &name);
	this->in_params.GetParam("class_type", &class_type);
	this->in_params.GetParam("group_id", &group_id);
 
	if (!manager->ExecuteQuery(&row_count, 
		"SELECT c.user_id "
		"FROM %s%s c, %s%s cs "
		"where cs.server_group_id = '%s' "
		"and cs.user_id = c.user_id "
		"and c.name = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
		gpManiDatabase->GetServerGroupID(),
		name))
	{
		return;
	}

	// Get User ID
	if (row_count != 0)
	{
		manager->FetchRow();
		user_id = manager->GetInt(0);

		if (!manager->ExecuteQuery("DELETE FROM %s%s "
				"WHERE user_id = %i "
				"AND group_id = '%s' "
				"AND type = '%s' "
				"AND server_group_id = '%s' ",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				user_id,
				group_id,
				class_type,
				gpManiDatabase->GetServerGroupID()))
		{
			return;
		}
	}
}
//---------------------------------------------------------------------------------
// Purpose: Process adding a client
//---------------------------------------------------------------------------------
void	SQLAddClient::ProcessBlock(SQLManager *manager)
{
	int user_id;
	char *name;

	this->in_params.GetParam("name", &name);
 
	if (!manager->ExecuteQuery("INSERT IGNORE INTO %s%s "
		"(name, password, email, notes) "
		"VALUES "
		"('%s', '', '', '')",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClient(),
		name))
	{
		return;
	}

	user_id = manager->GetRowID();

	// Setup client_server record
	if (!manager->ExecuteQuery("INSERT INTO %s%s (user_id, server_group_id) VALUES (%i, '%s')",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientServer(),
		user_id,
		gpManiDatabase->GetServerGroupID()))
	{
		return;
	}

	out_params.AddParam("user_id", user_id);
	out_params.AddParam("name", name);
	out_params.AddParam("update_user_id", 0);
}

//---------------------------------------------------------------------------------
// Purpose: Process adding a client
//---------------------------------------------------------------------------------
void	SQLAddFlagDesc::ProcessBlock(SQLManager *manager)
{
	int row_count = 0;
	char *class_type;
	char *flag_id;
	char *description;

	this->in_params.GetParam("class_type", &class_type);
	this->in_params.GetParam("flag_id", &flag_id);
	this->in_params.GetParam("description", &description);
 
	if (!manager->ExecuteQuery(&row_count, "SELECT f.description "
		"FROM %s%s f "
		"where f.flag_id = '%s' "
		"and f.type = '%s'",
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBFlag(),
		flag_id, class_type))
	{
		return;
	}

	if (row_count == 0)
	{
		// Setup flag record
		if (!manager->ExecuteQuery("INSERT INTO %s%s (flag_id, type, description) VALUES ('%s', '%s', '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBFlag(),
			flag_id,
			class_type,
			description))
		{
			return;
		}
	}
	else
	{
		manager->FetchRow();
		if (strcmp(manager->GetString(0), description) != 0)
		{
			// Update to the new description
			if (!manager->ExecuteQuery("UPDATE %s%s SET description = '%s' WHERE flag_id = '%s' AND type = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBFlag(),
				description,
				flag_id,
				class_type))
			{
				return;
			}
		}
	}
}

SQLManager *client_sql_manager=NULL;
