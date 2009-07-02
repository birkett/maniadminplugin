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
#include "mani_callback_sourcemm.h"
#include "mani_sourcehook.h"
#include "mani_convar.h"
#include "mani_parser.h"
#include "mani_player.h"
#include "mani_language.h"
#include "mani_menu.h"
#include "mani_client.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_output.h"
#include "mani_gametype.h"
#include "mani_warmuptimer.h"
#include "mani_vote.h" 
#include "mani_sounds.h"
#include "mani_maps.h"
#include "mani_vfuncs.h"
#include "mani_help.h"
#include "mani_commands.h"
#include "shareddefs.h"

extern	IVEngineServer *engine;
extern	IVoiceServer *voiceserver;
extern	IPlayerInfoManager *playerinfomanager;
extern	IServerGameEnts *serverents;
extern	int	max_players;
extern	CGlobalVars *gpGlobals;
extern	bool war_mode;
extern	ConVar	*sv_lan;
extern  float timeleft_offset;
extern  char *mani_version;
extern	team_scores_t	team_scores;
extern	bool		trigger_changemap;
extern	float		trigger_changemap_time;

extern ConVar	*mp_winlimit; 
extern ConVar	*mp_maxrounds; 
extern ConVar	*mp_timelimit; 
extern ConVar	*mp_fraglimit; 

static int sort_nominations_by_votes_cast ( const void *m1,  const void *m2);
static int sort_show_votes_cast ( const void *m1,  const void *m2);

inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

ManiVote::ManiVote()
{
	vote_option_list = NULL;
	vote_option_list_size = 0;
	user_vote_map_list = NULL;
	user_vote_map_list_size = 0;
	vote_rcon_list = NULL;
	vote_rcon_list_size = 0;
	vote_question_list = NULL;
	vote_question_list_size = 0;
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = 0;
		user_vote_list[i].kick_vote_timestamp = 0;
		user_vote_list[i].nominate_timestamp = 0;
		user_vote_list[i].map_vote_timestamp = 0;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;
	}

	gpManiVote = this;
}

ManiVote::~ManiVote()
{
	// Cleanup
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);
	FreeList((void **) &vote_rcon_list, &vote_rcon_list_size);
	FreeList((void **) &vote_question_list, &vote_question_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Plugin Loaded
//---------------------------------------------------------------------------------
void	ManiVote::Load(void)
{
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = 0;
		user_vote_list[i].kick_vote_timestamp = 0;
		user_vote_list[i].nominate_timestamp = 0;
		user_vote_list[i].map_vote_timestamp = 0;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;
	}

	system_vote.vote_in_progress = false;
	system_vote.map_decided = false;
	system_vote.start_rock_the_vote = false;
	system_vote.no_more_rock_the_vote = false;
	system_vote.number_of_extends = 0;
	map_start_time = gpGlobals->curtime;
	strcpy(show_hint_results, "");
}

//---------------------------------------------------------------------------------
// Purpose: Plugin un-loaded
//---------------------------------------------------------------------------------
void	ManiVote::Unload(void)
{
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);
	FreeList((void **) &vote_rcon_list, &vote_rcon_list_size);
	FreeList((void **) &vote_question_list, &vote_question_list_size);
	strcpy(show_hint_results, "");
}

//---------------------------------------------------------------------------------
// Purpose: Level Loaded
//---------------------------------------------------------------------------------
void	ManiVote::LevelInit(void)
{
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);
	FreeList((void **) &vote_rcon_list, &vote_rcon_list_size);
	FreeList((void **) &vote_question_list, &vote_question_list_size);

	// Init votes and menu system
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		user_vote_list[i].ban_id = -1;
		user_vote_list[i].kick_id = -1;
		user_vote_list[i].map_index = -1;
		user_vote_list[i].nominated_map = -1;
		user_vote_list[i].rock_the_vote = false;
		user_vote_list[i].ban_vote_timestamp = -99;
		user_vote_list[i].kick_vote_timestamp = -99;
		user_vote_list[i].nominate_timestamp = -99;
		user_vote_list[i].map_vote_timestamp = -99;
		user_vote_list[i].kick_votes = 0;
		user_vote_list[i].ban_votes = 0;
	}

	system_vote.vote_in_progress = false;
	system_vote.map_decided = false;
	system_vote.start_rock_the_vote = false;
	system_vote.no_more_rock_the_vote = false;
	system_vote.number_of_extends = 0;

	// Required for vote time restriction
	map_start_time = gpGlobals->curtime;
	strcpy(show_hint_results, "");
	this->LoadConfig();
}

//---------------------------------------------------------------------------------
// Purpose: Check Player on disconnect
//---------------------------------------------------------------------------------
void ManiVote::ClientDisconnect(player_t	*player_ptr)
{
	voter_list[player_ptr->index - 1].allowed_to_vote = false;

	// If vote starter drops out, just confirm the vote without them
	if (system_vote.vote_starter != -1 && system_vote.vote_in_progress)
	{
		if (player_ptr->index == system_vote.vote_starter)
		{
			system_vote.vote_confirmation = false;
		}
	}

	user_vote_list[player_ptr->index - 1].nominated_map = -1;
	user_vote_list[player_ptr->index - 1].nominate_timestamp = -99;
	user_vote_list[player_ptr->index - 1].rock_the_vote = false;

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_map.GetInt() == 1)
	{
		// De-vote map for player
		user_vote_list[player_ptr->index - 1].map_index = -1;
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());
		for (int i = 0; i <= user_vote_map_list_size; i++)
		{
			int votes_counted = 0;

			for (int j = 0; j < max_players; j++)
			{
				if (user_vote_list[j].map_index == i)
				{
					votes_counted ++;
				}
			}

			if (votes_counted >= votes_required)
			{
				ProcessUserVoteMapWin(i);
				SayToAll(GREEN_CHAT, true,"Player leaving server triggered vote completion");
				break;
			}
		}
	}

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_kick.GetInt() == 1)
	{
		// De-kick vote player

		if (user_vote_list[player_ptr->index - 1].kick_id != -1)
		{
			player_t target_player;

			target_player.user_id = user_vote_list[player_ptr->index - 1].kick_id;
			if (FindPlayerByUserID(&target_player))
			{
				if (!target_player.is_bot)
				{
					if (user_vote_list[target_player.index - 1].kick_votes > 0)
					{
						user_vote_list[target_player.index - 1].kick_votes --;
					}
				}
			}
		}

		user_vote_list[player_ptr->index - 1].kick_votes = 0;
		user_vote_list[player_ptr->index - 1].kick_id = -1;

		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].kick_id == player_ptr->user_id)
			{
				user_vote_list[i].kick_id = -1;
			}
		}
			
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());
		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].kick_votes >= votes_required)
			{
				player_t server_player;
				server_player.index = i + 1;
				if (!FindPlayerByIndex(&server_player)) continue;

				ProcessUserVoteKickWin(&server_player);
				SayToAll(GREEN_CHAT, true,"Player leaving server triggered vote kick");
				break;
			}
		}
	}

	if (!war_mode && mani_voting.GetInt() == 1 && !ProcessPluginPaused() &&
		mani_vote_allow_user_vote_ban.GetInt() == 1)
	{
		// De-ban vote player
		if (user_vote_list[player_ptr->index - 1].ban_id != -1)
		{
			player_t target_player;

			target_player.user_id = user_vote_list[player_ptr->index - 1].ban_id;
			if (FindPlayerByUserID(&target_player))
			{
				if (!target_player.is_bot)
				{
					if (user_vote_list[target_player.index - 1].ban_votes > 0)
					{
						user_vote_list[target_player.index - 1].ban_votes --;
					}
				}
			}
		}

		user_vote_list[player_ptr->index - 1].ban_votes = 0;
		user_vote_list[player_ptr->index - 1].ban_id = -1;

		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].ban_id == player_ptr->user_id)
			{
				user_vote_list[i].ban_id = -1;
			}
		}
			
		int votes_required = GetVotesRequiredForUserVote(true, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());
		for (int i = 0; i < max_players; i++)
		{
			if (user_vote_list[i].ban_votes >= votes_required)
			{
				player_t server_player;
				server_player.index = i + 1;
				if (!FindPlayerByIndex(&server_player)) continue;

				ProcessUserVoteBanWin(&server_player);
				SayToAll(GREEN_CHAT, true,"Player leaving server triggered vote ban");
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do timed afk if needed
//---------------------------------------------------------------------------------
void ManiVote::GameFrame(void)
{
	if (war_mode) return;

	if (system_vote.vote_in_progress)
	{
		if (!system_vote.waiting_decision)
		{
			// End of vote time ?
			if (gpGlobals->curtime > system_vote.end_vote_time)
			{
				ProcessVotes();
			}
			else
			{
				if (gpGlobals->curtime > show_hint_next)
				{
					show_hint_next = gpGlobals->curtime + 1.0;
					if (strcmp(show_hint_results, "") != 0) 
					{
						MRecipientFilter mrf;
						bool	found_player = false;

						mrf.MakeReliable();
						mrf.RemoveAllRecipients();

						// Show results to players who care about it
						for (int i = 0; i < max_players; i++)
						{
							player_t player;

							player.index = i + 1;
							if (!FindPlayerByIndex(&player)) continue;
							if (player.is_bot) continue;

							player_settings_t *player_settings = FindPlayerSettings(&player);
							if (player_settings)
							{
								if (player_settings->show_vote_results_progress == 1)
								{
									// Add to the viewing list
									mrf.AddPlayer(i + 1);
									found_player = true;
								}
							}
						}

						if (found_player)
						{
							// Show hint message at bottom of screen as it's less
							// pervasive.
							UTIL_SayHint(&mrf, this->GetCompleteVoteProgress());
						}
					}
				}
			}
		}
		else
		{
			// Waiting for admin acceptance of vote ?
			if (gpGlobals->curtime > system_vote.waiting_decision_time)
			{
				player_t player;
				
				player.entity = NULL;
				player.index = system_vote.vote_starter;
				FindPlayerByIndex(&player);
				ProcessVoteConfirmation(&player, true);
			}
		}
	}

	if (!system_vote.vote_in_progress && !system_vote.map_decided)
	{
		// We can run end of map votes
		if (mani_vote_allow_end_of_map_vote.GetInt() == 1)
		{
			if (mp_timelimit && mp_timelimit->GetInt() != 0)
			{
				// Check timeleft
				float time_left = (mp_timelimit->GetFloat() * 60) - (gpGlobals->curtime - timeleft_offset);
				if (time_left < mani_vote_time_before_end_of_map_vote.GetFloat() * 60)
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(551));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
					}

					StartSystemVote();
				}
			}

			if (mp_winlimit && mp_winlimit->GetInt() != 0)
			{
				// Check win limit threshold about to be broken
				int highest_score = 0;
				for (int i = 0; i < MANI_MAX_TEAMS; i++)
				{
					if (team_scores.team_score[i] > highest_score) highest_score = team_scores.team_score[i];
				}

				if ((mp_winlimit->GetInt() - highest_score) <= mani_vote_rounds_before_end_of_map_vote.GetInt())
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(551));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
					}

					StartSystemVote();
				}
			}

			if (mp_maxrounds && mp_maxrounds->GetInt() != 0)
			{
				// Check win limit threshold about to be broken
				int total_rounds = 0;
				for (int i = 0; i < MANI_MAX_TEAMS; i++)
				{
					total_rounds += team_scores.team_score[i];
				}

				if ((mp_maxrounds->GetInt() - total_rounds) <= mani_vote_rounds_before_end_of_map_vote.GetInt())
				{
					system_vote.delay_action = VOTE_END_OF_MAP_DELAY;
					system_vote.vote_type = VOTE_RANDOM_END_OF_MAP;
					system_vote.vote_starter = -1;
					system_vote.vote_confirmation = false;
					system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
					BuildRandomMapVote(mani_vote_max_maps_for_end_of_map_vote.GetInt());
					if (!IsYesNoVote())
					{
						Q_strcpy(system_vote.vote_title, Translate(551));
					}
					else
					{
						Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
					}

					StartSystemVote();
				}
			}
		}
	}

	if ( system_vote.start_rock_the_vote &&
		!system_vote.no_more_rock_the_vote && 
		!system_vote.vote_in_progress &&
		!system_vote.map_decided)
	{
		// Run rock the vote
		system_vote.start_rock_the_vote = false;
		ProcessStartRockTheVote();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Load config
//---------------------------------------------------------------------------------
void ManiVote::LoadConfig(void)
{
	FileHandle_t file_handle;
	char	rcon_command[512];
	char	alias_command[512];
	char	question[512];
	char	base_filename[256];

	//Get question vote list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/votequestionlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load votequestionlist.txt\n");
	}
	else
	{
//		MMsg("Vote Question List\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine3(rcon_command, alias_command, question,  true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &vote_question_list, sizeof(vote_question_t), &vote_question_list_size);
			Q_strcpy(vote_question_list[vote_question_list_size - 1].alias, alias_command);
			Q_strcpy(vote_question_list[vote_question_list_size - 1].question, question);
//			MMsg("Menu Alias[%s] Question [%s]\n", alias_command, question); 
		}

		filesystem->Close(file_handle);
	}

	//Get rcon vote list
	Q_snprintf(base_filename, sizeof (base_filename), "./cfg/%s/voterconlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Failed to load voterconlist.txt\n");
	}
	else
	{
//		MMsg("Vote RCON List\n");
		while (filesystem->ReadLine (rcon_command, sizeof(rcon_command), file_handle) != NULL)
		{
			if (!ParseAliasLine2(rcon_command, alias_command, question,  true, false))
			{
				// String is empty after parsing
				continue;
			}

			AddToList((void **) &vote_rcon_list, sizeof(vote_rcon_t), &vote_rcon_list_size);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].rcon_command, rcon_command);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].alias, alias_command);
			Q_strcpy(vote_rcon_list[vote_rcon_list_size - 1].question, question);
