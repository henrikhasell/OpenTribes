/// @file
/// The bit layout of readPacketData for the Tribes 2 classes a client drives.
///
/// This is a third layout per class, separate from the ghost layout in
/// t2GhostTables.h: a ghost's unpackUpdate carries what changed about an
/// object anyone can see, while readPacketData carries the per-tick state of
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
   // Camera::readPacketData, 0x005cc530 in Tribes2.exe.  Five floats -- the
   // position and two angles -- then three bits of mode, from
   // getBinLog2(getNextPow2(5)).  Modes 3 and 4 add three more floats; mode 3
   // then names an object by ghost index, and mode 4 goes on into a helper on
   // the connection that this has not followed yet, so it stops there rather
   // than guessing.  Every other mode ends after the first five floats.
   static const S16 sT2PdSel_Camera[] = { 11, 11, 11, 6, 12, 11, 11, 11 };
   static const T2DataNode sT2PdNodes_Camera[] =
   {
      { OpRaw,   32,  1, -1 },   //  0  position x
      { OpRaw,   32,  2, -1 },   //  1  position y
      { OpRaw,   32,  3, -1 },   //  2  position z
      { OpRaw,   32,  4, -1 },   //  3
      { OpRaw,   32,  5, -1 },   //  4
      { OpSelect, 3,  0, -1 },   //  5  mode
      { OpRaw,   32,  7, -1 },   //  6  mode 3
      { OpRaw,   32,  8, -1 },   //  7
      { OpRaw,   32,  9, -1 },   //  8
      { OpRaw,    1, 10, -1 },   //  9
      { OpRaw,   10, 11, -1 },   // 10  ghost index
      { OpEnd,    0, -1, -1 },   // 11
      { OpRaw,   32, 13, -1 },   // 12  mode 4
      { OpRaw,   32, 14, -1 },   // 13
      { OpRaw,   32, 99, -1 },   // 14  and then something not read yet
   };

   static const T2DataBlockLayout sT2PacketData[] =
   {
      { "Camera", sT2PdNodes_Camera, 15, sT2PdSel_Camera, 0 },
   };
}

#endif
