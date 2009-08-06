/**
 * vim: set ts=4 :
 * ======================================================
 * Metamod:Source
 * Copyright (C) 2004-2008 AlliedModders LLC and authors.
 * All rights reserved.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, 
 * including commercial applications, and to alter it and redistribute it 
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not 
 * claim that you wrote the original software. If you use this software in a 
 * product, an acknowledgment in the product documentation would be 
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_METAMOD_SOURCE_LOADER_H_
#define _INCLUDE_METAMOD_SOURCE_LOADER_H_

#define SH_COMP_GCC 	1
#define SH_COMP_MSVC	2

#if defined WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#define PLATFORM_MAX_PATH	MAX_PATH
#define SH_COMP				SH_COMP_MSVC
#define	PATH_SEP_STR		"\\"
#define PATH_SEP_CHAR		'\\'
#define ALT_SEP_CHAR		'/'
#elif defined __linux__
#include <dlfcn.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
typedef void *	HMODULE;
#define PLATFORM_MAX_PATH	PATH_MAX
#define SH_COMP				SH_COMP_GCC
#define	PATH_SEP_STR		"/"
#define PATH_SEP_CHAR		'/'
#define ALT_SEP_CHAR		'\\'
#else
#error "OS detection failed"
#endif

#include "loader_bridge.h"

#define SH_PTRSIZE sizeof(void*)

enum MetamodBackend
{
	MMBackend_Episode1 = 0,
	MMBackend_Episode2,
	MMBackend_Left4Dead,
	MMBackend_DarkMessiah,
	MMBackend_UNKNOWN
};

extern bool
mm_LoadMetamodLibrary(MetamodBackend backend, char *buffer, size_t maxlength);

extern void *
mm_GetProcAddress(const char *name);

extern void
mm_UnloadMetamodLibrary();

extern void
mm_LogFatal(const char *message, ...);

extern const char *
mm_GetGameName();

extern MetamodBackend
mm_DetermineBackend(QueryValveInterface qvi, const char *game_name);

#endif /* _INCLUDE_METAMOD_SOURCE_LOADER_H_ */

