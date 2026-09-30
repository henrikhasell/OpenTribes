// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "math/mPoint3.h"
#include "T3D/gameBase/t2Layout.h"

/// The bit layout of what Tribes 2 sends, walked a node at a time.
///
/// The tables were recovered by running each unpackData and unpack out of the
/// retail binary under an emulator, forking the walk at every readFlag and at
/// every narrow read that could be a count, then merging the states that
/// reconverge.  They come from BananaEngine's tools/t2 along with the write-up
/// of the four things such a walk has to get right.
///
/// Only the shape of the bits is here: each node says how many to take and
/// where to go next.

namespace T2Layout
{

// The tables open a namespace of their own, and carry the ops, the node and
// the layout struct with them.
#include "T3D/gameBase/t2DataBlockTables.h"
#include "T3D/gameBase/t2GhostTables.h"
#include "T3D/gameBase/t2EventTables.h"
#include "T3D/gameBase/t2PacketDataTables.h"

using namespace T2Protocol;

static bool sTrace = false;
static char sShapeName[256] = { 0 };

void setTrace(bool trace)
{
   sTrace = trace;
}

const char *lastShapeFile()
{
   return sShapeName[0] ? sShapeName : NULL;
}

/// The first string a datablock reads that names a shape is its shape file.
///
/// A datablock the port has no class for still yields the one field the port
/// wants out of it this way, without counting the same bits a second time
/// beside a graph that is known to be right.
static void noteString(const char *str)
{
   if(sShapeName[0] || !str)
      return;

   const U32 len = dStrlen(str);
   if(len > 4 && !dStricmp(str + len - 4, ".dts"))
   {
      dStrncpy(sShapeName, str, sizeof(sShapeName) - 1);
      sShapeName[sizeof(sShapeName) - 1] = 0;
   }
}

static bool runGraph(BitStream *stream, const T2DataBlockLayout *layout,
                     S32 at, U32 *slots, U32 depth)
{
   if(depth > 8)                       // Tribes 2 nests these two deep at most
   {
      Con::warnf("T2 layout: %s nested deeper than 8", layout->name);
      return false;
   }

   char buf[512];
   U32 guard = 0;

   while(at >= 0 && U32(at) < layout->count)
   {
      if(++guard > 65536)              // a malformed graph must not spin here
      {
         Con::warnf("T2 layout: %s spinning at node %d", layout->name, at);
         return false;
      }

      const T2DataNode &n = layout->nodes[at];

      if(sTrace)
      {
         static const char *const opNames[] =
         {
            "End", "EndBody", "Flag", "Select", "ReadBr", "Count", "Loop",
            "Int", "Float", "SignedFloat", "RangedF32", "RangedU32",
            "Color", "Raw", "DataBlockId", "String", "Point"
         };
         Con::printf("  [%4d] node %3d %-12s bits %d", stream->getCurPos(),
                     at, n.op < 17 ? opNames[n.op] : "?", n.bits);
      }

      switch(n.op)
      {
         case OpEnd:
         case OpEndBody:
            return true;

         case OpFlag:
            at = stream->readFlag() ? n.b : n.a;
            continue;

         // A few bits that are not a field but a count -- how many colours a
         // ParticleData carries, say -- so the path after them depends on the
         // value and not just on whether it is set.
         case OpSelect:
         {
            const U32 v = stream->readInt(n.bits);
            if(!layout->select || v >= (1U << n.bits))
            {
               Con::warnf("T2 layout: %s node %d select %u out of range",
                          layout->name, at, v);
               return false;
            }
            at = layout->select[n.a + v];
            continue;
         }

         // Too wide to enumerate.  The value is kept, because a loop may
         // count on it, and the path continues on whether it was zero --
         // which is what decides whether a guarded block is there at all.
         case OpReadBr:
         {
            const U32 v = n.bits ? stream->readInt(n.bits) : 0;
            if(!layout->select)
            {
               Con::warnf("T2 layout: %s node %d has no select table",
                          layout->name, at);
               return false;
            }
            if(n.b >= 0)
               slots[n.b & 7] = v;
            at = layout->select[n.a + (v ? 1 : 0)];
            continue;
         }

         // A length too wide to enumerate, so it is kept and counted down by
         // the loop that follows rather than being built into the graph.
         case OpCount:
            slots[n.b & 7] = n.bits ? stream->readInt(n.bits) : 0;
            break;

         case OpLoop:
         {
            const U32 count = slots[n.bits & 7];
            if(count > 4096)           // a misread length, not a real array
            {
               Con::warnf("T2 layout: %s node %d loop count %u",
                          layout->name, at, count);
               return false;
            }
            for(U32 i = 0; i < count; i++)
               if(!runGraph(stream, layout, n.b, slots, depth + 1))
                  return false;
            at = n.a;
            continue;
         }

         // Everything below takes a fixed number of bits.  What the bits mean
         // differs -- a scaled float, a colour, a datablock id -- but nothing
         // here keeps the values, so the width is all that matters.
         case OpInt:
         case OpFloat:
         case OpSignedFloat:
         case OpRangedF32:
         case OpRangedU32:
         case OpColor:
         case OpRaw:
         case OpDataBlockId:
            if(n.bits)
               stream->readInt(n.bits);
            break;

         // Two bits of type and then three deltas or three whole floats.
         case OpPoint:
         {
            Point3F p;
            stream->readCompressedPoint(&p, 0.01f);
            break;
         }

         // A string is as long as it is; the stream has to read it for real.
         case OpString:
            stream->readString(buf);
            noteString(buf);
            break;

         default:
            Con::warnf("T2 layout: %s unknown op %d at node %d",
                       layout->name, n.op, at);
            return false;
      }
      at = n.a;
   }

   if(at >= 0)
      Con::warnf("T2 layout: %s node %d past end (%d)", layout->name, at,
                 layout->count);
   return at < 0;
}

static bool runNamed(BitStream *stream, const char *name,
                     const T2DataBlockLayout *table, U32 count)
{
   if(!name)
      return false;

   for(U32 i = 0; i < count; i++)
   {
      if(dStrcmp(table[i].name, name))
         continue;

      U32 slots[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
      return runGraph(stream, &table[i], table[i].root, slots, 0);
   }

   return false;
}

#define ELEMENTS( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

bool readDataBlock(BitStream *stream, const char *className)
{
   sShapeName[0] = 0;
   return runNamed(stream, className, sT2DataBlocks, ELEMENTS(sT2DataBlocks));
}

bool readGhost(BitStream *stream, const char *className)
{
   return runNamed(stream, className, sT2Ghosts, ELEMENTS(sT2Ghosts));
}

bool readEvent(BitStream *stream, const char *className)
{
   return runNamed(stream, className, sT2Events, ELEMENTS(sT2Events));
}

bool readPacketData(BitStream *stream, const char *className)
{
   return runNamed(stream, className, sT2PacketData, ELEMENTS(sT2PacketData));
}

}
