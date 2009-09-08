// THESE FILES ARE PROVIDED TO THE PUBLIC VIA SOURCEMOD
// VISIT http://www.sourcemod.net

// Altered by Keeper for use in Mani Admin Plugin

#ifndef _INCLUDE_KNIGHT_KE_PLATFORM_H_
#define _INCLUDE_KNIGHT_KE_PLATFORM_H_

#if defined WIN32

#define KE_PLATFORM_WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>

#if !defined alloca
#define alloca _alloca
#endif

#else

#define KE_PLATFORM_POSIX

#if defined linux
#define KE_PLATFORM_LINUX
#endif

#endif

#endif //_INCLUDE_KNIGHT_KE_PLATFORM_H_
