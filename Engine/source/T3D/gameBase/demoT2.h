// HH: Added for Tribes 2 compatibility.
#ifndef _DEMOT2_H_
#define _DEMOT2_H_

#include "platform/types.h"

class BitStream;
class Stream;

/// Tribes 2's recording container and the two start-block steps that have no
/// Torque counterpart at all.
namespace DemoT2
{
   /// The word retail compares for exact equality: protocol 51, format 4.
   static const U32 Version = 0x00330004;

   bool readHeader(Stream *stream, U32 *version, U32 *lengthMs, U32 *startSize);

   void readTargetManagerState(BitStream *stream);
   void readProcessListState(BitStream *stream);
}

#endif
