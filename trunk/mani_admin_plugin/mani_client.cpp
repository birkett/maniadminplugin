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
#include "inetchannelinfo.h"
#include "mani_client_flags.h"
#include "mani_memory.h"
#include "mani_convar.h"
#include "mani_player.h"
#include "mani_output.h"
#include "mani_parser.h"
#include "mani_main.h"
#include "mani_mysql.h"
#include "mani_database.h"
#include "mani_language.h"
#include "mani_keyvalues.h"
#include "mani_commands.h"
#include "mani_util.h"
#include "mani_help.h"
#include "mani_client_util.h"
#include "mani_client.h"
#include "mani_client_sql.h"

#include <map>

#ifdef SOURCEMM
#include "mani_client_interface.h"
#endif

extern IFileSystem	*filesystem;
extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern	IPlayerInfoManager *playerinfomanager;
extern	int		con_command_index;
extern	bool	war_mode;
extern	int	max_players;

#ifdef SOURCEMM
extern unsigned int g_CallBackCount;
extern SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
#endif

static int sort_mask_list ( const void *m1,  const void *m2);

static int sort_mask_list ( const void *m1,  const void *m2) 
{
	struct mask_level_t *mi1 = (struct mask_level_t *) m1;
	struct mask_level_t *mi2 = (struct mask_level_t *) m2;

	int result = strcmp(mi1->class_type, mi2->class_type);
	if (result != 0) return result;
	return mi1->level_id - mi2->level_id;
}


inline bool FStruEq(const char *sz1, const char *sz2)
{
	return(Q_strcmp(sz1, sz2) == 0);
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}


ManiClient::ManiClient()
{
	// Init
	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		active_client_list[i] = NULL;
	}

	this->InitAdminFlags();
	this->InitImmunityFlags();
	gpManiClient = this;
}

ManiClient::~ManiClient()
{
	// Cleanup
	FreeClients();
}

//---------------------------------------------------------------------------------
// Purpose: Player active on server
//---------------------------------------------------------------------------------
void	ManiClient::NetworkIDValidated(player_t *player_ptr)
{
	int	index;

	index = this->FindClientIndex(player_ptr);
	if (index == -1)
	{
		active_client_list[player_ptr->index - 1] = NULL;
	}
	else
	{
		active_client_list[player_ptr->index - 1] = c_list[index];

		// Check levels to see if masking needs to be applied to other players
		if (!c_list[index]->level_list.IsEmpty())
		{
			this->SetupMasked();
		}

#ifdef SOURCEMM
		for(unsigned int i=0;i<g_CallBackCount;i++)
		{
			AdminInterfaceListner *ptr = (AdminInterfaceListner *)g_CallBackList[i]->ptr;
			if(!ptr)
				continue;

			ptr->Client_Authorized(player_ptr->index);
		}		
#endif
	}

	return;
}

//---------------------------------------------------------------------------------
// Purpose: Check if client is officially labelled as admin
//---------------------------------------------------------------------------------
void	ManiClient::ClientDisconnect(player_t *player_ptr)
{

	if (active_client_list[player_ptr->index - 1] != NULL)
	{
		// Check if they had any level ids
		if (!active_client_list[player_ptr->index - 1]->level_list.IsEmpty())
		{
			// Reset pointer then call SetupMasks to reset other
			// active player masks
			active_client_list[player_ptr->index - 1] = NULL;
			this->SetupMasked();
		}
		else
		{
			active_client_list[player_ptr->index - 1] = NULL;
		}
	}

	return;
}

void ManiClient::LevelShutdown() {
	WriteIPList();
}
//---------------------------------------------------------------------------------
// Purpose: Parses the Admin config line setting flags
//---------------------------------------------------------------------------------

bool ManiClient::OldAddClient
(
 char *file_details,
 old_style_client_t	*client_ptr,
 bool	is_admin
 )
{
	char	steam_id[MAX_NETWORKID_LENGTH]="";
	char	ip_address[128]="";
	char	name[MAX_PLAYER_NAME_LENGTH]="";
	char	password[128]="";
	int	i,j;

	Q_memset(client_ptr, 0, sizeof(old_style_client_t));

	// Setup flags for individual access
	if (is_admin)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i ++)
		{
			Q_strcpy(client_ptr->flags[i].flag_name, admin_flag_list[i].flag);
			if (mani_reverse_admin_flags.GetInt() == 1)
			{
				client_ptr->flags[i].enabled = false;
			}
			else
			{
				client_ptr->flags[i].enabled = true;
			}
		}

		client_ptr->flags[ALLOW_CLIENT_ADMIN].enabled = true;
	}
	else
	{ 
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i ++)
		{
			Q_strcpy(client_ptr->flags[i].flag_name, immunity_flag_list[i].flag);

			if (mani_reverse_immunity_flags.GetInt() == 1)
			{
				client_ptr->flags[i].enabled = true;
			}
			else
			{
				client_ptr->flags[i].enabled = false;
			}
		}

		client_ptr->flags[IMMUNITY_ALLOW_BASIC_IMMUNITY].enabled = true;
	}

	i = 0;

	if (file_details[i] != ';')
	{
		j = 0;
		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				steam_id[j] = '\0';
				Q_strcpy(client_ptr->steam_id, steam_id);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == '\t' ||
				file_details[i] == ';')
			{
				steam_id[j] = '\0';
				break;
			}

			steam_id[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->steam_id, steam_id);
	}

	MMsg("%s ", steam_id);

	if (file_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				ip_address[j] = '\0';
				Q_strcpy(client_ptr->ip_address, ip_address);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == ';' ||
				file_details[i] == '\t')
			{
				ip_address[j] = '\0';
				break;
			}

			ip_address[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->ip_address, ip_address);
	}

	MMsg("%s ", ip_address);

	if (file_details[i] == ';' && file_details[i + 1] == '\"')
	{
		i += 2;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				name[j] = '\0';
				Q_strcpy(client_ptr->name, name);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == '"')
			{
				i++;
				name[j] = '\0';
				break;
			}

			name[j] = file_details[i];
			i++;
			j++;
		}

		Q_strcpy(client_ptr->name, name);
	}

	MMsg("%s ", name);

	if (file_details[i] == ';')
	{
		i++;
		j = 0;

		for (;;)
		{
			if (file_details[i] == '\0')
			{
				// No more data
				password[j] = '\0';
				if (!client_ptr->steam_id && !client_ptr->ip_address && !client_ptr->name)
				{
					return false;
				}

				Q_strcpy(client_ptr->password, password);
				return true;
			}

			// If reached space or tab break out of loop
			if (file_details[i] == ' ' ||
				file_details[i] == '\t')
			{
				password[j] = '\0';
				break;
			}

			password[j] = file_details[i];
			i++;
			j++;
		}

		if (!client_ptr->steam_id && !client_ptr->ip_address && !client_ptr->name)
		{
			return false;
		}

		Q_strcpy(client_ptr->password, password);
	}

	MMsg("%s ", password);

	i++;

	while (file_details[i] == ' ' || file_details[i] == '\t')
	{
		i++;
	}

	const char *flags_string = &(file_details[i]);

	if (is_admin)
	{
        if (group_list.Find(ADMIN, flags_string))
		{
			strcpy(client_ptr->group_id, flags_string);
			for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
			{
				client_ptr->flags[k].enabled = false;
			}
			
			return true;
		}
	}
	else
	{
        if (group_list.Find(IMMUNITY, flags_string))
		{
			strcpy(client_ptr->group_id, flags_string);
			for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
			{
				client_ptr->flags[k].enabled = false;
			}

			return true;
		}
	}

	while (file_details[i] != '\0')
	{
		if (is_admin)
		{
			for (int k = 0; k < MAX_ADMIN_FLAGS; k ++)
			{
				if (file_details[i] == admin_flag_list[k].flag[0])
				{
					if (mani_reverse_admin_flags.GetInt() == 1)
					{
						client_ptr->flags[k].enabled = true;
					}
					else
					{	
						client_ptr->flags[k].enabled = false;
					}

					break;
				}
			}
		}
		else
		{
			for (int k = 0; k < MAX_IMMUNITY_FLAGS; k ++)
			{
				if (file_details[i] == immunity_flag_list[k].flag[0])
				{
					if (mani_reverse_immunity_flags.GetInt() == 1)
					{
						client_ptr->flags[k].enabled = false;
					}
					else
					{	
						client_ptr->flags[k].enabled = true;
					}

					break;
				}
			}
		}

		i++;
	}


	MMsg("\n");
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------
void ManiClient::OldAddGroup(char *file_details, char *class_type)
{
	char	group_id[128]="";
	int	i,j;
	bool	reverse_flags = mani_reverse_admin_flags.GetBool();

	if (file_details[0] != '\"') return;

	i = 1;
	j = 0;

	for (;;)
	{
		if (file_details[i] == '\0')
		{
			// No more data
			group_id[j] = '\0';

			if (reverse_flags)
			{
				// No point adding group
				return;
			}

			GlobalGroupFlag *g_flag = group_list.AddGroup(class_type, group_id);
			const DualStrKey *key_value = NULL;
			for (const char *desc = flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = flag_desc_list.FindNext(class_type, &key_value))
			{
				g_flag->SetFlag(key_value->key2, true);
			}
			return;
		}


		// If reached end quote
		if (file_details[i] == '\"')
		{
			group_id[j] = '\0';
			break;
		}

		group_id[j] = file_details[i];
		i++;
		j++;
	}

	// Populate all the flags if needed
	if (!reverse_flags)
	{
		group_list.AddGroup(class_type, group_id);
		GlobalGroupFlag *g_flag = group_list.AddGroup(class_type, group_id);
		const DualStrKey *key_value = NULL;
		for (const char *desc = flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = flag_desc_list.FindNext(class_type, &key_value))
		{
			g_flag->SetFlag(key_value->key2, true);
		}
	}
			
	i++;

	while (file_details[i] != '\0')
	{
		char temp_string[8];

		snprintf(temp_string, sizeof(temp_string), "%c", file_details[i]);

		if (flag_desc_list.IsValidFlag(class_type, temp_string))
		{
			if (!reverse_flags)
			{
				GlobalGroupFlag *g_flag = group_list.AddGroup(class_type, group_id);
				if (g_flag)
				{
					g_flag->SetFlag(temp_string, false);
				}
			}
			else
			{
				GlobalGroupFlag *g_flag = group_list.AddGroup(class_type, group_id);
				if (g_flag)
				{
					g_flag->SetFlag(temp_string, true);
				}			
			}
		}

		i++;
	}
}


//---------------------------------------------------------------------------------
// Purpose: Parses the Admin Group config line setting flags
//---------------------------------------------------------------------------------
bool	ManiClient::Init(void)
{
	// Setup the flags
	flag_desc_list.LoadFlags();
	this->AddBuiltInFlags();

	FreeClients();
	if (LoadOldStyle())
	{
		// Loaded from old style adminlist.txt etc so write in new file format
		WriteClients();
		if (gpManiDatabase->GetDBEnabled())
		{
			if (this->CreateDBTables(NULL))
			{
				if (this->CreateDBFlags(NULL))
				{
					this->ExportDataToDB(NULL);
				}
			}
		}
	}

	FreeClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		if (!this->GetClientsFromDatabase(NULL))
		{
			FreeClients();
			LoadClients();
		}
		else
		{
			WriteClients();
			this->SetupUnMasked();
			this->SetupMasked();
		}
	}
	else
	{
		// Load up new style clients
		LoadClients();
	}

	flag_desc_list.WriteFlags();
	this->SetupPlayersOnServer();
	LoadIPList();

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Finds any players on the server and creates a link ptr to the client class
//---------------------------------------------------------------------------------
void	ManiClient::SetupPlayersOnServer(void)
{
	for (int i = 1; i <= max_players; i++)
	{
		active_client_list[i - 1] = NULL;
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (strcmp(player.steam_id, "STEAM_ID_PENDING") == 0) continue;

		this->NetworkIDValidated(&player);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the old style admin flags
//---------------------------------------------------------------------------------
bool	ManiClient::LoadOldStyle(void)
{
	FileHandle_t file_handle;
	char	base_filename[512];
	char	old_base_filename[512];
	char	data_in[2048];
	bool	loaded_old_style = false;


	//Get admin groups list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/admingroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style admingroups.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Admin Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			OldAddGroup(data_in, ADMIN);
		}

		filesystem->Close(file_handle);

		snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	//Get immunity groups list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitygroups.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style immunitygroups.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Immunity Group list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			OldAddGroup(data_in, IMMUNITY);
		}

		filesystem->Close(file_handle);

		snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	//Get admin list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/adminlist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style adminlist.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Admin steam id list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			old_style_client_t	temp_client;

			if (!OldAddClient(data_in, &temp_client, true)) continue;

			// Client data is okay, add it to client list
			ConvertOldClientToNewClient(&temp_client, true);
		}

		filesystem->Close(file_handle);
		snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);

		loaded_old_style = true;
	}

	//Get immunity list
	snprintf(base_filename, sizeof (base_filename), "./cfg/%s/immunitylist.txt", mani_path.GetString());
	file_handle = filesystem->Open (base_filename,"rt",NULL);
	if (file_handle == NULL)
	{
//		MMsg("Old style immunitylist.txt file does not exist, using V1.2+ style\n");
	}
	else
	{
		MMsg("Immunity list\n");
		while (filesystem->ReadLine (data_in, sizeof(data_in), file_handle) != NULL)
		{
			if (!ParseLine(data_in, true, false))
			{
				// String is empty after parsing
				continue;
			}

			old_style_client_t	temp_client;

			if (!OldAddClient(data_in, &temp_client, false)) continue;

			// Client data is okay, add it to client list
			ConvertOldClientToNewClient(&temp_client, false);
		}

		filesystem->Close(file_handle);

		snprintf(old_base_filename, sizeof(old_base_filename), "%s.old", base_filename);
		filesystem->RenameFile(base_filename, old_base_filename);
		loaded_old_style = true;
	}

	// We have to botch in player names here that need to be unqiue
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		// No name set up
		if (gpManiDatabase->GetDBEnabled())
		{
			/* Need unique name !! */
			char new_name[128];
			snprintf(new_name, sizeof(new_name), "Client_%i_%s", i+1, 
				gpManiDatabase->GetServerGroupID());
			c_list[i]->SetName(new_name);
		}
		else
		{
			char new_name[128];
			snprintf(new_name, sizeof(new_name), "Client_%i", i+1);
			c_list[i]->SetName(new_name);

		}
	}

	this->SetupUnMasked();
	this->SetupMasked();

	return (loaded_old_style);
}

