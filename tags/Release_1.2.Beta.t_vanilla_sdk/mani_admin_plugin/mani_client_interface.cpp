/* ======== Basic Admin tool ========
* Copyright (C) 2004-2006 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Helping on misc errors/functions: BAILOPAN,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#ifdef SOURCEMM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "inetchannelinfo.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_main.h"
#include "mani_client.h"
#include "ISmmPlugin.h"
#include "sourcehook/sh_vector.h"
#include "mani_client_interface.h"


SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
unsigned int g_CallBackCount;

bool ClientInterface::RegisterFlag(const char *Class,const char *Flag,const char *Description)
{	
	gpManiClient->AddFlagDesc(Class, Flag, Description);
	return true;
}

bool ClientInterface::IsClient(int id)
{
	return (gpManiClient->IsClient(id));
}

bool ClientInterface::HasFlag(int id, const char *Class, const char *Flag)
{
	return (gpManiClient->HasAccess(id, Class, Flag)); 
}

bool ClientInterface::HasFlag(int id, const char *Flag)
{
	return (gpManiClient->HasAccess(id, ADMIN, Flag)); 
}

void ClientInterface::RemoveListner(AdminInterfaceListner *ptr)
{
	if(g_CallBackCount >= g_CallBackList.size()-1 || g_CallBackList.size() == 0)
	{
		g_CallBackList.push_back(new AdminInterfaceListnerStruct);
	}

	for(unsigned int i = 0; i < g_CallBackCount; i++)
	{
		if(g_CallBackList[i]->ptr == ptr) // We already have the pointer in the list, so we dont want to save it again
		{
			g_CallBackList[i]->ptr = NULL;			
			break;
		}
	}
}
void ClientInterface::AddEventListner(AdminInterfaceListner *ptr)
{
	if(g_CallBackCount >= g_CallBackList.size()-1 || g_CallBackList.size() == 0)
	{
		g_CallBackList.push_back(new AdminInterfaceListnerStruct);
	}

	bool IsOrginal=true;
	for(unsigned i=0;i<g_CallBackCount;i++)
	{
		if(g_CallBackList[i]->ptr == ptr) // We already have the pointer in the list, so we dont want to save it again
		{
			MMsg("ERROR: A plugin has tried to register the same interface 2 times (Pointer: %p)",ptr);

			IsOrginal = false;
			break;
		}
	}
	if(IsOrginal)
	{
		MMsg("echo A plugin has found the AdminInterface (%p)",ptr);
		g_CallBackList[g_CallBackCount]->ptr = ptr;
		g_CallBackCount++;
	}
	else
	{
		MMsg("A plugin has registered itself without providing a callback pointer, this will produce crashes if Mani Admin Plugin gets unloaded");
	}
}

ClientInterface g_ClientInterface;
#endif