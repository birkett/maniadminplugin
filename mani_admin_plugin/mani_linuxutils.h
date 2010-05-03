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

#ifndef MANI_LINUXUTILS
#define MANI_LINUXUTILS

struct symbol_t
{
        void *ptr;
        char *mangled_name;
        char *demangled_name;
};

class SymbolMap
{
public:

	SymbolMap();
	~SymbolMap();

	bool SymbolMap::GetLib(const char *lib_name);
	symbol_t *SymbolMap::GetAddr(void *ptr);
	symbol_t *SymbolMap::GetAddr(int index);

	symbol_t *SymbolMap::GetMangled(char *mangled_name);
	symbol_t *SymbolMap::GetMangled(int index);

	symbol_t *SymbolMap::GetDeMangled(char *demangled_name);
	symbol_t *SymbolMap::GetDeMangled(int index);

	int SymbolMap::GetMapSize() { return symbol_list_size; };
	
private:
	void FreeSymbols();

	// This is the main unsorted list
	symbol_t *addr_list;
	symbol_t *mangled_list;
	symbol_t *demangled_list;
	int    symbol_list_size;
};

#endif
