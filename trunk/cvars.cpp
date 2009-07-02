/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifdef SOURCEMM

#include "mani_callback_sourcemm.h"
#include "cvars.h"

//ConVar g_MyConVar("sample_version", "Test", FCVAR_SPONLY, "Sample Plugin version");

bool ManiAccessor::RegisterConCommandBase(ConCommandBase *pVar)
{
	//this will work on any type of concmd!
	return META_REGCVAR(pVar);
}

ManiAccessor g_Accessor;


//CON_COMMAND(sample_cmd, "Sample Plugin command")
//{
//	META_LOG(g_PLAPI, "This sentence is in Spanish when you're not looking.");
//}

#endif