// HH: Added for Tribes 2 compatibility.
#ifndef _NETCLASSTABLE_H_
#define _NETCLASSTABLE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

namespace NetClassTable
{
   extern const char *const Objects[];
   extern const char *const DataBlocks[];
   extern const char *const Events[];

   extern const U32 DataBlockCount;

   const char *const *tableFor(U32 classType, U32 *countOut, U32 *bitsOut);

   U32 toGameConnectionMessage(U32 message);
   U32 fromGameConnectionMessage(U32 message);
}

#endif
