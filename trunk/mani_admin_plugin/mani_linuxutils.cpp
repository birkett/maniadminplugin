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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <demangle.h>
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
#include "mani_linuxutils.h"

static int sort_by_address ( const void *m1,  const void *m2);
static int sort_by_mangled ( const void *m1,  const void *m2);
static int sort_by_demangled ( const void *m1,  const void *m2);

static int sort_by_address ( const void *m1,  const void *m2)
{
	return ((long) (*(symbol_t *) m1).ptr - (long) (*(symbol_t *) m2).ptr);
}

static int sort_by_mangled ( const void *m1,  const void *m2)
{
        return strcmp((*(symbol_t *) m1).mangled_name, (*(symbol_t *) m2).mangled_name);
}

static int sort_by_demangled ( const void *m1,  const void *m2)
{
        return strcmp((*(symbol_t *) m1).demangled_name, (*(symbol_t *) m2).demangled_name);
}

SymbolMap::SymbolMap()
{
	symbol_list_size = 0;
	addr_list = mangled_list = demangled_list = NULL;
}

SymbolMap::~SymbolMap()
{
	// Free up any lists
	FreeSymbols();
}

bool SymbolMap::GetLib(const char *lib_name)
{
        void *handle;
        void *var_address;

	FreeSymbols();

        var_address = NULL;
        handle = dlopen(lib_name, RTLD_NOW);
        if (handle == NULL)
        {
                return false;
        }

        // Borrowed from SourceMod.  Hidden symbols SUCK!!!!
        int dlfile;
        struct link_map *dlmap;
        struct stat dlstat;
        Elf32_Ehdr *file_hdr;
        uintptr_t map_base;
        Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
        Elf32_Sym *symtab;
        uint16_t section_count;
        const char *shstrtab, *strtab;
        uint32_t symbol_count;
        bool free_string = true;

        symtab_hdr = NULL;
        strtab_hdr = NULL;

        dlmap = (struct link_map *)handle;

        dlfile = open(dlmap->l_name, O_RDONLY);
        if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
        {
                close(dlfile);
                return false;
        }

        /* Map library file into memory */
        file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
        map_base = (uintptr_t)file_hdr;
        if (file_hdr == MAP_FAILED)
        {
                close(dlfile);
                return false;
        }

        close(dlfile);

        if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
        {
                munmap(file_hdr, dlstat.st_size);
                return false;
        }

        sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
        section_count = file_hdr->e_shnum;
        /* Get ELF section header string table */
        shstrtab_hdr = &sections[file_hdr->e_shstrndx];
        shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

        /* Iterate sections while looking for ELF symbol table and string table */
        for (uint16_t i = 0; i < section_count; i++)
        {
                Elf32_Shdr &hdr = sections[i];
                const char *section_name = shstrtab + hdr.sh_name;

                if (strcmp(section_name, ".symtab") == 0)
                {
                        symtab_hdr = &hdr;
                }
                else if (strcmp(section_name, ".strtab") == 0)
                {
                        strtab_hdr = &hdr;
                }
        }

        /* Uh oh, we don't have a symbol table or a string table */
        if (symtab_hdr == NULL || strtab_hdr == NULL)
        {
                munmap(file_hdr, dlstat.st_size);
                return false;
        }

        symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
        strtab = (const char *)(map_base + strtab_hdr->sh_offset);
        symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

        for (uint32_t i = 0; i < symbol_count; i++)
        {
                Elf32_Sym &sym = symtab[i];
                unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
                const char *sym_name = strtab + sym.st_name;

                // Skip symbols that are undefined or do not refer to functions or objects
                if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
                {
                         continue;
                }

                char *demangled_name = cplus_demangle(sym_name, DMGL_PARAMS);
                if (demangled_name == NULL)
                {
                        free_string = false;
                        demangled_name = (char *) sym_name;
                }

                AddToList((void **) &addr_list, sizeof(symbol_t), &symbol_list_size);
                addr_list[symbol_list_size - 1].ptr = (void *)(dlmap->l_addr + sym.st_value);
                addr_list[symbol_list_size - 1].mangled_name = (char *) malloc(strlen(sym_name) + 1);
                strcpy(addr_list[symbol_list_size - 1].mangled_name, sym_name);
                addr_list[symbol_list_size - 1].demangled_name = (char *) malloc(strlen(demangled_name) + 1);
                strcpy(addr_list[symbol_list_size - 1].demangled_name, demangled_name);

                if (free_string)
                {
                        free(demangled_name);
                }

                free_string = true;
        }

        munmap(file_hdr, dlstat.st_size);

	// Populate sub lists and sort */
	if (symbol_list_size != 0)
	{
		mangled_list = malloc(sizeof(symbol_t) * symbol_list_size);
		demangled_list = malloc(sizeof(symbol_t) * symbol_list_size);

		for (int i = 0; i < symbol_list_size; i++)
		{
			mangled_list[i].ptr = addr_list[i].ptr;
			mangled_list[i].mangled_name = addr_list[i].mangled_name;
			mangled_list[i].demangled_name = addr_list[i].demangled_name;
			demangled_list[i].ptr = addr_list[i].ptr;
			demangled_list[i].mangled_name = addr_list[i].mangled_name;
			demangled_list[i].demangled_name = addr_list[i].demangled_name;
		}

        	qsort(addr_list, symbol_list_size, sizeof(symbol_t), sort_by_address);
        	qsort(mangled_list, symbol_list_size, sizeof(symbol_t), sort_by_mangled);
        	qsort(demangled_list, symbol_list_size, sizeof(symbol_t), sort_by_demangled);
	}
	
        return true;

}

