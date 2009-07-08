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

#ifndef __linux__
#include <windows.h>      // Needed for all Winsock stuff
#endif

#ifdef __linux__
#include <sys/types.h>    // Needed for system defined identifiers.
#include <netinet/in.h>   // Needed for internet address structure.
#include <sys/socket.h>   // Needed for socket(), bind(), etc...
#include <arpa/inet.h>    // Needed for inet_ntoa()
#include <fcntl.h>
#include <netdb.h>
#endif

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
#include "mani_output.h"
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

int	UTIL_GetWebVersion(const char *ip_address, const int port, const char *filename)
{
#ifndef __linux__
	WORD wVersionRequested = MAKEWORD(1,1);    // Stuff for WSA functions
	WSADATA wsaData;                           // Stuff for WSA functions
#endif

	unsigned int         server_s;             // Server socket descriptor
	struct sockaddr_in   server_addr;          // Server Internet address
	char                 out_buf[4096];    // Output buffer for GET request
	char                 in_buf[4096];     // Input buffer for response
	unsigned int         retcode;              // Return code

#ifndef __linux__
	// This stuff initializes winsock
	WSAStartup(wVersionRequested, &wsaData);
#endif	

	// Create a socket
	server_s = socket(AF_INET, SOCK_STREAM, 0);

	// Fill-in the Web server socket's address information
	server_addr.sin_family = AF_INET;                 // Address family to use
	server_addr.sin_port = htons(port);           // Port num to use
	server_addr.sin_addr.s_addr = inet_addr(ip_address); // IP address to use

	// Do a connect (connect() blocks)
	retcode = connect(server_s, (struct sockaddr *)&server_addr,
		sizeof(server_addr));
	if (retcode != 0)
	{
		return -1;
	}

	// Send a GET to the Web server
	sprintf(out_buf, "GET %s HTTP/1.0\r\n\r\n", filename);
	send(server_s, out_buf, strlen(out_buf), 0);

	// Receive from the Web server
	//  - The return code from recv() is the number of bytes received
	int result = -1;
	retcode = recv(server_s, in_buf, sizeof(in_buf), 0);

	if (retcode != -1 && retcode > 4)
	{
		for (unsigned int i = 0; i < retcode - 4; i++)
		{
			if (in_buf[i] == 0x0D && in_buf[i + 1] == 0x0A &&
				in_buf[i + 2] == 0x0D && in_buf[i + 3] == 0x0A)
			{
				for (unsigned int j = i + 4; j < retcode; j ++)
				{
					if (in_buf[j] == 0x0A || in_buf[j] == 0x0D)
					{
						in_buf[j] = '\0';
						break;
					}
				}

				in_buf[retcode] = '\0';
				result = atoi(&in_buf[i + 4]);
				break;
			}
		}
	}


	// Close all open sockets
#ifndef __linux__
	closesocket(server_s);
#else
	close(server_s);
#endif

#ifndef __linux__
	// This stuff cleans-up winsock
	WSACleanup();
#endif

	if (result == 0)
	{
		return -1;
	}
	
	return (result);
}


#ifndef __linux__