//---------------------------------------------------------------------------------
// Purpose: Add the client to an already existing entry or create a new one
//---------------------------------------------------------------------------------
void	ManiClient::ConvertOldClientToNewClient
(
 old_style_client_t	*old_client_ptr,
 bool	is_admin
 )
{
	ClientPlayer	*client_ptr;
	int				client_index = -1;
	bool			by_steam = false;
	bool			by_ip = false;
	bool			by_name = false;

	// Find existing client record
	client_index = FindClientIndex(old_client_ptr->steam_id);
	if (client_index == -1)
	{
		client_index = FindClientIndex(old_client_ptr->ip_address);
		if (client_index == -1)
		{
			client_index = FindClientIndex(old_client_ptr->name);
			if (client_index != -1)
			{
				by_name = true;
			}
		}
		else
		{
			by_ip = true;
		}
	}
	else
	{
		by_steam = true;
	}

	if (client_index == -1)
	{
		// Create new client record as we didn't find one
		MMsg("Adding client *********\n");
		client_ptr = new ClientPlayer;
		c_list.push_back(client_ptr);
	}
	else
	{
		// We found a client so point there
		client_ptr = c_list[client_index];
		MMsg("Found client *********\n");
	}

	// Copy core information about player
	if (old_client_ptr->steam_id && !FStrEq(old_client_ptr->steam_id,""))
	{
		if (!by_steam)
		{
			client_ptr->steam_list.Add(old_client_ptr->steam_id);
		}
	}

	if (old_client_ptr->ip_address && !FStrEq(old_client_ptr->ip_address,""))
	{
		if (!by_ip)
		{
			client_ptr->ip_address_list.Add(old_client_ptr->ip_address);
		}
	}

	if (old_client_ptr->name && !FStrEq(old_client_ptr->name,""))
	{
		if (!by_name)
		{
			client_ptr->nick_list.Add(old_client_ptr->name);
		}
	}

	if (old_client_ptr->password && !FStrEq(old_client_ptr->password,""))
	{
		client_ptr->SetPassword(old_client_ptr->password);
	}


	if (old_client_ptr->group_id && !FStrEq(old_client_ptr->group_id, ""))
	{
		if (is_admin)
		{
			client_ptr->group_list.Add(ADMIN, old_client_ptr->group_id);
		}
		else
		{
			client_ptr->group_list.Add(IMMUNITY, old_client_ptr->group_id);
		}
	}


	// Handle flags for client
	if (is_admin)
	{
		for (int i = 0; i < MAX_ADMIN_FLAGS; i++)
		{
			if (old_client_ptr->flags[i].enabled)
			{
				client_ptr->personal_flag_list.SetFlag(ADMIN, old_client_ptr->flags[i].flag_name, true);
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_IMMUNITY_FLAGS; i++)
		{
			if (old_client_ptr->flags[i].enabled)
			{
				client_ptr->personal_flag_list.SetFlag(IMMUNITY, old_client_ptr->flags[i].flag_name, true);
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Free's up admin list memory
//---------------------------------------------------------------------------------

void	ManiClient::FreeClients(void)
{
	// New free clients section
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		delete c_list[i];
	}

	c_list.clear();
	group_list.Kill();
	level_list.Kill();

	for (int i = 0; i < MANI_MAX_PLAYERS; i++)
	{
		active_client_list[i] = NULL;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Sets up admin flags, if you add a new flag it needs to be initialised 
//          here !!
//---------------------------------------------------------------------------------
bool IsCommandIssuedByServerAdmin( void )
{
	if ( engine->IsDedicatedServer() && con_command_index > -1 )
		return false;

	if ( !engine->IsDedicatedServer() && con_command_index > 0 )
		return false;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Creates database tables
//---------------------------------------------------------------------------------
bool ManiClient::CreateDBTables(player_t *player_ptr)
{
	ManiMySQL *mani_mysql = new ManiMySQL();

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating DB tables if not existing....");
	if (!mani_mysql->Init(player_ptr))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL auto_increment, "
		"name varchar(32) NOT NULL, "
		"password varchar(32) default '', "
		"email varchar(255) default '', "
		"notes varchar(255) default '', "
		"PRIMARY KEY (user_id), "
		"UNIQUE KEY (name) "
		") TYPE=MyISAM AUTO_INCREMENT=1", 
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClient()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s( "
		"user_id mediumint(8) NOT NULL default '0', "
		"steam_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, steam_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBSteam()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"nick varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, nick) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBNick()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"ip_address varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, ip_address) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBIP()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBFlag());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"flag_id varchar(20) BINARY NOT NULL default '', "
		"type varchar(32) NOT NULL default '', "
		"description varchar(128) NOT NULL default '', "
		"PRIMARY KEY (flag_id, type) "
		") TYPE=MyISAM",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBFlag()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"server_id mediumint(8) NOT NULL default '0', "
		"name varchar(128) NOT NULL default '', "
		"ip_address varchar(32) NOT NULL default '', "
		"port mediumint(8) NOT NULL default '0', "
		"mod_name varchar(64) NOT NULL default '', "
		"rcon_password varchar(64) default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (server_id), "
		"UNIQUE KEY (name) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBGroup());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"group_id varchar(32) NOT NULL default '', "
		"flag_string text, "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (group_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBGroup()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"group_id varchar(32) NOT NULL default '', "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, group_id, type, server_group_id) "
		") TYPE=MyISAM",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientGroup()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"flag_string text, "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientFlag()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s( "
		"user_id mediumint(8) NOT NULL default '0', "
		"level_id mediumint(8) NOT NULL default '-1', "
		"type varchar(32) NOT NULL default '', "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (user_id, level_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientLevel()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBLevel());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"level_id mediumint(8) NOT NULL default '-1', "
		"type varchar(32) NOT NULL default '', "
		"flag_string text, "
		"server_group_id varchar(32) NOT NULL default '', "
		"PRIMARY KEY (level_id, type, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBLevel()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"user_id mediumint(8) NOT NULL default '0', "
		"server_group_id varchar(32) NOT NULL default '0', "
		"PRIMARY KEY (user_id, server_group_id) "
		") TYPE=MyISAM"
		, gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBClientServer()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Creating %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
	if (!mani_mysql->ExecuteQuery(player_ptr,
		"CREATE TABLE IF NOT EXISTS %s%s ( "
		"version_id varchar(20) NOT NULL)",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Checking %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());

	int row_count;
	if (mani_mysql->ExecuteQuery(player_ptr, &row_count, "SELECT 1 FROM %s%s", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion()))
	{
		if (row_count == 0)
		{
			OutputHelpText(GREEN_CHAT, player_ptr, "No rows found, inserting into %s%s",  gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
			// No rows so insert one
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT INTO %s%s VALUES ('%s')", 
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBVersion(),
				PLUGIN_VERSION_ID2))
			{
				delete mani_mysql;
				return false;
			}
		}
		else
		{
			OutputHelpText(GREEN_CHAT, player_ptr, "Row found, updating %s%s",  gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBVersion());
			if (!mani_mysql->ExecuteQuery(player_ptr,
				"UPDATE %s%s SET version_id = '%s'",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBVersion(),
				PLUGIN_VERSION_ID2))
			{
				delete mani_mysql;
				return false;
			}
		}
	}
	else
	{
		delete mani_mysql;
		return false;
	}

	delete mani_mysql;	

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Creates flag information in tables
//---------------------------------------------------------------------------------
bool ManiClient::CreateDBFlags(player_t *player_ptr)
{
	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init(player_ptr))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Generating DB access flags if not existing....");

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStrKey *key_value = NULL;
		for (const char *desc = flag_desc_list.FindFirst(c_type, &key_value); desc != NULL; desc = flag_desc_list.FindNext(c_type, &key_value))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "Checking class [%s] flag_id [%s]", c_type, key_value->key2);
			int row_count = 0;

			if (!mani_mysql->ExecuteQuery(player_ptr, &row_count, "SELECT f.description "
				"FROM %s%s f "
				"where f.flag_id = '%s' "
				"and f.type = '%s'",
				gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBFlag(),
				key_value->key2, c_type))
			{
				delete mani_mysql;
				return false;
			}

			if (row_count == 0)
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Inserting class [%s] flag_id [%s]", c_type, key_value->key2);
				// Setup flag record
				if (!mani_mysql->ExecuteQuery(player_ptr, "INSERT INTO %s%s (flag_id, type, description) VALUES ('%s', '%s', '%s')",
					gpManiDatabase->GetDBTablePrefix(),
					gpManiDatabase->GetDBTBFlag(),
					key_value->key2,
					c_type,
					desc))
				{
					delete mani_mysql;
					return false;
				}
			}
			else
			{
				mani_mysql->FetchRow();
				if (strcmp(mani_mysql->GetString(0), desc) != 0)
				{
					OutputHelpText(ORANGE_CHAT, player_ptr, "Updating class [%s] flag_id [%s] with new description [%s]", c_type, key_value->key2, desc);
					// Update to the new description
					if (!mani_mysql->ExecuteQuery(player_ptr, "UPDATE %s%s SET description = '%s' WHERE flag_id = '%s' AND type = '%s'",
						gpManiDatabase->GetDBTablePrefix(),
						gpManiDatabase->GetDBTBFlag(),
						desc,
						key_value->key2,
						c_type))
					{
						delete mani_mysql;
						return false;
					}
				}
			}
		}
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Updating version id..");

	mani_mysql->ExecuteQuery(player_ptr,
			"UPDATE %s%s "
			"SET version_id = '%s'",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBVersion(),
			PLUGIN_CORE_VERSION);

	delete mani_mysql;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Export data in memory to the database
//---------------------------------------------------------------------------------
bool ManiClient::ExportDataToDB(player_t *player_ptr)
{
	char	flag_string[2048];

	OutputHelpText(GREEN_CHAT, player_ptr, "Exporting data from clients.txt to DB....");

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init(player_ptr))
	{
		delete mani_mysql;
		return false;
	}

	// Clean out tables for this server id
	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBGroup(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBLevel(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_group_id = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(), gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_id = %i", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer(), gpManiDatabase->GetServerID()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Deleted existing DB data for this server....");

	// Do server details
	if (!mani_mysql->ExecuteQuery(player_ptr, 
		"INSERT INTO %s%s VALUES (%i, '%s', '%s', %i, '%s', '%s', '%s')",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer(),
		gpManiDatabase->GetServerID(),
		gpManiDatabase->GetServerName(),
		gpManiDatabase->GetServerIPAddress(),
		gpManiDatabase->GetServerPort(),
		gpManiDatabase->GetModName(),
		gpManiDatabase->GetRCONPassword(),
		gpManiDatabase->GetServerGroupID()
		))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Generated server details....");


	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStrIntKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = level_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = level_list.FindNext(c_type, &key_value))
		{
			Q_strcpy(flag_string, "");
			if (g_flag->CatFlags(flag_string))
			{
				if (!mani_mysql->ExecuteQuery(player_ptr, 
								"INSERT IGNORE INTO %s%s (level_id, type, flag_string, server_group_id) VALUES (%i, '%s', '%s',  '%s')", 
								gpManiDatabase->GetDBTablePrefix(),
								gpManiDatabase->GetDBTBLevel(),
								key_value->key2,
								key_value->key1,
								flag_string,
								gpManiDatabase->GetServerGroupID()))
					
				{
					delete mani_mysql;
					return false;
				}
			}
		}
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Generated level groups....");

	// Do the flag groups next

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStriKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = group_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = group_list.FindNext(c_type, &key_value))
		{
			Q_strcpy(flag_string, "");
			if (g_flag->CatFlags(flag_string))
			{
				if (!mani_mysql->ExecuteQuery(player_ptr, 
									"INSERT IGNORE INTO %s%s (group_id, flag_string, type, server_group_id) VALUES ('%s', '%s', '%s', '%s')", 
									gpManiDatabase->GetDBTablePrefix(),
									gpManiDatabase->GetDBTBGroup(),
									key_value->key2,
									flag_string,
									key_value->key1,
									gpManiDatabase->GetServerGroupID()))
				{
					delete mani_mysql;
					return false;
				}
			}
		}
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Generated DB global groups....");

	OutputHelpText(GREEN_CHAT, player_ptr, "Building DB client data for %i clients", (int) c_list.size());
	// Populate client list for players that already exist on the server
	// and generate new clients if necessary with user ids
	for (int i = 0; i != (int) c_list.size(); i ++)
	{
		int	row_count;

		OutputHelpText(GREEN_CHAT, player_ptr, "%i", (int) c_list.size() - i);

		c_list[i]->SetUserID(-1);

		if (!mani_mysql->ExecuteQuery(player_ptr, 
						&row_count, 
						"SELECT user_id FROM %s%s WHERE name = '%s'", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(), c_list[i]->GetName()))
		{
			delete mani_mysql;
			return false;
		}

		if (row_count != 0)
		{
			if (!mani_mysql->FetchRow())
			{
				// Should get at least 1 row
				delete mani_mysql;
				return false;
			}

			c_list[i]->SetUserID(mani_mysql->GetInt(0));
		}
		else
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s "
				"(name, password, email, notes) "
				"VALUES "
				"('%s', '%s', '%s', '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClient(),
				c_list[i]->GetName(),
				c_list[i]->GetPassword(),
				c_list[i]->GetEmailAddress(),
				c_list[i]->GetNotes()))
			{
				delete mani_mysql;
				return false;
			}

			c_list[i]->SetUserID(mani_mysql->GetRowID());
		}

		// Setup steam ids
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			"DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBSteam(),
			c_list[i]->GetUserID()))
		{
			delete mani_mysql;
			return false;
		}

		for (const char *steam_id = c_list[i]->steam_list.FindFirst(); steam_id != NULL; steam_id = c_list[i]->steam_list.FindNext())
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s (user_id, steam_id) VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBSteam(),
				c_list[i]->GetUserID(),
				steam_id))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup ip addresses
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			"DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBIP(),
			c_list[i]->GetUserID()))
		{
			delete mani_mysql;
			return false;
		}

		for (const char *ip_address = c_list[i]->ip_address_list.FindFirst(); ip_address != NULL; ip_address = c_list[i]->ip_address_list.FindNext())
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s (user_id, ip_address) VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBIP(),
				c_list[i]->GetUserID(),
				ip_address))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup nickname ids
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			"DELETE FROM %s%s WHERE user_id = %i",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBNick(),
			c_list[i]->GetUserID()))
		{
			delete mani_mysql;
			return false;
		}

		for (const char *nick = c_list[i]->nick_list.FindFirst(); nick != NULL; nick = c_list[i]->nick_list.FindNext())
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s (user_id, nick) VALUES (%i, '%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBNick(),
				c_list[i]->GetUserID(),
				nick))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Setup client_server record
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			"INSERT INTO %s%s (user_id, server_group_id) VALUES (%i, '%s')",
			gpManiDatabase->GetDBTablePrefix(),
			gpManiDatabase->GetDBTBClientServer(),
			c_list[i]->GetUserID(),
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}

		Q_strcpy(flag_string, "");

		// Client personal flags
		// Search through known about flags
		for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
		{
			if (c_list[i]->personal_flag_list.CatFlags(flag_string, c_type))
			{
				if (!mani_mysql->ExecuteQuery(player_ptr, 
									"INSERT IGNORE INTO %s%s (user_id, flag_string, type, server_group_id) VALUES (%i,'%s','%s','%s')",
									gpManiDatabase->GetDBTablePrefix(),
									gpManiDatabase->GetDBTBClientFlag(),
									c_list[i]->GetUserID(),
									flag_string,
									c_type,
									gpManiDatabase->GetServerGroupID()))
				{
					delete mani_mysql;
					return false;
				}
			}
		}

		// Do the client_group flags next
		const char *group_id = NULL;
		for (const char *c_type = c_list[i]->group_list.FindFirst(&group_id); c_type != NULL; c_type = c_list[i]->group_list.FindNext(&group_id))
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s (user_id, group_id, type, server_group_id) VALUES (%i,'%s','%s','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientGroup(),
				c_list[i]->GetUserID(),
				group_id,
				c_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}

		// Do the client_level flags next
		
		const char *cl_type = NULL;
		for (int level_id = c_list[i]->level_list.FindFirst(&cl_type); level_id != -99999; level_id = c_list[i]->level_list.FindNext(&cl_type))
		{
			if (!mani_mysql->ExecuteQuery(player_ptr, 
				"INSERT IGNORE INTO %s%s (user_id, level_id, type, server_group_id) VALUES (%i,%i,'%s','%s')",
				gpManiDatabase->GetDBTablePrefix(),
				gpManiDatabase->GetDBTBClientLevel(),
				c_list[i]->GetUserID(),
				level_id,
				cl_type,
				gpManiDatabase->GetServerGroupID()))
			{
				delete mani_mysql;
				return false;
			}
		}
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Clients built on DB");

	delete mani_mysql;

	return true;

}

//---------------------------------------------------------------------------------
// Purpose: Export server id information
//---------------------------------------------------------------------------------
bool ManiClient::UploadServerID(player_t *player_ptr)
{
	OutputHelpText(GREEN_CHAT, player_ptr, "Exporting data from database.txt to DB....");
	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init(player_ptr))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, "DELETE FROM %s%s WHERE server_id = %i", 
		gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer(), gpManiDatabase->GetServerID()))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Deleted existing server information for this server....");

	// Do server details
	if (!mani_mysql->ExecuteQuery(player_ptr, 
		"INSERT INTO %s%s VALUES (%i, '%s', '%s', %i, '%s', '%s', '%s')",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer(),
		gpManiDatabase->GetServerID(),
		gpManiDatabase->GetServerName(),
		gpManiDatabase->GetServerIPAddress(),
		gpManiDatabase->GetServerPort(),
		gpManiDatabase->GetModName(),
		gpManiDatabase->GetRCONPassword(),
		gpManiDatabase->GetServerGroupID()
		))
	{
		delete mani_mysql;
		return false;
	}

	OutputHelpText(GREEN_CHAT, player_ptr, "Generated server details....");

	return true;

}

