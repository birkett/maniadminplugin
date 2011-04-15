//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_FILE_H
#define MANI_FILE_H

#if defined __WIN32__ || defined _WIN32 || defined WIN32
	#define PATH_SEP_CHAR	'\\'
	#define ALT_SEP_CHAR	'/'
#elif defined __linux__
	#define PATH_SEP_CHAR	'/'
	#define ALT_SEP_CHAR	'\\'
#endif

class ManiFile
{
public:
	ManiFile();
	~ManiFile();

	FILE		*Open(const char *filename, const char *attrib);
	void		Close(FILE *fh);
	int			Write(const void *pOutput, int size, FILE *fh);
	char		*ReadLine(char *pOutput, int maxChars, FILE *fh);
	void		PathFormat(char *buffer, size_t len, const char *fmt, ...);
};

extern	ManiFile *gpManiFile;

#endif
