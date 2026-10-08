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

   static const T2DataNode sT2ErNodes_SimTargetAudioEvent[] =
   {
      { OpInt, 9, 1, -1 },
      { OpInt, 12, 2, -1 },
      { OpInt, 10, 3, -1 },
      { OpFlag, 1, 5, 4 },
      { OpPoint, 0, 5, -1 },
      { OpFlag, 1, 6, 6 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2ErNodes_TargetToEvent[] =
   {
      { OpFlag, 1, 2, 1 },
      { OpInt, 9, 2, -1 },
      { OpFlag, 1, 6, 3 },
      { OpInt, 32, 4, -1 },
      { OpInt, 32, 5, -1 },
      { OpInt, 32, 6, -1 },
      { OpFlag, 1, 7, 7 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataBlockLayout sT2EventsRecovered[] =
   {
      { "SensorGroupColorEvent", sT2ErNodes_SensorGroupColorEvent, 10, NULL, 9 },
      { "SimTargetAudioEvent", sT2ErNodes_SimTargetAudioEvent, 7, NULL, 0 },
      { "TargetToEvent", sT2ErNodes_TargetToEvent, 8, NULL, 0 },
   };
}