//			MMsg("Menu Alias[%s] Question [%s] Command[%s]\n", alias_command, question, rcon_command); 
		}

		filesystem->Close(file_handle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Do round end processing
//---------------------------------------------------------------------------------
void ManiVote::RoundEnd(void)
{

}

//---------------------------------------------------------------------------------
// Purpose: Game Commencing
//---------------------------------------------------------------------------------
void	ManiVote::GameCommencing(void)
{

}

//*******************************************************************************
//
// Voting engine
//
//*******************************************************************************
//*******************************************************************************
// Start the system vote by showing menu to players
//*******************************************************************************
void	ManiVote::StartSystemVote (void)
{
	player_t player;

	system_vote.votes_required = 0;
	system_vote.max_votes = 0;
	system_vote.votes_so_far = 0;

	// Search for players and give them voting options
	for (int i = 1; i <= max_players; i++)
	{
		player.index = i;
		voter_list[player.index - 1].allowed_to_vote = false;

		if (!FindPlayerByIndex(&player)) continue; // No Player
		if (player.is_bot) continue;  // Bots don't vote

		voter_list[player.index - 1].allowed_to_vote = true;
		voter_list[player.index - 1].voted = false;
		
		if (mani_vote_dont_show_if_alive.GetInt() == 1 && !player.is_dead)
		{
			// If option set, don't show vote to alive players but tell them a vote has started
			SayToPlayer(LIGHT_GREEN_CHAT, &player, "Vote has started, type 'vote' to make your choice");
		}
		else
		{
			// Force client to run menu command
			engine->ClientCommand(player.entity, "mani_votemenu\n");
		}

		ProcessPlayActionSound(&player, MANI_ACTION_SOUND_VOTESTART);
		system_vote.votes_required ++;
		system_vote.max_votes ++;
	}

	system_vote.waiting_decision = false;
	system_vote.vote_in_progress = true;
	system_vote.total_votes_needed = system_vote.votes_required;

	strcpy(show_hint_results, "");
	show_hint_next = gpGlobals->curtime + 1.0;

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
}


//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuSystemVoteRandomMap( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (gpManiVote->SysVoteInProgress()) return;

	if (argc - argv_offset == 4)
	{
		char	delay_type_string[64];
		int		delay_type = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset)); // delay type
		int		no_of_maps = Q_atoi(gpCmd->Cmd_Argv(3 + argv_offset)); // Number of maps

		if (delay_type == VOTE_NO_DELAY)
		{
			Q_strcpy (delay_type_string, "now");
		}
		else if (delay_type == VOTE_END_OF_ROUND_DELAY)
		{
			Q_strcpy (delay_type_string, "round");
		}
		else
		{
			Q_strcpy (delay_type_string, "end");
		}

		// Run the system vote
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_voterandom");
		gpCmd->AddParam(delay_type_string);
		gpCmd->AddParam(no_of_maps);
		this->ProcessMaVoteRandom(admin, "ma_voterandom", 0, M_MENU);
		return;
	}
	else
	{
		char	more_cmd[128];
		int		m_list_size;

		// Setup player list
		FreeMenu();

		// Pick the right map list size
		if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 0)
		{
			m_list_size = map_in_cycle_list_size;
		}
		else if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 1)
		{
			m_list_size = votemap_list_size;
		}
		else
		{
			m_list_size = map_list_size;
		}

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " [%i]",  i + 1);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin randommapvote %s %i", gpCmd->Cmd_Argv(2 + argv_offset), i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "randommapvote %s", gpCmd->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(700), Translate(701), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuSystemVoteSingleMap( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	if (argc - argv_offset == 4)
	{
		char	delay_type_string[64];
		int		delay_type = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset)); // delay type
		char	map_name[128];

		Q_strcpy(map_name, gpCmd->Cmd_Argv(3 + argv_offset));

		if (delay_type == VOTE_NO_DELAY)
		{
			Q_strcpy (delay_type_string, "now");
		}
		else if (delay_type == VOTE_END_OF_ROUND_DELAY)
		{
			Q_strcpy (delay_type_string, "round");
		}
		else
		{
			Q_strcpy (delay_type_string, "end");
		}

		// Run the system vote
		gpCmd->NewCmd();
		gpCmd->AddParam("ma_vote");
		gpCmd->AddParam("%s", delay_type_string);
		gpCmd->AddParam("%s", map_name);
		this->ProcessMaVote(admin, "ma_vote", 0, M_MENU);
		return;
	}
	else
	{
		char	more_cmd[128];
		int		m_list_size;
		map_t	*m_list;

		// Setup player list
		FreeMenu();

		// Set pointer to correct list
		switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
		{
			case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
			case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
			case 2: m_list = map_list; m_list_size = map_list_size;break;
			default : break;
		}

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " %s",  m_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin singlemapvote %s %s", gpCmd->Cmd_Argv(2 + argv_offset), m_list[i].map_name);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, m_list[i].map_name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "singlemapvote %s", gpCmd->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(710), Translate(711), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuSystemVoteBuildMap( player_t *admin, int next_index, int argv_offset )
{
	int		m_list_size;
	map_t	*m_list;

	const int argc = gpCmd->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	if (argc - argv_offset == 3)
	{
		// Map added or taken away from list

		int		map_index = -1;

		for (int i = 0; i < m_list_size; i++)
		{
			if (FStrEq(m_list[i].map_name, gpCmd->Cmd_Argv(2 + argv_offset)))
			{
				map_index = i;
				break;
			}
		}

		if (map_index == -1) 
		{
			SayToPlayer(ORANGE_CHAT, admin, "Invalid map [%s]", gpCmd->Cmd_Argv(2 + argv_offset));
			return;
		}

		if (m_list[map_index].selected_for_vote)
		{
			m_list[map_index].selected_for_vote = false;
			SayToPlayer(ORANGE_CHAT, admin, "%s", Translate(722, "%s", m_list[map_index].map_name));
		}
		else
		{
			m_list[map_index].selected_for_vote = true;
			SayToPlayer(ORANGE_CHAT, admin, "%s", Translate(723, "%s", m_list[map_index].map_name));
		}

		engine->ClientCommand(admin->entity, "admin buildmapvote\n");
		return;
	}
	else
	{
		char	more_cmd[128];

		// Setup player list
		FreeMenu();

		for( int i = 0; i < m_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			if (m_list[i].selected_for_vote)
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", Translate(724, "%s",  m_list[i].map_name));
			}
			else
			{
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), " %s",  m_list[i].map_name);
			}

			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin buildmapvote %s %s", gpCmd->Cmd_Argv(2 + argv_offset), m_list[i].map_name);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, m_list[i].map_name);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;                         
		}

		Q_snprintf( more_cmd, sizeof(more_cmd), "buildmapvote %s", gpCmd->Cmd_Argv(2 + argv_offset));

		DrawSubMenu (admin, Translate(720), Translate(721), next_index, "admin", more_cmd,	true, -1);
	}
	
	return;
}
//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuSystemVoteMultiMap( player_t *admin, int admin_index )
{
	const int argc = gpCmd->Cmd_Argc();

	if (system_vote.vote_in_progress) return;

	char	delay_type_string[64];
	int		delay_type = Q_atoi(gpCmd->Cmd_Argv(2)); // delay type

	if (delay_type == VOTE_NO_DELAY)
	{
		Q_strcpy (delay_type_string, "now");
	}
	else if (delay_type == VOTE_END_OF_ROUND_DELAY)
	{
		Q_strcpy (delay_type_string, "round");
	}
	else
	{
		Q_strcpy (delay_type_string, "end");
	}

	int		m_list_size;
	map_t	*m_list;

	// Setup player list
	FreeMenu();

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	int selected_map = 0;

	for (int i = 0; i < m_list_size; i++)
	{
		if (m_list[i].selected_for_vote)
		{
			selected_map ++;
		}
	}

	if (selected_map == 0) return;

	// Build the vote maps
	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	if (mani_vote_allow_extend.GetInt() == 1 && selected_map != 1)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend Map");
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	for (int i = 0; i < m_list_size; i++)
	{
		if (m_list[i].selected_for_vote)
		{
			this->AddMapToVote(admin, m_list[i].map_name);
			m_list[i].selected_for_vote = false;
		}
	}

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	system_vote.vote_starter = admin->index;
	system_vote.vote_confirmation = false;

	if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(551));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
	}

	StartSystemVote();
	LogCommand(admin, "Started a random map vote\n");
	AdminSayToAll(ORANGE_CHAT, admin, mani_adminvote_anonymous.GetInt(), "started a map vote"); 
	
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_votecancel command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVoteCancel(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	admin_index;

	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_CANCEL_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (!system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: No system voting to cancel!");
		return PLUGIN_STOP;
	}

	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "cancelled current vote"); 
	system_vote.vote_in_progress = false;
	if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
	{
		system_vote.map_decided = true;
	}

	for (int i = 0; i < max_players; i++)
	{
		voter_list[i].allowed_to_vote = false;
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_voterandom command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVoteRandom(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;

	int admin_index = -1;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_RANDOM_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	int	number_of_maps;

	if (gpCmd->Cmd_Argc() < 3)
	{
		// Only number of maps passed through
		number_of_maps = Q_atoi(gpCmd->Cmd_Argv(1));
	}
	else
	{
		// Delay type and number of maps passed through
		if (FStrEq(gpCmd->Cmd_Argv(1),"end"))
		{
			delay_type = VOTE_END_OF_MAP_DELAY;
		}
		else if (FStrEq(gpCmd->Cmd_Argv(1),"now"))
		{
			delay_type = VOTE_NO_DELAY;
		}
		else if (FStrEq(gpCmd->Cmd_Argv(1),"round") && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			delay_type = VOTE_END_OF_ROUND_DELAY;
		}

		number_of_maps = Q_atoi(gpCmd->Cmd_Argv(2));
	}

	if (number_of_maps == 0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You must have one or more maps !!");
		return PLUGIN_STOP;
	}

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_RANDOM_MAP;
	if (!player_ptr)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player_ptr->index;
	}

	system_vote.vote_confirmation = false;
	if (player_ptr && gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	BuildRandomMapVote(number_of_maps);
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title,Translate(551));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
	}

	StartSystemVote();
	LogCommand(player_ptr, "Started a random map vote\n");
	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "started a random map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_vote command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVote(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	admin_index;

	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;
	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	bool use_delay_string_as_first_map = false;

	if (FStrEq(gpCmd->Cmd_Argv(1),"end") ||
		(FStrEq(gpCmd->Cmd_Argv(1),"round") && gpManiGameType->IsGameType(MANI_GAME_CSS)) ||
		FStrEq(gpCmd->Cmd_Argv(1),"now"))
	{
		// Delay type parameter passed in
		if (FStrEq(gpCmd->Cmd_Argv(1),"end"))
		{
			delay_type = VOTE_END_OF_MAP_DELAY;
		}
		else if (FStrEq(gpCmd->Cmd_Argv(1),"now"))
		{
			delay_type = VOTE_NO_DELAY;
		}
		else if (FStrEq(gpCmd->Cmd_Argv(1),"round") && gpManiGameType->IsGameType(MANI_GAME_CSS))
		{
			delay_type = VOTE_END_OF_ROUND_DELAY;
		}
	}
	else
	{
		use_delay_string_as_first_map = true;
	}

	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);
	if (mani_vote_allow_extend.GetInt() == 1 &&
		((use_delay_string_as_first_map && gpCmd->Cmd_Argc() > 2) ||
		 (!use_delay_string_as_first_map && gpCmd->Cmd_Argc() > 3)))
		
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change && (winlimit_change || maxrounds_change))
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
		}
		else if (timelimit_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
		}
		else 
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
		}

		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	if (!use_delay_string_as_first_map && gpCmd->Cmd_Argc() < 3)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You must have one or more maps !!");
		return PLUGIN_STOP;
	}

	if (use_delay_string_as_first_map)
	{
		if (!AddMapToVote(player_ptr, gpCmd->Cmd_Argv(1))) return PLUGIN_STOP;
	}

	for (int i = 2; i < gpCmd->Cmd_Argc(); i++)
	{
		if (!AddMapToVote(player_ptr, gpCmd->Cmd_Argv(i))) return PLUGIN_STOP;
	}

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	if (!player_ptr)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player_ptr->index;
	}

	system_vote.vote_confirmation = false;
	if (player_ptr && gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(551));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552, "%s", vote_option_list[0].vote_command));
	}

	StartSystemVote();
	LogCommand(player_ptr, "Started a map vote\n");
	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "started a map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_voteextend command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVoteExtend(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	admin_index;

	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdminAllowed(player_ptr, command_name, ALLOW_MAP_VOTE, war_mode, &admin_index)) return PLUGIN_STOP;
	}

	if (system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		return PLUGIN_STOP;
	}

	if (!mp_timelimit && !mp_winlimit && !mp_maxrounds)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Server does not support a timelimit, win limit or max rounds !!");
		return PLUGIN_STOP;
	}

	bool timelimit_change = false;
	bool winlimit_change = false;
	bool maxrounds_change = false;

	if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
	if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
	if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

	if (!timelimit_change && !winlimit_change && !maxrounds_change)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: This server has an infinite time limit, win limit and max rounds limit!");
		return PLUGIN_STOP;
	}

	// Default mode
	int delay_type = VOTE_NO_DELAY;
	vote_option_t	vote_option;

	// Add Extend map if allowed and more than one map being voted
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	if (timelimit_change && winlimit_change && maxrounds_change)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
	}
	else if (timelimit_change)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
	}
	else 
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
	}

	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	system_vote.delay_action = delay_type;
	system_vote.vote_type = VOTE_MAP;
	if (!player_ptr)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player_ptr->index;
	}

	system_vote.vote_confirmation = false;
	if (player_ptr && gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();

	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(553,"%i", mani_vote_extend_time.GetInt()));

	StartSystemVote();
	LogCommand(player_ptr, "Started an extend map vote\n");
	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "started an extend map vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process Adding a map to the vote option list
