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



#ifndef MANI_MYSQL_H
#define MANI_MYSQL_H

#ifndef __linux__
#include <winsock.h>
#endif
#include <mysql.h>

class ManiMySQL
{

public:
	ManiMySQL();
	~ManiMySQL();

	bool	Init(void);
	bool	ExecuteQuery(  char	*sql_query, ... );
	bool	ExecuteQuery(  int	*row_count, char	*sql_query, ... );
	bool	FetchRow( void);
	int		GetInt(int column) {return (atoi(row[column]));}
	float	GetFloat(int column) {return (atof(row[column]));}
	char	*GetString(int column) {return (row[column]);}
	bool	GetBool(int column) {return ((atoi(row[column]) == 0) ? false:true);}
	int		GetRowID(void);

private:

	MYSQL			*my_data;
	MYSQL_RES		*res_ptr;
	MYSQL_FIELD		*fd;
	MYSQL_ROW		row;
	int			error_code;
	int			timer_id;
};

#endif
