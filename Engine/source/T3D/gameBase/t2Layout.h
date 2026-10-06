// HH: Added for Tribes 2 compatibility.
#ifndef _T2LAYOUT_H_
#define _T2LAYOUT_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

class BitStream;

namespace T2Layout
{
   bool readDataBlock(BitStream *stream, U32 classId);

   bool readGhost(BitStream *stream, const char *className,
                  bool isInitial = false);
   bool readEvent(BitStream *stream, const char *className);

   typedef const char *(*GhostClassFn)(void *ctx, U32 ghostIndex);

   bool readPacketData(BitStream *stream, const char *className,
                       GhostClassFn resolve = NULL, void *ctx = NULL);

   bool writeDataBlock(BitStream *stream, U32 classId);

   bool writeGhost(BitStream *stream, const char *className,
                   bool isInitial = false);
   bool writeEvent(BitStream *stream, const char *className);

   bool writePacketData(BitStream *stream, const char *className,
                        GhostClassFn resolve = NULL, void *ctx = NULL);

   const char *lastShapeFile();

   U32 firstDataBlockId();

   const char *firstString();

   bool firstRawU32(U32 *out);

   /// A Tribes 2 resource name is a bare file name; Torque wants a path.
   String resourcePath(const char *name, const char *dir);
}

#endif
