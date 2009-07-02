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
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"

#include "mani_main.h"
#include "mani_util.h"
#include "mani_file.h"
extern	IVEngineServer	*engine;

extern	ConVar	*sv_lan;

/* These hash functions were taken from http://www.sparknotes.com/cs/searching/hashtables/section2.rhtml */

/* djb2
 * This algorithm was first reported by Dan Bernstein
 * many years ago in comp.lang.c
 */
unsigned long djb2_hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c; 
	while ((c = *str++) != '\0') hash = ((hash << 5) + hash) + c; // hash*33 + c
	return hash;
}

unsigned long djb2_hash(unsigned char *str1, unsigned char *str2)
{
	unsigned long hash = 5381;
	int c; 
	while ((c = *str1++) != '\0') hash = ((hash << 5) + hash) + c; // hash*33 + c
	while ((c = *str2++) != '\0') hash = ((hash << 5) + hash) + c; // hash*33 + c
	return hash;
}

/* UNIX ELF hash
 * Published hash algorithm used in the UNIX ELF format for object files
 */
unsigned long elf_hash(char *str)
{
	unsigned long h = 0, g;

	while ( *str ) {
		h = ( h << 4 ) + *str++;
		if ((g = h & 0xF0000000 ) != 0)
		h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

unsigned long elf_hash(char *str1, char *str2)
{
	unsigned long h = 0, g;

	while ( *str1 ) {
		h = ( h << 4 ) + *str1++;
		if (( g = h & 0xF0000000 ) != 0)
		h ^= g >> 24;
		h &= ~g;
	}

	while ( *str2 ) {
		h = ( h << 4 ) + *str2++;
		if ((g = h & 0xF0000000 ) != 0)
		h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

/* This algorithm was created for the sdbm (a reimplementation of ndbm) 
 * database library and seems to work relatively well in scrambling bits
 */
unsigned long sdbm_hash(unsigned char *str)
{
	unsigned long hash = 0;
	int c; 
	while ((c = *str++) != '\0') hash = c + (hash << 6) + (hash << 16) - hash;
	return hash;
}

unsigned long sdbm_hash(unsigned char *str1, unsigned char *str2)
{
	unsigned long hash = 0;
	int c; 
	while ((c = *str1++) != '\0') hash = c + (hash << 6) + (hash << 16) - hash;
	while ((c = *str2++) != '\0') hash = c + (hash << 6) + (hash << 16) - hash;
	return hash;
}

void	MSleep(int	milliseconds)
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

// Return true if server setup to be a lan
bool IsLAN()
{
	return ((sv_lan && sv_lan->GetInt() == 1) ? true:false);
}

char	*AsciiToHTML(char *in_string)
{
	static char out_string[16384]="";
	int	index = 0;
	int	out_index = 0;
	while(in_string[index] != '\0')
	{
		switch (in_string[index])
		{
		case '&' : 
			out_string[out_index++] = '&';
			out_string[out_index++] = 'a';
			out_string[out_index++] = 'm';
			out_string[out_index++] = 'p';
			out_string[out_index++] = ';';
			break;
		case '<' : 
			out_string[out_index++] = '&';
			out_string[out_index++] = 'l';
			out_string[out_index++] = 't';
			out_string[out_index++] = ';';
			break;
		case '>' : 
			out_string[out_index++] = '&';
			out_string[out_index++] = 'g';
			out_string[out_index++] = 't';
			out_string[out_index++] = ';';
			break;
		case '\"' : 
			out_string[out_index++] = '&';
			out_string[out_index++] = 'l';
			out_string[out_index++] = 'd';
			out_string[out_index++] = 'q';
			out_string[out_index++] = 'u';
			out_string[out_index++] = 'o';
			out_string[out_index++] = ';';
			break;
		case 0x0A : 
			out_string[out_index++] = '<';
			out_string[out_index++] = 'B';
			out_string[out_index++] = 'R';
			out_string[out_index++] = '>';
			out_string[out_index++] = '<';
			out_string[out_index++] = 'B';
			out_string[out_index++] = 'R';
			out_string[out_index++] = ' ';
			out_string[out_index++] = '/';
			out_string[out_index++] = '>';
			break;
		default:out_string[out_index++] = in_string[index];
		}

		if (out_index == sizeof(out_string) - 2)
		{
			break;
		}

		index ++;
	}

	out_string[out_index] = '\0';
	return out_string;
}

#ifdef PROFILING

void ValveTimer::Start()
{
	start = engine->Time();
}

float ValveTimer::Stop()
{
	return (engine->Time() - start);
}

Profile profile_list[MAX_PROFILES]=
{
"LanguageGameFrame()",
"gpManiNetIDValid->GameFrame()",
"client_sql_manager->GameFrame()",
"gpManiReservedSlot->GameFrame()",
"gpManiSprayRemove->GameFrame()",
"gpManiWarmupTimer->GameFrame()",
"gpManiAFK->GameFrame()",
"gpManiPing->GameFrame()",
"ProcessCheatCVars()",
"TurnOffOverviewMap()",
"gpManiStats->GameFrame()",
"gpManiTeam->GameFrame()",
"ProcessAdverts()",
"ProcessInGamePunishments()",
"gpManiVote->GameFrame()",
"gpManiAutoMap->GameFrame()",
"gpManiAFK->ProcessUsercmds()",
"gpManiGameType->GameFrame()",
};
#endif

void ResetProfiles()
{
#ifdef PROFILING
	for (int i = 0; i < MAX_PROFILES; i++)
	{
		profile_list[i].Reset();
	}
#endif
}

void WriteProfiles()
{
#ifdef PROFILING
	bool found_profile = false;
	for (int i = 0; i < MAX_PROFILES; i ++)
	{
		if (profile_list[i].GetCount() != 0)
		{
			found_profile = true;
			break;
		}
	}

	if (!found_profile)
	{
		// Don't write any profiles if none set
		return;
	}

	ManiFile *mf = new ManiFile();
	FILE *ptr;

	struct	tm	*time_now;
	time_t	current_time;
	char temp_string[1024];
	char temp_string2[1024];

	time(&current_time);
	time_now = localtime(&current_time);

	snprintf(temp_string, sizeof(temp_string), "%02i.%02i.%04i.%02i.%02i.%02i.profile.log",
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec);

	ptr = mf->Open(temp_string, "a");
	if (ptr == NULL)
	{
		delete mf;
		return;
	}

	for (int i = 0; i < MAX_PROFILES; i++)
	{
		if (profile_list[i].GetCount() == 0)
		{
			continue;
		}

		current_time = profile_list[i].GetMaxTimeStamp();
		time_now = localtime(&current_time);

		snprintf(temp_string, sizeof(temp_string), "%02i/%02i/%04i %02i:%02i:%02i",
								time_now->tm_mon + 1,
								time_now->tm_mday,
								time_now->tm_year,
								time_now->tm_hour,
								time_now->tm_min,
								time_now->tm_sec);

		int length = snprintf(temp_string2, sizeof(temp_string), "%s, %f, %i, %f, %s\n",
						profile_list[i].GetFunctionID(),
						profile_list[i].GetAverage(),
						profile_list[i].GetCount(),
						profile_list[i].GetMaxTime(),
						temp_string);

		mf->Write(temp_string2, length, ptr); 
	}

	mf->Close(ptr);
	delete mf;
#endif
}




