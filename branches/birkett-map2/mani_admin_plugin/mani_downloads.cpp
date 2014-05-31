//
// Mani Admin Plugin
//
// Copyright © 2009-2014 Giles Millward (Mani). All rights reserved.
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
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_memory.h"
#include "mani_player.h"
#include "mani_skins.h"
#include "mani_maps.h"
#include "mani_client.h"
#include "mani_customeffects.h"
#include "mani_mapadverts.h"
#include "mani_downloads.h"
#include "KeyValues.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;
extern	INetworkStringTableContainer *networkstringtable;

extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

//class ManiDownloads
//class ManiDownloads
//{

ManiDownloads::ManiDownloads()
{
	// Init
	gpManiDownloads = this;
}

ManiDownloads::~ManiDownloads()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Read in core config and setup
//---------------------------------------------------------------------------------
void ManiDownloads::Init(void)
{
	char	core_filename[256];

	// Read the downloads.txt file
//	MMsg("*********** Loading downloads.txt ************\n");

	KeyValues *kv_ptr = new KeyValues("downloads.txt");

	for (;;)
	{
		snprintf(core_filename, sizeof (core_filename), "./cfg/%s/downloads.txt", mani_path.GetString());
		if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
		{
//			MMsg("Failed to load downloads.txt\n");
			kv_ptr->deleteThis();
			break;
		}

		KeyValues *base_key_ptr;

		// Get first key (should be "downloads")
		base_key_ptr = kv_ptr->GetFirstSubKey();
		if (!base_key_ptr)
		{
//			MMsg("Nothing found\n");
			kv_ptr->deleteThis();
			break;
		}

		for (;;)
		{
			// Call function to handle multiple download entrys
			if (FStrEq(base_key_ptr->GetName(), "downloads"))
			{
				gpManiDownloads->AddDownloadsKeyValues(base_key_ptr);
			}

			base_key_ptr = base_key_ptr->GetNextKey();
			if (!base_key_ptr)
			{
				break;
			}
		}

		kv_ptr->deleteThis();

//		MMsg("*********** downloads.txt loaded ************\n");
		break;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Read "downloads" sub keys
//---------------------------------------------------------------------------------
void ManiDownloads::AddDownloadsKeyValues(KeyValues *kv_ptr)
{
	KeyValues *kv_downloads_ptr;

	kv_downloads_ptr = kv_ptr->GetFirstValue();
	if (!kv_downloads_ptr)
	{
		return;
	}

	for (;;)
	{
		const char *download_name =  kv_downloads_ptr->GetString();
		if (download_name)
		{
//			MMsg("Adding %s to download list.... ", download_name);
			AddToDownloads(download_name);
//			MMsg("Done.\n");
		}

		kv_downloads_ptr = kv_downloads_ptr->GetNextValue();
		if (!kv_downloads_ptr)
		{
			break;
		}
	}

}

//---------------------------------------------------------------------------------
// Purpose: Add downloads to source engine
//---------------------------------------------------------------------------------
void ManiDownloads::AddToDownloads(const char *filename)
{
	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);
	char	res_string[512];

	// Set up .res downloadables
	if (pDownloadablesTable)
	{
		snprintf(res_string, sizeof(res_string), "%s", filename);
#ifdef GAME_ORANGE
		pDownloadablesTable->AddString(true, res_string, sizeof(res_string));
#else
		pDownloadablesTable->AddString(res_string, sizeof(res_string));
#endif	
	}

	engine->LockNetworkStringTables(save);
}

ManiDownloads	g_ManiDownloads;
ManiDownloads	*gpManiDownloads;
