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
#include "engine/iserverplugin.h"

//---------------------------------------------------------------------------------
// Purpose: Generic way of adding 1 item to a list
//---------------------------------------------------------------------------------
bool AddToList(void **list_ptr, size_t size_of_structure, int *list_size_ptr)
{
	void *temp_ptr;

	// Is list empty ?
	if (*list_ptr == NULL)
	{
		*list_ptr = (void *) malloc (size_of_structure * (*list_size_ptr + 1));
		if (*list_ptr == NULL)
		{
			Msg("Run out of memory running malloc !\n");
			return false;
		}

		*list_size_ptr = *list_size_ptr + 1;
		return true;
	}
	
	// List already created, use realloc to add one
	temp_ptr = (void *) realloc(*list_ptr, (*list_size_ptr + 1) * size_of_structure);
	if (temp_ptr == NULL)
	{
		// Realloc failed !!
		Msg("Run out of memory running realloc !\n");
		return false;
	}

	*list_size_ptr = *list_size_ptr + 1;
	*list_ptr = temp_ptr;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Generic way of creating a list of items
//---------------------------------------------------------------------------------
bool CreateList(void **list_ptr, size_t size_of_structure, int size_required, int *list_size_ptr)
{
	// Is list empty ?
	if (*list_ptr != NULL)
	{
		Msg("Warning list_ptr not null !\n");
		return false;
	}

	*list_ptr = (void *) malloc (size_of_structure * size_required);
	if (*list_ptr == NULL)
		{
			Msg("Run out of memory running malloc !\n");
			return false;
		}

	*list_size_ptr = size_required;
	
	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Generic way of freeing a list
//---------------------------------------------------------------------------------
void FreeList(void **list_ptr, int *list_size_ptr)
{

	if (*list_ptr != NULL)
	{
		free(*list_ptr);
	}
	
	*list_ptr = NULL;
	*list_size_ptr = 0;
	return;
}
