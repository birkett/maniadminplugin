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
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_player.h"
#include "mani_parser.h"
#include "mani_crontab.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;

extern	bool war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

// Non global
static	crontab_t	*crontab_list = NULL;
static	int			crontab_list_size = 0;

static bool	GetIntTime(char *time_section, int *time_value);
static bool	ShallWeExecute(crontab_t *crontab, struct tm *time_now);
static bool	WithinHours(int start_hour, int end_hour, int current_hour);

//---------------------------------------------------------------------------------
// Purpose: Load the cron tabs into memory
//---------------------------------------------------------------------------------
void	LoadCronTabs(void)
{
	FileHandle_t file_handle;
	char	time_string[512];
	char	day_string[512];
	char	server_command[512];
	char	base_filename[256];
	char	time_section[3];
	int		time_value;

	crontab_t	crontab;

	FreeCronTabs();

	//Get rcon list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/crontablist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load crontablist.txt\n");
	}
	else
	{
//		MMsg("Crontab list\n");
		while (filesystem->ReadLine (server_command, sizeof(server_command), file_handle) != NULL)
		{
			if (!ParseAliasLine2(server_command, day_string, time_string, true, false))
			{
				// String is empty after parsing
				continue;
			}

			bool all_days = false;

			if (FStrEq(day_string,""))
			{
				// Day string empty so mark as all days 
				for (int i = 0; i < 7; i++)
				{
					crontab.days_of_week[i] = true;
				}

				all_days = true;
			}
			else
			{
				// Day string empty so mark as all days 
				for (int i = 0; i < 7; i++)
				{
					crontab.days_of_week[i] = false;
				}

				for (int i = 0; i < Q_strlen(day_string); i++)
				{
					int	day_of_week = 0;
					char single_day[2];

					single_day[0] = day_string[i];
					single_day[1] = '\0';

					day_of_week = atoi(single_day);
					if (day_of_week < 1 || day_of_week > 7)
					{
						continue;
					}

					day_of_week --;
					crontab.days_of_week[day_of_week] = true;
				}
			}

			// Format of time string must be HH24:MI-HH24:MI
			if (Q_strlen(time_string) < 11) continue;

			time_section[0] = time_string[0]; // First hours
			time_section[1] = time_string[1];
			time_section[2] = '\0';
			if (!GetIntTime(time_section, &time_value)) continue;
			crontab.start_hour = time_value;

			time_section[0] = time_string[3]; // First minutes
			time_section[1] = time_string[4];
			if (!GetIntTime(time_section, &time_value)) continue;
			crontab.start_minute = time_value;

			time_section[0] = time_string[6]; // Second hours
			time_section[1] = time_string[7];
			time_section[2] = '\0';
			if (!GetIntTime(time_section, &time_value)) continue;
			crontab.end_hour = time_value;

			time_section[0] = time_string[9]; // Second minutes
			time_section[1] = time_string[10];
			if (!GetIntTime(time_section, &time_value)) continue;
			crontab.end_minute = time_value;

			Q_strcpy(crontab.server_command, server_command);

			AddToList((void **) &crontab_list, sizeof(rcon_t), &crontab_list_size);
			crontab_list[crontab_list_size - 1] = crontab;

			if (all_days)
			{
//				MMsg("[ALL DAYS] %02d:%02d to %02d:%02d [%s]\n", 
//								crontab.start_hour,
//								crontab.start_minute,
//								crontab.end_hour,
//								crontab.end_minute,
//								crontab.server_command);
			}
			else
			{
//				MMsg("Days [%s] %02d:%02d to %02d:%02d [%s]\n", 
//								day_string,
//								crontab.start_hour,
//								crontab.start_minute,
//								crontab.end_hour,
//								crontab.end_minute,
//								crontab.server_command);
			}
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free Cron Tab lists
//---------------------------------------------------------------------------------
void	FreeCronTabs(void)
{
	FreeList((void **) &crontab_list, &crontab_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Free Cron Tab lists
//---------------------------------------------------------------------------------
static
bool	GetIntTime(char *time_section, int *time_value)
{
	for (int i = 0; i < Q_strlen(time_section); i++)
	{
		if (isdigit(time_section[i]) == 0) return false;
	}

	*time_value = atoi(time_section);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Execute any cron tabs for this time
//---------------------------------------------------------------------------------
void	ExecuteCronTabs(bool post_map_config)
{
	struct	tm	*time_now;
	time_t	current_time;

	if (war_mode) return;

	time(&current_time);

	current_time += (time_t) (mani_adjust_time.GetInt() * 60);
	time_now = localtime(&current_time);

	for (int i = 0; i < crontab_list_size; i ++)
	{
		char *server_command;
		char final_server_command[512];

		if (!crontab_list[i].days_of_week[time_now->tm_wday]) continue;

		if (crontab_list[i].server_command[0] == '#' && post_map_config)
		{
			server_command = &(crontab_list[i].server_command[1]);
		}
		else if (crontab_list[i].server_command[0] != '#' && !post_map_config)
		{
			server_command = &(crontab_list[i].server_command[0]);
		}
		else
		{
			continue;
		}

		if (!ShallWeExecute(&(crontab_list[i]), time_now)) continue;

		snprintf(final_server_command, sizeof(final_server_command), "%s\n", server_command);
		engine->ServerCommand(final_server_command);
		MMsg("Executed crontab server command [%s]", final_server_command);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Check if time falls within parameters
//---------------------------------------------------------------------------------
static
bool	ShallWeExecute(crontab_t *crontab, struct tm *time_now)
{

	if (!WithinHours(crontab->start_hour, crontab->end_hour, time_now->tm_hour))
	{
		return false;
	}

	if (crontab->start_hour == time_now->tm_hour)
	{
		// Need to check minutes portion of time
		if (crontab->start_minute > time_now->tm_min)
		{
			return false;
		}
	}
	
	if (crontab->end_hour == time_now->tm_hour)
	{
		// Need to check minutes portion of time
		if (crontab->end_minute < time_now->tm_min)
		{
			return false;
		}
	}

	return true;

}

//---------------------------------------------------------------------------------
// Purpose: Check if hours are within current hour
//---------------------------------------------------------------------------------
static
bool	WithinHours(int start_hour, int end_hour, int current_hour)
{
	if (start_hour <= end_hour)
	{
		// Start time less than end time

		// Check we are within hours specified
		if (start_hour <= current_hour && end_hour >= current_hour)
		{
			return true;
		}

		// Not in hour range
		return false;
	}

	// Start and end hours span over midnight ie
	// start hour might be 22:00, end hour might be 03:00

	if (current_hour >= start_hour) return true;
	if (current_hour <= end_hour) return true;

	return false;
}
