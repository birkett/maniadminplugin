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



#ifndef MANI_KEYVALUES_H
#define MANI_KEYVALUES_H

enum
{
	M_COMMENT				= 0,
	M_KEY,
	M_KEYVALUE,
	M_ENDSUB
};

struct	kv_key_t
{
	char	*key_name;
	char	*key_value;
	kv_key_t	*next_key;
};

struct	kv_sub_key_t
{
	kv_sub_key_t		 *next_key;
	struct read_t		 *sub_key;
};

struct	read_t
{
	char	 *sub_key_name;
		
	kv_key_t		*key_list;
	kv_key_t		*last_key;

	kv_sub_key_t	*sub_key_list;
	kv_sub_key_t	*last_sub_key;
	kv_sub_key_t	*cur_sub_key;
};

class ManiKeyValues
{
public:



	ManiKeyValues();
	~ManiKeyValues();
	ManiKeyValues(char *title_str);

	// Writer functions
	bool	WriteStart(char *filename);
	void	SetIndent(int indent_level_size) {indent_level = indent_level_size;}
	bool	WriteNewSubKey(char *sub_key);
	bool	WriteNewSubKey(int  sub_key);
	bool	WriteNewSubKey(unsigned int  sub_key);
	bool	WriteNewSubKey(float  sub_key);
	bool	WriteEndSubKey(void);
	bool	WriteKey(char *key_name, char *value);
	bool	WriteKey(char *key_name, int value);
	bool	WriteKey(char *key_name, unsigned int value);
	bool	WriteKey(char *key_name, float value);
	bool	WriteComment(char *comment);
	bool	WriteCR(void);
	bool	WriteEnd(void);
	
	bool	ReadFile(char *filename);
	void	DeleteThis(void);

	// Returns the first key (usually the filename entry)
	read_t	*GetPrimaryKey(void);

	// Find the next sub key (pointer to sub key)
	read_t	*GetNextKey(read_t *read_ptr);

	// Find a sub key
	read_t	*FindKey(read_t *read_ptr, char *name);

	float	GetFloat(char *name, float init);
	int		GetInt(char *name, int init);
	char	*GetString(char *name, char *init);

private:

	kv_key_t	*FindKeyVal(char *name);
	bool		ReadLine(char *key, int *k_length, char *value, int *v_length, int *type);
	bool		RecursiveLoad(read_t *ptr);
	void		Destroy(read_t *ptr);

	char	buffer[2048];

	FileHandle_t fh;

	// Write side of things
	void	SetupIndentLevels(void);
	int		indent_level;
	char	title[256];
	char	indent_levels[21][21];
	int		current_indent;

	// Read side of things
	read_t	core_read;
	read_t	*current_read;
	char	key_buffer[2048];
	char	key_values_buffer[2048];
};

#endif
