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
#include "dlls/iplayerinfo.h"
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
#include "mani_gametype.h"
#include "mani_client.h"
#include "mani_vfuncs.h"
#include "mani_commands.h"
#include "mani_file.h"
#include "mani_vars.h"
#include "cbaseentity.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IFileSystem	*filesystem;
extern	IServerGameDLL	*serverdll;

extern	CGlobalVars *gpGlobals;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}


//********************************************************
// Do all the non virtual stuff for setting network vars

CON_COMMAND(ma_getprop, "Debug Tool")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

#ifndef ORANGE
	CCommand args;
#endif

	if (args.ArgC() == 1)
	{
		ServerClass *sc = serverdll->GetAllServerClasses();
		while (sc)
		{
			MMsg("%s\n", sc->GetName());
			sc = sc->m_pNext;
		}	
	}
	else if (args.ArgC() == 2)
	{
		ServerClass *sc = serverdll->GetAllServerClasses();
		while (sc)
		{
			if (FStrEq(sc->GetName(), args.Arg(1)))
			{
				int NumProps = sc->m_pTable->GetNumProps();
				for (int i=0; i<NumProps; i++)
				{
					MMsg("%s [%i]\n", sc->m_pTable->GetProp(i)->GetName(), sc->m_pTable->GetProp(i)->GetOffset());
				}

				return ;
			}
			sc = sc->m_pNext;
		}
	}
	else if (args.ArgC() == 3)
	{
		ServerClass *sc = serverdll->GetAllServerClasses();
		while (sc)
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (Q_stristr(sc->m_pTable->GetProp(i)->GetName(), args.Arg(1)))
				{
					MMsg("%s.%s\n", sc->GetName(), sc->m_pTable->GetProp(i)->GetName());
				}
			}

			sc = sc->m_pNext;
		}
	}
}

CON_COMMAND(ma_getpropfilt, "Debug Tool")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;
#ifndef ORANGE
	CCommand args;
#endif

	if (args.ArgC() == 1)
	{
		ServerClass *sc = serverdll->GetAllServerClasses();
		while (sc)
		{
			MMsg("%s\n", sc->GetName());
			sc = sc->m_pNext;
		}	
	}
	else if (args.ArgC() == 2)
	{
		ServerClass *sc = serverdll->GetAllServerClasses();
		while (sc)
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (Q_stristr(sc->m_pTable->GetProp(i)->GetName(), args.Arg(1)))
				{
					MMsg("%s.%s [%i] [%i] [Signed: %s]\n", 
							sc->GetName(), 
							sc->m_pTable->GetProp(i)->GetName(), 
							(int) sc->m_pTable->GetProp(i)->GetType(),
							sc->m_pTable->GetProp(i)->m_nBits,
							sc->m_pTable->GetProp(i)->IsSigned() ? "true":"false");
				}
			}

			sc = sc->m_pNext;
		}
	}
}

int UTIL_FindPropOffset(const char *CombinedProp, int &type, bool find_type)
{
	char	ClassName[256]="";
	char	Property[256]="";

	if (!UTIL_SplitCombinedProp(CombinedProp, ClassName, Property))
	{
		return -1;
	}

	ServerClass *sc = serverdll->GetAllServerClasses();
	while (sc)
	{
		if (FStrEq(sc->GetName(), ClassName))
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (stricmp(sc->m_pTable->GetProp(i)->GetName(), Property) == 0)
				{
					int offset = sc->m_pTable->GetProp(i)->GetOffset();

					if (find_type)
					{
						int	bits = sc->m_pTable->GetProp(i)->m_nBits;
						bool is_signed = sc->m_pTable->GetProp(i)->IsSigned();
						int var_type = sc->m_pTable->GetProp(i)->GetType();
						type = UTIL_CalculatePropType
									(
									var_type, 
									bits, 
									is_signed
									);
					}

					return offset;
				}
			}
			return -1;
		}
		sc = sc->m_pNext;
	}

	return -1;
}

