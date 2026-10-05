// HH: Added for Tribes 2 compatibility.
#ifndef _T2LAYOUT_H_
#define _T2LAYOUT_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class BitStream;
class T2Reader;

namespace T2Layout
{
   /// Where a class takes its fields.  The tables are here, so the cursor over
   /// them is built here too and handed to the class rather than the other way
   /// about; what the class leaves is discarded.  A callback rather than an
   /// interface to inherit, so that NetObject does not grow a second vtable
   /// and this header needs nothing of the engine.  See t2Fields.h.
   typedef void (*FieldsFn)(void *ctx, T2Reader &reader);

   bool readDataBlock(BitStream *stream, U32 classId,
                      FieldsFn fields = NULL, void *ctx = NULL);

   bool readGhost(BitStream *stream, const char *className,
                  bool isInitial = false,
                  FieldsFn fields = NULL, void *ctx = NULL);
   bool readEvent(BitStream *stream, const char *className,
                  FieldsFn fields = NULL, void *ctx = NULL);

   /// Which node of a ghost's layout holds a named field, or T2Reader::NoNode
   /// if that layout does not name its fields.
   ///
   /// Only the layouts written out by hand in Tools/t2/handlayout.py name
   /// anything -- a layout recovered by walking the binary has nothing to call
   /// its nodes.  A field's shape is the better handle and survives the tables
   /// being regenerated; this is for where shape cannot tell two fields apart,
   /// which in a ShapeBase body is most of it.  Resolve it once per class
   /// rather than per packet: it is a string search.
   U32 ghostNode(const char *className, const char *field);

   /// Which node of a datablock's layout holds a named field, or
   /// T2Reader::NoNode if that layout is not described.
   ///
   /// The datablock tables came out of the walk anonymous, so this is not a
   /// property of the table the way ghostNode() is: the names are computed
   /// from it by Tools/t2/gendatablocknames.py, which walks the committed
   /// graph and lays a field list alongside.  Same caution as ghostNode --
   /// shape first, this where shape cannot tell two fields apart, and
   /// resolve it once per class rather than per packet.
   U32 dataBlockNode(const char *className, const char *field);

   typedef const char *(*GhostClassFn)(void *ctx, U32 ghostIndex);

   /// What the object a client drives sends each tick.  `fields` is the
   /// control object's own take, as for readGhost; the inner read that
   /// OpPacketDataOf triggers passes none, so a piloted vehicle's body is read
   /// out and discarded rather than filed onto the player driving it.
   bool readPacketData(BitStream *stream, const char *className,
                       GhostClassFn resolve = NULL, void *ctx = NULL,
                       FieldsFn fields = NULL, void *fieldsCtx = NULL);

   const char *lastShapeFile();
}

#endif
