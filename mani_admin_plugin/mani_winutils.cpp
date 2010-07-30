//
// Mani Admin Plugin
//
// Copyright (c) 2010 Giles Millward (Mani). All rights reserved.
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

#if defined ( WIN32 )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <winnt.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "convar.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_gametype.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_client_flags.h"
#include "mani_client.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "cbaseentity.h"
#include "mani_winutils.h"


CWinSigUtil::CWinSigUtil()
{
}

CWinSigUtil::~CWinSigUtil()
{
}

bool CWinSigUtil::GetLib(void *pAddr)
{
	base_address = 0;
	binary_len = 0;
	MEMORY_BASIC_INFORMATION mem;

	if(!pAddr)
		return false; // GetDllMemInfo failed: !pAddr

	if(!VirtualQuery(pAddr, &mem, sizeof(mem)))
		return false;

	base_address = (unsigned char *)mem.AllocationBase;

	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)mem.AllocationBase;
	IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS*)((unsigned long)dos+(unsigned long)dos->e_lfanew);

	if(pe->Signature != IMAGE_NT_SIGNATURE) {
		base_address = 0;
		return false; // GetDllMemInfo failed: pe points to a bad location
	}

	binary_len = (size_t)pe->OptionalHeader.SizeOfImage;
	MMsg("Found base %p and length %i [%p]\n", base_address, binary_len, base_address + binary_len);

	return true;
}

void *CWinSigUtil::FindSignature(unsigned char *pSignature)
{
	unsigned char *pBasePtr = base_address;
	unsigned char *pEndPtr = base_address + binary_len;	

	unsigned char	sigscan[128];
	bool			sigscan_wildcard[128];
	unsigned int	scan_length = 0;
	int				str_index = 0;

	while(1)
	{
		if (pSignature[str_index] == ' ')
		{
			str_index++;
			continue;
		}

		if (pSignature[str_index] == '\0')
		{
			break;
		}

		if (pSignature[str_index] == '?')
		{
			sigscan_wildcard[scan_length++] = true;
			str_index ++;
			continue;
		}

		if (pSignature[str_index + 1] == '\0' || 
			pSignature[str_index + 1] == '?' || 
			pSignature[str_index + 1] == ' ')
		{
			MMsg("Failed to decode [%s], single digit hex code\n", pSignature);
			return (void *) NULL;
		}

		// We are expecting a two digit hex code
		char upper_case1 = toupper(pSignature[str_index]);
		if (!this->ValidHexChar(upper_case1))
		{
			MMsg("Failed to decode [%s], bad hex code\n", pSignature);
			return NULL;
		}

		char upper_case2 = toupper(pSignature[str_index + 1]);
		if (!this->ValidHexChar(upper_case2))
		{
			MMsg("Failed to decode [%s], bad hex code\n", pSignature);
			return NULL;
		}

		// Generate our byte code
		unsigned char byte = (HexToBin(upper_case1) << 4) + HexToBin(upper_case2);
		str_index += 2;
		sigscan_wildcard[scan_length] = false;
		sigscan[scan_length] = byte;
		scan_length ++;
		if (scan_length == sizeof(sigscan))
		{
			MMsg("Sigscan too long!\n");
			return NULL;
		}
	}

	unsigned int i;
	while (pBasePtr < pEndPtr)
	{
		for (i=0; i < scan_length; i++)
		{
			if (sigscan_wildcard[i] != true && sigscan[i] != pBasePtr[i])
				break;
		}
		//if i reached the end, we know we have a match!
		if (i == scan_length)
			return (void *)pBasePtr;
		pBasePtr += sizeof(unsigned char);  //search memory in an aligned manner
	}

	return NULL;
}

unsigned char CWinSigUtil::HexToBin(char hex_char)
{
	char upper_char = toupper(hex_char);
	return ((upper_char >= '0' && upper_char <= '9') ? upper_char - 48:upper_char - 55);
}

bool CWinSigUtil::ValidHexChar(char hex_char)
{
	char upper_char = toupper(hex_char);
	return ((upper_char >= '0' && upper_char <= '9') || (upper_char >= 'A' && upper_char <= 'F'));
}

#endif