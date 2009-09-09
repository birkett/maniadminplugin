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

#include "eiface.h"
#include "iplayerinfo.h"
#include "mani_detours.h"
#include "KeCodeAllocator.h"
#include "mani_output.h"
#include "asm.h"
#include "jit_helpers.h"
#include "x86/x86_macros.h"

using namespace Knight;

KeCodeCache	   *g_pCodeCache;

CDetour::CDetour( const char *function, void *address_from, void *address_to, void **ret_tramp )
{
	strcpy ( FunctionDetoured, function );
	DetourAddress = address_from;
	NewFunctionAddress = address_to;
	TrampolineFunc = ret_tramp;
	g_pCodeCache = KE_CreateCodeCache();
	state = UNDEFINED;

	TrampolineAddress = NULL;
}

bool CDetour::Init ( ) {
	if (!StartDetour()) return false;
	
	state = DEFINED;
	return true;
}

DETOUR_STATE CDetour::DetourState() {
	return state;
}

void CDetour::Destroy() {
	EndDetour();
	delete this;
}

void CDetour::DetourFunction() { 
	if ( state == DEFINED ) state = ApplyDetour ( (mem_t *)DetourAddress, &NewFunctionAddress );
}

void CDetour::RestoreFunction() {
	if ( state == INPLACE ) state = RestoreCode( (mem_t *)DetourAddress, &orig_bytes );
}

// this was pretty much copied verbatim from pRED's CBaseServer code.  Too much I didn't 
// understand in this area .... yet
bool CDetour::StartDetour() {
	if (!DetourAddress) {
		MMsg ("Detour for %s failed - no valid pointer was provided.\n", FunctionDetoured);
		return false;
	}

	orig_bytes.size_stored = copy_bytes((mem_t *)DetourAddress, NULL, OP_JMP_SIZE+1);

	for ( size_t i=0; i < orig_bytes.size_stored; i++ )
		orig_bytes.bytes_from_memory[i] = ((mem_t *)DetourAddress)[i];

	JitWriter wr;
	JitWriter *jit = &wr;
	jit_uint32_t CodeSize = 0;
	
	wr.outbase = NULL;
	wr.outptr = NULL;

jit_rewind:

	/* Patch old bytes in */
	if (wr.outbase != NULL) 
		copy_bytes((mem_t *)DetourAddress, (mem_t *)wr.outptr, orig_bytes.size_stored);
	wr.outptr += orig_bytes.size_stored;

	/* Return to the original function */
	jitoffs_t call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (mem_t *)DetourAddress + orig_bytes.size_stored);

	if (wr.outbase == NULL)
	{
		CodeSize = wr.get_outputpos();
		wr.outbase = (jitcode_t)KE_AllocCode(g_pCodeCache, CodeSize);
		wr.outptr = wr.outbase;
		TrampolineAddress = wr.outbase;
		goto jit_rewind;
	}

	*TrampolineFunc = TrampolineAddress;

	return true;
}

void CDetour::EndDetour() { 
	RestoreFunction();
	TrampolineAddress = FreeMemory(TrampolineAddress);
}

void *CDetour::FreeMemory ( void *addr ) {
	if ( !addr )
		return NULL;

	KE_FreeCode ( g_pCodeCache, addr );
	return NULL;
}

CDetour *CDetourManager::CreateDetour( const char *function, void *address_from, void *address_to, void **ret_tramp )
{
	CDetour *detour = new CDetour(function, address_from, address_to, ret_tramp);
	if (detour) {
		if (!detour->Init()) {
			delete detour;
			return NULL;
		}
		return detour;
	}

	return NULL;
}