//---------------------------------------------------------------------------------
bool	ManiVote::AddMapToVote
(
 player_t *player_ptr,
 const char *map_name
)
{
	vote_option_t vote_option;
	map_t	*m_list;
	int		m_list_size;

	// Set pointer to correct list
	switch (mani_vote_mapcycle_mode_for_admin_map_vote.GetInt())
	{
		case 0: m_list = map_in_cycle_list; m_list_size = map_in_cycle_list_size;break;
		case 1: m_list = votemap_list; m_list_size = votemap_list_size;break;
		case 2: m_list = map_list; m_list_size = map_list_size;break;
		default : break;
	}

	bool valid_map = false;

	for (int i = 0; i < m_list_size; i ++)
	{
		if (FStrEq(m_list[i].map_name, map_name))
		{
			valid_map = true;
			break;
		}
	}

	if (!valid_map)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Map [%s] is invalid !!", map_name);
		return false;
	}
	
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", map_name);
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", map_name);
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_votequestion command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVoteQuestion(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	admin_index;

	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdmin(player_ptr, &admin_index))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to use admin commands");
			return PLUGIN_STOP;
		}

		if ((command_type != M_MENU && !gpManiClient->IsAdminAllowed(admin_index, ALLOW_QUESTION_VOTE)) ||
		    (command_type == M_MENU && !gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_QUESTION_VOTE))  || war_mode)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to start a question vote");
			return PLUGIN_STOP;
		}
	}

	if (gpCmd->Cmd_Argc() < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Cannot run as system vote is already in progress!");
		return PLUGIN_STOP;
	}

	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	for (int i = 2; i < gpCmd->Cmd_Argc(); i++)
	{
		this->AddQuestionToVote(gpCmd->Cmd_Argv(i));
	}

	if (vote_option_list_size == 0)
	{
		vote_option_t vote_option;
	
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_YES));
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , Translate(M_MENU_YES));
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	system_vote.delay_action = 0;
	system_vote.vote_type = VOTE_QUESTION;
	if (!player_ptr)
	{
        system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player_ptr->index;
	}

	system_vote.vote_confirmation = false;
	if (player_ptr && gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();
	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", gpCmd->Cmd_Argv(1));

	StartSystemVote();
	LogCommand(player_ptr, "Started a question vote\n");
	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "started a question vote"); 

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Process Adding a question to the vote option list
//---------------------------------------------------------------------------------
bool	ManiVote::AddQuestionToVote(const char *answer)
{
	vote_option_t vote_option;
	
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", answer);
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", answer);
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Process the ma_votercon command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiVote::ProcessMaVoteRCon(player_t *player_ptr, const char *command_name, const int help_id, const int command_type)
{
	int	admin_index;

	if (mani_voting.GetInt() == 0) return PLUGIN_CONTINUE;

	if (player_ptr)
	{
		// Check if player is admin
		if (!gpManiClient->IsAdmin(player_ptr, &admin_index))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to use admin commands");
			return PLUGIN_STOP;
		}

		if ((command_type != M_MENU && !gpManiClient->IsAdminAllowed(admin_index, ALLOW_RCON_VOTE)) || 
		    (command_type == M_MENU && !gpManiClient->IsAdminAllowed(admin_index, ALLOW_MENU_RCON_VOTE)))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: You are not authorised to start a rcon vote");
			return PLUGIN_STOP;
		}
	}

	if (gpCmd->Cmd_Argc() < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	if (system_vote.vote_in_progress)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Cannot run as system vote is already in progress !!");
		return PLUGIN_STOP;
	}

	vote_option_t	vote_option;

	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", gpCmd->Cmd_Args(2));
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", gpCmd->Cmd_Args(2));
	vote_option.votes_cast = 0;
	vote_option.null_command = false;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;

	system_vote.delay_action = 0;
	system_vote.vote_type = VOTE_RCON;
	if (!player_ptr)
	{
		system_vote.vote_starter = -1;
	}
	else
	{
		system_vote.vote_starter = player_ptr->index;
	}

	system_vote.vote_confirmation = false;
	if (player_ptr && gpManiClient->IsAdminAllowed(admin_index, ALLOW_ACCEPT_VOTE))
	{
		system_vote.vote_confirmation = true;
	}

	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	IsYesNoVote();
	Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", gpCmd->Cmd_Argv(1));

	this->StartSystemVote();
	LogCommand(player_ptr, "Started a RCON vote\n");
	AdminSayToAll(ORANGE_CHAT, player_ptr, mani_adminvote_anonymous.GetInt(), "started a RCON vote"); 

	return PLUGIN_STOP;
}

//*******************************************************************************
// Count Votes
//*******************************************************************************
void	ManiVote::ProcessVotes (void)
{
	int number_of_votes = 0;
	int	highest_index = 0;
	int votes_needed;
	show_vote_t	*show_vote_list = NULL;
	int			show_vote_list_size = 0;

	player_t admin;

	
	// Get number of votes cast and arrange in list, this could be 
	// done with a simple for loop and find the highest index, but
	// we need to match the vote results seen on screen if there
	// are or more options that share the same highest votes.
	for (int i = 0; i < vote_option_list_size; i ++)
	{
		// Mark the option with the highest number of votes
		if (vote_option_list[i].votes_cast > 0)
		{
			// Build up list of vote entries
			AddToList((void **) &show_vote_list, sizeof(show_vote_t), &show_vote_list_size);
			strcpy(show_vote_list[show_vote_list_size - 1].option, vote_option_list[i].vote_name);
			show_vote_list[show_vote_list_size - 1].votes_cast = vote_option_list[i].votes_cast;
			show_vote_list[show_vote_list_size - 1].original_vote_list_index = i;
			number_of_votes +=  vote_option_list[i].votes_cast;
		}
	}

	// Sort into order
	qsort(show_vote_list, show_vote_list_size, sizeof(show_vote_t), sort_show_votes_cast); 

	system_vote.split_winner = 0;

	if (show_vote_list_size != 0)
	{
		if (show_vote_list_size > 1)
		{
			// Two or more options voted for
			if (show_vote_list[0].votes_cast == show_vote_list[1].votes_cast)
			{
				// At the minimum, the top two votes have the same number of votes
				// Need to count how many have the same amount of votes cast

				int vote_count = 0;
				for (int i = 0; i < show_vote_list_size; i ++)
				{
					if (show_vote_list[i].votes_cast == show_vote_list[0].votes_cast)
					{
						vote_count ++;
					}
				}

				// Get random choice from the number of options found
				highest_index = show_vote_list[(rand() % vote_count)].original_vote_list_index;
				system_vote.split_winner = vote_count;
			}
			else
			{
				highest_index = show_vote_list[0].original_vote_list_index;
				system_vote.split_winner = 1;
			}
		}
		else
		{
			highest_index = show_vote_list[0].original_vote_list_index;
			system_vote.split_winner = 1;
		}
	}

	// Free up memory
	FreeList((void **) &show_vote_list, &show_vote_list_size);

	if (mani_vote_show_vote_mode.GetInt() != 0)
	{
		SayToAll(GREEN_CHAT, true, "Results are in, %i vote%s cast", number_of_votes, (number_of_votes == 1) ? " was":"s were");
		if (system_vote.split_winner > 1)
		{
			SayToAll(GREEN_CHAT, true, "%i options share equal highest votes, picking random choice...", 
						system_vote.split_winner);
		}
	}

	// We must have at least 1 vote
	if (number_of_votes == 0)
	{
		system_vote.vote_in_progress = false;
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			// Stop another random vote happening
			system_vote.map_decided = true;
		}

		SayToAll(GREEN_CHAT, true, "Vote failed, nobody voted");
		return;
	}

	float vote_percentage;

	switch (system_vote.vote_type)
	{
		case VOTE_RANDOM_END_OF_MAP: vote_percentage = mani_vote_end_of_map_percent_required.GetFloat();break;
		case VOTE_RANDOM_MAP: vote_percentage = mani_vote_random_map_percent_required.GetFloat();break;
		case VOTE_MAP: vote_percentage = mani_vote_map_percent_required.GetFloat();break;
		case VOTE_QUESTION: vote_percentage = mani_vote_question_percent_required.GetFloat();break;
		case VOTE_RCON: vote_percentage = mani_vote_rcon_percent_required.GetFloat();break;
		case VOTE_EXTEND_MAP: vote_percentage = mani_vote_extend_percent_required.GetFloat();break;
		default:break;
	}

	votes_needed = (int) ((float) system_vote.max_votes * (vote_percentage * 0.01));

	// Do some safety rounding
	if (votes_needed == 0) votes_needed = 1;
	if (votes_needed > number_of_votes) votes_needed = system_vote.max_votes;

	int	player_vote_count = 0;

	for (int i = 0; i < max_players; i++)
	{
		if (voter_list[i].voted)
		{
			player_vote_count ++;
		}
	}

	// Stop more players from voting
	for (int i = 0; i < max_players; i++)
	{
		voter_list[i].allowed_to_vote = false;
	}

	if (player_vote_count == 0 || player_vote_count < votes_needed)
	{
		SayToAll (GREEN_CHAT, true, "Voting failed, not enough players voted");
		system_vote.vote_in_progress = false;
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			// Stop another random vote happening
			system_vote.map_decided = true;
		}

		return;
	}

	system_vote.winner_index = highest_index;

	// Yes we do
	// Do we need need an admin to confirm results (exclude 'No' wins) ?
	if (system_vote.vote_confirmation && !(vote_option_list[1].null_command && highest_index == 1) && system_vote.vote_type != VOTE_QUESTION)
	{
		admin.index = system_vote.vote_starter;
		// We must find the admin if they are still there
		if (FindPlayerByIndex(&admin))
		{
			char	result_text[512];

			// Yes we do, show them the menu
			FreeMenu();

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_YES));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_vote_accept");

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), Translate(M_MENU_NO));
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_vote_refuse");

			Q_snprintf(result_text, sizeof (result_text), "%s", Translate(661, "%s", vote_option_list[highest_index].vote_name));
			DrawStandardMenu(&admin, Translate(660), result_text, false);
			system_vote.waiting_decision = true;
			system_vote.waiting_decision_time = gpGlobals->curtime + 30.0;
			return;
		}
	}

	// Process the win
	ProcessVoteWin (highest_index);
	system_vote.vote_in_progress = false;

}