//---------------------------------------------------------------------------------
// Purpose: Get Client data from database
//---------------------------------------------------------------------------------
bool ManiClient::GetClientsFromDatabase(player_t *player_ptr)
{
	bool	found_flag;
	int	row_count;
	char	flags_string[2048];

	// Upgrade the database to V1.2 Beta M functionality
	UpgradeDB1();

	OutputHelpText(GREEN_CHAT, player_ptr, "Getting client info from the database....");

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init(player_ptr))
	{
		delete mani_mysql;
		return false;
	}

	if (!mani_mysql->ExecuteQuery(player_ptr, 
		"UPDATE %s%s SET server_group_id = '%s' WHERE server_id = %i",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBServer(),
		gpManiDatabase->GetServerGroupID(),
		gpManiDatabase->GetServerID()))
	{
		delete mani_mysql;
		mani_mysql = new ManiMySQL();
		if (!mani_mysql->Init(player_ptr))
		{
			delete mani_mysql;
			return false;
		}
	}


	// Get admin groups
	if (!mani_mysql->ExecuteQuery(player_ptr, 
		&row_count, 
		"SELECT g.group_id, g.flag_string, g.type "
		"FROM %s%s g "
		"WHERE g.server_group_id = '%s' ",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBGroup(),
		gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (row_count != 0)
	{
		// Found rows
		while (mani_mysql->FetchRow())
		{
			char *group_id = mani_mysql->GetString(0);
			Q_strcpy(flags_string, mani_mysql->GetString(1));
			char *class_type = mani_mysql->GetString(2);

			int flag_index = 0;
			for (;;)
			{
				char *flag_id = SplitFlagString(flags_string, &flag_index);
				if (flag_id == NULL)
				{
					break;
				}

				if (flag_desc_list.IsValidFlag(class_type, flag_id))
				{
					// Create/Update group/level
					GlobalGroupFlag *g_flag = group_list.AddGroup(class_type, group_id);
					if (g_flag)
					{
						g_flag->SetFlag(flag_id, true);
					}
				}
			}
		}
	}

	// Get admin levels
	if (!mani_mysql->ExecuteQuery(player_ptr, 
		&row_count, 
		"SELECT l.level_id, l.flag_string, l.type "
		"FROM %s%s l "
		"WHERE l.server_group_id = '%s' "
		"ORDER BY l.level_id",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBLevel(),
		gpManiDatabase->GetServerGroupID()))
	{
		delete mani_mysql;
		return false;
	}

	if (row_count != 0)
	{
		// Found rows
		while (mani_mysql->FetchRow())
		{
			char *level_id = mani_mysql->GetString(0);
			Q_strcpy(flags_string, mani_mysql->GetString(1));
			char *class_type = mani_mysql->GetString(2);

			int flag_index = 0;
			for (;;)
			{
				char *flag_id = SplitFlagString(flags_string, &flag_index);
				if (flag_id == NULL)
				{
					break;
				}

				if (flag_desc_list.IsValidFlag(class_type, flag_id))
				{
					// Create/Update group/level
					GlobalGroupFlag *g_flag = level_list.AddGroup(class_type, atoi(level_id));
					if (g_flag)
					{
						g_flag->SetFlag(flag_id, true);
					}
				}
			}
		}
	}

	// Get clients for this server
	// Ridiculous search that brings back too many rows but ultimately
	// is faster than doing all the seperate selects per client
	// to do the same thing :(

	OutputHelpText(GREEN_CHAT, player_ptr, "SQL server version [%s]", mani_mysql->GetServerVersion());
	OutputHelpText(GREEN_CHAT, player_ptr, "Major [%i] Minor [%i] Issue [%i]", mani_mysql->GetMajor(), mani_mysql->GetMinor(), mani_mysql->GetIssue());

	if (mani_mysql->IsHigherVer(5,0,11))
	{
		// Post 5.0.11
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			&row_count, 
			"select c.user_id, c.name, c.password, c.email, c.notes, "
			"cf.type, cf.flag_string, "
			"s.steam_id, n.nick, i.ip_address, "
			"cg.type, cg.group_id, "
			"cl.type, cl.level_id "
			"from (%s%s c, %s%s cs) "
			"LEFT JOIN (%s%s s) ON (s.user_id = c.user_id) "
			"LEFT JOIN (%s%s cf) ON (cf.user_id = c.user_id) "
			"LEFT JOIN (%s%s n) ON (n.user_id = c.user_id) "
			"LEFT JOIN (%s%s i) ON (i.user_id = c.user_id) "
			"LEFT JOIN (%s%s cg) ON (cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id) "
			"LEFT JOIN (%s%s cl) ON (cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id) "
			"where c.user_id = cs.user_id "
			"and cs.server_group_id = '%s' "
			"order by c.user_id, cf.type, s.steam_id, n.nick, i.ip_address, cg.type,  "
			"cg.group_id, cl.type, cl.level_id ",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(),
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}
	else
	{
		if (!mani_mysql->ExecuteQuery(player_ptr, 
			&row_count, 
			"select c.user_id, c.name, c.password, c.email, c.notes, "
			"cf.type, cf.flag_string, "
			"s.steam_id, n.nick, i.ip_address, "
			"cg.type, cg.group_id, "
			"cl.type, cl.level_id "
			"from %s%s c, %s%s cs "
			"LEFT JOIN %s%s s ON (s.user_id = c.user_id) "
			"LEFT JOIN %s%s cf ON (cf.user_id = c.user_id) "
			"LEFT JOIN %s%s n ON (n.user_id = c.user_id) "
			"LEFT JOIN %s%s i ON (i.user_id = c.user_id) "
			"LEFT JOIN %s%s cg ON (cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id) "
			"LEFT JOIN %s%s cl ON (cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id) "
			"where c.user_id = cs.user_id "
			"and cs.server_group_id = '%s' "
			"order by c.user_id, cf.type, s.steam_id, n.nick, i.ip_address, cg.type,  "
			"cg.group_id, cl.type, cl.level_id ",
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClient(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientServer(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBSteam(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientFlag(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBNick(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBIP(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientGroup(),
			gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBClientLevel(),
			gpManiDatabase->GetServerGroupID()))
		{
			delete mani_mysql;
			return false;
		}
	}

	// Declare 'last' vars to keep track of changing data
	int last_user_id = -1;

	if (row_count != 0)
	{
		ClientPlayer	*client_ptr = NULL;
		// Found rows
		while (mani_mysql->FetchRow())
		{
			if (last_user_id != mani_mysql->GetInt(0))
			{
				client_ptr = new ClientPlayer;
				c_list.push_back(client_ptr);

				client_ptr->SetUserID(mani_mysql->GetInt(0));
				client_ptr->SetName(mani_mysql->GetString(1));
				client_ptr->SetPassword(mani_mysql->GetString(2));
				client_ptr->SetEmailAddress(mani_mysql->GetString(3));
				client_ptr->SetNotes(mani_mysql->GetString(4));
				last_user_id = client_ptr->GetUserID();
			}

			// Do personal flags first
			if (mani_mysql->GetString(5) && 
				mani_mysql->GetString(6))
			{
				Q_strcpy(flags_string, mani_mysql->GetString(6));

				int flag_index = 0;
				for (;;)
				{
					char *flag_id = this->SplitFlagString(flags_string, &flag_index);
					if (flag_id == NULL)
					{
						break;
					}

					if (flag_desc_list.IsValidFlag(mani_mysql->GetString(5), flag_id))
					{
						// Set personal flag for class type
						client_ptr->personal_flag_list.SetFlag(mani_mysql->GetString(5), flag_id, true);
					}
				}
			}

			// Group levels
			if (mani_mysql->GetString(12) && 
				mani_mysql->GetString(13))
			{
				// Ensure no duplicates
				client_ptr->level_list.Add(mani_mysql->GetString(12), mani_mysql->GetInt(13));
			}

			// Check Steam ID
			if (mani_mysql->GetString(7))
			{
				client_ptr->steam_list.Add(mani_mysql->GetString(7));
			}

			// Check Nick names
			if (mani_mysql->GetString(8))
			{
				client_ptr->nick_list.Add(mani_mysql->GetString(8));
			}

			// Check IP Addresses
			if (mani_mysql->GetString(9))
			{
				client_ptr->ip_address_list.Add(mani_mysql->GetString(9));
			}

			// Check admin groups
			// Group levels
			found_flag = false;
			if (mani_mysql->GetString(10) && 
				mani_mysql->GetString(11))
			{
				client_ptr->group_list.Add(mani_mysql->GetString(10), mani_mysql->GetString(11));
			}
		}
	}

	delete mani_mysql;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Process the ma_setflag command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaSetFlag(player_t *player_ptr, const char *command_name, const int	help_id, const int command_type)
{
	int	admin_index;

	if (player_ptr)
	{
		// Check if player is admin
		if (!this->HasAccess(player_ptr->index, ADMIN, ADMIN_SET_FLAG, war_mode)) return PLUGIN_BAD_ADMIN;
	}

	if (gpCmd->Cmd_Argc() < 4) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	// Whoever issued the commmand is authorised to do it.
	char *target_string = (char *) gpCmd->Cmd_Argv(1);

	admin_index = this->FindClientIndex(target_string);
	if (admin_index != -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", Translate(player_ptr, M_NO_TARGET, "%s", target_string));
		return PLUGIN_STOP;
	}

	// Found an admin to update in memory via admin_index
	// Loop through flags to set them

	char *class_type = (char *) gpCmd->Cmd_Argv(2);
	char *flags = (char *) gpCmd->Cmd_Argv(3);
	int	length = Q_strlen(flags);

	ClientPlayer *client_ptr = c_list[admin_index];

	bool add_flag = true;
	for (int i = 0; i < length; i ++)
	{
		if (flags[i] == '+') 
		{
			add_flag = true;
			continue;
		}

		if (flags[i] == '-')
		{
			add_flag = false;
			continue;
		}

		if (flags[i] == ' ') continue;

		int flag_index = 0;

		char *flag_id = this->SplitFlagString(flags, &flag_index);
		if (flag_id)
		{
			if (flag_desc_list.IsValidFlag(class_type, flag_id))
			{
				client_ptr->personal_flag_list.SetFlag(class_type, flag_id, add_flag);
			}
			else
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Flag %s is invalid", flag_id);
			}
		}
	}

	LogCommand (player_ptr, "%s [%s] [%s]\n", command_name, target_string, flags);

	return PLUGIN_STOP;
}


//---------------------------------------------------------------------------------
// Purpose: Find an entry for the target string in the client list
//---------------------------------------------------------------------------------
int		ManiClient::FindClientIndex
(
 char	*target_string
 )
{
	player_t player;
	int	client_index = -1;
	player.entity = NULL;

	// Whoever issued the commmand is authorised to do it.

	int	target_user_id = atoi(target_string);
	char target_steam_id[MAX_NETWORKID_LENGTH];
	player_t	target_player;

	if (!target_string) return -1;
	if (FStrEq(target_string,"")) return -1;

	// Try looking for user id first in players on server
	if (target_user_id != 0)
	{
		target_player.user_id = target_user_id;
		if (FindPlayerByUserID(&target_player))
		{
			client_index = this->FindClientIndex(&target_player);
			if (client_index != -1)
			{
				return client_index;
			}
		}
	}

	// Try steam id next
	Q_strcpy(target_steam_id, target_string);
	if (Q_strlen(target_steam_id) > 6)
	{
		target_steam_id[6] = '\0';
		if (FStruEq(target_steam_id, "STEAM_"))
		{
			for (int i = 0; i != (int) c_list.size(); i++)
			{
				if (c_list[i]->steam_list.Find(target_string))
				{
					return i;
				}
			}
		}
	}

	// Try ip address next
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		if (c_list[i]->steam_list.Find(target_string))
		{
			return i;
		}
	}

	// Try name id next
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		if (c_list[i]->GetName() && FStrEq(c_list[i]->GetName(), target_string))
		{
			return i;
		}
	}

	// Try nick name next
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		if (c_list[i]->nick_list.Find(target_string))
		{
			return i;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Find an entry for the target string in the client list
//---------------------------------------------------------------------------------
const char *ManiClient::FindClientName
(
 player_t *player_ptr
 )
{
	int index = FindClientIndex(player_ptr);
	if (index == -1)
	{
		return NULL;
	}

	return c_list[index]->GetName();
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaClient(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	bool	invalid_args = false;

	if (player_ptr)
	{
		// Check if player is admin
		if (!HasAccess(player_ptr->index, ADMIN, ADMIN_CLIENT_ADMIN)) return PLUGIN_BAD_ADMIN;
	}

	// Cover only command and subcommand being passed in
	int argc = gpCmd->Cmd_Argc();

	if (argc < 2) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	char *sub_command = (char *) gpCmd->Cmd_Argv(1);
	char *param1 = (char *) gpCmd->Cmd_Argv(2);
	char *param2 = (char *) gpCmd->Cmd_Argv(3);

	// Parse sub command
	if (FStrEq(sub_command, "addclient"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessAddClient(player_ptr, param1);
	}
	else if (FStrEq(sub_command, "addsteam"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddSteam(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addip"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddIP(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addnick"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddNick(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setname"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetName(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setpassword"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetPassword(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setemail"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetEmail(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setnotes"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetNotes(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setalevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetLevel(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setilevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetLevel(IMMUNITY, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroup(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroup(IMMUNITY, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setaflag"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetFlag(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "setiflag"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessSetFlag(IMMUNITY, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeclient"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveClient(player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removesteam"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveSteam(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeip"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveIP(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removenick"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveNick(player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveGroup(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "removeigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessRemoveGroup(IMMUNITY, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "status"))
	{
		if (argc == 3)
		{
			ProcessClientStatus(player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientStatus(player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else if (FStrEq(sub_command, "aflag"))
	{
		if (argc == 3)
		{
			ProcessClientFlagDesc(ADMIN, player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientFlagDesc(ADMIN, player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}	
	else if (FStrEq(sub_command, "iflag"))
	{
		if (argc == 3)
		{
			ProcessClientFlagDesc(IMMUNITY, player_ptr, param1);
		}
		else if (argc == 2)
		{
			ProcessAllClientFlagDesc(IMMUNITY, player_ptr);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else if (FStrEq(sub_command, "upload"))
	{
		if (argc != 2) invalid_args = true;
		else ProcessClientUpload(player_ptr);
	}
	else if (FStrEq(sub_command, "download"))
	{
		if (argc != 2) invalid_args = true;
		else ProcessClientDownload(player_ptr);
	}
	else if (FStrEq(sub_command, "serverid"))
	{
		if (argc != 2) invalid_args = true;
		else ProcessClientServerID(player_ptr);
	}
	else
	{
		invalid_args = true;
	}

	if (invalid_args)
	{
		gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}

	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddClient
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddClient
(
 player_t *player_ptr, 
 char *param1
 )
{
	ClientPlayer	*client_ptr;

	for (int i = 0; i != (int) c_list.size(); i ++)
	{
		if (FStrEq(c_list[i]->GetName(), param1))
		{
			// Bad, client already exists 
			OutputHelpText(ORANGE_CHAT, player_ptr, "ERROR: This client name already exists !!");
			return;
		}
	}

	// Add a new client
	client_ptr = new ClientPlayer;
	c_list.push_back(client_ptr);

	client_ptr->SetName(param1);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddClient();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has been added", client_ptr->GetName());
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddSteam
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddSteam
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	BasicStr steam_id(param2);
	steam_id.Upper();
	if (strncmp("STEAM_", steam_id.str, 6) != 0)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "[%s] is not a valid Steam ID", param1);
		return;
	}

	client_ptr->steam_list.Add(steam_id.str);
	this->SetupPlayersOnServer();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddSteam();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("steam_id", steam_id.str);

		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added Steam ID [%s] for client [%s]", param2, client_ptr->GetName());
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddIP
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddIP
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	int count = 0;
	for (int i = 0; param2[i] != '\0'; i++)
	{
		if (param2[i] == '.')
		{
			count ++;
		}
	}

	if (count < 3 || count > 3)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "IP Address [%s] is invalid", param2);
		return;
	}

	client_ptr->ip_address_list.Add(param2);
	this->SetupPlayersOnServer();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddIPAddress();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("ip_adddress", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added IP Address [%s] for client [%s]", param2, client_ptr->GetName());
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddNick
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddNick
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->nick_list.Add(param2);
	this->SetupPlayersOnServer();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddNick();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("nick", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Added Nickname [%s] for client [%s]", param2, client_ptr->GetName());
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetName
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetName
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;
	char	old_name[256];

	if (param2 == NULL || FStrEq(param2, ""))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "You cannot set a client name to be blank !!");
		return;
	}

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	if (FStrEq(client_ptr->GetName(), param2))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Both names [%s] and [%s] are the same !!", client_ptr->GetName(), param2);
		return;
	}

	// Check for existing clients with the same name
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		// Skip target client name
		if (i == client_index)
		{
			continue;
		}

		if (FStrEq(c_list[i]->GetName(), param2))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "A Client already exists with this name !!");
			return;
		}
	}
		
	Q_strcpy(old_name, client_ptr->GetName());	
	client_ptr->SetName(param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLSetName();
		ptr->in_params.AddParam("old_name", old_name);
		ptr->in_params.AddParam("new_name", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new name of [%s]", old_name, param2);
	return;

}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetPassword
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetPassword
(
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->SetPassword(param2);
	this->SetupPlayersOnServer();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLSetPassword();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("password", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new password of [%s]", client_ptr->GetName(), param2);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetEmail
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetEmail
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->SetEmailAddress(param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLSetEmailAddress();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("email", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new email address of [%s]", client_ptr->GetName(), param2);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddNotes
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetNotes
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->SetNotes(param2);
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLSetEmailAddress();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("notes", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Set client [%s] with new notes of [%s]", client_ptr->GetName(), param2);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetLevel
(
 const char *class_type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;
	int	level_id;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	if (param2 == NULL || FStrEq(param2, ""))
	{
		level_id = -1;
	}
	else
	{
		level_id = atoi(param2);
	}

	client_ptr = c_list[client_index];

	int old_level_id = client_ptr->level_list.FindFirst(class_type);
	if (old_level_id != -99999)
	{
		client_ptr->level_list.Remove(class_type, old_level_id);
	}

	if (level_id != -1)
	{
		client_ptr->level_list.Add(class_type, level_id);
	}


	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLSetLevel();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("class_type", class_type);
		ptr->in_params.AddParam("level_id", level_id);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Updated client [%s] with new %s level id [%s]", client_ptr->GetName(), class_type, param2);

	this->SetupMasked();
	WriteClients();
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddGroup
(
 const char *class_type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;
	bool	found_group = false;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	if (group_list.Find(class_type, param2))
	{
		found_group = true;
	}

	if (!found_group)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group ID [%s] is invalid !!", param2);
		return;
	}

	if (client_ptr->group_list.Find(class_type, param2))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group ID [%s] is already setup for this user", param2);
		return;
	}


	client_ptr->group_list.Add(class_type, param2);
	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddGroup();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("class_type", class_type);
		ptr->in_params.AddParam("group_id", param2);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client [%s] now has %s group [%s] access", client_ptr->GetName(), class_type, param2);

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command AddGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddGroupType
(
 const char	*class_type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	bool	insert_required = false;

	// See if group already exists
	GlobalGroupFlag *group_ptr = group_list.Find(class_type, param1);
	if (group_ptr == NULL)
	{
		insert_required = true;
		// No Group exists yet so we shall create one
		group_ptr = group_list.AddGroup(class_type, param1);
	}

	int		param2_index = 0;

	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/Remove all flags for this group
			const DualStrKey *key_value = NULL;
			for (const char *flag_id = flag_desc_list.FindFirst(class_type, &key_value); flag_id != NULL; flag_id = flag_desc_list.FindNext(class_type, &key_value))
			{
				group_ptr->SetFlag(key_value->key2, flag_add);
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}			

		char *flag_id = this->SplitFlagString(param2, &param2_index);
		if (flag_id)
		{
			if (flag_desc_list.IsValidFlag(class_type, flag_id))
			{
				group_ptr->SetFlag(flag_id, flag_add);
			}
		}
	}

	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		char	flag_string[2048];
		Q_strcpy (flag_string,"");
		if (group_ptr->CatFlags(flag_string))
		{
			SQLProcessBlock *ptr = new SQLAddGroupType();
			ptr->in_params.AddParam("class_type", class_type);
			ptr->in_params.AddParam("group_id", param1);
			ptr->in_params.AddParam("flag_string", flag_string);
			ptr->in_params.AddParam("insert", insert_required);
			client_sql_manager->AddRequest(ptr);
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s group [%s] updated", class_type, param1);
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command RemoveGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveGroupType
(
 const char *class_type,
 player_t *player_ptr, 
 char *param1
 )
{
	// See if group already exists
	if (group_list.Find(class_type, param1))
	{
		group_list.RemoveGroup(class_type, param1);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Group [%s] does not exist !!", param1);
		return;
	}

	for (int i = 0; i != (int) c_list.size(); i++)
	{
		c_list[i]->group_list.Remove(class_type, param1);
	}

	this->SetupUnMasked();
	this->SetupMasked();
	this->WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
			SQLProcessBlock *ptr = new SQLRemoveGroupType();
			ptr->in_params.AddParam("class_type", class_type);
			ptr->in_params.AddParam("group_id", param1);
			client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s group [%s] updated", class_type, param1);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command AddLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAddLevelType
(
 const char *class_type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int		level_id = 0;
	bool	insert_required = false;

	level_id = atoi(param1);

	GlobalGroupFlag *level_ptr = level_list.Find(class_type, level_id);
	if (level_ptr == NULL)
	{
		insert_required = true;
		// No Group exists yet so we shall create one
		level_ptr = level_list.AddGroup(class_type, level_id);
	}

	int		param2_index = 0;
	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/Remove all flags for this level
			const DualStrKey *key_value = NULL;
			for (const char *flag_id = flag_desc_list.FindFirst(class_type, &key_value); flag_id != NULL; flag_id = flag_desc_list.FindNext(class_type, &key_value))
			{
				level_ptr->SetFlag(key_value->key2, flag_add);
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}

		char *flag_id = this->SplitFlagString(param2, &param2_index);
		if (flag_id)
		{
			if (flag_desc_list.IsValidFlag(class_type, flag_id))
			{
				level_ptr->SetFlag(flag_id, flag_add);
			}
		}
	}

	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		// Build the strings needed
		char	flag_string[2048]="";
		level_ptr->CatFlags(flag_string);
		SQLProcessBlock *ptr = new SQLAddLevelType();
		ptr->in_params.AddParam("class_type", class_type);
		ptr->in_params.AddParam("level_id", level_id);
		ptr->in_params.AddParam("flag_string", flag_string);
		ptr->in_params.AddParam("insert", insert_required);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s level [%s] updated", class_type, param1);

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command RemoveLevel
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveLevelType
(
 const char *class_type,
 player_t *player_ptr,  
 char *param1
 )
{
	int		level_id = atoi(param1);

	// See if level already exists
	// See if group already exists
	if (level_list.Find(class_type, level_id))
	{
		level_list.RemoveGroup(class_type, level_id);
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Level [%s] does not exist !!", param1);
		return;
	}

	for (int i = 0; i != (int) c_list.size(); i++)
	{
		c_list[i]->level_list.Remove(class_type, level_id);
	}

	this->SetupUnMasked();
	this->SetupMasked();
	this->WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveLevelType();
		ptr->in_params.AddParam("class_type", class_type);
		ptr->in_params.AddParam("level_id", level_id);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s level [%s] updated", class_type, param1);
	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command SetFlag
//---------------------------------------------------------------------------------
void		ManiClient::ProcessSetFlag
(
 const char *class_type,
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;
	int	param2_index = 0;


	client_index = FindClientIndex(param1);
	if (client_index ==	-1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable	to find	target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	for	(;;)
	{
		bool	flag_add;

		// Scan	for	start of add/remove	flag or	end	of string
		while(param2[param2_index] != '\0' && 
			param2[param2_index] !=	'-'	&&
			param2[param2_index] !=	'+')
		{
			param2_index ++;
		}

		if (param2[param2_index] ==	'\0')
		{
			// End of string reached
			break;
		}

		if (param2[param2_index] ==	'+')
		{
			flag_add = true;
		}
		else
		{
			flag_add = false;
		}

		param2_index ++;
		if (param2[param2_index] ==	'\0')
		{
			break;
		}

		if (param2[param2_index] == '#')
		{
			// Add/Remove all flags for this level
			const DualStrKey *key_value = NULL;
			for (const char *flag_id = flag_desc_list.FindFirst(class_type, &key_value); flag_id != NULL; flag_id = flag_desc_list.FindNext(class_type, &key_value))
			{
				client_ptr->personal_flag_list.SetFlag(class_type, key_value->key2, flag_add);
			}

			param2_index++;
			if (param2[param2_index] == '\0')
			{
				break;
			}

			continue;
		}

		char *flag_id = this->SplitFlagString(param2, &param2_index);
		if (flag_id)
		{
			if (flag_desc_list.IsValidFlag(class_type, flag_id))
			{
				client_ptr->personal_flag_list.SetFlag(class_type, flag_id, flag_add);
			}
		}
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		char	flag_string[2048];
		client_ptr->personal_flag_list.CatFlags(flag_string, class_type);

		SQLProcessBlock *ptr = new SQLSetFlag();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("class_type", class_type);
		ptr->in_params.AddParam("flag_string", flag_string);
		client_sql_manager->AddRequest(ptr);
	}

	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Processed %s flags to client [%s]", class_type, client_ptr->GetName());

	return;

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command AddClient
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveClient
(
 player_t *player_ptr,  
 char *param1
 )
{
	ClientPlayer	*client_ptr;
	int		client_index;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveClient();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		client_sql_manager->AddRequest(ptr);
	}

	for (int i = 0; i < max_players; i++)
	{
		if (active_client_list[i] == client_ptr)
		{
			active_client_list[i] = NULL;
		}
	}

	delete client_ptr;
	
	int index = 0;
	for (std::vector<ClientPlayer *>::iterator itr = c_list.begin(); itr != c_list.end(); ++itr)
	{
		if (index++ == client_index)
		{
			c_list.erase(itr);
			break;
		}
	}

	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();
	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has been removed !!", param1);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveSteam
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveSteam
(
 player_t *player_ptr, 
 char *param1,
 char *param2
 )
{
	ClientPlayer	*client_ptr;
	int		client_index;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->steam_list.Remove(param2);
	this->SetupPlayersOnServer();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveSteam();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("steam_id", param2);
		client_sql_manager->AddRequest(ptr);
	}

	WriteClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had steam id [%s] removed", client_ptr->GetName(), param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveIP
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveIP
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	ClientPlayer	*client_ptr;
	int		client_index;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->ip_address_list.Remove(param2);
	this->SetupPlayersOnServer();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveIPAddress();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("ip", param2);
		client_sql_manager->AddRequest(ptr);
	}

	WriteClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had IP Address [%s] removed", client_ptr->GetName(), param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveNick
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveNick
(
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	ClientPlayer	*client_ptr;
	int		client_index;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];
	client_ptr->nick_list.Remove(param2);
	this->SetupPlayersOnServer();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveNick();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("nick", param2);
		client_sql_manager->AddRequest(ptr);
	}

	WriteClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Client %s has had nickname [%s] removed", client_ptr->GetName(), param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command RemoveGroup
//---------------------------------------------------------------------------------
void		ManiClient::ProcessRemoveGroup
(
 const char *class_type,
 player_t *player_ptr,  
 char *param1,
 char *param2
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;

	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	client_ptr->group_list.Remove(class_type, param2);

	this->SetupUnMasked();
	this->SetupMasked();
	WriteClients();

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLRemoveGroup();
		ptr->in_params.AddParam("name", client_ptr->GetName());
		ptr->in_params.AddParam("group_id", param2);
		ptr->in_params.AddParam("class_type", class_type);
		client_sql_manager->AddRequest(ptr);
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Removed client [%s] from %s flag group [%s]", client_ptr->GetName(), class_type, param2);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAllClientFlagDesc
(
 const char *class_type,
 player_t *player_ptr
 )
{
	const DualStrKey *key_value = NULL;
	for (const char *desc = flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = flag_desc_list.FindNext(class_type, &key_value))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", key_value->key2, desc);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientFlagDesc
(
 const char *class_type,
 player_t *player_ptr, 
 char *param1
 )
{
	const DualStrKey *key_value = NULL;
	for (const char *desc = flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = flag_desc_list.FindNext(class_type, &key_value))
	{
		if (strcmp(key_value->key2, param1) == 0)
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%-20s %s", key_value->key2, desc);
			return;
		}
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "%s flag [%s] does not exist !!", class_type, param1);
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessAllClientStatus
(
 player_t *player_ptr
 )
{
	if (c_list.empty())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "No clients setup yet !!");
		return;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "List of clients, use ma_client status <name> for detailed info on a client");
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", c_list[i]->GetName());
	}	
}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientStatus
(
 player_t *player_ptr,  
 char *param1 // player
 )
{
	int	client_index;
	ClientPlayer	*client_ptr;
	char	temp_string[8192];
	char	temp_string2[512];


	client_index = FindClientIndex(param1);
	if (client_index == -1)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Unable to find target [%s]", param1);
		return;
	}

	client_ptr = c_list[client_index];

	OutputHelpText(ORANGE_CHAT, player_ptr, "Name              : %s", client_ptr->GetName() ? client_ptr->GetName():"");
	OutputHelpText(ORANGE_CHAT, player_ptr, "Email             : %s", client_ptr->GetEmailAddress() ? client_ptr->GetEmailAddress():"");
	OutputHelpText(ORANGE_CHAT, player_ptr, "Notes             : %s", client_ptr->GetNotes() ? client_ptr->GetNotes():"");
	OutputHelpText(ORANGE_CHAT, player_ptr, "Password          : %s", client_ptr->GetPassword() ? client_ptr->GetPassword():"");
	const char *c_type1 = NULL;
	for (int level_id = client_ptr->level_list.FindFirst(&c_type1); level_id != -99999; level_id = client_ptr->level_list.FindNext(&c_type1))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s Level ID    : %i", c_type1, level_id);
	}

	if (!client_ptr->steam_list.IsEmpty())
	{
		Q_strcpy(temp_string, "Steam IDs : ");
		for (const char *steam_id = client_ptr->steam_list.FindFirst(); steam_id != NULL; steam_id = client_ptr->steam_list.FindNext())
		{
			snprintf(temp_string2, sizeof(temp_string2), "%s ", steam_id);
			strcat(temp_string, temp_string2);
		}
		
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	if (!client_ptr->ip_address_list.IsEmpty())
	{
		Q_strcpy(temp_string, "IP Addresses : ");
		for (const char *ip_address = client_ptr->ip_address_list.FindFirst(); ip_address != NULL; ip_address = client_ptr->ip_address_list.FindNext())
		{
			snprintf(temp_string2, sizeof(temp_string2), "%s ", ip_address);
			strcat(temp_string, temp_string2);
		}
		
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	if (!client_ptr->nick_list.IsEmpty())
	{
		Q_strcpy(temp_string, "Nicknames : ");
		for (const char *nick = client_ptr->nick_list.FindFirst(); nick != NULL; nick = client_ptr->nick_list.FindNext())
		{
			snprintf(temp_string2, sizeof(temp_string2), "%s ", nick);
			strcat(temp_string, temp_string2);
		}
		
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s", temp_string);
	}

	const char *group_id = NULL;
	for (const char *c_type = client_ptr->group_list.FindFirst(&group_id); c_type != NULL; c_type = client_ptr->group_list.FindNext(&group_id))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "%s Group : %s", c_type, group_id);
	}

	// Build up flags we are using for this client
	char string[2048];
	
	bool found = false;
	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		if (client_ptr->personal_flag_list.CatFlags(string, c_type))
		{
			if (!found)
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Personal Flags:-");
				found = true;
			}			

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s flags: %s", c_type, string);
		}
	}

	found = false;
	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		if (client_ptr->unmasked_list.CatFlags(string, c_type))
		{
			if (!found)
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Flags including flags from groups:-");
				found = true;
			}

			OutputHelpText(ORANGE_CHAT, player_ptr, "%s flags: %s", c_type, string);
		}
	}

	found = false;
	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		if (client_ptr->masked_list.CatFlags(string, c_type))
		{
			if (!found)
			{
				OutputHelpText(ORANGE_CHAT, player_ptr, "Flags after level group mask applied:-");
				found = true;
			}
		
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s in game flags: %s", c_type, string);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup sub command Status
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientGroupStatus
(
 player_t *player_ptr, 
 char *param1 // type
 )
{
	char	flags_string[2048] = "";

	const DualStriKey *key_value1 = NULL;
	for (GlobalGroupFlag *g_flag = group_list.FindFirst(param1, &key_value1); g_flag != NULL; g_flag = group_list.FindNext(param1, &key_value1))
	{
		if (g_flag->CatFlags(flags_string))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s : %s => %s", key_value1->key1, key_value1->key2, flags_string);
		}
	}

	const DualStrIntKey *key_value2 = NULL;
	for (GlobalGroupFlag *g_flag = level_list.FindFirst(param1, &key_value2); g_flag != NULL; g_flag = level_list.FindNext(param1, &key_value2))
	{
		if (g_flag->CatFlags(flags_string))
		{
			OutputHelpText(ORANGE_CHAT, player_ptr, "%s : %i => %s", key_value2->key1, key_value2->key2, flags_string);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Upload
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientUpload
(
 player_t *player_ptr
 )
{

	if (!gpManiDatabase->GetDBEnabled())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot upload as database functionality not enabled, see database.txt");
		return;
	}

	// Upload to the database

	OutputHelpText(ORANGE_CHAT, player_ptr, "Uploading data.....");
	if (this->CreateDBTables(NULL))
	{
		if (this->CreateDBFlags(NULL))
		{
			this->ExportDataToDB(NULL);
			OutputHelpText(ORANGE_CHAT, NULL, "Upload suceeded");
			return;
		}
	}

	OutputHelpText(ORANGE_CHAT, NULL, "Upload failed !!");

}

//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command ServerID
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientServerID
(
 player_t *player_ptr
 )
{

	if (!gpManiDatabase->GetDBEnabled())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot upload as database functionality not enabled, see database.txt");
		return;
	}

	// Upload to the database

	OutputHelpText(ORANGE_CHAT, player_ptr, "Uploading Server ID data.....");
	if (!this->UploadServerID(player_ptr))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Upload failed !!");
		return;
	}

	OutputHelpText(ORANGE_CHAT, player_ptr, "Upload suceeded");

}
//---------------------------------------------------------------------------------
// Purpose: Handle ma_client sub command Download
//---------------------------------------------------------------------------------
void		ManiClient::ProcessClientDownload
(
 player_t *player_ptr
 )
{

	if (!gpManiDatabase->GetDBEnabled())
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Cannot download as database functionality not enabled, see database.txt");
		return;
	}

	// Upload to the database

	FreeClients();

	OutputHelpText(ORANGE_CHAT, player_ptr, "Downloading data.....");
	if (!this->GetClientsFromDatabase(player_ptr))
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Download failed, using existing clients.txt file instead");
		FreeClients();
		LoadClients();
	}
	else
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Download succeeded, updating clients.txt");
		this->SetupUnMasked();
		this->SetupMasked();
		WriteClients();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
void		ManiClient::UpgradeDB1( void )
{
	bool update_db = false;
	char version_string[32];
	int	row_count = 0;

	if (!gpManiDatabase->GetDBEnabled())
	{
		return;
	}

	ManiMySQL *mani_mysql = new ManiMySQL();

	if (!mani_mysql->Init(NULL))
	{
		delete mani_mysql;
		return;
	}

	if (!mani_mysql->ExecuteQuery(NULL,
		&row_count, 
		"SELECT v.version_id "
		"FROM %s%s v",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion()))
	{
		delete mani_mysql;
		return;
	}

	if (row_count != 0)
	{
		// Found rows
		while (mani_mysql->FetchRow())
		{
			Q_strcpy(version_string, mani_mysql->GetString(0));

			if (Q_strlen(version_string) < 8)
			{
				delete mani_mysql;
				return;
			}

			char v_letter = version_string[7];

			if (v_letter == 'A' || 
				v_letter == 'B' ||
				v_letter == 'C' ||
				v_letter == 'D' ||
				v_letter == 'E' ||
				v_letter == 'F' ||
				v_letter == 'G' ||
				v_letter == 'H' ||
				v_letter == 'I' ||
				v_letter == 'J' ||
				v_letter == 'K' ||
				v_letter == 'L')
			{
				update_db = true; 
			}
	
			break;
		}
	}

	if (!update_db) 
	{
		delete mani_mysql;
		return;
	}

	MMsg("Updating database from pre V%s to new format\n", version_string);

	bool column_exists;

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBGroup(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBGroup())) {delete mani_mysql; return;}

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientGroup(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientGroup())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientFlag(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientFlag())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientLevel(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientLevel())) {delete mani_mysql; return;}
	
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBLevel(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBLevel())) {delete mani_mysql; return;}

	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBClientServer(),"server_id", &column_exists)){delete mani_mysql; return;}
	if (column_exists) if (!this->UpgradeServerIDToServerGroupID(mani_mysql, gpManiDatabase->GetDBTBClientServer())) {delete mani_mysql; return;}

	bool column_matches;

	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientFlag(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientFlag())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientGroup(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientGroup())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBClientLevel(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBClientLevel())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBGroup(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBGroup())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBLevel(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBLevel())) {delete mani_mysql; return;}
	if (!this->TestColumnType(mani_mysql, gpManiDatabase->GetDBTBFlag(), "type", "char(1)", &column_matches)) {delete mani_mysql;return;}
	if (column_matches) if (!this->UpgradeClassTypes(mani_mysql, gpManiDatabase->GetDBTBFlag())) {delete mani_mysql; return;}

	MMsg("Updating table %s%s to have column 'server_group_id'....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!this->TestColumnExists(mani_mysql, gpManiDatabase->GetDBTBServer(),"server_group_id", &column_exists)){delete mani_mysql; return;}
	if (!column_exists)
	{
		MMsg("Updating table %s%s to have column 'server_group_id'....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
		if (!mani_mysql->ExecuteQuery(NULL, "ALTER TABLE %s%s ADD server_group_id varchar(32) NOT NULL default ''", 
			gpManiDatabase->GetDBTablePrefix(),	gpManiDatabase->GetDBTBServer()))
		{
			return ;
		}
	}

	MMsg("Updating table %s%s to have 'Default' for server group id....\n", gpManiDatabase->GetDBTablePrefix(), gpManiDatabase->GetDBTBServer());
	if (!mani_mysql->ExecuteQuery(NULL, "UPDATE %s%s SET server_group_id = 'Default'", 
		gpManiDatabase->GetDBTablePrefix(),	gpManiDatabase->GetDBTBServer()))
	{
		return ;
	}

	MMsg("Updating stored database version ID to %s....\n",  PLUGIN_VERSION_ID2);
	if (!mani_mysql->ExecuteQuery(NULL, "UPDATE %s%s SET version_id = '%s'",
		gpManiDatabase->GetDBTablePrefix(),
		gpManiDatabase->GetDBTBVersion(),
			PLUGIN_VERSION_ID2))
	{
		delete mani_mysql;
		return;
	}

	MMsg("Update completed successfully!\n", version_string);

	delete mani_mysql;
}

//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
bool		ManiClient::UpgradeServerIDToServerGroupID
(
 ManiMySQL *mani_mysql_ptr,
 const char	*table_name
 )
{
	MMsg("Updating 'server_id' to 'server_group_id' on table '%s%s'....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, "ALTER TABLE %s%s CHANGE server_id server_group_id varchar(32) NOT NULL default ''", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Defaulting 'server_group_id' to 'Default' on table '%s%s'....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, "UPDATE %s%s t1 SET t1.server_group_id = 'Default'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Test a column exists
//---------------------------------------------------------------------------------
bool		ManiClient::TestColumnExists(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, bool *found_column)
{
	int row_count;

	MMsg("Testing column '%s' exists on table '%s%s'....\n", column_name, gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, &row_count, "SHOW COLUMNS FROM %s%s LIKE '%s'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name, column_name))
	{
		*found_column = false;
		return false;
	}

	if (row_count != 0)
	{
		MMsg("Column exists\n");
		*found_column = true;
	}
	else
	{
		MMsg("Column does not exist\n");
		*found_column = false;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Test a column type matches our one
//---------------------------------------------------------------------------------
bool		ManiClient::TestColumnType(ManiMySQL *mani_mysql_ptr, char *table_name, char *column_name, char *column_type, bool *column_matches)
{
	int row_count;

	MMsg("Testing column type '%s' matches column '%s' on table '%s%s'....\n", column_type, column_name, gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, &row_count, "SHOW COLUMNS FROM %s%s LIKE '%s'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name, column_name))
	{
		*column_matches = false;
		return false;
	}

	if (row_count == 0)
	{
		return false;
	}

	mani_mysql_ptr->FetchRow();
	if (FStrEq(mani_mysql_ptr->GetString(1), column_type))
	{
		*column_matches = true;
		return true;
	}

	*column_matches = true;
	return true;
}


//---------------------------------------------------------------------------------
// Purpose: Handle upgrade from versions prior to V1.2BetaM
//---------------------------------------------------------------------------------
bool		ManiClient::UpgradeClassTypes
(
 ManiMySQL *mani_mysql_ptr,
 const char	*table_name
 )
{
	MMsg("Updating 'type' to be varchar(32) on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, "ALTER TABLE %s%s CHANGE type type varchar(32) NOT NULL default ''", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Updating 'A' type to be 'Admin' on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, "UPDATE %s%s t1 SET t1.type = 'Admin' where type = 'A'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	MMsg("Updating 'I' type to be 'Immunity' on table %s%s....\n", gpManiDatabase->GetDBTablePrefix(), table_name);
	if (!mani_mysql_ptr->ExecuteQuery(NULL, "UPDATE %s%s t1 SET t1.type = 'Immunity' where type = 'I'", 
		gpManiDatabase->GetDBTablePrefix(),	table_name))
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------

SCON_COMMAND(ma_client, 2221, MaClient, true);

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaClientGroup(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	bool	invalid_args = false;

	if (player_ptr)
	{
		// Check if player is admin
		if (!HasAccess(player_ptr->index, ADMIN, ADMIN_CLIENT_ADMIN)) return PLUGIN_BAD_ADMIN;
	}

	int argc = gpCmd->Cmd_Argc();

	// Cover only command and subcommand being passed in
	if (argc < 3) return gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);

	char *sub_command = (char *) gpCmd->Cmd_Argv(1);
	char *param1 = (char *) gpCmd->Cmd_Argv(2);
	char *param2 = (char *) gpCmd->Cmd_Argv(3);

	// Parse sub command
	if (FStrEq(sub_command, "addagroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroupType(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addigroup"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddGroupType(IMMUNITY, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addalevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddLevelType(ADMIN, player_ptr, param1, param2);
	}
	else if (FStrEq(sub_command, "addilevel"))
	{
		if (argc != 4) invalid_args = true;
		else ProcessAddLevelType(IMMUNITY, player_ptr, param1, param2);
	}	
	else if (FStrEq(sub_command, "removeagroup"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveGroupType(ADMIN, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removeigroup"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveGroupType(IMMUNITY, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removealevel"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveLevelType(ADMIN, player_ptr, param1);
	}
	else if (FStrEq(sub_command, "removeilevel"))
	{
		if (argc != 3) invalid_args = true;
		else ProcessRemoveLevelType(IMMUNITY, player_ptr, param1);
	}	
	else if (FStrEq(sub_command, "status"))
	{
		if (argc == 3)
		{
			ProcessClientGroupStatus(player_ptr, param1);
		}
		else
		{	
			invalid_args = true;
		}
	}
	else
	{
		invalid_args = true;
	}

	if (invalid_args)
	{
		gpManiHelp->ShowHelp(player_ptr, command_name, help_id, command_type);
	}
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_clientgroup, 2223, MaClientGroup, true);

//---------------------------------------------------------------------------------
// Purpose: Con command for setting admin flags
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_setflag, 0, MaSetFlag, true);

//---------------------------------------------------------------------------------
// Purpose: Handle ma_clientgroup command
//---------------------------------------------------------------------------------
PLUGIN_RESULT	ManiClient::ProcessMaReloadClients(player_t *player_ptr, const char	*command_name, const int	help_id, const int	command_type)
{
	if (player_ptr)
	{
		// Check if player is admin
		if (!this->HasAccess(player_ptr->index, ADMIN, ADMIN_CLIENT_ADMIN)) return PLUGIN_BAD_ADMIN;
	}

	this->Init();
	return PLUGIN_STOP;
}

//---------------------------------------------------------------------------------
// Purpose: Con command for re loading admin lists
//---------------------------------------------------------------------------------
SCON_COMMAND(ma_reloadclients, 2167, MaReloadClients, true);

//---------------------------------------------------------------------------------
// Purpose: Sets up immunity flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
void	ManiClient::InitImmunityFlags(void)
{
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag, IMMUNITY_ALLOW_GIMP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIMP].flag_desc,IMMUNITY_ALLOW_GIMP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_KICK].flag, IMMUNITY_ALLOW_KICK_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_KICK].flag_desc,IMMUNITY_ALLOW_KICK_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag, IMMUNITY_ALLOW_SLAY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAY].flag_desc,IMMUNITY_ALLOW_SLAY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BAN].flag, IMMUNITY_ALLOW_BAN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BAN].flag_desc,IMMUNITY_ALLOW_BAN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag, IMMUNITY_ALLOW_CEXEC_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_CEXEC].flag_desc,IMMUNITY_ALLOW_CEXEC_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag, IMMUNITY_ALLOW_BLIND_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BLIND].flag_desc,IMMUNITY_ALLOW_BLIND_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag, IMMUNITY_ALLOW_SLAP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SLAP].flag_desc,IMMUNITY_ALLOW_SLAP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag, IMMUNITY_ALLOW_FREEZE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZE].flag_desc,IMMUNITY_ALLOW_FREEZE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag, IMMUNITY_ALLOW_TELEPORT_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TELEPORT].flag_desc,IMMUNITY_ALLOW_TELEPORT_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag, IMMUNITY_ALLOW_DRUG_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_DRUG].flag_desc,IMMUNITY_ALLOW_DRUG_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag, IMMUNITY_ALLOW_SWAP_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SWAP].flag_desc,IMMUNITY_ALLOW_SWAP_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TAG].flag, IMMUNITY_ALLOW_TAG_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TAG].flag_desc,IMMUNITY_ALLOW_TAG_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag, IMMUNITY_ALLOW_BALANCE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BALANCE].flag_desc,IMMUNITY_ALLOW_BALANCE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BURN].flag, IMMUNITY_ALLOW_BURN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BURN].flag_desc,IMMUNITY_ALLOW_BURN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag, IMMUNITY_ALLOW_MUTE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_MUTE].flag_desc,IMMUNITY_ALLOW_MUTE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag, IMMUNITY_ALLOW_RESERVE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE].flag_desc,IMMUNITY_ALLOW_RESERVE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag, IMMUNITY_ALLOW_SETSKIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_SETSKIN].flag_desc,IMMUNITY_ALLOW_SETSKIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag, IMMUNITY_ALLOW_RESERVE_SKIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_RESERVE_SKIN].flag_desc,IMMUNITY_ALLOW_RESERVE_SKIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag, IMMUNITY_ALLOW_TIMEBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_TIMEBOMB].flag_desc,IMMUNITY_ALLOW_TIMEBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag, IMMUNITY_ALLOW_FIREBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FIREBOMB].flag_desc,IMMUNITY_ALLOW_FIREBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag, IMMUNITY_ALLOW_FREEZEBOMB_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_FREEZEBOMB].flag_desc,IMMUNITY_ALLOW_FREEZEBOMB_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag, IMMUNITY_ALLOW_BEACON_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BEACON].flag_desc,IMMUNITY_ALLOW_BEACON_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag, IMMUNITY_ALLOW_GHOST_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GHOST].flag_desc,IMMUNITY_ALLOW_GHOST_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag, IMMUNITY_ALLOW_GIVE_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GIVE].flag_desc,IMMUNITY_ALLOW_GIVE_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag, IMMUNITY_ALLOW_COLOUR_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_COLOUR].flag_desc, IMMUNITY_ALLOW_COLOUR_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BASIC_IMMUNITY].flag, IMMUNITY_ALLOW_BASIC_IMMUNITY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_BASIC_IMMUNITY].flag_desc, IMMUNITY_ALLOW_BASIC_IMMUNITY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GRAVITY].flag, IMMUNITY_ALLOW_GRAVITY_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_GRAVITY].flag_desc, IMMUNITY_ALLOW_GRAVITY_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AUTOJOIN].flag, IMMUNITY_ALLOW_AUTOJOIN_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AUTOJOIN].flag_desc, IMMUNITY_ALLOW_AUTOJOIN_DESC);

	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AFK].flag, IMMUNITY_ALLOW_AFK_FLAG);
	Q_strcpy(immunity_flag_list[IMMUNITY_ALLOW_AFK].flag_desc, IMMUNITY_ALLOW_AFK_DESC);

}


