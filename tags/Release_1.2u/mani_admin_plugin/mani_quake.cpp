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
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_quake.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IEngineSound *esounds; // sound
extern	IFileSystem	*filesystem;
extern	INetworkStringTableContainer *networkstringtable;

extern	int	max_players;
extern	bf_write *msg_buffer;
extern	int	text_message_index;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}

track_kill_t	quake_tracked_kills[MANI_MAX_PLAYERS];
quake_sound_t	quake_sound_list[MANI_MAX_QUAKE_SOUNDS]= 
				{
					{"", "firstblood", false},
					{"", "humiliation", false},
					{"", "multikill", false},
					{"", "monsterkill", false},
					{"", "ultrakill", false},
					{"", "godlike", false},
					{"", "headshot", false},
					{"", "dominating", false},
					{"", "holyshit", false},
					{"", "killingspree", false},
					{"", "ludicrouskill", false},
					{"", "prepare", false},
					{"", "rampage", false},
					{"", "unstoppable", false},
					{"", "wickedsick", false},
					{"", "teamkiller", false}
				};


bool			quake_first_blood;

static	void SetupAutoDownloads(void);
static	void ResetQuakePlayer(int index);
static	long IncrementKillStreak (int index, bool *multi_kill);
static	void PlayQuakeSound (player_t *attacker, player_t *victim, int sound_index, int mode);
static	void ShowQuakeSound (player_t *attacker, player_t *victim, int mode, char *fmt, ...);
CONVAR_CALLBACK_PROTO (QuakeAutoDownload);

ConVar mani_quake_auto_download ("mani_quake_auto_download", "0", 0, "0 = Don't auto download files to client, 1 = automatically download files to client", true, 0, true, 1, CONVAR_CALLBACK_REF(QuakeAutoDownload)); 

// Monster kill sounds
ConVar mani_quake_sounds ("mani_quake_sounds", "0", 0, "Turn on quake sounds like headshot, monster kill etc 1 = on, 0 = off", true, 0, true, 1); 
ConVar mani_quake_kill_streak_mode ("mani_quake_kill_streak_mode", "0", 0, "Reset kill streaks per round 1 = per round/death, 0 = per death", true, 0, true, 1); 

