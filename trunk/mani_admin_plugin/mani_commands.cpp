//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
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
#include "ivoiceserver.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"
#include "mani_timers.h"
#include "mani_player.h"
#include "mani_client_flags.h"
#include "mani_language.h"
#include "mani_gametype.h"
#include "mani_memory.h"
#include "mani_parser.h"
#include "mani_convar.h"
#include "mani_client.h"
#include "mani_menu.h"
#include "mani_output.h"
#include "mani_panel.h"
#include "mani_quake.h"
#include "mani_crontab.h"
#include "mani_adverts.h"
#include "mani_maps.h"
#include "mani_stats.h"
#include "mani_replace.h"
#include "mani_effects.h"
#include "mani_sounds.h"
#include "mani_voice.h"
#include "mani_skins.h"
#include "mani_weapon.h"
#include "mani_team.h"
#include "mani_teamkill.h"
#include "mani_ghost.h"
#include "mani_customeffects.h"
#include "mani_mapadverts.h"
#include "mani_downloads.h"
#include "mani_victimstats.h"
#include "mani_database.h"
#include "mani_netidvalid.h"
#include "mani_autokickban.h"
#include "mani_reservedslot.h"
#include "mani_spawnpoints.h"
#include "mani_sprayremove.h"
#include "mani_chattrigger.h"
#include "mani_warmuptimer.h"
#include "mani_log_css_stats.h"
#include "mani_log_dods_stats.h"
#include "mani_mostdestructive.h"
#include "mani_messagemode.h"
#include "mani_trackuser.h"
#include "mani_save_scores.h"
#include "mani_team_join.h"
#include "mani_afk.h"
#include "mani_ping.h"
#include "mani_mysql.h"
#include "mani_vote.h"
#include "mani_observer_track.h"
#include "mani_team.h"
#include "mani_file.h"
#include "mani_vfuncs.h"
#include "mani_help.h"
#include "mani_mainclass.h"
#include "mani_callback_sourcemm.h"
#include "mani_callback_valve.h"
#include "mani_commands.h"
#include "mani_sigscan.h"
#include "mani_globals.h"
#include "mani_commands.h"
#include "shareddefs.h"
#include "mani_command_control.h"

extern	IVEngineServer *engine;
extern	IPlayerInfoManager *playerinfomanager;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	int	con_command_index;
static int sort_by_cmd_name ( const void *m1,  const void *m2);

static int sort_by_cmd_name ( const void *m1,  const void *m2) 
{
	struct cmd_t *mi1 = (struct cmd_t *) m1;
	struct cmd_t *mi2 = (struct cmd_t *) m2;
	return stricmp(mi1->cmd_name, mi2->cmd_name);
}

ConVar mani_say_command_prefix ("mani_say_command_prefix", "@", 0, "Prefix to use for chat commands, default = @");


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

static	char		*cmd_null_string = "";

ManiCommands	g_Cmd;
ManiCommands	*gpCmd;

ManiCommands::ManiCommands()
{
	// Init
	cmd_list = NULL;
	cmd_list_size = 0;

	gpCmd = this;
}

ManiCommands::~ManiCommands()
{
	// Cleanup
	this->CleanUp();
}

//---------------------------------------------------------------------------------
// Purpose: Clean our list
//---------------------------------------------------------------------------------
void ManiCommands::CleanUp(void)
{
	for (int i = 0; i < cmd_list_size; i ++)
	{
		free(cmd_list[i].cmd_name);
	}

	FreeList((void **) &cmd_list, &cmd_list_size); 
}