//---------------------------------------------------------------------------------
// Purpose: Sets up admin flags, if you add a new flag it needs to be intialised 
//          here !!
//---------------------------------------------------------------------------------
void	ManiClient::InitAdminFlags(void)
{
	Q_strcpy(admin_flag_list[ALLOW_GIMP].flag, ALLOW_GIMP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GIMP].flag_desc,ALLOW_GIMP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_KICK].flag, ALLOW_KICK_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_KICK].flag_desc,ALLOW_KICK_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON].flag, ALLOW_RCON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON].flag_desc,ALLOW_RCON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON_MENU].flag, ALLOW_RCON_MENU_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON_MENU].flag_desc,ALLOW_RCON_MENU_DESC);

	Q_strcpy(admin_flag_list[ALLOW_EXPLODE].flag, ALLOW_EXPLODE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_EXPLODE].flag_desc,ALLOW_EXPLODE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SLAY].flag, ALLOW_SLAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SLAY].flag_desc,ALLOW_SLAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BAN].flag, ALLOW_BAN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BAN].flag_desc,ALLOW_BAN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SAY].flag, ALLOW_SAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SAY].flag_desc,ALLOW_SAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CHAT].flag, ALLOW_CHAT_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CHAT].flag_desc,ALLOW_CHAT_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PSAY].flag, ALLOW_PSAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PSAY].flag_desc,ALLOW_PSAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CHANGEMAP].flag, ALLOW_CHANGEMAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CHANGEMAP].flag_desc,ALLOW_CHANGEMAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PLAYSOUND].flag, ALLOW_PLAYSOUND_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PLAYSOUND].flag_desc,ALLOW_PLAYSOUND_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RESTRICTWEAPON].flag, ALLOW_RESTRICTWEAPON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RESTRICTWEAPON].flag_desc,ALLOW_RESTRICTWEAPON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CONFIG].flag, ALLOW_CONFIG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CONFIG].flag_desc,ALLOW_CONFIG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CEXEC].flag, ALLOW_CEXEC_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CEXEC].flag_desc,ALLOW_CEXEC_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CEXEC_MENU].flag, ALLOW_CEXEC_MENU_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CEXEC_MENU].flag_desc,ALLOW_CEXEC_MENU_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BLIND].flag, ALLOW_BLIND_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BLIND].flag_desc,ALLOW_BLIND_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SLAP].flag, ALLOW_SLAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SLAP].flag_desc,ALLOW_SLAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FREEZE].flag, ALLOW_FREEZE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FREEZE].flag_desc,ALLOW_FREEZE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_TELEPORT].flag, ALLOW_TELEPORT_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_TELEPORT].flag_desc,ALLOW_TELEPORT_DESC);

	Q_strcpy(admin_flag_list[ALLOW_DRUG].flag, ALLOW_DRUG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_DRUG].flag_desc,ALLOW_DRUG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SWAP].flag, ALLOW_SWAP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SWAP].flag_desc,ALLOW_SWAP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag, ALLOW_RCON_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCON_VOTE].flag_desc,ALLOW_RCON_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MENU_RCON_VOTE].flag, ALLOW_MENU_RCON_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MENU_RCON_VOTE].flag_desc,ALLOW_MENU_RCON_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag, ALLOW_RANDOM_MAP_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RANDOM_MAP_VOTE].flag_desc,ALLOW_RANDOM_MAP_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MAP_VOTE].flag, ALLOW_MAP_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MAP_VOTE].flag_desc,ALLOW_MAP_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_QUESTION_VOTE].flag, ALLOW_QUESTION_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_QUESTION_VOTE].flag_desc,ALLOW_QUESTION_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag, ALLOW_MENU_QUESTION_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MENU_QUESTION_VOTE].flag_desc,ALLOW_MENU_QUESTION_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CANCEL_VOTE].flag, ALLOW_CANCEL_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CANCEL_VOTE].flag_desc,ALLOW_CANCEL_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_ACCEPT_VOTE].flag, ALLOW_ACCEPT_VOTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_ACCEPT_VOTE].flag_desc,ALLOW_ACCEPT_VOTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MA_RATES].flag, ALLOW_MA_RATES_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MA_RATES].flag_desc,ALLOW_MA_RATES_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BURN].flag, ALLOW_BURN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BURN].flag_desc,ALLOW_BURN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_NO_CLIP].flag, ALLOW_NO_CLIP_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_NO_CLIP].flag_desc,ALLOW_NO_CLIP_DESC);

	Q_strcpy(admin_flag_list[ALLOW_WAR].flag, ALLOW_WAR_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_WAR].flag_desc,ALLOW_WAR_DESC);

	Q_strcpy(admin_flag_list[ALLOW_MUTE].flag, ALLOW_MUTE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_MUTE].flag_desc,ALLOW_MUTE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RESET_ALL_RANKS].flag, ALLOW_RESET_ALL_RANKS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RESET_ALL_RANKS].flag_desc,ALLOW_RESET_ALL_RANKS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CASH].flag, ALLOW_CASH_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CASH].flag_desc,ALLOW_CASH_DESC);

	Q_strcpy(admin_flag_list[ALLOW_RCONSAY].flag, ALLOW_RCONSAY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_RCONSAY].flag_desc,ALLOW_RCONSAY_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SKINS].flag, ALLOW_SKINS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SKINS].flag_desc,ALLOW_SKINS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SETSKINS].flag, ALLOW_SETSKINS_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SETSKINS].flag_desc,ALLOW_SETSKINS_DESC);

	Q_strcpy(admin_flag_list[ALLOW_DROPC4].flag, ALLOW_DROPC4_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_DROPC4].flag_desc,ALLOW_DROPC4_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SETADMINFLAG].flag, ALLOW_SETADMINFLAG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SETADMINFLAG].flag_desc,ALLOW_SETADMINFLAG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_COLOUR].flag, ALLOW_COLOUR_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_COLOUR].flag_desc,ALLOW_COLOUR_DESC);

	Q_strcpy(admin_flag_list[ALLOW_TIMEBOMB].flag, ALLOW_TIMEBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_TIMEBOMB].flag_desc,ALLOW_TIMEBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FIREBOMB].flag, ALLOW_FIREBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FIREBOMB].flag_desc,ALLOW_FIREBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_FREEZEBOMB].flag, ALLOW_FREEZEBOMB_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_FREEZEBOMB].flag_desc,ALLOW_FREEZEBOMB_DESC);

	Q_strcpy(admin_flag_list[ALLOW_HEALTH].flag, ALLOW_HEALTH_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_HEALTH].flag_desc,ALLOW_HEALTH_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BEACON].flag, ALLOW_BEACON_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BEACON].flag_desc,ALLOW_BEACON_DESC);

	Q_strcpy(admin_flag_list[ALLOW_GIVE].flag, ALLOW_GIVE_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GIVE].flag_desc,ALLOW_GIVE_DESC);

	Q_strcpy(admin_flag_list[ALLOW_BASIC_ADMIN].flag, ALLOW_BASIC_ADMIN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_BASIC_ADMIN].flag_desc,ALLOW_BASIC_ADMIN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_CLIENT_ADMIN].flag, ALLOW_CLIENT_ADMIN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_CLIENT_ADMIN].flag_desc,ALLOW_CLIENT_ADMIN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_PERM_BAN].flag, ALLOW_PERM_BAN_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_PERM_BAN].flag_desc,ALLOW_PERM_BAN_DESC);

	Q_strcpy(admin_flag_list[ALLOW_SPRAY_TAG].flag, ALLOW_SPRAY_TAG_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_SPRAY_TAG].flag_desc,ALLOW_SPRAY_TAG_DESC);

	Q_strcpy(admin_flag_list[ALLOW_GRAVITY].flag, ALLOW_GRAVITY_FLAG);
	Q_strcpy(admin_flag_list[ALLOW_GRAVITY].flag_desc,ALLOW_GRAVITY_DESC);

}