//*******************************************************************************
// Builds up a string that can be shown to players who are interested in the
// voting progress
//*******************************************************************************
void	ManiVote::BuildCurrentVoteLeaders (void)
{
	show_vote_t	*show_vote_list = NULL;
	int			show_vote_list_size = 0;
	
	// Get number of votes cast
	for (int i = 0; i < vote_option_list_size; i ++)
	{
		// Mark the option with the highest number of votes
		if (vote_option_list[i].votes_cast > 0)
		{
			// Build up list of vote entries
			AddToList((void **) &show_vote_list, sizeof(show_vote_t), &show_vote_list_size);
			strcpy(show_vote_list[show_vote_list_size - 1].option, vote_option_list[i].vote_name);
			show_vote_list[show_vote_list_size - 1].votes_cast = vote_option_list[i].votes_cast;
		}
	}

	strcpy(show_hint_results, "");
	if (show_vote_list_size == 0)
	{
		return;
	}

	// Sort into order
	qsort(show_vote_list, show_vote_list_size, sizeof(show_vote_t), sort_show_votes_cast); 

	bool do_cr = false;
	for (int i = 0; i < show_vote_list_size && i < 3; i++)
	{
		char	result_string[256];

		if (do_cr)
		{
			strcat(show_hint_results,"\n");
		}

		Q_snprintf(result_string, sizeof(result_string), "%i. %s: (%i)", 
					i+1,
					show_vote_list[i].option, 
					show_vote_list[i].votes_cast);
		strcat(show_hint_results, result_string);
		do_cr = true;
	}

	MRecipientFilter mrf;
	bool	found_player = false;

	mrf.MakeReliable();
	mrf.RemoveAllRecipients();

	// Show results to players who care about it
	for (int i = 0; i < max_players; i++)
	{
		player_t player;

		player.index = i + 1;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		player_settings_t *player_settings = FindPlayerSettings(&player);
		if (player_settings)
		{
			if (player_settings->show_vote_results_progress == 1)
			{
				// Add to the viewing list
				mrf.AddPlayer(i + 1);
				found_player = true;
			}
		}
	}

	if (found_player)
	{
		// Show hint message at bottom of screen as it's less
		// pervasive.
		UTIL_SayHint(&mrf, this->GetCompleteVoteProgress());
	}

	FreeList((void **) &show_vote_list, &show_vote_list_size);
}

//*******************************************************************************
// Get vote in progress string
//*******************************************************************************
char	*ManiVote::GetCompleteVoteProgress (void)
{
	int time_left = (int) (system_vote.end_vote_time - gpGlobals->curtime);

	if (time_left < 0) time_left = 0;

	// 'Votes: 12/32, 5s left'
	Q_snprintf(vote_progress_string, sizeof(vote_progress_string),
			"%s %i/%i, %is %s\n%s",
				Translate(1268),
				system_vote.votes_so_far,
				system_vote.total_votes_needed,
				time_left,
				Translate(1267),
				show_hint_results); 

	return vote_progress_string;
}
//*******************************************************************************
// Process an admins confirmation selection
//*******************************************************************************
void	ManiVote::ProcessVoteConfirmation (player_t *player, bool accept)
{
	if (!system_vote.vote_confirmation) return;
	if (!system_vote.vote_in_progress) return;
	if (system_vote.vote_starter == -1)
	{
		// Non player started vote
		ProcessVoteWin (system_vote.winner_index);
		system_vote.vote_in_progress = false;
		return;
	}

	// If vote starter matches this player slot
	if (system_vote.vote_starter == player->index)
	{
		if (accept)
		{
			AdminSayToAll(ORANGE_CHAT, player, mani_adminvote_anonymous.GetInt(), "accepted vote"); 
			ProcessVoteWin (system_vote.winner_index);
		}
		else
		{
			AdminSayToAll(ORANGE_CHAT, player, mani_adminvote_anonymous.GetInt(), "refused vote"); 
			ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_VOTEEND);
		}
	}

	system_vote.vote_in_progress = false;
}

//*******************************************************************************
// Process a Vote Win, i.e change map, run rcon command, show result
//*******************************************************************************
void	ManiVote::ProcessVoteWin (int win_index)
{
	switch (system_vote.vote_type)
	{
		case VOTE_RANDOM_END_OF_MAP: ProcessMapWin(win_index);break;
		case VOTE_RANDOM_MAP: ProcessMapWin(win_index);break;
		case VOTE_MAP: ProcessMapWin(win_index);break;
		case VOTE_QUESTION: ProcessQuestionWin(win_index);break;
		case VOTE_RCON: ProcessRConWin(win_index);break;
		case VOTE_EXTEND_MAP: ProcessExtendWin(win_index);break;
		case VOTE_ROCK_THE_VOTE: ProcessRockTheVoteWin(win_index);break;
		default:break;
	}

	ProcessPlayActionSound(NULL, MANI_ACTION_SOUND_VOTEEND);

}

//*******************************************************************************
// Finalise a Random Map Win
//*******************************************************************************
void	ManiVote::ProcessMapWin (int win_index)
{
	SayToAll (LIGHT_GREEN_CHAT, true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (LIGHT_GREEN_CHAT, true, "Next map will still be %s", next_map);
		system_vote.map_decided = true;
		return;
	}

	// If extend allowed and win index = 0 (extend)
	if (FStrEq(vote_option_list[win_index].vote_command,"mani_extend_map"))
	{
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP) system_vote.number_of_extends ++;
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "System vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		return;
	}

	Q_strcpy(forced_nextmap,vote_option_list[win_index].vote_command);
	Q_strcpy(next_map, vote_option_list[win_index].vote_command);
	mani_nextmap.SetValue(next_map);

	LogCommand (NULL, "System vote set nextmap to %s\n", vote_option_list[win_index].vote_command);
	override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
	override_setnextmap = true;
	if (system_vote.delay_action == VOTE_NO_DELAY)
	{
		SayToAll(LIGHT_GREEN_CHAT, true, "Map will change to %s in 5 seconds", vote_option_list[win_index].vote_command);
		trigger_changemap = true;
		trigger_changemap_time = gpGlobals->curtime + 5.0;
	}
	else if (system_vote.delay_action == VOTE_END_OF_ROUND_DELAY)
	{
		SayToAll(LIGHT_GREEN_CHAT, true, "Map will change to %s at the end of the round", vote_option_list[win_index].vote_command);
		if (mp_timelimit)
		{
			mp_timelimit->SetValue(1);
		}
	}
	else
	{
		SayToAll(LIGHT_GREEN_CHAT, true, "Next Map will be %s", vote_option_list[win_index].vote_command);
	}

	system_vote.map_decided = true;
}

//*******************************************************************************
// Finalise an RCon vote
//*******************************************************************************
void	ManiVote::ProcessRConWin (int win_index)
{
	SayToAll (LIGHT_GREEN_CHAT, true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (LIGHT_GREEN_CHAT, true, "Command will not be executed");
		return;
	}
	else
	{
		char	server_cmd[512];
		Q_snprintf(server_cmd, sizeof(server_cmd), "%s\n", vote_option_list[win_index].vote_command);
		SayToAll(LIGHT_GREEN_CHAT, true, "Executing RCON command voted for");
		LogCommand (NULL, "System vote ran rcon command %s\n", vote_option_list[win_index].vote_command);
		engine->ServerCommand(server_cmd);
	}
}


//*******************************************************************************
// Finalise a Question win
//*******************************************************************************
void	ManiVote::ProcessQuestionWin (int win_index)
{
	SayToAll (LIGHT_GREEN_CHAT, true, "%s", Translate(554,"%s", system_vote.vote_title));
	SayToAll (LIGHT_GREEN_CHAT, true, "%s", Translate(555,"%s", vote_option_list[win_index].vote_name));
}

//*******************************************************************************
// Finalise an Extend win
//*******************************************************************************
void	ManiVote::ProcessExtendWin (int win_index)
{
	SayToAll (LIGHT_GREEN_CHAT, true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (LIGHT_GREEN_CHAT, true, "Map will not be extended");
		return;
	}

	// If extend allowed and win index = 0 (extend)
	if (FStrEq(vote_option_list[win_index].vote_command,"mani_extend_map"))
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "System vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "System vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		return;
	}
}


//*******************************************************************************
// Check to see if there is only one vote option, and if so, make it a Yes/No
// vote
//*******************************************************************************
bool	ManiVote::IsYesNoVote (void)
{
	vote_option_t vote_option;

	if (vote_option_list_size > 1) return false;

	// Need to make sure that vote options include 'No'
	Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_NO));
	Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "");
	vote_option.votes_cast = 0;
	vote_option.null_command = true;
	AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
	vote_option_list[vote_option_list_size - 1] = vote_option;
	Q_snprintf(vote_option_list[0].vote_name, sizeof(vote_option.vote_name) , Translate(M_MENU_YES));

	return true;
}


