//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_output.h"
#include "mani_file.h"
#include "mani_language.h"
#include "mani_client_util.h"
#include "mani_util.h"
#include "mani_keyvalues.h"

#undef strlen
//********************************************************************************
// 
//********************************************************************************

GlobalGroupFlag::GlobalGroupFlag()
{
}

GlobalGroupFlag::~GlobalGroupFlag()
{
}

void GlobalGroupFlag::AddFlag(const char *flag_id)
{
	const unsigned int hash_id = djb2_hash((unsigned char *) flag_id);

	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator p;
	flag_list.insert(std::make_pair(hash_id, FlagAccessSwitch(flag_id, true)));
}

void GlobalGroupFlag::SetFlag(const char *flag_id, const bool enable)
{
	const unsigned int hash_id = djb2_hash((unsigned char *)flag_id);
	bool found = false;
	typedef	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator RangeType;
	std::pair<RangeType, RangeType> range = flag_list.equal_range(hash_id);

	for (RangeType i = range.first; i != range.second; i++)
	{
		if (strcmp(i->second.GetFlagID(), flag_id) == 0)
		{
			i->second.SetEnabled(enable);
			found = true;
			break;
		}
	}

	if (!found)
	{
		this->AddFlag(flag_id);
	}
}

bool GlobalGroupFlag::IsFlagSet(const char *flag_id)
{
	const unsigned int hash_id = djb2_hash((unsigned char *)flag_id);

	typedef	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator RangeType;
	std::pair<RangeType, RangeType> range = flag_list.equal_range(hash_id);

	for (RangeType i = range.first; i != range.second; i++)
	{
		if (strcmp(i->second.GetFlagID(), flag_id) == 0)
		{
			if (i->second.IsEnabled())
			{
				return true;
			}

			return false;
		}
	}

	return false;
}

bool GlobalGroupFlag::CatFlags(char *string)
{
	bool found_flag = false;
	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator i;

	strcpy(string, "");
	for (i = flag_list.begin(); i != flag_list.end(); i++)
	{
		if (i->second.IsEnabled())
		{
			found_flag = true;
			strcat(string, i->second.GetFlagID());
			strcat(string, " ");
		}
	}

	if (found_flag)	string[strlen(string) - 1] = '\0';
	return found_flag;
}

bool GlobalGroupFlag::CatFlags(char *string, const unsigned int length, const bool start)
{
	bool found_flag = false;

	if (start) 
	{
		l_itr = flag_list.begin();
	}

	strcpy(string, "");
	for (;l_itr != flag_list.end(); l_itr++)
	{
		if (l_itr->second.IsEnabled())
		{
			if (strlen(string) + strlen(l_itr->second.GetFlagID()) > length)
			{
				break;
			}

			found_flag = true;
			strcat(string, l_itr->second.GetFlagID());
			strcat(string, " ");
		}
	}

	if (found_flag)	string[strlen(string) - 1] = '\0';
	return found_flag;
}

const char	*GlobalGroupFlag::FindFirst()
{
	for (l_itr2 = flag_list.begin(); l_itr2 != flag_list.end(); l_itr2++)
	{
		if (l_itr2->second.IsEnabled())
		{
			return l_itr2->second.GetFlagID();
		}
	}

	return NULL;
}

const char	*GlobalGroupFlag::FindNext()
{
	l_itr2++;
	for (;l_itr2 != flag_list.end(); l_itr2++)
	{
		if (l_itr2->second.IsEnabled())
		{
			return l_itr2->second.GetFlagID();
		}
	}

	return NULL;
}

//********************************************************************************
// 
//********************************************************************************

PersonalFlag::PersonalFlag()
{
}

PersonalFlag::~PersonalFlag()
{
}

void PersonalFlag::Copy(PersonalFlag &src)
{
	this->flag_list.clear();
	std::multimap<const unsigned int, ClassFlagAccess, ltint>::iterator itr;
	itr = src.flag_list.begin();
	for (itr = src.flag_list.begin(); itr != src.flag_list.end(); itr++)
	{
		if (itr->second.IsEnabled())
		{
			this->flag_list.insert(std::make_pair(itr->first, ClassFlagAccess(itr->second.GetClassType(), itr->second.GetFlagID(), true)));
		}
	}
}

