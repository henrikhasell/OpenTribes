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