symbol_t *SymbolMap::GetAddr(void *ptr)
{
	symbol_t        *symbol_ptr = NULL;
	symbol_t        search_term;

        search_term.ptr = ptr;
        symbol_ptr  = (symbol_t *) bsearch (&search_term,
        				addr_list, symbol_list_size,
                                        sizeof(symbol_t),
                                        sort_by_address);

	return symbol_ptr;
}

symbol_t *SymbolMap::GetAddr(int index)
{
	return &(addr_list[index]);
}

symbol_t *SymbolMap::GetMangled(char *mangled_name)
{
	symbol_t        *symbol_ptr = NULL;
	symbol_t        search_term;

        strcpy(search_term.mangled_name, mangled_name);
        symbol_ptr  = (symbol_t *) bsearch (&search_term,
        				mangled_list, symbol_list_size,
                                        sizeof(symbol_t),
                                        sort_by_mangled);

	return symbol_ptr;
}

symbol_t *SymbolMap::GetMangled(int index)
{
	return &(mangled_list[index]);
}

symbol_t *SymbolMap::GetDeMangled(char *demangled_name)
{
	symbol_t        *symbol_ptr = NULL;
	symbol_t        search_term;

        strcpy(search_term.demangled_name, demangled_name);
        symbol_ptr  = (symbol_t *) bsearch (&search_term,
        				demangled_list, symbol_list_size,
                                        sizeof(symbol_t),
                                        sort_by_demangled);

	return symbol_ptr;
}

symbol_t *SymbolMap::GetDeMangled(int index)
{
	return &(demangled_list[index]);
}

void SymbolMap::FreeSymbols()
{
	for (int i = 0; i < symbol_list_size; i++)
	{
		if (addr_list[i].mangled_name)
		{
			free(addr_list[i].mangled_name);
		}

		if (addr_list[i].demangled_name)
		{
			free(addr_list[i].demangled_name);
		}
	}

	if (addr_list)
	{
		free(addr_list);
		free(mangled_list);
		free(demangled_list);
		addr_list = NULL;
		mangled_list = NULL;
		demangled_list = NULL;
		symbol_list_size = 0;
	}
}
