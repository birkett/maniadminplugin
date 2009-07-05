/* 
* Copyright (C) 2004-2006 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Helping on misc errors/functions: BAILOPAN,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#ifdef SOURCEMM

#ifndef MANI_CLIENT_INTERFACE_H
#define MANI_CLIENT_INTERFACE_H
#include "ISmmPlugin.h"
#include "sourcehook/sh_vector.h"

#define ADMININTERFACE_VERSION 1
#define ADMININTERFACE_MAXACCESSLENGTHTEXT 20		// This is the maximum length of a "flag" access text.

class AdminInterfaceListner
{
public:
	virtual void OnAdminInterfaceUnload()=0;
	virtual void Client_Authorized(int id)=0;
};

typedef struct 
{
	AdminInterfaceListner *ptr;
}AdminInterfaceListnerStruct;

class AdminInterface
{
public:
	virtual bool RegisterFlag(const char *Class,const char *Flag,const char *Description) = 0; // Registers a new admin access
	virtual bool IsClient(int id) = 0;				// returns false if client is bot, or NOT connected
	virtual bool HasFlag(int id, const char *Class, const char *Flag) = 0;	// returns true if the player has this access flag, case sensitive
	virtual bool HasFlag(int id, const char *Flag) = 0;	// returns true if the player has this access flag, case sensitive, Assumes the class type of 'Admin'
	virtual int GetInterfaceVersion() = 0 ;		// Returns the interface version of the admin mod
	virtual const char* GetModName() = 0;			// Returns the name of the current admin mod
	virtual void AddEventListner(AdminInterfaceListner *ptr) = 0; // You should ALLWAYS set this, so you know when the "server"  plugin gets unloaded
	virtual void RemoveListner(AdminInterfaceListner *ptr) = 0;   // You MUST CALL this function in your plugin unloads function, or the admin plugin will crash on next client connect.
};

class ClientInterface : public AdminInterface
{
public:
	bool RegisterFlag(const char *Class,const char *Flag,const char *Description); // Max 1 admin access at the time, returns true if done successfully
	bool IsClient(int id);		// returns false if client is bot, or NOT connected
	bool HasFlag(int id, const char *Class, const char *Flag);	// returns true if the player has this access flag
	bool HasFlag(int id, const char *Flag);	// returns true if the player has this access flag, case sensitive, Assumes the class type of 'Admin'
	int GetInterfaceVersion() { return ADMININTERFACE_VERSION; } // Returns the interface version of the admin mod
	const char* GetModName() { return "Mani Admin Plugin"; }		// Returns the name of the current admin mod
	void AddEventListner(AdminInterfaceListner *ptr); // You should ALLWAYS set this, so you know when the "server"  plugin gets unloaded
	void RemoveListner(AdminInterfaceListner *ptr);
};

#endif
#endif
