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

#define ADMIN ("Admin")
#define IMMUNITY ("Immunity")

#include <vector>
#include "mani_player.h"
#include "mani_client_util.h"

extern	bool	IsCommandIssuedByServerAdmin( void );

class ManiMySQL;
class ManiKeyValues;
class LevelList;
class GroupList;

struct read_t;

class ClientPlayer
{
	MENUFRIEND_DEC(LevelClient);
	MENUFRIEND_DEC(GroupClient);
	MENUFRIEND_DEC(SelectClient);
	MENUFRIEND_DEC(SetEmail);
	MENUFRIEND_DEC(SetNotes);
	MENUFRIEND_DEC(SetName);
	MENUFRIEND_DEC(SetPassword);
	MENUFRIEND_DEC(ClientUpdateOption);
	MENUFRIEND_DEC(PlayerSteam);
	MENUFRIEND_DEC(PlayerIP);
	MENUFRIEND_DEC(PlayerNick);
	MENUFRIEND_DEC(SetSteam);
	MENUFRIEND_DEC(SetIP);
	MENUFRIEND_DEC(SetNick);
	MENUFRIEND_DEC(RemoveSteam);
	MENUFRIEND_DEC(RemoveIP);
	MENUFRIEND_DEC(RemoveNick);
	MENUFRIEND_DEC(SetPersonalFlag);
	MENUFRIEND_DEC(NewName);
	MENUFRIEND_DEC(NameOnServer);
	MENUFRIEND_DEC(SteamOnServer);
	MENUFRIEND_DEC(IPOnServer);

public:
	ClientPlayer(){};
	~ClientPlayer(){};

	void SetEmailAddress(const char *str_ptr) {email_address.Set(str_ptr);}
	void SetName(const char *str_ptr) {name.Set(str_ptr);}
	void SetPassword(const char *str_ptr) {password.Set(str_ptr);}
	void SetNotes(const char *str_ptr) {notes.Set(str_ptr);}
	void SetUserID(const int user_id) {this->user_id = user_id;}
	const char *GetEmailAddress() {return email_address.str;}
	const char *GetName() {return name.str;}
	const char *GetPassword() {return password.str;}
	const char *GetNotes() {return notes.str;}
	const int	GetUserID() {return user_id;}
	
	StringSet	ip_address_list;
	StringSet	nick_list;
	StringSet	steam_list;

	// Personal flag storage here
	PersonalFlag	personal_flag_list;

	// Summed access flags including grouped
	PersonalFlag	unmasked_list;

	// Summed access flags after applying level masks
	// Computed using applicable level id and unmasked_list above
	PersonalFlag	masked_list;

	// Groups that this client belongs to
	GroupSet		group_list;
	LevelSet		level_list;

private:
	BasicStr	email_address;
	BasicStr	name;
	BasicStr	password;
	int			user_id;
	BasicStr	notes;
};

class ManiClient
{
	MENUFRIEND_DEC(GroupClassType);
	MENUFRIEND_DEC(GroupUpdate);	
	MENUFRIEND_DEC(LevelClassType);
	MENUFRIEND_DEC(LevelUpdate);
	MENUFRIEND_DEC(CreateLevel);
	MENUFRIEND_DEC(CreateGroup);
	MENUFRIEND_DEC(LevelRemove);
	MENUFRIEND_DEC(GroupRemove);
	MENUFRIEND_DEC(LevelClient);
	MENUFRIEND_DEC(GroupClient);
	MENUFRIEND_DEC(SelectClient);
	MENUFRIEND_DEC(SetEmail);
	MENUFRIEND_DEC(SetName);
	MENUFRIEND_DEC(SetNotes);
	MENUFRIEND_DEC(SetPassword);
	MENUFRIEND_DEC(ClientUpdateOption);
	MENUFRIEND_DEC(PlayerName);
	MENUFRIEND_DEC(PlayerSteam);
	MENUFRIEND_DEC(PlayerIP);
	MENUFRIEND_DEC(PlayerNick);
	MENUFRIEND_DEC(SetSteam);
	MENUFRIEND_DEC(SetIP);
	MENUFRIEND_DEC(SetNick);
	MENUFRIEND_DEC(RemoveSteam);
	MENUFRIEND_DEC(RemoveIP);
	MENUFRIEND_DEC(RemoveNick);
	MENUFRIEND_DEC(SetPersonalFlag);
	MENUFRIEND_DEC(NewName);
	MENUFRIEND_DEC(NameOnServer);
	MENUFRIEND_DEC(SteamOnServer);
	MENUFRIEND_DEC(IPOnServer);

public:
	ManiClient();
	~ManiClient();