//---------------------------------------------------------------------------------
// Purpose: Add custom flag and write flag descriptors
//---------------------------------------------------------------------------------
bool ManiClient::AddFlagDesc(const char *class_name, const char *flag_name, const char *description, bool	replace_description)
{
	if (flag_desc_list.AddFlag(class_name, flag_name, description, replace_description))
	{
		flag_desc_list.WriteFlags();
	}

	if (gpManiDatabase->GetDBEnabled())
	{
		SQLProcessBlock *ptr = new SQLAddFlagDesc();
		ptr->in_params.AddParam("description", description);
		ptr->in_params.AddParam("class_type", class_name);
		ptr->in_params.AddParam("flag_id", flag_name);
		client_sql_manager->AddRequest(ptr);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Add Mani Admin Plugin built in flags
//---------------------------------------------------------------------------------
void	ManiClient::AddBuiltInFlags(void)
{
	// Admin class first
	flag_desc_list.AddFlag(ADMIN, ADMIN_KICK, Translate(NULL, 1501));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCON, Translate(NULL, 1502));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCON_MENU1, Translate(NULL, 1503));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCON_MENU2, Translate(NULL, 1504));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCON_MENU3, Translate(NULL, 1505));
	flag_desc_list.AddFlag(ADMIN, ADMIN_EXPLODE, Translate(NULL, 1506));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SLAY, Translate(NULL, 1507));
	flag_desc_list.AddFlag(ADMIN, ADMIN_BAN, Translate(NULL, 1508));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SAY, Translate(NULL, 1509));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CHAT, Translate(NULL, 1510));
	flag_desc_list.AddFlag(ADMIN, ADMIN_PSAY, Translate(NULL, 1511));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CHANGEMAP, Translate(NULL, 1512));
	flag_desc_list.AddFlag(ADMIN, ADMIN_PLAYSOUND, Translate(NULL, 1513));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RESTRICT_WEAPON, Translate(NULL, 1514));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CONFIG, Translate(NULL, 1515));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CEXEC, Translate(NULL, 1516));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CEXEC_MENU, Translate(NULL, 1517));
	flag_desc_list.AddFlag(ADMIN, ADMIN_BLIND, Translate(NULL, 1518));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SLAP, Translate(NULL, 1519));
	flag_desc_list.AddFlag(ADMIN, ADMIN_FREEZE, Translate(NULL, 1520));
	flag_desc_list.AddFlag(ADMIN, ADMIN_TELEPORT, Translate(NULL, 1521));
	flag_desc_list.AddFlag(ADMIN, ADMIN_DRUG, Translate(NULL, 1522));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SWAP, Translate(NULL, 1523));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCON_VOTE, Translate(NULL, 1524));
	flag_desc_list.AddFlag(ADMIN, ADMIN_MENU_RCON_VOTE, Translate(NULL, 1525));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RANDOM_MAP_VOTE, Translate(NULL, 1526));
	flag_desc_list.AddFlag(ADMIN, ADMIN_MAP_VOTE, Translate(NULL, 1527));
	flag_desc_list.AddFlag(ADMIN, ADMIN_QUESTION_VOTE, Translate(NULL, 1528));
	flag_desc_list.AddFlag(ADMIN, ADMIN_MENU_QUESTION_VOTE, Translate(NULL, 1529));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CANCEL_VOTE, Translate(NULL, 1530));
	flag_desc_list.AddFlag(ADMIN, ADMIN_ACCEPT_VOTE, Translate(NULL, 1531));
	flag_desc_list.AddFlag(ADMIN, ADMIN_MA_RATES, Translate(NULL, 1532));
	flag_desc_list.AddFlag(ADMIN, ADMIN_BURN, Translate(NULL, 1533));
	flag_desc_list.AddFlag(ADMIN, ADMIN_NO_CLIP, Translate(NULL, 1534));
	flag_desc_list.AddFlag(ADMIN, ADMIN_WAR, Translate(NULL, 1535));
	flag_desc_list.AddFlag(ADMIN, ADMIN_MUTE, Translate(NULL, 1536));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RESET_ALL_RANKS, Translate(NULL, 1537));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CASH, Translate(NULL, 1538));
	flag_desc_list.AddFlag(ADMIN, ADMIN_RCONSAY, Translate(NULL, 1539));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SKINS, Translate(NULL, 1540));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SETSKINS, Translate(NULL, 1541));
	flag_desc_list.AddFlag(ADMIN, ADMIN_DROPC4, Translate(NULL, 1542));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SET_FLAG, Translate(NULL, 1543));
	flag_desc_list.AddFlag(ADMIN, ADMIN_COLOUR, Translate(NULL, 1544));
	flag_desc_list.AddFlag(ADMIN, ADMIN_TIMEBOMB, Translate(NULL, 1545));
	flag_desc_list.AddFlag(ADMIN, ADMIN_FIREBOMB, Translate(NULL, 1546));
	flag_desc_list.AddFlag(ADMIN, ADMIN_FREEZEBOMB, Translate(NULL, 1547));
	flag_desc_list.AddFlag(ADMIN, ADMIN_HEALTH, Translate(NULL, 1548));
	flag_desc_list.AddFlag(ADMIN, ADMIN_BEACON, Translate(NULL, 1549));
	flag_desc_list.AddFlag(ADMIN, ADMIN_GIVE, Translate(NULL, 1550));
	flag_desc_list.AddFlag(ADMIN, ADMIN_BASIC_ADMIN, Translate(NULL, 1551));
	flag_desc_list.AddFlag(ADMIN, ADMIN_CLIENT_ADMIN, Translate(NULL, 1552));
	flag_desc_list.AddFlag(ADMIN, ADMIN_PERM_BAN, Translate(NULL, 1553));
	flag_desc_list.AddFlag(ADMIN, ADMIN_SPRAY_TAG, Translate(NULL, 1554));
	flag_desc_list.AddFlag(ADMIN, ADMIN_GRAVITY, Translate(NULL, 1555));

	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_KICK, Translate(NULL, 1701));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_SLAY, Translate(NULL, 1702));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_BAN, Translate(NULL, 1703));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_CEXEC, Translate(NULL, 1704));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_BLIND, Translate(NULL, 1705));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_SLAP, Translate(NULL, 1706));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_FREEZE, Translate(NULL, 1707));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_TELEPORT, Translate(NULL, 1708));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_DRUG, Translate(NULL, 1709));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_SWAP, Translate(NULL, 1710));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_TAG, Translate(NULL, 1711));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_BALANCE, Translate(NULL, 1712));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_BURN, Translate(NULL, 1713));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_MUTE, Translate(NULL, 1714));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_RESERVE, Translate(NULL, 1715));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_SETSKIN, Translate(NULL, 1716));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_RESERVE_SKIN, Translate(NULL, 1717));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_TIMEBOMB, Translate(NULL, 1718));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_FIREBOMB, Translate(NULL, 1719));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_FREEZEBOMB, Translate(NULL, 1720));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_BEACON, Translate(NULL, 1721));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_GHOST, Translate(NULL, 1722));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_GIVE, Translate(NULL, 1723));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_COLOUR, Translate(NULL, 1724));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_GRAVITY, Translate(NULL, 1725));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_AUTOJOIN, Translate(NULL, 1726));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_AFK, Translate(NULL, 1727));
	flag_desc_list.AddFlag(IMMUNITY, IMMUNITY_PING, Translate(NULL, 1728));

}