int	UTIL_CalculatePropType(int var_type, int bits, bool is_signed)
{
	switch (var_type)
	{
	case DPT_Int:if (bits == 1)
				 {
					 return PROP_BOOL;
				 }
				 else if (bits <= 8)
				 {
					 if (is_signed)
					 {
						 return PROP_CHAR;
					 }
					 else
					 {
						 return PROP_UNSIGNED_CHAR;
					 }
				 }
				 else if (bits <= 16)
				 {
					 if (is_signed)
					 {
						 return PROP_SHORT;
					 }
					 else
					 {
						 return PROP_UNSIGNED_SHORT;
					 }
				 }
				 else
				 {
					 if (is_signed)
					 {
						 return PROP_INT;
					 }
					 
					 return PROP_UNSIGNED_INT;
				 }

	case DPT_Float: return PROP_FLOAT;
	case DPT_String: if (is_signed)
					 {
						 return PROP_CHAR_PTR;
					 }
					 else
					 {
						 return PROP_UNSIGNED_CHAR_PTR;
					 }
	case DPT_Vector: return PROP_VECTOR;
	default: break;
	}

	return -1;
}

bool	UTIL_SplitCombinedProp(const char *source, char *part1, char *part2)
{
	int length = strlen(source);

	bool found_split = false;
	int j = 0;

	for (int i = 0; i < length; i++)
	{
		if (found_split)
		{
			part2[j] = source[i];
			j++;
		}
		else
		{
			part1[i] = source[i];
		}

		if (!found_split)
		{
			if (source[i] == '.')
			{
				part1[i] = '\0';
				found_split = true;
			}
		}
	}

	if (!found_split) return false;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns the index of a variable associated with an entity
//---------------------------------------------------------------------------------
int		ManiGameType::GetPtrIndex(CBaseEntity *pCBE, int index)
{
	if (var_index[index].index == -1)
	{
		// Search for var if first time
		var_index[index].index = UTIL_GetVarValue(pCBE, var_index[index].name, var_index[index].type);
		if (var_index[index].index == -1)
		{
			var_index[index].index = 2;
			return -1;
		}
	}
	else if (var_index[index].index == -2)
	{
		// It was never found on the first go.
		return -1;
	}

	// return true ptr
	return var_index[index].index;
}

// Search datamap for value
int	UTIL_GetVarValue(CBaseEntity *pCBE, const char *var_id, int &type)
{
	datamap_t *dmap = CBaseEntity_GetDataDescMap(pCBE);
	if (dmap)
	{
		while (dmap)
		{
			for ( int i = 0; i < dmap->dataNumFields; i++ )
			{
				if (dmap->dataDesc[i].fieldName &&
					strcmp(var_id,dmap->dataDesc[i].fieldName) == 0)
				{
					switch(dmap->dataDesc[i].fieldType)
					{
						case FIELD_FLOAT: type = PROP_FLOAT; break;
						case FIELD_STRING: type = PROP_CHAR_PTR; break;
						case FIELD_INTEGER: type = PROP_INT; break;
						case FIELD_BOOLEAN: type = PROP_BOOL; break;
						case FIELD_SHORT: type = PROP_SHORT; break;
						case FIELD_CHARACTER: type = PROP_CHAR; break;
						default: type = -1;
					}
					return dmap->dataDesc[i].fieldOffset[0];
				}
			}

			dmap = dmap->baseMap;
		}
	}

	return -1;
}

static	
void ShowDMap(datamap_t *dmap);

static FILE *fh;

CON_COMMAND(ma_getmap, "Debug Tool")
{
	if (!IsCommandIssuedByServerAdmin()) return;
	if (ProcessPluginPaused()) return;

	player_t player;

	player.entity = NULL;

#ifndef ORANGE
	CCommand args;
#endif

	if (args.ArgC() < 2)
	{
		MMsg("Need more args :)\n");
		return;
	}

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(&player, args.Arg(1), NULL))
	{
		return;
	}

	player_t *target_ptr = &(target_player_list[0]);

	CBaseEntity *pPlayer = target_ptr->entity->GetUnknown()->GetBaseEntity();

	MMsg("Attempting to get map for player [%s]\n",target_ptr->name);

	datamap_t *dmap = CBaseEntity_GetDataDescMap(pPlayer);
	if (dmap)
	{
		char base_filename[512];
		snprintf(base_filename, sizeof (base_filename), "./cfg/%s/clipboard.txt", mani_path.GetString());

		fh = gpManiFile->Open(base_filename, "wb");
		if (fh == NULL)
		{
			MMsg("Failed to open file %s\n", base_filename);
			return;
		}

		ShowDMap(dmap);
		gpManiFile->Close(fh);
	}
	else
	{
		MMsg("did not obtain datamap\n");
	}
}

