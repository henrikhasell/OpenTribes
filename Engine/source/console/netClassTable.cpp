#include "console/netClassTable.h"
#include "console/consoleObject.h"

#ifndef NET_CLASS_TABLE_HEADER
#define NET_CLASS_TABLE_HEADER "console/netClassTableTribes2.h"
#endif

#include NET_CLASS_TABLE_HEADER

#define ELEMENTS( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

namespace NetClassTable
{

#define NET_CLASS_TABLE_NAME( name )            #name,

const char *const Objects[]    = { NET_CLASS_TABLE_OBJECT_NAMES( NET_CLASS_TABLE_NAME ) };
const char *const DataBlocks[] = { NET_CLASS_TABLE_DATABLOCK_NAMES( NET_CLASS_TABLE_NAME ) };
const char *const Events[]     = { NET_CLASS_TABLE_EVENT_NAMES( NET_CLASS_TABLE_NAME ) };

const U32 DataBlockCount = ELEMENTS( DataBlocks );

static const U32 sEngineGhostStates = 6;
static const U32 sGameMessages = 2;

// Our engine has more ghost states than Tribes 2, and the surplus two are the
// file-download half of the handshake, which Tribes 2 has no name for. Moving
// only the GameConnection messages left them on the wire values Tribes 2 gives
// DataBlocksDone and DataBlocksDownloadDone, where they were read back as those.
// So the surplus goes above Tribes 2's own set instead, into the wire values
// three bits hold and Tribes 2 never sends.
U32 toGameConnectionMessage( U32 message )
{
   if( message < NET_CLASS_TABLE_GHOST_STATES )
      return message;
   if( message < sEngineGhostStates )
      return message + sGameMessages;
   return message - sEngineGhostStates + NET_CLASS_TABLE_GHOST_STATES;
}

U32 fromGameConnectionMessage( U32 message )
{
   if( message < NET_CLASS_TABLE_GHOST_STATES )
      return message;
   if( message < NET_CLASS_TABLE_GHOST_STATES + sGameMessages )
      return message - NET_CLASS_TABLE_GHOST_STATES + sEngineGhostStates;
   return message - sGameMessages;
}

const char *const *tableFor( U32 classType, U32 *countOut, U32 *bitsOut )
{
   switch( classType )
   {
      case NetClassTypeObject:
         *countOut = ELEMENTS( Objects );
         *bitsOut  = NET_CLASS_TABLE_OBJECT_BITS;
         return Objects;
      case NetClassTypeDataBlock:
         *countOut = DataBlockCount;
         *bitsOut  = NET_CLASS_TABLE_DATABLOCK_BITS;
         return DataBlocks;
      case NetClassTypeEvent:
         *countOut = ELEMENTS( Events );
         *bitsOut  = NET_CLASS_TABLE_EVENT_BITS;
         return Events;
   }
   *countOut = 0;
   *bitsOut  = 0;
   return NULL;
}

}
