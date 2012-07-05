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



#ifndef MANI_VOTE_H
#define MANI_VOTE_H

#include "mani_player.h"
#include "mani_maps.h"
#include "mani_menu.h"

struct vote_option_t
{
	char	vote_name[512];
	char	vote_command[512];
	bool	null_command;
	int		votes_cast;
};

struct show_vote_t
{
	char	option[512];
	int		votes_cast;
	int		original_vote_list_index;
};

class ManiVote
{
	MENUFRIEND_DEC(SystemVotemap);
	MENUFRIEND_DEC(UserVoteMap);
	MENUFRIEND_DEC(RockTheVoteNominateMap);
	MENUFRIEND_DEC(UserVoteKick);
	MENUFRIEND_DEC(UserVoteBan);
	MENUFRIEND_DEC(RConVote);
	MENUFRIEND_DEC(QuestionVote);

public:
	ManiVote();
	~ManiVote();

	void		NetworkIDValidated(player_t	*player_ptr);
	void		ClientDisconnect(player_t *player_ptr);
	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		GameCommencing(void);
	void		GameFrame(void);
	void		RoundEnd(void);
	bool		SysVoteInProgress(void)	{return	system_vote.vote_in_progress;}
	bool		SysMapDecided(void)	{return	system_vote.map_decided;}
	void		SysSetMapDecided(bool decided) {system_vote.map_decided	= decided;}
	bool		CanVote(player_t *player_ptr = NULL);