	bool		Init(void);

	// Update level masks
	void		NetworkIDValidated(player_t *player_ptr);
	// Update level masks
	void		ClientDisconnect(player_t *player_ptr);

	bool			IsClient(int id);
	PLUGIN_RESULT	ProcessMaSetFlag(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaClient(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaClientGroup(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	PLUGIN_RESULT	ProcessMaReloadClients(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type);
	const char		*FindClientName(player_t *player_ptr);

	// New accessors
	bool			HasAccess(player_t *player_ptr, const char *class_type, const char *flag_name, bool check_war = false, bool check_unmasked_only = false);
	bool			HasAccess(int player_index, const char *class_type, const char *flag_name, bool check_war = false, bool check_unmasked_only = false);
	bool			AddFlagDesc(const char *class_name, const char *flag_name, const char *description, bool	replace_description = true );
	void			UpdateClientUserID(const int user_id, const char *name);

private:

	/******* structs required for V1.1 upgrade to new format ******/
	struct old_flag_t
	{
		bool	enabled;
		char	flag_name[16];
	};

	struct old_style_client_t
	{
		char		steam_id[MAX_NETWORKID_LENGTH];
		char		ip_address[128];
		char		name[128];
		char		password[128];
		char		group_id[128];
		old_flag_t	flags[MAX_ADMIN_FLAGS + MAX_IMMUNITY_FLAGS];
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

	// These two used for port of old flags
	flag_t		admin_flag_list[MAX_ADMIN_FLAGS];
	flag_t		immunity_flag_list[MAX_IMMUNITY_FLAGS];

	/************* End of structs for V1.1 upgrade ***************/

	void		DefaultValues(void);
	bool		FindBaseKey(KeyValues *kv, KeyValues *base_key_ptr);
	void		InitAdminFlags(void);
	void		InitImmunityFlags(void);
	void		FreeClients(void);
	void		SetupPlayersOnServer(void);
	bool		OldAddClient( char *file_details, old_style_client_t *client_ptr, bool is_admin);
	void		OldAddGroup(char *admin_details, char *class_type);
	void		ConvertOldClientToNewClient(old_style_client_t	*old_client_ptr, bool is_admin);
	bool		LoadOldStyle(void);
	bool		CreateDBTables(player_t *player_ptr);
	bool		CreateDBFlags(player_t *player_ptr);
	bool		ExportDataToDB(player_t *player_ptr);
	bool		UploadServerID(player_t *player_ptr);
	bool 		GetClientsFromDatabase(player_t *player_ptr);
	void		ProcessAddClient( player_t *player_ptr,  char *param1);
	void		ProcessAddSteam( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddIP( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddNick( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetName( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetPassword( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetEmail( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetNotes( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddGroup( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetLevel( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessSetFlag( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveClient( player_t *player_ptr,  char *param1);
 	void		ProcessRemoveSteam( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveIP( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveNick( player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveGroup( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessClientStatus( player_t *player_ptr,  char *param1 );
	void		ProcessClientUpload( player_t *player_ptr);
	void		ProcessClientDownload( player_t *player_ptr);
	void		ProcessAllClientStatus( player_t *player_ptr);
	void		ProcessClientServerID(player_t *player_ptr);

	// Group based commands
	void		ProcessAddGroupType( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessAddLevelType( const char *class_type, player_t *player_ptr,  char *param1, char *param2);
	void		ProcessRemoveGroupType( const char *class_type, player_t *player_ptr,  char *param1);
	void		ProcessRemoveLevelType( const char *class_type, player_t *player_ptr,  char *param1);
	void		ProcessClientGroupStatus( player_t *player_ptr,  char *param1 );
	void		ProcessClientFlagDesc( const char *class_type, player_t *player_ptr,  char *param1);
	void		ProcessAllClientFlagDesc( const char *class_type, player_t *player_ptr);

	// V1.2BetaM upgrade functions
	void		UpgradeDB1( void );
	bool		UpgradeServerIDToServerGroupID( ManiMySQL *mani_mysql_ptr, const char	*table_name );
	bool		UpgradeClassTypes( ManiMySQL *mani_mysql_ptr, const char	*table_name );
	bool		TestColumnExists(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, bool *found_column);
	bool		TestColumnType(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, char *column_type, bool *column_matches);

	void		AddBuiltInFlags(void);
	void		SetupUnMasked(void);
	void		SetupMasked(void);
	void		WriteClients(void);
	void		LoadClients(void);
	char		*SplitFlagString(char *flags_ptr, int *index);
	void		ReadGroups(ManiKeyValues *kv_ptr, read_t *read_ptr, bool group_type);
	void		ReadPlayers(ManiKeyValues *kv_ptr, read_t *read_ptr);

	void		LoadClientsBeta(void);
	void		GetAdminGroupsBeta(KeyValues *ptr);
	void		GetImmunityGroupsBeta(KeyValues *ptr);
	void		GetAdminLevelsBeta(KeyValues *ptr);
	void		GetImmunityLevelsBeta(KeyValues *ptr);
	void		GetClientsBeta(KeyValues *ptr);
	int			FindClientIndex( player_t *player_ptr );
	int			FindClientIndex( char *target_string );


	// List of available global groups
	GroupList	group_list;

	// List of available global levels
	LevelList	level_list;

	// Stores ptrs to client records for players actually on the server, 
	// this allows fast lookups by index
	ClientPlayer	*active_client_list[64];

	// Stores all clients in a list
	std::vector<ClientPlayer *> c_list;
	FlagDescList	flag_desc_list;
};

struct	mask_level_t
{
	ClientPlayer	*client_ptr;
	char	class_type[32];
	int		level_id;
};

MENUALL_DEC(ClientOrGroup);
MENUALL_DEC(GroupOption);
MENUALL_DEC(LevelOption);
MENUALL_DEC(GroupClassType);
MENUALL_DEC(GroupUpdate);
MENUALL_DEC(LevelClassType);
MENUALL_DEC(LevelUpdate);
MENUALL_DEC(ChooseClassType);
MENUALL_DEC(CreateLevel);
MENUALL_DEC(CreateGroup);
MENUALL_DEC(LevelRemove);
MENUALL_DEC(GroupRemove);
MENUALL_DEC(LevelClient);
MENUALL_DEC(GroupClient);
MENUALL_DEC(ClientOption);
MENUALL_DEC(SelectClient);
MENUALL_DEC(ClientUpdateOption);
MENUALL_DEC(SetEmail);
MENUALL_DEC(SetPassword);
MENUALL_DEC(SetNotes);
MENUALL_DEC(SetName);
MENUALL_DEC(ClientName);
MENUALL_DEC(ClientSteam);
MENUALL_DEC(ClientIP);
MENUALL_DEC(ClientNick);
MENUALL_DEC(PlayerName);
MENUALL_DEC(PlayerSteam);
MENUALL_DEC(PlayerIP);
MENUALL_DEC(PlayerNick);
MENUALL_DEC(SetSteam);
MENUALL_DEC(SetIP);
MENUALL_DEC(SetNick);
MENUALL_DEC(RemoveSteam);
MENUALL_DEC(RemoveIP);
MENUALL_DEC(RemoveNick);
MENUALL_DEC(SetPersonalClass);
MENUALL_DEC(SetPersonalFlag);
MENUALL_DEC(AddClientOption);
MENUALL_DEC(NewName);
MENUALL_DEC(NameOnServer);
MENUALL_DEC(SteamOnServer);
MENUALL_DEC(IPOnServer);

extern	ManiClient *gpManiClient;

#endif
