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
//#include "filesystem.h"
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
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_file.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiFile::ManiFile()
{
	gpManiFile = this;
}

ManiFile::~ManiFile()
{
	// Cleanup
}

FILE *ManiFile::Open(const char *filename, const char *attrib)
{
	char	unformatted_filename[1024];
	char	formatted_filename[1024];

	engine->GetGameDir(unformatted_filename, sizeof(formatted_filename));
	this->PathFormat(formatted_filename, sizeof(formatted_filename), "%s/%s", unformatted_filename, filename);

//	filesystem->GetLocalPath(filename, formatted_filename, sizeof(formatted_filename));

	return (FILE *) fopen(formatted_filename, attrib);
}

void ManiFile::Close(FILE *fh)
{
	fclose(fh);
}

int	ManiFile::Write(const void *pOutput, int size, FILE *fh)
{
	return (int) fwrite(pOutput, size, 1, fh);
}

char *ManiFile::ReadLine(char *pOutput, int maxChars, FILE *fh)
{
	return (char *) fgets(pOutput, maxChars, fh);
}

// Function from SourceMM CSmmAPI.cpp (CSmmAPI::PathFormat) : Thanks Bailopan :)
void ManiFile::PathFormat(char *buffer, size_t len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	size_t mylen = vsnprintf(buffer, len, fmt, ap);
	va_end(ap);

	for (size_t i=0; i<mylen; i++)
	{
		if (buffer[i] == ALT_SEP_CHAR)
			buffer[i] = PATH_SEP_CHAR;
	}

//	Msg("Formatted filename [%s]\n", buffer);
}

ManiFile	g_ManiFile;
ManiFile	*gpManiFile;
