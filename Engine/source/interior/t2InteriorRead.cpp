// HH: Added for Tribes 2 compatibility.
//
// What a .dif actually is, asked of the engine rather than of a model of the
// format.  The version 0 interior reader was written against the files and
// against the engine that wrote them; this is the other half, a count over
// every .dif the game ships, taken by the reader itself.
//
// 902 interiors in 325 files is what the archives hold.  It is not a load
// count and was quoted as one for a while.  Tools/swarm/count-interiors.py
// takes the load count with this.
//
// It reads the file directly rather than through the resource manager, for
// the reasons t2ShapeRead gives: a Resource hands back the resource without
// the stream, and the question that matters second is whether the reader
// consumed the file to its last byte.  An interior that reads "successfully"
// and leaves forty kilobytes unread has not been understood, the same way a
// packet body with bits left over has not.
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "core/stream/fileStream.h"
#include "core/strings/stringFunctions.h"
#include "interior/interiorRes.h"

// The version is not asked for here.  A .dif carries it in its first few
// bytes and the driver reads it off the file, for the reason count-shapes.py
// gives about .dts: a file the engine cannot read still has a version, and
// asking the engine for it loses exactly the rows worth seeing.
DefineEngineFunction( t2InteriorRead, const char *, ( const char *path ), ,
   "@brief Read one .dif and say how much of it the reader took.\n\n"
   "Returns four space-separated fields: whether it read (ok, bad or open), "
   "the number of interiors in it, the bytes the reader took and the bytes "
   "the file holds.  'open' means the file could not be opened at all, which "
   "for a mounted volume means the name is wrong rather than the interior "
   "is.\n\n"
   "@internal" )
{
   FileStream stream;
   stream.open( path, Torque::FS::File::Read );
   if ( stream.getStatus() != Stream::Ok )
      return "open 0 0 0";

   const U32 size = stream.getStreamSize();

   InteriorResource *res = new InteriorResource;
   const bool ok = res->read( stream );
   const U32 took = stream.getPosition();
   const S32 count = res->getNumDetailLevels() + res->getNumSubObjects();
   delete res;

   char *out = Con::getReturnBuffer( 96 );
   dSprintf( out, 96, "%s %d %u %u", ok ? "ok" : "bad", count, took, size );
   return out;
}