//---------------------------------------------------------------------------------
// Purpose: Checks if a player on the server has access
//---------------------------------------------------------------------------------
bool	 ManiClient::HasAccess
(
 player_t *player_ptr, 
 const char *class_type, 
 const char *flag_name,
 bool check_war,
 bool check_unmasked_only
 )
{
	if (check_war)
	{
		OutputHelpText(ORANGE_CHAT, player_ptr, "Mani Admin Plugin: Command is disabled in war mode");
		return false;
	}

	int index = FindClientIndex(player_ptr);
	if (index == -1)
	{
		return false;
	}

	// Found a client, look for flag now
	ClientPlayer *client_ptr = c_list[index];

	// Get unmasked access first
	bool unmasked_access = client_ptr->unmasked_list.IsFlagSet(class_type, flag_name);

	if (!unmasked_access) 
	{
		return false;
	}

	if (check_unmasked_only) return true;

	if (!client_ptr->level_list.IsEmpty())
	{
		// Need to check masked access too
		if (client_ptr->masked_list.IsFlagSet(class_type, flag_name))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Checks if client has access to functionality
//---------------------------------------------------------------------------------
bool	 ManiClient::HasAccess
(
 int  player_index, 
 const char *class_type, 
 const char *flag_name,
 bool check_war,
 bool check_unmasked_only
 )
{
	if (player_index < 1 || player_index > max_players) return false;

	if (check_war)
	{
		player_t player;

		player.index = player_index;
		if (FindPlayerByIndex(&player))
		{
			OutputHelpText(ORANGE_CHAT, &player, "Mani Admin Plugin: Command is disabled in war mode");
		}

		return false;
	}

	// Found a client, look for flag now
	if (!active_client_list[player_index - 1]) return false;

	ClientPlayer *client_ptr = active_client_list[player_index - 1];

	// Get unmasked access first
	bool unmasked_access = client_ptr->unmasked_list.IsFlagSet(class_type, flag_name);
	if (!unmasked_access) 
	{
		return false;
	}

	if (check_unmasked_only) return true;

	if (!client_ptr->level_list.IsEmpty())
	{
		// Need to check masked access too
		if (client_ptr->masked_list.IsFlagSet(class_type, flag_name))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Free's up memory for an access list
//---------------------------------------------------------------------------------
int	ManiClient::FindClientIndex
(
 player_t *player_ptr
 )
{
	if (player_ptr->is_bot)
	{
		return -1;
	}

	const char *password = NULL;
	if ( player_ptr->index != 0 )
		password  = engine->GetClientConVarValue(player_ptr->index, "_password" );

	//Search through admin list for match
	for (int i = 0; i != (int) c_list.size(); i ++)
	{
		// Check Steam ID
		if (c_list[i]->steam_list.Find(player_ptr->steam_id))
		{
			return i;
		}

		// Check IP address
		if (c_list[i]->ip_address_list.Find(player_ptr->ip_address))
		{
			return i;
		}

		// Check name password combo
		if (password && c_list[i]->GetPassword() && 
			strcmp(c_list[i]->GetPassword(), password) == 0 &&
			c_list[i]->nick_list.Find(player_ptr->name))
		{
			return i;
		}	
	}

	return -1;
}

//---------------------------------------------------------------------------------
// Purpose: Sets up the unmasked flag accesses for a client
//---------------------------------------------------------------------------------
void	ManiClient::SetupUnMasked(void)
{
	// Loop through all client lists
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		ClientPlayer *client_ptr = c_list[i];

		// Kill unmasked list if already setup
		client_ptr->unmasked_list.Kill();
		client_ptr->unmasked_list.Copy(client_ptr->personal_flag_list);

		// Search groups that this client is a member of
		const char *group_id = NULL;
		for (const char *c_type = client_ptr->group_list.FindFirst(&group_id); c_type != NULL; c_type = client_ptr->group_list.FindNext(&group_id))
		{
			GlobalGroupFlag *g_flag = group_list.Find(c_type, group_id);
			if (g_flag)
			{
				// Found a group for this client so OR in the flags
				for(const char *flag_id = g_flag->FindFirst(); flag_id != NULL; flag_id = g_flag->FindNext())
				{
					client_ptr->unmasked_list.SetFlag(c_type, flag_id, true);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Sets up the masked flag accesses for a client
//---------------------------------------------------------------------------------
void	ManiClient::SetupMasked(void)
{
	mask_level_t	*mask_list = NULL;
	int				mask_list_size = 0;

	// Loop through all clients on server
	for (int i = 0; i < max_players; i++)
	{
		if (active_client_list[i] != NULL)
		{
			// Found a client
			ClientPlayer *client_ptr = active_client_list[i];
			client_ptr->masked_list.Kill();

			const char *c_type1 = NULL;
			for (int level_id = client_ptr->level_list.FindFirst(&c_type1); level_id != -99999; level_id = client_ptr->level_list.FindNext(&c_type1))
			{
				// Has levels setup !!
				if (level_id > -1)
				{
					AddToList((void **) &mask_list, sizeof(mask_level_t), &mask_list_size);
					mask_level_t	*mask_ptr = &(mask_list[mask_list_size - 1]);

					mask_ptr->client_ptr = active_client_list[i];
					mask_ptr->level_id = level_id;
					Q_strcpy(mask_ptr->class_type, c_type1);
				}
			}
		}
	}

	if (mask_list_size == 0)
	{
		// Nothing to do
		return;
	}

	// Sort mask list
	qsort(mask_list, mask_list_size, sizeof(mask_level_t), sort_mask_list);

	for (int i = 0; i < mask_list_size; i ++)
	{
		for (int j = i; j < mask_list_size; j++)
		{
			if (strcmp(mask_list[j].class_type, mask_list[i].class_type) != 0)
			{
				break;
			}

			if (mask_list[j].level_id > mask_list[i].level_id)
			{
				// Found a candidate to apply the mask to
				ClientPlayer *mclient_ptr = mask_list[j].client_ptr;

				GlobalGroupFlag *g_flag = level_list.Find(mask_list[i].class_type, mask_list[i].level_id);
				if (g_flag)
				{
					// Found a group for this client so OR in the flags
					for(const char *flag_id = g_flag->FindFirst(); flag_id != NULL; flag_id = g_flag->FindNext())
					{
						// Set the clients masked flag enabled to true
						mclient_ptr->masked_list.SetFlag(mask_list[i].class_type, flag_id, true);
					}
				}
			}
		}
	}

	FreeList((void **) &mask_list, &mask_list_size);
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the super new style admin flags
//---------------------------------------------------------------------------------
void	ManiClient::LoadClients(void)
{
	char	core_filename[256];
	char	version_string[32];
	ManiKeyValues *kv_ptr;

	kv_ptr = new ManiKeyValues("clients.txt");
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/clients.txt", mani_path.GetString());

	if (!kv_ptr->ReadFile(core_filename))
	{
		MMsg("Failed to load %s\n", core_filename);
		kv_ptr->DeleteThis();
		return;
	}

	// clients.txt key
	read_t *rd_ptr = kv_ptr->GetPrimaryKey();
	if (!rd_ptr)
	{
		kv_ptr->DeleteThis();
		return;
	}

	Q_strcpy(version_string, kv_ptr->GetString("version", "NONE"));
	if (strcmp(version_string, "NONE") == 0)
	{
		kv_ptr->DeleteThis();
		// Load old style clients (will be saved in new format)
		// Call old style
		this->LoadClientsBeta();
		this->WriteClients();
		return;
	}

	// Find groups
	read_t *groups_ptr = kv_ptr->FindKey(rd_ptr, "groups");
	if (groups_ptr)
	{
		// Read in the groups
		this->ReadGroups(kv_ptr, groups_ptr, true);
	}

	// Find levels 
	read_t *levels_ptr = kv_ptr->FindKey(rd_ptr, "levels");
	if (levels_ptr)
	{
		// Read in the levels
		this->ReadGroups(kv_ptr, levels_ptr, false);
	}

	// Find players
	read_t *players_ptr = kv_ptr->FindKey(rd_ptr, "players");
	if (players_ptr)
	{
		this->ReadPlayers(kv_ptr, players_ptr);
	}

	kv_ptr->DeleteThis();

	// Calculates each clients unmasked flags (flags without levels applied)
	SetupUnMasked();
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the super new style admin flags
//---------------------------------------------------------------------------------
void	ManiClient::ReadGroups
(
 ManiKeyValues *kv_ptr,
 read_t	*read_ptr,
 bool	group_type
)
{
	for (;;)
	{
		read_t	*class_ptr = kv_ptr->GetNextKey(read_ptr);
		if (class_ptr == NULL)
		{
			return;
		}

		for (;;)
		{
			char *class_name = class_ptr->sub_key_name;
			kv_ptr->ResetKeyIndex();

			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				// Value = Flag string (multiple flags in a string)
				// Name = Group ID
				if (value == NULL)
				{
					// No more flags for this class
					break;
				}

				int i = 0;
				// Get individual flags
				for (;;)
				{
					char *flag_id = SplitFlagString(value, &i);
					if (flag_id == NULL)
					{
						break;
					}

					if (flag_desc_list.IsValidFlag(class_name, flag_id))
					{
						// Create/Update group/level
						if (group_type)
						{
							GlobalGroupFlag *g_flag = group_list.AddGroup(class_name, name);
							if (g_flag)
							{
								g_flag->SetFlag(flag_id, true);
							}
						}
						else
						{
							GlobalGroupFlag *g_flag = level_list.AddGroup(class_name, atoi(name));
							if (g_flag)
							{
								g_flag->SetFlag(flag_id, true);
							}
						}
					}
				}
			}

			class_ptr = kv_ptr->GetNextKey(read_ptr);
			if (!class_ptr)
			{
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Takes a string an picks out the individual flags (not thread safe!)
//---------------------------------------------------------------------------------
char	*ManiClient::SplitFlagString(char *flags_ptr, int *index)
{
	static	char	flag_name[4096];

	// Skip space seperators
	while (flags_ptr[*index] == ';' || flags_ptr[*index] == ' ' || flags_ptr[*index] == '\t')
	{
		*index = *index + 1;
	}

	if (flags_ptr[*index] == '\0') return NULL;
	int	i = 0;

	while (flags_ptr[*index] != ' ' && 
		flags_ptr[*index] != ';' && 
		flags_ptr[*index] != '\t' && 
		flags_ptr[*index] != '\0')
	{
		flag_name[i++] = flags_ptr[*index];
		*index = *index + 1;
	}

	flag_name[i] = '\0';

	return flag_name;
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the players details
//---------------------------------------------------------------------------------
void	ManiClient::ReadPlayers
(
 ManiKeyValues *kv_ptr,
 read_t	*read_ptr
)
{
	char	temp_string[256];

	for (;;)
	{
		// Get Player unique id {Client_1_1}
		read_t	*player_ptr = kv_ptr->GetNextKey(read_ptr);
		if (player_ptr == NULL)
		{
			return;
		}

		// Add a new client
		ClientPlayer *client_ptr = new ClientPlayer;
		c_list.push_back(client_ptr);

		// Do simple stuff first
		client_ptr->SetEmailAddress(kv_ptr->GetString("email", ""));
		client_ptr->SetName(kv_ptr->GetString("name", ""));
		client_ptr->SetPassword(kv_ptr->GetString("password", ""));
		client_ptr->SetNotes(kv_ptr->GetString("notes", ""));

		/* Handle single steam id */
		Q_strcpy(temp_string, kv_ptr->GetString("steam", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->steam_list.Add(temp_string);
		}


		/* Handle single ip */
		Q_strcpy(temp_string, kv_ptr->GetString("ip", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->ip_address_list.Add(temp_string);
		}

		/* Handle single nickname */
		Q_strcpy(temp_string, kv_ptr->GetString("nick", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->nick_list.Add(temp_string);
		}

		// Steam IDs
		if (kv_ptr->FindKey(player_ptr, "steam"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				if (value == NULL)
				{
					// No more steam ids
					break;
				}

				client_ptr->steam_list.Add(value);
			}
		}

		// IP Addresses
		if (kv_ptr->FindKey(player_ptr, "ip"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				if (value == NULL)
				{
					// No more ip addresses
					break;
				}

				client_ptr->ip_address_list.Add(value);
			}
		}

		// Player nicknames
		if (kv_ptr->FindKey(player_ptr, "nick"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				if (value == NULL)
				{
					// No more nick names
					break;
				}

				client_ptr->nick_list.Add(value);
			}
		}

		// Player groups
		if (kv_ptr->FindKey(player_ptr, "groups"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				if (value == NULL)
				{
					// No more groups
					break;
				}


				if (group_list.Find(name, value))
				{
					client_ptr->group_list.Add(name, value);
				}
			}
		}

		// Player levels
		if (kv_ptr->FindKey(player_ptr, "levels"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *name = NULL;
				char *value = kv_ptr->GetNextKeyValue(&name);
				if (value == NULL)
				{
					// No more groups
					break;
				}

				int level_id = atoi(value);

				client_ptr->level_list.Add(name, level_id);
			}
		}

		// Last but not least we need to do the personal flags
		if (kv_ptr->FindKey(player_ptr, "flags"))
		{
			kv_ptr->ResetKeyIndex();
			for (;;)
			{
				char *class_type = NULL;
				char *value = kv_ptr->GetNextKeyValue(&class_type);
				if (value == NULL)
				{
					// No more flags
					break;
				}

				int i = 0;

				// Get individual flags
				for (;;)
				{
					char *flag_id = SplitFlagString(value, &i);
					if (flag_id == NULL)
					{
						break;
					}

					if (flag_desc_list.IsValidFlag(class_type, flag_id))
					{
						// Create flag
						client_ptr->personal_flag_list.SetFlag(class_type, flag_id, true);
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the clients.txt file based on data in memory
//---------------------------------------------------------------------------------
void		ManiClient::WriteClients(void)
{
	char temp_string[2048];

	char	core_filename[256];
	ManiKeyValues *client;

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/clients.txt", mani_path.GetString());

	client = new ManiKeyValues( "clients.txt" );

	if (!client->WriteStart(core_filename))
	{
		MMsg("Failed to write %s\n", core_filename);
		delete client;
		return;
	}

	client->WriteKey("version", "1");
	client->WriteCR();
	client->WriteComment("This key group lists all your client players");
	client->WriteNewSubKey("players");

//	MMsg("Writing %i client(s)\n",  c_list_size);
	// Loop through all clients
	for (int i = 0; i != (int) c_list.size(); i ++)
	{
		ClientPlayer *client_ptr = c_list[i];

		if (!client_ptr->GetName() || FStrEq(client_ptr->GetName(), ""))
		{
			// No name set up
			if (gpManiDatabase->GetDBEnabled())
			{
				/* Need unique name !! */
				snprintf(temp_string, sizeof(temp_string), "Client_%i_%s", i+1, 
					gpManiDatabase->GetServerGroupID());
			}
			else
			{
				snprintf(temp_string, sizeof(temp_string), "Client_%i", i+1);
			}

			client_ptr->SetName(temp_string);
		}
		else
		{
			snprintf(temp_string, sizeof(temp_string), "%s", client_ptr->GetName());
		}

		client->WriteComment("This must be a unique client name");
		client->WriteNewSubKey(temp_string);

		if (client_ptr->GetEmailAddress()) client->WriteKey("email", client_ptr->GetEmailAddress());
		if (client_ptr->GetName())
		{
			client->WriteComment("Client real name");
			client->WriteKey("name", client_ptr->GetName());
		}
		if (client_ptr->GetPassword()) client->WriteKey("password", client_ptr->GetPassword());
		if (client_ptr->GetNotes()) client->WriteKey("notes", client_ptr->GetNotes());
		if (client_ptr->steam_list.Size() == 1) 
		{
			client->WriteComment("Steam ID for client");
			client->WriteKey("steam", client_ptr->steam_list.FindFirst());
		}
		if (client_ptr->ip_address_list.Size() == 1) client->WriteKey("ip", client_ptr->ip_address_list.FindFirst());
		if (client_ptr->nick_list.Size() == 1) client->WriteKey("nick", client_ptr->nick_list.FindFirst());

		// Write Steam IDs if needed.
		if (client_ptr->steam_list.Size() > 1)
		{
			client->WriteComment("This sub key is for multiple steam ids");
			client->WriteNewSubKey("steam");
			
			int j = 1;
			for (const char *steam_id = client_ptr->steam_list.FindFirst(); steam_id != NULL; steam_id = client_ptr->steam_list.FindNext())
			{
				snprintf(temp_string, sizeof(temp_string), "steam%i", j++);
				client->WriteKey(temp_string, steam_id);
			}

			client->WriteEndSubKey();
		}

		// Write IP Addresses if needed.
		if (client_ptr->ip_address_list.Size() > 1)
		{
			client->WriteComment("This sub key is for multiple ip addresses");
			client->WriteNewSubKey("ip");

			int j = 1;
			for (const char *ip_address = client_ptr->ip_address_list.FindFirst(); ip_address != NULL; ip_address = client_ptr->ip_address_list.FindNext())
			{
				snprintf(temp_string, sizeof(temp_string), "IP%i", j++);
				client->WriteKey(temp_string, ip_address);
			}

			client->WriteEndSubKey();
		}

		// Write Player Nicknames if needed.
		if (client_ptr->nick_list.Size() > 1)
		{
			client->WriteComment("This sub key is for multiple player nick names");
			client->WriteNewSubKey("nick");

			int j = 1;
			for (const char *nick = client_ptr->nick_list.FindFirst(); nick != NULL; nick = client_ptr->nick_list.FindNext())
			{
				snprintf(temp_string, sizeof(temp_string), "nick%i", j++);
				client->WriteKey(temp_string, nick);
			}

			client->WriteEndSubKey();
		}

		bool found_flags = false;
		for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
		{
			bool start_flag = true;
			while(client_ptr->personal_flag_list.CatFlags(temp_string, c_type, 60, start_flag))
			{
				if (!found_flags)
				{
					client->WriteComment("These are personal access flags for a player");
					client->WriteNewSubKey("flags");
					found_flags = true;
				}

				if (start_flag)
				{
					start_flag = false;
				}

				client->WriteKey(c_type, temp_string);
			}
		}

		if (found_flags) client->WriteEndSubKey();

		// Write out the groups we belong to 
		if (!client_ptr->group_list.IsEmpty())
		{
			client->WriteNewSubKey("groups");
			const char *group_id = NULL;
			for (const char *c_type = client_ptr->group_list.FindFirst(&group_id); c_type != NULL; c_type = client_ptr->group_list.FindNext(&group_id))
			{
				client->WriteKey(c_type, group_id);
			}
			client->WriteEndSubKey();
		}

		// Write out the levels we belong to 
		if (!client_ptr->level_list.IsEmpty())
		{
			client->WriteNewSubKey("levels");
			const char *c_type = NULL;
			for (int level_id = client_ptr->level_list.FindFirst(&c_type); level_id != -99999; level_id = client_ptr->level_list.FindNext(&c_type))
			{
				client->WriteKey(c_type, level_id);
			}

			client->WriteEndSubKey();
		}

		client->WriteEndSubKey();

		if (i != ((int) c_list.size() - 1))
		{
			// Pad out the spacing between players
			client->WriteCR();
		}
	}

	// End Players
	client->WriteEndSubKey();
	client->WriteCR();

	// Need to write out our global groups and levels now
	bool written_primary_key = false;

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		bool new_class_type = true;

		const DualStriKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = group_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = group_list.FindNext(c_type, &key_value))
		{
			if (!written_primary_key)
			{
				client->WriteComment("These are global groups of flags that can be assigned to clients");
				client->WriteNewSubKey("groups");
				written_primary_key = true;
			}

			bool start = true;
			while(g_flag->CatFlags(temp_string, 60, start))
			{
				start = false;
				if (new_class_type)
				{
                    // Write key 'Admin', 'Immunity' etc
					client->WriteNewSubKey(c_type);
					new_class_type = false;
				}

				// Write each flag
				client->WriteKey(key_value->key2, temp_string);
			}
		}

		if (!new_class_type)
		{
			// Write class type end of key
			client->WriteEndSubKey();
		}
	}

	if (written_primary_key)
	{
			client->WriteEndSubKey();
			client->WriteCR();
	}

	written_primary_key = false;
	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		bool new_class_type = true;

		const DualStrIntKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = level_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = level_list.FindNext(c_type, &key_value))
		{
			if (!written_primary_key)
			{
				client->WriteComment("These are global groups of flags that can be assigned to clients");
				client->WriteNewSubKey("levels");
				written_primary_key = true;
			}

			bool start = true;
			while(g_flag->CatFlags(temp_string, 60, start))
			{
				start = false;
				if (new_class_type)
				{
                    // Write key 'Admin', 'Immunity' etc
					client->WriteNewSubKey(c_type);
					new_class_type = false;
				}

				// Write each flag
				char str_key[32];

				snprintf(str_key, sizeof(str_key), "%d", key_value->key2);
				client->WriteKey(str_key, temp_string);
			}
		}

		if (!new_class_type)
		{
			// Write class type end of key
			client->WriteEndSubKey();
		}
	}

	if (written_primary_key)
	{
			client->WriteEndSubKey();
			client->WriteCR();
	}

	client->WriteEnd();

	delete client;
	return;
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin flags from Pre Beta RC2
//---------------------------------------------------------------------------------
void	ManiClient::LoadClientsBeta(void)
{
	char	core_filename[256];
	bool found_match;
	KeyValues *base_key_ptr;

//	MMsg("*********** Loading admin section of clients.txt ************\n");
	// Read the clients.txt file

	KeyValues *kv_ptr = new KeyValues("clients.txt");

	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/clients.txt", mani_path.GetString());
	if (!kv_ptr->LoadFromFile( filesystem, core_filename, NULL))
	{
		MMsg("Failed to load clients.txt\n");
		kv_ptr->deleteThis();
		return;
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our groups first
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "admingroups"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the groups in
		GetAdminGroupsBeta(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our immunity groups
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "immunitygroups"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the groups in
		GetImmunityGroupsBeta(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our levels next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "adminlevels"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the levels in
		GetAdminLevelsBeta(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our levels next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "immunitylevels"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the levels in
		GetImmunityLevelsBeta(base_key_ptr);
	}

	//////////////////////////////////////////////
	base_key_ptr = kv_ptr->GetFirstTrueSubKey();
	if (!base_key_ptr)
	{
//		MMsg("No true subkey found\n");
		kv_ptr->deleteThis();
		return;
	}

	found_match = false;

	// Find our clients next
	for (;;)
	{
		if (FStrEq(base_key_ptr->GetName(), "players"))
		{
			found_match = true;
			break;
		}

		base_key_ptr = base_key_ptr->GetNextKey();
		if (!base_key_ptr)
		{
			break;
		}
	}

	if (found_match)
	{
		// Get all the clients in
		GetClientsBeta(base_key_ptr);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin groups from Pre Beta 1.2RC2
//---------------------------------------------------------------------------------
void	ManiClient::GetAdminGroupsBeta(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	char	group_name[128];
	admin_group_t	admin_group;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&admin_group, 0, sizeof(admin_group_t));

		Q_strcpy(group_name, kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString());
		Q_strcpy(admin_group.group_id, group_name);

		if (!FStrEq("", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				char *flag_name = this->SplitFlagString(flag_string, &flag_string_index);
				if (flag_name != NULL)
				{
					if (flag_desc_list.IsValidFlag(ADMIN, flag_name))
					{
						GlobalGroupFlag *g_flag = group_list.AddGroup(ADMIN, admin_group.group_id);
						if (g_flag)
						{
							g_flag->SetFlag(flag_name, true);
						}
					}
				}
			}
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style immunity groups from Pre Beta 1.2RC2
//---------------------------------------------------------------------------------
void	ManiClient::GetImmunityGroupsBeta(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];
	char	group_name[128];
	immunity_group_t	immunity_group;

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_memset(&immunity_group, 0, sizeof(immunity_group_t));

		Q_strcpy(group_name, kv_ptr->GetName());
		Q_strcpy(flag_string, kv_ptr->GetString());
		Q_strcpy(immunity_group.group_id, group_name);

		if (!FStrEq("", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				char *flag_name = this->SplitFlagString(flag_string, &flag_string_index);
				if (flag_name != NULL)
				{
					if (flag_desc_list.IsValidFlag("Immunity", flag_name))
					{
						GlobalGroupFlag *g_flag = group_list.AddGroup(IMMUNITY, immunity_group.group_id);
						if (g_flag)
						{
							g_flag->SetFlag(flag_name, true);
						}
					}
				}
			}
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style admin levels Pre Beta RC2
//---------------------------------------------------------------------------------
void	ManiClient::GetAdminLevelsBeta(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_strcpy(flag_string, kv_ptr->GetString());

		if (!FStrEq("", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				char *flag_name = this->SplitFlagString(flag_string, &flag_string_index);
				if (flag_name != NULL)
				{
					if (flag_desc_list.IsValidFlag(ADMIN, flag_name))
					{
						GlobalGroupFlag *g_flag = level_list.AddGroup(ADMIN, atoi(kv_ptr->GetName()));
						if (g_flag)
						{
							g_flag->SetFlag(flag_name, true);
						}
					}
				}
			}
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style immunity levels pre Beta 1.2RC2
//---------------------------------------------------------------------------------
void	ManiClient::GetImmunityLevelsBeta(KeyValues *ptr)
{
	KeyValues *kv_ptr;
	char	flag_string[4096];

	kv_ptr = ptr->GetFirstValue();
	if (!kv_ptr)
	{
		return;
	}

	for (;;)
	{
		Q_strcpy(flag_string, kv_ptr->GetString());

		if (!FStrEq("", flag_string))
		{
			int flag_string_index = 0;

			while(flag_string[flag_string_index] != '\0')
			{
				char *flag_name = this->SplitFlagString(flag_string, &flag_string_index);
				if (flag_name != NULL)
				{
					if (flag_desc_list.IsValidFlag(ADMIN, flag_name))
					{
						GlobalGroupFlag *g_flag = level_list.AddGroup(IMMUNITY, atoi(kv_ptr->GetName()));
						if (g_flag)
						{
							g_flag->SetFlag(flag_name, true);
						}
					}
				}
			}
		}

		kv_ptr = kv_ptr->GetNextValue();
		if (!kv_ptr)
		{
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Loads into memory the new style clients flags Pre Beta 1.2RC2
// This will upgrade the old style clients.txt file to be stored in the new format
//---------------------------------------------------------------------------------
void	ManiClient::GetClientsBeta(KeyValues *ptr)
{
	KeyValues *players_ptr;
	KeyValues *kv_ptr;
	KeyValues *temp_ptr;

	char	flag_string[4096];
	char	temp_string[256];
	ClientPlayer	*client_ptr;

	// Should be at key 'players'
	players_ptr = ptr->GetFirstSubKey();
	if (!players_ptr)
	{
		return;
	}

	for (;;)
	{
		client_ptr = (ClientPlayer *) new ClientPlayer;
		c_list.push_back(client_ptr);

		kv_ptr = players_ptr;

		// Do simple stuff first
		client_ptr->SetEmailAddress(kv_ptr->GetString("email"));
		int admin_level_id = kv_ptr->GetInt("admin_level", -1);
		if (admin_level_id != -1)
		{
			client_ptr->level_list.Add(ADMIN, admin_level_id);
		}

		int immunity_level_id = kv_ptr->GetInt("immunity_level", -1);
		if (immunity_level_id != -1)
		{
			client_ptr->level_list.Add(IMMUNITY, immunity_level_id);
		}

		client_ptr->SetName(kv_ptr->GetString("name"));
		client_ptr->SetPassword(kv_ptr->GetString("password"));
		client_ptr->SetNotes(kv_ptr->GetString("notes"));

		/* Handle single steam id */
		Q_strcpy(temp_string, kv_ptr->GetString("steam", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->steam_list.Add(temp_string);
		}


		/* Handle single ip */
		Q_strcpy(temp_string, kv_ptr->GetString("ip", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->ip_address_list.Add(temp_string);
		}

		/* Handle single nickname */
		Q_strcpy(temp_string, kv_ptr->GetString("nick", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->nick_list.Add(temp_string);
		}

		/* Handle single admin group */
		Q_strcpy(temp_string, kv_ptr->GetString("admingroups", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->group_list.Add(ADMIN, temp_string);
		}

		/* Handle single immunity group */
		Q_strcpy(temp_string, kv_ptr->GetString("immunitygroups", ""));
		if (!FStrEq(temp_string, ""))
		{
			client_ptr->group_list.Add(IMMUNITY, temp_string);
		}

		// Steam IDs
		temp_ptr = kv_ptr->FindKey("steam", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					client_ptr->steam_list.Add(temp_ptr->GetString());
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// IP Addresses
		temp_ptr = kv_ptr->FindKey("ip", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					client_ptr->ip_address_list.Add(temp_ptr->GetString());
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Player nicknames
		temp_ptr = kv_ptr->FindKey("nick", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					client_ptr->nick_list.Add(temp_ptr->GetString());
					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Admin Groups 
		temp_ptr = kv_ptr->FindKey("admingroups", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					if (group_list.Find(ADMIN, temp_ptr->GetString()))
					{
						client_ptr->group_list.Add(ADMIN, temp_ptr->GetString());
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Immunity Groups 
		temp_ptr = kv_ptr->FindKey("immunitygroups", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					if (group_list.Find(IMMUNITY, temp_ptr->GetString()))
					{
						client_ptr->group_list.Add(IMMUNITY, temp_ptr->GetString());
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// do the admin flags 
		temp_ptr = kv_ptr->FindKey("adminflags", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					Q_strcpy(flag_string, temp_ptr->GetString());
					if (!FStrEq("", flag_string))
					{
						int flag_string_index = 0;

						while(flag_string[flag_string_index] != '\0')
						{
							char *flag_id = this->SplitFlagString(flag_string, &flag_string_index);
							if (flag_id != NULL)
							{
								if (flag_desc_list.IsValidFlag(ADMIN, flag_id))
								{
									// Create flag
									client_ptr->personal_flag_list.SetFlag(ADMIN, flag_id, true);
								}
							}
						}
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		// Do the immunity flags
		temp_ptr = kv_ptr->FindKey("immunityflags", false);
		if (temp_ptr)
		{
			temp_ptr = temp_ptr->GetFirstValue();
			if (temp_ptr)
			{
				for (;;)
				{
					Q_strcpy(flag_string, temp_ptr->GetString());
					if (!FStrEq("", flag_string))
					{
						int flag_string_index = 0;

						while(flag_string[flag_string_index] != '\0')
						{
							char *flag_id = this->SplitFlagString(flag_string, &flag_string_index);
							if (flag_id != NULL)
							{
								if (flag_desc_list.IsValidFlag(IMMUNITY, flag_id))
								{
									// Create flag
									client_ptr->personal_flag_list.SetFlag(IMMUNITY, flag_id, true);
								}
							}
						}
					}

					temp_ptr = temp_ptr->GetNextValue();
					if (!temp_ptr)
					{
						break;
					}
				}
			}
		}

		players_ptr = players_ptr->GetNextKey();
		if (!players_ptr)
		{
			break;
		}
	}

	// Calculates each clients unmasked flags (flags without levels applied)
	SetupUnMasked();
}

//---------------------------------------------------------------------------------
// Purpose: Checks index of player to see if they are a client
//---------------------------------------------------------------------------------
bool	ManiClient::IsClient(int id)
{
	if (id < 1 || id > max_players) return false;
	return ((active_client_list[id - 1] == NULL) ? false:true);
}

//---------------------------------------------------------------------------------
// Purpose: Finds a user by name and updates their user_id 
//---------------------------------------------------------------------------------
void	ManiClient::UpdateClientUserID(const int user_id, const char *name)
{
	for (int i = 0; i != (int) c_list.size(); i++)
	{
		if (strcmp(c_list[i]->GetName(), name) == 0)
		{
			c_list[i]->SetUserID(user_id);
			break;
		}
	}
}

//---------------------------------------------------------------------------------
// Loads the simple IP list into a ManiKeyValues structure
//---------------------------------------------------------------------------------
bool ManiClient::LoadIPList() {
	char	core_filename[256];
	KeyValues *kv_ptr;

	kv_ptr = new KeyValues("client_ip_history.txt");
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/client_ip_history.txt", mani_path.GetString());

	ip_list.clear();
	Q_memset( &ip_list, 0, sizeof (ip_list) );

	if ( !kv_ptr->LoadFromFile(filesystem, core_filename) ) {
		MMsg("Failed to load %s\n", core_filename);
		kv_ptr->deleteThis();
		return false;
	}

	KeyValues *acct_type = kv_ptr->GetFirstSubKey(); // A = Admin, R = Reserved Slot
	while ( acct_type ) {
		bool admin = ( FStrEq(acct_type->GetName(), "a" ) );
		KeyValues *steam_info = acct_type->GetFirstSubKey();
		while ( steam_info ) {
			IPClient *client = new IPClient(steam_info->GetName(), admin );
			ip_list.push_back(client);
			KeyValues *data = steam_info->GetFirstValue();
			while ( data ) {
				client->AddIP(data->GetName(), data->GetInt());
				data = data->GetNextValue();
			}
			steam_info = steam_info->GetNextKey();
		}
		acct_type = acct_type->GetNextKey();
	}

	return true;
}

//---------------------------------------------------------------------------------
// Writes the simple IP list into a ManiKeyValues structure
//---------------------------------------------------------------------------------
bool ManiClient::WriteIPList() {
	char	core_filename[256];
	

	KeyValues *kv_ptr = new KeyValues("client_ip_history.txt");
	KeyValues *admin = new KeyValues("A");
	KeyValues *reserved = new KeyValues("R");
	
	snprintf(core_filename, sizeof (core_filename), "./cfg/%s/data/client_ip_history.txt", mani_path.GetString());
	
	KeyValues *client = NULL;
	CleanupIPList( 7 );
	for ( int i = 0; i < (int)ip_list.size(); i++ ) {
		if ( ip_list[i]->int_ip_list.size() == 0 )
			continue;
		client = new KeyValues(ip_list[i]->_steamid);
		for ( int x = 0; x < (int)ip_list[i]->int_ip_list.size(); x++ ) {
			client->SetInt(ip_list[i]->int_ip_list[x].ip, ip_list[i]->int_ip_list[x].last_played);
		}
		if ( ip_list[i]->_admin )
			admin->AddSubKey(client);			
		else 
			reserved->AddSubKey(client);

	}
	kv_ptr->AddSubKey(admin);
	kv_ptr->AddSubKey(reserved);
	kv_ptr->SaveToFile( filesystem, core_filename );
	return true;
}

//---------------------------------------------------------------------------------
// Cleans stale records from the IP list
//---------------------------------------------------------------------------------
int ManiClient::CleanupIPList(int days) {
	int count = 0;

	for ( int i = 0; i < (int)ip_list.size(); i++ )
		count += ip_list[i]->RemoveStale(days);

	return count;
}

//---------------------------------------------------------------------------------
// Menus for client admin
//---------------------------------------------------------------------------------
int ClientOrGroupItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("client", sub_option) == 0)
	{
		MENUPAGE_CREATE(ClientOptionPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("group", sub_option) == 0)
	{
		MENUPAGE_CREATE(GroupOptionPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("level", sub_option) == 0)
	{
		MENUPAGE_CREATE(LevelOptionPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientOrGroupPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2601));
	this->SetTitle("%s", Translate(player_ptr, 2602));

	MENUOPTION_CREATE(ClientOrGroupItem, 2603, client);
	MENUOPTION_CREATE(ClientOrGroupItem, 2604, group);
	MENUOPTION_CREATE(ClientOrGroupItem, 2605, level);
	return true;
}

//---------------------------------------------------------------------------------
int GroupOptionItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("update", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(GroupClassTypePage, player_ptr, AddParam("sub_option", "update"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ChooseClassTypePage, player_ptr, AddParam("sub_option", "add_group_type"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove", sub_option) == 0)
	{
		MENUPAGE_CREATE(GroupRemovePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("client", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(GroupClassTypePage, player_ptr, AddParam("sub_option", "client"), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool GroupOptionPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2610));
	this->SetTitle("%s", Translate(player_ptr, 2611));

	MENUOPTION_CREATE(GroupOptionItem, 2612, update);
	MENUOPTION_CREATE(GroupOptionItem, 2613, add);
	MENUOPTION_CREATE(GroupOptionItem, 2614, remove);
	MENUOPTION_CREATE(GroupOptionItem, 2615, client);
	return true;
}

//---------------------------------------------------------------------------------
int GroupClassTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *class_type;
	char *group_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("group_id", &group_id) ||
		!m_page_ptr->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp(sub_option, "update") == 0)
	{
		MENUPAGE_CREATE_PARAM2(GroupUpdatePage, player_ptr, AddParam("class_type", class_type), AddParam("group_id", group_id), 0, -1);
	}
	else if (strcmp(sub_option, "client") == 0)
	{
		MENUPAGE_CREATE_PARAM2(GroupClientPage, player_ptr, AddParam("class_type", class_type), AddParam("group_id", group_id), 0, -1);
	}

	return NEW_MENU;
}

bool GroupClassTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2630));
	this->SetTitle("%s", Translate(player_ptr, 2631));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStriKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = gpManiClient->group_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = gpManiClient->group_list.FindNext(c_type, &key_value))
		{
			MenuItem *ptr = new GroupClassTypeItem();
			ptr->params.AddParam("class_type", key_value->key1);
			ptr->params.AddParam("group_id", key_value->key2);
			ptr->SetDisplayText("%s -> %s", key_value->key1, key_value->key2);
			this->AddItem(ptr);
		}
	}

	this->SortDisplay();
	return true;
}


//---------------------------------------------------------------------------------
int GroupRemoveItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *group_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("group_id", &group_id)) return CLOSE_MENU;

	gpManiClient->ProcessRemoveGroupType(class_type, player_ptr, group_id);
	return REPOP_MENU;
}

bool GroupRemovePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2710));
	this->SetTitle("%s", Translate(player_ptr, 2711));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStriKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = gpManiClient->group_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = gpManiClient->group_list.FindNext(c_type, &key_value))
		{
			MenuItem *ptr = new GroupRemoveItem();
			ptr->params.AddParam("class_type", key_value->key1);
			ptr->params.AddParam("group_id", key_value->key2);
			ptr->SetDisplayText("%s -> %s", key_value->key1, key_value->key2);
			this->AddItem(ptr);
		}
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int GroupUpdateItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *group_id;
	char *flag_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("group_id", &group_id) ||
		!this->params.GetParam("flag_id", &flag_id)) return CLOSE_MENU;

	gpManiClient->ProcessAddGroupType(class_type, player_ptr, group_id, flag_id);
	return REPOP_MENU;
}

bool GroupUpdatePage::PopulateMenuPage(player_t *player_ptr)
{
	char *class_type;
	char *group_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("group_id", &group_id)) return false;

	GlobalGroupFlag *g_flag = gpManiClient->group_list.Find(class_type, group_id);

	this->SetEscLink("%s", Translate(player_ptr, 2640));
	this->SetTitle("%s", Translate(player_ptr, 2641, "%s%s", class_type, group_id));

	MenuItem *ptr = new GroupUpdateItem();
	ptr->params.AddParam("class_type", class_type);
	ptr->params.AddParam("group_id", group_id);
	ptr->params.AddParam("flag_id", "+#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2642));
	ptr->SetHiddenText("1");
	this->AddItem(ptr);

	ptr = new GroupUpdateItem();
	ptr->params.AddParam("class_type", class_type);
	ptr->params.AddParam("group_id", group_id);
	ptr->params.AddParam("flag_id", "-#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2643));
	ptr->SetHiddenText("2");
	this->AddItem(ptr);

	// Search for flags we can use for this type
	const DualStrKey *key_value = NULL;
	for (const char *desc = gpManiClient->flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = gpManiClient->flag_desc_list.FindNext(class_type, &key_value))
	{
		if (strcmp(key_value->key1, class_type) == 0)
		{
			ptr = new GroupUpdateItem();

			if (g_flag && g_flag->IsFlagSet(key_value->key2))
			{
				ptr->SetDisplayText("* %s", desc);
				ptr->params.AddParamVar("flag_id", "-%s", key_value->key2);
			}
			else
			{
				ptr->SetDisplayText("%s", desc);
				ptr->params.AddParamVar("flag_id", "+%s", key_value->key2);
			}

			ptr->params.AddParam("class_type", class_type);
			ptr->params.AddParam("group_id", group_id);
			ptr->SetHiddenText("%s", desc);
			this->AddItem(ptr);
		}
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
int LevelOptionItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("update", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(LevelClassTypePage, player_ptr, AddParam("sub_option", "update"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ChooseClassTypePage, player_ptr, AddParam("sub_option", "add_level_type"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove", sub_option) == 0)
	{
		MENUPAGE_CREATE(LevelRemovePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("client", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(LevelClassTypePage, player_ptr, AddParam("sub_option", "client"), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool LevelOptionPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2620));
	this->SetTitle("%s", Translate(player_ptr, 2621));

	MENUOPTION_CREATE(LevelOptionItem, 2622, update);
	MENUOPTION_CREATE(LevelOptionItem, 2623, add);
	MENUOPTION_CREATE(LevelOptionItem, 2624, remove);
	MENUOPTION_CREATE(LevelOptionItem, 2625, client);
	return true;
}

//---------------------------------------------------------------------------------
int LevelClassTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *sub_option;
	int	 level_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("level_id", &level_id) ||
		!m_page_ptr->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp(sub_option, "update") == 0)
	{
		MENUPAGE_CREATE_PARAM2(LevelUpdatePage, player_ptr, AddParam("class_type", class_type), AddParam("level_id", level_id), 0, -1);
	}
	else if (strcmp(sub_option, "client") == 0)
	{
		MENUPAGE_CREATE_PARAM2(LevelClientPage, player_ptr, AddParam("class_type", class_type), AddParam("level_id", level_id), 0, -1);
	}

	return NEW_MENU;
}

bool LevelClassTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2650));
	this->SetTitle("%s", Translate(player_ptr, 2651));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStrIntKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = gpManiClient->level_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = gpManiClient->level_list.FindNext(c_type, &key_value))
		{
			MenuItem *ptr = new LevelClassTypeItem();
			ptr->params.AddParam("class_type", key_value->key1);
			ptr->params.AddParam("level_id", key_value->key2);
			ptr->SetDisplayText("%s -> %i", key_value->key1, key_value->key2);
			this->AddItem(ptr);
		}
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int LevelClientItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *name;
	bool add;
	int	 level_id;

	if (!m_page_ptr->params.GetParam("class_type", &class_type) ||
		!m_page_ptr->params.GetParam("level_id", &level_id) ||
		!this->params.GetParam("add", &add) ||
		!this->params.GetParam("name", &name)) return CLOSE_MENU;

	char level_id_str[16];
	snprintf(level_id_str, sizeof(level_id_str), "%i", level_id);

	if (add)
	{
		gpManiClient->ProcessSetLevel(class_type, player_ptr, name, level_id_str);
	}
	else
	{
		gpManiClient->ProcessSetLevel(class_type, player_ptr, name, "-1");
	}

	return REPOP_MENU;
}

bool LevelClientPage::PopulateMenuPage(player_t *player_ptr)
{
	char *class_type;
	int	level_id;

	this->params.GetParam("class_type", &class_type);
	this->params.GetParam("level_id", &level_id);

	this->SetEscLink("%s", Translate(player_ptr, 2730));
	this->SetTitle("%s", Translate(player_ptr, 2731, "%s%i", class_type, level_id));

	MenuItem *ptr;
	for (int i = 0; i != (int) gpManiClient->c_list.size(); i ++)
	{
		ClientPlayer *c_ptr = gpManiClient->c_list[i];
		ptr = new LevelClientItem();

		int old_level_id = c_ptr->level_list.FindFirst(class_type);
		if (old_level_id != -99999)
		{
			ptr->SetDisplayText("* %s -> Level %i", c_ptr->name.str, old_level_id);
			ptr->params.AddParam("add", false);
		}
		else
		{
			ptr->SetDisplayText("%s", c_ptr->name.str);
			ptr->params.AddParam("add", true);
		}

		ptr->params.AddParam("name", c_ptr->name.str);
		ptr->SetHiddenText("%s", c_ptr->name.str);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
int GroupClientItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *name;
	bool add;
	char *group_id;

	if (!m_page_ptr->params.GetParam("class_type", &class_type) ||
		!m_page_ptr->params.GetParam("group_id", &group_id) ||
		!this->params.GetParam("add", &add) ||
		!this->params.GetParam("name", &name)) return CLOSE_MENU;

	if (add)
	{
		gpManiClient->ProcessAddGroup(class_type, player_ptr, name, group_id);
	}
	else
	{
		gpManiClient->ProcessRemoveGroup(class_type, player_ptr, name, group_id);
	}

	return REPOP_MENU;
}

bool GroupClientPage::PopulateMenuPage(player_t *player_ptr)
{
	char *class_type;
	char *group_id;

	this->params.GetParam("class_type", &class_type);
	this->params.GetParam("group_id", &group_id);

	this->SetEscLink("%s", Translate(player_ptr, 2720));
	this->SetTitle("%s", Translate(player_ptr, 2721, "%s%s", class_type, group_id));

	MenuItem *ptr;
	for (int i = 0; i != (int) gpManiClient->c_list.size(); i ++)
	{
		ClientPlayer *c_ptr = gpManiClient->c_list[i];
		ptr = new GroupClientItem();

		if (c_ptr->group_list.Find(class_type, group_id))
		{
			ptr->SetDisplayText("* %s", c_ptr->name.str);
			ptr->params.AddParam("add", false);
		}
		else
		{
			ptr->SetDisplayText("%s", c_ptr->name.str);
			ptr->params.AddParam("add", true);
		}

		ptr->params.AddParam("name", c_ptr->name.str);
		ptr->SetHiddenText("%s", c_ptr->name.str);
		this->AddItem(ptr);
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
int LevelRemoveItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	int	 level_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("level_id", &level_id)) return CLOSE_MENU;

	char level_id_str[16];

	snprintf(level_id_str, sizeof(level_id_str), "%i", level_id);
	gpManiClient->ProcessRemoveLevelType(class_type, player_ptr, level_id_str);
	return REPOP_MENU;
}

bool LevelRemovePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2701));
	this->SetTitle("%s", Translate(player_ptr, 2701));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		const DualStrIntKey *key_value = NULL;
		for (GlobalGroupFlag *g_flag = gpManiClient->level_list.FindFirst(c_type, &key_value); g_flag != NULL; g_flag = gpManiClient->level_list.FindNext(c_type, &key_value))
		{
			MenuItem *ptr = new LevelClassTypeItem();
			ptr->params.AddParam("class_type", key_value->key1);
			ptr->params.AddParam("level_id", key_value->key2);
			ptr->SetDisplayText("%s -> %i", key_value->key1, key_value->key2);
			this->AddItem(ptr);
		}
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int LevelUpdateItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	int	level_id;
	char *flag_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("level_id", &level_id) ||
		!this->params.GetParam("flag_id", &flag_id)) return CLOSE_MENU;

	char level_id_str[16];

	snprintf(level_id_str, sizeof(level_id_str), "%i", level_id);
	gpManiClient->ProcessAddLevelType(class_type, player_ptr, level_id_str, flag_id);
	return REPOP_MENU;
}

bool LevelUpdatePage::PopulateMenuPage(player_t *player_ptr)
{
	char *class_type;
	int	level_id;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("level_id", &level_id)) return false;

	GlobalGroupFlag *g_flag = gpManiClient->level_list.Find(class_type, level_id);

	this->SetEscLink("%s", Translate(player_ptr, 2660));
	this->SetTitle("%s", Translate(player_ptr, 2661, "%s%i", class_type, level_id));

	MenuItem *ptr = new LevelUpdateItem();
	ptr->params.AddParam("class_type", class_type);
	ptr->params.AddParam("level_id", level_id);
	ptr->params.AddParam("flag_id", "+#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2642));
	ptr->SetHiddenText("1");
	this->AddItem(ptr);

	ptr = new LevelUpdateItem();
	ptr->params.AddParam("class_type", class_type);
	ptr->params.AddParam("level_id", level_id);
	ptr->params.AddParam("flag_id", "-#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2643));
	ptr->SetHiddenText("2");
	this->AddItem(ptr);

	// Search for flags we can use for this type
	const DualStrKey *key_value = NULL;
	for (const char *desc = gpManiClient->flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = gpManiClient->flag_desc_list.FindNext(class_type, &key_value))
	{
		if (strcmp(key_value->key1, class_type) == 0)
		{
			ptr = new LevelUpdateItem();

			if (g_flag && g_flag->IsFlagSet(key_value->key2))
			{
				ptr->SetDisplayText("* %s", desc);
				ptr->params.AddParamVar("flag_id", "-%s", key_value->key2);
			}
			else
			{
				ptr->SetDisplayText("%s", desc);
				ptr->params.AddParamVar("flag_id", "+%s", key_value->key2);
			}

			ptr->params.AddParam("class_type", class_type);
			ptr->params.AddParam("level_id", level_id);
			ptr->SetHiddenText("%s", desc);
			this->AddItem(ptr);
		}
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
int ChooseClassTypeItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *sub_option;

	if (!this->params.GetParam("class_type", &class_type) ||
		!m_page_ptr->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp(sub_option, "add_level_type") == 0)
	{
		INPUTPAGE_CREATE_PARAM(CreateLevelPage, player_ptr, AddParam("class_type", class_type), 0, -1);
	}
	else if (strcmp(sub_option, "add_group_type") == 0)
	{
		INPUTPAGE_CREATE_PARAM(CreateGroupPage, player_ptr, AddParam("class_type", class_type), 0, -1);
	}

	return NEW_MENU;
}

bool ChooseClassTypePage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2670));
	this->SetTitle("%s", Translate(player_ptr, 2671));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		MenuItem *ptr = new ChooseClassTypeItem();
		ptr->params.AddParam("class_type", c_type);
		ptr->SetDisplayText("%s", c_type);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int CreateLevelItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;

	if (!m_page_ptr->params.GetParam("class_type", &class_type)) return CLOSE_MENU;

	gpManiClient->ProcessAddLevelType(class_type, player_ptr, (char *) gpCmd->Cmd_Args(), "");
	return PREVIOUS_MENU;
}

bool CreateLevelPage::PopulateMenuPage(player_t *player_ptr)
{
	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2680));
	this->SetTitle("%s", Translate(player_ptr, 2681));

	MenuItem *ptr = new CreateLevelItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int CreateGroupItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;

	if (!m_page_ptr->params.GetParam("class_type", &class_type)) return CLOSE_MENU;

	gpManiClient->ProcessAddGroupType(class_type, player_ptr, (char *) gpCmd->Cmd_Args(), "");
	return PREVIOUS_MENU;
}

bool CreateGroupPage::PopulateMenuPage(player_t *player_ptr)
{
	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2690));
	this->SetTitle("%s", Translate(player_ptr, 2691));

	MenuItem *ptr = new CreateGroupItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int ClientOptionItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("update", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(SelectClientPage, player_ptr, AddParam("sub_option", "update"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(AddClientOptionPage, player_ptr, AddParam("sub_option", "add"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(SelectClientPage, player_ptr, AddParam("sub_option", "remove"), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("show", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(SelectClientPage, player_ptr, AddParam("sub_option", "show"), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientOptionPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2740));
	this->SetTitle("%s", Translate(player_ptr, 2741));

	MENUOPTION_CREATE(ClientOptionItem, 2742, update);
	MENUOPTION_CREATE(ClientOptionItem, 2743, add);
	MENUOPTION_CREATE(ClientOptionItem, 2744, remove);
	MENUOPTION_CREATE(ClientOptionItem, 2745, show);
	return true;
}

//---------------------------------------------------------------------------------
int SelectClientItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("name", &name) ||
		!m_page_ptr->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp(sub_option, "update") == 0)
	{
		MENUPAGE_CREATE_PARAM(ClientUpdateOptionPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp(sub_option, "remove") == 0)
	{
		gpManiClient->ProcessRemoveClient(player_ptr, name);
		return REPOP_MENU;
	}
	else if (strcmp(sub_option, "show") == 0)
	{
		gpManiClient->ProcessClientStatus(player_ptr, name);
		return REPOP_MENU;
	}

	return CLOSE_MENU;
}

bool SelectClientPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2750));
	this->SetTitle("%s", Translate(player_ptr, 2751));

	for (int i = 0; i != (int) gpManiClient->c_list.size(); i++)
	{
		ClientPlayer *c_ptr = gpManiClient->c_list[i];

		MenuItem *ptr = new SelectClientItem();
		ptr->params.AddParam("name", c_ptr->name.str);
		ptr->SetDisplayText("%s", c_ptr->name.str);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int ClientUpdateOptionItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("sub_option", &sub_option) ||
		!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	if (strcmp("set_name", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ClientNamePage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add_steam", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ClientSteamPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("set_flags", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(SetPersonalClassPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove_steam", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(RemoveSteamPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add_ip", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ClientIPPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove_ip", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(RemoveIPPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("add_nick", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(ClientNickPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove_nick", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(RemoveNickPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("set_password", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(SetPasswordPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("remove_password", sub_option) == 0)
	{
		gpManiClient->ProcessSetPassword(player_ptr, name, "");
		return REPOP_MENU;
	}
	else if (strcmp("set_email", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetEmailPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("set_notes", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetNotesPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientUpdateOptionPage::PopulateMenuPage(player_t *player_ptr)
{
	char temp_string[128];
	char *name;
	this->params.GetParam("name", &name);

	int client_index = gpManiClient->FindClientIndex(name);
	if (client_index == -1)
	{
		return false;
	}

	this->SetEscLink("%s", Translate(player_ptr, 2760));
	this->SetTitle("%s", Translate(player_ptr, 2761, "%s", name));

	MENUOPTION_CREATE(ClientUpdateOptionItem, 2773, set_flags);
	MENUOPTION_CREATE(ClientUpdateOptionItem, 2762, set_name);

	ClientPlayer *c_ptr = gpManiClient->c_list[client_index];

	MenuItem *ptr = new ClientUpdateOptionItem();

	Q_strcpy(temp_string, "");
	int l_size = (int) c_ptr->steam_list.Size();
	if (l_size != 0)
	{
		if (l_size > 1)
		{
			snprintf(temp_string, sizeof(temp_string), " | %s,...", c_ptr->steam_list.FindFirst());
		}
		else
		{
			snprintf(temp_string, sizeof(temp_string), " | %s", c_ptr->steam_list.FindFirst());
		}
	}

	ptr->params.AddParam("sub_option", "add_steam");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2763), temp_string);
	this->AddItem(ptr);

	MENUOPTION_CREATE(ClientUpdateOptionItem, 2764, remove_steam);

	ptr = new ClientUpdateOptionItem();
	Q_strcpy(temp_string, "");
	l_size = c_ptr->ip_address_list.Size();
	if (l_size != 0)
	{
		if (l_size > 1)
		{
			snprintf(temp_string, sizeof(temp_string), " | %s,...", c_ptr->ip_address_list.FindFirst());
		}
		else
		{
			snprintf(temp_string, sizeof(temp_string), " | %s", c_ptr->ip_address_list.FindFirst());
		}
	}

	ptr->params.AddParam("sub_option", "add_ip");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2765), temp_string);
	this->AddItem(ptr);

	MENUOPTION_CREATE(ClientUpdateOptionItem, 2766, remove_ip);

	ptr = new ClientUpdateOptionItem();
	Q_strcpy(temp_string, "");
	l_size = c_ptr->nick_list.Size();
	if (l_size != 0)
	{
		if (l_size > 1)
		{
			snprintf(temp_string, sizeof(temp_string), " | %s,...", c_ptr->nick_list.FindFirst());
		}
		else
		{
			snprintf(temp_string, sizeof(temp_string), " | %s", c_ptr->nick_list.FindFirst());
		}
	}

	ptr->params.AddParam("sub_option", "add_nick");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2767), temp_string);
	this->AddItem(ptr);

	MENUOPTION_CREATE(ClientUpdateOptionItem, 2768, remove_nick);

	ptr = new ClientUpdateOptionItem();
	Q_strcpy(temp_string, "");
	if (c_ptr->password.str && strcmp(c_ptr->password.str, "") != 0)
	{
		if (snprintf(temp_string, 15, " | %s", c_ptr->password.str) == 15)
		{
			strcat(temp_string, "...");
		}
	}
	
	ptr->params.AddParam("sub_option", "set_password");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2769), temp_string);
	this->AddItem(ptr);

	MENUOPTION_CREATE(ClientUpdateOptionItem, 2770, remove_password);

	ptr = new ClientUpdateOptionItem();
	Q_strcpy(temp_string, "");
	if (c_ptr->email_address.str && strcmp(c_ptr->email_address.str, "") != 0)
	{
		if (snprintf(temp_string, 15, " | %s", c_ptr->email_address.str) == 15)
		{
			strcat(temp_string, "...");
		}
	}
	
	ptr->params.AddParam("sub_option", "set_email");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2771), temp_string);
	this->AddItem(ptr);

	ptr = new ClientUpdateOptionItem();
	Q_strcpy(temp_string, "");
	if (c_ptr->notes.str && strcmp(c_ptr->notes.str, "") != 0)
	{
		if (snprintf(temp_string, 15, " | %s", c_ptr->notes.str) == 15)
		{
			strcat(temp_string, "...");
		}
	}
	
	ptr->params.AddParam("sub_option", "set_notes");
	ptr->SetDisplayText("%s%s", Translate(player_ptr, 2772), temp_string);
	this->AddItem(ptr);

	return true;
}

//---------------------------------------------------------------------------------
int SetNotesItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	gpManiClient->ProcessSetNotes(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetNotesPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2840));
	this->SetTitle("%s", Translate(player_ptr, 2841, "%s", name));

	MenuItem *ptr = new SetNotesItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int SetEmailItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	gpManiClient->ProcessSetEmail(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetEmailPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2830));
	this->SetTitle("%s", Translate(player_ptr, 2831, "%s", name));

	MenuItem *ptr = new SetEmailItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int SetPasswordItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	gpManiClient->ProcessSetPassword(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetPasswordPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2820));
	this->SetTitle("%s", Translate(player_ptr, 2821, "%s", name));

	MenuItem *ptr = new SetPasswordItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int ClientNameItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("sub_option", &sub_option) ||
		!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	if (strcmp("type_name", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetNamePage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("player", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(PlayerNamePage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientNamePage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2850));
	this->SetTitle("%s", Translate(player_ptr, 2851, "%s", name));

	MENUOPTION_CREATE(ClientNameItem, 2852, type_name);
	MENUOPTION_CREATE(ClientNameItem, 2853, player);
	return true;
}

//---------------------------------------------------------------------------------
int SetNameItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	gpManiClient->ProcessSetName(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetNamePage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2780));
	this->SetTitle("%s", Translate(player_ptr, 2781, "%s", name));

	MenuItem *ptr = new SetNameItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int PlayerNameItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	int user_id;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("user_id", &user_id)) return CLOSE_MENU;

	player_t player;
	player.user_id = user_id;
	if (!FindPlayerByUserID(&player)) return PREVIOUS_MENU;
	gpManiClient->ProcessSetName(player_ptr, name, player.name);
	return PREVIOUS_MENU;
}

bool PlayerNamePage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2920));
	this->SetTitle("%s", Translate(player_ptr, 2921, "%s", name));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;

		// Ignore unicode players
		bool found_unicode = false;
		for (int j = 0; j < Q_strlen(player.name); j++)
		{
			if (player.name[j] & 0x80)
			{
				found_unicode = true;
				break;
			}
		}

		if (found_unicode) continue;

		MenuItem *ptr = new PlayerNameItem();
		ptr->params.AddParam("user_id", player.user_id);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int ClientSteamItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("sub_option", &sub_option) ||
		!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	if (strcmp("type_name", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetSteamPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("player", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(PlayerSteamPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientSteamPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2860));
	this->SetTitle("%s", Translate(player_ptr, 2861, "%s", name));

	MENUOPTION_CREATE(ClientSteamItem, 2862, type_name);
	MENUOPTION_CREATE(ClientSteamItem, 2863, player);
	return true;
}

//---------------------------------------------------------------------------------
int SetSteamItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;
	gpManiClient->ProcessAddSteam(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetSteamPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2790));
	this->SetTitle("%s", Translate(player_ptr, 2791, "%s", name));

	MenuItem *ptr = new SetSteamItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int PlayerSteamItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *steam_id;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("steam_id", &steam_id)) return CLOSE_MENU;

	gpManiClient->ProcessAddSteam(player_ptr, name, steam_id);
	return PREVIOUS_MENU;
}

bool PlayerSteamPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2920));
	this->SetTitle("%s", Translate(player_ptr, 2921, "%s", name));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (strcmp(player.steam_id, "STEAM_ID_PENDING") == 0) continue;
		if (strcmp(player.steam_id, "STEAM_ID_LAN") == 0) continue;
		MenuItem *ptr = new PlayerSteamItem();
		ptr->params.AddParam("steam_id", player.steam_id);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int ClientIPItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("sub_option", &sub_option) ||
		!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	if (strcmp("type_name", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetIPPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("player", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(PlayerIPPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientIPPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2870));
	this->SetTitle("%s", Translate(player_ptr, 2871, "%s", name));

	MENUOPTION_CREATE(ClientIPItem, 2872, type_name);
	MENUOPTION_CREATE(ClientIPItem, 2873, player);
	return true;
}

//---------------------------------------------------------------------------------
int SetIPItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;
	gpManiClient->ProcessAddIP(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetIPPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2800));
	this->SetTitle("%s", Translate(player_ptr, 2801, "%s", name));

	MenuItem *ptr = new SetIPItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int PlayerIPItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *ip;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("ip", &ip)) return CLOSE_MENU;

	gpManiClient->ProcessAddIP(player_ptr, name, ip);
	return PREVIOUS_MENU;
}

bool PlayerIPPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2920));
	this->SetTitle("%s", Translate(player_ptr, 2921, "%s", name));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		MenuItem *ptr = new PlayerIPItem();
		ptr->params.AddParam("ip", player.ip_address);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int ClientNickItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;
	char *name;

	if (!this->params.GetParam("sub_option", &sub_option) ||
		!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;

	if (strcmp("type_name", sub_option) == 0)
	{
		INPUTPAGE_CREATE_PARAM(SetNickPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("player", sub_option) == 0)
	{
		MENUPAGE_CREATE_PARAM(PlayerNickPage, player_ptr, AddParam("name", name), 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool ClientNickPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2880));
	this->SetTitle("%s", Translate(player_ptr, 2881, "%s", name));

	MENUOPTION_CREATE(ClientNickItem, 2882, type_name);
	MENUOPTION_CREATE(ClientNickItem, 2883, player);
	return true;
}

//---------------------------------------------------------------------------------
int SetNickItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!m_page_ptr->params.GetParam("name", &name)) return CLOSE_MENU;
	gpManiClient->ProcessAddNick(player_ptr, name, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool SetNickPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return false;

	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2810));
	this->SetTitle("%s", Translate(player_ptr, 2811, "%s", name));

	MenuItem *ptr = new SetNickItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int PlayerNickItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *nick;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("nick", &nick)) return CLOSE_MENU;

	gpManiClient->ProcessAddNick(player_ptr, name, nick);
	return PREVIOUS_MENU;
}

bool PlayerNickPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2920));
	this->SetTitle("%s", Translate(player_ptr, 2921, "%s", name));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		MenuItem *ptr = new PlayerNickItem();
		ptr->params.AddParam("nick", player.name);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int RemoveSteamItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *steam_id;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("steam_id", &steam_id)) return CLOSE_MENU;

	gpManiClient->ProcessRemoveSteam(player_ptr, name, steam_id);
	return PREVIOUS_MENU;
}

bool RemoveSteamPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2890));
	this->SetTitle("%s", Translate(player_ptr, 2891, "%s", name));

	int client_index = gpManiClient->FindClientIndex(name);
	if (client_index == -1) return false;

	ClientPlayer *c_ptr = gpManiClient->c_list[client_index];

	for (const char *steam_id = c_ptr->steam_list.FindFirst(); steam_id != NULL; steam_id = c_ptr->steam_list.FindNext())
	{
		MenuItem *ptr = new RemoveSteamItem();
		ptr->params.AddParam("steam_id", steam_id);
		ptr->SetDisplayText("%s", steam_id);
		this->AddItem(ptr);
	}	

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int RemoveIPItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *ip;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("ip", &ip)) return CLOSE_MENU;

	gpManiClient->ProcessRemoveIP(player_ptr, name, ip);
	return PREVIOUS_MENU;
}

bool RemoveIPPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2900));
	this->SetTitle("%s", Translate(player_ptr, 2901, "%s", name));

	int client_index = gpManiClient->FindClientIndex(name);
	if (client_index == -1) return false;

	ClientPlayer *c_ptr = gpManiClient->c_list[client_index];

	for (const char *ip = c_ptr->ip_address_list.FindFirst(); ip != NULL; ip = c_ptr->ip_address_list.FindNext())
	{
		MenuItem *ptr = new RemoveIPItem();
		ptr->params.AddParam("ip", ip);
		ptr->SetDisplayText("%s", ip);
		this->AddItem(ptr);
	}	

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int RemoveNickItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *nick;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("nick", &nick)) return CLOSE_MENU;

	gpManiClient->ProcessRemoveNick(player_ptr, name, nick);
	return PREVIOUS_MENU;
}

bool RemoveNickPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2910));
	this->SetTitle("%s", Translate(player_ptr, 2911, "%s", name));

	int client_index = gpManiClient->FindClientIndex(name);
	if (client_index == -1) return false;

	ClientPlayer *c_ptr = gpManiClient->c_list[client_index];

	for (const char *nick = c_ptr->nick_list.FindFirst(); nick != NULL; nick = c_ptr->nick_list.FindNext())
	{
		MenuItem *ptr = new RemoveNickItem();
		ptr->params.AddParam("nick", nick);
		ptr->SetDisplayText("%s", nick);
		this->AddItem(ptr);
	}	

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int SetPersonalClassItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *class_type;

	if (!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("class_type", &class_type)) return CLOSE_MENU;

	MENUPAGE_CREATE_PARAM2(SetPersonalFlagPage, player_ptr, AddParam("class_type", class_type), AddParam("name", name), 0, -1);
	return NEW_MENU;
}

