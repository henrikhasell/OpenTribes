// HH: Added for Tribes 2 compatibility.
#ifndef _NETCLASSTABLET2_H_
#define _NETCLASSTABLET2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// The class ids Tribes 2 puts on the wire.
///
/// Tribes 2 sorts every registered class into one table by type and then by
/// name, and an object's place in that table is the number a packet names it
/// by.  The tables below are that order, read out of the retail binary, so a
/// class here answers to the same number on both ends.  A name we have not
/// ported yet leaves its number empty rather than shifting the ones after it.
namespace NetClassTableT2
{
   /// Names in wire order, one array per class type.
   extern const char *const Objects[];
   extern const char *const DataBlocks[];
   extern const char *const Events[];

   /// How many names each of those arrays holds.
   extern const U32 ObjectCount;
   extern const U32 DataBlockCount;
   extern const U32 EventCount;

   /// How many bits a packet spends on a class id of each type.
   extern const U32 ObjectBits;
   extern const U32 DataBlockBits;
   extern const U32 EventBits;

   /// The names for a class type, or NULL when the type has no table.
   const char *const *tableFor(U32 classType, U32 *countOut, U32 *bitsOut);

   /// Whether we have ported the packing this class puts on the wire.
   ///
   /// A class seated under a Tribes 2 name still answers with Torque's own
   /// packing until somebody ports it, and reading that is worse than reading
   /// nothing.  A class that is not ported keeps its number and its name, so
   /// a packet asking for one says which it wanted.
   bool isPorted(U32 classType, const char *className);
}

#endif
