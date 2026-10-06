// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/demoT2.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "core/stream/fileStream.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"

namespace DemoT2
{

static const char *sSignature = "Tribes2 Recording";

bool readHeader(Stream *stream, U32 *version, U32 *lengthMs, U32 *startSize)
{
   char signature[256];
   stream->readString(signature);
   if(dStrcmp(signature, sSignature))
      return false;

   stream->read(version);
   stream->read(lengthMs);
   stream->read(startSize);
   return stream->getStatus() == Stream::Ok;
}

// The target manager's state (0x00670660).  Tribes 2's targeting has no
// Torque counterpart, so the bits are consumed and nothing is kept: a word of
// its own, a sensor-group by target-type grid, then 512 target slots.
void readTargetManagerState(BitStream *stream)
{
   U32 word;
   stream->read(&word);

   for(U32 i = 0; i < 32; i++)
      for(U32 j = 0; j < 32; j++)
         if(stream->readFlag())
            stream->read(&word);

   for(U32 i = 0; i < 512; i++)
   {
      if(!stream->readFlag())
         continue;

      if(stream->readFlag())
         stream->read(&word);
      if(stream->readFlag())
         stream->read(&word);

      char name[256];
      for(U32 j = 0; j < 5; j++)
         if(stream->readFlag())
            stream->readString(name);

      stream->readInt(5);                       // target type
      stream->readInt(9);                       // owner
      // The first 32 slots are the sensor groups' own and skip the read.
      if(i >= 32 && stream->readFlag())
         stream->readInt(11);                   // datablock id
      stream->readInt(7);                       // scale
   }
}

// The read at 0x006021b0, run over the client and server process lists.
void readProcessListState(BitStream *stream)
{
   bool flag;
   U32 word;
   stream->read(&flag);
   for(U32 i = 0; i < 4; i++)
      stream->read(&word);
}

} // namespace DemoT2
