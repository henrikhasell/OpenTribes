// HH: Added for Tribes 2 compatibility.
//
// What a Tribes 2 .ml actually is, asked of the engine rather than of a model
// of the format.
//
// A .dif holds only the base bake, which is dim -- 25 to 49 of 255 across the
// seven buildings Dust to Dust loads, with half to three quarters of each
// one's surfaces under 8.  Tribes 2 computes the rest per mission and caches
// it under lighting/, and retail's own GameData has two of those: Talus at
// 510 KB and Training4 at 209 KB, 69 and 20 lightmaps, none of them near
// black.  That is the lit case, and it is sitting on disk.
//
// Whether this engine can read one is a question with a one-line answer and
// no join in it: PersistInfo is a plain file format, and the only thing
// standing between the two is a version number -- ours is 17 and Tribes 2's
// files say 16, with the header matching field for field underneath.  So this
// opens one and says how far it got, the same way t2InteriorRead does for a
// .dif, because a file that parses to its last byte is the only evidence that
// the rest of the structure matches too.
//
// It reads through PersistInfo rather than reimplementing it, so what it
// tests is the reader this engine would actually use.
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "core/stream/fileStream.h"
#include "core/strings/stringFunctions.h"
#include "lighting/common/scenePersist.h"

DefineEngineFunction( t2LightingRead, const char *, ( const char *path ), ,
   "@brief Read one cached lighting file and say how much of it the reader "
   "took.\n\n"
   "Returns four space-separated fields: whether it read (ok, bad or open), "
   "the version the file carries, the bytes the reader took and the bytes the "
   "file holds.  A version this engine does not accept reports the file's own "
   "rather than failing silently, because that difference is the thing worth "
   "seeing.\n\n"
   "@internal" )
{
   FileStream stream;
   stream.open( path, Torque::FS::File::Read );
   if ( stream.getStatus() != Stream::Ok )
      return "open -1 0 0";

   const U32 size = stream.getStreamSize();

   U32 fileVersion = 0;
   stream.read( &fileVersion );
   stream.setPosition( 0 );

   PersistInfo info;
   const bool ok = info.read( stream );
   const U32 took = stream.getPosition();

   char *out = Con::getReturnBuffer( 96 );
   dSprintf( out, 96, "%s %u %u %u", ok ? "ok" : "bad", fileVersion, took, size );
   return out;
}
