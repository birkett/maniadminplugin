//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
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
};

struct	read_t
{
	char	 *sub_key_name;
		
	kv_key_t *key_list;
	int		 key_list_size;
	int		 key_list_allocated_size;

    read_t	 *sub_key_list;
	int		 sub_key_list_size;
	int		 sub_key_list_allocated_size;

	int		 current_sub_key;
	int		 current_key;
};

class ManiKeyValues
{
public:
	ManiKeyValues();
	~ManiKeyValues();
	ManiKeyValues(const char *title_str);

	// Writer functions
	bool	WriteStart(const char *filename);
	void	SetIndent(int indent_level_size) {indent_level = indent_level_size;}
	bool	WriteNewSubKey(const char *sub_key);
	bool	WriteNewSubKey(const int  sub_key);
	bool	WriteNewSubKey(const unsigned int  sub_key);
	bool	WriteNewSubKey(const float  sub_key);
	bool	WriteEndSubKey(void);
	bool	WriteKey(const char *key_name, const char *value);
	bool	WriteKey(const char *key_name, const int value);
	bool	WriteKey(const char *key_name, const unsigned int value);
	bool	WriteKey(const char *key_name, const float value);
	bool	WriteComment(const char *comment);
	bool	WriteCR(void);
	bool	WriteEnd(void);
	
	void	SetKeySize(int trigger, int block_size) {key_size_trigger = trigger; key_size_block = block_size;}
	void	SetKeyPairSize(int trigger, int block_size) {key_pair_size_trigger = trigger; key_pair_size_block = block_size;}
	bool	ReadFile(char *filename);
	void	DeleteThis(void);

	// Returns the first key (usually the filename entry)
	read_t	*GetPrimaryKey(void);

	// Find the next sub key (pointer to sub key)
	read_t	*GetNextKey(read_t *read_ptr);

	char	*GetNextKeyValue(char **name);
	void	ResetKeyIndex(void);

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
	char		*FastMalloc(int bytes);

	char	buffer[2048];

	FILE	*fh;

	// Write side of things
	void	SetupIndentLevels(void);
	int		indent_level;
	char	title[256];
	char	indent_levels[21][21];
	int		current_indent;

	// Read side of things
	read_t	core_read;
	read_t	*current_read;

	int		key_pair_size_trigger;
	int		key_pair_size_block;
	int		key_size_trigger;
	int		key_size_block;

	struct	fmalloc_t
	{
		char	*block;
		int		index;
	};

	fmalloc_t	*fmalloc_list;
	int			fmalloc_list_size;

	char	key_buffer[2048];
	char	key_values_buffer[2048];

	//float	readline_timer;
	//float	struct_timer;

};

#endif
