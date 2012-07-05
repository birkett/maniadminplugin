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

#include "const.h"

struct player_t;

struct ban_settings_t {
	char	key_id[MAX_NETWORKID_LENGTH]; // steamid or IP
	bool	byID; // this makes it easier when reading the settings to know if it's steam or IP
	time_t	expire_time; // note not the length of time to ban for, the actual time it expires
	char	ban_initiator[MAX_PLAYER_NAME_LENGTH];
	char	player_name[MAX_PLAYER_NAME_LENGTH];
	char	reason[256];
};

extern	ban_settings_t *ban_list;
extern	int				ban_list_size;

class CManiHandleBans {
public:
	void	LevelInit(void);

	void	WriteBans(void);
	bool	AddBan ( ban_settings_t *ban );
	bool	AddBan ( player_t *player, const char *key, const char *initiator, int ban_time, const char *prefix, const char *reason );
	bool	RemoveBan( const char *key );
	void	ReadBans(void);
	void	GameFrame(void);

	bool	DoneProcessing(void) { return current_index == starting_count; }
private:
	void	SendBanToServer(int ban_index);

	int		current_index;
	int		starting_count;
};

extern CManiHandleBans *gpManiHandleBans;