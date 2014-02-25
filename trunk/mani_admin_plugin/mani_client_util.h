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



#ifndef MANI_CLIENT_UTIL_H
#define MANI_CLIENT_UTIL_H

#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include "mani_output.h"
#include "mani_util.h"

class DualStrKey
{
public:
	DualStrKey(const char *k1, const char *k2)
	{
		key1 = (char *) malloc(Q_strlen(k1) + 1);
		strcpy(key1, k1);
		key2 = (char *) malloc(Q_strlen(k2) + 1);
		strcpy(key2, k2);
	}

	DualStrKey(const DualStrKey &src)
	{
		key1 = (char *) malloc(Q_strlen(src.key1) + 1);
		strcpy(key1, src.key1);
		key2 = (char *) malloc(Q_strlen(src.key2) + 1);
		strcpy(key2, src.key2);
	}

	~DualStrKey()
	{
		free(key1);
		free(key2);
	}
		
	bool operator<(const DualStrKey &right) const 
	{
		int s1 = strcmp(key1, right.key1);
		if (s1 < 0) return true;
		int s2 = strcmp(key2, right.key2);
		if (s1 == 0 && s2 < 0) return true;

		return false;
	}

	char *key1;
	char *key2;
};

// Same as DualStrKey but with case insensitive search for key2
// Use this for client groups
class DualStriKey
{
public:
	DualStriKey(const char *k1, const char *k2)
	{
		key1 = (char *) malloc(Q_strlen(k1) + 1);
		strcpy(key1, k1);
		key2 = (char *) malloc(Q_strlen(k2) + 1);
		strcpy(key2, k2);
	}

	DualStriKey(const DualStriKey &src)
	{
		key1 = (char *) malloc(Q_strlen(src.key1) + 1);
		strcpy(key1, src.key1);
		key2 = (char *) malloc(Q_strlen(src.key2) + 1);
		strcpy(key2, src.key2);
	}

	~DualStriKey()
	{
		free(key1);
		free(key2);
	}
		
	bool operator<(const DualStriKey &right) const 
	{
		int s1 = strcmp(key1, right.key1);
		if (s1 < 0) return true;
		// Do case insensitive search
		int s2 = stricmp(key2, right.key2);
		if (s1 == 0 && s2 < 0) return true;

		return false;
	}

	char *key1;
	char *key2;
};

class DualStrIntKey
{
public:
	DualStrIntKey(const char *k1, const int k2)
	{
		key1 = (char *) malloc(Q_strlen(k1) + 1);
		strcpy(key1, k1);
		key2 = k2;
	}

	DualStrIntKey(const DualStrIntKey &src)
	{
		key1 = (char *) malloc(Q_strlen(src.key1) + 1);
		strcpy(key1, src.key1);
		key2 = src.key2;
	}

	~DualStrIntKey()
	{
		free(key1);
	}
		
	bool operator<(const DualStrIntKey &right) const 
	{
		int s1 = strcmp(key1, right.key1);
		if (s1 < 0) return true;;
		if (s1 == 0 && key2 < right.key2) return true;
		return false;
	}

	char *key1;
	int	 key2;
};

class FlagAccess
{
public:
	FlagAccess() {flag_id = NULL;}
	FlagAccess(const char *flag_id_ptr) 
	{
		flag_id = (char *) malloc(strlen(flag_id_ptr) + 1); 
		strcpy(flag_id, flag_id_ptr);
	}
	FlagAccess(const FlagAccess &src)
	{
		flag_id = (char *) malloc(Q_strlen(src.flag_id) + 1);
		strcpy(flag_id, src.flag_id);
	}
	FlagAccess &operator=(const FlagAccess &right)
	{
		if (this->flag_id) free (this->flag_id);
		this->flag_id = (char *) malloc(strlen(right.flag_id) + 1);
		strcpy(this->flag_id, right.flag_id);
		return *this;
	}
	~FlagAccess() {if (flag_id) free(flag_id);}
	const char *GetFlagID() {return flag_id;}
private:
	char *flag_id;
};

class FlagAccessSwitch : public FlagAccess
{
public:
	FlagAccessSwitch():FlagAccess() {enabled = true;}
	FlagAccessSwitch(const char *flag_id):FlagAccess(flag_id) {enabled = true;}
	FlagAccessSwitch(const char *flag_id, const bool enabled):FlagAccess(flag_id) {this->enabled = enabled;}
	FlagAccessSwitch(const FlagAccessSwitch &src) : FlagAccess(src)
	{
		enabled = src.enabled;
	}
	FlagAccessSwitch &operator=(const FlagAccessSwitch &right)
	{
		enabled = right.enabled;
		return *this;
	}

