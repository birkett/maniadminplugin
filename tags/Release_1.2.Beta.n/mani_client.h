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



#ifndef MANI_CLIENT_H
#define MANI_CLIENT_H

#define MAX_ADMIN_FLAGS (54)
#define MAX_IMMUNITY_FLAGS (29)

#define MANI_ADMIN_TYPE (0)
#define MANI_IMMUNITY_TYPE (1)

extern	bool	IsCommandIssuedByServerAdmin( void );

struct admin_level_t
{
	int		level_id;
	bool	flags[MAX_ADMIN_FLAGS];
};

struct immunity_level_t
{
	int		level_id;
	bool	flags[MAX_IMMUNITY_FLAGS];
};

struct player_level_t
{
	int		level_id;
	int		client_index;
};

class ManiMySQL;

class ManiClient
{

public:
	ManiClient();
	~ManiClient();

	bool		Init(void);

	// Update level masks
	void		NetworkIDValidated(player_t *player_ptr);
	// Update level masks
	void		ClientDisconnect(player_t *player_ptr);

	bool			IsAdmin(player_t *player_ptr, int *client_index);
	bool			IsImmune(player_t *player_ptr, int *client_index);
	bool			IsImmuneNoPlayer(player_t *player_ptr, int *client_index);
	bool			IsPotentialAdmin(player_t *player_ptr, int *client_index);
	bool			IsAdminAllowed(player_t *player, const char *command, int admin_flag, bool check_war, int *admin_index);
	inline	bool	IsAdminAllowed(int admin_index, int flag) const {return client_list[admin_index].admin_flags[flag];}
	inline	bool	IsImmunityAllowed(int immunity_index, int flag) const {return client_list[immunity_index].immunity_flags[flag];}
	PLUGIN_RESULT	ProcessMaSetAdminFlag(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaClient(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaClientGroup(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaReloadClients(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);

private:

	struct steam_t
	{
		char	steam_id[MAX_NETWORKID_LENGTH];
	};

	struct nick_t
	{
		char	nick[32];
	};

	struct ip_address_t
	{
		char	ip_address[128];
	};

	struct group_t
	{
		char	group_id[128];
	};

	struct client_t
	{
		char	email_address[128];
		ip_address_t	*ip_address_list;
		nick_t	*nick_list;
		steam_t	*steam_list;
		group_t	*admin_group_list;
		group_t	*immunity_group_list;
		int		ip_address_list_size;
		int		steam_list_size;
		int		nick_list_size;
		int		admin_group_list_size;
		int		immunity_group_list_size;
		char	name[128];
		char	password[128];
		int		user_id;
		int		admin_level_id;
		int		immunity_level_id;
		char	notes[256];
		bool	has_admin_potential;
		bool	has_immunity_potential;
		bool	admin_flags[MAX_ADMIN_FLAGS];
		bool	grouped_admin_flags[MAX_ADMIN_FLAGS];
		bool	original_admin_flags[MAX_ADMIN_FLAGS];

		bool	immunity_flags[MAX_IMMUNITY_FLAGS];
		bool	grouped_immunity_flags[MAX_IMMUNITY_FLAGS];
		bool	original_immunity_flags[MAX_IMMUNITY_FLAGS];

//		bool	admin_exclude_flag;
//		bool	immunity_exclude_flag;
	};

	struct old_style_client_t
	{
		char	steam_id[MAX_NETWORKID_LENGTH];
		char	ip_address[128];
		char	name[128];
		char	password[128];
		char	group_id[128];
		bool	flags[MAX_ADMIN_FLAGS + MAX_IMMUNITY_FLAGS];
	};

	struct admin_group_t
	{
		char	group_id[128];
		bool	flags[MAX_ADMIN_FLAGS];
	};

	struct immunity_group_t
	{
		char	group_id[128];
		bool	flags[MAX_IMMUNITY_FLAGS];
	};

	struct flag_t
	{
		char	flag[20];
		char	flag_desc[64];
	};

	void		DefaultValues(void);
	bool		FindBaseKey(KeyValues *kv, KeyValues *base_key_ptr);
	void		InitAdminFlags(void);
	void		InitImmunityFlags(void);
	void		FreeClients(void);
	void		FreeClient(client_t *client_ptr);
	bool		OldAddClient( char *file_details, old_style_client_t *client_ptr, bool	is_admin);
	void		OldAddAdminGroup(char *admin_details);
	void		OldAddImmunityGroup(char *immunity_details);
	void		ConvertOldClientToNewClient(old_style_client_t	*old_client_ptr,bool	is_admin);
	bool		LoadOldStyle(void);
	void		LoadClients(void);
	void		GetAdminGroups(KeyValues *ptr);
	void		GetImmunityGroups(KeyValues *ptr);
	void		GetAdminLevels(KeyValues *ptr);
	void		GetImmunityLevels(KeyValues *ptr);
	void		GetClients(KeyValues *ptr);
	int			GetNextFlag(char *flags_ptr, int *index, int type);
	void		WriteClients(void);
	void		DumpClientsToConsole(void);
	void		ComputeAdminLevels(void);
	void		ComputeImmunityLevels(void);
	bool		IsAdminCore( player_t *player_ptr, int *client_index, bool recursive);
	bool		IsImmuneCore( player_t *player_ptr, int *client_index, bool recursive);
	bool		IsPotentialImmune(player_t *player_ptr, int *client_index);
	int			FindClientIndex( char *target_string, bool check_if_admin,  bool	check_if_immune);
	bool		CreateDBTables(void);
	bool		CreateDBFlags(void);
	bool		ExportDataToDB(void);
	bool 		GetClientsFromDatabase(void);
	void		ProcessAddClient( player_t *player_ptr,  char *param1);
	void		ProcessAddSteam( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddIP( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddNick( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetName( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetPassword( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetEmail( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetNotes( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddGroup( int type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetLevel( int type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetFlag( int type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveClient( player_t *player_ptr,  char *param1);
 	void		ProcessRemoveSteam( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveIP( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveNick( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveGroup( int type, player_t *player_ptr,  char *param1, char *param2);
	void		RebuildFlags(client_t *client_ptr, int type);
	void		ProcessClientStatus( player_t *player_ptr,  char *param1 );
	void		ProcessClientUpload( player_t *player_ptr);
	void		ProcessClientDownload( player_t *player_ptr);
	void		ProcessAllClientStatus( player_t *player_ptr);

	// Group based commands
	void		ProcessAddGroupType( int type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddLevelType( int type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveGroupType( int type, player_t *player_ptr,  char *param1);
	void		ProcessRemoveLevelType( int type, player_t *player_ptr,  char *param1);
	void		ProcessClientGroupStatus( player_t *player_ptr,  char *param1 );
	void		ProcessClientFlagDesc( int type, player_t *player_ptr,  char *param1);
	void		ProcessAllClientFlagDesc( int type, player_t *player_ptr);

	// V1.2BetaM upgrade functions
	void		UpgradeDB1( void );
	bool		UpgradeServerIDToServerGroupID( ManiMySQL *mani_mysql_ptr, const char	*table_name );
	bool		UpgradeClassTypes( ManiMySQL *mani_mysql_ptr, const char	*table_name );
	bool		TestColumnExists(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, bool *found_column);
	bool		TestColumnType(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, char *column_type, bool *column_matches);

	client_t		*client_list;
	int				client_list_size;
	
	admin_group_t	*admin_group_list;
	int				admin_group_list_size;
	
	immunity_group_t	*immunity_group_list;
	int				immunity_group_list_size;

	admin_level_t	*admin_level_list;
	int				admin_level_list_size;

	immunity_level_t	*immunity_level_list;
	int				immunity_level_list_size;

	flag_t		admin_flag_list[MAX_ADMIN_FLAGS];
	flag_t		immunity_flag_list[MAX_IMMUNITY_FLAGS];

	int			active_admin_list[MANI_MAX_PLAYERS];
	int			active_immunity_list[MANI_MAX_PLAYERS];
};

extern	ManiClient *gpManiClient;

#endif