bool SetPersonalClassPage::PopulateMenuPage(player_t *player_ptr)
{
	char *name;
	this->params.GetParam("name", &name);

	this->SetEscLink("%s", Translate(player_ptr, 2930));
	this->SetTitle("%s", Translate(player_ptr, 2931, "%s", name));

	for (const char *c_type = class_type_list.FindFirst(); c_type != NULL; c_type = class_type_list.FindNext())
	{
		MenuItem *ptr = new SetPersonalClassItem();
		ptr->params.AddParam("class_type", c_type);
		ptr->params.AddParam("name", name);
		ptr->SetDisplayText("%s", c_type);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int SetPersonalFlagItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *class_type;
	char *name;
	char *flag_id;

	if (!m_page_ptr->params.GetParam("class_type", &class_type) ||
		!m_page_ptr->params.GetParam("name", &name) ||
		!this->params.GetParam("flag_id", &flag_id)) return CLOSE_MENU;

	gpManiClient->ProcessSetFlag(class_type, player_ptr, name, flag_id);
	return REPOP_MENU;
}

bool SetPersonalFlagPage::PopulateMenuPage(player_t *player_ptr)
{
	char *class_type;
	char *name;

	if (!this->params.GetParam("class_type", &class_type) ||
		!this->params.GetParam("name", &name)) return false;

	int client_index = gpManiClient->FindClientIndex(name);
	if (client_index == -1) return false;

	ClientPlayer *c_ptr = gpManiClient->c_list[client_index];

	this->SetEscLink("%s", Translate(player_ptr, 2940));
	this->SetTitle("%s", Translate(player_ptr, 2941, "%s%s", name, class_type));

	MenuItem *ptr = new SetPersonalFlagItem();
	ptr->params.AddParam("flag_id", "+#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2642));
	ptr->SetHiddenText("1");
	this->AddItem(ptr);

	ptr = new SetPersonalFlagItem();
	ptr->params.AddParam("flag_id", "-#");
	ptr->SetDisplayText("%s", Translate(player_ptr, 2643));
	ptr->SetHiddenText("2");
	this->AddItem(ptr);

	// Search for flags we can use for this type
	const DualStrKey *key_value = NULL;
	for (const char *desc = gpManiClient->flag_desc_list.FindFirst(class_type, &key_value); desc != NULL; desc = gpManiClient->flag_desc_list.FindNext(class_type, &key_value))
	{
		if (strcmp(key_value->key1, class_type) == 0)
		{
			ptr = new SetPersonalFlagItem();

			if (c_ptr->personal_flag_list.IsFlagSet(class_type, key_value->key2))
			{
				ptr->SetDisplayText("* %s", desc);
				ptr->params.AddParamVar("flag_id", "-%s", key_value->key2);
			}
			else
			{
				ptr->SetDisplayText("%s", desc);
				ptr->params.AddParamVar("flag_id", "+%s", key_value->key2);
			}

			ptr->SetHiddenText("%s", desc);
			this->AddItem(ptr);
		}
	}

	this->SortHidden();
	return true;
}

//---------------------------------------------------------------------------------
int AddClientOptionItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *sub_option;

	if (!this->params.GetParam("sub_option", &sub_option)) return CLOSE_MENU;

	if (strcmp("manual", sub_option) == 0)
	{
		INPUTPAGE_CREATE(NewNamePage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("player", sub_option) == 0)
	{
		MENUPAGE_CREATE(NameOnServerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("steam", sub_option) == 0)
	{
		MENUPAGE_CREATE(SteamOnServerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}
	else if (strcmp("ip", sub_option) == 0)
	{
		MENUPAGE_CREATE(IPOnServerPage, player_ptr, 0, -1);
		return NEW_MENU;
	}

	return CLOSE_MENU;
}

bool AddClientOptionPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2950));
	this->SetTitle("%s", Translate(player_ptr, 2951));

	MENUOPTION_CREATE(AddClientOptionItem, 2952, manual);
	MENUOPTION_CREATE(AddClientOptionItem, 2953, player);
	MENUOPTION_CREATE(AddClientOptionItem, 2954, steam);
	MENUOPTION_CREATE(AddClientOptionItem, 2955, ip);
	return true;
}

//---------------------------------------------------------------------------------
int NewNameItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	gpManiClient->ProcessAddClient(player_ptr, (char *) gpCmd->Cmd_Args());
	return PREVIOUS_MENU;
}

