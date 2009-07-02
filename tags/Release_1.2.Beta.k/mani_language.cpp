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
#include "filesystem.h"
#include "engine/iserverplugin.h"
//#include "convar.h"
#include "mani_language.h"
#include "mani_memory.h"
#include "mani_parser.h"
#include "mani_convar.h"

#define MANI_MAX_TRANSLATIONS (2000)

extern IFileSystem	*filesystem;

struct lang_trans_t
{
	char	*translation;
};

static	bool	GetLanguageIntoMemory(char *language_type, bool overwrite_old);
static	void	GetLanguageType(char *language);
static	char	*ParseTranslationString ( char *in, int  *trans_no, char *error_string);
static	bool	ExtractFmtToken ( const char	*fmt_in,  char	*fmt_token, int	*index);
static  int		GetParamIndex ( const char	*param_in, int	*index);

static	lang_trans_t	*lang_list = NULL;
static  int				lang_list_size = 0;

//---------------------------------------------------------------------------------
// Purpose: Loads language text into memory
//---------------------------------------------------------------------------------
bool LoadLanguage(void)
{
	char	language_type[128];

	FreeLanguage();

	// Load English first as it should be reliable
	if (!GetLanguageIntoMemory("english", false))
	{
		return false;
	}

	// Get the language type into language
	GetLanguageType(language_type);

	// Load second language over the first, if there are any errors
	// it won't stop the plugin from falling over hopefully
	if (!GetLanguageIntoMemory(language_type, true))
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Unloads Language from memory
//---------------------------------------------------------------------------------
void FreeLanguage(void)
{

	if (lang_list_size == 0) return;

	for (int i = 0; i < lang_list_size; i++)
	{
		if (lang_list[i].translation)
		{
			free(lang_list[i].translation);
		}
	}

	FreeList((void **) &lang_list, &lang_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Gets translation
//---------------------------------------------------------------------------------
char *Translate(int translate_id)
{
	if (lang_list[translate_id].translation == NULL)
	{
		Msg("WARNING TRANSLATION ID [%05i] DOES NOT EXIST !!!\n", translate_id);
		return NULL;
	}

	return (lang_list[translate_id].translation);
}

//---------------------------------------------------------------------------------
// Purpose: Loads language text into memory
//---------------------------------------------------------------------------------
static
bool GetLanguageIntoMemory(char *language_type, bool overwrite_old)
{
	char	base_filename[256];
	char	raw_string[1024];
	char	*translated = NULL;
	int		translation_id;
	char	error_string[1024];


	FileHandle_t file_handle;

	if (!overwrite_old)
	{
		if (!CreateList ((void **) &lang_list, sizeof(lang_trans_t), MANI_MAX_TRANSLATIONS , &lang_list_size))
		{
			return false;
		}
	}

	if (!overwrite_old)
	{
		for (int i = 0; i < lang_list_size; i ++)
		{
			lang_list[i].translation = NULL;
		}
	}

//	Msg("Attempting to load %s.cfg\n", language_type);

	// Get the language type we are trying to load
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/language/%s.cfg", mani_path.GetString(), language_type);
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load %s.cfg, defaulting to english\n", language_type);
		return false;
	}
	else
	{
		bool failed_translation = false;
		int	line_count = 1;
		int	translations_processed = 0;

		while (filesystem->ReadLine (raw_string, sizeof(raw_string), file_handle) != NULL)
		{
			Q_strcpy(error_string,"");
			if (NULL == (translated = ParseTranslationString(raw_string, &translation_id, error_string)))
			{
				if (error_string && error_string[0] != '\0')
				{
					Msg("Line [%i] Error : %s\n", line_count, error_string);
					// Translation is borked for this line
					if (!overwrite_old)
					{
						failed_translation = true;
					}
				}
			}
			else
			{
				if (overwrite_old)
				{
					if (lang_list[translation_id].translation == NULL)
					{
						// Overwriting the wrong index !!!
						Msg ("Line [%i] Error : Translation ID [%05i] number incorrect !!!\n", line_count, translation_id);
					}
					else
					{
						// Free oringal text and assign new language
						free(lang_list[translation_id].translation);
						lang_list[translation_id].translation = translated;
					}
				}
				else
				{
					if (lang_list[translation_id].translation != NULL)
					{
						// Overwriting the wrong index !!!
						Msg ("Line [%i] Error : Translation ID [%05i] number incorrect !!!\n", line_count, translation_id);
						failed_translation = true;
					}
					else
					{
						lang_list[translation_id].translation = translated;
					}
				}

				translations_processed ++;

			}

			line_count ++;
		}

		filesystem->Close(file_handle);
//		Msg("Processed %i translations for file [%s.cfg]\n", translations_processed, language_type);
		if (failed_translation)
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the language type from language.cfg
//---------------------------------------------------------------------------------
static
void GetLanguageType(char *language)
{
	char	base_filename[512];

	FileHandle_t file_handle;

	// Get the language type we are trying to load
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/language/language.cfg", mani_path.GetString());

	Msg("Attempting to load [%s]\n", base_filename);

	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
		Msg ("Failed to load language.cfg, defaulting to english\n");
		Q_strcpy(language,"english");
		return;
	}
	else
	{
		while (filesystem->ReadLine (language, 128, file_handle) != NULL)
		{
			if (!ParseLine(language, true))
			{
				// String is empty after parsing
				continue;
			}
		
			break;
		}

		Msg("Language to be used [%s]\n", language);
		filesystem->Close(file_handle);
	}

	return;
}
 
//---------------------------------------------------------------------------------
// Purpose: Parses the translation file                                              
//---------------------------------------------------------------------------------
static
char *ParseTranslationString
(
char *in, 
int  *trans_no,
char *error_string
)
{
	int	i;
	int	j;
	int 	length;
	int	copy_start;
	int 	copy_end;
	char	trans_no_string[128];	
	int	trans_string_length;
	char	params_string[128];
	char	single_param[128];
	bool	no_params = false;
	char	*find_param;
	char	*translation;
	int		parameter_number;

	// Cut carriage return out
	if (!in) return NULL;

	length = Q_strlen(in);
	if (length == 0) return NULL;

	// Strip end of string
	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return NULL;
		}
		else
		{
			break;
		}
	}

	copy_start = 0;

	// find start of line
	for (;;)
	{
		if (copy_start == length) return NULL;
		if (in[copy_start] == '\0') return NULL;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	if (copy_start > 0)
	{
		i = 0;
		for (;;)
		{
			if (in[copy_start] == '\0')
			{
				in[i] = '\0';
				break;
			}

			in[i] = in[copy_start];
			copy_start ++;
			i ++;
		}
	}

	// in should now be suitably mangled to our needs

	if (Q_strlen(in) < 2) return NULL;

	// Check if it's a comment
	if (in[0] == '/' && in[1] == '/') return NULL;

	// Get translation number

	i = 0;
	for (;;)
	{
		if (in[i] == '\0') 
		{
			Q_strcpy(error_string, "Premature end of line");
			return NULL;
		}

		if (in[i] == ';') 
		{
			trans_no_string[i] = '\0';
			break;
		}

		trans_no_string[i] = in[i];
		i ++;
	}

	*trans_no = Q_atoi(trans_no_string);
	if (*trans_no == 0)
	{
		// Bad string conversion
		Q_snprintf(error_string, 1024, "Bad translation id [%s]", trans_no_string);
		return NULL;
	}

	i ++;
	if (in[i] == '\0')
	{
		Q_snprintf(error_string, 1024, "Only translation id [%s] in line", trans_no_string);
		return NULL;
	}

	if (in[i] == ';') no_params = true;

	if (!no_params)
	{
		// Parse parameters string
		j = 0;
		for (;;)
		{
			if (in[i] == '\0') 
			{
				Q_snprintf(error_string, 1024, "Premature end of line while reading parameters, translation id [%s]", trans_no_string);
				return NULL;
			}

			if (in[i] == ';')
			{
				params_string[j] = '\0';
				break;
			}

			params_string[j] = in[i];
			i++;
			j++;
		}
	}

	i++;
	if (in[i] == '\0') 
	{
		Q_snprintf(error_string, 1024, "Premature end of line after reading parameters, translation id [%s]", trans_no_string);
		return NULL;
	}


	trans_string_length = (Q_strlen(in) - i) + 1;
	translation = (char *) malloc(sizeof(char) * trans_string_length);

	j = 0;
	while (in[i] != '\0')
	{
		if (in[i] == '\\' && in[i + 1] != '\0')
		{
			switch (in[i + 1])
			{
				case 'n': translation[j] = 0x0d; i++; break;
				case '\\': translation[j] = '\\'; i++; break;
				default : translation[j] = in[i];break;
			}
		}
		else
		{
			translation[j] = in[i];
		}
		
		i ++;
		j ++;
	}

	translation[j] = '\0';
	
	// Check if we need to parse the string
	if (!no_params)
	{
		// Set up start search 
		find_param = translation;
		i = 0;
		parameter_number = 1;

		for (;;)
		{
			if (params_string[i] == '\0') break;

			if (params_string[i] != '%')
			{
				Q_snprintf(error_string, 1024, "Invalid parameter string [%s], translation id [%s]", params_string, trans_no_string);
				free (translation);
				return NULL;
			}

			j = 0;

			for (;;)
			{
				single_param[j] = params_string[i];
				j ++;
				i ++;
				if (params_string[i] == '\0' ||
				    params_string[i] == '%')
				{
					single_param[j] = '\0';
					break;
				}
			}

			if (!single_param)
			{
				Q_snprintf(error_string, 1024, "Invalid parameter string [%s], translation id [%s]", params_string, trans_no_string);
				free (translation);
				return NULL;
			}

			if (Q_strlen(single_param) < 2)
			{
				// Only a % in there
				Q_snprintf(error_string, 1024, "Invalid parameter string [%s], translation id [%s]", params_string, trans_no_string);
				free (translation);
				return NULL;
			}	

			// Check param exists
			find_param = strstr(find_param, single_param);
			if (find_param == NULL)
			{
				Q_snprintf(error_string, 1024, "Parameter %i [%s] from param list [%s] is not in translation string [%s] or in wrong order, translation id [%s]", 
																	parameter_number, 
																	single_param, 
																	params_string,
																	translation,
																	trans_no_string);
				free (translation);
				return NULL;
			}

			parameter_number ++;
		}	
	}			

	return translation;
}

//---------------------------------------------------------------------------------
// Purpose: Gets translation
//---------------------------------------------------------------------------------
char *Translate2
(
 int translate_id, 
 char *fmt1, 
 char *fmt2, 
 ...
 )
{
	static	char final_string[4096];
	static	char fmt_parameter[20][10];
	static	char converted_parameter[20][2048];
	va_list		argptr;
	char	*translation_ptr;
	int		index = 0;
	int		final_index = 0;
	int		param_size = 0;

	/* fmt1 contains the actual parameters used by sprintf
	   and should match any va_arg parameters begin passed in */

	if (fmt1)
	{
		for (;;)
		{
			if (fmt1[index] == '\0')
			{
				// End of fmt1
				break;
			}

			if (!ExtractFmtToken(fmt1, fmt_parameter[param_size], &index))
			{
				Msg("Error in fmt string [%s] for string [%s] translation id [%i]\n", fmt1, fmt2, translate_id);
				return ("Error");
			}

			param_size ++;
			if (param_size == 20)
			{
				break;
			}
		}
	}


	/* Get parameters and convert them into strings we can use*/

	if (param_size != 0)
	{
		va_start (argptr, fmt2); 
		for (int i = 0; i < param_size; i++)
		{
			int	param_len = Q_strlen(fmt_parameter[i]) - 1;

			switch (fmt_parameter[i][param_len])
			{
			case 's' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], (char *) va_arg(argptr, char *)); break;
			case 'i' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], va_arg(argptr, int)); break;
			case 'f' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], (float) va_arg(argptr, double)); break;
			case 'd' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], va_arg(argptr, int)); break;
			case 'X' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], va_arg(argptr, int)); break;
			case 'c' : Q_snprintf(converted_parameter[i], 2048, fmt_parameter[i], (char) va_arg(argptr, int)); break;
			default :	{
						Msg("Error in fmt string [%s] for string [%s] translation id [%i]\n", fmt1, fmt2, translate_id);
						va_end(argptr);
						return ("Error");
						}
			}
		}

		va_end(argptr);
	}

   /* fmt2 contains the default text to be used should none be provided.
      In this text %1p */

	if (lang_list[translate_id].translation != NULL)
	{
		// Use provided translation
		translation_ptr = lang_list[translate_id].translation;
	}
	else
	{
		// Use default
		translation_ptr = fmt2;
	}

	index = 0;

	Q_strcpy(final_string, "");

	// Build our final string
	while (translation_ptr[index] != '\0')
	{
		if (translation_ptr[index] == '%')
		{
			if (translation_ptr[index + 1] != '%' && translation_ptr[index + 1] != '\0')
			{
				int parameter_index = GetParamIndex(translation_ptr, &index);
				if (parameter_index < param_size)
				{
					int param_len = Q_strlen(converted_parameter[parameter_index]);

					for (int i = 0; i < param_len; i++)
					{
						final_string[final_index ++] = converted_parameter[parameter_index][i];
					}
				}
				else
				{
					if (translation_ptr[index] != '\0')
					{
						final_string[final_index++] = translation_ptr[index++];
					}
				}
			}
			else
			{
				final_string[final_index++] = translation_ptr[index++];
			}
		}
		else
		{
			final_string[final_index++] = translation_ptr[index++];
		}
	}

	return (final_string);
}

