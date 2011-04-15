//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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
#include "mani_mutex.h"

// Small class for handling mutexes (win32 & linux)

#ifndef __linux__
static int	number_mutex = 0;
#endif

//---------------------------------------------------------------------------------
// Purpose: Create a mutex
//---------------------------------------------------------------------------------
bool	Mutex::Create()
{
#ifndef __linux__
	char mutex_id[128];

	_snprintf_s( mutex_id, _countof(mutex_id), 128, "mutex_%i", number_mutex++);
	//_snprintf(mutex_id, sizeof(mutex_id), "mutex_%i", number_mutex++);
	mutex = CreateMutex( 
			NULL,      // no security attributes
			FALSE,     // initially not owned
			mutex_id);  // name of mutex

	if (mutex == NULL) 
	{
		// Check for error.
		return false;
	}
#else
	if (pthread_mutex_init (&mutex, NULL) != 0)
	{
		return false;
	}
#endif

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Create a mutex
//---------------------------------------------------------------------------------
void	Mutex::Destroy()
{
#ifndef __linux__
	CloseHandle(mutex);
#else
	while(pthread_mutex_unlock(&mutex) != 0);
	pthread_mutex_destroy(&mutex);
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Release the mutex
//---------------------------------------------------------------------------------
void	Mutex::Unlock()
{
#ifndef __linux__
	// Windows version
	ReleaseMutex(mutex);
#else
	// Linux version
	while(pthread_mutex_unlock(&mutex) != 0);
#endif
}

//---------------------------------------------------------------------------------
// Purpose: Lock the Mutext with timeout (milliseconds)
//---------------------------------------------------------------------------------
void	Mutex::LockWait()
{
#ifndef __linux__
	// Windows version
	while(WaitForSingleObject( mutex, 0) != WAIT_OBJECT_0);
#else
	// Linux version
	while(pthread_mutex_trylock(&mutex) != 0);

#endif
}

//---------------------------------------------------------------------------------
// Purpose: Lock the Mutex with timeout
//---------------------------------------------------------------------------------
bool	Mutex::Lock()
{
	bool status = false;

#ifndef __linux__
	// Windows version
	if (WaitForSingleObject( mutex, 0) == WAIT_OBJECT_0)
	{
		status = true;
	}
#else
	// Linux version
	if (pthread_mutex_trylock(&mutex) == 0)
	{
		status = true;
	}
#endif

	return status;
}