	~FlagAccessSwitch(){};
	bool		IsEnabled() {return enabled;}
	void		SetEnabled(bool enabled) {this->enabled = enabled;}
private:
	bool enabled;
};

class ClassFlagAccess : public FlagAccessSwitch
{
public:
	ClassFlagAccess():FlagAccessSwitch(){class_type = NULL;}
	ClassFlagAccess(const char *class_type_ptr, const char *flag_id):FlagAccessSwitch(flag_id)
	{
		class_type = (char *) malloc(strlen(class_type_ptr) + 1);
		strcpy(class_type, class_type_ptr);
	}

	ClassFlagAccess(const char *class_type_ptr, const char *flag_id, const bool enabled):FlagAccessSwitch(flag_id, enabled)
	{
		class_type = (char *) malloc(strlen(class_type_ptr) + 1);
		strcpy(class_type, class_type_ptr);
	}

	ClassFlagAccess(const ClassFlagAccess &src) : FlagAccessSwitch(src)
	{
		class_type = (char *) malloc(strlen(src.class_type) + 1);
		strcpy(class_type, src.class_type);
	}

	ClassFlagAccess &operator=(const ClassFlagAccess &right)
	{
		if (this->class_type) free (this->class_type);
		this->class_type = (char *) malloc(strlen(right.class_type) + 1);
		strcpy(this->class_type, right.class_type);
		return *this;
	}

	~ClassFlagAccess(){if (class_type) free(class_type);}
	const char *GetClassType() {return class_type;}
private:
	char *class_type;
};

struct ltint
{
	bool operator()(const unsigned int i1, const unsigned int i2) const
	{
		return (i1 < i2);
	}
};

class GlobalGroupFlag
{
public:
	GlobalGroupFlag();
	~GlobalGroupFlag();

	void	Kill() {if (!flag_list.empty()) flag_list.clear();}
	void	AddFlag(const char *flag_id);
	void	SetFlag(const char *flag_id, const bool enable);
	bool	IsFlagSet(const char *flag_id);
	bool	CatFlags(char *string);
	bool	CatFlags(char *string, const unsigned int length, const bool start);
	const char	*FindFirst();
	const char  *FindNext();


private:
	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator l_itr;
	std::multimap<const unsigned int, FlagAccessSwitch, ltint>::iterator l_itr2;
	std::multimap<const unsigned int, FlagAccessSwitch, ltint> flag_list;
};

class FlagDesc
{
public:
	FlagDesc() {description = NULL;}
	FlagDesc(const char *desc) 
	{
		description = (char *) malloc(Q_strlen(desc) + 1);
		strcpy(description, desc);
	}

	~FlagDesc() {if (description) free(description);}

	FlagDesc(const FlagDesc &src)
	{
		description = (char *) malloc(Q_strlen(src.description) + 1);
		strcpy(description, src.description);
	}

	void SetDesc(const char *desc)
	{
		if (description) free (description);
		description = (char *) malloc(Q_strlen(desc) + 1);
		strcpy(description, desc);
	}

	char *description;
};

class FlagDescList
{
public:
	FlagDescList() {};
	~FlagDescList() {};
	void Kill() {if (!flag_desc_list.empty()) flag_desc_list.clear();}
	bool AddFlag(const char *class_type, const char *flag_id, const char *description, bool	replace_description = true);
	void RemoveFlag(const char *class_type, const char *flag_id);
	bool IsValidFlag(const char *class_type, const char *flag_id);
	const char *Find(const char *class_type, const char *flag_id);
	const char *FindFirst(const DualStrKey **ptr);
	const char *FindNext(const DualStrKey **ptr);
	const char *FindFirst(const char *class_type, const DualStrKey **ptr);
	const char *FindNext(const char *class_type,  const DualStrKey **ptr);
	void	LoadFlags();
	void	WriteFlags();
	void	DumpFlags();

private:
	std::map<DualStrKey, FlagDesc>::iterator l_itr;
	std::map<DualStrKey, FlagDesc> flag_desc_list;
};

class PersonalFlag
{
public:
	PersonalFlag();
	~PersonalFlag();

