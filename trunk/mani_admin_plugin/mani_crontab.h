//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_CRONTAB_H
#define MANI_CRONTAB_H

struct crontab_t
{
	int		start_hour;
	int		start_minute;
	int		end_hour;
	int		end_minute;
	char	server_command[512];
	bool	days_of_week[8];
};

extern	void	LoadCronTabs(void);
extern	void	ExecuteCronTabs(bool post_map_config);
extern	void	FreeCronTabs(void);

#endif