//---------------------------------------------------------------------------------
// Purpose: Init stuff for plugin load
//---------------------------------------------------------------------------------
void ManiCommands::Load(void)
{
	// Register the commands that can be used in the plugin
	this->CleanUp();
	this->RegisterCommand("ma_say", 2005, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSay);
	this->RegisterCommand("ma_msay", 2007, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMSay);
	this->RegisterCommand("ma_psay", 2009, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPSay);
	this->RegisterCommand("ma_pmess", 2233, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPMess);
	this->RegisterCommand("ma_exit", 2235, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaExit);
	this->RegisterCommand("ma_chat", 2011, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaChat);
	this->RegisterCommand("ma_csay", 2013, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCSay);
	this->RegisterCommand("ma_session", 2015, /*admin?*/ false, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSession);
	this->RegisterCommand("ma_statsme", 2017, /*admin?*/ false, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaStatsMe);
	this->RegisterCommand("ma_rcon", 2019, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRCon);
	this->RegisterCommand("ma_browse", 2021, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBrowse);
	this->RegisterCommand("ma_cexec", 2023, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCExec);
	this->RegisterCommand("ma_cexec_t", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCExecT);
	this->RegisterCommand("ma_cexec_ct", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCExecCT);
	this->RegisterCommand("ma_cexec_all", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCExecAll);
	this->RegisterCommand("ma_cexec_spec", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaCExecSpec);
	this->RegisterCommand("ma_slap", 2025, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSlap);
	this->RegisterCommand("ma_setflag", 0, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSetFlag);
	this->RegisterCommand("ma_setskin", 2027, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSetSkin);
	this->RegisterCommand("ma_setcash", 2029, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSetCash);
	this->RegisterCommand("ma_givecash", 2031, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveCash);
	this->RegisterCommand("ma_givecashp", 2033, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveCashP);
	this->RegisterCommand("ma_takecash", 2035, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTakeCash);
	this->RegisterCommand("ma_takecashp", 2037, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTakeCashP);
	this->RegisterCommand("ma_sethealth", 2039, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSetHealth);
	this->RegisterCommand("ma_givehealth", 2041, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveHealth);
	this->RegisterCommand("ma_givehealthp", 2043, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveHealthP);
	this->RegisterCommand("ma_takehealth", 2045, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTakeHealth);
	this->RegisterCommand("ma_takehealthp", 2047, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTakeHealthP);
	this->RegisterCommand("ma_blind", 2049, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBlind);
	this->RegisterCommand("ma_freeze", 2051, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaFreeze);
	this->RegisterCommand("ma_noclip", 2053, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaNoClip);
	this->RegisterCommand("ma_burn", 2055, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBurn);
	this->RegisterCommand("ma_gravity", 2057, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGravity);
	this->RegisterCommand("ma_colour", 2059, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaColour);
	this->RegisterCommand("ma_color", 2061, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaColour);
	this->RegisterCommand("ma_colour_weapon", 2063, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaColourWeapon);
	this->RegisterCommand("ma_color_weapon", 2065, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaColourWeapon);
	this->RegisterCommand("ma_render_mode", 2067, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRenderMode);
	this->RegisterCommand("ma_render_fx", 2069, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRenderFX);
	this->RegisterCommand("ma_give", 2071, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGive);
	this->RegisterCommand("ma_give_ammo", 2073, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveAmmo);
	this->RegisterCommand("ma_giveammo", 2073, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaGiveAmmo);
	this->RegisterCommand("ma_drug", 2075, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaDrug);
	this->RegisterCommand("ma_decal", 2077, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaDecal);
	this->RegisterCommand("ma_timebomb", 2081, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTimeBomb);
	this->RegisterCommand("ma_freezebomb", 2083, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaFreezeBomb);
	this->RegisterCommand("ma_firebomb", 2085, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaFireBomb);
	this->RegisterCommand("ma_beacon", 2087, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBeacon);
	this->RegisterCommand("ma_mute", 2089, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMute);
	this->RegisterCommand("ma_teleport", 2091, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTeleport);
	this->RegisterCommand("ma_position", 2093, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPosition);
	this->RegisterCommand("ma_swapteam", 2095, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSwapTeam);
	this->RegisterCommand("ma_swapteam_d", 2237, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSwapTeamD);
	this->RegisterCommand("ma_spec", 2097, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSpec);
	this->RegisterCommand("ma_balance", 2099, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBalance);
	this->RegisterCommand("ma_dropc4", 2101, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaDropC4);
	this->RegisterCommand("ma_saveloc", 2103, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSaveLoc);
	this->RegisterCommand("ma_resetrank", 2105, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaResetRank);
	this->RegisterCommand("ma_map", 2107, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMap);
	this->RegisterCommand("ma_skipmap", 2109, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSkipMap);
	this->RegisterCommand("ma_nextmap", 2111, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaNextMap);
	this->RegisterCommand("ma_listmaps", 2113, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaListMaps);
	this->RegisterCommand("ma_maplist", 2115, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMapList);
	this->RegisterCommand("ma_maphistory", 2227, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMapHistory);
	this->RegisterCommand("ma_mapcycle", 2117, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaMapCycle);
	this->RegisterCommand("ma_votemaplist", 2119, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteMapList);
	this->RegisterCommand("ma_war", 2121, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaWar);
	this->RegisterCommand("ma_setnextmap", 2123, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSetNextMap);
	this->RegisterCommand("ma_voterandom", 2125, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteRandom);
	this->RegisterCommand("ma_voteextend", 2127, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteExtend);
	this->RegisterCommand("ma_votercon", 2129, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteRCon);
	this->RegisterCommand("ma_vote", 2131, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVote);
	this->RegisterCommand("ma_votequestion", 2133, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteQuestion);
	this->RegisterCommand("ma_votecancel", 2135, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaVoteCancel);
	this->RegisterCommand("ma_play", 2137, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPlaySound);
	this->RegisterCommand("ma_showrestrict", 2139, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaShowRestrict);
	this->RegisterCommand("ma_restrict", 2141, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRestrict);
	this->RegisterCommand("ma_knives", 2143, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaKnives);
	this->RegisterCommand("ma_pistols", 2145, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPistols);
	this->RegisterCommand("ma_shotguns", 2147, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaShotguns);
	this->RegisterCommand("ma_nosnipers", 2149, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaNoSnipers);
	this->RegisterCommand("ma_unrestrict", 2151, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnRestrict);
	this->RegisterCommand("ma_unrestrictall", 2153, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnRestrictAll);
	this->RegisterCommand("ma_restrictall", 2225, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRestrictAll);
	this->RegisterCommand("ma_tklist", 2155, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTKList);
	this->RegisterCommand("ma_kick", 2157, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaKick);
	this->RegisterCommand("ma_chattriggers", 2159, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaChatTriggers);
	this->RegisterCommand("ma_spray", 2161, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSpray);
	this->RegisterCommand("ma_slay", 2163, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaSlay);
	this->RegisterCommand("ma_offset", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaOffset);
	this->RegisterCommand("ma_offsetscan", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaOffsetScan);
	this->RegisterCommand("ma_offsetscanf", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaOffsetScanF);
	this->RegisterCommand("ma_teamindex", 2165, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTeamIndex);
	this->RegisterCommand("ma_client", 2221, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaClient);
	this->RegisterCommand("ma_clientgroup", 2223, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaClientGroup);
	this->RegisterCommand("ma_reloadclients", 2167, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaReloadClients);
	this->RegisterCommand("ma_ban", 2169, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBan);
	this->RegisterCommand("ma_banip", 2171, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaBanIP);
	this->RegisterCommand("ma_unban", 2173, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnBan);
	this->RegisterCommand("ma_favourites", 2175, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaFavourites);
	this->RegisterCommand("ma_rates", 2177, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRates);
	this->RegisterCommand("ma_users", 2179, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUsers);
	this->RegisterCommand("ma_showsounds", 2181, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaShowSounds);
	this->RegisterCommand("ma_config", 2183, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaConfig);
	this->RegisterCommand("ma_timeleft", 2185, /*admin?*/ false, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaTimeLeft);
	this->RegisterCommand("ma_aban_name", 2187, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoBanName);
	this->RegisterCommand("ma_aban_pname", 2189, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoBanPName);
	this->RegisterCommand("ma_akick_name", 2191, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoKickName);
	this->RegisterCommand("ma_akick_pname", 2193, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoKickPName);
	this->RegisterCommand("ma_akick_steam", 2195, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoKickSteam);
	this->RegisterCommand("ma_akick_ip", 2197, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoKickIP);
	this->RegisterCommand("ma_unauto_name", 2199, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnAutoName);
	this->RegisterCommand("ma_unauto_pname", 2201, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnAutoPName);
	this->RegisterCommand("ma_unauto_steam", 2203, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnAutoSteam);
	this->RegisterCommand("ma_unauto_ip", 2205, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaUnAutoIP);
	this->RegisterCommand("ma_ashow_name", 2207, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoShowName);
	this->RegisterCommand("ma_ashow_pname", 2209, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoShowPName);
	this->RegisterCommand("ma_ashow_steam", 2211, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoShowSteam);
	this->RegisterCommand("ma_ashow_ip", 2213, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAutoShowIP);
	this->RegisterCommand("ma_ranks", 2215, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRanks);
	this->RegisterCommand("ma_plranks", 2217, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaPLRanks);
	this->RegisterCommand("ma_help", 2219, /*admin?*/ false, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaHelp);
	this->RegisterCommand("ma_effect", 0, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ false, /*tsay ?*/ false, /*say ?*/ false, &ManiCommands::MaEffect);
	this->RegisterCommand("ma_restrictratio", 2229, /*admin?*/ true, /*war ?*/ false, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaRestrictRatio);
	this->RegisterCommand("ma_admins", 2231, /*admin?*/ true, /*war ?*/ true, /*server ?*/ true, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaAdmins);
	this->RegisterCommand("ma_observe", 2239, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaObserve);
	this->RegisterCommand("ma_endobserve", 2241, /*admin?*/ true, /*war ?*/ false, /*server ?*/ false, /*client con ?*/ true, /*tsay ?*/ true, /*say ?*/ true, &ManiCommands::MaEndObserve);
	
	// Register Menu commands triggered via client console
	//this->RegisterCommand("mam_plranks", /*war ?*/ false, &ManiCommands::MaPLRanks);

	// Sort so we can use a bsearch on the commands, maybe use hash index for them in the future ?
	qsort(cmd_list, cmd_list_size, sizeof(cmd_t), sort_by_cmd_name); 
	this->WriteHelpHTML();

// Do some tests

	//float curtime = engine->Time();
	//int	max_tests = 1000000;
	//char found_print[256];
	//cmd_t	cmd_key;
	//cmd_t	*found_cmd;

	//srand(0);

	//for (int i = 0;i < max_tests; i ++)
	//{
	//	int fetch = rand() % cmd_list_size;

	//	Q_strcpy(cmd_key.cmd_name, cmd_list[fetch].cmd_name);

	//	// Do BSearch for function name
	//	found_cmd = (cmd_t *) bsearch
	//					(
	//					&cmd_key, 
	//					cmd_list, 
	//					cmd_list_size, 
	//					sizeof(cmd_t), 
	//					sort_by_cmd_name
	//					);

	//	if (found_cmd) 
	//	{
	//		// function found
	//		Q_strcpy(found_print, found_cmd->cmd_name);
	//	}
	//}

	//Msg("Time taken %f, %s\n", engine->Time() - curtime, found_print);

	//srand(0);

	//for (int i = 0;i < max_tests; i ++)
	//{
	//	int fetch = rand() % cmd_list_size;

	//	Q_strcpy(cmd_key.cmd_name, cmd_list[fetch].cmd_name);

	//	for (int j = 0; j < cmd_list_size; j++)
	//	{
	//		if (FStrEq(cmd_key.cmd_name, cmd_list[j].cmd_name))
	//		{
	//			// function found
	//			Q_strcpy(found_print, found_cmd->cmd_name);
	//			break;
	//		}
	//	}
	//}

	//Msg("Time taken %f, %s\n", engine->Time() - curtime, found_print);
	//while(1)
	//{
	//	Sleep(1);
	//}
}