bool NewNamePage::PopulateMenuPage(player_t *player_ptr)
{
	// This is an input object
	this->SetEscLink("%s", Translate(player_ptr, 2960));
	this->SetTitle("%s", Translate(player_ptr, 2961));

	MenuItem *ptr = new NewNameItem();
	this->AddItem(ptr);
	return true;
}

//---------------------------------------------------------------------------------
int NameOnServerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;

	if (!this->params.GetParam("name", &name)) return CLOSE_MENU;

	gpManiClient->ProcessAddClient(player_ptr, name);
	return PREVIOUS_MENU;
}

bool NameOnServerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2970));
	this->SetTitle("%s", Translate(player_ptr, 2971));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		MenuItem *ptr = new NameOnServerItem();
		ptr->params.AddParam("name", player.name);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int SteamOnServerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *steam_id;

	if (!this->params.GetParam("name", &name) ||
		!this->params.GetParam("steam_id", &steam_id)) return CLOSE_MENU;

	gpManiClient->ProcessAddClient(player_ptr, name);
	gpManiClient->ProcessAddSteam(player_ptr, name, steam_id);
	return PREVIOUS_MENU;
}

bool SteamOnServerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2980));
	this->SetTitle("%s", Translate(player_ptr, 2981));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		if (strcmp(player.steam_id, "STEAM_ID_PENDING") == 0) continue;
		if (strcmp(player.steam_id, "STEAM_ID_LAN") == 0) continue;
		MenuItem *ptr = new SteamOnServerItem();
		ptr->params.AddParam("name", player.name);
		ptr->params.AddParam("steam_id", player.steam_id);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

//---------------------------------------------------------------------------------
int IPOnServerItem::MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr)
{
	char *name;
	char *ip;

	if (!this->params.GetParam("name", &name) ||
		!this->params.GetParam("ip", &ip)) return CLOSE_MENU;

	gpManiClient->ProcessAddClient(player_ptr, name);
	gpManiClient->ProcessAddIP(player_ptr, name, ip);
	return PREVIOUS_MENU;
}

bool IPOnServerPage::PopulateMenuPage(player_t *player_ptr)
{
	this->SetEscLink("%s", Translate(player_ptr, 2990));
	this->SetTitle("%s", Translate(player_ptr, 2991));

	for (int i = 1; i <= max_players; i++)
	{
		player_t player;
		player.index = i;
		if (!FindPlayerByIndex(&player)) continue;
		if (player.is_bot) continue;
		MenuItem *ptr = new IPOnServerItem();
		ptr->params.AddParam("name", player.name);
		ptr->params.AddParam("ip", player.ip_address);
		ptr->SetDisplayText("%s", player.name);
		this->AddItem(ptr);
	}

	this->SortDisplay();
	return true;
}

bool IPClient::SetSteam(const char *steamid) {
	if ( !steamid || (steamid[0] == 0) ) return false;

	Q_memset( _steamid, 0, sizeof(_steamid) );
	Q_strcpy ( _steamid, steamid );
	return true;
}

bool IPClient::AddIP ( const char *ip, int tm ) {
	if ( !ip || (ip[0] == 0 ) ) return false;
	time_t expire = (time_t)(tm);

	IP_entry_t entry;
	Q_memset ( &entry, 0, sizeof(entry) );
	Q_strcpy( entry.ip, ip );
	entry.last_played = expire;
	int_ip_list.push_back(entry);
	return true;
}

int IPClient::RemoveStale(int days) {
	int count = 0;
	int seconds = ( days * 86400 ); // 86400 seconds/day
	time_t now;
	time (&now);

	std::vector<IP_entry_t>::iterator index;	

	for ( index = int_ip_list.begin(); index != int_ip_list.end(); ) {
		if ((index->last_played + seconds ) < now) {
			count++;
			index = int_ip_list.erase(index);
		} else
			index++;
	}
	return count;
}
ManiClient	g_ManiClient;
ManiClient	*gpManiClient;

/*
select c.user_id, c.name, cf.flag_string, cf.type, s.steam_id, n.nick, i.ip_address, cg.group_id, cg.type, cl.level_id, cl.type
from map_client c, map_client_server cs
LEFT JOIN map_steam s ON s.user_id = c.user_id
LEFT JOIN map_client_flag cf ON cf.user_id = c.user_id
LEFT JOIN map_nick n ON n.user_id = c.user_id
LEFT JOIN map_ip i ON i.user_id = c.user_id
LEFT JOIN map_client_group cg ON cg.user_id = c.user_id AND cg.server_group_id = cs.server_group_id
LEFT JOIN map_client_level cl ON cl.user_id = c.user_id AND cl.server_group_id = cs.server_group_id
where c.user_id = cs.user_id
and cs.server_group_id = 1
order by 1, 4, 5,6,7,9, 8, 11, 10
*/


