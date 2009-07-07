#include "mrecipientfilter.h"
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"

#include "shake.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

extern IVEngineServer   *engine;
extern IPlayerInfoManager *playerinfomanager;
extern IServerPluginHelpers *helpers;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

MRecipientFilter::MRecipientFilter(void)
{
}

MRecipientFilter::~MRecipientFilter(void)
{
}

int MRecipientFilter::GetRecipientCount() const
{
   return m_Recipients.Size();
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
   if ( slot < 0 || slot >= GetRecipientCount() )
      return -1;

   return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
   return false;
}

bool MRecipientFilter::IsReliable() const
{
   return m_bReliable;
}

void MRecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

void MRecipientFilter::RemoveAllRecipients( void )
{
	m_Recipients.RemoveAll();
}

void MRecipientFilter::AddAllPlayers(int maxClients)
{
   m_Recipients.RemoveAll();
   int i;
   for ( i = 1; i <= maxClients; i++ )
   {
      edict_t *pPlayer = engine->PEntityOfEntIndex(i);
      if ( !pPlayer || pPlayer->IsFree()) {
         continue;
      }

	  IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pPlayer );
	  if (!playerinfo || !playerinfo->IsConnected())
	  {
		 continue;
	  }
		  
	  if (playerinfo->IsHLTV())
	  {
		  continue;
	  }

	  if (strcmp(playerinfo->GetNetworkIDString(),"BOT") == 0)
	  {
		  continue;
	  }

	  m_Recipients.AddToTail(i);
   }
} 

void MRecipientFilter::AddPlayer(int playerIndex)
{
   m_Recipients.AddToTail(playerIndex);
} 