//---------------------------------------------------------------------------------
// Purpose: Plugin is unloaded
//---------------------------------------------------------------------------------
void ManiCommands::Unload(void)
{
	this->CleanUp(); 
}

//---------------------------------------------------------------------------------
// Purpose: Get command index
//---------------------------------------------------------------------------------
int	ManiCommands::GetCmdIndexForHelpID
(
 const int help_id
 )
{
	for (int i = 0; i < cmd_list_size; i ++)
	{
		if (help_id == cmd_list[i].help_id)
		{
			return i;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Show all the help commands
//---------------------------------------------------------------------------------
void	ManiCommands::ShowAllCommands
(
 player_t *player_ptr, 
 bool	admin_flag
 )
{
	if (player_ptr)
	{
		SayToPlayer(GREEN_CHAT, player_ptr, "Check console for output");
	}

	for (int i = 0; i < cmd_list_size; i ++)
	{
		// Don't show admin commands to regular players
		if (cmd_list[i].admin_required && !admin_flag) continue;
		if (!player_ptr && !cmd_list[i].server_command) continue;
		OutputToConsole(player_ptr, "%s\n", cmd_list[i].cmd_name);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Show all the help commands
//---------------------------------------------------------------------------------
void	ManiCommands::SearchCommands
(
 player_t *player_ptr, 
 bool	admin_flag,
 const char *pattern,
 const int command_type
 )
{
	int cmd_count = 0; 
	int cmd_index = -1;

	for (int i = 0; i < cmd_list_size; i ++)
	{
		// Don't show admin commands to regular players
		if (cmd_list[i].admin_required && !admin_flag) continue;
		if (!player_ptr && !cmd_list[i].server_command) continue;
		if (war_mode && !cmd_list[i].war_mode_allowed) continue;

		if (Q_stristr(cmd_list[i].cmd_name, pattern) != NULL)
		{
			if (FStrEq(cmd_list[i].cmd_name, pattern))
			{
				cmd_index = i;
				cmd_count = 1;
				break;
			}

			cmd_index = i;
			cmd_count ++;
		}
	}

	if (cmd_count == 0)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "No match found");
		return;
	}

	if (player_ptr && cmd_count > 1)
	{
		SayToPlayer(GREEN_CHAT, player_ptr, "Check console for output");
	}

	if (cmd_count > 20)
	{
		for (int i = 0; i < cmd_list_size; i ++)
		{
			// Don't show admin commands to regular players
			if (cmd_list[i].admin_required && !admin_flag) continue;
			if (!player_ptr && !cmd_list[i].server_command) continue;
			if (war_mode && !cmd_list[i].war_mode_allowed) continue;

			if (Q_stristr(cmd_list[i].cmd_name, pattern) != NULL)
			{
				OutputToConsole(player_ptr, "%s\n", cmd_list[i].cmd_name);
			}
		}
	}
	else if (cmd_count > 1)
	{
		for (int i = 0; i < cmd_list_size; i ++)
		{
			// Don't show admin commands to regular players
			if (cmd_list[i].admin_required && !admin_flag) continue;
			if (!player_ptr && !cmd_list[i].server_command) continue;
			if (war_mode && !cmd_list[i].war_mode_allowed) continue;

			if (Q_stristr(cmd_list[i].cmd_name, pattern) != NULL)
			{
				OutputToConsole(player_ptr, "%s : %s\n", cmd_list[i].cmd_name, Translate(player_ptr, cmd_list[i].help_id));
			}
		}
	}
	else
	{
		DumpHelp(player_ptr, cmd_index, command_type);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Output Help in various formats for say/console/server console format
//---------------------------------------------------------------------------------
void	ManiCommands::DumpHelp
(
 player_t *player_ptr,
 int	cmd_index,
 const int command_type
 )
{
	int help_id = cmd_list[cmd_index].help_id;
	bool server_console = cmd_list[cmd_index].server_command;
	bool client_console = cmd_list[cmd_index].client_command;
	bool client_chat = cmd_list[cmd_index].say_command | cmd_list[cmd_index].tsay_command;
	bool war_allowed = cmd_list[cmd_index].war_mode_allowed;

	// Show command name
	OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 2000, "%s", cmd_list[cmd_index].cmd_name));
	// Show types of operation
	OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 2001, "%s%s%s%s", 
		(server_console == true) ? Translate(player_ptr, M_MENU_YES):Translate(player_ptr, M_MENU_NO),
		(client_console == true) ? Translate(player_ptr, M_MENU_YES):Translate(player_ptr, M_MENU_NO),
		(client_chat == true) ? Translate(player_ptr, M_MENU_YES):Translate(player_ptr, M_MENU_NO),
		(war_allowed == true) ? Translate(player_ptr, M_MENU_YES):Translate(player_ptr, M_MENU_NO)));
	// Show Parameters
	OutputHelpText(GREEN_CHAT, player_ptr, "%s %s", Translate(player_ptr, 2002), Translate(player_ptr, help_id));
	// Show Description

	if (command_type == M_SAY || command_type == M_TSAY)
	{
		char help_text[2048];

		int length = snprintf(help_text, sizeof(help_text), "%s %s", Translate(player_ptr, 2003), Translate(player_ptr, help_id + 1));
		if (length > 254)
		{
			int start_index = 0;
			int end_index = 254;

			for(;;)
			{
				bool found_line;

				found_line = false;
				for (int i = end_index; i > start_index; i--)
				{
					if (help_text[i] == 0x0a)
					{
						// Found line break
						found_line = true;
						help_text[i] = '\0';
						OutputHelpText(GREEN_CHAT, player_ptr, "%s", &(help_text[start_index]));
						start_index = i + 1;
						end_index = start_index + 254;

						if (end_index > length)
						{
							OutputHelpText(GREEN_CHAT, player_ptr, "%s", &(help_text[start_index]));
							return;
						}

						break;
					}
				}

				if (!found_line)
				{
					break;
				}
			}
		}
		else
		{
			OutputHelpText(GREEN_CHAT, player_ptr, "%s", help_text);
		}
	}
	else if (command_type == M_CCONSOLE)
	{
		char help_text[2048];

		int length = snprintf(help_text, sizeof(help_text), "%s %s", Translate(player_ptr, 2003), Translate(player_ptr, help_id + 1));
		if (length > 700)
		{
			int start_index = 0;
			int end_index = 700;

			for(;;)
			{
				bool found_line;

				found_line = false;
				for (int i = end_index; i > start_index; i--)
				{
					if (help_text[i] == 0x0a)
					{
						// Found line break
						found_line = true;
						help_text[i] = '\0';
						OutputToConsole(player_ptr, "%s\n", &(help_text[start_index]));
						start_index = i + 1;
						end_index = start_index + 700;

						if (end_index > length)
						{
							OutputToConsole(player_ptr, "%s\n", &(help_text[start_index]));
							return;
						}

						break;
					}
				}

				if (!found_line)
				{
					break;
				}
			}
		}
		else
		{
			OutputToConsole(player_ptr, "%s\n", help_text);
		}
	}
	else
	{
		OutputToConsole(player_ptr, "%s %s\n", Translate(player_ptr, 2003), Translate(player_ptr, help_id + 1));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Process the the say command strings
//---------------------------------------------------------------------------------
void	ManiCommands::ExtractSayCommand(bool team_say, const CCommand &args)
{
	char	one[8];
	char	two[8];
	char	three[8];

	this->ParseSayCommand(args);

	strcpy(say_argv0, "");

	if (cmd_argc != 0)
	{
		char *argv0 = cmd_argv[0];

		strcpy(one, mani_say_command_prefix.GetString());
		strcpy(two, one);
		strcat(two, mani_say_command_prefix.GetString());
		strcpy(three, two);
		strcat(three, mani_say_command_prefix.GetString());

		if (argv0[0] == one[0])
		{
			// This is the start of a command as we have the @ symbol
			bool copied_command = false;

			if (mani_use_ma_in_say_command.GetInt() == 0)
				{
				// The prefix ma_ is not required so we should add it if
				// it does not exist after the @ command

				if (Q_strlen(cmd_argv[0]) > 3)
				{
					// If we don't have ma_ in the command
					if (toupper(argv0[1] != 'M') &&
						toupper(argv0[2] != 'A') &&
						argv0[3] != '_')
					{
    					// Add it in
						Q_strcpy(say_argv0, "ma_");
						strcat(say_argv0, &(argv0[1]));
						copied_command = true;
					}
				}
			}

			if (!copied_command)
			{
				Q_strcpy(say_argv0, &(argv0[1]));
			}

			if (FStrEq(argv0, one))
			{
				if (!team_say) 
				{
					strcpy(say_argv0, "ma_say");
				}
				else 
				{
					strcpy(say_argv0, "ma_chat");
				}
			}
			else if (FStrEq(argv0, two)) 
			{
				strcpy(say_argv0, "ma_psay");
			}
			else if (FStrEq(argv0, three)) 
			{
				strcpy(say_argv0, "ma_csay");
			}
		}
	}

	this->DumpCommands();
}

//---------------------------------------------------------------------------------
// Purpose: Dump out the contents of the new cmd details
//---------------------------------------------------------------------------------
void	ManiCommands::DumpCommands(void)
{
	return;
	Msg("Argc %i\nArgs [%s]\nArgv-> ", gpCmd->Cmd_Argc(), gpCmd->Cmd_Args());

	for (int i = 0; i < gpCmd->Cmd_Argc(); i++)
	{
		Msg("%i [%s] ", i, gpCmd->Cmd_Argv(i));
	}

	Msg("\n");


	Msg("Args(x)\n");
	for (int i = 0; i < gpCmd->Cmd_Argc(); i++)
	{
		Msg("%i [%s]\n", i, gpCmd->Cmd_Args(i));
	}

	Msg("say_argv0 [%s]\n", say_argv0);
}

//---------------------------------------------------------------------------------
// Purpose: Process the the say command strings
//---------------------------------------------------------------------------------
void	ManiCommands::ParseSayCommand(const CCommand &args)
{
	static int i,j;
	static char terminate_char;
	static bool found_quotes;
	static int say_length;
	static const char *say_string;

	cmd_argc = 0;

	for (i = 0; i < 80; i++)
	{
		// Reset strings for safety
		cmd_argv[i] = cmd_null_string;
		cmd_argv_cat[i] = cmd_null_string;
	}

	if (!args.ArgS()) return;

	say_string = args.ArgS();
	say_length = Q_strlen(say_string);
	if (say_length == 0)
	{
		return;
	}

	if (say_length == 1)
	{
		// Only one character in string
		Q_strcpy(temp_string1, say_string);
		cmd_argc = 1;
		cmd_argv[0] = &(temp_string1[0]);
		cmd_argv_cat[0] = cmd_argv[0];
		cmd_args = cmd_argv[0];
		return;
	}

	// Check if quotes are needed to be removed
	if (say_string[0] == '\"' && say_string[say_length - 1] == '\"')
	{
		strncpy(temp_string1, &(say_string[1]), say_length - 2);
		temp_string1[say_length - 2] = '\0';
	}
	else
	{
		strcpy(temp_string1, say_string);
	}

	cmd_args = &(temp_string1[0]);

	// Extract tokens
	i = 0;
	j = 0;

	while (cmd_argc != 80)
	{
		// Find first non white space
		while (temp_string1[i] == ' ' && temp_string1[i] != '\0') i++;
		if (temp_string1[i] == '\0') return;

		cmd_argv_cat[cmd_argc] = &(temp_string1[i]);
		cmd_argv[cmd_argc] = &(temp_string2[j]);

		if (temp_string1[i] == '\"')
		{
			// This is a command in quotes
			i++;

			while (temp_string1[i] != '\"' && temp_string1[i] != '\0')
			{
				// Copy char
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0') return;

			// Skip end quote char
			i++;
		}
		else
		{
			// Command is not in quotes
			while (temp_string1[i] != ' ' && temp_string1[i] != '\0')
			{
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0') return;

			// Skip white space
			i++;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the the event say commands
//---------------------------------------------------------------------------------
void	ManiCommands::ParseEventSayCommand(const char *player_say)
{
	static int i,j;
	static char terminate_char;
	static bool found_quotes;
	static int say_length;
	static const char *say_string;

	cmd_argc = 0;

	for (i = 0; i < 80; i++)
	{
		// Reset strings for safety
		cmd_argv[i] = cmd_null_string;
		cmd_argv_cat[i] = cmd_null_string;
	}

//	if (!engine->Cmd_Args()) return;

	say_string = player_say;
	say_length = Q_strlen(say_string);
	if (say_length == 0)
	{
		return;
	}

	if (say_length == 1)
	{
		// Only one character in string
		Q_strcpy(temp_string1, say_string);
		cmd_argc = 1;
		cmd_argv[0] = &(temp_string1[0]);
		cmd_argv_cat[0] = cmd_argv[0];
		cmd_args = cmd_argv[0];
		return;
	}

	strcpy(temp_string1, say_string);
	cmd_args = &(temp_string1[0]);

	// Extract tokens
	i = 0;
	j = 0;

	while (cmd_argc != 80)
	{
		// Find first non white space
		while (temp_string1[i] == ' ' && temp_string1[i] != '\0') i++;
		if (temp_string1[i] == '\0') return;

		cmd_argv_cat[cmd_argc] = &(temp_string1[i]);
		cmd_argv[cmd_argc] = &(temp_string2[j]);

		if (temp_string1[i] == '\"')
		{
			// This is a command in quotes
			i++;

			while (temp_string1[i] != '\"' && temp_string1[i] != '\0')
			{
				// Copy char
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0') return;

			// Skip end quote char
			i++;
		}
		else
		{
			// Command is not in quotes
			while (temp_string1[i] != ' ' && temp_string1[i] != '\0')
			{
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0') return;

			// Skip white space
			i++;
		}
	}

	return;
}
//---------------------------------------------------------------------------------
// Purpose: Process the the Client and Server command strings
//---------------------------------------------------------------------------------
void	ManiCommands::ExtractClientAndServerCommand(const CCommand &args)
{
	static int i,j;
	static char terminate_char;
	static bool found_quotes;
	static int say_length;
	static const char *say_string;

	cmd_argc = 1;

	for (i = 0; i < 80; i++)
	{
		// Reset strings for safety
		cmd_argv[i] = cmd_null_string;
		cmd_argv_cat[i] = cmd_null_string;
	}
	
	Q_strcpy(say_argv0, "");

	int initial_len = snprintf(temp_string1, sizeof(temp_string1), "%s ", args.Arg(0));
	Q_strcpy(temp_string2, args.Arg(0));

	say_string = args.ArgS();
	if (!say_string)
	{
		Q_strcpy(temp_string1, args.Arg(0));
		cmd_args = &(temp_string1[0]);
		cmd_argv_cat[0] = &(temp_string1[0]);
		cmd_argv[0] = &(temp_string2[0]);
		this->DumpCommands();
		return;
	}

	say_length = Q_strlen(say_string);

	// Check if quotes are needed to be removed
/*	if (say_string[0] == '\"' && say_string[say_length - 1] == '\"')
	{
		for (int x = 0; x < say_length - 2; x++)
		{
			temp_string1[initial_len + x] = say_string[x + 1];
		}

		temp_string1[initial_len + (say_length - 2)] = '\0';
	}
	else
	{*/
		strcat(temp_string1, say_string);
//	}

	cmd_args = &(temp_string1[0]);
	cmd_argv_cat[0] = &(temp_string1[0]);
	cmd_argv[0] = &(temp_string2[0]);

	// Extract tokens
	i = initial_len;
	j = strlen(temp_string2) + 1;

	while (cmd_argc != 80)
	{
		// Find first non white space
		while (temp_string1[i] == ' ' && temp_string1[i] != '\0') i++;
		if (temp_string1[i] == '\0')
		{
			this->DumpCommands();
			return;
		}

		cmd_argv_cat[cmd_argc] = &(temp_string1[i]);
		cmd_argv[cmd_argc] = &(temp_string2[j]);

		if (temp_string1[i] == '\"')
		{
			// This is a command in quotes
			i++;

			while (temp_string1[i] != '\"' && temp_string1[i] != '\0')
			{
				// Copy char
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0')
			{
				this->DumpCommands();
				return;
			}


			// Skip end quote char
			i++;
		}
		else
		{
			// Command is not in quotes
			while (temp_string1[i] != ' ' && temp_string1[i] != '\0')
			{
				temp_string2[j] = temp_string1[i];
				j++;
				i++;
			}

			cmd_argc++;
			temp_string2[j++] = '\0';
			if (temp_string1[i] == '\0')
			{
				this->DumpCommands();
				return;
			}

			// Skip white space
			i++;
		}
	}

	this->DumpCommands();
	return;
}


//---------------------------------------------------------------------------------
// Purpose: Register a command be it con command, client command or say command
//---------------------------------------------------------------------------------
void ManiCommands::RegisterCommand
(
 const char *name, 
 int help_id, 
 bool admin_required, 
 bool war_mode_allowed,
 bool server_command, 
 bool client_command, 
 bool say_command,
 bool tsay_command,
 int (ManiCommands::*funcPtr)(player_t *, const char *, int, int, bool)
)
{
	AddToList((void **) &cmd_list, sizeof(cmd_t), &cmd_list_size);
	int	index = cmd_list_size - 1;

	cmd_list[index].cmd_name = (char *) malloc((sizeof(char) * strlen(name)) + 1);
	strcpy(cmd_list[index].cmd_name, name);
	cmd_list[index].help_id = help_id;
	cmd_list[index].admin_required = admin_required;
	cmd_list[index].server_command = server_command;
	cmd_list[index].client_command = client_command;
	cmd_list[index].say_command = say_command;
	cmd_list[index].tsay_command = tsay_command;
	cmd_list[index].funcPtr = funcPtr;
	cmd_list[index].war_mode_allowed = war_mode_allowed;
	cmd_list[index].menu_command = false;
}

//---------------------------------------------------------------------------------
// Purpose: Register a command for menu use
//---------------------------------------------------------------------------------
void ManiCommands::RegisterCommand
(
 const char *name, 
 bool war_mode_allowed,
 int (ManiCommands::*funcPtr)(player_t *, const char *, int, int, bool)
)
{
	AddToList((void **) &cmd_list, sizeof(cmd_t), &cmd_list_size);
	int	index = cmd_list_size - 1;

	cmd_list[index].cmd_name = (char *) malloc((sizeof(char) * strlen(name)) + 1);
	strcpy(cmd_list[index].cmd_name, name);
	cmd_list[index].help_id = 0;
	cmd_list[index].menu_command = true;
	cmd_list[index].server_command = false;
	cmd_list[index].client_command = false;
	cmd_list[index].say_command = false;
	cmd_list[index].tsay_command = false;
	cmd_list[index].funcPtr = funcPtr;
	cmd_list[index].war_mode_allowed = war_mode_allowed;
}
//---------------------------------------------------------------------------------
// Purpose: Handle say commands and client console commands
//---------------------------------------------------------------------------------
int	ManiCommands::HandleCommand
(
 player_t *player_ptr, 
 int command_type,
 const CCommand &args
 )
{
	cmd_t	cmd_key;
	cmd_t	*found_cmd;

	if (ProcessPluginPaused()) return PLUGIN_CONTINUE;

	if (command_type == M_SAY || command_type == M_TSAY)
	{
		// Commands are already extract in the HookSay function
		if (strcmp(say_argv0, "") == 0) return true;
		cmd_key.cmd_name = (char *) say_argv0; 
	}
	else
	{
		if (args.ArgC() == 0) return PLUGIN_CONTINUE;
		cmd_key.cmd_name = (char *) args.Arg(0);
	}

	// Do BSearch for function name
	found_cmd = (cmd_t *) bsearch
						(
						&cmd_key, 
						cmd_list, 
						cmd_list_size, 
						sizeof(cmd_t), 
						sort_by_cmd_name
						);

	if (found_cmd == NULL) 
	{
		// No function found
		return (int) PLUGIN_CONTINUE;
	}

	if (command_type == M_CCONSOLE)
	{
		if (found_cmd->client_command || found_cmd->menu_command)
		{
			// Client Console command
			return (g_Cmd.*found_cmd->funcPtr) (player_ptr, cmd_key.cmd_name, found_cmd->help_id, command_type, found_cmd->war_mode_allowed);
		}
		
		return PLUGIN_CONTINUE;
	}
	else if (command_type == M_SAY)
	{
		// Say command
		if (!found_cmd->say_command) return PLUGIN_CONTINUE;

		if (engine->IsDedicatedServer() && con_command_index == -1)
		{
			return PLUGIN_CONTINUE;
		}

		(g_Cmd.*found_cmd->funcPtr) (player_ptr, cmd_key.cmd_name, found_cmd->help_id, command_type, found_cmd->war_mode_allowed);
		return PLUGIN_STOP;
	}
	else if (command_type == M_TSAY)
	{
		// Team Say command
		if (!found_cmd->tsay_command) return PLUGIN_CONTINUE;

		if (engine->IsDedicatedServer() && con_command_index == -1)
		{
			return PLUGIN_CONTINUE;
		}

		(g_Cmd.*found_cmd->funcPtr) (player_ptr, cmd_key.cmd_name, found_cmd->help_id, command_type, found_cmd->war_mode_allowed);	
		return PLUGIN_STOP;
	}

	return 0;
}

int			ManiCommands::Cmd_Argc(void) 
{
	return cmd_argc;
}

const char *ManiCommands::Cmd_Argv(int i) 
{
	return cmd_argv[i];
}

const char *ManiCommands::Cmd_Args(void) 
{
	return cmd_argv_cat[1];
}

const char *ManiCommands::Cmd_Args(int i) 
{
	return cmd_argv_cat[i];
}

const char *ManiCommands::Cmd_SayArg0(void) 
{
	return say_argv0;
}
//---------------------------------------------------------------------------------
// Purpose: Initialise a new command
//---------------------------------------------------------------------------------
void	ManiCommands::NewCmd(void)
{
	cmd_argc = 0;

	for (int i = 0; i < 80; i++)
	{
		// Reset strings for safety
		cmd_argv[i] = cmd_null_string;
		cmd_argv_cat[i] = cmd_null_string;
	}

	Q_strcpy(temp_string1, "");
	Q_strcpy(temp_string2, "");
	Q_strcpy(say_argv0, "");
	cmd_args = cmd_null_string;
	current_marker1 = 0;
	current_marker2 = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Add a integer number as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::AddParam(int number)
{
	char	temp_buffer[32];
	int		length;
	length = snprintf(temp_buffer, sizeof(number), "%i", number);
	this->AddStringParam(temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a float number as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::AddParam(float number)
{
	char	temp_buffer[64];
	int		length;
	length = snprintf(temp_buffer, sizeof(number), "%f", number);
	this->AddStringParam(temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a variable string as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::AddParam(char *fmt, ...)
{
	va_list		argptr;
	int			length;
	char		temp_buffer[2048];
	
	va_start ( argptr, fmt );
	length = vsnprintf( temp_buffer, sizeof(temp_buffer), fmt, argptr );
	va_end   ( argptr );
	this->AddStringParam(temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a integer number as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::SetParam(int argno, int number)
{
	char	temp_buffer[32];
	int		length;
	length = snprintf(temp_buffer, sizeof(temp_buffer), "%i", number);
	this->SetStringParam(argno, temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a float number as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::SetParam(int argno, float number)
{
	char	temp_buffer[64];
	int		length;
	length = snprintf(temp_buffer, sizeof(temp_buffer), "%f", number);
	this->SetStringParam(argno, temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a variable string as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::SetParam(int argno, char *fmt, ...)
{
	va_list		argptr;
	int			length;
	char		temp_buffer[2048];
	
	va_start ( argptr, fmt );
	length = vsnprintf( temp_buffer, sizeof(temp_buffer), fmt, argptr );
	va_end   ( argptr );
	this->SetStringParam(argno, temp_buffer, length);
}

//---------------------------------------------------------------------------------
// Purpose: Add a string as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::AddStringParam(char *string, int length)
{
	// Nothing to add
	if (string[0] == '\0') return;

	if (cmd_argc == 0)
	{
		// First arguement to add
		Q_strcpy(temp_string1, string);
		Q_strcpy(temp_string2, string);
		cmd_argv[cmd_argc] = &(temp_string1[0]);
		cmd_argv_cat[cmd_argc] = &(temp_string2[0]);
		cmd_args = &(temp_string2[0]);
	}
	else
	{
		// Not the first argument
		Q_strcpy(&(temp_string1[current_marker1]), string);
		strcat(temp_string2, " ");
		strcat(temp_string2, string);
		cmd_argv[cmd_argc] = &(temp_string1[current_marker1]);
		cmd_argv_cat[cmd_argc] = &(temp_string2[current_marker2]);
	}

	current_marker1 += length + 1;
	current_marker2 += length + 1;
	cmd_argc ++;
}

//---------------------------------------------------------------------------------
// Purpose: Add a string as a parameter
//---------------------------------------------------------------------------------
void	ManiCommands::SetStringParam(int argno, char *string, int length)
{
	// Nothing to add
	if (string[0] == '\0') return;
	int no_of_cmds = cmd_argc;
	if (no_of_cmds == 0) return;

	memcpy(this->back_up_string, temp_string2, sizeof(temp_string2));
	this->NewCmd();

	const char *prefix = mani_say_command_prefix.GetString();
	if (prefix && string[0] == prefix[0])
	{
		Q_strcpy(this->say_argv0, &(string[1]));
	}
	else
	{
		Q_strcpy(this->say_argv0, string);
	}

	char *str = this->back_up_string;

	for (int i = 0; i < no_of_cmds; i ++)
	{
		if (i == argno)
		{
			gpCmd->AddParam("%s", string);
			while(*str++ != 0);
		}
		else
		{
			gpCmd->AddParam("%s", str);
			while(*str++ != 0);
		}
	}
}

int		ManiCommands::BadAdmin(int result, player_t *player_ptr, const char *name)
{
	// RCON command
	if (!player_ptr)
	{
		if (result == PLUGIN_BAD_ADMIN)
		{
			return PLUGIN_STOP;
		}

		return result;
	}

	if (result == PLUGIN_BAD_ADMIN)
	{
		OutputHelpText(GREEN_CHAT, player_ptr, "%s", Translate(player_ptr, 2580, "%s", name));
		return PLUGIN_STOP;
	}

	return result;
}

void	ManiCommands::WriteHelpHTML(void)
{
	char	filename[512];

	FILE *filehandle = NULL;
	ManiFile *mf = new ManiFile();

	snprintf(filename, sizeof(filename), "./cfg/%s/data/command_help.html", mani_path.GetString());
	filehandle = mf->Open(filename, "wt");
	if (filehandle == NULL)
	{
		delete mf;
		return;
	}

	const char *header_row_bg_colour = "#000080";
	const char *header_row_font_colour = "#ffffff";
	const char *cmd_col_bg_colour = "#4d4d4d";
	const char *cmd_col_font_colour = "#ffffff";
	const char *rest_bg_colour = "#cccccc";
	const char *rest_font_colour = "#000000";
	const char *border_colour = "#000000";

	fprintf(filehandle, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
	fprintf(filehandle, "<HTML>\n");
	fprintf(filehandle, "<HEAD>\n");
	fprintf(filehandle, "\t<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=utf-8\">\n");
	fprintf(filehandle, "\t<TITLE></TITLE>\n");
	fprintf(filehandle, "\t<META NAME=\"GENERATOR\" CONTENT=\"%s\">\n", PLUGIN_VERSION);
	fprintf(filehandle, "\t<META NAME=\"AUTHOR\" CONTENT=\"Mani\">\n");
	fprintf(filehandle, "</HEAD>\n");
	fprintf(filehandle, "<BODY LANG=\"en-GB\" DIR=\"LTR\">\n");
	fprintf(filehandle, "<P ALIGN=CENTER STYLE=\"page-break-before: always\"><FONT SIZE=4 STYLE=\"font-size: 16pt\"><B>Mani Admin Plugin Commands %s</B></FONT></P>\n", PLUGIN_VERSION_ID2);
	fprintf(filehandle, "<TABLE WIDTH=100%% BORDER=1 BORDERCOLOR=\"%s\" CELLPADDING=4 CELLSPACING=0>\n", border_colour);
	fprintf(filehandle, "\t<COL WIDTH=43*>\n");
	fprintf(filehandle, "\t<COL WIDTH=61*>\n");
	fprintf(filehandle, "\t<COL WIDTH=60*>\n");
	fprintf(filehandle, "\t<COL WIDTH=19*>\n");
	fprintf(filehandle, "\t<COL WIDTH=19*>\n");
	fprintf(filehandle, "\t<COL WIDTH=18*>\n");
	fprintf(filehandle, "\t<COL WIDTH=18*>\n");
	fprintf(filehandle, "\t<COL WIDTH=18*>\n");

	fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=17%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3065)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=24%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3066)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t\t<TH WIDTH=23%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3067)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=8%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3068)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=7%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3069)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=7%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3070)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=7%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3071)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=7%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML(Translate(NULL, 3072)));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t</TR>\n");

	for (int i = 0; i < cmd_list_size; i ++)
	{
		fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=17%% BGCOLOR=\"%s\">\n", cmd_col_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", cmd_col_font_colour, cmd_list[i].cmd_name);
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=24%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			((cmd_list[i].help_id != 0) ? AsciiToHTML(Translate(NULL, cmd_list[i].help_id + 1)):Translate(NULL, 3073)));
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=23%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n",
			rest_font_colour,
			((cmd_list[i].help_id != 0) ? AsciiToHTML(Translate(NULL, cmd_list[i].help_id)):"-"));
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=8%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			(cmd_list[i].say_command) ? "X":".");
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=7%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			(cmd_list[i].tsay_command) ? "X":".");
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=7%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			(cmd_list[i].client_command) ? "X":".");
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=7%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			(cmd_list[i].server_command) ? "X":".");
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t\t<TD WIDTH=7%% BGCOLOR=\"%s\">\n", rest_bg_colour);
		fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
		fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
			rest_font_colour,
			(cmd_list[i].war_mode_allowed) ? "X":".");
		fprintf(filehandle, "\t\t</TD>\n");
		fprintf(filehandle, "\t</TR>\n");
	}

	fprintf(filehandle, "</TABLE>\n<P><BR><BR>\n</P>\n</BODY>\n</HTML>");
	mf->Close(filehandle);
	delete mf;

	// Write HTML version of convar help

	filehandle = NULL;
	mf = new ManiFile();

	snprintf(filename, sizeof(filename), "./cfg/%s/data/cvar_help.html", mani_path.GetString());
	filehandle = mf->Open(filename, "wt");
	if (filehandle == NULL)
	{
		delete mf;
		return;
	}


/*	const char *header_row_bg_colour = "#000080";
	const char *header_row_font_colour = "#ffffff";
	const char *cmd_col_bg_colour = "#4d4d4d";
	const char *cmd_col_font_colour = "#ffffff";
	const char *rest_bg_colour = "#cccccc";
	const char *rest_font_colour = "#000000";
	const char *border_colour = "#000000";*/

	fprintf(filehandle, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
	fprintf(filehandle, "<HTML>\n");
	fprintf(filehandle, "<HEAD>\n");
	fprintf(filehandle, "\t<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=utf-8\">\n");
	fprintf(filehandle, "\t<TITLE></TITLE>\n");
	fprintf(filehandle, "\t<META NAME=\"GENERATOR\" CONTENT=\"%s\">\n", PLUGIN_VERSION);
	fprintf(filehandle, "\t<META NAME=\"AUTHOR\" CONTENT=\"Mani\">\n");
	fprintf(filehandle, "</HEAD>\n");
	fprintf(filehandle, "<BODY LANG=\"en-GB\" DIR=\"LTR\">\n");
	fprintf(filehandle, "<P ALIGN=CENTER STYLE=\"page-break-before: always\"><FONT SIZE=4 STYLE=\"font-size: 16pt\"><B>Mani Admin Plugin CVar List %s</B></FONT></P>\n", PLUGIN_VERSION_ID2);
	fprintf(filehandle, "<TABLE WIDTH=100%% BORDER=1 BORDERCOLOR=\"%s\" CELLPADDING=4 CELLSPACING=0>\n", border_colour);
	fprintf(filehandle, "\t<COL WIDTH=43*>\n");
	fprintf(filehandle, "\t<COL WIDTH=61*>\n");
	fprintf(filehandle, "\t<COL WIDTH=60*>\n");

	fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=17%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML("ConVar name"));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t<TH WIDTH=24%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML("Description"));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t\t\t<TH WIDTH=23%% BGCOLOR=\"%s\">\n", header_row_bg_colour);
	fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
	fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", header_row_font_colour, AsciiToHTML("Default Value"));
	fprintf(filehandle, "\t\t</TH>\n");
	fprintf(filehandle, "\t</TR>\n");

	ConCommandBase *pPtr = g_pCVar->GetCommands();
	while (pPtr)
	{
		if (!pPtr->IsCommand())
		{
			const char *name = pPtr->GetName();

			if (NULL != Q_stristr(name, "mani_"))
			{
				// Found mani cvar
				ConVar *mani_var = g_pCVar->FindVar(name);
				fprintf(filehandle, "\t<TR VALIGN=TOP>\n");
				fprintf(filehandle, "\t\t<TD WIDTH=17%% BGCOLOR=\"%s\">\n", cmd_col_bg_colour);
				fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
				fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", cmd_col_font_colour, name);
				fprintf(filehandle, "\t\t</TD>\n");

				fprintf(filehandle, "\t\t<TD WIDTH=24%% BGCOLOR=\"%s\">\n", rest_bg_colour);
				fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
				fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n", 
					rest_font_colour, AsciiToHTML((char *) mani_var->m_pszHelpString));
				fprintf(filehandle, "\t\t</TD>\n");

				fprintf(filehandle, "\t\t<TD WIDTH=23%% BGCOLOR=\"%s\">\n", rest_bg_colour);
				fprintf(filehandle, "\t\t\t<P ALIGN=LEFT STYLE=\"font-style: normal; font-weight: medium; text-decoration: none\">\n");
				fprintf(filehandle, "\t\t\t<FONT COLOR=\"%s\"><FONT FACE=\"Times New Roman, serif\"><FONT SIZE=2>%s</FONT></FONT></FONT></P>\n",
					rest_font_colour, mani_var->GetDefault());
				fprintf(filehandle, "\t\t</TD>\n");
				fprintf(filehandle, "\t</TR>\n");
			}
		}

		pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
	}


	fprintf(filehandle, "</TABLE>\n<P><BR><BR>\n</P>\n</BODY>\n</HTML>");
	mf->Close(filehandle);
	delete mf;
}


// Non-class called version
#define CON_BODY(_func) \
	int ManiCommands::_func(player_t *player_ptr, const char *name, const int help_id, int command_type, bool war_mode_allowed) \
	{ \
	if (war_mode && !war_mode_allowed) return PLUGIN_CONTINUE; \
	return g_Cmd.BadAdmin((int) Process##_func (player_ptr, name, help_id, command_type), player_ptr, name); \
	}

// Class called version
#define CON_CBODY(_class, _func) \
	int ManiCommands::_func(player_t *player_ptr, const char *name, const int help_id, int command_type, bool war_mode_allowed) \
	{ \
		if (war_mode && !war_mode_allowed) return PLUGIN_CONTINUE; \
		return g_Cmd.BadAdmin(_class->Process##_func (player_ptr, name, help_id, command_type), player_ptr, name); \
	}


	CON_CBODY(gpManiMessageMode, MaSay)
	CON_CBODY(gpManiMessageMode, MaMSay)
	CON_CBODY(gpManiMessageMode, MaPSay)
	CON_CBODY(gpManiMessageMode, MaPMess)
	CON_CBODY(gpManiMessageMode, MaExit)
	CON_CBODY(gpManiMessageMode, MaChat)
	CON_CBODY(gpManiMessageMode, MaCSay)
	CON_CBODY(gpManiStats, MaSession)
	CON_CBODY(gpManiStats, MaStatsMe)
	CON_CBODY(gpManiAdminPlugin, MaRCon)
	CON_CBODY(gpManiAdminPlugin, MaBrowse)
	CON_CBODY(gpManiAdminPlugin, MaCExec)
	CON_CBODY(gpManiAdminPlugin, MaCExecT)
	CON_CBODY(gpManiAdminPlugin, MaCExecCT)
	CON_CBODY(gpManiAdminPlugin, MaCExecAll)
	CON_CBODY(gpManiAdminPlugin, MaCExecSpec)
	CON_CBODY(gpManiAdminPlugin, MaSlap)
	CON_CBODY(gpManiClient, MaSetFlag)
	CON_BODY(MaSetSkin)
	CON_CBODY(gpManiAdminPlugin, MaSetCash)
	CON_CBODY(gpManiAdminPlugin, MaGiveCash)
	CON_CBODY(gpManiAdminPlugin, MaGiveCashP)
	CON_CBODY(gpManiAdminPlugin, MaTakeCash)
	CON_CBODY(gpManiAdminPlugin, MaTakeCashP)
	CON_CBODY(gpManiAdminPlugin, MaSetHealth)
	CON_CBODY(gpManiAdminPlugin, MaGiveHealth)
	CON_CBODY(gpManiAdminPlugin, MaGiveHealthP)
	CON_CBODY(gpManiAdminPlugin, MaTakeHealth)
	CON_CBODY(gpManiAdminPlugin, MaTakeHealthP)
	CON_CBODY(gpManiAdminPlugin, MaBlind)
	CON_CBODY(gpManiAdminPlugin, MaFreeze)
	CON_CBODY(gpManiAdminPlugin, MaNoClip)
	CON_CBODY(gpManiAdminPlugin, MaBurn)
	CON_CBODY(gpManiAdminPlugin, MaGravity)
	CON_CBODY(gpManiAdminPlugin, MaColour)
	CON_CBODY(gpManiAdminPlugin, MaColourWeapon)
	CON_CBODY(gpManiAdminPlugin, MaRenderMode)
	CON_CBODY(gpManiAdminPlugin, MaRenderFX)
	CON_CBODY(gpManiAdminPlugin, MaGive)
	CON_CBODY(gpManiAdminPlugin, MaGiveAmmo)
	CON_CBODY(gpManiAdminPlugin, MaDrug)
	CON_CBODY(gpManiAdminPlugin, MaDecal)
	CON_CBODY(gpManiAdminPlugin, MaTimeBomb)
	CON_CBODY(gpManiAdminPlugin, MaBeacon)
	CON_CBODY(gpManiAdminPlugin, MaFireBomb)
	CON_CBODY(gpManiAdminPlugin, MaFreezeBomb)
	CON_CBODY(gpManiAdminPlugin, MaMute)
	CON_CBODY(gpManiAdminPlugin, MaTeleport)
	CON_CBODY(gpManiAdminPlugin, MaPosition)
	CON_CBODY(gpManiTeam, MaSwapTeam)
	CON_CBODY(gpManiTeam, MaSwapTeamD)
	CON_CBODY(gpManiTeam, MaSpec)
	CON_CBODY(gpManiTeam, MaBalance)
	CON_CBODY(gpManiAdminPlugin, MaDropC4)
	CON_CBODY(gpManiAdminPlugin, MaSaveLoc)
	CON_CBODY(gpManiStats, MaResetRank)
	CON_CBODY(gpManiStats, MaRanks)
	CON_CBODY(gpManiStats, MaPLRanks)
	CON_BODY(MaMap)
	CON_BODY(MaSkipMap)
	CON_BODY(MaNextMap)
	CON_BODY(MaListMaps)
	CON_BODY(MaMapList)
	CON_BODY(MaMapHistory)
	CON_BODY(MaMapCycle)
	CON_BODY(MaVoteMapList)

	CON_CBODY(gpManiAdminPlugin, MaWar)
	CON_BODY(MaSetNextMap)
	CON_CBODY(gpManiVote, MaVoteRandom)
	CON_CBODY(gpManiVote, MaVoteExtend)
	CON_CBODY(gpManiVote, MaVoteRCon)
	CON_CBODY(gpManiVote, MaVote)
	CON_CBODY(gpManiVote, MaVoteQuestion)
	CON_CBODY(gpManiVote, MaVoteCancel)
	CON_BODY(MaPlaySound)
	CON_BODY(MaFavourites)
	CON_CBODY(gpManiWeaponMgr, MaShowRestrict)
	CON_CBODY(gpManiWeaponMgr, MaRestrict)
	CON_CBODY(gpManiWeaponMgr, MaKnives)
	CON_CBODY(gpManiWeaponMgr, MaPistols)
	CON_CBODY(gpManiWeaponMgr, MaShotguns)
	CON_CBODY(gpManiWeaponMgr, MaNoSnipers)
	CON_CBODY(gpManiWeaponMgr, MaUnRestrict)
	CON_CBODY(gpManiWeaponMgr, MaUnRestrictAll)
	CON_CBODY(gpManiWeaponMgr, MaRestrictAll)
	CON_CBODY(gpManiWeaponMgr, MaRestrictRatio)
	CON_BODY(MaTKList)
	CON_CBODY(gpManiAdminPlugin, MaKick)
	CON_CBODY(gpManiChatTriggers, MaChatTriggers)
	CON_CBODY(gpManiSprayRemove, MaSpray)
	CON_CBODY(gpManiAdminPlugin, MaSlay)
	CON_CBODY(gpManiAdminPlugin, MaOffset)
	CON_CBODY(gpManiAdminPlugin, MaOffsetScan)
	CON_CBODY(gpManiAdminPlugin, MaOffsetScanF)
	CON_CBODY(gpManiAdminPlugin, MaTeamIndex)
	CON_CBODY(gpManiClient, MaClient)
	CON_CBODY(gpManiClient, MaClientGroup)
	CON_CBODY(gpManiClient, MaReloadClients)
	CON_CBODY(gpManiAdminPlugin, MaBan)
	CON_CBODY(gpManiAdminPlugin, MaBanIP)
	CON_CBODY(gpManiAdminPlugin, MaUnBan)

	CON_CBODY(gpManiAdminPlugin, MaUsers)
	CON_CBODY(gpManiAdminPlugin, MaAdmins)
	CON_CBODY(gpManiAdminPlugin, MaRates)
	CON_BODY(MaShowSounds)
	CON_CBODY(gpManiAdminPlugin, MaConfig)
	CON_CBODY(gpManiAdminPlugin, MaTimeLeft)

	CON_CBODY(gpManiAutoKickBan, MaAutoBanName)
	CON_CBODY(gpManiAutoKickBan, MaAutoBanPName)
	CON_CBODY(gpManiAutoKickBan, MaAutoKickName)
	CON_CBODY(gpManiAutoKickBan, MaAutoKickPName)
	CON_CBODY(gpManiAutoKickBan, MaAutoKickSteam)
	CON_CBODY(gpManiAutoKickBan, MaAutoKickIP)

	CON_CBODY(gpManiAutoKickBan, MaAutoShowName)
	CON_CBODY(gpManiAutoKickBan, MaAutoShowPName)
	CON_CBODY(gpManiAutoKickBan, MaAutoShowSteam)
	CON_CBODY(gpManiAutoKickBan, MaAutoShowIP)

	CON_CBODY(gpManiAutoKickBan, MaUnAutoName)
	CON_CBODY(gpManiAutoKickBan, MaUnAutoPName)
	CON_CBODY(gpManiAutoKickBan, MaUnAutoSteam)
	CON_CBODY(gpManiAutoKickBan, MaUnAutoIP)
	CON_CBODY(gpManiHelp, MaHelp)
	CON_CBODY(gpManiCustomEffects, MaEffect)

	CON_CBODY(gpManiObserverTrack, MaObserve)
	CON_CBODY(gpManiObserverTrack, MaEndObserve)

