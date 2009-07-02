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
#include <math.h>
#include <time.h>
#include "interface.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_callback_sourcemm.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_output.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_file.h"
#include "mani_keyvalues.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiKeyValues::ManiKeyValues(char *title_str)
{
	strcpy(title, title_str);
	indent_level = 1;
	this->SetupIndentLevels();
}

ManiKeyValues::ManiKeyValues()
{
	strcpy(title, "");
	indent_level = 1;
	this->SetupIndentLevels();
}

ManiKeyValues::~ManiKeyValues()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Create Indent Levels
//---------------------------------------------------------------------------------
void	ManiKeyValues::SetupIndentLevels(void)
{
	for (int i = 0; i < 20; i ++)
	{
		for (int j = 0; j < i; j++)
		{
			// Tab
			indent_levels[i][j] = 0x09;
		}

		indent_levels[i][i] = '\0';
	}

	core_read.current_sub_key = 0;
	core_read.key_list = NULL;
	core_read.key_list_size = 0;
	core_read.sub_key_list = NULL;
	core_read.sub_key_list_size = 0;
	core_read.sub_key_name = NULL;

	key_pair_size_trigger = 0;
	key_pair_size_block = 0;
	key_size_trigger = 0;
	key_size_block = 0;

	fmalloc_list = NULL;
	fmalloc_list_size = 0;

//	readline_timer = 0;
//	struct_timer = 0; 
}