void PersonalFlag::AddFlag(const char *class_type, const char *flag_id)
{
	const unsigned int hash_id = djb2_hash((unsigned char *) class_type, (unsigned char *) flag_id);
	flag_list.insert(std::pair<const unsigned int, ClassFlagAccess> (hash_id, ClassFlagAccess(class_type, flag_id, true)));
}

void PersonalFlag::SetFlag(const char *class_type, const char *flag_id, const bool enable)
{
	bool found = false;

	if (!flag_list.empty())
	{
		const unsigned int hash_id = djb2_hash((unsigned char *) class_type, (unsigned char *)flag_id);
		typedef	std::multimap<const unsigned int, ClassFlagAccess, ltint>::iterator RangeType;
		std::pair<RangeType, RangeType> range = flag_list.equal_range(hash_id);

		for (RangeType i = range.first; i != range.second; i++)
		{
			if (strcmp(i->second.GetFlagID(), flag_id) == 0 &&
				strcmp(i->second.GetClassType(), class_type) == 0)
			{
				i->second.SetEnabled(enable);
				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		this->AddFlag(class_type, flag_id);
	}
}

void PersonalFlag::SetAll(const char *class_type, FlagDescList *flag_list_ptr)
{
	this->Kill();
	const DualStrKey *key_value = NULL;
	for (const char *flag_id = flag_list_ptr->FindFirst(class_type, &key_value); flag_id != NULL; flag_id = flag_list_ptr->FindNext(class_type, &key_value))
	{
		const unsigned int hash_id = djb2_hash((unsigned char *) class_type, (unsigned char *) flag_id);
		flag_list.insert(std::pair<const unsigned int, ClassFlagAccess> (hash_id, ClassFlagAccess(class_type, flag_id, true)));
	}
}

bool PersonalFlag::IsFlagSet(const char *class_type, const char *flag_id)
{
	if (!flag_list.empty())
	{
		const unsigned int hash_id = djb2_hash((unsigned char *) class_type, (unsigned char *)flag_id);

		typedef	std::multimap<const unsigned int, ClassFlagAccess, ltint>::iterator RangeType;
		std::pair<RangeType, RangeType> range = flag_list.equal_range(hash_id);

		for (RangeType i = range.first; i != range.second; i++)
		{
			if (strcmp(i->second.GetFlagID(), flag_id) == 0 &&
				strcmp(i->second.GetClassType(), class_type) == 0)
			{
				if (i->second.IsEnabled())
				{
					return true;
				}

				return false;
			}
		}
	}

	return false;
}

bool PersonalFlag::CatFlags(char *string, const char *class_type)
{
	bool found_flag = false;
	std::multimap<const unsigned int, ClassFlagAccess, ltint>::iterator i;

	strcpy(string, "");
	for (i = flag_list.begin(); i != flag_list.end(); i++)
	{
		if (strcmp(i->second.GetClassType(), class_type) == 0 && 
			i->second.IsEnabled())
		{
			found_flag = true;
			strcat(string, i->second.GetFlagID());
			strcat(string, " ");
		}
	}

	if (found_flag)	string[strlen(string) - 1] = '\0';
	return found_flag;
}

bool PersonalFlag::CatFlags(char *string, const char *class_type, const unsigned int length, const bool start)
{
	bool found_flag = false;

	if (start) 
	{
		l_itr = flag_list.begin();
	}

	strcpy(string, "");
	for (;l_itr != flag_list.end(); l_itr++)
	{
		if (strcmp(l_itr->second.GetClassType(), class_type) == 0 &&
			l_itr->second.IsEnabled())
		{
			if (strlen(string) + strlen(l_itr->second.GetFlagID()) > length)
			{
				break;
			}

			found_flag = true;
			strcat(string, l_itr->second.GetFlagID());
			strcat(string, " ");
		}
	}

	if (found_flag)	string[strlen(string) - 1] = '\0';
	return found_flag;
}

//********************************************************************************
// 
//********************************************************************************

GlobalGroupFlag	*GroupList::AddGroup(const char *class_type, const char *group_id)
{
	std::map<DualStriKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStriKey(class_type, group_id));
	if (itr != group_list.end())
	{
		return &itr->second;
	}

	group_list.insert(std::make_pair<DualStriKey, GlobalGroupFlag>(DualStriKey(class_type, group_id),GlobalGroupFlag()));
	return this->Find(class_type, group_id);
}

void	GroupList::RemoveGroup(const char *class_type, const char *group_id)
{
	std::map<DualStriKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStriKey(class_type, group_id));
	if (itr != group_list.end()) group_list.erase(itr);
}

