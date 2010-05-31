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
// Many thanks from pRED*, Lance Vorgin, and SourceMod for helping me understand detouring.

//



#if defined WIN32
#include <windows.h>
#include <winnt.h>
#endif

#ifndef MANI_DETOURS_H
#define MANI_DETOURS_H

#if !defined ( WIN32 )
#include <sys/mman.h>
#define	PAGE_SIZE	4096
#define ALIGN(ar) ((long)ar & ~(PAGE_SIZE-1))
#define	PAGE_EXECUTE_READWRITE	PROT_READ|PROT_WRITE|PROT_EXEC
#endif

typedef unsigned char mem_t;

/**
 * Converts a member function pointer to a void pointer.
 * This relies on the assumption that the code address lies at mfp+0
 * This is the case for both g++ and later MSVC versions on non virtual functions but may be different for other compilers
 * Based on research by Don Clugston : http://www.codeproject.com/cpp/FastDelegate.asp
 */
class GenericClass {};
typedef void (GenericClass::*VoidFunc)();
inline void *GetMemberFunctionAddress(VoidFunc MemberFunctionPointer) {
	return *(void **)&MemberFunctionPointer;
}
#define GetFunctionAddress(MemberFunctionPointer) GetMemberFunctionAddress(reinterpret_cast<VoidFunc>(MemberFunctionPointer))


#define MEMORY_TO_USE 20

enum DETOUR_STATE {
	UNDEFINED = 0,
	DEFINED,
	INPLACE,
};

struct save_memory_t {
	mem_t bytes_from_memory[MEMORY_TO_USE];
	size_t size_stored;

	save_memory_t() {
		bytes_from_memory[0] = 0;
		size_stored = 0;
	}
};
class CDetour {
public:
	CDetour			( const char *function, void *address_from, void *address_to, void **ret_tramp );

	// write the jmp to your trampoline
	void			DetourFunction(); 
	// restore the original memory
	void			RestoreFunction();

	DETOUR_STATE	DetourState();
	void			Destroy();
	bool			Init ();
private:
	DETOUR_STATE	state;

	// this is a backup of the original bytes ( 6 for JMP )
	save_memory_t	orig_bytes;
	// the function you want to detour
	void		   *DetourAddress;
	// the function you want to go to instead
	void		   *TrampolineAddress;
	// the new function to be detoured to
	void		   *NewFunctionAddress;
	// this is what we'll use to call the original function. ( *TrampolineFunc=TrampolineAddress )
	void		  **TrampolineFunc;

	char			FunctionDetoured[32];

	// allocate the trampoline space and create the pointers
	bool			StartDetour();
	// disable the detour ( RestoreFunction() ) and free the allocated space.
	void			EndDetour();

	void		   *FreeMemory ( void *addr );
};

inline void AllocateMemory ( mem_t *address_from ) {
#if !defined ( WIN32 )
	void *trash = (void *)ALIGN(address_from);
	mprotect(trash, sysconf(_SC_PAGESIZE), PAGE_EXECUTE_READWRITE);
#else
	DWORD trash;
	VirtualProtect(address_from, MEMORY_TO_USE, PAGE_EXECUTE_READWRITE, &trash);
#endif
}

inline DETOUR_STATE ApplyDetour( mem_t *address_from, void *address_to )
{
	AllocateMemory ( address_from );
	address_from[0] = 0xFF;
	address_from[1] = 0x25;
	*(void **)(&address_from[2]) = address_to;

	return INPLACE;
}

inline DETOUR_STATE RestoreCode( mem_t *address_from, save_memory_t *orig_bytes ) {

	AllocateMemory ( address_from );

	mem_t *addr = (mem_t *)address_from;

	for ( size_t i=0; i < orig_bytes->size_stored; i++ )
		addr[i] = orig_bytes->bytes_from_memory[i];

	return DEFINED;
}

class CDetourManager
{
public:
	static CDetour *CreateDetour( const char *function, void *address_from, void *address_to, void **ret_tramp);
	friend class CDetour;
};
#endif // MANI_DETOURS_H