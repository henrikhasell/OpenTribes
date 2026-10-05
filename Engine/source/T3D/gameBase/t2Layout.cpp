#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "console/netClassTable.h"
#include "math/mPoint3.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/gameBase/t2Reader.h"

// The tables sit at file scope rather than inside T2Layout because the cursor
// in t2Reader.h is declared against T2Protocol too, and each table header opens
// that namespace itself: nested, the one the cursor names and the one the
// tables land in would be two different namespaces.  They are static either
// way.
#include "T3D/gameBase/t2GhostTablesRecovered.h"
#include "T3D/gameBase/t2GhostNodes.h"
#include "T3D/gameBase/t2DataBlockNodes.h"
#include "T3D/gameBase/t2EventTables.h"
#include "T3D/gameBase/t2EventTablesRecovered.h"
#include "T3D/gameBase/t2PacketDataTables.h"
#include "T3D/gameBase/t2PacketDataTablesRecovered.h"

namespace T2Layout
{

using namespace T2Protocol;

static GhostClassFn sResolve = NULL;
static void *sResolveCtx = NULL;
static char sShapeName[256] = { 0 };

const char *lastShapeFile()
{
   return sShapeName[0] ? sShapeName : NULL;
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

/// The discard-read and the read a class drives are the same walk, because the
/// cursor is both: a class that has not been ported yet and one that takes
/// every field therefore cannot disagree about where the body ends.
class LayoutEnv : public T2Reader::Env
{
public:
   const char *ghostClass(U32 ghostIndex)
   {
      return sResolve ? sResolve(sResolveCtx, ghostIndex) : NULL;
   }

   bool readPacketData(BitStream *stream, const char *className)
   {
      return T2Layout::readPacketData(stream, className, sResolve, sResolveCtx);
   }
};

static bool runGraph(BitStream *stream, const T2DataBlockLayout *layout,
                     bool isInitial, FieldsFn fields = NULL, void *ctx = NULL)
{
   LayoutEnv env;
   T2Reader reader(stream, layout, isInitial, &env);
   // The class takes what it knows how to take; finish() reads out the rest.
   // One path, so a class ported half way cannot end up anywhere but where a
   // class not ported at all ends up.
   if(fields)
      fields(ctx, reader);
   const bool ok = reader.finish();
   noteString(reader.shapeFile());
   return ok;
}

static bool runNamed(BitStream *stream, const char *name,
                     const T2DataBlockLayout *table, U32 count,
                     bool *found = NULL, bool isInitial = false,
                     FieldsFn fields = NULL, void *ctx = NULL)
{
   if(!name)
      return false;

   for(U32 i = 0; i < count; i++)
   {
      if(dStrcmp(table[i].name, name))
         continue;

      if(found)
         *found = true;

      return runGraph(stream, &table[i], isInitial, fields, ctx);
   }

   return false;
}

#define ELEMENTS( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

static Vector<StringTableEntry> sUnimplemented(__FILE__, __LINE__);

static void warnUnimplemented(const char *kind, const char *name)
{
   char what[192];
   dSprintf(what, sizeof(what), "%s %s", kind, name ? name : "(unnamed)");

   StringTableEntry key = StringTable->insert(what);
   for(U32 i = 0; i < sUnimplemented.size(); i++)
      if(sUnimplemented[i] == key)
         return;
   sUnimplemented.push_back(key);

   Con::errorf("No Tribes 2 layout for %s -- the rest of the packet is lost.",
               what);
}

bool readDataBlock(BitStream *stream, U32 classId, FieldsFn fields, void *ctx)
{
   AssertFatal(ELEMENTS(sT2DataBlocks) == NetClassTable::DataBlockCount,
      "The datablock layouts are not one per net class id!");

   sShapeName[0] = 0;
   if(classId >= ELEMENTS(sT2DataBlocks))
   {
      char id[32];
      dSprintf(id, sizeof(id), "class id %u", classId);
      warnUnimplemented("datablock", id);
      return false;
   }

   return runGraph(stream, &sT2DataBlocks[classId], false, fields, ctx);
}

bool readGhost(BitStream *stream, const char *className, bool isInitial,
               FieldsFn fields, void *ctx)
{
   bool found = false;
   const bool ok = runNamed(stream, className, sT2GhostsRecovered,
                            ELEMENTS(sT2GhostsRecovered), &found, isInitial,
                            fields, ctx);
   if(!found)
      warnUnimplemented("ghost", className);
   return ok;
}

bool readEvent(BitStream *stream, const char *className,
               FieldsFn fields, void *ctx)
{
   bool found = false;
   const bool ok = runNamed(stream, className, sT2EventsRecovered,
                            ELEMENTS(sT2EventsRecovered), &found, false,
                            fields, ctx);
   if(found)
      return ok;

   const bool shipped = runNamed(stream, className, sT2Events,
                                 ELEMENTS(sT2Events), &found, false,
                                 fields, ctx);
   if(!found)
      warnUnimplemented("event", className);
   return shipped;
}

U32 ghostNode(const char *className, const char *field)
{
   if(!className || !field)
      return U32(T2Reader::NoNode);

   for(U32 i = 0; i < ELEMENTS(sT2GhostNodeNames); i++)
   {
      if(dStrcmp(sT2GhostNodeNames[i].layout, className))
         continue;

      const T2LayoutNames &l = sT2GhostNodeNames[i];
      for(U32 f = 0; f < l.count; f++)
         if(!dStrcmp(l.fields[f].field, field))
            return l.fields[f].node;
      break;
   }

   return U32(T2Reader::NoNode);
}

U32 dataBlockNode(const char *className, const char *field)
{
   if(!className || !field)
      return U32(T2Reader::NoNode);

   for(U32 i = 0; i < ELEMENTS(sT2DataBlockNodeNames); i++)
   {
      if(dStrcmp(sT2DataBlockNodeNames[i].layout, className))
         continue;

      const T2DbLayoutNames &l = sT2DataBlockNodeNames[i];
      for(U32 f = 0; f < l.count; f++)
         if(!dStrcmp(l.fields[f].field, field))
            return l.fields[f].node;
      break;
   }

   return U32(T2Reader::NoNode);
}

bool readPacketData(BitStream *stream, const char *className,
                    GhostClassFn resolve, void *ctx,
                    FieldsFn fields, void *fieldsCtx)
{
   sResolve = resolve;
   sResolveCtx = ctx;

   bool found = false;
   bool ok = runNamed(stream, className, sT2PacketDataRecovered,
                      ELEMENTS(sT2PacketDataRecovered), &found, false,
                      fields, fieldsCtx);
   if(!found)
      ok = runNamed(stream, className, sT2PacketData,
                    ELEMENTS(sT2PacketData), &found, false,
                    fields, fieldsCtx);
   if(!found)
      warnUnimplemented("control object", className);
   return ok;
}

}
