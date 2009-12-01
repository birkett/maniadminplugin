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



#ifndef MANI_CLIENT_SQL_H
#define MANI_CLIENT_SQL_H

#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include "mani_output.h"
#include "mani_client_util.h"
#ifndef __linux__
#include <winsock.h>
#else
#include <pthread.h>
#endif
#include <mysql.h>
#include "mani_mutex.h"


//************************************************************************
// SQLProcessBlock - A block of code for specific client sql requests
// that are run in the back ground. virtual ProcessBlock is different for 
// each process block derived type.
//************************************************************************
class SQLManager;

class SQLProcessBlock
{
public:

	SQLProcessBlock() {completed = false;}
	~SQLProcessBlock() {}
	virtual void ProcessBlock(SQLManager *manager) = 0;
	bool	IsComplete() {return completed;}
	void	SetCompleted(bool complete) {completed = complete;}

	ParamManager in_params;		// Parameters for input
	ParamManager out_params;	// Parameters for output

private:
	bool completed;
};

// Short macro to define lots of derived classes from SQLProcessBlock
#define CSQLBLOCK_DEC(name_) \
class name_ : public SQLProcessBlock {void ProcessBlock(SQLManager *manager);}

CSQLBLOCK_DEC(SQLAddClient);
CSQLBLOCK_DEC(SQLRemoveClient);
CSQLBLOCK_DEC(SQLAddSteam);
CSQLBLOCK_DEC(SQLAddIPAddress);
CSQLBLOCK_DEC(SQLAddNick);
CSQLBLOCK_DEC(SQLRemoveSteam);
CSQLBLOCK_DEC(SQLRemoveIPAddress);
CSQLBLOCK_DEC(SQLRemoveNick);
CSQLBLOCK_DEC(SQLSetName);
CSQLBLOCK_DEC(SQLSetNotes);
CSQLBLOCK_DEC(SQLSetPassword);
CSQLBLOCK_DEC(SQLSetEmailAddress);
CSQLBLOCK_DEC(SQLSetLevel);
CSQLBLOCK_DEC(SQLAddGroup);
CSQLBLOCK_DEC(SQLRemoveGroup);
CSQLBLOCK_DEC(SQLAddGroupType);
CSQLBLOCK_DEC(SQLRemoveGroupType);
CSQLBLOCK_DEC(SQLAddLevelType);
CSQLBLOCK_DEC(SQLRemoveLevelType);
CSQLBLOCK_DEC(SQLSetFlag);
CSQLBLOCK_DEC(SQLAddFlagDesc);

class SQLManager
{
public:
	struct request_list_t
	{
        SQLProcessBlock	*request;
        struct request_list_t	*next_ptr;
		struct request_list_t	*prev_ptr;
	};

	SQLManager (int thread_number, int sleep_time);
	~SQLManager ();
	bool	Load();
	void	Unload();
	void	GameFrame();
	void	AddRequest(SQLProcessBlock *ptr);

	// SQL Specific commands
	bool	ExecuteQuery(  char	*sql_query, ... );
	bool	ExecuteQuery(  int	*row_count, char	*sql_query, ... );
	bool	FetchRow( void);
	int		GetInt(int column) {return atoi(((row[column] == NULL) ? "-1":row[column]));}
	float	GetFloat(int column) {return atof(((row[column] == NULL) ? "-1":row[column]));}
	char	*GetString(int column) {return ((row[column] == NULL) ? NULL:row[column]);}
	bool	GetBool(int column) 
	{
		if (row[column] == NULL) return false; 
		else 
		return (((row[column] == 0) ? false:true));
	}

	int		GetRowID(void);

private:
	// Private functions
	bool	GetConnection();
	SQLProcessBlock	 *CheckRequests(void);

#ifndef __linux__
	static  DWORD CALLBACK_ThreadFunc(LPVOID data);
	DWORD	RunThread(void);
#else
	static  void *CALLBACK_ThreadFunc(void *data);
	void	*RunThread(void);
#endif

	void	KillThread(void);

	void	ProcessRequestComplete(SQLProcessBlock *request);
	void    RemoveRequestStruct(struct request_list_t *node_ptr);
	void    RemoveAllRequests(void);	// Remove all the nodes
	void	LogSQL(const char *sql, ...);

	// Vars
#ifndef __linux__
	HANDLE		thread_id;
	HANDLE		request_list_mutex;
#else
	pthread_t	thread_id;
#endif

	MYSQL			*my_data;
	MYSQL_RES		*res_ptr;
	MYSQL_FIELD		*fd;
	MYSQL_ROW		row;
	int			error_code;
	int			timer_id;

	struct  request_list_t    *first;
	struct  request_list_t    *last;

	bool		accept_requests;
	bool		thread_alive;
	bool		kill_thread;
	int			thread_number;
	int			sleep_time;
	Mutex		mutex;
};

extern SQLManager *client_sql_manager;
#endif