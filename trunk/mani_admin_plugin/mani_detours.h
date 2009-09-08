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


#include <string.h>

#ifndef MANI_DETOURS_H
#define MANI_DETOURS_H

typedef unsigned char mem_t;

enum DETOUR_STATE {
	UNDEFINED = 0,
	DEFINED,
	INPLACE,
};

struct save_memory_t {
	mem_t bytes_from_memory[32];  //Why 32?  Good question.  
	size_t size_stored;

	save_memory_t() {
		memset ( bytes_from_memory, 0, 32 );
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
	bool			EndDetour();
};
#endif // MANI_DETOURS_H