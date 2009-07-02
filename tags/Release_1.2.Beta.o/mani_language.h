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



#ifndef MANI_LANGUAGE_H
#define MANI_LANGUAGE_H

#include <vector>
#include "mani_player.h"

class CTranslate
{
	struct str_result
	{
		str_result() {result[0]='\0';}
		char	result[2048];
	};
public:
	CTranslate(const int translation_id)
	: translate_id  (translation_id)
	{}

	template <class X> CTranslate& p(const char *fmt, const X value)
	{
		array.push_back(new char[2048]);
		Q_snprintf(array[array.size() - 1], 2048, fmt, value);
		return *this;
	};

	CTranslate& p(const int value)
	{
		array.push_back(new char[2048]);
		Q_snprintf(array[array.size() - 1], 2048, "%i", value);
		return *this;
	};

	CTranslate& p(const char value)
	{
		array.push_back(new char[2048]);
		Q_snprintf(array[array.size() - 1], 2048, "%c", value);
		return *this;
	};

	CTranslate& p(const char *value)
	{
		array.push_back(new char[2048]);
		Q_snprintf(array[array.size() - 1], 2048, "%s", value);
		return *this;
	};

	const int	GetSize() {return (int) array.size();}
	const char *GetString(int index) {return array[index];}
	const int	GetID() {return translate_id;}
private:
	std::vector<char *> array;
	int	translate_id;
};



extern	bool LoadLanguage(void);
extern  void FreeLanguage(void);
extern  char *Translate(player_t *player_ptr, int translate_id);
extern	char *Translate(player_t *player_ptr, int translate_id,  const char *fmt1, ...);
extern	char *Translate(player_t *player_ptr, CTranslate& trans_obj);

extern  void LanguageGameFrame(void);

// I did have loads of #defines in here for each translation index but it is
// easier just to use numbers directly in the source code for non-repeating
// translations. This saves having to look at 3 different files when examining
// each translation, you only need the source code file and english.cfg open.

#define M_MENU_YES (670)
#define M_MENU_NO (671)
#define M_MENU_BACK (672)
#define M_MENU_MORE (673)
#define M_MENU_BACK_AMX (674)
#define M_MENU_MORE_AMX (675)
#define M_MENU_EXIT_AMX (676)

#define M_STATS_KILL_SINGLE (1000)
#define M_STATS_KILL_PLURAL (1001)
#define M_STATS_DEATH_SINGLE (1002)
#define M_STATS_DEATH_PLURAL (1003)

#define M_STATS_HEADSHOTS (1012)

// Victim stats
#define M_VSTATS_BODY (1100)
#define M_VSTATS_HEAD (1101)
#define M_VSTATS_RIGHT_LEG (1102)
#define M_VSTATS_LEFT_LEG (1103)
#define M_VSTATS_RIGHT_ARM (1104)
#define M_VSTATS_LEFT_ARM (1105)
#define M_VSTATS_STOMACH (1106)
#define M_VSTATS_CHEST (1107)
#define M_VSTATS_GEAR (1108)

#define M_VSTATS_HIT_SINGLE (1121)
#define M_VSTATS_HIT_PLURAL (1122)
#define M_VSTATS_HS (1123)

#define M_NO_TARGET (1260)
#define M_TARGET_BOT (1261)
#define M_TARGET_DEAD (1262)
#define M_TARGET_UNDER_TK (1263)
#define M_OFF (1264)
#define M_ON (1265)
#define M_NONE (1266)

#endif