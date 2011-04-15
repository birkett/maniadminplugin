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



#ifndef MANI_VARS_H
#define MANI_VARS_H

extern	int	UTIL_CalculatePropType(int var_type, int bits, bool is_signed);
extern	int	UTIL_GetVarValue(CBaseEntity *pCBE, const char *var_id, int &type);
extern	int	UTIL_FindPropOffset(const char *CombinedProp, int &type, bool find_type);
extern	bool UTIL_SplitCombinedProp(const char *source, char *part1, char *part2);

extern	ManiGameType *gpManiGameType;

// Prop manager calls
extern	int			Prop_GetInt(edict_t *pEntity, int offset);
extern	void		Prop_SetInt(edict_t *pEntity, int offset, int value);

extern	unsigned int Prop_GetUnsignedInt(edict_t *pEntity, int offset);
extern	void		Prop_SetUnsignedInt(edict_t *pEntity, int offset, int value);

extern	long		Prop_GetLong(edict_t *pEntity, int offset);
extern	void		Prop_SetLong(edict_t *pEntity, int offset, long value);

extern	unsigned long Prop_GetUnsignedLong(edict_t *pEntity, int offset);
extern	void		Prop_SetUnsignedLong(edict_t *pEntity, int offset, long value);

extern	char		Prop_GetChar(edict_t *pEntity, int offset);
extern	void		Prop_SetChar(edict_t *pEntity, int offset, long value);

extern	unsigned char Prop_GetUnsignedChar(edict_t *pEntity, int offset);
extern	void		Prop_SetUnsignedChar(edict_t *pEntity, int offset, unsigned char value);

extern	short		Prop_GetShort(edict_t *pEntity, int offset);
extern	void		Prop_SetShort(edict_t *pEntity, int offset, short value);

extern  unsigned short Prop_GetUnsignedShort(edict_t *entity_ptr, int offset);
extern  void		Prop_SetUnsignedShort(edict_t *entity_ptr, int offset, unsigned short value);

extern	char		*Prop_GetCharPtr(edict_t *pEntity, int offset);
extern	void		Prop_SetCharPtr(edict_t *pEntity, int offset, char *value);

extern	unsigned char *Prop_GetUnsignedCharPtr(edict_t *pEntity, int offset);
extern	void		Prop_SetUnsignedCharPtr(edict_t *pEntity, int offset, unsigned char *value);

extern	bool		Prop_GetBool(edict_t *pEntity, int offset);
extern	void		Prop_SetBool(edict_t *pEntity, int offset, bool value);

extern	QAngle		*Prop_GetQAngle(edict_t *pEntity, int offset);
extern	void		Prop_SetQAngle(edict_t *pEntity, int offset, QAngle &value);

extern	Vector		*Prop_GetVector(edict_t *pEntity, int offset);
extern	void		Prop_SetVector(edict_t *pEntity, int offset, Vector &value);

extern	void		Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b, byte a);
extern	void		Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b);

extern	QAngle		*Prop_GetAng(edict_t *entity_ptr);
extern	void		Prop_SetAng(edict_t *entity_ptr, QAngle &angle_ptr);

extern	Vector		*Prop_GetVec(edict_t *entity_ptr);
extern	void		Prop_SetVec(edict_t *entity_ptr, Vector &vector_ptr);

extern	bool		Prop_SetVal(edict_t *entity_ptr, int index, int value);
extern	int			Prop_GetVal(edict_t *entity_ptr, int index, int default_value);
extern	bool		Prop_SetVal(edict_t *entity_ptr, int index, char *value);
extern	char		*Prop_GetVal(edict_t *entity_ptr, int index, char *default_value);