GlobalGroupFlag	*GroupList::Find(const char *class_type, const char *group_id)
{
	std::map<DualStriKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStriKey(class_type, group_id));
	return ((itr != group_list.end()) ? &itr->second:NULL);
}

GlobalGroupFlag	*GroupList::FindFirst(const DualStriKey **ptr)
{
	l_itr = group_list.begin();
	if (l_itr != group_list.end())
	{
		*ptr = &l_itr->first;
		return &l_itr->second;
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*GroupList::FindNext(const DualStriKey **ptr)
{
	l_itr ++;
	if (l_itr != group_list.end())
	{
		*ptr = &l_itr->first;
		return &l_itr->second;
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*GroupList::FindFirst(const char *class_type, const DualStriKey **ptr)
{
	for (l_itr = group_list.begin(); l_itr != group_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return &(l_itr->second);
		}
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*GroupList::FindNext(const char *class_type, const DualStriKey **ptr)
{
	l_itr ++;
	for (;l_itr != group_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return &(l_itr->second);
		}
	}

	*ptr = NULL;
	return NULL;
}


//********************************************************************************
// 
//********************************************************************************

GlobalGroupFlag	*LevelList::AddGroup(const char *class_type, const int level_id)
{
	std::map<DualStrIntKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStrIntKey(class_type, level_id));
	if (itr != group_list.end())
	{
		return &itr->second;
	}

	group_list.insert(std::make_pair<DualStrIntKey, GlobalGroupFlag>(DualStrIntKey(class_type, level_id),GlobalGroupFlag()));
	return this->Find(class_type, level_id);
}

void	LevelList::RemoveGroup(const char *class_type, const int level_id)
{
	std::map<DualStrIntKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStrIntKey(class_type, level_id));
	if (itr != group_list.end()) 
	{
		group_list.erase(itr);
	}
}

GlobalGroupFlag	*LevelList::Find(const char *class_type, const int level_id)
{
	std::map<DualStrIntKey, GlobalGroupFlag>::iterator itr = group_list.find(DualStrIntKey(class_type, level_id));
	return ((itr != group_list.end()) ? &itr->second:NULL);
}

GlobalGroupFlag	*LevelList::FindFirst(const DualStrIntKey **ptr)
{
	l_itr = group_list.begin();
	if (l_itr != group_list.end())
	{
		*ptr = &l_itr->first;
		return &l_itr->second;
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*LevelList::FindNext(const DualStrIntKey **ptr)
{
	l_itr ++;
	if (l_itr != group_list.end())
	{
		*ptr = &l_itr->first;
		return &l_itr->second;
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*LevelList::FindFirst(const char *class_type, const DualStrIntKey **ptr)
{
	for (l_itr = group_list.begin(); l_itr != group_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return &(l_itr->second);
		}
	}

	*ptr = NULL;
	return NULL;
}

GlobalGroupFlag	*LevelList::FindNext(const char *class_type, const DualStrIntKey **ptr)
{
	l_itr ++;
	for (;l_itr != group_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return &(l_itr->second);
		}
	}

	*ptr = NULL;
	return NULL;
}

//********************************************************************************
// 
//********************************************************************************

void GroupSet::Add(const char *class_type, const char *group_id)
{
	group_set.insert(DualStriKey(class_type, group_id));
}

void GroupSet::Remove(const char *class_type, const char *group_id)
{
	std::set<DualStriKey>::iterator itr = group_set.find(DualStriKey(class_type, group_id));
	if (itr != group_set.end()) group_set.erase(itr);
}

bool GroupSet::Find(const char *class_type, const char *group_id)
{
	std::set<DualStriKey>::iterator itr = group_set.find(DualStriKey(class_type, group_id));
	return ((itr != group_set.end()) ? true:false);
}

const char *GroupSet::FindFirst(const char **ptr)
{
	l_itr = group_set.begin();
	if (l_itr != group_set.end())
	{
		*ptr = l_itr->key2;
		return l_itr->key1;
	}

	return NULL;
}

const char *GroupSet::FindNext(const char **ptr)
{
	l_itr ++;
	if (l_itr != group_set.end())
	{
		*ptr = l_itr->key2;
		return l_itr->key1;
	}

	return NULL;
}

const char *GroupSet::FindFirst(const char *class_type)
{
	for (l_itr = group_set.begin(); l_itr != group_set.end(); l_itr++)
	{
		if (strcmp(l_itr->key1, class_type) == 0)
		{
			return l_itr->key2;
		}
	}

	return NULL;
}

const char *GroupSet::FindNext(const char *class_type)
{
	l_itr ++;
	for (;l_itr != group_set.end(); l_itr++)
	{
		if (strcmp(l_itr->key1, class_type) == 0)
		{
			return l_itr->key2;
		}
	}

	return NULL;
}


//********************************************************************************
// 
//********************************************************************************

void LevelSet::Add(const char *class_type, const int level_id)
{
	group_set.insert(DualStrIntKey(class_type, level_id));
}

void	LevelSet::Remove(const char *class_type, const int level_id)
{
	std::set<DualStrIntKey>::iterator itr = group_set.find(DualStrIntKey(class_type, level_id));
	if (itr != group_set.end()) 
	{
		group_set.erase(itr);
	}
}

bool LevelSet::Find(const char *class_type, const int level_id)
{
	std::set<DualStrIntKey>::iterator itr = group_set.find(DualStrIntKey(class_type, level_id));
	return ((itr != group_set.end()) ? true:false);
}

const int LevelSet::FindFirst(const char **ptr)
{
	if (group_set.empty()) return -99999;
	l_itr = group_set.begin();
	if (l_itr != group_set.end())
	{
		*ptr = l_itr->key1;
		return l_itr->key2;
	}

	return -99999;
}

const int LevelSet::FindNext(const char **ptr)
{
	l_itr ++;
	if (l_itr != group_set.end())
	{
		*ptr = l_itr->key1;
		return l_itr->key2;
	}

	return -99999;
}

const int LevelSet::FindFirst(const char *class_type)
{
	for (l_itr = group_set.begin(); l_itr != group_set.end(); l_itr++)
	{
		if (strcmp(l_itr->key1, class_type) == 0)
		{
			return l_itr->key2;
		}
	}

	return -99999;
}

const int LevelSet::FindNext(const char *class_type)
{
	l_itr ++;
	for (;l_itr != group_set.end(); l_itr++)
	{
		if (strcmp(l_itr->key1, class_type) == 0)
		{
			return l_itr->key2;
		}
	}

	return -99999;
}

//********************************************************************************
// 
//********************************************************************************
bool FlagDescList::AddFlag(const char *class_type, const char *flag_id, const char *description, bool	replace_description)
{
	std::map<DualStrKey, FlagDesc>::iterator itr = flag_desc_list.find(DualStrKey(class_type, flag_id));
	if (itr != flag_desc_list.end())
	{
		if (replace_description)
		{
			itr->second.SetDesc(description);
			return true;
		}

		return false;
	}
	else
	{
		flag_desc_list.insert(std::make_pair<DualStrKey, FlagDesc>(DualStrKey(class_type, flag_id), FlagDesc(description)));
		class_type_list.Add(class_type);
	}

	return true;
}

void	FlagDescList::RemoveFlag(const char *class_type, const char *flag_id)
{
	std::map<DualStrKey, FlagDesc>::iterator itr = flag_desc_list.find(DualStrKey(class_type, flag_id));
	if (itr != flag_desc_list.end()) 
	{
		flag_desc_list.erase(itr);
	}
}

bool	FlagDescList::IsValidFlag(const char *class_type, const char *flag_id)
{
	std::map<DualStrKey, FlagDesc>::iterator itr = flag_desc_list.find(DualStrKey(class_type, flag_id));
	return ((itr != flag_desc_list.end()) ? true:false);
}

const char *FlagDescList::Find(const char *class_type, const char *flag_id)
{
	std::map<DualStrKey, FlagDesc>::iterator itr = flag_desc_list.find(DualStrKey(class_type, flag_id));
	return ((itr != flag_desc_list.end()) ? itr->second.description:NULL);
}

const char *FlagDescList::FindFirst(const DualStrKey **ptr)
{
	l_itr = flag_desc_list.begin();
	if (l_itr != flag_desc_list.end())
	{
		*ptr = &l_itr->first;
		return l_itr->second.description;
	}

	*ptr = NULL;
	return NULL;
}

const char *FlagDescList::FindNext(const DualStrKey **ptr)
{
	l_itr ++;
	if (l_itr != flag_desc_list.end())
	{
		*ptr = &l_itr->first;
		return l_itr->second.description;
	}

	*ptr = NULL;
	return NULL;
}

const char *FlagDescList::FindFirst(const char *class_type, const DualStrKey **ptr)
{
	for (l_itr = flag_desc_list.begin(); l_itr != flag_desc_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return l_itr->second.description;
		}
	}

	*ptr = NULL;
	return NULL;
}

const char *FlagDescList::FindNext(const char *class_type, const DualStrKey **ptr)
{
	l_itr ++;
	for (;l_itr != flag_desc_list.end(); l_itr++)
	{
		if (strcmp(l_itr->first.key1, class_type) == 0)
		{
			*ptr = &l_itr->first;
			return l_itr->second.description;
		}
	}

	*ptr = NULL;
	return NULL;
}

void	FlagDescList::DumpFlags()
{
	std::map<DualStrKey, FlagDesc>::iterator itr;
	for (itr = flag_desc_list.begin(); itr != flag_desc_list.end(); itr++)
	{
		Msg("[%s] [%s] [%s]\n", itr->first.key1, itr->first.key2, itr->second.description);
	}
}

void	FlagDescList::LoadFlags()
{
	char	core_filename[256];
	ManiKeyValues *kv_ptr;

	// Free flag desc list
	this->Kill();
	kv_ptr = new ManiKeyValues("flags.txt");
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/flags.txt", mani_path.GetString());

	if (!kv_ptr->ReadFile(core_filename))
	{
		MMsg("Failed to load %s\n", core_filename);
		kv_ptr->DeleteThis();
		return;
	}

	// flags.txt key
	read_t *rd_ptr = kv_ptr->GetPrimaryKey();
	if (!rd_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	// class_type key
	rd_ptr = kv_ptr->GetNextKey(rd_ptr);
	if (!rd_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	// start with actual class type names
	read_t *class_ptr = kv_ptr->GetNextKey(rd_ptr);
	if (!class_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	for (;;)
	{
		char *class_name = class_ptr->sub_key_name;
		kv_ptr->ResetKeyIndex();

		for (;;)
		{
			char *name = NULL;
			char *value = kv_ptr->GetNextKeyValue(&name);
			if (value == NULL)
			{
				// No more flags for this class
				break;
			}

			// Add flag to list with auto sort
			this->AddFlag(class_name, name, value);
		}

		class_ptr = kv_ptr->GetNextKey(rd_ptr);
		if (!class_ptr)
		{
			break;
		}
	}

	kv_ptr->DeleteThis();
	return;
}

void	FlagDescList::WriteFlags()
{
	char	core_filename[256];
	ManiKeyValues *flags;

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/flags.txt", mani_path.GetString());

	flags = new ManiKeyValues( "flags.txt" );

	if (!flags->WriteStart(core_filename))
	{
		MMsg("Failed to write %s\n", core_filename);
		delete flags;
		return;
	}

	flags->WriteComment("Do not edit this file!");
	flags->WriteCR();
	flags->WriteComment("These keys define each type of access class");
	flags->WriteNewSubKey("class_types");

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		flags->WriteNewSubKey((char *) c_type);
		flags->WriteComment("These keys define the flag id for the class and description");

		const DualStrKey *key_value = NULL;
		for (const char *desc = this->FindFirst(c_type, &key_value); desc != NULL; desc = this->FindNext(c_type, &key_value))
		{
			flags->WriteKey(key_value->key2, (char *) desc);
		}

		flags->WriteEndSubKey();
	}
			
	flags->WriteEndSubKey();
	flags->WriteEnd();
	delete flags;

	// Write HTML version

	FILE *filehandle = NULL;
	ManiFile *mf = new ManiFile();

	snprintf(core_filename, sizeof(core_filename), "./cfg/%s/data/flags_help.html", mani_path.GetString());
	filehandle = mf->Open(core_filename, "wt");
	if (filehandle == NULL)
	{
		delete mf;
		return;
	}


	const char *header_row_bg_colour = "#000080";
	const char *header_row_font_colour = "#ffffff";
	const char *cmd_col_bg_colour = "#4d4d4d";
	const char *cmd_col_font_colour = "#ffffff";
	const char *rest_bg_colour = "#cccccc";
	const char *rest_font_colour = "#000000";
	const char *border_colour = "#000000";

	fprintf(filehandle, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
	fprintf(filehandle, "<HTML>\n");
	fprintf(filehandle, "<HEAD>\n");
	fprintf(filehandle, "\t<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=utf-8\">\n");
	fprintf(filehandle, "\t<TITLE></TITLE>\n");
	fprintf(filehandle, "\t<META NAME=\"GENERATOR\" CONTENT=\"%s\">\n", PLUGIN_VERSION);
	fprintf(filehandle, "\t<META NAME=\"AUTHOR\" CONTENT=\"Mani\">\n");
	fprintf(filehandle, "</HEAD>\n");
	fprintf(filehandle, "<BODY LANG=\"en-GB\" DIR=\"LTR\">\n");
	fprintf(filehandle, "<P ALIGN=CENTER STYLE=\"page-break-before: always\"><FONT SIZE=4 STYLE=\"font-size: 16pt\"><B>Mani Admin Plugin Flags List %s</B></FONT></P>\n", PLUGIN_VERSION_ID2);
	fprintf(filehandle, "<TABLE WIDTH=100%% BORDER=1 BORDERCOLOR=\"%s\" CELLPADDING=4 CELLSPACING=0>\n", border_colour);
	fprintf(filehandle, "\t<COL WIDTH=43*>\n");
	fprintf(filehandle, "\t<COL WIDTH=61*>\n");
	fprintf(filehandle, "\t<COL WIDTH=60*>\n");

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
		fprintf(filehandle, "\t\t<TH WIDTH=17%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s : %s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3090)), c_type);
		fprintf(filehandle, "\t\t</TH>\n");
		fprintf(filehandle, "\t\t<TH WIDTH=24%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3091)));
		fprintf(filehandle, "\t\t</TH>\n");
		fprintf(filehandle, "\t\t\t<TH WIDTH=23%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3066)));
		fprintf(filehandle, "\t\t</TH>\n");
		fprintf(filehandle, "\t</TR>\n");

		const DualStrKey *key_value = NULL;
		for (const char *desc = this->FindFirst(c_type, &key_value); desc != NULL; desc = this->FindNext(c_type, &key_value))
		{
			fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
			fprintf(filehandle, "\t\t<TD WIDTH=17%% BGCOLOR=\"%s\">\n", cmd_col_bg_colour);
			fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
			fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", cmd_col_font_colour, c_type);
			fprintf(filehandle, "\t\t</TD>\n");

			fprintf(filehandle, "\t\t<TD WIDTH=24%% BGCOLOR=\"%s\">\n", rest_bg_colour);
			fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
			fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
				rest_font_colour, AsciiToHTML(key_value->key2));
			fprintf(filehandle, "\t\t</TD>\n");

			fprintf(filehandle, "\t\t<TD WIDTH=23%% BGCOLOR=\"%s\">\n", rest_bg_colour);
			fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
			fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n",
				rest_font_colour, desc);
			fprintf(filehandle, "\t\t</TD>\n");
			fprintf(filehandle, "\t</TR>\n");
		}

	}

	fprintf(filehandle, "</TABLE>\n<P><BR><BR>\n</P>\n</BODY>\n</HTML>");
	mf->Close(filehandle);
	delete mf;
}

StringSet class_type_list;
