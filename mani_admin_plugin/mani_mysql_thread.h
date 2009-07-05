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



#ifndef MANI_MYSQLTHREAD_H
#define MANI_MYSQLTHREAD_H

#ifndef __linux__
#include <winsock.h>
#else
#include <pthread.h>
#endif
#include <mysql.h>

#define REQUEST_RELEASED (0)
#define REQUEST_COMPLETE (1)
#define REQUEST_INPROGRESS (2)
#define REQUEST_BUILDING (3)
#define REQUEST_ERROR (4)

	struct	row_t
	{
		MYSQL_ROW	row;
	};

	struct	sql_format_t
	{
		char	*sql_string;
		int		error_code;
		char	*error_string;
		int		row_id;			// 0 = don't get row_id, 1 = get row id
		int		get_rows;		// -1 = don't check rows, 0 = get all rows
		row_t	*row_list;
		int		row_list_size;
		int		num_fields;
		int		status;
	};

	struct	request_t
	{
		int		user_id;	// User who requested it 0 = console, -1 = plugin
		char	**argv; // Arguments
		int		argc;	// No of arguments
		sql_format_t	*sql_list;
		int				sql_list_size;
		int				status;
	};

	struct request_list_t
	{
        request_t				request;
        struct request_list_t	*next_ptr;
		struct request_list_t	*prev_ptr;
	};

class ManiMySQLThread
{

public:
	ManiMySQLThread();
	~ManiMySQLThread();

	bool	Init(void);
	// Multi thread publics
	bool	Load(void);
	void	Unload(void);

	void	AddRequest(int	user_id, request_list_t **request_ptr);
	void	AddSQL(request_list_t *request_list_ptr, bool	get_row_id, const char *fmt, ...); 
	void	AddSQL(request_list_t *request_list_ptr, int	max_rows, const char *fmt, ...); 
	void	AddSQL(request_list_t *request_list_ptr, const char *fmt, ...); // Use for inserts, updates

	void	PostRequest(request_list_t *request_list_ptr);

	// Call from Plugin::GameFrame()
	void	GameFrame(void);
	void	ProcessRequestComplete(request_list_t *request_list_ptr);
	void	ProcessRequestFailed(request_list_t *request_list_ptr);

private:

	MYSQL			*my_data;
	MYSQL_RES		*res_ptr;
	MYSQL_FIELD		*fd;
	MYSQL_ROW		row;
	int			error_code;
	int			timer_id;



	struct  request_list_t    *first;
	struct  request_list_t    *last;

#ifndef __linux__
	static  DWORD CALLBACK_ThreadFunc(LPVOID data);
	DWORD	RunThread(void);
#else
	static  void *CALLBACK_ThreadFunc(void *data);
	void	*RunThread(void);
#endif

	// Mutex and sleep 
	void    MSleep(int   milliseconds);
	void    UnlockMutex (bool thread_call);
	void    LockMutexWait(bool thread_call);
	bool    LockMutex(bool thread_call);
	void	KillThread(void);

	void	ProcessRequestList(request_t *request_ptr);
	void	ProcessRequestListError(sql_format_t *sql_ptr);
	void	 *CheckRequests(void);

	void    AddRequestStruct(struct	request_list_t *node_ptr); // Add to tail
	void    RemoveRequestStruct(struct request_list_t *node_ptr);	// Remove a specific node
	void	FreeRequest(struct request_list_t *node_ptr); // Free up the memory for the request
	void    RemoveAllRequests(void);	// Remove all the nodes

#ifndef __linux__
	HANDLE		thread_id;
	HANDLE		request_list_mutex;
#else
	pthread_t	thread_id;
#endif

	bool		accept_requests;
	bool		thread_alive;
	bool		kill_thread;
	bool		connection_okay;
};

extern ManiMySQLThread *mysql_thread;

#endif