	PLUGIN_RESULT	ProcessMaVoteRandom(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaVoteExtend(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaVote(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaVoteQuestion(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaVoteRCon(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);
	PLUGIN_RESULT	ProcessMaVoteCancel(player_t *player_ptr, const char *command_name, const int help_id, const int command_type);

	void			ProcessMaUserVoteBan(player_t *player, int argc, const char	*user_id);
	void			ProcessUserVoteBanWin(player_t *player);
	void			ProcessMaRockTheVoteNominateMap(player_t *player, int argc,	const char *map_id);
	void			ProcessMaRockTheVote(player_t *player);
	void			ProcessMaUserVoteKick(player_t *player,	int	argc, const	char *user_id);
	void			ProcessMaUserVoteMap(player_t *player_ptr, int argc, const char *map_id);
	void			ProcessBuildUserVoteMaps(void);
	void			ProcessVoteConfirmation	(player_t *player, bool	accept);
	void			ProcessMenuSystemVoteMultiMap( player_t	*admin, const char *delay_type_string);

private:

	struct system_vote_t
	{
		bool	vote_in_progress;	// Flag	if vote	is in progress
		int		vote_starter;		// Player index	who	started	vote
		bool	vote_confirmation;	// Flag	if vote	confirmation required
		int		vote_type;			// Type	of vote	in progress
		int		votes_required;		// Number of votes required	for	early result
		float	end_vote_time;		// end vote	time when results are collated
		bool	waiting_decision;	// Used	for	Admin refuse/accept
		float	waiting_decision_time;	// Used	for	Admin refuse timeout
		int		delay_action;		// Delay action	of vote
		char	vote_title[512];	// Name	of vote	title
		bool	map_decided;		// Stops end of	map	vote overriding	an already selected	map
		int		winner_index;		// Index of	the	winning	vote option
		bool	start_rock_the_vote;	// Flag	to start rock the vote
		bool	no_more_rock_the_vote;	// Flag	to stop	more rock the votes
		int		max_votes;			//	Max	players	who	can	vote
		int		number_of_extends;	// Max number of extends allowed
		int		votes_so_far;		// Used by show vote progress
		int		total_votes_needed; // Used by show vote progress
		int		split_winner;		// More than one option one.
	};

	struct voter_t
	{
		bool	allowed_to_vote;
		bool	voted;
		int		vote_option_index;
	};

	struct user_vote_t
	{
		int		map_index;			// Map that	user voted for
		float	map_vote_timestamp;	
		bool	rock_the_vote;		// User	typed rock the vote
		float	nominate_timestamp;
		int		nominated_map;		// User	nominated map index
		char	kick_id[MAX_NETWORKID_LENGTH];			// Steam ID
		float	kick_vote_timestamp;
		int		kick_votes;			// Number of votes against player
		char	ban_id[MAX_NETWORKID_LENGTH];				// User	ID of player to	ban
		float	ban_vote_timestamp;
		int		ban_votes;			// Number of votes against player
	};

	struct user_vote_map_t
	{
		char	map_name[128];
	};

	struct	vote_rcon_t
	{
		char	rcon_command[512];
		char	question[512];
		char	alias[512];
	};

	struct	vote_question_t
	{
		char	question[512];
		char	alias[512];
	};

/*	struct map_vote_t
	{
		int	user_id;
		bool	slot_in_use;
		int	map_index;
		float	vote_time_stamp;
	};
*/
	void			StartSystemVote	(void);
	void			ProcessVotes (void);
	void			ProcessVoteWin (int	win_index);
	void			ProcessMapWin (int win_index);
	void			ProcessQuestionWin (int	win_index);
	void			ProcessExtendWin (int win_index);
	void			ProcessRConWin (int	win_index);
	bool			IsYesNoVote	(void);
	void			BuildRandomMapVote(	int	max_maps);
	void			ProcessPlayerVoted(	player_t *player, int vote_index);
	void			ShowCurrentUserMapVotes( player_t *player, int votes_required );
	void			ProcessUserVoteMapWin(int map_index);
	bool			CanWeUserVoteMapYet( player_t *player );
	bool			CanWeUserVoteMapAgainYet( player_t *player );
	int				GetVotesRequiredForUserVote( bool player_leaving, float	percentage,	int	minimum_votes );
	void			ShowCurrentUserKickVotes( player_t *player,	int	votes_required );
	void			ShowCurrentRockTheVoteMaps(	player_t *player);
	void			ProcessRockTheVoteWin (int win_index);
	bool			CanWeRockTheVoteYet(player_t *player);
	bool			CanWeNominateAgainYet( player_t	*player	);
	void			BuildRockTheVoteMapVote	(void);
	void			ProcessStartRockTheVote(void);
	void			ProcessUserVoteKickWin(player_t	*player);
	bool			CanWeUserVoteKickYet( player_t *player );
	bool			CanWeUserVoteKickAgainYet( player_t	*player	);
	void			ShowCurrentUserBanVotes( player_t *player, int votes_required );
	bool			CanWeUserVoteBanYet( player_t *player );
	bool			CanWeUserVoteBanAgainYet( player_t *player );
	bool			AddMapToVote(player_t *player_ptr, const char	*map_name);
	bool			AddQuestionToVote(const	char *answer);
	void			LoadConfig(void);
	void			BuildCurrentVoteLeaders (void);
	char			*GetCompleteVoteProgress (void);

	float			map_start_time;

	system_vote_t	system_vote;
	
	voter_t			voter_list[MANI_MAX_PLAYERS];
	
	vote_option_t	*vote_option_list;
	int				vote_option_list_size;
	
	map_t			*user_vote_map_list;
	int				user_vote_map_list_size;
	
	vote_rcon_t		*vote_rcon_list;
	int				vote_rcon_list_size;

	vote_question_t	*vote_question_list;
	int				vote_question_list_size;

	//map_vote_t		map_vote[MANI_MAX_PLAYERS];

	user_vote_t		user_vote_list[MANI_MAX_PLAYERS];

	char	show_hint_results[256];	   // Sent to hint message
	char	vote_progress_string[256]; // Header
	float	show_hint_next;
};

extern	ManiVote *gpManiVote;

MENUALL_DEC(SystemVoteRandomMap);
MENUALL_DEC(SystemVoteSingleMap);
MENUALL_DEC(SystemVoteBuildMap);
MENUALL_DEC(SystemAcceptVote);
MENUALL_DEC(SystemVotemap);
MENUALL_DEC(UserVoteMap);
MENUALL_DEC(RockTheVoteNominateMap);
MENUALL_DEC(UserVoteKick);
MENUALL_DEC(UserVoteBan);
MENUALL_DEC(RConVote);
MENUALL_DEC(QuestionVote);


#endif