static	
void ShowDMap(datamap_t *dmap)
{
	static int indent = 0;
	char indents[256];

	Q_strcpy(indents,"");

	for (int i = 0; i < indent; i++)
	{
		strcat(indents,"\t");
	}

	while (dmap)
	{
		char	classname[128];

		int headerbytes = snprintf(classname, sizeof(classname), "%s%s\n", indents, dmap->dataClassName);
		gpManiFile->Write(classname, headerbytes, fh);
		MMsg("%s", classname);

		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			char	field_type[128];

			switch(dmap->dataDesc[i].fieldType)
			{
			case FIELD_VOID: snprintf(field_type, sizeof(field_type), "VOID"); break;
			case FIELD_FLOAT: snprintf(field_type, sizeof(field_type), "FLOAT"); break;
			case FIELD_STRING: snprintf(field_type, sizeof(field_type), "STRING"); break;
			case FIELD_VECTOR: snprintf(field_type, sizeof(field_type), "VECTOR"); break;
			case FIELD_QUATERNION: snprintf(field_type, sizeof(field_type), "QUATERNION"); break;
			case FIELD_INTEGER: snprintf(field_type, sizeof(field_type), "INTEGER"); break;
			case FIELD_BOOLEAN: snprintf(field_type, sizeof(field_type), "BOOLEAN"); break;
			case FIELD_SHORT: snprintf(field_type, sizeof(field_type), "SHORT"); break;
			case FIELD_CHARACTER: snprintf(field_type, sizeof(field_type), "CHARACTER"); break;
			case FIELD_COLOR32: snprintf(field_type, sizeof(field_type), "COLOR32"); break;
			case FIELD_EMBEDDED: snprintf(field_type, sizeof(field_type), "EMBEDDED"); break;
			case FIELD_CUSTOM: snprintf(field_type, sizeof(field_type), "CUSTOM"); break;
			case FIELD_CLASSPTR: snprintf(field_type, sizeof(field_type), "CLASSPTR"); break;
			case FIELD_EHANDLE: snprintf(field_type, sizeof(field_type), "EHANDLE"); break;
			case FIELD_EDICT: snprintf(field_type, sizeof(field_type), "EDICT"); break;
			case FIELD_POSITION_VECTOR: snprintf(field_type, sizeof(field_type), "POSITION_VECTOR"); break;
			case FIELD_TIME: snprintf(field_type, sizeof(field_type), "TIME"); break;
			case FIELD_TICK: snprintf(field_type, sizeof(field_type), "TICK"); break;
			case FIELD_MODELNAME: snprintf(field_type, sizeof(field_type), "MODELNAME"); break;
			case FIELD_SOUNDNAME: snprintf(field_type, sizeof(field_type), "SOUNDNAME"); break;
			case FIELD_INPUT: snprintf(field_type, sizeof(field_type), "INPUT"); break;
			case FIELD_FUNCTION: snprintf(field_type, sizeof(field_type), "FUNCTION"); break;
			case FIELD_VMATRIX: snprintf(field_type, sizeof(field_type), "VMATRIX"); break;
			case FIELD_VMATRIX_WORLDSPACE: snprintf(field_type, sizeof(field_type), "VMATRIX_WORLDSPACE"); break;
			case FIELD_MATRIX3X4_WORLDSPACE: snprintf(field_type, sizeof(field_type), "MATRIX3X4_WORLDSPACE"); break;
			case FIELD_INTERVAL: snprintf(field_type, sizeof(field_type), "INTERVAL"); break;
			case FIELD_MODELINDEX: snprintf(field_type, sizeof(field_type), "MODELINDEX"); break;
			case FIELD_MATERIALINDEX: snprintf(field_type, sizeof(field_type), "MATERIALINDEX"); break;
			default : snprintf(field_type, sizeof(field_type), "UNKNOWN TYPE"); break;
			}
			char	address1[32] = "";
			char	address2[32] = "";
			char	outstring[1024] = "";

			snprintf(address1,sizeof(address1), " [%p]", dmap->dataDesc[i].inputFunc);
			snprintf(address2,sizeof(address2), " [%p]", dmap->dataDesc[i].td);

			int bytes = snprintf(outstring, sizeof(outstring), "%s - %s %s (%s)%s (off1: %d  off2: %d)%s\n", indents, dmap->dataDesc[i].fieldName, field_type, dmap->dataDesc[i].externalName, (!dmap->dataDesc[i].inputFunc) ? "":address1, dmap->dataDesc[i].fieldOffset[0], dmap->dataDesc[i].fieldOffset[1], (!dmap->dataDesc[i].td) ? "":address2 );
			gpManiFile->Write(outstring, bytes, fh);
			MMsg("%s", outstring);
			if (dmap->dataDesc[i].td)
			{
				datamap_t *dmap2 = dmap->dataDesc[i].td;
				indent ++;
				ShowDMap(dmap2);
				indent --;
			}
		}
		dmap = dmap->baseMap; 
	}
}

