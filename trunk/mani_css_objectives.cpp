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
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_warmuptimer.h"
#include "mani_effects.h"
#include "mani_css_objectives.h" 
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;

ConVar mani_css_objectives ("mani_css_objectives", "0", 0, "0 = css objective punishments disabled, 1 = css objective punishments enabled", true, 0, true, 1);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

ManiCSSObjectives::ManiCSSObjectives()
{
	gpManiCSSObjectives = this;
}

ManiCSSObjectives::~ManiCSSObjectives()
{
	// Cleanup
}

//---------------------------------------------------------------------------------
// Purpose: Process the round ending
//---------------------------------------------------------------------------------
void ManiCSSObjectives::CSSRoundEnd(int winning_team, const char *message)
{
	if (war_mode) return;
	if (mani_css_objectives.GetInt() == 0) return;
	if (gpManiWarmupTimer->InWarmupRound()) return;

	if (FStrEq(message, "#Game_Commencing") || 
		FStrEq(message, "#Round_Draw")) return;

	bool found_player = false;

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;

		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.team != 2 && player.team != 3) continue;
		if (player.team == winning_team) continue;
		if (player.is_dead) continue;

		found_player = true;

		// Slay the player
		SlayPlayer(&player, false, false, false);
	}

	if (!found_player) return;

	int	help_index = 0;

	if (winning_team == 2)
	{
		// T's won
		help_index = 1360;
	}
	else
	{
		// CT's won
		help_index = 1361;
	}

	SayToAll(LIGHT_GREEN_CHAT, true, "%s", Translate(help_index));
}

ManiCSSObjectives	g_ManiCSSObjectives;
ManiCSSObjectives	*gpManiCSSObjectives;