//*******************************************************************************
// Build Random map vote
//*******************************************************************************
void	ManiVote::BuildRandomMapVote (int max_maps)
{
	last_map_t *last_maps;
	int	maps_to_skip;
	vote_option_t vote_option;
	map_t	*m_list;
	int		m_list_size;
	int		map_index;
	int		exclude;
	map_t	select_map;
	map_t	*select_list;
	map_t	*temp_ptr;
	int		select_list_size;
	map_t	temp_map;

	select_list = NULL;
	select_list_size = 0;

	// Get list of maps already played and exclude them from being voted !!
	last_maps = GetLastMapsPlayed(&maps_to_skip, mani_vote_dont_show_last_maps.GetInt());
	
	// Pick the right map list
	if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 0)
	{
		m_list = map_in_cycle_list;
		m_list_size = map_in_cycle_list_size;
	}
	else if (mani_vote_mapcycle_mode_for_random_map_vote.GetInt() == 1)
	{
		m_list = votemap_list;
		m_list_size = votemap_list_size;
	}
	else
	{
		m_list = map_list;
		m_list_size = map_list_size;
	}

	// Exclude maps already played by pretending they are already selected.
	exclude = 0;
	for (int i = 0; i < m_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < maps_to_skip; j ++)
		{
			if (FStrEq(last_maps[j].map_name, m_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", m_list[i].map_name);
			AddToList((void **) &select_list, sizeof(map_t), &select_list_size);
			select_list[select_list_size - 1] = select_map;
		}
	}

	if (max_maps > select_list_size)
	{
		// Restrict max maps
		max_maps = select_list_size;
	}

	// Add Extend map ?
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	bool allow_extend = false;

	if (mani_vote_allow_extend.GetInt() == 1 && 
		max_maps > 1)
	{
		allow_extend = true;
	}

	if (allow_extend)
	{
		if (system_vote.vote_type == VOTE_RANDOM_END_OF_MAP)
		{
			if (mani_vote_max_extends.GetInt() != 0)
			{
				if (system_vote.number_of_extends >= mani_vote_max_extends.GetInt())
				{
					allow_extend = false;
				}
			}
		}
	}

	if (allow_extend)
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change && winlimit_change && maxrounds_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes and %i rounds", mani_vote_extend_time.GetInt(), mani_vote_extend_rounds.GetInt());
		}
		else if (timelimit_change)
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i minutes", mani_vote_extend_time.GetInt());
		}
		else
		{
			Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "Extend by %i rounds", mani_vote_extend_rounds.GetInt());
		}

		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "mani_extend_map");
		vote_option.votes_cast = 0;
		vote_option.null_command = false;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	// Generate a bit more randomness
	srand( (unsigned)time(NULL));
	for (int i = 0; i < max_maps; i ++)
	{
		map_index = rand() % select_list_size;

		// Add map to vote options list
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", select_list[map_index].map_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", select_list[map_index].map_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;

		if (map_index != select_list_size - 1)
		{
			temp_map = select_list[select_list_size - 1];
			select_list[select_list_size - 1] = select_list[map_index];
			select_list[map_index] = temp_map;
		}

		if (select_list_size != 1)
		{
			// Shrink array by 1
			temp_ptr = (map_t *) realloc(select_list, (select_list_size - 1) * sizeof(map_t));
			select_list = temp_ptr;
			select_list_size --;
		}
		else
		{
			free(select_list);
			select_list = NULL;
			select_list_size = 0;
			break;
		}
	}

	if (select_list != NULL)
	{
		free(select_list);
	}

	// Map list built
}



//---------------------------------------------------------------------------------
// Purpose: Generic function to handle any system voting
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuSystemVotemap( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	// Check if system vote still allowed 

	if (!system_vote.vote_in_progress)
	{
		SayToPlayer(ORANGE_CHAT, player, "Voting not allowed at this time !!");
		return;
	}

	if (voter_list[player->index - 1].voted)
	{
		SayToPlayer(ORANGE_CHAT, player, "You have already voted !!");
		return;
	}

	if (!voter_list[player->index - 1].allowed_to_vote)
	{
		SayToPlayer(ORANGE_CHAT, player, "You are too late to join this vote !!");
		return;
	}

	if (argc - argv_offset == 2)
	{
		ProcessPlayerVoted(player, Q_atoi(gpCmd->Cmd_Argv(1 + argv_offset)));
		return;
	}

	// Show votes to player
	FreeMenu();
	CreateList ((void **) &menu_list, sizeof(menu_t), vote_option_list_size, &menu_list_size);

	for( int i = 0; i < vote_option_list_size; i++ )
	{
		Q_snprintf( menu_list[i].menu_text, sizeof(menu_list[i].menu_text), "%s", vote_option_list[i].vote_name);
		Q_snprintf( menu_list[i].menu_command, sizeof(menu_list[i].menu_command), "mani_votemenu %i", i);
	}

	if (menu_list_size == 0) return;

	// List size may have changed
	if (next_index > menu_list_size)
	{
		// Reset index
		next_index = 0;
	}

	// Show menu to player
	DrawSubMenu (player, Translate(550), system_vote.vote_title, next_index,"mani_votemenu", "", false, (int) (system_vote.end_vote_time - gpGlobals->curtime));
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Generic function to handle any system voting
//---------------------------------------------------------------------------------
void ManiVote::ProcessPlayerVoted( player_t *player, int vote_index)
{
	voter_list[player->index - 1].voted = true;
	voter_list[player->index - 1].allowed_to_vote = false;
	voter_list[player->index - 1].vote_option_index = vote_index;
	vote_option_list[vote_index].votes_cast ++;
	system_vote.votes_so_far ++;

	switch(mani_vote_show_vote_mode.GetInt())
	{
		case (0): SayToPlayer (ORANGE_CHAT, player, "You voted for %s", vote_option_list[vote_index].vote_name); break;
		case (1): SayToAll (ORANGE_CHAT, true, "Player %s voted", player->name); break;
		case (2): SayToAll (ORANGE_CHAT,true, "Vote for %s", vote_option_list[vote_index].vote_name); break;
		case (3): SayToAll(ORANGE_CHAT,true, "%s voted %s", player->name, vote_option_list[vote_index].vote_name); break;
		default : break;
	}

	system_vote.votes_required --;
	if (system_vote.votes_required <= 0)
	{
		// Everyone has voted, force timeout in game frame code !!
		system_vote.end_vote_time = 0;
	}

	this->BuildCurrentVoteLeaders();
}

//---------------------------------------------------------------------------------
// Purpose: Build up a map list for user started votes (run once a map)
//---------------------------------------------------------------------------------
void ManiVote::ProcessBuildUserVoteMaps(void)
{
	last_map_t *last_maps;
	int	maps_to_skip;
	map_t	*m_list;
	int		m_list_size;
	map_t	select_map;

	FreeList ((void **) &user_vote_map_list, &user_vote_map_list_size);

	// Get list of maps already played and exclude them from being voted !!
	last_maps = GetLastMapsPlayed(&maps_to_skip, mani_vote_dont_show_last_maps.GetInt());

	if (maps_to_skip != 0)
	{
//		MMsg("Maps Not Included for voting !!\n");
		for (int i = 0; i < maps_to_skip; i ++)
		{
//			MMsg("%s ", last_maps[i].map_name);
		}

//		MMsg("\n");
	}

	m_list = votemap_list;
	m_list_size = votemap_list_size;

	// Exclude maps already played
	for (int i = 0; i < m_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < maps_to_skip; j ++)
		{
			if (FStrEq(last_maps[j].map_name, m_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", m_list[i].map_name);
			AddToList((void **) &user_vote_map_list, sizeof(map_t), &user_vote_map_list_size);
			user_vote_map_list[user_vote_map_list_size - 1] = select_map;
		}
	}

//	MMsg("Maps available for user vote\n");
	for (int i = 0; i < user_vote_map_list_size; i ++)
	{
//		MMsg("%s ", user_vote_map_list[i].map_name);
	}
//	MMsg("\n");	
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
void ManiVote::ShowCurrentUserMapVotes( player_t *player_ptr, int votes_required )
{

	OutputToConsole(player_ptr, "\n");
	OutputToConsole(player_ptr, "%s\n", mani_version);
	OutputToConsole(player_ptr, "\nVotes required for map change is %i\n\n", votes_required);
	OutputToConsole(player_ptr, "ID  Map Name            Votes\n");
	OutputToConsole(player_ptr, "-----------------------------\n");

	int votes_found;

	if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
		system_vote.number_of_extends < mani_vote_max_extends.GetInt())
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change || winlimit_change || maxrounds_change)
		{
			// Extension of map allowed
			votes_found = 0;
			for (int j = 0; j < max_players; j++)
			{
				// Count votes for extension index 0
				if (user_vote_list[j].map_index == 0) votes_found ++;
			}

			OutputToConsole(player_ptr, false, "%-4i%-20s%i\n", 0, "Extend Map", votes_found);
		}
	}

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		votes_found = 0;
		for (int j = 0; j < max_players; j++)
		{
			if (user_vote_list[j].map_index == i + 1)
			{
				votes_found ++;
			}
		}

		OutputToConsole(player_ptr, false, "%-4i%-20s%i\n", i + 1, user_vote_map_list[i].map_name, votes_found);
	}

	OutputToConsole(player_ptr, false, "\nTo vote for a map, type votemap <id> or votemap <map name>\n");
	OutputToConsole(player_ptr, false, "e.g votemap 3, votemap de_dust2\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMaUserVoteMap(player_t *player_ptr, int argc, const char *map_id)
{
	int votes_required;

	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserMapVotes(player_ptr, votes_required);
		return;
	}

	if (!CanWeUserVoteMapYet(player_ptr)) return;
	if (!CanWeUserVoteMapAgainYet(player_ptr)) return;

	int		map_index = -1;

	// Deal with extend if it's available
	if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
		system_vote.number_of_extends < mani_vote_max_extends.GetInt())
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		if (timelimit_change || winlimit_change || maxrounds_change)
		{
			if (FStrEq(map_id,"0") || FStrEq(map_id, "Extend Map"))
			{
				map_index = 0;
			}
		}
	}

	if (map_index == -1)
	{
		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			// Try and match by name
			if (FStrEq(map_id, user_vote_map_list[i].map_name))
			{
				map_index = i + 1;
				break;
			}
		}

		if (map_index == -1)
		{
			// Map name not found, try map number instead
			map_index = Q_atoi (map_id);
			if (map_index < 1 || map_index > user_vote_map_list_size)
			{
				map_index = -1;
			}
		}
	}

	// Map failed validation so tell user
	if (map_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s is an invalid map to vote for, type 'votemap' to see the maps available", map_id);
		return;
	}

	// Check if player has voted already and change the vote

	bool found_vote = false;
	for (int i = 0; i < max_players; i++)
	{
		if (user_vote_list[i].map_index != -1)
		{
			found_vote = true;
			break;
		}
	}

	if (!found_vote)
	{
		// First vote, inform world
		SayToAll(GREEN_CHAT, false, "Voting started, type votemap in console or say 'votemap' in game for options");
	}

	user_vote_list[player_ptr->index - 1].map_index = map_index;
	user_vote_list[player_ptr->index - 1].map_vote_timestamp = gpGlobals->curtime;

	int counted_votes = 0;

	for (int i = 0; i < max_players; i ++)
	{
		if (user_vote_list[i].map_index == map_index)
		{
			counted_votes ++;
		}
	}

	int votes_left = votes_required - counted_votes;

	SayToAll(ORANGE_CHAT, false, "Player %s voted %s %s, %i more vote%s required", 
								player_ptr->name, 
								(map_index == 0) ? "to":"for",
								(map_index == 0) ? "Extend Map":user_vote_map_list[map_index - 1].map_name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the change of map
		ProcessUserVoteMapWin(map_index);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
void ManiVote::ProcessUserVoteMapWin(int map_index)
{
	if (map_index == 0)
	{
		bool timelimit_change = false;
		bool winlimit_change = false;
		bool maxrounds_change = false;

		system_vote.number_of_extends ++;

		if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
		if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
		if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

		// Extend the map
		if (timelimit_change)
		{
			int timelimit = mp_timelimit->GetInt();
			// Increase timeleft
			mp_timelimit->SetValue(timelimit + mani_vote_extend_time.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i minutes", mani_vote_extend_time.GetInt());
			LogCommand (NULL, "User vote extended map by %i minutes\n", mani_vote_extend_time.GetInt());
			map_start_time = gpGlobals->curtime + (mani_vote_extend_time.GetInt() * 60);
		}

		if (winlimit_change)
		{
			int winlimit = mp_winlimit->GetInt();
			// Increase timeleft
			mp_winlimit->SetValue(winlimit + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_winlimit)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "User vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}

		if (maxrounds_change)
		{
			int maxrounds = mp_maxrounds->GetInt();
			// Increase timeleft
			mp_maxrounds->SetValue(maxrounds + mani_vote_extend_rounds.GetInt());
			SayToAll(LIGHT_GREEN_CHAT, true, "Map extended by %i rounds (mp_maxrounds)", mani_vote_extend_rounds.GetInt());
			LogCommand (NULL, "User vote extended map by %i rounds\n", mani_vote_extend_rounds.GetInt());
		}
	}
	else
	{
		Q_strcpy(forced_nextmap,user_vote_map_list[map_index - 1].map_name);
		Q_strcpy(next_map, user_vote_map_list[map_index - 1].map_name);
		mani_nextmap.SetValue(next_map);
		LogCommand (NULL, "User vote set nextmap to %s\n", user_vote_map_list[map_index - 1].map_name);

		override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
		override_setnextmap = true;
		SayToAll(LIGHT_GREEN_CHAT, true, "Map will change to %s in 5 seconds", user_vote_map_list[map_index - 1].map_name);
		trigger_changemap = true;
		system_vote.map_decided = true;
		trigger_changemap_time = gpGlobals->curtime + 5.0;
		map_start_time = gpGlobals->curtime;
	}

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes
		user_vote_list[i].map_index = -1;
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteMapYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_map_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteMapAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].map_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Gets User votes required
//---------------------------------------------------------------------------------
int ManiVote::GetVotesRequiredForUserVote
( 
 bool player_leaving, 
 float percentage, 
 int minimum_votes
 )
{
	int number_of_players;
	int votes_required;

	number_of_players = GetNumberOfActivePlayers () - ((player_leaving) ? 1:0);

	votes_required = (int) ((float) number_of_players * (percentage * 0.01));

	// Kludge the votes required if unreasonable	
	if (votes_required <= 0) 
	{
		votes_required = 1;
	}
	else if (votes_required > number_of_players) 
	{
		votes_required = number_of_players;
	}

	if (votes_required < minimum_votes) votes_required = minimum_votes;

	return votes_required;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Map menu draw 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuUserVoteMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	int votes_required;


	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *map_id = gpCmd->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteMap (player, 2, map_id);
		return;
	}
	else
	{
		int votes_found;
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteMapYet(player)) return;
		if (!CanWeUserVoteMapAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		if (mani_vote_allow_user_vote_map_extend.GetInt() == 1 &&
			system_vote.number_of_extends < mani_vote_max_extends.GetInt())
		{
			bool timelimit_change = false;
			bool winlimit_change = false;
			bool maxrounds_change = false;

			if (mp_timelimit && mp_timelimit->GetInt() != 0) timelimit_change = true;
			if (mp_winlimit && mp_winlimit->GetInt() != 0) winlimit_change = true;
			if (mp_maxrounds && mp_maxrounds->GetInt() != 0) maxrounds_change = true;

			if (timelimit_change || winlimit_change || maxrounds_change)
			{
				// Extension of map allowed
				votes_found = 0;
				for (int j = 0; j < max_players; j++)
				{
					// Count votes for extension index 0
					if (user_vote_list[j].map_index == 0) votes_found ++;
				}

				AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
				Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "Extend Map [%i]", votes_found);							
				Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotemapmenu 0");
			}
		}

		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			votes_found = 0;
			for (int j = 0; j < max_players; j++)
			{
				if (user_vote_list[j].map_index == i + 1)
				{
					votes_found ++;
				}
			}

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", votes_found, user_vote_map_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotemapmenu %i", i + 1);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_map_percentage.GetFloat(), mani_vote_user_vote_map_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), "%s", Translate(561,"%i", votes_required));							

		// Draw menu list
		DrawSubMenu (player, Translate(560), votes_required_text, next_index, "mani_uservotemapmenu","", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Rock the Vote code
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// Purpose: Shows maps you can nominate
//---------------------------------------------------------------------------------
void ManiVote::ShowCurrentRockTheVoteMaps( player_t *player_ptr)
{

	OutputToConsole(player_ptr, "\n");
	OutputToConsole(player_ptr, "%s\n", mani_version);
	OutputToConsole(player_ptr, "\nMaps available for nomination\n");
	OutputToConsole(player_ptr, "ID  Map Name\n");
	OutputToConsole(player_ptr, "-----------------------------\n");

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		OutputToConsole(player_ptr, "%-4i%-20s\n", i + 1, user_vote_map_list[i].map_name);
	}

	OutputToConsole(player_ptr, false, "\nTo nominate a map, type nominate <id> or nominate <map name>\n");
	OutputToConsole(player_ptr, false, "e.g nominate 3, nominate de_dust2\n\n");

	return;
}

