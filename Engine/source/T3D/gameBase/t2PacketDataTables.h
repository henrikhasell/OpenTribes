/// @file
/// The bit layout of readPacketData for the Tribes 2 classes a client drives.
///
/// This is a third layout per class, separate from the ghost layout in
/// t2GhostTablesRecovered.h: a ghost's unpackUpdate carries what changed about
/// an object anyone can see, while readPacketData carries the per-tick state of
/// the one object this client controls -- where it is, how fast, which way it
/// is looking -- and the server sends it in the packet header rather than in
/// the ghost stream.  Same class, different fields, different order.
///
/// GameConnection::readPacket calls it through the object's vtable at +0x108,
/// on the object the ghost index just read resolves to.

#ifndef _T2PACKETDATATABLES_H_
#define _T2PACKETDATATABLES_H_

namespace T2Protocol
{
   // HH: Modified for Tribes 2 compatibility.
   // Camera::readPacketData, 0x005cc530 in Tribes2.exe.  It begins with its
   // parent's: ShapeBase::readPacketData sends the energy level and the
   // recharge rate, two whole floats, and leaving them out cost sixty-four
   // bits at the head of every packet carrying control data -- which threw
   // out the event section after it and lost the rest of the packet.
   //
   // Then the camera's own: a position, two angles, and three bits of mode
   // from getBinLog2(getNextPow2(5)).  Modes 3 and 4 add three orbit
   // distances; mode 3 then names an object by ghost index, and mode 4 reads
   // a compressed point.  Every other mode ends after the angles.
   //
   // Cross-checked against t2-demo-parser (MIT,
   // github.com/exogen/t2-demo-parser) on a recorded session: 227 bits.
   static const S16 sT2PdSel_Camera[] = { 13, 13, 13, 8, 14, 13, 13, 13 };
   static const T2DataNode sT2PdNodes_Camera[] =
   {
      { OpRaw,   32,  1, -1 },   //  0  energy level, from ShapeBase
      { OpRaw,   32,  2, -1 },   //  1  recharge rate, likewise
      { OpRaw,   32,  3, -1 },   //  2  position x
      { OpRaw,   32,  4, -1 },   //  3  position y
      { OpRaw,   32,  5, -1 },   //  4  position z
      { OpRaw,   32,  6, -1 },   //  5  rotation x
      { OpRaw,   32,  7, -1 },   //  6  rotation z
      { OpSelect, 3,  0, -1 },   //  7  mode
      { OpRaw,   32,  9, -1 },   //  8  mode 3: minimum orbit distance
      { OpRaw,   32, 10, -1 },   //  9            maximum
      { OpRaw,   32, 11, -1 },   // 10            current
      { OpRaw,    1, 12, -1 },   // 11  observing this client's own object
      { OpRaw,   10, 13, -1 },   // 12  the object orbited, by ghost index
      { OpEnd,    0, -1, -1 },   // 13
      { OpRaw,   32, 15, -1 },   // 14  mode 4: the same three distances
      { OpRaw,   32, 16, -1 },   // 15
      { OpRaw,   32, 17, -1 },   // 16
      { OpPoint,  0, 13, -1 },   // 17  and the point orbited, compressed
   };

   static const T2DataBlockLayout sT2PacketData[] =
   {
      { "Camera", sT2PdNodes_Camera, 18, sT2PdSel_Camera, 0 },
   };
}

#endif