ConVar mani_quake_humiliation_mode ("mani_quake_humiliation_mode", "1", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_humiliation_visual_mode ("mani_quake_humiliation_visual_mode", "1", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_humiliation_weapon ("mani_quake_humiliation_weapon", "knife", 0, "Weapon that triggers the humiliation sound");
ConVar mani_quake_humiliation_weapon2 ("mani_quake_humiliation_weapon2", "", 0, "Second weapon that triggers the humiliation sound");

ConVar mani_quake_firstblood_mode ("mani_quake_firstblood_mode", "1", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_firstblood_visual_mode ("mani_quake_firstblood_visual_mode", "1", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_firstblood_reset_per_round ("mani_quake_firstblood_reset_per_round", "1", 0, "CSS Only, 1 = reset per round, 0 = per map", true, 0, true, 1); 

ConVar mani_quake_headshot_mode ("mani_quake_headshot_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_headshot_visual_mode ("mani_quake_headshot_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 

ConVar mani_quake_prepare_to_fight_mode ("mani_quake_prepare_to_fight_mode", "0", 0, "0 = off, 1 = on", true, 0, true, 1); 
ConVar mani_quake_prepare_to_fight_visual_mode ("mani_quake_prepare_to_fight_visual_mode", "0", 0, "0 = off, 1 = on", true, 0, true, 1); 

ConVar mani_quake_multi_kill_mode ("mani_quake_multi_kill_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_multi_kill_visual_mode ("mani_quake_multi_kill_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 

ConVar mani_quake_monster_kill_mode ("mani_quake_monster_kill_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_monster_kill_visual_mode ("mani_quake_monster_kill_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_monster_kill_trigger_count ("mani_quake_monster_kill_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_ultra_kill_mode ("mani_quake_ultra_kill_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_ultra_kill_visual_mode ("mani_quake_ultra_kill_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_ultra_kill_trigger_count ("mani_quake_ultra_kill_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_god_like_mode ("mani_quake_god_like_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_god_like_visual_mode ("mani_quake_god_like_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_god_like_trigger_count ("mani_quake_god_like_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_unstoppable_mode ("mani_quake_unstoppable_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_unstoppable_visual_mode ("mani_quake_unstoppable_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_unstoppable_trigger_count ("mani_quake_unstoppable_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_rampage_mode ("mani_quake_rampage_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_rampage_visual_mode ("mani_quake_rampage_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_rampage_trigger_count ("mani_quake_rampage_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_ludicrous_kill_mode ("mani_quake_ludicrous_kill_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_ludicrous_kill_visual_mode ("mani_quake_ludicrous_kill_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_ludicrous_kill_trigger_count ("mani_quake_ludicrous_kill_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_killing_spree_mode ("mani_quake_killing_spree_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_killing_spree_visual_mode ("mani_quake_killing_spree_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_killing_spree_trigger_count ("mani_quake_killing_spree_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_holy_shit_mode ("mani_quake_holy_shit_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_holy_shit_visual_mode ("mani_quake_holy_shit_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_holy_shit_trigger_count ("mani_quake_holy_shit_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_dominating_mode ("mani_quake_dominating_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_dominating_visual_mode ("mani_quake_dominating_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_dominating_trigger_count ("mani_quake_dominating_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_wicked_sick_mode ("mani_quake_wicked_sick_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_wicked_sick_visual_mode ("mani_quake_wicked_sick_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 
ConVar mani_quake_wicked_sick_trigger_count ("mani_quake_wicked_sick_trigger_count", "0", 0, "Kills streak required to trigger sound", true, 1, true, 99999); 

ConVar mani_quake_team_killer_mode ("mani_quake_team_killer_mode", "0", 0, "0 = off, 1 = all players hear it, 2 = players involved hear it, 3 = attacker hears it, 4 = victim hears it", true, 0, true, 4); 
ConVar mani_quake_team_killer_visual_mode ("mani_quake_team_killer_visual_mode", "0", 0, "0 = off, 1 = all players see it, 2 = players involved see it, 3 = attacker sees it, 4 = victim sees it", true, 0, true, 4); 

//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the quake style sounds
//---------------------------------------------------------------------------------
void	LoadQuakeSounds(void)
{
	FileHandle_t file_handle;
	char	sound_id[512];
	char	sound_name[512];

	char	base_filename[256];

	if (!esounds) return;

	// Reset list
	for (int i = 0; i < MANI_MAX_QUAKE_SOUNDS; i++)
	{
		quake_sound_list[i].in_use = false;
		Q_strcpy(quake_sound_list[i].sound_file,"");
	}

	//Get quake sound list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/quakesoundlist.txt", mani_path.GetString());
	file_handle = filesystem->Open(base_filename, "rt", NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load quakesoundlist.txt\n");
	}
	else
	{
//		MMsg("Quake Style Sound list\n");
		while (filesystem->ReadLine (sound_id, sizeof(sound_id), file_handle) != NULL)
		{
			if (!ParseAliasLine(sound_id, sound_name, true, false))
			{
				// String is empty after parsing
				continue;
			}

			bool found_id = false;

			for (int i = 0; i < MANI_MAX_QUAKE_SOUNDS; i++)
			{
				if (FStrEq(sound_name, quake_sound_list[i].alias))
				{
					char	exists_string[512];

					// Check file exists on server
					snprintf(exists_string, sizeof(exists_string), "./sound/%s", sound_id);
					if (filesystem->FileExists(exists_string))
					{
						Q_strcpy(quake_sound_list[i].sound_file, sound_id);
						quake_sound_list[i].in_use = true;
						found_id = true;
						if (esounds)
						{
							// esounds->PrecacheSound(sound_id, true);
						}

						break;
					}
				}
			}

			if (!found_id)
			{
//				MMsg("WARNING Quake Sound Name [%s] for sound file [%s] is not valid !!\n",
//								sound_name,
//								sound_id);
			}
			else
			{
//				MMsg("Loaded Quake Sound Name [%s] for file [%s]\n", 
//								sound_name,
//								sound_id);
			}
		}

		filesystem->Close(file_handle);
	}

	SetupAutoDownloads();
	ResetQuakeDeaths();
	quake_first_blood = true;
}

//---------------------------------------------------------------------------------
// Purpose: Load and pre-cache the quake style sounds
//---------------------------------------------------------------------------------
static
void	SetupAutoDownloads(void)
{
	if (mani_quake_auto_download.GetInt() == 0) return;

	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
	bool save = engine->LockNetworkStringTables(false);
	char	res_string[512];

	for (int i = 0; i < MANI_MAX_QUAKE_SOUNDS; i++)
	{
		if (quake_sound_list[i].in_use)
		{
 			// Set up .res downloadables
			if (pDownloadablesTable)
			{
				snprintf(res_string, sizeof(res_string), "sound/%s", quake_sound_list[i].sound_file);
#ifdef ORANGE
				pDownloadablesTable->AddString(true, res_string, sizeof(res_string));
#else
				pDownloadablesTable->AddString(res_string, sizeof(res_string));
#endif
			}
		}
	}

	engine->LockNetworkStringTables(save);
}

//---------------------------------------------------------------------------------
// Purpose: Reset everyones kills streaks
//---------------------------------------------------------------------------------
void	ResetQuakeDeaths(void)
{
	for (int i = 0; i < max_players; i++)
	{
		quake_tracked_kills[i].kill_streak = 0;
		quake_tracked_kills[i].last_kill = -99.0;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset everyones kills streaks
//---------------------------------------------------------------------------------
void	ProcessQuakeRoundStart(void)
{
	if (war_mode) return;
	if (mani_quake_sounds.GetInt() == 0) return;

	if (mani_quake_firstblood_reset_per_round.GetInt() == 1)
	{
		quake_first_blood = true;
	}

	if (gpManiGameType->IsGameType(MANI_GAME_CSS) && mani_quake_prepare_to_fight_mode.GetInt() == 1)
	{
		player_t attacker, victim;
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_PREPARE, 1);
		if (mani_quake_prepare_to_fight_visual_mode.GetInt() == 1)
		{
			ShowQuakeSound(&attacker, &victim, 1, Translate(NULL, 800));
		}
	}

	if (mani_quake_kill_streak_mode.GetInt() == 1)
	{
		ResetQuakeDeaths();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Reset everyones kills streaks
//---------------------------------------------------------------------------------
void	ProcessQuakeDeath(IGameEvent *event)
{
	player_t	attacker;
	player_t	victim;
	bool		headshot;
	char		weapon_name[128];

	if (war_mode) return;
	if (mani_quake_sounds.GetInt() == 0) return;

	// Get kill information
	attacker.user_id = event->GetInt("attacker", -1);
	victim.user_id = event->GetInt("userid", -1);
	headshot = event->GetBool("headshot", false);
	Q_strcpy(weapon_name, event->GetString("weapon", ""));
	
	if (victim.user_id == -1)
	{
		return;
	}

	if (!FindPlayerByUserID(&victim)) return;

	if (attacker.user_id == -1 || attacker.user_id == 0)
	{
		// World attacked
		return;
	}

	// Suicide
	if (attacker.user_id == victim.user_id) 
	{
		ResetQuakePlayer (victim.index - 1);
		return;
	}

	// Get attacker information
	if (!FindPlayerByUserID(&attacker)) return;

	if (gpManiGameType->IsTeamPlayAllowed())
	{
		// Team based game
		if (attacker.team == victim.team)
		{
			PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_TEAMKILLER, mani_quake_team_killer_mode.GetInt());
			ShowQuakeSound(&attacker, &victim, mani_quake_team_killer_mode.GetInt(), "%s", Translate(NULL, 815, "%s%s", attacker.name, victim.name));
			// On same team so reset their score
			ResetQuakePlayer (attacker.index - 1);
			return;
		}
	}

	ResetQuakePlayer(victim.index - 1);

	// Need to prioritize sounds
	// 1 - First Blood (first kill)
	// 2 - Humiliation (knife kill)
	// 3 - Dominating
	// 4 - Killing Spree
	// 5 - Monster Kill (multiple kills in one go
	// 6 - Unstoppable
	// 7 - Ultra Kill
	// 8 - God Like
	// 9 - Wicked Sick
	// 10 - Ludicrous Kill
	// 11 - Holy Shit
	// 12 - Head shot 
	
	bool multi_kill;
	int	 kill_streak;

	kill_streak = IncrementKillStreak (attacker.index - 1, &multi_kill);

	// First Blood
	if (quake_first_blood && mani_quake_firstblood_mode.GetInt() != 0)
	{
		quake_first_blood = false;
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_FIRSTBLOOD, mani_quake_firstblood_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_firstblood_visual_mode.GetInt(), "%s", Translate(NULL, 801,"%s", attacker.name));
		return;
	}

	// Humiliation
	if ((FStrEq(mani_quake_humiliation_weapon.GetString(), weapon_name) || FStrEq(mani_quake_humiliation_weapon2.GetString(), weapon_name))
		&& mani_quake_humiliation_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_HUMILIATION, mani_quake_humiliation_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_humiliation_visual_mode.GetInt(), "%s", Translate(NULL, 802, "%s%s", victim.name, attacker.name));
		return;
	}

	// Multi Kill
	if (multi_kill && mani_quake_multi_kill_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_MULTIKILL, mani_quake_multi_kill_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_multi_kill_visual_mode.GetInt(), "%s", Translate(NULL, 803, "%s", attacker.name));
		return;
	}

	// Dominating
	if (kill_streak == mani_quake_dominating_trigger_count.GetInt() && mani_quake_dominating_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_DOMINATING, mani_quake_dominating_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_dominating_visual_mode.GetInt(), "%s", Translate(NULL, 804, "%s", attacker.name));
		return;
	}

	// Rampage
	if (kill_streak == mani_quake_rampage_trigger_count.GetInt() && mani_quake_rampage_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_RAMPAGE, mani_quake_rampage_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_rampage_visual_mode.GetInt(), "%s", Translate(NULL, 805, "%s", attacker.name));
		return;
	}

	// Killing Spree
	if (kill_streak == mani_quake_killing_spree_trigger_count.GetInt() && mani_quake_killing_spree_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_KILLINGSPREE, mani_quake_killing_spree_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_killing_spree_visual_mode.GetInt(), "%s", Translate(NULL, 806, "%s", attacker.name));
		return;
	}

	// Monster Kill
	if (kill_streak == mani_quake_monster_kill_trigger_count.GetInt() && mani_quake_monster_kill_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_MONSTERKILL, mani_quake_monster_kill_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_monster_kill_visual_mode.GetInt(), "%s", Translate(NULL, 807, "%s", attacker.name));
		return;
	}

	// Unstoppable
	if (kill_streak == mani_quake_unstoppable_trigger_count.GetInt() && mani_quake_unstoppable_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_UNSTOPPABLE, mani_quake_unstoppable_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_unstoppable_visual_mode.GetInt(), "%s", Translate(NULL, 808, "%s", attacker.name));
		return;
	}

	// Ultra Kill
	if (kill_streak == mani_quake_ultra_kill_trigger_count.GetInt() && mani_quake_ultra_kill_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_ULTRAKILL, mani_quake_ultra_kill_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_ultra_kill_visual_mode.GetInt(), "%s", Translate(NULL, 809, "%s", attacker.name));
		return;
	}

	// God Like
	if (kill_streak == mani_quake_god_like_trigger_count.GetInt() && mani_quake_god_like_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_GODLIKE, mani_quake_god_like_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_god_like_visual_mode.GetInt(), "%s", Translate(NULL, 810, "%s", attacker.name));
		return;
	}

	// Wicked Sick
	if (kill_streak == mani_quake_wicked_sick_trigger_count.GetInt() && mani_quake_wicked_sick_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_WICKEDSICK, mani_quake_wicked_sick_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_wicked_sick_visual_mode.GetInt(), "%s", Translate(NULL, 811, "%s", attacker.name));
		return;
	}

	// Ludicrous Kill
	if (kill_streak == mani_quake_ludicrous_kill_trigger_count.GetInt() && mani_quake_ludicrous_kill_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_LUDICROUSKILL, mani_quake_ludicrous_kill_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_ludicrous_kill_visual_mode.GetInt(), "%s", Translate(NULL, 812, "%s", attacker.name));
		return;
	}

	// Holy Shit
	if (kill_streak == mani_quake_holy_shit_trigger_count.GetInt() && mani_quake_holy_shit_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_HOLYSHIT, mani_quake_holy_shit_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_holy_shit_visual_mode.GetInt(), "%s", Translate(NULL, 813, "%s", attacker.name));
		return;
	}

	// Head shot
	if (headshot && mani_quake_headshot_mode.GetInt() != 0)
	{
		PlayQuakeSound(&attacker, &victim, MANI_QUAKE_SOUND_HEADSHOT, mani_quake_headshot_mode.GetInt());
		ShowQuakeSound(&attacker, &victim, mani_quake_headshot_visual_mode.GetInt(), "%s", Translate(NULL, 814, "%s", attacker.name));
		return;
	}


}

//---------------------------------------------------------------------------------
// Purpose: Reset individual player streak
//---------------------------------------------------------------------------------
static
void	ResetQuakePlayer (int index)
{
		quake_tracked_kills[index].kill_streak = 0;
		quake_tracked_kills[index].last_kill = -99.0;
}

//---------------------------------------------------------------------------------
// Purpose: Bump up kill streak
//---------------------------------------------------------------------------------
static
long	IncrementKillStreak (int index, bool *multi_kill)
{
		quake_tracked_kills[index].kill_streak ++;
		if (quake_tracked_kills[index].last_kill == gpGlobals->curtime)
		{
			*multi_kill = true;
			quake_tracked_kills[index].last_kill = gpGlobals->curtime + 0.001;
		}
		else if (quake_tracked_kills[index].last_kill < gpGlobals->curtime)
		{
			quake_tracked_kills[index].last_kill = gpGlobals->curtime;
			*multi_kill = false;
		}
		else
		{
			*multi_kill = false;
		}

		return quake_tracked_kills[index].kill_streak;
}

//---------------------------------------------------------------------------------
// Purpose: Player the sound
//---------------------------------------------------------------------------------
static	
void PlayQuakeSound (player_t *attacker, player_t *victim, int sound_index, int mode)
{
	char	client_string[256];

	if (!esounds || mode == 0) return;
	if (!quake_sound_list[sound_index].in_use) return;

	if ( mani_play_sound_type.GetBool() ) 
		snprintf(client_string, sizeof(client_string), "play \"%s\"\n", quake_sound_list[sound_index].sound_file);
	else
		snprintf(client_string, sizeof(client_string), "playgamesound \"%s\"\n", quake_sound_list[sound_index].sound_file);

	if (mode == 1)
	{
		// All players must hear
		for (int i = 1; i <= max_players; i++)
		{
			player_settings_t	*player_settings;
			player_t	sound_player;

			sound_player.index = i;
			if (!FindPlayerByIndex(&sound_player)) continue;
			if (sound_player.is_bot) continue;
			player_settings = FindPlayerSettings(&sound_player);
			if (player_settings && player_settings->quake_sounds)
			{
				//UTIL_EmitSoundSingle(&sound_player, quake_sound_list[sound_index].sound_file);
				engine->ClientCommand(sound_player.entity, client_string);
			}
		}
	}
	else if (mode == 2)
	{
		// Attacker and victim hear it
		player_settings_t	*player_settings;

		if (!attacker->is_bot)
		{
			player_settings = FindPlayerSettings(attacker);
			if (player_settings && player_settings->quake_sounds)
			{
				//UTIL_EmitSoundSingle(attacker, quake_sound_list[sound_index].sound_file);
				engine->ClientCommand(attacker->entity, client_string);
			}
		}

		if (!victim->is_bot)
		{
			player_settings = FindPlayerSettings(victim);
			if (player_settings && player_settings->quake_sounds)
			{
//				UTIL_EmitSoundSingle(victim, quake_sound_list[sound_index].sound_file);
				engine->ClientCommand(victim->entity, client_string);
			}
		}

	}
	else if (mode == 3)
	{
		// Attacker hears it.
		player_settings_t	*player_settings;

		if (!attacker->is_bot)
		{
			player_settings = FindPlayerSettings(attacker);
			if (player_settings && player_settings->quake_sounds)
			{
//				UTIL_EmitSoundSingle(attacker, quake_sound_list[sound_index].sound_file);
				engine->ClientCommand(attacker->entity, client_string);
			}
		}
	}
	else if (mode == 4)
	{
		player_settings_t	*player_settings;

		// Victim hears it.
		if (!victim->is_bot)
		{
			player_settings = FindPlayerSettings(victim);
			if (player_settings && player_settings->quake_sounds)
			{
//				UTIL_EmitSoundSingle(victim, quake_sound_list[sound_index].sound_file);
				engine->ClientCommand(victim->entity, client_string);
			}
		}
	}

}

//---------------------------------------------------------------------------------
// Purpose: Show hud message for kill
//---------------------------------------------------------------------------------
static	
void ShowQuakeSound (player_t *attacker, player_t *victim, int mode, char *fmt, ...)
{
	va_list		argptr;
	char		temp_string[256];
	

	va_start ( argptr, fmt );
	vsnprintf( temp_string, sizeof(temp_string), fmt, argptr );
	va_end   ( argptr );

	MRecipientFilter mrf;

	if (mode == 1)
	{
		mrf.RemoveAllRecipients();
		mrf.MakeReliable();

		// All players must see
		for (int i = 1; i <= max_players; i++)
		{

			player_settings_t	*player_settings;
			player_t			sound_player;

			sound_player.index = i;
			if (!FindPlayerByIndex(&sound_player)) continue;
			if (sound_player.is_bot) continue;
			
			player_settings = FindPlayerSettings(&sound_player);
			if (player_settings && player_settings->quake_sounds) 
			{
				mrf.AddPlayer(sound_player.index);
			}
		}
	}
	else if (mode == 2)
	{
		player_settings_t	*player_settings;

		player_settings = FindPlayerSettings(attacker);
		if (player_settings && player_settings->quake_sounds)
		{
			mrf.AddPlayer(attacker->index);
		}

		player_settings = FindPlayerSettings(victim);
		if (player_settings && player_settings->quake_sounds)
		{
			mrf.AddPlayer(victim->index);
		}
	}
	else if (mode == 3)
	{
		player_settings_t	*player_settings;
		player_settings = FindPlayerSettings(attacker);
		if (player_settings && player_settings->quake_sounds)
		{
			// Attacker sees it.
			mrf.AddPlayer(attacker->index);
		}
	}
	else if (mode == 4)
	{
		player_settings_t	*player_settings;
		player_settings = FindPlayerSettings(victim);
		if (player_settings && player_settings->quake_sounds)
		{
			// Victim sees it.
			mrf.AddPlayer(victim->index);
		}
	}

	msg_buffer = engine->UserMessageBegin( &mrf, text_message_index ); // Show TextMsg type user message
	msg_buffer->WriteByte(4); // Center area
	msg_buffer->WriteString(temp_string);
	engine->MessageEnd();
}

//---------------------------------------------------------------------------------
// Purpose: Handle change of quake sound download cvar
//---------------------------------------------------------------------------------
CONVAR_CALLBACK_FN(QuakeAutoDownload)
{
	if (!FStrEq(pOldString, mani_quake_auto_download.GetString()))
	{
		if (atoi(mani_quake_auto_download.GetString()) == 1)
		{
			SetupAutoDownloads();
		}
	}
}