//*******************************************************************************
// Finalise a Random Map Win
//*******************************************************************************
void	ManiVote::ProcessRockTheVoteWin (int win_index)
{
	SayToAll (LIGHT_GREEN_CHAT, true, "Option '%s' has won the vote", vote_option_list[win_index].vote_name);

	if (vote_option_list[win_index].null_command)
	{
		SayToAll (LIGHT_GREEN_CHAT, true, "Next map will still be %s", next_map);
		system_vote.map_decided = true;
		return;
	}

	Q_strcpy(forced_nextmap,vote_option_list[win_index].vote_command);
	Q_strcpy(next_map, vote_option_list[win_index].vote_command);
	mani_nextmap.SetValue(next_map);
	LogCommand (NULL, "System vote set nextmap to %s\n", vote_option_list[win_index].vote_command);
	override_changelevel = MANI_MAX_CHANGELEVEL_TRIES;
	override_setnextmap = true;
	SayToAll(LIGHT_GREEN_CHAT, true, "Map will change to %s in 5 seconds", vote_option_list[win_index].vote_command);
	trigger_changemap = true;
	trigger_changemap_time = gpGlobals->curtime + 5.0;

	system_vote.map_decided = true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle player saying 'nominate'
//---------------------------------------------------------------------------------
void ManiVote::ProcessRockTheVoteNominateMap(player_t *player)
{
	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(ORANGE_CHAT, player, "Your rockthevote command has already been registered, you can't nominate a map");
		return;
	}

	// Run nominate menu
	engine->ClientCommand(player->entity, "mani_rtvnominate\n");
	
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool ManiVote::CanWeRockTheVoteYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_time_before_rock_the_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "Rock the Vote is not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can nominate again 
//---------------------------------------------------------------------------------
bool ManiVote::CanWeNominateAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].nominate_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "You cannot nominate again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//*******************************************************************************
// Build Rock the vote map
//*******************************************************************************
void	ManiVote::BuildRockTheVoteMapVote (void)
{
	vote_option_t vote_option;
	int	map_index;
	int	exclude;
	map_t	select_map;
	map_t	*select_list;
	map_t	*temp_ptr;
	int	select_list_size;
	map_t	temp_map;
	int	max_maps;
	vote_option_t *nominate_list = NULL;
	vote_option_t *temp_nominate_ptr;
	int		nominate_list_size = 0;

	select_list = NULL;
	select_list_size = 0;

	// Get maps nominated by users
	for (int i = 0; i < max_players; i++ )
	{
		bool	found_map;

		found_map = false;

		if (user_vote_list[i].nominated_map == -1) continue;

		for (int j = 0; j < nominate_list_size; j++)
		{
			if (FStrEq(nominate_list[j].vote_name, user_vote_map_list[user_vote_list[i].nominated_map].map_name)) 
			{
				nominate_list[j].votes_cast ++;
				found_map = true;
				break;
			}
		}

		if (!found_map)
		{
			AddToList((void **) &nominate_list, sizeof(vote_option_t), &nominate_list_size);
			Q_strcpy(nominate_list[nominate_list_size - 1].vote_name,  user_vote_map_list[user_vote_list[i].nominated_map].map_name);
			nominate_list[nominate_list_size - 1].votes_cast = 1;
		}
	}

	// Sort into winning order
	qsort(nominate_list, nominate_list_size, sizeof(vote_option_t), sort_nominations_by_votes_cast); 

	for (int i = 0; i < nominate_list_size; i++)
	{
		MMsg("Nominations [%s] Votes [%i]\n", nominate_list[i].vote_name, nominate_list[i].votes_cast);
	}

	if (mani_vote_rock_the_vote_number_of_nominations.GetInt() < nominate_list_size)
	{
		// Prune the list down
		temp_nominate_ptr = (vote_option_t *) realloc(nominate_list, 
					(mani_vote_rock_the_vote_number_of_nominations.GetInt() * sizeof(vote_option_t)));
		nominate_list = temp_nominate_ptr;
		nominate_list_size = mani_vote_rock_the_vote_number_of_nominations.GetInt();
	}


	// Exclude maps already played by pretending they are already selected.
	exclude = 0;
	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		bool exclude_map = false;

		for (int j = 0; j < nominate_list_size; j ++)
		{
			if (FStrEq(nominate_list[j].vote_name, user_vote_map_list[i].map_name))
			{
				exclude_map = true;
				break;
			}
		}

		if (!exclude_map)
		{
			// Add map to selection list
			Q_snprintf(select_map.map_name, sizeof(select_map.map_name) , "%s", user_vote_map_list[i].map_name);
			AddToList((void **) &select_list, sizeof(map_t), &select_list_size);
			select_list[select_list_size - 1] = select_map;
		}
	}

	// Calculate number of random maps to add
	max_maps = mani_vote_rock_the_vote_number_of_maps.GetInt() - nominate_list_size;
	if (max_maps < 0) max_maps = 0;

	if (max_maps > select_list_size)
	{
		// Restrict max maps
		max_maps = select_list_size;
	}

	// Add Extend map ?
	FreeList ((void **) &vote_option_list, &vote_option_list_size);

	// Add nominated maps to vote options list
	for (int i = 0; i < nominate_list_size; i++)
	{
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", nominate_list[i].vote_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", nominate_list[i].vote_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;
	}

	// Generate a bit more randomness
	srand( (unsigned)time(NULL));
	for (int i = 0; i < max_maps; i ++)
	{
		map_index = rand() % select_list_size;

		// Add map to vote options list
		Q_snprintf(vote_option.vote_name, sizeof(vote_option.vote_name) , "%s", select_list[map_index].map_name);
		Q_snprintf(vote_option.vote_command, sizeof(vote_option.vote_command) , "%s", select_list[map_index].map_name);
		vote_option.null_command = false;
		vote_option.votes_cast = 0;
		AddToList((void **) &vote_option_list, sizeof(vote_option_t), &vote_option_list_size);
		vote_option_list[vote_option_list_size - 1] = vote_option;

		if (map_index != select_list_size - 1)
		{
			temp_map = select_list[select_list_size - 1];
			select_list[select_list_size - 1] = select_list[map_index];
			select_list[map_index] = temp_map;
		}

		if (select_list_size != 1)
		{
			// Shrink array by 1
			temp_ptr = (map_t *) realloc(select_list, (select_list_size - 1) * sizeof(map_t));
			select_list = temp_ptr;
			select_list_size --;
		}
		else
		{
			free(select_list);
			select_list = NULL;
			select_list_size = 0;
			break;
		}
	}

	if (select_list != NULL)
	{
		free(select_list);
	}

	// Map list built
}