//********************************************************************
//********************************************************************
//********************************************************************
//********************************************************************
// Handle prop offset calls
// Handle prop offset calls

int Prop_GetInt(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	int *iptr = (int *)ptr;
	return *iptr;
}

void Prop_SetInt(edict_t *pEntity, int offset, int value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	int *iptr = (int *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

unsigned int Prop_GetUnsignedInt(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned int *iptr = (unsigned int *)ptr;
	return *iptr;
}

void Prop_SetUnsignedInt(edict_t *pEntity, int offset, int value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned int *iptr = (unsigned int *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

long Prop_GetLong(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	long *iptr = (long *)ptr;
	return *iptr;
}

void Prop_SetLong(edict_t *pEntity, int offset, long value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	long *iptr = (long *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

unsigned long Prop_GetUnsignedLong(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned long *iptr = (unsigned long *)ptr;
	return *iptr;
}

void Prop_SetUnsignedLong(edict_t *pEntity, int offset, long value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned long *iptr = (unsigned long *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

char Prop_GetChar(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	char *cptr = (char *)ptr;
	return *cptr;
}

void Prop_SetChar(edict_t *pEntity, int offset, long value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	char *cptr = (char *) ptr;
	*cptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

unsigned char Prop_GetUnsignedChar(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	return *ptr;
}

void Prop_SetUnsignedChar(edict_t *pEntity, int offset, unsigned char value)
{
	unsigned char *ptr = (unsigned char *) (pEntity->GetUnknown() + offset);
	*ptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

short Prop_GetShort(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	short *sptr = (short *) ptr;
	return *sptr;
}

void Prop_SetShort(edict_t *pEntity, int offset, short value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	short *sptr = (short *) ptr;
	*sptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

unsigned short Prop_GetUnsignedShort(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned short *sptr = (unsigned short *) ptr;
	return *sptr;
}

void Prop_SetUnsignedShort(edict_t *pEntity, int offset, unsigned short value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned short *sptr = (unsigned short *) ptr;
	*sptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

char *Prop_GetCharPtr(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	char  *cptr = (char *) ptr;
	return cptr;
}

void Prop_SetCharPtr(edict_t *pEntity, int offset, char *value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	char *cptr = (char *) ptr;
	cptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

unsigned char *Prop_GetUnsignedCharPtr(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned char  *cptr = (unsigned char *) ptr;
	return cptr;
}

void Prop_SetUnsignedCharPtr(edict_t *pEntity, int offset, unsigned char *value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	unsigned char *cptr = (unsigned char *) ptr;
	cptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

bool Prop_GetBool(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	bool *iptr = (bool *) ptr;
	return *iptr;
}

void Prop_SetBool(edict_t *pEntity, int offset, bool value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	bool *iptr = (bool *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

QAngle *Prop_GetQAngle(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	QAngle *iptr = (QAngle *) ptr;
	return iptr;
}

void Prop_SetQAngle(edict_t *pEntity, int offset, QAngle &value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	QAngle *iptr = (QAngle *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

Vector *Prop_GetVector(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	Vector *iptr = (Vector *) ptr;
	return iptr;
}

void Prop_SetVector(edict_t *pEntity, int offset, Vector &value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	Vector *iptr = (Vector *) ptr;
	*iptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

void Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b, byte a)
{
	if (gpManiGameType->prop_index[MANI_PROP_COLOUR].offset == -1) return;
	int offset = gpManiGameType->prop_index[MANI_PROP_COLOUR].offset;

	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	color32 *cptr = (color32 *) ptr;
	cptr->r = r;
	cptr->g = g;
	cptr->b = b;
	cptr->a = a;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

void Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b)
{
	if (gpManiGameType->prop_index[MANI_PROP_COLOUR].offset == -1) return;
	int offset = gpManiGameType->prop_index[MANI_PROP_COLOUR].offset;

	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	color32 *cptr = (color32 *) ptr;
	cptr->r = r;
	cptr->g = g;
	cptr->b = b;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

QAngle *Prop_GetAng(edict_t *entity_ptr)
{
	if (gpManiGameType->prop_index[MANI_PROP_ANG_ROTATION].offset  == -1) return NULL;
	int offset = gpManiGameType->prop_index[MANI_PROP_ANG_ROTATION].offset;
	return Prop_GetQAngle(entity_ptr, offset);
}

void	Prop_SetAng(edict_t *entity_ptr, QAngle &angle_ptr)
{
	if (gpManiGameType->prop_index[MANI_PROP_ANG_ROTATION].offset == -1) return;
	int offset = gpManiGameType->prop_index[MANI_PROP_ANG_ROTATION].offset;
	Prop_SetQAngle(entity_ptr, offset, angle_ptr);
}

Vector *Prop_GetVec(edict_t *entity_ptr)
{
	if (gpManiGameType->prop_index[MANI_PROP_VEC_ORIGIN].offset == -1) return NULL;
	int offset = gpManiGameType->prop_index[MANI_PROP_VEC_ORIGIN].offset;
	return Prop_GetVector(entity_ptr, offset);
}

void	Prop_SetVec(edict_t *entity_ptr, Vector &vector_ptr)
{
	if (gpManiGameType->prop_index[MANI_PROP_VEC_ORIGIN].offset == -1) return;
	int offset = gpManiGameType->prop_index[MANI_PROP_VEC_ORIGIN].offset;
	Prop_SetVector(entity_ptr, offset, vector_ptr);
}

bool	Prop_SetVal(edict_t *entity_ptr, int index, int value)
{
	if (gpManiGameType->prop_index[index].offset == -1) return false;
	int offset = gpManiGameType->prop_index[index].offset;

	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_BOOL:Prop_SetBool(entity_ptr, offset, static_cast<bool>((((value == 0) ? false:true))));break;
	case PROP_INT:Prop_SetInt(entity_ptr, offset, value);break;
	case PROP_CHAR:Prop_SetChar(entity_ptr, offset, static_cast<char>(value));break;
	case PROP_SHORT:Prop_SetShort(entity_ptr, offset, static_cast<short>(value));break;
	case PROP_UNSIGNED_SHORT:Prop_SetUnsignedShort(entity_ptr, offset, static_cast<unsigned short>(value));break;
	case PROP_UNSIGNED_INT:Prop_SetUnsignedInt(entity_ptr, offset, value);break;
	case PROP_UNSIGNED_CHAR:Prop_SetChar(entity_ptr, offset, value);break;
	default : return false;
	}

	return true;
}

int	Prop_GetVal(edict_t *entity_ptr, int index, int default_value)
{
	if (gpManiGameType->prop_index[index].offset == -1) return default_value;
	int offset = gpManiGameType->prop_index[index].offset;

	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_BOOL:return Prop_GetBool(entity_ptr, offset);
	case PROP_INT:return Prop_GetInt(entity_ptr, offset);
	case PROP_CHAR:return Prop_GetChar(entity_ptr, offset);
	case PROP_SHORT:return Prop_GetShort(entity_ptr, offset);
	case PROP_UNSIGNED_SHORT:return Prop_GetUnsignedShort(entity_ptr, offset);
	case PROP_UNSIGNED_INT:return Prop_GetUnsignedInt(entity_ptr, offset);
	case PROP_UNSIGNED_CHAR:return Prop_GetChar(entity_ptr, offset);
	default : break;
	}

	return default_value;
}

bool	Prop_SetVal(edict_t *entity_ptr, int index, char *value)
{
	if (gpManiGameType->prop_index[index].offset == -1) return false;
	int offset = gpManiGameType->prop_index[index].offset;

	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_CHAR_PTR:Prop_SetCharPtr(entity_ptr, offset, value);break;
	case PROP_UNSIGNED_CHAR_PTR:Prop_SetUnsignedCharPtr(entity_ptr, offset, (unsigned char *) value);break;
	default : return false;
	}

	return true;
}

char *Prop_GetVal(edict_t *entity_ptr, int index, char *default_value)
{
	if (gpManiGameType->prop_index[index].offset == -1) return default_value;
	int offset = gpManiGameType->prop_index[index].offset;

	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_CHAR_PTR:return Prop_GetCharPtr(entity_ptr, offset);
	case PROP_UNSIGNED_CHAR_PTR:return (char *) Prop_GetUnsignedCharPtr(entity_ptr, offset);
	default : break;
	}

	return default_value;
}
//********************************************************************
// Map Desc functions

int Map_GetInt(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	int *iptr = (int *)ptr;
	return *iptr;
}

void Map_SetInt(CBaseEntity *p_CBE, int offset, int value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	int *iptr = (int *) ptr;
	*iptr = value;
}

float Map_GetFloat(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	float *fptr = (float *)ptr;
	return *fptr;
}

void Map_SetFloat(CBaseEntity *p_CBE, int offset, float value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	float *fptr = (float *) ptr;
	*fptr = value;
}

char Map_GetChar(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	char *cptr = (char *)ptr;
	return *cptr;
}

void Map_SetChar(CBaseEntity *p_CBE, int offset, long value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	char *iptr = (char *) ptr;
	*iptr = value;
}

short Map_GetShort(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	short *sptr = (short *) ptr;
	return *sptr;
}

void Map_SetShort(CBaseEntity *p_CBE, int offset, short value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	short *sptr = (short *) ptr;
	*sptr = value;
}

char *Map_GetCharPtr(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	char  *cptr = (char *) ptr;
	return cptr;
}

void Map_SetCharPtr(CBaseEntity *p_CBE, int offset, char *value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	char *cptr = (char *) ptr;
	cptr = value;
}

bool Map_GetBool(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	bool *bptr = (bool *) ptr;
	return *bptr;
}

void Map_SetBool(CBaseEntity *p_CBE, int offset, bool value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	bool *bptr = (bool *) ptr;
	*bptr = value;
}

bool	Map_SetVal(CBaseEntity *p_CBE, int index, float value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return false;
	Map_SetFloat(p_CBE, offset, value);
	return true;
}

float Map_GetVal(CBaseEntity *p_CBE, int index, float default_value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return default_value;
	return Map_GetFloat(p_CBE, offset);
}


bool	Map_SetVal(CBaseEntity *p_CBE, int index, int value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return false;

	switch(gpManiGameType->var_index[index].type)
	{
	case PROP_BOOL:Map_SetBool(p_CBE, offset, static_cast<bool>((((value == 0) ? false:true))));break;
	case PROP_INT:Map_SetInt(p_CBE, offset, value);break;
	case PROP_CHAR:Map_SetChar(p_CBE, offset, static_cast<char>(value));break;
	case PROP_SHORT:Map_SetShort(p_CBE, offset, static_cast<short>(value));break;
	default : return false;
	}

	return true;
}

int	Map_GetVal(CBaseEntity *p_CBE, int index, int default_value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return default_value;

	switch(gpManiGameType->var_index[index].type)
	{
	case PROP_BOOL:return Map_GetBool(p_CBE, offset);
	case PROP_INT:return Map_GetInt(p_CBE, offset);
	case PROP_CHAR:return Map_GetChar(p_CBE, offset);
	case PROP_SHORT:return Map_GetShort(p_CBE, offset);
	default : break;
	}

	return default_value;
}

bool	Map_SetVal(CBaseEntity *p_CBE, int index, char *value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return false;
	Map_SetCharPtr(p_CBE, offset, value);
	return true;
}

char *Map_GetVal(CBaseEntity *p_CBE, int index, char *default_value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return default_value;
	return Map_GetCharPtr(p_CBE, offset);
}

bool Map_CanUseMap(CBaseEntity *p_CBE, int index)
{
	if (gpManiGameType->GetPtrIndex(p_CBE, index) == -1) return false;
	return true;
}
