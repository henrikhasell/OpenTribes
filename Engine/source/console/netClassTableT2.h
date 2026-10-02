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

   /// How many names the datablock array holds, for naming one we lack.
   extern const U32 DataBlockCount;

   /// The names for a class type, or NULL when the type has no table.
   /// The name Tribes 2 gives a class the port had to name otherwise.
   const char *t2NameFor( const char *className );

   /// @name Connection messages
   ///
   /// The message rides in three bits of a ghosting message event, as an
   /// index into an enum both ends have to agree on.  Torque's has grown
   /// three states since Tribes 2 -- GhostAlwaysStarting,
   /// SendNextDownloadRequest and FileDownloadSizeMessage -- which pushes
   /// everything after them along, so DataBlocksDone is 6 here and 3 there.
   /// @{

   U32 toT2ConnectionMessage( U32 message );
   U32 fromT2ConnectionMessage( U32 message );

   /// @}

   const char *const *tableFor(U32 classType, U32 *countOut, U32 *bitsOut);
}

#endif
