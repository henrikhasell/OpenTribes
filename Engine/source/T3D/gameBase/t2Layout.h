// HH: Added for Tribes 2 compatibility.
#ifndef _T2LAYOUT_H_
#define _T2LAYOUT_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class BitStream;

/// The bit layout of what Tribes 2 sends, walked a node at a time.
///
/// A datablock, a ghost update and an event each ride in a packet with no
/// length in front of them, and none of them is a fixed run of bits: a flag
/// partway through decides whether the fields after it are there, and some
/// carry a count read from the stream.  So a body cannot be stepped over
/// without being understood, and one read wrongly takes the rest of the
/// packet with it.
///
/// The tables in t2DataBlockTables.h, t2GhostTables.h, t2EventTables.h and
/// t2PacketDataTables.h hold a graph per class, recovered by running each
/// unpackData and unpack out of the retail binary under an emulator.  They
/// come from BananaEngine's tools/t2, where the walk and the four things it
/// has to get right are written up.
namespace T2Layout
{
   /// Consume one datablock body of the named Tribes 2 class.  False when
   /// there is no layout for that name, which leaves the stream unusable.
   bool readDataBlock(BitStream *stream, const char *className);

   /// The same for a ghost update, and for an event body.
   bool readGhost(BitStream *stream, const char *className);
   bool readEvent(BitStream *stream, const char *className);

   /// The per-tick state of the object this client drives, which Tribes 2
   /// sends through readPacketData rather than unpackUpdate.
   bool readPacketData(BitStream *stream, const char *className);

   /// The shape file the datablock last read named, or NULL.  The walk keeps
   /// the first string that names one, which is how a class the port has no
   /// reader for still yields the one field it needs.
   const char *lastShapeFile();

   /// Print every node a walk takes, with the bits it took.
   void setTrace(bool trace);
}

#endif