// Windows version
char	*UTIL_GetHTTPFile(const char *ip_address, const int port, const char *filename, long cap_size, long *filesize)
{
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;

	unsigned int         server_s;
	struct sockaddr_in   server_addr;
	char                 out_buf[512];
	char                 in_buf[4096];
	unsigned int         retcode;
	unsigned int         i;
	char 			*final_buf;/*
	char			*temp_buf;
	long			final_buf_size;
	long 			arg;
	fd_set		myset;
	struct timeval	tv;
	int			valopt;
	int		lon;

	WSAStartup(wVersionRequested, &wsaData);

	*filesize = 0;
	printf("Getting socket ref\n");
	server_s = socket(AF_INET, SOCK_STREAM, 0);

	arg = ioctlsocket(server_s, F_GETFL, NULL); 
	arg |= O_NONBLOCK; 
	ioctlsocket(server_s, F_SETFL, arg);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip_address);

	printf("Connecting to socket\n");
	retcode = connect(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (retcode != 0)
	{
		if (errno == EINPROGRESS)
		{
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			FD_ZERO(&myset);
			FD_SET(server_s, &myset);
			if (select(server_s + 1, NULL, &myset, NULL, &tv) > 0)
			{
				lon = sizeof(int);
				getsockopt(server_s, SOL_SOCKET, SO_ERROR, (void *) &valopt, &lon);
				if (valopt)
				{
					printf("Error in connection() %d - %s\n", valopt, strerror(valopt));
					exit(1);
				}
			}
			else
			{
				printf("Timeout or error() %d - %s\n", valopt, strerror(valopt));
				exit(1);
			}
		}
		else
		{
			printf("Error connecting %d - %s\n", errno, strerror(errno));
			exit(1);
		}
	}

	arg = ioctlsocket(server_s, F_GETFL, NULL);
	arg &= (~O_NONBLOCK);
	ioctlsocket(server_s, F_SETFL, arg);

	sprintf(out_buf, "GET %s HTTP/1.0\r\n\r\n", filename);
	printf("Sending request %s\n", out_buf);
	send(server_s, out_buf, strlen(out_buf), 0);

	final_buf = NULL;
	final_buf_size = 0;

	printf("Waiting for receipt\n");
	retcode = recv(server_s, in_buf, sizeof(in_buf), 0);
	if (retcode > 0)
	{
		printf("Receipt valid\n");
		while ((retcode > 0) || (retcode == -1))
		{
			if (retcode == -1)
			{
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			temp_buf = (char *) realloc(final_buf, ((final_buf_size + retcode) * sizeof(char)) + sizeof(char));
			if (!temp_buf)
			{
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			memcpy(&temp_buf[final_buf_size], in_buf, sizeof(char) * retcode);
			final_buf_size += retcode;
			final_buf = temp_buf;
			if (cap_size > 0 && final_buf_size > cap_size)
			{
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			retcode = recv(server_s, in_buf, sizeof(in_buf), 0);
		}
	}	

	closesocket(server_s);
	WSACleanup();

	if (final_buf)
	{
		*filesize = final_buf_size;
		final_buf[final_buf_size] = '\0';
	}
	else
	{
		*filesize = 0;
	}
*/
	return final_buf;
}
#else
// linux version
char	*UTIL_GetHTTPFile(const char *ip_address, const int port, const char *filename, long cap_size, long *filesize)
{
#ifdef BLAHBLAH
	unsigned int         server_s;
	struct sockaddr_in   server_addr;
	char                 out_buf[512];
	char                 in_buf[4096];
	unsigned int         retcode;
	unsigned int         i;
	char 			*final_buf;
	char			*temp_buf;
	long			final_buf_size;
	long 			arg;
	fd_set		myset;
	struct timeval	tv;
	int			valopt;
	socklen_t		lon;

	*filesize = 0;
	printf("Getting socket ref\n");
	server_s = socket(AF_INET, SOCK_STREAM, 0);

	arg = fcntl(server_s, F_GETFL, NULL); 
	arg |= O_NONBLOCK; 
	fcntl(server_s, F_SETFL, arg);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip_address);

	printf("Connecting to socket\n");
	retcode = connect(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (retcode != 0)
	{
		if (errno == EINPROGRESS)
		{
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			FD_ZERO(&myset);
			FD_SET(server_s, &myset);
			if (select(server_s + 1, NULL, &myset, NULL, &tv) > 0)
			{
				lon = sizeof(int);
				getsockopt(server_s, SOL_SOCKET, SO_ERROR, (void *) &valopt, &lon);
				if (valopt)
				{
					printf("Error in connection() %d - %s\n", valopt, strerror(valopt));
					exit(1);
				}
			}
			else
			{
				printf("Timeout or error() %d - %s\n", valopt, strerror(valopt));
				exit(1);
			}
		}
		else
		{
			printf("Error connecting %d - %s\n", errno, strerror(errno));
			exit(1);
		}
	}

	arg = fcntl(server_s, F_GETFL, NULL);
	arg &= (~O_NONBLOCK);
	fcntl(server_s, F_SETFL, arg);

	sprintf(out_buf, "GET %s HTTP/1.0\r\n\r\n", filename);
	printf("Sending request %s\n", out_buf);
	send(server_s, out_buf, strlen(out_buf), 0);

	final_buf = NULL;
	final_buf_size = 0;

	printf("Waiting for receipt\n");
	retcode = recv(server_s, in_buf, sizeof(in_buf), 0);
	if (retcode > 0)
	{
		printf("Receipt valid\n");
		while ((retcode > 0) || (retcode == -1))
		{
			if (retcode == -1)
			{
				/* Failed */
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			temp_buf = (char *) realloc(final_buf, ((final_buf_size + retcode) * sizeof(char)) + sizeof(char));
			if (!temp_buf)
			{
				/* No memory left */
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			memcpy(&temp_buf[final_buf_size], in_buf, sizeof(char) * retcode);
			final_buf_size += retcode;
			final_buf = temp_buf;
			if (cap_size > 0 && final_buf_size > cap_size)
			{
				if (final_buf)
				{
					free(final_buf);
					final_buf = NULL;
				}

				break;
			}

			retcode = recv(server_s, in_buf, sizeof(in_buf), 0);
		}
	}	

	close(server_s);

	if (final_buf)
	{
		*filesize = final_buf_size;
		final_buf[final_buf_size] = '\0';
	}
	else
	{
		*filesize = 0;
	}
#endif
	return "test";
}


#endif



const char	*UTIL_FindHTTPContent(const char *buffer)
{
	const char *search;

	search = strstr(buffer, "\r\n\r\n");
	if (search)
	{
		search += 4;
		return search;
	}

	search = strstr(buffer, "\n\r\n\r");
	if (search)
	{
		search += 4;
		return search;
	}

	return NULL;
}

void	UTIL_WriteBufferToDisk(const char *buffer, const char *filename)
{
	FILE *fh; 

	fh = fopen(filename, "wt");
	if (!fh)
	{	
		return;
	}

	int i = 0;
	while(buffer[i] != '\0')
	{
		fputc(buffer[i++], fh);
		if (ferror(fh))
		{
			break;
		}
	}

	fclose(fh);
}

int	UTIL_GetVersionFromBuffer(const char *buffer)
{
	char temp_buffer[64];

	int i = 0;
	int j = 0;

	while(buffer[i] != '\0')
	{
		if (j == sizeof(temp_buffer))
		{
			return -1;
		}

		if (isdigit(buffer[i]))
		{
			/* char is numeric */
			temp_buffer[j] = buffer[i];
			j++;
		}
		else if (buffer[i] == '\n' || buffer[i] == '\r')
		{
			break;
		}

		i++;
	}

	if (j == 0)
	{
		return -1;
	}

	temp_buffer[j] = '\0';
	return atoi(temp_buffer);
}
