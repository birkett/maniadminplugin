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
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "mrecipientfilter.h" 
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "bitbuf.h"
#include "convar.h"
#include "Color.h"
#include "mani_player.h"
#include "mani_maps.h"
#include "mani_main.h"
#include "mani_vote.h"
#include "mani_convar.h"
#include "mani_parser.h"

// Max alias name length
#define ALIAS_LENGTH (50)
#define MANI_MAX_COLOURS (11)

extern	int	server_tickrate;
extern	ConVar	*mp_friendlyfire;
extern	ConVar	*hostname;

static	char	*GetSubToken( const char *in_string, int *token_length);
static	char	*TranslateToken( player_t *player, const char *token_string);
static	bool	TranslateColourToken( const	char	*token_string, Color	*out_colour);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

mani_colour_t	mani_colour_list[MANI_MAX_COLOURS] = 
{
	{"{BLUE}",0,0,255},
	{"{RED}",255,0,0},
	{"{GREEN}",0,255,0},
	{"{MAGENTA}",255,0,255},
	{"{BROWN}",192,0,0},
	{"{GREY}",128,128,128},
	{"{CYAN}",0,255,255},
	{"{YELLOW}",255,255,0},
	{"{ORANGE}",255,128,0},
	{"{WHITE}",255,255,255},
	{"{PINK}",255,192,255},
};

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//---------------------------------------------------------------------------------
bool ParseLine(char *in, bool strip_comments, bool strip_start_comments)
{
	int	i;
	int length;
	int	copy_start;
	int copy_end;

	// Cut carriage return out
	if (!in) return false;

	length = Q_strlen(in);
	if (length == 0) return false;

	if (strip_comments)
	{
		if (length > 1)
		{
			// chop out comments
			for (i = 0; i < length - 1; i ++)
			{
				if (in[i] == '/' && in[i+1] == '/')
				{
					in[i] = '\0';
					length = i;
					break;
				}

			// Strip end carriage returns
			}

			if (length == 0) return false;
		}
	}
	else if (strip_start_comments)
	{
		if (length > 1)
		{
			if (in[0] == '/' && in[1] == '/')
			{
				return false;
			}
		}
	}
	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return false;
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
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	// find and chop end of line
	for (;;)
	{
		if (in[copy_end] != ' ' && in[copy_end] != '\t') 
		{
			in[copy_end + 1] = '\0';
			break;
		}
		
		copy_end--;
	}

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

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//
// Format of string is 
// "alias name" command here //comments here
//---------------------------------------------------------------------------------
bool ParseAliasLine(char *in, char *alias, bool strip_comments, bool strip_start_comments)
{
	int	i;
	int length;
	int	copy_start;
	int copy_end;
	int	end_alias_index = 0;
	bool found_alias = false;

	// Cut carriage return out
	Q_strcpy(alias,"");

	if (!in) return false;

	length = Q_strlen(in);
	if (length == 0) return false;

	if (strip_comments)
	{
		if (length > 1)
		{
			// chop out comments
			for (i = 0; i < length - 1; i ++)
			{
				if (in[i] == '/' && in[i+1] == '/')
				{
					in[i] = '\0';
					length = i;
					break;
				}

			}
			
			if (length == 0) return false;
		}
	}
	else if (strip_start_comments)
	{
		if (length > 1)
		{
			if (in[0] == '/' && in[1] == '/')
			{
				return false;
			}
		}
	}


	// Strip end carriage returns

	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return false;
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
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	// find and chop end of line
	for (;;)
	{
		if (in[copy_end] != ' ' && in[copy_end] != '\t') 
		{
			in[copy_end + 1] = '\0';
			break;
		}
		
		copy_end--;
	}

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

	// String is now devoid of end and start spaces/tabs

	// Is this an alias string ?
	if (in[0] == '\"')
	{
		// Check for end marker of alias
		for (i = 1; i < (int) Q_strlen(in); i++)
		{
			if (in[i] == '\"')
			{
				found_alias = true;
				end_alias_index = i;
				break;
			}
		}

		if (!found_alias)
		{
			// Didn't find end marker
			Q_strcpy(alias,in);
			// Chop max length
			alias[ALIAS_LENGTH] = '\0';
			return true;
		}

		// Copy into alias string
		for (i = 1; i < end_alias_index; i++)
		{
			alias[i-1] = in[i];
		}

		// Terminate alias string
		alias[end_alias_index - 1] = '\0';

		if (in[end_alias_index + 1] == '\0')
		{
			// Only alias string found
			Q_strcpy(in, alias);
			alias[ALIAS_LENGTH] = '\0';
			return true;
		}

		length = Q_strlen(in);
		copy_start = end_alias_index + 1;

		// find start of command
		for (;;)
		{
			if (copy_start == length) return false;
			if (in[copy_start] == '\0') return false;
			if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
			copy_start++;
		}

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
	else
	{
		Q_strcpy(alias, in);
		alias[ALIAS_LENGTH] = '\0';
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//
// Format of string is 
// "alias name" "question" command here //comments here
//---------------------------------------------------------------------------------
bool ParseAliasLine2(char *in, char *alias, char *question, bool strip_comments, bool strip_start_comments)
{
	int	i;
	int length;
	int	copy_start;
	int copy_end;
	int	end_alias_index = 0;
	bool found_alias = false;

	// Cut carriage return out
	Q_strcpy(alias,"");
	Q_strcpy(question,"");

	if (!in) return false;

	length = Q_strlen(in);
	if (length == 0) return false;

	if (strip_comments)
	{
		if (length > 1)
		{
			// chop out comments
			for (i = 0; i < length - 1; i ++)
			{
				if (in[i] == '/' && in[i+1] == '/')
				{
					in[i] = '\0';
					length = i;
					break;
				}

			}
			
			if (length == 0) return false;
		}
	}
	else if (strip_start_comments)
	{
		if (length > 1)
		{
			if (in[0] == '/' && in[1] == '/')
			{
				return false;
			}
		}
	}
	// Strip end carriage returns

	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return false;
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
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	// find and chop end of line
	for (;;)
	{
		if (in[copy_end] != ' ' && in[copy_end] != '\t') 
		{
			in[copy_end + 1] = '\0';
			break;
		}
		
		copy_end--;
	}

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

	// String is now devoid of end and start spaces/tabs

	// Is this an alias string ?
	if (in[0] == '\"')
	{
		// Check for end marker of alias
		for (i = 1; i < (int) Q_strlen(in); i++)
		{
			if (in[i] == '\"')
			{
				found_alias = true;
				end_alias_index = i;
				break;
			}
		}
	}
	else
	{
//		MMsg("Alias string missing !!\n");
		return false;
	}

	if (!found_alias)
	{
		// Didn't find end marker
//		MMsg("Alias string not formed correctly !!\n");
		return false;
	}


	// Copy into alias string
	for (i = 1; i < end_alias_index; i++)
	{
		alias[i-1] = in[i];
	}

	// Terminate alias string
	alias[end_alias_index - 1] = '\0';


	if (in[end_alias_index + 1] == '\0')
	{
		// Only alias string found
//		MMsg("Must have question and command in string !!\n");
		return false;
	}

	// Find start of question string
	for (;;)
	{
		end_alias_index ++;
		if (in[end_alias_index] == '\"') break;
		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have question and command in string !!\n");
			return false;
		}
	}			
		
	end_alias_index ++;
	if (in[end_alias_index] == '\0')
	{
		// Only alias string found
//		MMsg("Must have question and command in string !!\n");
		return false;
	}

	// Copy question into question
	i = 0;
	for (;;)
	{
		question[i] = in[end_alias_index];
		i++;
		end_alias_index ++;
		if (in[end_alias_index] == '\"')
		{
			// End marker for question
			question[i] = '\0';
			break;
		}

		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have question and command in string !!\n");
			return false;
		}
	}

	length = Q_strlen(in);
	copy_start = end_alias_index + 1;

	// find start of command
	for (;;)
	{
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

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

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//
// Format of string is 
// "alias name" "question" //comments here
//---------------------------------------------------------------------------------
bool ParseAliasLine3(char *in, char *alias, char *question, bool strip_comments, bool strip_start_comments)
{
	int	i;
	int length;
	int	copy_start;
	int copy_end;
	int	end_alias_index = 0;
	bool found_alias = false;

	// Cut carriage return out
	Q_strcpy(alias,"");
	Q_strcpy(question,"");

	if (!in) return false;

	length = Q_strlen(in);
	if (length == 0) return false;

	if (strip_comments)
	{
		if (length > 1)
		{
			// chop out comments
			for (i = 0; i < length - 1; i ++)
			{
				if (in[i] == '/' && in[i+1] == '/')
				{
					in[i] = '\0';
					length = i;
					break;
				}

			}
			
			if (length == 0) return false;
		}
	}
	else if (strip_start_comments)
	{
		if (length > 1)
		{
			if (in[0] == '/' && in[1] == '/')
			{
				return false;
			}
		}
	}
	// Strip end carriage returns

	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return false;
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
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	// find and chop end of line
	for (;;)
	{
		if (in[copy_end] != ' ' && in[copy_end] != '\t') 
		{
			in[copy_end + 1] = '\0';
			break;
		}
		
		copy_end--;
	}

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

	// String is now devoid of end and start spaces/tabs

	// Is this an alias string ?
	if (in[0] == '\"')
	{
		// Check for end marker of alias
		for (i = 1; i < (int) Q_strlen(in); i++)
		{
			if (in[i] == '\"')
			{
				found_alias = true;
				end_alias_index = i;
				break;
			}
		}
	}
	else
	{
//		MMsg("Alias string missing !!\n");
		return false;
	}

	if (!found_alias)
	{
		// Didn't find end marker
//		MMsg("Alias string not formed correctly !!\n");
		return false;
	}


	// Copy into alias string
	for (i = 1; i < end_alias_index; i++)
	{
		alias[i-1] = in[i];
	}

	// Terminate alias string
	alias[end_alias_index - 1] = '\0';


	if (in[end_alias_index + 1] == '\0')
	{
		// Only alias string found
//		MMsg("Must have question in string !!\n");
		return false;
	}

	// Find start of question string
	for (;;)
	{
		end_alias_index ++;
		if (in[end_alias_index] == '\"') break;
		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have question in string !!\n");
			return false;
		}
	}			
		
	end_alias_index ++;
	if (in[end_alias_index] == '\0')
	{
		// Only alias string found
//		MMsg("Must have question in string !!\n");
		return false;
	}

	// Copy question into question
	i = 0;
	for (;;)
	{
		question[i] = in[end_alias_index];
		i++;
		end_alias_index ++;
		if (in[end_alias_index] == '\"')
		{
			// End marker for question
			question[i] = '\0';
			break;
		}

		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have question in string !!\n");
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Take a string, remove carriage return, remove comments, strips leading
//          and ending spaces
//
// Format of string is 
// "alias name" "question" //comments here
//---------------------------------------------------------------------------------
bool ParseCommandReplace(char *in, char *alias, char *command_type, char *replacement)
{
	int	i;
	int length;
	int	copy_start;
	int copy_end;
	int	end_alias_index = 0;
	bool found_alias = false;

	// Cut carriage return out
	Q_strcpy(alias,"");
	Q_strcpy(replacement,"");
	Q_strcpy(command_type,"");

	if (!in) return false;

	length = Q_strlen(in);
	if (length <= 2) return false;

	// Strip leading comments
	if (in[0] == '/' && in[1] == '/') return false;

	// Strip end carriage returns
	for (;;)
	{
		if (in[length - 1] == '\n' 
			|| in[length - 1] == '\r'
			|| in[length - 1] == '\f')
		{
			in[length - 1] = '\0';
			length --;
			if (length == 0) return false;
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
		if (copy_start == length) return false;
		if (in[copy_start] == '\0') return false;
		if (in[copy_start] != ' ' && in[copy_start] != '\t') break;
		copy_start++;
	}

	copy_end = length - 1;

	// find and chop end of line
	for (;;)
	{
		if (in[copy_end] != ' ' && in[copy_end] != '\t') 
		{
			in[copy_end + 1] = '\0';
			break;
		}
		
		copy_end--;
	}

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

	// String is now devoid of end and start spaces/tabs

	// Is this an alias string ?
	if (in[0] == '\"')
	{
		// Check for end marker of alias
		for (i = 1; i < (int) Q_strlen(in); i++)
		{
			if (in[i] == '\"')
			{
				found_alias = true;
				end_alias_index = i;
				break;
			}
		}
	}
	else
	{
//		MMsg("Alias string missing !!\n");
		return false;
	}

	if (!found_alias)
	{
		// Didn't find end marker
//		MMsg("Alias string not formed correctly !!\n");
		return false;
	}


	// Copy into alias string
	for (i = 1; i < end_alias_index; i++)
	{
		alias[i-1] = in[i];
	}

	// Terminate alias string
	alias[end_alias_index - 1] = '\0';


	if (in[end_alias_index + 1] == '\0')
	{
		// Only alias string found
//		MMsg("Must have command type in string !!\n");
		return false;
	}

	// Find start of command type string
	for (;;)
	{
		end_alias_index ++;
		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have command type in string !!\n");
			return false;
		}

		if (in[end_alias_index] != ' ' && in[end_alias_index] != '\t') break;
	}			
		
	// This should be the type string 
	command_type[0] = in[end_alias_index];
	command_type[1] = '\0';

	if (!FStrEq(command_type, "R") && // RCon
		!FStrEq(command_type, "C") && // Client
		!FStrEq(command_type, "S")) // Say
	{
//		MMsg("Invalid command\n");
		return false;
	}

	// Find start of command string
	for (;;)
	{
		end_alias_index ++;
		if (in[end_alias_index] == '\0')
		{
			// Only alias string found
//			MMsg("Must have command in string !!\n");
			return false;
		}

		if (in[end_alias_index] != ' ' && in[end_alias_index] != '\t') break;
	}	

	// Copy command into replacement string
	i = 0;
	for (;;)
	{
		replacement[i] = in[end_alias_index];
		i++;
		end_alias_index ++;
		if (in[end_alias_index] == '\0')
		{
			// End marker for question
			replacement[i] = '\0';
			break;
		}
	}

	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Look for tokens in a string begining with { and ending with }
//---------------------------------------------------------------------------------
void	ParseSubstituteStrings
(
 player_t	*player,
 const char	*in_string,
 char	*out_string
)
{
	int in_index = 0;
	int	out_index = 0;
	int	token_length;
	char	*out_token;
	char	*substitute_token;

	Q_strcpy(out_string, "");
	if (!in_string) return;

	while (in_string[in_index] != '\0')
	{
		bool	substituted = false;
		if (in_string[in_index] == '{')
		{
			out_token = GetSubToken(&(in_string[in_index]), &token_length);
			if (out_token && !FStrEq(out_token,""))
			{
				substitute_token = TranslateToken(player, out_token);
				if (substitute_token && !FStrEq(substitute_token,""))
				{
					int	substitute_length = Q_strlen(substitute_token);

					in_index += token_length;

					for (int j = 0; j < substitute_length; j++)
					{
						out_string[out_index] = substitute_token[j];
						out_index ++;
					}

					substituted = true;
				}
			}
		}

		if (!substituted)
		{
			out_string[out_index] = in_string[in_index];
			in_index ++;
			out_index ++;
		}
	}

	out_string[out_index] = '\0';

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Extract token from string
//---------------------------------------------------------------------------------
static
char	*GetSubToken
(
 const char	*in_string,
 int	*token_length
)
{
	static	char	out_token[256];
	bool	found_token = false;
	int	i = 0;
	
	Q_strcpy(out_token, "");
	*token_length = 0;

	while (in_string[i] != '\0')
	{
		if (i == 255) break;

		out_token[i] = in_string[i];

		if (in_string[i] == '}')
		{
			out_token[i + 1] = '\0';
			*token_length = i + 1;
			found_token = true;
			break;
		}

		i++;
	}

	if (found_token)
	{
		return out_token;
	}

	return (char *) NULL;
}

//---------------------------------------------------------------------------------
// Purpose: Translate Token into something useful
//---------------------------------------------------------------------------------
static
char	*TranslateToken
(
 player_t	*player,
 const	char	*token_string
)
{
	static	char	translated_string[512];

	Q_strcpy(translated_string, "");

	if (FStrEq(token_string, "{STEAMID}")) 
	{
		if (player)
		{
			Q_strcpy(translated_string, player->steam_id);
		}
	}
	else if (FStrEq(token_string, "{IPADDRESS}")) 
	{
		if (player)
		{
			Q_strcpy(translated_string, player->ip_address);
		}
	}
	else if (FStrEq(token_string, "{SERVERHOST}")) 
	{
		if (hostname)
		{
			Q_strcpy(translated_string, hostname->GetString());
		}
	}	
	else if (FStrEq(token_string, "{NEXTMAP}"))
	{
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1 && gpManiVote->SysMapDecided() == false)
		{
			Q_strcpy(translated_string, "Map decided by vote");
		}
		else
		{
			Q_strcpy(translated_string, next_map);
		}
	}
	else if (FStrEq(token_string, "{CURRENTMAP}")) Q_strcpy(translated_string, current_map);
	else if (FStrEq(token_string, "{TICKRATE}")) snprintf(translated_string, sizeof(translated_string), "%i", server_tickrate);
	else if (FStrEq(token_string, "{FF}"))
	{
		if (mp_friendlyfire)
		{
			if (mp_friendlyfire->GetInt() == 1)
			{
				Q_strcpy(translated_string, "On");
			}
			else
			{
				Q_strcpy(translated_string, "Off");
			}
		}
	}
	else if (FStrEq(token_string, "{THETIME}"))
	{
		char	tmp_buf[128];
		struct	tm	*time_now;
		time_t	current_time;

		time(&current_time);
		current_time += (time_t) (mani_adjust_time.GetInt() * 60);

		time_now = localtime(&current_time);
		if (mani_military_time.GetInt() == 1)
		{
			// Miltary 24 hour
			strftime(tmp_buf, sizeof(tmp_buf), "%H:%M:%S",time_now);
		}
		else
		{
			// Standard 12 hour clock
			strftime(tmp_buf, sizeof(tmp_buf), "%I:%M:%S %p",time_now);
		}

		snprintf( translated_string, sizeof(translated_string), "%s %s", tmp_buf, mani_thetime_timezone.GetString());
	}

	return (char *) translated_string;
}
	
//---------------------------------------------------------------------------------
// Purpose: Look for tokens in a string begining with { and ending with }
//---------------------------------------------------------------------------------
void	ParseColourStrings
(
 const char	*in_string,
 char	*out_string,
 Color	*out_colour
)
{
	int in_index = 0;
	int	out_index = 0;
	int	token_length;
	char	*out_token;

	Q_strcpy(out_string, "");
	if (!in_string) return;

	while (in_string[in_index] != '\0')
	{
		bool	substituted = false;
		if (in_string[in_index] == '{')
		{
			out_token = GetSubToken(&(in_string[in_index]), &token_length);
			if (out_token && !FStrEq(out_token,""))
			{
				if (TranslateColourToken(out_token, out_colour))
				{
					in_index += token_length;
					substituted = true;
				}
			}
		}

		if (!substituted)
		{
			out_string[out_index] = in_string[in_index];
			in_index ++;
			out_index ++;
		}
	}

	out_string[out_index] = '\0';

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Translate Colour Token into something useful
//---------------------------------------------------------------------------------
static
bool	TranslateColourToken
(
 const	char	*token_string,
 Color	*out_colour
)
{
	for (int i = 0; i < MANI_MAX_COLOURS; i++)
	{
		if (FStrEq(mani_colour_list[i].alias, token_string))
		{
			out_colour->SetColor
							(
							mani_colour_list[i].red,
							mani_colour_list[i].green,
							mani_colour_list[i].blue,
							255
							);
			return true;
		}
	}

	return false;
}

