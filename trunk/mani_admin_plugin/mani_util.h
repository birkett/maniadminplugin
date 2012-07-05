//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_UTIL_H
#define MANI_UTIL_H

#include <map>
#include <ctype.h>

extern	unsigned long djb2_hash(unsigned char *str);
extern	unsigned long djb2_hash(unsigned char *str1, unsigned char *str2);
extern	unsigned long elf_hash(char *str);
extern	unsigned long elf_hash(char *str1, char *str2);
extern	unsigned long sdbm_hash(unsigned char *str);
extern	unsigned long sdbm_hash(unsigned char *str1, unsigned char *str2);
extern	void		  MSleep(int   milliseconds);
extern  bool		  IsLAN();
extern  char	*AsciiToHTML(char *in_string);
extern  int		UTIL_GetWebVersion(const char *ip_address, const int port, const char *filename);
extern	void	UTIL_GetGamePath( char *path );
extern	bool	UTIL_ScanValveFile ( char *path, char *text );
extern	bool	UTIL_ScanFile ( char *path, char *text );
extern  void	UTIL_CleanID ( char *steamid );

class BasicStr
{
public:
	BasicStr() {str = NULL;len = 0;};
	~BasicStr() {if (str) free (str);}
	BasicStr(const char *str_ptr)
	{
		len = strlen(str_ptr);
		str = (char *) malloc(len + 1);
		strcpy(str, str_ptr);
	}

	BasicStr(const BasicStr &src)
	{
		len = strlen(src.str);
		str = (char *) malloc(len + 1);
		strcpy(str, src.str);
	}

	BasicStr &operator=(const BasicStr &right)
	{
		if (this->str) free (this->str);
		len = strlen(right.str);
		this->str = (char *) malloc(len + 1);
		strcpy(this->str, right.str);
		return *this;
	}

	BasicStr &operator=(const char *str_ptr)
	{
		if (this->str) free (this->str);
		len = strlen(str_ptr);
		this->str = (char *) malloc(len + 1);
		strcpy(this->str, str_ptr);
		return *this;
	}

	bool operator<(const BasicStr &right) const 
	{
		return (strcmp(right.str, str) < 0);
	}

	void	Set(const char *str_ptr)
	{
		if (str != NULL) free(str);
		len = strlen(str_ptr);
		str = (char *) malloc(len + 1);
		strcpy(str, str_ptr);
	}

	void	Upper() {if (str) for (int i = 0; str[i] != '\0'; i++) str[i] = toupper(str[i]);}
	void	Lower()	{if (str) for (int i = 0; str[i] != '\0'; i++) str[i] = tolower(str[i]);}

	void	Cat(const char *str_ptr)
	{
		int old_len = len;
		int	str_len = strlen(str_ptr);
		len += str_len;
		str = (char *) realloc(str, len + 1);
		for (int i = 0; i <= str_len; i++)
		{
			str[i + old_len] =  str_ptr[i];
		}
	}

	void	Cat(const BasicStr &right)
	{
		int old_len = len;
		int	str_len = right.len;
		len += str_len;
		str = (char *) realloc(str, len + 1);
		for (int i = 0; i <= str_len; i++)
		{
			str[i + old_len] =  right.str[i];
		}
	}

	char *str;
	int	 len;
};

//************************************************************************
// Param Manager class allows parameters to be added to a list by name and
// type. The parameters can then be retrieved by name too
//************************************************************************
class ParamManager
{
public:
	ParamManager () {};
	~ParamManager () {};
	void	AddParam(const char *id, const int param) {int_list.insert(std::make_pair<BasicStr, int>(BasicStr(id), param));}
	void	AddParam(const char *id, const char *param) {str_list.insert(std::make_pair<BasicStr, BasicStr>(BasicStr(id), BasicStr(param)));}
	// Separate version for ... version
	void	AddParamVar(const char *id, const char *param, ...) 
	{
		char temp_str[2048];
		va_list		argptr;

		va_start ( argptr, param );
		vsnprintf( temp_str, sizeof(temp_str), param, argptr );
		va_end   ( argptr );
		str_list.insert(std::make_pair<BasicStr, BasicStr>(BasicStr(id), BasicStr(temp_str)));
	}
	void	AddParam(const char *id, const float param) {float_list.insert(std::make_pair<BasicStr, float>(BasicStr(id), param));}
	void	AddParam(const char *id, const bool param) {bool_list.insert(std::make_pair<BasicStr, bool>(BasicStr(id), param));}
	void	AddParam(const char *id, void *param) {ptr_list.insert(std::make_pair<BasicStr, void *>(BasicStr(id), param));}
	bool	GetParam(const char *id, char **param)
	{
		std::map<BasicStr, BasicStr>::iterator itr = str_list.find(BasicStr(id));
		if (itr != str_list.end())
		{
			*param = itr->second.str;
			return true;
		}

		return false;
	}

	bool	GetParam(const char *id, int *param)
	{
		std::map<BasicStr, int>::iterator itr = int_list.find(BasicStr(id));
		if (itr != int_list.end())
		{
			*param = itr->second;
			return true;
		}

		return false;
	}

	bool	GetParam(const char *id, float *param)
	{
		std::map<BasicStr, float>::iterator itr = float_list.find(BasicStr(id));
		if (itr != float_list.end())
		{
			*param = itr->second;
			return true;
		}

		return false;
	}

	bool	GetParam(const char *id, bool *param)
	{
		std::map<BasicStr, bool>::iterator itr = bool_list.find(BasicStr(id));
		if (itr != bool_list.end())
		{
			*param = itr->second;
			return true;
		}

		return false;
	}

	bool	GetParam(const char *id, void **param)
	{
		std::map<BasicStr, void *>::iterator itr = ptr_list.find(BasicStr(id));
		if (itr != ptr_list.end())
		{
			*param = itr->second;
			return true;
		}

		return false;
	}

private:
	std::map<BasicStr, BasicStr> str_list;
	std::map<BasicStr, int> int_list;
	std::map<BasicStr, float> float_list;
	std::map<BasicStr, bool> bool_list;
	std::map<BasicStr, void *> ptr_list;
};



#endif
