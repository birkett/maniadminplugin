//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_MUTEX_H
#define MANI_MUTEX_H

class Mutex
{
public:
	Mutex() {};
	~Mutex() {};
	bool	Create();
	void	Destroy();
	void    Unlock();
	void    LockWait();
	bool    Lock();

private:

#ifndef __linux__
	HANDLE		mutex;
#else
	pthread_mutex_t mutex;
#endif

};

#endif