//---------------------------------------------------------------------------------
// Purpose: Start Rock the Vote proceedings
//---------------------------------------------------------------------------------
void ManiVote::ProcessStartRockTheVote(void)
{
	MMsg("Triggering Rock The Vote!!\n");
	system_vote.delay_action = VOTE_NO_DELAY;
	system_vote.vote_type = VOTE_ROCK_THE_VOTE;
	system_vote.vote_starter = -1;
	system_vote.vote_confirmation = false;
	system_vote.end_vote_time = gpGlobals->curtime + mani_vote_allowed_voting_time.GetFloat();
	BuildRockTheVoteMapVote ();
	if (!IsYesNoVote())
	{
		Q_strcpy(system_vote.vote_title, Translate(551));
	}
	else
	{
		Q_snprintf(system_vote.vote_title, sizeof (system_vote.vote_title), "%s", Translate(552,"%s", vote_option_list[0].vote_command));
	}

	StartSystemVote();
	system_vote.start_rock_the_vote = false;
	system_vote.no_more_rock_the_vote = true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Map menu draw 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuRockTheVoteNominateMap( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (!CanWeNominateAgainYet(player))
	{
		return;
	}

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *map_id = gpCmd->Cmd_Argv(1 + argv_offset);
		ProcessMaRockTheVoteNominateMap (player, 2, map_id);
		return;
	}
	else
	{
		if (mani_vote_rock_the_vote_number_of_nominations.GetInt() == 0)
		{
			SayToPlayer(ORANGE_CHAT, player, "Nominations are not allowed on this server for rock the vote");
			return;
		}

		if (system_vote.map_decided)
		{
			SayToPlayer(ORANGE_CHAT, player, "Map has already been decided, rock the vote not available");
			return;
		}

		if (system_vote.no_more_rock_the_vote) 
		{
			SayToPlayer(ORANGE_CHAT, player, "No more 'rockthevote' commands are allowed on this map");
			return;
		}

		// Check if already typed rock the vote
		if (user_vote_list[player->index - 1].rock_the_vote)
		{
			SayToPlayer(ORANGE_CHAT, player, "Your rockthevote command has already been registered, you can't nominate another map");
			return;
		}

		if (!CanWeNominateAgainYet(player))
		{
			return;
		}

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 0; i < user_vote_map_list_size; i++)
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", user_vote_map_list[i].map_name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_rtvnominate %i", i + 1);
		}

		if (menu_list_size == 0) return;

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		// Draw menu list
		DrawSubMenu (player, "Rock The Vote\nChoose a map to nominated then type rockthevote in game", "Press Esc to nominate a map", next_index, "mani_rtvnominate","", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process nominations for rock the vote
//---------------------------------------------------------------------------------
void ManiVote::ProcessMaRockTheVoteNominateMap(player_t *player, int argc, const char *map_id)
{
	int map_index;

	if (mani_voting.GetInt() == 0) return;
	if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;
	if (mani_vote_rock_the_vote_number_of_nominations.GetInt() == 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "Nominations are not allowed on this server for rock the vote");
		return;
	}

	if (system_vote.map_decided)
	{
		SayToPlayer(ORANGE_CHAT, player, "Map has already been decided, rock the vote not available");
		return;
	}

	if (system_vote.no_more_rock_the_vote) 
	{
		SayToPlayer(ORANGE_CHAT, player, "No more 'rockthevote' commands are allowed on this map");
		return;
	}

	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(ORANGE_CHAT, player, "Your rockthevote command has already been registered, you can't nominate another map");
		return;
	}


	if (argc == 1)
	{
		ShowCurrentRockTheVoteMaps(player);
		return;
	}

	if (!CanWeNominateAgainYet(player))
	{
		return;
	}

	map_index = -1;

	for (int i = 0; i < user_vote_map_list_size; i++)
	{
		// Try and match by name
		if (FStrEq(map_id, user_vote_map_list[i].map_name))
		{
			map_index = i;
			break;
		}
	}

	if (map_index == -1)
	{
		// Map name not found, try map number instead
		map_index = Q_atoi (map_id);
		if (map_index < 1 || map_index > user_vote_map_list_size)
		{
			map_index = -1;
		}
		else
		{
			map_index --;
		}
	}

	// Map failed validation so tell user
	if (map_index == -1)
	{
		SayToPlayer(ORANGE_CHAT, player, "%s is an invalid map to vote for, type 'nominate' to see the maps available", map_id);
		return;
	}

	user_vote_list[player->index - 1].nominated_map = map_index;
	user_vote_list[player->index - 1].nominate_timestamp = gpGlobals->curtime;

	SayToAll(ORANGE_CHAT, false, "Player %s nominated map %s for rock the vote", 
								player->name, 
								user_vote_map_list[map_index].map_name);

}


