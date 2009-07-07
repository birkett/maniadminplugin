#ifndef _MRECIPIENT_FILTER_H
#define _MRECIPIENT_FILTER_H
#include "irecipientfilter.h"
#include "bitvec.h"
#include "tier1/utlvector.h"

class MRecipientFilter :
   public IRecipientFilter
{
public:
   MRecipientFilter(void);
   ~MRecipientFilter(void);

   virtual bool IsReliable( void ) const;
   virtual bool IsInitMessage( void ) const;
   virtual void MakeReliable( void );
   virtual void RemoveAllRecipients( void );
   virtual int GetRecipientCount( void ) const;
   virtual int GetRecipientIndex( int slot ) const;
   void AddAllPlayers( int maxClients );
   void AddPlayer(int playerIndex);

private:
   bool m_bReliable;
   bool m_bInitMessage;
   CUtlVector< int > m_Recipients;
};

#endif 