extern	int		Map_GetInt(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetInt(CBaseEntity *p_CBE, int offset, int value);

extern	float	Map_GetFloat(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetFloat(CBaseEntity *p_CBE, int offset, float value);

extern	char	Map_GetChar(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetChar(CBaseEntity *p_CBE, int offset, long value);

extern	short	Map_GetShort(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetShort(CBaseEntity *p_CBE, int offset, short value);

extern	char    *Map_GetCharPtr(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetCharPtr(CBaseEntity *p_CBE, int offset, char *value);

extern	bool	Map_GetBool(CBaseEntity *p_CBE, int offset);
extern	void	Map_SetBool(CBaseEntity *p_CBE, int offset, bool value);

extern	bool	Map_SetVal(CBaseEntity *p_CBE, int index, float value);
extern	float	Map_GetVal(CBaseEntity *p_CBE, int index, float default_value);

extern	bool	Map_CanUseMap(CBaseEntity *p_CBE, int index);

extern	bool	Map_SetVal(CBaseEntity *p_CBE, int index, int value);
extern	int		Map_GetVal(CBaseEntity *p_CBE, int index, int default_value);
extern	bool	Map_SetVal(CBaseEntity *p_CBE, int index, char *value);
extern	char	*Map_GetVal(CBaseEntity *p_CBE, int index, char *default_value);

// Prop manager calls
/*template <class X> X Prop_Get(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	X *nptr = (X *)ptr;
	return *nptr;
};

template <class X> void Prop_Set(edict_t *pEntity, int offset, X value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	X *nptr = (X *) ptr;
	*nptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

template <class X> X Prop_GetStr(edict_t *pEntity, int offset)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	X nptr = (X)ptr;
	return nptr;
};

template <class X> void Prop_SetStr(edict_t *pEntity, int offset, X value)
{
	unsigned char *ptr = (unsigned char *) pEntity->GetUnknown() + offset;
	X nptr = (X) ptr;
	nptr = value;
    pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}

template <class X> bool Prop_SetVal(edict_t *entity_ptr, int index, X value)
{
	if (gpManiGameType->prop_index[index].offset == -1) return false;
	int offset = gpManiGameType->prop_index[index].offset;

	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_BOOL:Prop_Set(entity_ptr, offset, static_cast<bool>((((value == 0) ? false:true))));break;
	case PROP_INT:Prop_Set(entity_ptr, offset, value);break;
	case PROP_CHAR:Prop_Set(entity_ptr, offset, static_cast<char>(value));break;
	case PROP_SHORT:Prop_Set(entity_ptr, offset, static_cast<short>(value));break;
	case PROP_UNSIGNED_SHORT:Prop_Set(entity_ptr, offset, static_cast<unsigned short>(value));break;
	case PROP_UNSIGNED_INT:Prop_Set(entity_ptr, offset, static_cast<unsigned int>(value));break;
	case PROP_UNSIGNED_CHAR:Prop_Set(entity_ptr, offset, static_cast<unsigned char>(value));break;
	case PROP_CHAR_PTR:Prop_SetStr(entity_ptr, offset, reinterpret_cast<char *>(value));break;
	case PROP_UNSIGNED_CHAR_PTR:Prop_SetStr(entity_ptr, offset, reinterpret_cast<unsigned char *>(value));break;
	default : return false;
	}

	return true;
};

template <class X> X Prop_GetVal(edict_t *entity_ptr, int index, X default_val)
{
	if (gpManiGameType->prop_index[index].offset == -1) return default_val;
	int offset = gpManiGameType->prop_index[index].offset;
	switch(gpManiGameType->prop_index[index].type)
	{
	case PROP_BOOL:return (X)(Prop_Get<bool>(entity_ptr, offset));
	case PROP_INT:return (X)(Prop_Get<int>(entity_ptr, offset));
	case PROP_CHAR:return (X)(Prop_Get<char>(entity_ptr, offset));
	case PROP_SHORT:return (X)(Prop_Get<short>(entity_ptr, offset));
	case PROP_UNSIGNED_SHORT:return (X)(Prop_Get<unsigned short>(entity_ptr, offset));
	case PROP_UNSIGNED_INT:return (X)(Prop_Get<unsigned int>(entity_ptr, offset));
	case PROP_UNSIGNED_CHAR:return (X)(Prop_Get<unsigned char>(entity_ptr, offset));
	case PROP_CHAR_PTR:return (X)(Prop_GetStr<char *>(entity_ptr, offset));
	case PROP_UNSIGNED_CHAR_PTR:return (X)(Prop_GetStr<unsigned char *>(entity_ptr, offset));
	default : break;
	}

	return default_val;
}

template <class X> X Map_Get(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	X *nptr = (X *)ptr;
	return *nptr;
};

template <class X> void Map_Set(CBaseEntity *p_CBE, int offset, X value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	X *nptr = (X *) ptr;
	*nptr = value;
}

template <class X> X Map_GetStr(CBaseEntity *p_CBE, int offset)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	X nptr = (X )ptr;
	return nptr;
};

template <class X> void Map_SetStr(CBaseEntity *p_CBE, int offset, X value)
{
	unsigned char *ptr = (unsigned char *) p_CBE + offset;
	X nptr = (X ) ptr;
	nptr = value;
}

template <class X> bool Map_SetVal(CBaseEntity *p_CBE, int index, X value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return false;

	switch(gpManiGameType->var_index[index].type)
	{
	case PROP_BOOL:Map_Set(p_CBE, offset, static_cast<bool>((((value == 0) ? false:true))));break;
	case PROP_INT:Map_Set(p_CBE, offset, value);break;
	case PROP_CHAR:Map_Set(p_CBE, offset, static_cast<char>(value));break;
	case PROP_SHORT:Map_Set(p_CBE, offset, static_cast<short>(value));break;
	case PROP_FLOAT:Map_Set(p_CBE, offset, static_cast<float>(value));break;
	default : return false;
	}

	return true;
}

template <class X> X Map_GetVal(CBaseEntity *p_CBE, int index, X default_val)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return default_val;

	switch(gpManiGameType->var_index[index].type)
	{
	case PROP_BOOL:return (X)(Map_Get<bool>(p_CBE, offset));
	case PROP_INT:return (X)(Map_Get<int>(p_CBE, offset));
	case PROP_CHAR:return (X)(Map_Get<char>(p_CBE, offset));
	case PROP_SHORT:return (X)(Map_Get<short>(p_CBE, offset));
	case PROP_FLOAT:return (X)(Map_Get<float>(p_CBE, offset));
	default : break;
	}

	return default_val;
}

template <class X> bool Map_SetString(CBaseEntity *p_CBE, int index, X value)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return false;
	Map_SetStr(p_CBE, offset, static_cast<char *>(value));
	return true;
}

template <class X> X Map_GetString(CBaseEntity *p_CBE, int index, X default_val)
{
	int offset;
	if ((offset = gpManiGameType->GetPtrIndex(p_CBE, index)) == -1) return default_val;
	return (X)(Map_Get<char *>(p_CBE, offset));
}

extern	QAngle *Prop_GetQAngle(edict_t *pEntity, int offset);
extern	void Prop_SetQAngle(edict_t *pEntity, int offset, QAngle &value);
extern	Vector *Prop_GetVector(edict_t *pEntity, int offset);
extern	void Prop_SetVector(edict_t *pEntity, int offset, Vector &value);
extern	void Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b, byte a);
extern	void Prop_SetColor(edict_t *pEntity, byte r, byte g, byte b);
extern	QAngle *Prop_GetAng(edict_t *entity_ptr);
extern	void	Prop_SetAng(edict_t *entity_ptr, QAngle &angle_ptr);
extern	Vector *Prop_GetVec(edict_t *entity_ptr);
extern	void	Prop_SetVec(edict_t *entity_ptr, Vector &vector_ptr);
extern	bool Map_CanUseMap(CBaseEntity *p_CBE, int index);
*/

#endif
