/// @file
/// Event layouts recovered by walking unpack, for the events that had no
/// reader at all.
///
/// An event's unpack is not at a fixed vtable slot the way unpackUpdate
/// is: it sits four words past the .data word holding the class's
/// getClassRep.  Finding it that way brought back all twenty-six, and
/// every one that t2EventTables.h already carried came back with the same
/// node count -- an independent check on a table recovered another way.
/// Only the ones nothing could read are written here; the rest keep
/// whatever already reads them.
///
/// Recovered by Tools/t2/eventwalk.py; see Tools/t2/README.md.

// HH: Added for Tribes 2 compatibility.

namespace T2Protocol
{
   /// SensorGroupColorEvent: The shipped table and eventwalk.py both give this fifty-eight nodes and both read 111 bits where a session carries 133.  Its colours are not a counted list: it sends a thirty-two bit mask and then an entry for each bit standing in it, each of those being a flag and, if set, four bytes of colour.  No op could say that until OpCountBits, which keeps a mask's population count.  Measured on a spawned session: the mask came back all ones with two colours carried, 5 + 32 + 32 + 2*32 = 133.  Cross-checked against t2-demo-parser (MIT, github.com/exogen/t2-demo-parser).
   static const T2DataNode sT2ErNodes_SensorGroupColorEvent[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpEndBody, 0, -1, -1 },
      { OpRaw, 8, 1, -1 },
      { OpRaw, 8, 2, -1 },
      { OpRaw, 8, 3, -1 },
      { OpRaw, 8, 4, -1 },
      { OpFlag, 1, 1, 5 },
      { OpLoop, 0, 0, 6 },
      { OpCountBits, 32, 7, 0 },
      { OpRaw, 5, 8, -1 },
   };

   /// SimTargetAudioEvent: 7 nodes from 26 explored, 1 call(s) stepped over
   static const T2DataNode sT2ErNodes_SimTargetAudioEvent[] =
   {
      { OpInt, 9, 1, -1 },
      { OpInt, 10, 2, -1 },
      { OpFlag, 1, 3, 6 },
      { OpFlag, 1, 4, 4 },
      { OpFlag, 1, 5, 5 },
      { OpEnd, 0, -1, -1 },
      { OpInt, 11, 3, -1 },
   };

   /// TargetToEvent: 1 nodes from 2 explored, exact
   static const T2DataNode sT2ErNodes_TargetToEvent[] =
   {
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataBlockLayout sT2EventsRecovered[] =
   {
      { "SensorGroupColorEvent", sT2ErNodes_SensorGroupColorEvent, 10, NULL, 9 },
      { "SimTargetAudioEvent", sT2ErNodes_SimTargetAudioEvent, 7, NULL, 0 },
      { "TargetToEvent", sT2ErNodes_TargetToEvent, 1, NULL, 0 },
   };
}