//---------------------------------------------------------------------------------
// Purpose: Gets our %1p, %p, %20p etc parameter index
//---------------------------------------------------------------------------------
static
int	GetParamIndex 
(
 const char	*param_in,
 int	*index
)
{
	char	param_string[3];
	int		param_number;

	// Handle just %p (parameter 1)
	if (param_in[*index + 1] == 'p')
	{
		*index = *index + 2;
		return 0;
	}

	*index = *index + 1;
	param_string[0] = param_in[*index];
	if (param_in[*index + 1] != 'p')
	{
		// Handle double digit param like %15p
		param_string[1] = param_in[*index + 1];
		param_string[2] = '\0';
		*index = *index + 3;
	}
	else
	{
		// Handle single digit param like %5p
		param_string[1] = '\0';
		*index = *index + 2;
	}

	param_number = Q_atoi(param_string);
	if (param_number == 0)
	{
		// Just in case
		return 0;
	}

	// Return our parameter index
	return (param_number - 1);
}


//---------------------------------------------------------------------------------
// Purpose: Gets token from fmt passed in
//---------------------------------------------------------------------------------
static
bool	ExtractFmtToken 
(
 const	char	*fmt_in, 
 char	*fmt_token,
 int	*index
)

{
	int i = 0;

	if (fmt_in[*index] != '%')
	{
		return false;
	}

	while (fmt_in[*index] != '\0')
	{
		fmt_token[i++] = fmt_in[*index];
		*index = *index + 1;

		if (fmt_token[*index] == '%')
		{
			break;
		}
	}

	fmt_token[i] = '\0';
	return true;

}
