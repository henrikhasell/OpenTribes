#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "console/netClassTable.h"
#include "math/mPoint3.h"
#include "T3D/gameBase/t2Layout.h"

namespace T2Layout
{
#include "T3D/gameBase/t2DataBlockTables.h"
#include "T3D/gameBase/t2GhostTablesRecovered.h"
#include "T3D/gameBase/t2EventTables.h"
#include "T3D/gameBase/t2EventTablesRecovered.h"
#include "T3D/gameBase/t2PacketDataTables.h"
#include "T3D/gameBase/t2PacketDataTablesRecovered.h"

using namespace T2Protocol;

static GhostClassFn sResolve = NULL;
static void *sResolveCtx = NULL;
static char sShapeName[256] = { 0 };
static U32 sDataBlockId = 0;
static char sFirstString[256] = { 0 };
static U32 sFirstWord32 = 0;
static bool sHaveFirstWord32 = false;

const char *lastShapeFile()
{
   return sShapeName[0] ? sShapeName : NULL;
}

U32 firstDataBlockId()
{
   return sDataBlockId;
}

const char *firstString()
{
   return sFirstString[0] ? sFirstString : NULL;
}

bool firstWord32(U32 *out)
{
   if(!sHaveFirstWord32)
      return false;
   *out = sFirstWord32;
   return true;
}

String resourcePath(const char *name, const char *dir)
{
   String path(name);
   if(path.find('/') != String::NPos || path.find('\\') != String::NPos)
      return path;
   return String(dir) + path;
}

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

/// Reading takes a node's value off the wire; writing puts a default one on
/// it at the same width. Every branch below therefore asks for its value
/// through one of these two and then treats it the same way, so the writer
/// cannot drift away from the reader's idea of how wide a node is.
static U32 xferInt(BitStream *stream, U32 bits, bool write)
{
   if(!bits)
      return 0;

   if(write)
   {
      stream->writeInt(0, bits);
      return 0;
   }

   return stream->readInt(bits);
}

static bool xferFlag(BitStream *stream, bool write)
{
   if(write)
   {
      stream->writeFlag(false);
      return false;
   }

   return stream->readFlag();
}

static bool runGraph(BitStream *stream, const T2DataBlockLayout *layout,
                     S32 at, U32 *slots, U32 depth, bool isInitial,
                     bool write)
{
   if(depth > 8)
   {
      Con::warnf("T2 layout: %s nested deeper than 8", layout->name);
      return false;
   }

   char buf[512];
   U32 guard = 0;

   while(at >= 0 && U32(at) < layout->count)
   {
      if(++guard > 65536)
      {
         Con::warnf("T2 layout: %s spinning at node %d", layout->name, at);
         return false;
      }

      const T2DataNode &n = layout->nodes[at];

      switch(n.op)
      {
         case OpEnd:
         case OpEndBody:
            return true;

         case OpFlag:
            at = xferFlag(stream, write) ? n.b : n.a;
            continue;

         case OpInitial:
            at = isInitial ? n.b : n.a;
            continue;

         case OpCountBits:
         {
            U32 v = xferInt(stream, n.bits, write);
            U32 set = 0;
            for(; v; v >>= 1)
               set += v & 1;
            slots[n.b & 7] = set;
            at = n.a;
            continue;
         }

         case OpPacketDataOf:
         {
            const U32 idx = xferInt(stream, n.bits, write);
            const char *cls = sResolve ? sResolve(sResolveCtx, idx) : NULL;
            if(!cls)
            {
               Con::warnf("T2 layout: %s %s the packet data of ghost %d, "
                          "which we cannot name", layout->name,
                          write ? "writes" : "reads", idx);
               return false;
            }
            if(write)
            {
               if(!writePacketData(stream, cls, sResolve, sResolveCtx))
                  return false;
            }
            else if(!readPacketData(stream, cls, sResolve, sResolveCtx))
               return false;
            at = n.a;
            continue;
         }

         case OpSelect:
         {
            const U32 v = xferInt(stream, n.bits, write);
            if(!layout->select || v >= (1U << n.bits))
            {
               Con::warnf("T2 layout: %s node %d select %u out of range",
                          layout->name, at, v);
               return false;
            }
            at = layout->select[n.a + v];
            continue;
         }

         case OpReadBr:
         {
            const U32 v = xferInt(stream, n.bits, write);
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

         case OpCount:
            slots[n.b & 7] = xferInt(stream, n.bits, write);
            break;

         case OpLoop:
         {
            const U32 count = slots[n.bits & 7];
            if(count > 4096)
            {
               Con::warnf("T2 layout: %s node %d loop count %u",
                          layout->name, at, count);
               return false;
            }
            for(U32 i = 0; i < count; i++)
               if(!runGraph(stream, layout, n.b, slots, depth + 1, isInitial,
                            write))
                  return false;
            at = n.a;
            continue;
         }

         case OpDataBlockId:
         {
            const U32 dbId = xferInt(stream, n.bits, write);
            if(!sDataBlockId)
               sDataBlockId = dbId;
            break;
         }

         case OpFloat:
         case OpSignedFloat:
         case OpRangedF32:
         case OpRangedU32:
         case OpColor:
            xferInt(stream, n.bits, write);
            break;

         case OpInt:
         case OpRaw:
         {
            const U32 word = xferInt(stream, n.bits, write);
            if(n.bits == 32 && !sHaveFirstWord32 && !write)
            {
               sHaveFirstWord32 = true;
               sFirstWord32 = word;
            }
            break;
         }

         case OpPoint:
         {
            Point3F p(0.0f, 0.0f, 0.0f);
            if(write)
               stream->writeCompressedPoint(p, 0.01f);
            else
               stream->readCompressedPoint(&p, 0.01f);
            break;
         }

         case OpString:
            if(write)
               stream->writeString("");
            else
            {
               stream->readString(buf);
               noteString(buf);
               if(!sFirstString[0])
               {
                  dStrncpy(sFirstString, buf, sizeof(sFirstString) - 1);
                  sFirstString[sizeof(sFirstString) - 1] = 0;
               }
            }
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
                     const T2DataBlockLayout *table, U32 count,
                     bool *found = NULL, bool isInitial = false,
                     bool write = false)
{
   if(!name)
      return false;

   for(U32 i = 0; i < count; i++)
   {
      if(dStrcmp(table[i].name, name))
         continue;

      if(found)
         *found = true;

      U32 slots[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
      return runGraph(stream, &table[i], table[i].root, slots, 0,
                      isInitial, write);
   }

   return false;
}

#define ELEMENTS( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

static Vector<StringTableEntry> sUnimplemented(__FILE__, __LINE__);

static void warnUnimplemented(const char *kind, const char *name, bool write)
{
   char what[192];
   dSprintf(what, sizeof(what), "%s %s", kind, name ? name : "(unnamed)");

   char keyed[256];
   dSprintf(keyed, sizeof(keyed), "%s %s", write ? "write" : "read", what);

   StringTableEntry key = StringTable->insert(keyed);
   for(U32 i = 0; i < sUnimplemented.size(); i++)
      if(sUnimplemented[i] == key)
         return;
   sUnimplemented.push_back(key);

   Con::errorf("No Tribes 2 layout for %s -- the rest of the %s is lost.",
               what, write ? "body we are writing" : "packet");
}

static bool runDataBlock(BitStream *stream, U32 classId, bool write)
{
   AssertFatal(ELEMENTS(sT2DataBlocks) == NetClassTable::DataBlockCount,
      "The datablock layouts are not one per net class id!");

   sShapeName[0] = 0;
   if(classId >= ELEMENTS(sT2DataBlocks))
   {
      char id[32];
      dSprintf(id, sizeof(id), "class id %u", classId);
      warnUnimplemented("datablock", id, write);
      return false;
   }

   const T2DataBlockLayout *layout = &sT2DataBlocks[classId];
   U32 slots[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
   return runGraph(stream, layout, layout->root, slots, 0, false, write);
}

static bool runGhost(BitStream *stream, const char *className,
                     bool isInitial, bool write)
{
   sDataBlockId = 0;
   sFirstString[0] = 0;
   sHaveFirstWord32 = false;
   sFirstWord32 = 0;
   bool found = false;
   const bool ok = runNamed(stream, className, sT2GhostsRecovered,
                            ELEMENTS(sT2GhostsRecovered), &found, isInitial,
                            write);
   if(!found)
      warnUnimplemented("ghost", className, write);
   return ok;
}

static bool runEvent(BitStream *stream, const char *className, bool write)
{
   bool found = false;
   const bool ok = runNamed(stream, className, sT2EventsRecovered,
                            ELEMENTS(sT2EventsRecovered), &found, false,
                            write);
   if(found)
      return ok;

   const bool shipped = runNamed(stream, className, sT2Events,
                                 ELEMENTS(sT2Events), &found, false, write);
   if(!found)
      warnUnimplemented("event", className, write);
   return shipped;
}

static bool runPacketData(BitStream *stream, const char *className,
                          GhostClassFn resolve, void *ctx, bool write)
{
   sResolve = resolve;
   sResolveCtx = ctx;

   bool found = false;
   bool ok = runNamed(stream, className, sT2PacketDataRecovered,
                      ELEMENTS(sT2PacketDataRecovered), &found, false, write);
   if(!found)
      ok = runNamed(stream, className, sT2PacketData,
                    ELEMENTS(sT2PacketData), &found, false, write);
   if(!found)
      warnUnimplemented("control object", className, write);
   return ok;
}

bool readDataBlock(BitStream *stream, U32 classId)
{
   return runDataBlock(stream, classId, false);
}

bool readGhost(BitStream *stream, const char *className, bool isInitial)
{
   return runGhost(stream, className, isInitial, false);
}

bool readEvent(BitStream *stream, const char *className)
{
   return runEvent(stream, className, false);
}

bool readPacketData(BitStream *stream, const char *className,
                    GhostClassFn resolve, void *ctx)
{
   return runPacketData(stream, className, resolve, ctx, false);
}

bool writeDataBlock(BitStream *stream, U32 classId)
{
   return runDataBlock(stream, classId, true);
}

bool writeGhost(BitStream *stream, const char *className, bool isInitial)
{
   return runGhost(stream, className, isInitial, true);
}

bool writeEvent(BitStream *stream, const char *className)
{
   return runEvent(stream, className, true);
}

bool writePacketData(BitStream *stream, const char *className,
                     GhostClassFn resolve, void *ctx)
{
   return runPacketData(stream, className, resolve, ctx, true);
}

}
