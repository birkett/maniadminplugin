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



#ifndef MANI_STATS_H
#define MANI_STATS_H

struct rank_t
{
	char	steam_id[50];
	char	name[64];
	int		kills;
	int		deaths;
	int		suicides;
	int		headshots;
	float	kd_ratio;
	time_t	last_connected;
	int		rank;
};

extern	void	LoadStats(void);
extern	void	FreeStats(void);
extern	void	AddPlayerToRankList(player_t *player);
extern	void	AddPlayerNameToRankList(player_t *player);
extern	void	CalculateStats(bool round_end);
extern	void	CalculateNameStats(bool round_end);
extern	void	ReBuildStatsPlayerList(void);
extern	void	ReBuildStatsPlayerNameList(void);
extern	void	ShowRank(player_t *player);
extern	void	ShowTop(player_t *player ,int number_of_ranks);
extern	void	ShowStatsMe(player_t *player);
extern	void	ProcessDeathStats(IGameEvent *event);
extern	void	ProcessNameDeathStats(IGameEvent *event);
extern	void	ResetStats(void);
extern	void	ReadStats(void);
extern	void	ReadNameStats(void);
extern	void	WriteStats(void);
extern	void	WriteNameStats(void);
extern	void	ReadStats(void);
extern	void	ReadNameStats(void);
extern	PLUGIN_RESULT	ProcessMaRanks( int index,  bool svr_command,  int argc,  char *command_string,  char *start_rank, char *end_rank);
extern	PLUGIN_RESULT	ProcessMaPLRanks( int index,  bool svr_command,  int argc,  char *command_string,  char *start_rank, char *end_rank);
extern	PLUGIN_RESULT	ProcessMaResetPlayerRank( int index,  bool svr_command,  int argc,  char *command_string,  char *player_steam_id);




#endif