//---------------------------------------------------------------------------------
// Purpose: Open up a file for writing
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteStart(char *filename)
{
	current_indent = 0;
	fh = gpManiFile->Open(filename,"wb");
	if (fh == NULL) return false;

	if (!this->WriteNewSubKey(title))
	{
		gpManiFile->Close(fh);
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new sub key
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteNewSubKey(char *sub_key)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%s\"\n%s{\n", 
							indent_levels[current_indent], 
							sub_key, 
							indent_levels[current_indent]);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	current_indent += indent_level;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new sub key
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteNewSubKey(int sub_key)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%i\"\n%s{\n", 
							indent_levels[current_indent], 
							sub_key, 
							indent_levels[current_indent]);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	current_indent += indent_level;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new sub key
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteNewSubKey(unsigned int sub_key)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%i\"\n%s{\n", 
							indent_levels[current_indent], 
							sub_key, 
							indent_levels[current_indent]);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	current_indent += indent_level;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new sub key
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteNewSubKey(float sub_key)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%f\"\n%s{\n", 
							indent_levels[current_indent], 
							sub_key, 
							indent_levels[current_indent]);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	current_indent += indent_level;
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new end sub key
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteEndSubKey(void)
{
	int	bytes;

	current_indent -= indent_level;
	bytes = Q_snprintf(buffer,sizeof(buffer), "%s}\n", 
							indent_levels[current_indent]);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new key and name
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteKey(char *key_name, char *value)
{
	int	bytes;

	if (value[0] == '\0') return true;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%s\"\t\"%s\"\n", 
							indent_levels[current_indent], 
							key_name, 
							value
							);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new key and name
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteKey(char *key_name, int value)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%s\"\t\"%i\"\n", 
							indent_levels[current_indent], 
							key_name, 
							value
							);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new key and name
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteKey(char *key_name, unsigned int value)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%s\"\t\"%i\"\n", 
							indent_levels[current_indent], 
							key_name, 
							value
							);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new key and name
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteKey(char *key_name, float value)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s\"%s\"\t\"%f\"\n", 
							indent_levels[current_indent], 
							key_name, 
							value
							);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a new comment
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteComment(char *comment)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "%s// %s\n", 
							indent_levels[current_indent], 
							comment
							);
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Write a line feed
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteCR(void)
{
	int	bytes;

	bytes = Q_snprintf(buffer,sizeof(buffer), "\n");
	if (bytes == 0) 
	{
		return false;
	}

	gpManiFile->Write(buffer, bytes, fh);
	return true;
}
//---------------------------------------------------------------------------------
// Purpose: End the key values file and close up
//---------------------------------------------------------------------------------
bool	ManiKeyValues::WriteEnd(void)
{
	// Close the last indent
	if (!this->WriteEndSubKey())
	{
		return false;
	}

	gpManiFile->Close(fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Get Primary Key
//---------------------------------------------------------------------------------
read_t	*ManiKeyValues::GetPrimaryKey(void)
{
	current_read = &core_read;
	return (read_t *) current_read;
}

//---------------------------------------------------------------------------------
// Purpose: Get Next sibling key
//---------------------------------------------------------------------------------
read_t	*ManiKeyValues::GetNextKey(read_t *read_ptr)
{
	if (read_ptr->sub_key_list_size == 0) return NULL;
	if (read_ptr->current_sub_key == read_ptr->sub_key_list_size) return NULL;

	current_read = &(read_ptr->sub_key_list[read_ptr->current_sub_key ++]);
	return (read_t *) current_read;
}

//---------------------------------------------------------------------------------
// Purpose: Find a sibling key
//---------------------------------------------------------------------------------
read_t	*ManiKeyValues::FindKey(read_t *read_ptr, char *name)
{
	if (read_ptr->sub_key_list_size == 0) return NULL;

	for (int i = 0; i < read_ptr->sub_key_list_size; i ++)
	{
		if (strcmp(read_ptr->sub_key_list[i].sub_key_name, name) == 0)
		{
			read_ptr->sub_key_list[i].current_sub_key = 0;
			current_read = &(read_ptr->sub_key_list[i]);
			return (read_t *) current_read;
		}
	}

	return (read_t *) NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Find a sibling key
//---------------------------------------------------------------------------------
kv_key_t	*ManiKeyValues::FindKeyVal(char *name)
{
	if (current_read->key_list_size == 0) return NULL;

	for (int i = 0; i < current_read->key_list_size; i ++)
	{
		if (strcmp(current_read->key_list[i].key_name, name) == 0)
		{
			return (kv_key_t *) &(current_read->key_list[i]);
		}
	}

	return (kv_key_t *) NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Find float var
//---------------------------------------------------------------------------------
float	ManiKeyValues::GetFloat(char *name, float init)
{
	kv_key_t *ptr = FindKeyVal(name);

	if (ptr == NULL)
	{
		return init;
	}
	
	return Q_atof(ptr->key_value);
}

//---------------------------------------------------------------------------------
// Purpose: Find int var
//---------------------------------------------------------------------------------
int	ManiKeyValues::GetInt(char *name, int init)
{
	kv_key_t *ptr = FindKeyVal(name);

	if (ptr == NULL)
	{
		return init;
	}
	
	return Q_atoi(ptr->key_value);
}

//---------------------------------------------------------------------------------
// Purpose: Find char *var
//---------------------------------------------------------------------------------
char *ManiKeyValues::GetString(char *name, char *init)
{
	kv_key_t *ptr = FindKeyVal(name);

	if (ptr == NULL)
	{
		return init;
	}
	
	return ptr->key_value;
}

//---------------------------------------------------------------------------------
// Purpose: Find char *var
//---------------------------------------------------------------------------------
bool	ManiKeyValues::ReadFile(char *filename)
{

	// Open up the file
	fh = gpManiFile->Open(filename,"rb");
	if (fh == NULL) 
	{
		MMsg("Failed to open %s\n", filename);
		return false;
	}

	char	key[128];
	char	value[128];
	int		k_length;
	int		v_length;
	int		type;



	// Get the primary key
	if (!this->ReadLine(key, &k_length, value, &v_length, &type))
	{
		MMsg("Bad line 1 in %s\n", filename);
		gpManiFile->Close(fh);
		return false;
	}

	if (type != M_KEY)
	{
		MMsg("Invalid primary key in %s\n", filename);
		gpManiFile->Close(fh);
		return false;
	}

	k_length ++;
	core_read.sub_key_name = (char *) FastMalloc(sizeof(char) * k_length);
	strcpy(core_read.sub_key_name, key);

	if (!this->RecursiveLoad(&core_read))
	{
        gpManiFile->Close(fh);
		return false;
	}

    gpManiFile->Close(fh);
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Clean up read structures
//---------------------------------------------------------------------------------
void	ManiKeyValues::DeleteThis(void)
{
	Destroy(&core_read);
	if (fmalloc_list_size != 0)
	{
		for (int i = 0; i < fmalloc_list_size; i ++)
		{
			free(fmalloc_list[i].block);
		}

		free(fmalloc_list);
	}

	delete this;
}

//---------------------------------------------------------------------------------
// Purpose: Clean up read structures
//---------------------------------------------------------------------------------
void	ManiKeyValues::Destroy(read_t *ptr)
{
//	for (int i = 0; i != ptr->key_list_size; i++)
//	{
//		free(ptr->key_list[i].key_name);
//		free(ptr->key_list[i].key_value);
//	}

	if (ptr->key_list_size != 0)
	{
		free(ptr->key_list);
	}

	for (int i = 0; i != ptr->sub_key_list_size; i++)
	{
		Destroy(&ptr->sub_key_list[i]);
	}

	if (ptr->sub_key_name)
	{
//		free(ptr->sub_key_name);
	}
}
//---------------------------------------------------------------------------------
// Purpose: Find a key and value, return type of 
// M_COMMENT, M_KEY or M_KEYVALUE if false then end of file
//---------------------------------------------------------------------------------
bool	ManiKeyValues::RecursiveLoad(read_t *ptr)
{
	static	int		k_length;
	static	int		v_length;
	static	int		type;
	static	int		index;
	static	read_t	*k_ptr;
	static	kv_key_t *kp_ptr;

	for(;;)
	{
		if (!this->ReadLine(key_buffer, &k_length, key_values_buffer, &v_length, &type))
		{
			// No more lines
			return true;
		}

		if (type == M_KEYVALUE)
		{
			// Key/Value pair found
			if (key_pair_size_trigger != 0)
			{
				if (ptr->key_list_size < key_pair_size_trigger)
				{
					// Normal
					AddToList((void **) &(ptr->key_list), sizeof(kv_key_t), &(ptr->key_list_size));
					ptr->key_list_allocated_size = ptr->key_list_size;
					index = ptr->key_list_size - 1;
					kp_ptr = &(ptr->key_list[ptr->key_list_size - 1]);
					kp_ptr->key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
					kp_ptr->key_value = (char *) FastMalloc(sizeof(char) * ++v_length);
					strcpy(kp_ptr->key_name, key_buffer);
					strcpy(kp_ptr->key_value, key_values_buffer);
				}
				else
				{
					if (ptr->key_list_size < ptr->key_list_allocated_size)
					{
						ptr->key_list_size ++;
					}
					else
					{
						// Add block to memory list
						index = ptr->key_list_size;
						ptr->key_list_size += key_pair_size_block;
						AddToList((void **) &(ptr->key_list), sizeof(kv_key_t), &(ptr->key_list_size));
						ptr->key_list_allocated_size = ptr->key_list_size;
						ptr->key_list_size = index + 1;
					}

					index = ptr->key_list_size - 1;
					kp_ptr = &(ptr->key_list[ptr->key_list_size - 1]);
					kp_ptr->key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
					kp_ptr->key_value = (char *) FastMalloc(sizeof(char) * ++v_length);
					strcpy(kp_ptr->key_name, key_buffer);
					strcpy(kp_ptr->key_value, key_values_buffer);
				}
			}
			else
			{
				// Normal
				AddToList((void **) &(ptr->key_list), sizeof(kv_key_t), &(ptr->key_list_size));
				ptr->key_list_allocated_size = ptr->key_list_size;
				index = ptr->key_list_size - 1;
				kp_ptr = &(ptr->key_list[ptr->key_list_size - 1]);
				kp_ptr->key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
				kp_ptr->key_value = (char *) FastMalloc(sizeof(char) * ++v_length);
				strcpy(kp_ptr->key_name, key_buffer);
				strcpy(kp_ptr->key_value, key_values_buffer);
			}
			continue;
		}

		if (type == M_KEY)
		{
			// Key found
			if (key_size_trigger != 0)
			{
				if (ptr->sub_key_list_size < key_size_trigger)
				{
					// Normal
					AddToList((void **) &(ptr->sub_key_list), sizeof(read_t), &(ptr->sub_key_list_size));
					ptr->sub_key_list_allocated_size = ptr->sub_key_list_size;
					index = ptr->sub_key_list_size - 1;
					k_ptr = &(ptr->sub_key_list[ptr->sub_key_list_size - 1]);

					k_ptr->sub_key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
					strcpy(k_ptr->sub_key_name, key_buffer);
					k_ptr->current_sub_key = 0;
					k_ptr->sub_key_list_size = 0;
					k_ptr->key_list_size = 0;
					k_ptr->sub_key_list = NULL;
					k_ptr->key_list = NULL;
				}
				else
				{
					if (ptr->sub_key_list_size < ptr->sub_key_list_allocated_size)
					{
						ptr->sub_key_list_size ++;
					}
					else
					{
						// Add block to memory list
						index = ptr->sub_key_list_size;
						ptr->sub_key_list_size += key_size_block;
						AddToList((void **) &(ptr->sub_key_list), sizeof(read_t), &(ptr->sub_key_list_size));
						ptr->sub_key_list_allocated_size = ptr->sub_key_list_size;
						ptr->sub_key_list_size = index + 1;
					}

					index = ptr->sub_key_list_size - 1;
					k_ptr = &(ptr->sub_key_list[ptr->sub_key_list_size - 1]);
					k_ptr->sub_key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
					strcpy(k_ptr->sub_key_name, key_buffer);
					k_ptr->current_sub_key = 0;
					k_ptr->sub_key_list_size = 0;
					k_ptr->key_list_size = 0;
					k_ptr->sub_key_list = NULL;
					k_ptr->key_list = NULL;
				}
			}
			else
			{
				// Sub key found
				AddToList((void **) &(ptr->sub_key_list), sizeof(read_t), &(ptr->sub_key_list_size));
				index = ptr->sub_key_list_size - 1;
				k_ptr = &(ptr->sub_key_list[ptr->sub_key_list_size - 1]);

				k_ptr->sub_key_name = (char *) FastMalloc(sizeof(char) * ++k_length);
				strcpy(k_ptr->sub_key_name, key_buffer);
				k_ptr->current_sub_key = 0;
				k_ptr->sub_key_list_size = 0;
				k_ptr->key_list_size = 0;
				k_ptr->sub_key_list = NULL;
				k_ptr->key_list = NULL;
				k_ptr->sub_key_list_allocated_size = 0;
				k_ptr->key_list_allocated_size = 0;
			}

			RecursiveLoad(&(ptr->sub_key_list[ptr->sub_key_list_size - 1]));
			continue;
		}

		if (type == M_COMMENT)
		{
			// Comment found
			continue;
		}

		if (type == M_ENDSUB)
		{
			// End of sub key
			return true;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Find a key and value, return type of 
// M_COMMENT, M_KEY or M_KEYVALUE if false then end of file
//---------------------------------------------------------------------------------
bool	ManiKeyValues::ReadLine(char *key, int *k_length, char *value, int *v_length, int *type)
{
	static	char buffer[2048];
	static	int	j;
	static	bool in_token;
	static	char *buffer_ptr;
	static	char c; 
	
	*type = M_COMMENT;
	buffer_ptr = (char *) &buffer;

	if (gpManiFile->ReadLine (buffer, sizeof(buffer), fh) == NULL)
	{
		return false;
	}


	j = 0;
	*k_length = 0;
	*v_length = 0;
	in_token = false;

	// Parse the line and search for key
	for (;;)
	{
		c = *buffer_ptr;

		if (c == '\0')
		{
			return true;
		}

		// Comment or key delimeter ?
		if (!in_token)
		{
			if ((c == '\\' && *(buffer_ptr + 1) == '\\') ||
				c == '{')
			{
				return true;
			}

			// End of sub key char
			if (c == '}')
			{
				*type = M_ENDSUB;
				return true;
			}
		}

		if (c == '\"')
		{
			if (!in_token)
			{
				in_token = true;
				buffer_ptr++;
				continue;
			}
			else
			{
				key[j] = '\0';
				*type = M_KEY;
				buffer_ptr++;
				break;
			}
		}
	
		if (in_token)
		{
			key[j++] = c;
			*k_length = *k_length + 1;
		}

		buffer_ptr++;
	}


	// Parse the line and search for key value
	in_token = false;
	j = 0;

	for (;;)
	{
		c = *buffer_ptr;

		if (c == '\0')
		{
			break;
		}

		// Comment ?
		if (!in_token && c == '\\' && *(buffer + 1) == '\\')
		{
			return true;
		}

		if (c == '\"')
		{
			if (!in_token)
			{
				in_token = true;
				buffer_ptr++;
				continue;
			}
			else
			{
				value[j] = '\0';
				*type = M_KEYVALUE;
				return true;
			}
		}
	
		if (in_token)
		{
			*v_length = *v_length + 1;
			value[j++] = c;
		}

		buffer_ptr++;
	}

	// If here then this means no key name found, we shall assume the next 
	// line contains a {
	if (gpManiFile->ReadLine (buffer, sizeof(buffer), fh) == NULL)
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: A fast memory managed allocation routine
//---------------------------------------------------------------------------------
char	*ManiKeyValues::FastMalloc(int bytes)
{
	static int	current_index;

	bytes = ((bytes) + (4-1) &~(4 -1));
	if (fmalloc_list_size == 0)
	{
		AddToList((void **) &fmalloc_list, sizeof(fmalloc_t), &fmalloc_list_size);
		fmalloc_list[fmalloc_list_size - 1].index = bytes;
		fmalloc_list[fmalloc_list_size - 1].block = (char *) malloc(sizeof(char) * 65536);
		return (char *) &(fmalloc_list[fmalloc_list_size - 1].block[0]);
	}

	if (fmalloc_list[fmalloc_list_size - 1].index + bytes >= 65536)
	{
		AddToList((void **) &fmalloc_list, sizeof(fmalloc_t), &fmalloc_list_size);
		fmalloc_list[fmalloc_list_size - 1].index = bytes;
		fmalloc_list[fmalloc_list_size - 1].block = (char *) malloc(sizeof(char) * 65536);
		return (char *) &(fmalloc_list[fmalloc_list_size - 1].block[0]);
	}
	else
	{
		current_index = fmalloc_list[fmalloc_list_size - 1].index;
		fmalloc_list[fmalloc_list_size - 1].index += bytes;
		return (char *) &(fmalloc_list[fmalloc_list_size - 1].block[current_index]);
	}
}