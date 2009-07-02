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



#ifndef MANI_MEMORY_H
#define MANI_MEMORY_H

extern	bool AddToList(void **list_ptr, size_t size_of_structure, int *list_size_ptr);
extern	bool RemoveIndexFromList(void **list_ptr, size_t size_of_structure, int *list_size_ptr, int index,  void *index_ptr, void *end_ptr);
extern	bool CreateList(void **list_ptr, size_t size_of_structure, int size_required, int *list_size_ptr);
extern	void FreeList(void **list_ptr, int *list_size_ptr);

#endif
