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
#include "mani_mysql_thread.h"
#include "KeyValues.h"
#include "cbaseentity.h"

//#define MANI_SHOW_LOCKS

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	CGlobalVars *gpGlobals;
extern	int	max_players;
extern	bool	war_mode;

#ifdef __linux__
static	pthread_mutex_t request_list_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiMySQLThread::ManiMySQLThread()
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
}

ManiMySQLThread::~ManiMySQLThread()
{
	// Cleanup
	this->Unload();
}

//---------------------------------------------------------------------------------
// Purpose: Stop our thread
//---------------------------------------------------------------------------------
void ManiMySQLThread::Unload(void)
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
void ManiMySQLThread::KillThread(void)
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
	CloseHandle(request_list_mutex);
#else
	pthread_join(thread_id, NULL);
	thread_alive = false;
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Fire up a new thread
//---------------------------------------------------------------------------------
bool ManiMySQLThread::Load(void)
{
	if (!gpManiDatabase->GetDBEnabled()) return false;

	kill_thread = false;

#ifndef __linux__
// Create a mutex with no initial owner.

	request_list_mutex = CreateMutex( 
			NULL,                       // no security attributes
			FALSE,                      // initially not owned
			"request_list_mutex");  // name of mutex

	if (request_list_mutex == NULL) 
	{
		// Check for error.
		return false;
	}

	thread_id = CreateThread( 
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		( LPTHREAD_START_ROUTINE ) ManiMySQLThread::CALLBACK_ThreadFunc,					 // thread function 
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


	pthread_create (&thread_id, NULL, (void*(*)(void *))ManiMySQLThread::CALLBACK_ThreadFunc, this);
#endif

	thread_alive = true;
	accept_requests = true;
	MMsg("Create thread success\n");
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Windows hack to return static ptr
//---------------------------------------------------------------------------------
#ifndef __linux__
DWORD ManiMySQLThread::CALLBACK_ThreadFunc(LPVOID data)
{
	return static_cast<ManiMySQLThread*>(data)->RunThread(); 
}
#else
void *ManiMySQLThread::CALLBACK_ThreadFunc(void *data)
{
	return reinterpret_cast<ManiMySQLThread*>(data)->RunThread(); 
}
#endif
//---------------------------------------------------------------------------------
// Purpose: Thread start function
//---------------------------------------------------------------------------------
#ifndef __linux__
DWORD ManiMySQLThread::RunThread(void)
#else
void *ManiMySQLThread::RunThread(void)
#endif
{
	int	keep_alive;

	// Open database up for use
	while (!this->Init())
	{
		this->MSleep(2000);
		if (kill_thread)
		{
			break;
		}
	}

	connection_okay = true;
	keep_alive = 0;

	// Loop until a request comes in 
	for (;;)
	{
		// Check keep alive
		// Request ownership of mutex.
		if (this->LockMutex(true))
		{
			// Got lock on mutex
			request_t *request_ptr = (request_t *) this->CheckRequests();
			if (request_ptr == NULL)
			{
				// Give mutex back
				this->UnlockMutex(true);
				this->MSleep(100);
			}
			else
			{
				this->ProcessRequestList(request_ptr);
				// Give mutex back
				this->UnlockMutex(true);
			}
		}
		else
		{
			this->MSleep(100);
		}

		if (!connection_okay)
		{
			if (res_ptr) mysql_free_result( res_ptr ) ;
			if (my_data) mysql_close( my_data ) ;
			my_data = NULL;
			res_ptr = NULL;
			if (!this->Init())
			{
				// Failed to open database, retry every 5 seconds
				this->MSleep(5000);
			}
			else
			{
				connection_okay = true;
			}
		}

		if (kill_thread)
		{
			break;
		}
	}

	if (res_ptr) mysql_free_result( res_ptr ) ;
	if (my_data) mysql_close( my_data ) ;
	my_data = NULL;
	res_ptr = NULL;

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
void *ManiMySQLThread::CheckRequests(void)
{
	if (first == NULL)
	{
		return NULL;
	}

	for (request_list_t *i = first; i != NULL; i = i->next_ptr)
	{
		if (i->request.status == REQUEST_RELEASED)
		{
			return &(i->request);
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Here we pick up a request from the list and execute it
//---------------------------------------------------------------------------------
void ManiMySQLThread::ProcessRequestList(request_t *orig_request_ptr)
{
	bool error_flag = false;

	request_t	local_request;
	request_t	*request_ptr;

	// Copy to local structure then release mutex. Need to do all this so we can release the mutex lock
	local_request = *orig_request_ptr;
	request_ptr = &local_request;

	request_ptr->sql_list = (sql_format_t *) malloc(sizeof(sql_format_t) * orig_request_ptr->sql_list_size);
	if (!orig_request_ptr->sql_list)
	{
		// We are screwed, no memory
		return;
	}

	for (int i = 0; i < request_ptr->sql_list_size; i++)
	{
		request_ptr->sql_list[i] = orig_request_ptr->sql_list[i];
	}

	request_ptr->status = REQUEST_INPROGRESS;

	this->UnlockMutex(true);

	for (int sql_index = 0; sql_index < request_ptr->sql_list_size; sql_index++)
	{
		if (request_ptr->sql_list[sql_index].status != REQUEST_RELEASED)
		{
			continue;
		}

		// Okay we've found our SQL index for this request
		// Let's run the sql for it.

		sql_format_t *sql_ptr = &(request_ptr->sql_list[sql_index]);

		sql_ptr->status = REQUEST_INPROGRESS;

		if (res_ptr)
		{
			mysql_free_result( res_ptr ) ;
			res_ptr = NULL;
		}

		if (mysql_query( my_data, sql_ptr->sql_string ) != 0)
		{
			// Call failed
			this->ProcessRequestListError(sql_ptr);
			error_flag = true;
			sql_ptr->status = REQUEST_ERROR;
			break;
		}

		res_ptr = mysql_store_result( my_data );

		// Get row id if needed
		if (sql_ptr->row_id)
		{
			sql_ptr->row_id = mysql_insert_id(my_data);
		}

		int	row_count;

		if (sql_ptr->get_rows != -1)
		{
			if (res_ptr)
			{
				row_count = (int) mysql_num_rows( res_ptr ); 
			}
			else
			{
				row_count = -1;
			}

			if (row_count > 0)
			{
				int	rows_to_get;

				if (sql_ptr->get_rows == 0)
				{
					// Get all rows
					rows_to_get = row_count;
				}
				else
				{
					// Get limited number of rows
					rows_to_get = sql_ptr->get_rows;
				}

				// Alloc number of rows needed
				sql_ptr->row_list = (row_t *) malloc(sizeof(row_t *) * rows_to_get);

				if (sql_ptr->row_list)
				{
					sql_ptr->row_list_size = rows_to_get;

					for (int m = 0; m < rows_to_get; m ++)
					{
						sql_ptr->row_list[m].row = NULL;
					}

					int i = 0;
					while ((row = mysql_fetch_row(res_ptr)) != NULL)
					{
						sql_ptr->num_fields  = mysql_num_fields(res_ptr);
						// Allocate number of char pointers in the list we need
						sql_ptr->row_list[i].row = (MYSQL_ROW) malloc(sizeof(char *) * sql_ptr->num_fields);
						if (sql_ptr->row_list[i].row == NULL)
						{
							// Failed
							sql_ptr->status = REQUEST_ERROR;
							error_flag = true;
							break;
						}

						for (int j = 0; j < sql_ptr->num_fields; j++)
						{
							int	field_size = (strlen(row[j]) + 1) * sizeof(char);

							sql_ptr->row_list[i].row[j] = (char *) malloc(field_size);
							if (!sql_ptr->row_list[i].row[j])
							{
								sql_ptr->status = REQUEST_ERROR;
								error_flag = true;
								break;
							}

							strcpy(sql_ptr->row_list[i].row[j], row[j]);
						}

						if (error_flag)
						{
							break;
						}

						i++;
						if (rows_to_get == i)
						{
							break;
						}
					}
				}
			}
		}

		if (error_flag)
		{
			break;
		}
		sql_ptr->status = REQUEST_COMPLETE;
	}

	if (error_flag == true)
	{
		request_ptr->status = REQUEST_ERROR;
	}
	else
	{
		request_ptr->status = REQUEST_COMPLETE;
	}

	// Get the mutex again so we can copy back to the original 
	while(!this->LockMutex(true))
	{
		this->MSleep(50);
	}

	orig_request_ptr->status = request_ptr->status;

	// Loop through all SQL statements for this request
	for (int i = 0; i < orig_request_ptr->sql_list_size; i++)
	{
		sql_format_t *orig_sql_ptr = &(orig_request_ptr->sql_list[i]);
		sql_format_t *sql_ptr = &(request_ptr->sql_list[i]);

		orig_sql_ptr->error_code = sql_ptr->error_code;
		if (sql_ptr->error_string)
		{
			orig_sql_ptr->error_string = (char *) malloc((strlen(sql_ptr->error_string) + 1) * sizeof(char));
			strcpy(orig_sql_ptr->error_string, sql_ptr->error_string);
			free (sql_ptr->error_string);
			sql_ptr->error_string = NULL;
		}

		orig_sql_ptr->row_id = sql_ptr->row_id;
		orig_sql_ptr->row_list_size = sql_ptr->row_list_size;
		orig_sql_ptr->num_fields = sql_ptr->num_fields;
		orig_sql_ptr->status = sql_ptr->status;

		if (sql_ptr->row_list_size == 0)
		{
			sql_ptr->row_list = NULL;
		}
		else
		{
			// Create as many rows as we need
			orig_sql_ptr->row_list = (row_t *) malloc(sizeof(row_t) * sql_ptr->row_list_size);
			if (orig_sql_ptr->row_list == NULL)
			{
				break;
			}

			// For each row, create the number of fields
			for (int j = 0; j < sql_ptr->row_list_size; j ++)
			{
				orig_sql_ptr->row_list[j].row = (MYSQL_ROW) malloc(sizeof(char *) * sql_ptr->num_fields);
				if (orig_sql_ptr->row_list[j].row == NULL)
				{
					break;
				}

				// Copy each field in
				for (int k = 0; k < sql_ptr->num_fields; k ++)
				{
					orig_sql_ptr->row_list[j].row[k] = (char *) malloc(sizeof(char) * (strlen(sql_ptr->row_list[j].row[k]) + 1));
					if (orig_sql_ptr->row_list[j].row[k] == NULL)
					{
						break;
					}

					// Finally copy the string
					strcpy(orig_sql_ptr->row_list[j].row[k], sql_ptr->row_list[j].row[k]);
					free(sql_ptr->row_list[j].row[k]);
				}
			
				free(sql_ptr->row_list[j].row);
			}
				
			free(sql_ptr->row_list);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Create a connection to a database
//---------------------------------------------------------------------------------
void ManiMySQLThread::ProcessRequestListError(sql_format_t *sql_ptr)
{
	sql_ptr->error_code = mysql_errno(my_data);
	sql_ptr->error_string = (char *) malloc((strlen(mysql_error(my_data)) + 1) * sizeof(char));
	strcpy(sql_ptr->error_string, mysql_error(my_data));
	sql_ptr->status = REQUEST_ERROR;
	return;
}


//---------------------------------------------------------------------------------
// Purpose: Create a connection to a database
//---------------------------------------------------------------------------------
bool ManiMySQLThread::Init(void)
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
		MMsg("Failed to init database\n");
		return false;
	}

	if (mysql_options(my_data, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &timeout))
	{
		MMsg("mysql_options failed !!\n");
		Msg( "%s\n", mysql_error(my_data));
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
		Msg( "mysql_real_connect failed !\n") ;
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}
	

	if ( mysql_select_db( my_data, gpManiDatabase->GetDBName()) != 0 ) 
	{
		error_code = mysql_errno(my_data);
		Msg( "Can't select the %s database !\n", gpManiDatabase->GetDBName() ) ;
		Msg( "%s\n", mysql_error(my_data));
		mysql_close( my_data ) ;
		my_data = NULL;
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Add a node onto the end
//---------------------------------------------------------------------------------
void    ManiMySQLThread::AddRequestStruct(struct	request_list_t *node_ptr)
{
	if (first == NULL)
	{
		first = node_ptr;
		last = node_ptr;
		node_ptr->next_ptr = NULL;
		node_ptr->prev_ptr = NULL;
	}
	else
	{
		node_ptr->prev_ptr = last;
		last->next_ptr = node_ptr;
		last = node_ptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Remove a node from the system
//---------------------------------------------------------------------------------
void    ManiMySQLThread::RemoveRequestStruct(struct request_list_t *node_ptr)
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
// Purpose: Free all the contents of the node
//---------------------------------------------------------------------------------
void    ManiMySQLThread::FreeRequest(struct request_list_t *request_list_ptr)
{
	request_t *request_ptr;

	request_ptr = &(request_list_ptr->request);

	// Kill args
	if (request_ptr->argc != 0)
	{
		for (int i = 0; i < request_ptr->argc; i ++)
		{
			free(request_ptr->argv[i]);
		}
	}

	for (int i = 0; i < request_ptr->sql_list_size; i ++)
	{
		sql_format_t *sql_ptr = &(request_ptr->sql_list[i]);

		for (int j = 0; j < sql_ptr->row_list_size; j ++)
		{
			for (int k = 0; k < sql_ptr->num_fields; k ++)
			{
				free(sql_ptr->row_list[j].row[k]);
			}
		}

		if (sql_ptr->row_list_size)
		{
			free(sql_ptr->row_list);
		}

		free(sql_ptr->sql_string);
		if (sql_ptr->error_string)
		{
			free(sql_ptr->error_string);
		}
	}

	if (request_ptr->sql_list_size)
	{
		free(request_ptr->sql_list);
	}

	free(request_list_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Free all the list
//---------------------------------------------------------------------------------
void    ManiMySQLThread::RemoveAllRequests(void)
{
	request_list_t *current_ptr;
	request_list_t *next_ptr;

	if (first == NULL) return;

	current_ptr = first;

	while(current_ptr != NULL)
	{
		next_ptr = current_ptr->next_ptr;
		FreeRequest(current_ptr);
		current_ptr = next_ptr;
	}

	first = NULL;
	last = NULL;

}

//---------------------------------------------------------------------------------
// Purpose: AddRequest
//---------------------------------------------------------------------------------
void	ManiMySQLThread::AddRequest
(
 int	user_id,
 request_list_t **request_list_ptr
)
{
	*request_list_ptr = (request_list_t *) malloc(sizeof(request_list_t));
	if (*request_list_ptr == NULL)
	{
		*request_list_ptr = NULL;
		return;
	}

	request_list_t *temp_ptr = *request_list_ptr;

	request_t *request_ptr = &(temp_ptr->request);

	request_ptr->argc = 0;
	request_ptr->argv = NULL;
	request_ptr->status = REQUEST_BUILDING;
	request_ptr->user_id = user_id;
	request_ptr->sql_list = NULL;
	request_ptr->sql_list_size = 0;

	temp_ptr->next_ptr = NULL;
	temp_ptr->prev_ptr = NULL;

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Post the request onto the list
//---------------------------------------------------------------------------------
void	ManiMySQLThread::PostRequest
(
 request_list_t *request_list_ptr
)
{
	// Add it to the end of the linked list (need mutex for it)
	this->LockMutexWait(false);

	// Add to the linked list
	request_list_ptr->request.status = REQUEST_RELEASED;
	AddRequestStruct(request_list_ptr);

	// Release Mutex
	this->UnlockMutex(false);
}


//---------------------------------------------------------------------------------
// Purpose: AddSQL (full version)
//---------------------------------------------------------------------------------
void	ManiMySQLThread::AddSQL(request_list_t *request_list_ptr, bool get_row_id, const char *fmt, ...)
{
	request_t *request_ptr = &(request_list_ptr->request);
	sql_format_t *temp_ptr;

	// Add one to list size

	// Is list empty ?
	if (request_ptr->sql_list == NULL)
	{
		request_ptr->sql_list = (sql_format_t *) malloc (sizeof(sql_format_t));
		if (request_ptr->sql_list == NULL)
		{
			// Screwed
			return;
		}

		request_ptr->sql_list_size = 1;
	}
	else
	{
		// List already created, use realloc to add one
		temp_ptr = (sql_format_t *) realloc(request_ptr->sql_list, (request_ptr->sql_list_size + 1) * sizeof(sql_format_t));
		if (temp_ptr == NULL)
		{
			// Realloc failed !!
			return;
		}

		request_ptr->sql_list_size ++;
		request_ptr->sql_list = temp_ptr;
	}

	// Get handy pointer
	temp_ptr = &(request_ptr->sql_list[request_ptr->sql_list_size - 1]);

	temp_ptr->error_code = 0;
	temp_ptr->error_string = NULL;
	temp_ptr->get_rows = -1;
	temp_ptr->num_fields = 0;
	temp_ptr->row_list = NULL;
	temp_ptr->row_list_size = 0;
	temp_ptr->status = REQUEST_RELEASED;

	// Process SQL statement
	va_list		argptr;
	char		temp_string[4096];

	va_start ( argptr, fmt );
#ifndef __linux__
	int length = _vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#else
	int length = vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#endif	
	va_end   ( argptr );

	if (length <= 0) return;

	length ++;
	temp_ptr->sql_string = (char *) malloc(length * sizeof(char));
	strcpy(temp_ptr->sql_string, temp_string);
	temp_ptr->row_id = get_row_id;
}

//---------------------------------------------------------------------------------
// Purpose: AddSQL (full version)
//---------------------------------------------------------------------------------
void	ManiMySQLThread::AddSQL(request_list_t *request_list_ptr, int	max_rows, const char *fmt, ...)
{
	request_t *request_ptr = &(request_list_ptr->request);
	sql_format_t *temp_ptr;

	// Add one to list size

	// Is list empty ?
	if (request_ptr->sql_list == NULL)
	{
		request_ptr->sql_list = (sql_format_t *) malloc (sizeof(sql_format_t));
		if (request_ptr->sql_list == NULL)
		{
			// Screwed
			return;
		}

		request_ptr->sql_list_size = 1;
	}
	else
	{
		// List already created, use realloc to add one
		temp_ptr = (sql_format_t *) realloc(request_ptr->sql_list, (request_ptr->sql_list_size + 1) * sizeof(sql_format_t));
		if (temp_ptr == NULL)
		{
			// Realloc failed !!
			return;
		}

		request_ptr->sql_list_size ++;
		request_ptr->sql_list = temp_ptr;
	}

	// Get handy pointer
	temp_ptr = &(request_ptr->sql_list[request_ptr->sql_list_size - 1]);

	temp_ptr->error_code = 0;
	temp_ptr->error_string = NULL;
	temp_ptr->get_rows = max_rows;
	temp_ptr->num_fields = 0;
	temp_ptr->row_list = NULL;
	temp_ptr->row_list_size = 0;
	temp_ptr->status = REQUEST_RELEASED;

	// Process SQL statement
	va_list		argptr;
	char		temp_string[4096];

	va_start ( argptr, fmt );
#ifndef __linux__
	int length = _vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#else
	int length = vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#endif	
	va_end   ( argptr );

	if (length <= 0) return;

	length ++;
	temp_ptr->sql_string = (char *) malloc(length * sizeof(char));
	strcpy(temp_ptr->sql_string, temp_string);
	temp_ptr->row_id = 0;
}

//---------------------------------------------------------------------------------
// Purpose: AddSQL Just sql statement, no rows
//---------------------------------------------------------------------------------
void	ManiMySQLThread::AddSQL(request_list_t *request_list_ptr, const char *fmt, ...)
{
	request_t *request_ptr = &(request_list_ptr->request);
	sql_format_t *temp_ptr;

	// Add one to list size

	// Is list empty ?
	if (request_ptr->sql_list == NULL)
	{
		request_ptr->sql_list = (sql_format_t *) malloc (sizeof(sql_format_t));
		if (request_ptr->sql_list == NULL)
		{
			// Screwed
			return;
		}

		request_ptr->sql_list_size = 1;
	}
	else
	{
		// List already created, use realloc to add one
		temp_ptr = (sql_format_t *) realloc(request_ptr->sql_list, (request_ptr->sql_list_size + 1) * sizeof(sql_format_t));
		if (temp_ptr == NULL)
		{
			// Realloc failed !!
			return;
		}

		request_ptr->sql_list_size ++;
		request_ptr->sql_list = temp_ptr;
	}

	// Get handy pointer
	temp_ptr = &(request_ptr->sql_list[request_ptr->sql_list_size - 1]);

	temp_ptr->error_code = 0;
	temp_ptr->error_string = NULL;
	temp_ptr->get_rows = -1;
	temp_ptr->num_fields = 0;
	temp_ptr->row_list = NULL;
	temp_ptr->row_list_size = 0;
	temp_ptr->status = REQUEST_RELEASED;

	// Process SQL statement
	va_list		argptr;
	char		temp_string[4096];

	va_start ( argptr, fmt );
#ifndef __linux__
	int length = _vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#else
	int length = vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
#endif	
	va_end   ( argptr );

	if (length <= 0) return;

	length ++;
	temp_ptr->sql_string = (char *) malloc(length * sizeof(char));
	strcpy(temp_ptr->sql_string, temp_string);
	temp_ptr->row_id = 0;
}

//---------------------------------------------------------------------------------
// Purpose: GameFrame to parse results (main process calls this)
//---------------------------------------------------------------------------------
void	ManiMySQLThread::GameFrame(void)
{
	if (war_mode) return;
	if (first == NULL) return; // Nothing to check
	if (!accept_requests) return;

	// Get Mutex, quit if it can't get it
	if (thread_alive)
	{
		if (!this->LockMutex(false)) return;
	}

	for (request_list_t *i = first; i != NULL; i = i->next_ptr)
	{
		if (i->request.status == REQUEST_COMPLETE)
		{
			this->ProcessRequestComplete(i);
		}
		else
		if (i->request.status == REQUEST_ERROR)
		{
			this->ProcessRequestFailed(i);
		}
	}

	// Release Mutex
	if (thread_alive)
	{
		this->UnlockMutex(false);
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
void	ManiMySQLThread::ProcessRequestComplete(request_list_t *request_list_ptr)
{
	request_t *request_ptr = &(request_list_ptr->request);
	bool	svr_command = false;
	player_t	player;

	player.entity = NULL;

	// Get our destination output
	if (request_ptr->user_id == 0 || request_ptr->user_id == -1)
	{
		svr_command = true;
	}
	else
	{
		player.user_id = request_ptr->user_id;
		if (!FindPlayerByUserID(&player))
		{
			svr_command = true;
		}
	}

	for (int i = 0; i < request_ptr->sql_list_size; i ++)
	{
		if (request_ptr->sql_list[i].status == REQUEST_COMPLETE)
		{
			if (svr_command)
			{
				OutputHelpText(ORANGE_CHAT, NULL, "SUCCESS %s", request_ptr->sql_list[i].sql_string);
			}
			else
			{
				OutputHelpText(ORANGE_CHAT, &player, "SUCCESS %s", request_ptr->sql_list[i].sql_string);
			}
			
			char	sql_out[4096];
			for (int j = 0; j < request_ptr->sql_list[i].row_list_size; j ++)
			{
				Q_strcpy(sql_out,"");

				for (int k = 0; k < request_ptr->sql_list[i].num_fields; k ++)
				{
					strcat(sql_out, request_ptr->sql_list[i].row_list[j].row[k]);
					strcat(sql_out, " ");
				}

				if (svr_command)
				{
					OutputHelpText(ORANGE_CHAT, NULL, "%s", sql_out);
				}
				else
				{
					OutputHelpText(ORANGE_CHAT, &player, "%s", sql_out);
				}
			}
		}
	}

	this->RemoveRequestStruct(request_list_ptr);
	this->FreeRequest(request_list_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Process a failed result
//---------------------------------------------------------------------------------
void	ManiMySQLThread::ProcessRequestFailed(request_list_t *request_list_ptr)
{
	request_t *request_ptr = &(request_list_ptr->request);
	bool	svr_command = false;
	player_t	player;

	player.entity = NULL;

	// Get our destination output
	if (request_ptr->user_id == 0 || request_ptr->user_id == -1)
	{
		svr_command = true;
	}
	else
	{
		player.user_id = request_ptr->user_id;
		if (!FindPlayerByUserID(&player))
		{
			svr_command = true;
		}
	}

	for (int i = 0; i < request_ptr->sql_list_size; i ++)
	{
		if (request_ptr->sql_list[i].status == REQUEST_ERROR)
		{
			if (svr_command)
			{
				OutputHelpText(ORANGE_CHAT, NULL, "%s", request_ptr->sql_list[i].error_string);
			}
			else
			{
				OutputHelpText(ORANGE_CHAT, &player, "%s", request_ptr->sql_list[i].error_string);
			}
		}
	}

	this->RemoveRequestStruct(request_list_ptr);
	this->FreeRequest(request_list_ptr);
}

//---------------------------------------------------------------------------------
// Purpose: Sleep for time period
//---------------------------------------------------------------------------------
void	ManiMySQLThread::MSleep(int	milliseconds)
{
#ifndef __linux__
	// Windows version
	Sleep(milliseconds);
#else
	// Linux version
	if (milliseconds > 9999)
	{
		sleep(milliseconds / 1000);
	}
	else
	{	
		usleep(milliseconds * 1000);
	}
#endif
}
	
//---------------------------------------------------------------------------------
// Purpose: Release the mutex
//---------------------------------------------------------------------------------
void	ManiMySQLThread::UnlockMutex(bool thread_call)
{
#ifdef MANI_SHOW_LOCKS
	MMsg("UnlockMutex Thread Called [%s]\n", (thread_call) ? "TRUE":"FALSE");
#endif

#ifndef __linux__
	// Windows version
	ReleaseMutex(request_list_mutex);
#else
	// Linux version
	while(pthread_mutex_unlock(&request_list_mutex) != 0);
#endif

#ifdef MANI_SHOW_LOCKS
	MMsg("UNLOCKED\n");
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Lock the Mutext with timeout (milliseconds)
//---------------------------------------------------------------------------------
void	ManiMySQLThread::LockMutexWait(bool thread_call)
{
#ifdef MANI_SHOW_LOCKS
	MMsg("LockMutexWait Thread Called [%s]\n", (thread_call) ? "TRUE":"FALSE");
#endif

#ifndef __linux__
	// Windows version
	while(WaitForSingleObject( request_list_mutex, 0) != WAIT_OBJECT_0);
#else
	// Linux version
	while(pthread_mutex_trylock(&request_list_mutex) != 0);

#endif
}

//---------------------------------------------------------------------------------
// Purpose: Lock the Mutext with timeout
//---------------------------------------------------------------------------------
bool	ManiMySQLThread::LockMutex(bool thread_call)
{
	bool status = false;

#ifdef MANI_SHOW_LOCKS
	MMsg("LockMutex Thread Called [%s]\n", (thread_call) ? "TRUE":"FALSE");
#endif

#ifndef __linux__
	// Windows version
	if (WaitForSingleObject( request_list_mutex, 0) == WAIT_OBJECT_0)
	{
		status = true;
	}

	
#else
	// Linux version
	if (pthread_mutex_trylock(&request_list_mutex) == 0)
	{
		status = true;
	}

#endif

#ifdef MANI_SHOW_LOCKS
	if (status)
	{
		MMsg("GOT LOCK\n");
	}
	else
	{
		MMsg("LOCK FAILED\n");
	}
#endif
	return status;
}

ManiMySQLThread *mysql_thread=NULL;