//---------------------------------------------------------------------------------
// Purpose: Process players who type 'rockthevote'
//---------------------------------------------------------------------------------
void ManiVote::ProcessMaRockTheVote(player_t *player)
{

	if (mani_voting.GetInt() == 0) return;
	if (mani_vote_allow_rock_the_vote.GetInt() == 0) return;
	if (system_vote.map_decided) 
	{
		SayToPlayer(ORANGE_CHAT, player, "Map has already been decided, rock the vote not available");
		return;
	}

	if (system_vote.no_more_rock_the_vote) 
	{
		SayToPlayer(ORANGE_CHAT, player, "No more 'rockthevote' commands are allowed on this map");
		return;
	}

	// Check if already typed rock the vote
	if (user_vote_list[player->index - 1].rock_the_vote)
	{
		SayToPlayer(ORANGE_CHAT, player, "Your rockthevote command has already been registered");
		return;
	}

	if (!CanWeRockTheVoteYet(player))
	{
		return;
	}


	int counted_votes = 0;

	user_vote_list[player->index - 1].rock_the_vote = true;

	for (int i = 0; i < max_players; i ++)
	{
		if (user_vote_list[i].rock_the_vote)
		{
			counted_votes ++;
		}
	}

	int votes_required;

	votes_required = GetVotesRequiredForUserVote(false, mani_vote_rock_the_vote_threshold_percent.GetFloat(), mani_vote_rock_the_vote_threshold_minimum.GetInt());
	if (votes_required > counted_votes)
	{
		if (counted_votes == 1)
		{
			SayToAll(LIGHT_GREEN_CHAT, false, "Player %s has started rock the vote, type 'rockthevote' to join in, %i votes required", player->name, votes_required - counted_votes);
			if (mani_vote_rock_the_vote_number_of_maps.GetInt() != 0)
			{
				SayToAll(LIGHT_GREEN_CHAT, false, "Type 'nominate' to bring up a nomination menu for your favourite map");
			}
		}
		else
		{
			SayToAll(ORANGE_CHAT, false, "Player %s has entered rock the vote, %i votes required to trigger map vote", player->name, votes_required - counted_votes);
		}

		return;
	}

	// Threshold reached
	system_vote.start_rock_the_vote = true;
	if (system_vote.vote_in_progress)
	{
		SayToAll(ORANGE_CHAT, false, "Rock the vote will start after the current system vote has ended");
	}

	for (int i = 0; i < max_players; i ++)
	{
		user_vote_list[i].rock_the_vote = true;
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Vote kick code 
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void ManiVote::ShowCurrentUserKickVotes( player_t *player_ptr, int votes_required )
{

	OutputToConsole(player_ptr, "\n");
	OutputToConsole(player_ptr, "%s\n", mani_version);
	OutputToConsole(player_ptr, "\nVotes required for user kick is %i\n\n", votes_required);
	OutputToConsole(player_ptr, "ID   Name                     Votes\n");
	OutputToConsole(player_ptr, "-----------------------------------\n");

	for (int i = 1; i <= max_players; i++)
	{
		player_t	server_player;

		// Don't show votes against player who ran it
		if (player_ptr->index == i) continue;

		server_player.index = i;
		if (!FindPlayerByIndex(&server_player)) continue;
		if (server_player.is_bot) continue;

		OutputToConsole(player_ptr, "%-5i%-26s%i\n", server_player.user_id, server_player.name, user_vote_list[i-1].kick_votes);
	}

	OutputToConsole(player_ptr, "\nTo vote to kick a player, type votekick <id> or votekick <player name or part of their name>\n");
	OutputToConsole(player_ptr, "e.g votekick 3, votekick Mani\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMaUserVoteKick(player_t *player_ptr, int argc, const char *kick_id)
{
	int votes_required;
	int admin_index = -1;

	if (mani_vote_user_vote_kick_mode.GetInt() == 0)
	{
		bool found_admin = false;

		for (int i = 1; i < max_players; i ++)
		{
			player_t	admin_player;

			admin_player.index = i;
			if (!FindPlayerByIndex(&admin_player)) continue;
			if (admin_player.is_bot) continue;

			if (!gpManiClient->IsAdmin(&admin_player, &admin_index)) continue;
			if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_KICK))
			{
				found_admin = true;
				break;
			}
		}

		if (found_admin)
		{
			SayToPlayer(ORANGE_CHAT, player_ptr, "You can not vote kick when admin is on the server");
			return;
		}
	}
		
	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserKickVotes(player_ptr, votes_required);
		return;
	}

	if (!CanWeUserVoteKickYet(player_ptr)) return;
	if (!CanWeUserVoteKickAgainYet(player_ptr)) return;

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, (char *) kick_id, IMMUNITY_ALLOW_KICK))
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", kick_id));
		return;
	}

	player_t *target_player = &(target_player_list[0]);

	if (target_player->index == player_ptr->index)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "Voting yourself is not a good idea !!");
		return;
	}

	if (target_player->is_bot)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "Player %s is a bot, cannot perform command\n", target_player->name);
		return;
	}

	admin_index = -1;
	gpManiClient->IsAdmin(target_player, &admin_index);

	if (user_vote_list[player_ptr->index - 1].kick_id != -1 && admin_index == -1)
	{
		// Player already voted, alter previous vote if not admin
		player_t change_player;

		change_player.user_id = user_vote_list[player_ptr->index - 1].kick_id;
		if (FindPlayerByUserID(&change_player))
		{
			user_vote_list[change_player.index - 1].kick_votes --;
		}
	}

	if (admin_index == -1)
	{
		// Add votes if not admin
		user_vote_list[target_player->index - 1].kick_votes ++;
	}

	user_vote_list[player_ptr->index - 1].kick_id = target_player->user_id;
	user_vote_list[player_ptr->index - 1].kick_vote_timestamp = gpGlobals->curtime;

	int votes_left = votes_required - user_vote_list[target_player->index - 1].kick_votes;

	SayToAll(ORANGE_CHAT, false, "Player %s voted to kick %s, %i more vote%s required", 
								player_ptr->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the kick of the player
		ProcessUserVoteKickWin(target_player);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process a user vote kick win
//---------------------------------------------------------------------------------
void ManiVote::ProcessUserVoteKickWin(player_t *player_ptr)
{
	char	kick_cmd[256];

	PrintToClientConsole(player_ptr->entity, "You have been kicked by vote\n");
	Q_snprintf( kick_cmd, sizeof(kick_cmd), "kickid %i You were vote kicked\n", player_ptr->user_id);
	LogCommand (NULL, "User vote kick using %s", kick_cmd);
	engine->ServerCommand(kick_cmd);
	SayToAll(GREEN_CHAT, true, "Player %s has been kicked by user vote", player_ptr->name);

	user_vote_list[player_ptr->index - 1].kick_votes = 0;
	user_vote_list[player_ptr->index - 1].kick_id = -1;

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes for that player
		if (user_vote_list[i].kick_id == player_ptr->user_id) 
		{
			user_vote_list[i].kick_id = -1;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteKickYet( player_t *player_ptr )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_kick_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteKickAgainYet( player_t *player_ptr )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player_ptr->index - 1].kick_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Kick menu draw 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuUserVoteKick( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	int votes_required;

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *user_id = gpCmd->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteKick (player, 2, user_id);
		return;
	}
	else
	{
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteKickYet(player)) return;
		if (!CanWeUserVoteKickAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 1; i <= max_players; i++)
		{
			player_t server_player;

			if (player->index == i) continue;

			server_player.index = i;
			if (!FindPlayerByIndex(&server_player)) continue;
			if (server_player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", user_vote_list[i-1].kick_votes, server_player.name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotekickmenu %i", server_player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, server_player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_kick_percentage.GetFloat(), mani_vote_user_vote_kick_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), "%s", Translate(571,"%i", votes_required));

		// Draw menu list
		DrawSubMenu (player, Translate(570), votes_required_text, next_index, "mani_uservotekickmenu", "", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// Purpose: Vote ban code (practically a copy of the vote kick code :/
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void ManiVote::ShowCurrentUserBanVotes( player_t *player_ptr, int votes_required )
{

	OutputToConsole(player_ptr, "\n");
	OutputToConsole(player_ptr, "%s\n", mani_version);
	OutputToConsole(player_ptr, "\nVotes required for user ban is %i\n\n", votes_required);
	OutputToConsole(player_ptr, "ID   Name                     Votes\n");
	OutputToConsole(player_ptr, "-----------------------------------\n");

	for (int i = 1; i <= max_players; i++)
	{
		player_t	server_player;

		// Don't show votes against player who ran it
		if (player_ptr->index == i) continue;

		server_player.index = i;
		if (!FindPlayerByIndex(&server_player)) continue;
		if (server_player.is_bot) continue;

		OutputToConsole(player_ptr, "%-5i%-26s%i\n", server_player.user_id, server_player.name, user_vote_list[i-1].ban_votes);
	}

	OutputToConsole(player_ptr, "\nTo vote to ban a player, type voteban <id> or voteban <player name or part of their name>\n");
	OutputToConsole(player_ptr, "e.g voteban 3, voteban Mani\n\n");

	return;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMaUserVoteBan(player_t *player_ptr, int argc, const char *ban_id)
{
	int votes_required;
	int admin_index = -1;

	if (mani_vote_user_vote_ban_mode.GetInt() == 0)
	{
		bool found_admin = false;

		for (int i = 1; i < max_players; i ++)
		{
			player_t	admin_player;

			admin_player.index = i;
			if (!FindPlayerByIndex(&admin_player)) continue;
			if (admin_player.is_bot) continue;

			if (!gpManiClient->IsAdmin(&admin_player, &admin_index)) continue;
			if (gpManiClient->IsAdminAllowed(admin_index, ALLOW_BAN))
			{
				found_admin = true;
				break;
			}
		}

		if (found_admin)
		{
			SayToPlayer(ORANGE_CHAT, player_ptr, "You can not vote ban when admin is on the server");
			return;
		}
	}
		
	votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());

	if (argc == 1)
	{
		ShowCurrentUserBanVotes(player_ptr, votes_required);
		return;
	}

	if (!CanWeUserVoteBanYet(player_ptr)) return;
	if (!CanWeUserVoteBanAgainYet(player_ptr)) return;

	// Whoever issued the commmand is authorised to do it.
	if (!FindTargetPlayers(player_ptr, (char *) ban_id, IMMUNITY_ALLOW_BAN))
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "%s", Translate(M_NO_TARGET, "%s", ban_id));
		return;
	}

	player_t *target_player = &(target_player_list[0]);

	if (target_player->index == player_ptr->index)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "Voting yourself is not a good idea !!");
		return;
	}

	if (target_player->is_bot)
	{
		SayToPlayer(ORANGE_CHAT, player_ptr, "Player %s is a bot, cannot perform command\n", target_player->name);
		return;
	}

	admin_index = -1;
	gpManiClient->IsAdmin(target_player, &admin_index);

	if (user_vote_list[player_ptr->index - 1].ban_id != -1 && admin_index == -1)
	{
		// Player already voted, alter previous vote if not admin
		player_t change_player;

		change_player.user_id = user_vote_list[player_ptr->index - 1].ban_id;
		if (FindPlayerByUserID(&change_player))
		{
			user_vote_list[change_player.index - 1].ban_votes --;
		}
	}

	if (admin_index == -1)
	{
		// Add votes if not admin
		user_vote_list[target_player->index - 1].ban_votes ++;
	}

	user_vote_list[player_ptr->index - 1].ban_id = target_player->user_id;
	user_vote_list[player_ptr->index - 1].ban_vote_timestamp = gpGlobals->curtime;

	int votes_left = votes_required - user_vote_list[target_player->index - 1].ban_votes;

	SayToAll(ORANGE_CHAT, false, "Player %s voted to ban %s, %i more vote%s required", 
								player_ptr->name, 
								target_player->name,
								votes_left, 
								(votes_left == 1) ? " is":"s are");

	if (votes_left <= 0)
	{
		// Run the process to trigger the ban of the player
		ProcessUserVoteBanWin(target_player);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Process a user vote ban win
//---------------------------------------------------------------------------------
void ManiVote::ProcessUserVoteBanWin(player_t *player)
{
	char	ban_cmd[256];

	if (mani_vote_user_vote_ban_type.GetInt() == 0 && sv_lan->GetInt() != 1)
	{
		// Ban by user id
		Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
										mani_vote_user_vote_ban_time.GetInt(), 
										player->user_id);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeid\n");
	}
	else if (mani_vote_user_vote_ban_type.GetInt() == 1)
	{
		// Ban by user ip address
		Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
										mani_vote_user_vote_ban_time.GetInt(), 
										player->ip_address);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeip\n");
	}
	else if (mani_vote_user_vote_ban_type.GetInt() == 2)
	{
		// Ban by user id and ip address
		if (sv_lan->GetInt() == 0)
		{
			Q_snprintf( ban_cmd, sizeof(ban_cmd), "banid %i %i kick\n", 
								mani_vote_user_vote_ban_time.GetInt(), 
								player->user_id);
			LogCommand(NULL, "User vote banned using %s", ban_cmd);
			engine->ServerCommand(ban_cmd);
			engine->ServerCommand("writeid\n");
		}

		Q_snprintf( ban_cmd, sizeof(ban_cmd), "addip %i \"%s\"\n", 
								mani_vote_user_vote_ban_time.GetInt(), 
								player->ip_address);
		LogCommand(NULL, "User vote banned using %s", ban_cmd);
		engine->ServerCommand(ban_cmd);
		engine->ServerCommand("writeip\n");
	}

	PrintToClientConsole(player->entity, "You have been banned by vote\n");
	SayToAll(GREEN_CHAT, true, "Player %s has been banned by user vote", player->name);

	user_vote_list[player->index - 1].ban_votes = 0;
	user_vote_list[player->index - 1].ban_id = -1;

	for (int i = 0; i < max_players; i ++)
	{
		// Reset votes for that player
		if (user_vote_list[i].ban_id == player->user_id) 
		{
			user_vote_list[i].ban_id = -1;
		}
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote yet
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteBanYet( player_t *player )
{
	int time_left_before_vote = (int) (mani_vote_user_vote_ban_time_before_vote.GetFloat() - (gpGlobals->curtime - map_start_time));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "Voting not allowed for %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if we can vote again
//---------------------------------------------------------------------------------
bool ManiVote::CanWeUserVoteBanAgainYet( player_t *player )
{
	int time_left_before_vote = (int) (15 - (gpGlobals->curtime - user_vote_list[player->index - 1].ban_vote_timestamp));

	if (time_left_before_vote > 0)
	{
		SayToPlayer(ORANGE_CHAT, player, "You cannot vote again for another %i second%s", time_left_before_vote, (time_left_before_vote == 1) ? "":"s");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Handle User Vote Ban menu draw 
//---------------------------------------------------------------------------------
void ManiVote::ProcessMenuUserVoteBan( player_t *player, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();
	int votes_required;

	if (argc - argv_offset == 2)
	{
		// User voted by menu system, should be a map index
		const char *user_id = gpCmd->Cmd_Argv(1 + argv_offset);
		ProcessMaUserVoteBan (player, 2, user_id);
		return;
	}
	else
	{
		// Check and warn player if voting not allowed yet
		if (!CanWeUserVoteBanYet(player)) return;
		if (!CanWeUserVoteBanAgainYet(player)) return;

		// Setup map list that the user can vote for
		FreeMenu();

		for (int i = 1; i <= max_players; i++)
		{
			player_t server_player;

			if (player->index == i) continue;

			server_player.index = i;
			if (!FindPlayerByIndex(&server_player)) continue;
			if (server_player.is_bot) continue;

			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "[%i] %s", user_vote_list[i-1].ban_votes, server_player.name);
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "mani_uservotebanmenu %i", server_player.user_id);
			Q_strcpy (menu_list[menu_list_size - 1].sort_name, server_player.name);
		}

		if (menu_list_size == 0) return;

		SortMenu();

		// List size may have changed
		if (next_index > menu_list_size) next_index = 0;

		votes_required = GetVotesRequiredForUserVote(false, mani_vote_user_vote_ban_percentage.GetFloat(), mani_vote_user_vote_ban_minimum_votes.GetInt());

		char	votes_required_text[128];
		Q_snprintf( votes_required_text, sizeof(votes_required_text), "%s", Translate(581,"%i", votes_required));

		// Draw menu list
		DrawSubMenu (player, Translate(580), votes_required_text, next_index, "mani_uservotebanmenu", "", false,-1);
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Run the RCON vote list menu
//---------------------------------------------------------------------------------
void ManiVote::ProcessRConVote( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		int vote_rcon_index = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset));

		if (vote_rcon_index == 0) return;
		vote_rcon_index --;

		gpCmd->NewCmd();
		gpCmd->AddParam("ma_votercon");
		gpCmd->AddParam("%s", vote_rcon_list[vote_rcon_index].question);
		gpCmd->AddParam("%s", vote_rcon_list[vote_rcon_index].rcon_command);
		this->ProcessMaVoteRCon(admin, "ma_votercon", 0, false);
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 0; i < vote_rcon_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", vote_rcon_list[i].alias);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votercon %i", i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(240), Translate(241), next_index,"admin","votercon",true,-1);
	}


	return;
}

//---------------------------------------------------------------------------------
// Purpose: Run the Question vote list menu
//---------------------------------------------------------------------------------
void ManiVote::ProcessQuestionVote( player_t *admin, int next_index, int argv_offset )
{
	const int argc = gpCmd->Cmd_Argc();

	if (argc - argv_offset == 3)
	{
		int vote_question_index = Q_atoi(gpCmd->Cmd_Argv(2 + argv_offset));

		if (vote_question_index == 0) return;
		vote_question_index --;

		gpCmd->NewCmd();
		gpCmd->AddParam("ma_votequestion");
		gpCmd->AddParam("%s", vote_question_list[vote_question_index].question);
		this->ProcessMaVoteQuestion(admin, "ma_votequestion", 0, M_MENU);
		return;
	}
	else
	{

		// Setup player list
		FreeMenu();

		for( int i = 0; i < vote_question_list_size; i++ )
		{
			AddToList((void **) &menu_list, sizeof(menu_t), &menu_list_size); 
			Q_snprintf( menu_list[menu_list_size - 1].menu_text, sizeof(menu_list[menu_list_size - 1].menu_text), "%s", vote_question_list[i].alias);							
			Q_snprintf( menu_list[menu_list_size - 1].menu_command, sizeof(menu_list[menu_list_size - 1].menu_command), "admin votequestion %i", i + 1);
		}

		if (menu_list_size == 0)
		{
			return;
		}

		// List size may have changed
		if (next_index > menu_list_size)
		{
			// Reset index
			next_index = 0;
		}

		// Draw menu list
		DrawSubMenu (admin, Translate(250), Translate(251), next_index, "admin", "votequestion",true,-1);
	}


	return;
}

SCON_COMMAND(ma_voterandom, 2123, MaVoteRandom, false);
SCON_COMMAND(ma_voteextend, 2127, MaVoteExtend, false);
SCON_COMMAND(ma_votercon, 2129, MaVoteRCon, false);
SCON_COMMAND(ma_vote, 2131, MaVote, false);
SCON_COMMAND(ma_votequestion, 2133, MaVoteQuestion, false);
SCON_COMMAND(ma_votecancel, 2135, MaVoteCancel, false);

static int sort_nominations_by_votes_cast ( const void *m1,  const void *m2) 
{
	struct vote_option_t *mi1 = (struct vote_option_t *) m1;
	struct vote_option_t *mi2 = (struct vote_option_t *) m2;
	return (mi2->votes_cast - mi1->votes_cast);
}

static int sort_show_votes_cast ( const void *m1,  const void *m2) 
{
	struct show_vote_t *mi1 = (struct show_vote_t *) m1;
	struct show_vote_t *mi2 = (struct show_vote_t *) m2;

	if (mi1->votes_cast > mi2->votes_cast)
	{
		return -1;
	}
	else if (mi1->votes_cast < mi2->votes_cast)
	{
		return 1;
	}

	return strcmp(mi1->option, mi2->option);
}

ManiVote	g_ManiVote;
ManiVote	*gpManiVote;