	void	Kill() {if (!flag_list.empty()) flag_list.clear();}
	void	AddFlag(const char *class_type, const char *flag_id);
	void	SetFlag(const char *class_type, const char *flag_id, const bool enable);
	void	SetAll(const char *class_type, FlagDescList *flag_list_ptr);
	bool	IsFlagSet(const char *class_type, const char *flag_id);
	bool	CatFlags(char *string, const char *class_type);
	bool	CatFlags(char *string, const char *class_type, const unsigned int length, const bool start);
	void	Copy(PersonalFlag &src);

private:
	std::multimap<const unsigned int, ClassFlagAccess, ltint>::iterator l_itr;
	std::multimap<const unsigned int, ClassFlagAccess, ltint> flag_list;
};

class GroupList
{
public:
	GroupList() {};
	~GroupList() {};
	void Kill() {if (!group_list.empty()) group_list.clear();}
	GlobalGroupFlag *AddGroup(const char *class_type, const char *group_id);
	void RemoveGroup(const char *class_type, const char *group_id);
	GlobalGroupFlag *Find(const char *class_type, const char *group_id);
	GlobalGroupFlag *FindFirst(const DualStriKey **ptr);
	GlobalGroupFlag *FindNext(const DualStriKey **ptr);

	GlobalGroupFlag *FindFirst(const char *class_type, const DualStriKey **ptr);
	GlobalGroupFlag	*FindNext(const char *class_type, const DualStriKey **ptr);

private:
	std::map<DualStriKey, GlobalGroupFlag>::iterator l_itr;
	std::map<DualStriKey, GlobalGroupFlag> group_list;
};


class LevelList
{
public:
	LevelList() {};
	~LevelList() {};
	void Kill() {if (!group_list.empty()) group_list.clear();}
	GlobalGroupFlag *AddGroup(const char *class_type, const int level_id);
	void RemoveGroup(const char *class_type, const int level_id);
	GlobalGroupFlag *Find(const char *class_type, const int level_id);
	GlobalGroupFlag *FindFirst(const DualStrIntKey **ptr);
	GlobalGroupFlag *FindNext(const DualStrIntKey **ptr);

	GlobalGroupFlag *FindFirst(const char *class_type, const DualStrIntKey **ptr);
	GlobalGroupFlag	*FindNext(const char *class_type, const DualStrIntKey **ptr);

private:
	std::map<DualStrIntKey, GlobalGroupFlag>::iterator l_itr;
	std::map<DualStrIntKey, GlobalGroupFlag> group_list;
};

class GroupSet
{
public:
	GroupSet() {};
	~GroupSet() {};
	bool IsEmpty() {return group_set.empty();}
	void Kill() {if (!group_set.empty()) group_set.clear();}
	void Add(const char *class_type, const char *group_id);
	void Remove(const char *class_type, const char *group_id);
	bool Find(const char *class_type, const char *group_id);
	const char *FindFirst(const char **ptr);
	const char *FindNext(const char **ptr);

	const char *FindFirst(const char *class_type);
	const char *FindNext(const char *class_type);

private:
	std::set<DualStriKey>::iterator l_itr;
	std::set<DualStriKey> group_set;
};


class LevelSet
{
public:
	LevelSet() {};
	~LevelSet() {};
	bool IsEmpty() {return group_set.empty();}
	void Kill() {if (!group_set.empty()) group_set.clear();}
	void Add(const char *class_type, const int level_id);
	void Remove(const char *class_type, const int level_id);
	bool Find(const char *class_type, const int level_id);
	const int FindFirst(const char **ptr);
	const int FindNext(const char **ptr);

	const int FindFirst(const char *class_type);
	const int FindNext(const char *class_type);

private:
	std::set<DualStrIntKey>::iterator l_itr;
	std::set<DualStrIntKey> group_set;
};




struct ltstr { bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; } };

class StringSet
{
public:
	StringSet() {};
	~StringSet() {};

	int		Size() {return (int) string_list.size();}
	bool	IsEmpty() {return string_list.empty();}
	void	Kill() {if (!string_list.empty()) string_list.clear();}
	void	Add(const char *str) {string_list.insert(str);}
	void	Remove(const char *str) {string_list.erase(str);}
	bool	Find(const char *str)
	{
		std::set<BasicStr> ::iterator itr = string_list.find(str);
		return ((itr != string_list.end()) ? true:false);
	}

	const char *FindFirst()
	{
		l_itr = string_list.begin();
		return ((l_itr != string_list.end()) ? l_itr->str:NULL);
	}

	const char *FindNext()
	{
		l_itr++;
		if (l_itr != string_list.end())
		{
			return (const char *) l_itr->str;
		}

		return NULL;
	}

private:
	std::set<BasicStr>::iterator l_itr;
	std::set<BasicStr> string_list;
};

extern StringSet class_type_list;

#endif

