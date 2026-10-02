/// @file
/// What the object a client drives sends each tick, per class.
///
/// Tribes 2 sends it through readPacketData -- vtable slot 66 -- rather
/// than through unpackUpdate, so it is a table of its own.  The shipped
/// t2PacketDataTables.h has only Camera, which is what a client watching
/// a mission is given; the moment it spawns the control object is a
/// Player, and without a layout for that the packet could not be read at
/// all.
///
/// Player's is the one with a shape no walk could recover: when the player
/// is piloting, what follows is the vehicle's own packet data, and how
/// wide that is depends on which ghost the index names rather than on any
/// bit in the stream.  OpPacketDataOf asks the connection.
///
/// Written by hand against a session; see BananaEngine's tools/t2.

#ifndef _T2PACKETDATATABLESRECOVERED_H_
#define _T2PACKETDATATABLESRECOVERED_H_

// HH: Added for Tribes 2 compatibility.

namespace T2Protocol
{
   /// FlyingVehicle: FlyingVehicle::readPacketData, which is Vehicle's unchanged.
   static const T2DataNode sT2PrNodes_FlyingVehicle[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpFlag, 1, 0, 0 },
      { OpFlag, 1, 1, 1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 32, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpRaw, 32, 10, -1 },
      { OpRaw, 32, 11, -1 },
      { OpRaw, 32, 12, -1 },
      { OpRaw, 32, 13, -1 },
      { OpRaw, 32, 14, -1 },
      { OpRaw, 32, 15, -1 },
      { OpRaw, 32, 16, -1 },
      { OpRaw, 32, 17, -1 },
      { OpRaw, 32, 18, -1 },
   };

   /// HoverVehicle: HoverVehicle::readPacketData, which is Vehicle's unchanged.
   static const T2DataNode sT2PrNodes_HoverVehicle[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpFlag, 1, 0, 0 },
      { OpFlag, 1, 1, 1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 32, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpRaw, 32, 10, -1 },
      { OpRaw, 32, 11, -1 },
      { OpRaw, 32, 12, -1 },
      { OpRaw, 32, 13, -1 },
      { OpRaw, 32, 14, -1 },
      { OpRaw, 32, 15, -1 },
      { OpRaw, 32, 16, -1 },
      { OpRaw, 32, 17, -1 },
      { OpRaw, 32, 18, -1 },
   };

   /// Player: Player::readPacketData, 0x5dab20 -- the control object a client gets the moment it spawns, which the shipped table had no layout for at all.  Cross-checked against t2-demo-parser (MIT, github.com/exogen/t2-demo-parser).
   static const T2DataNode sT2PrNodes_Player[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpFlag, 1, 0, 0 },
      { OpFlag, 1, 1, 1 },
      { OpPacketDataOf, 10, 2, -1 },
      { OpFlag, 1, 2, 3 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 4, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpRaw, 32, 10, -1 },
      { OpRaw, 32, 11, -1 },
      { OpRaw, 32, 12, -1 },
      { OpRaw, 32, 13, -1 },
      { OpFlag, 1, 7, 14 },
      { OpRaw, 7, 15, -1 },
      { OpFlag, 1, 15, 16 },
      { OpRaw, 7, 17, -1 },
      { OpFlag, 1, 17, 18 },
      { OpRaw, 3, 19, -1 },
      { OpRaw, 32, 20, -1 },
      { OpRaw, 32, 21, -1 },
   };

   /// ShapeBase: ShapeBase::readPacketData, the layer every control object opens with.
   static const T2DataNode sT2PrNodes_ShapeBase[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpRaw, 32, 0, -1 },
      { OpRaw, 32, 1, -1 },
   };

   /// Turret: Turret::readPacketData.
   static const T2DataNode sT2PrNodes_Turret[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpRaw, 32, 0, -1 },
      { OpRaw, 32, 1, -1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 3, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
   };

   /// Vehicle: Vehicle::readPacketData, 0x60d740.
   static const T2DataNode sT2PrNodes_Vehicle[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpFlag, 1, 0, 0 },
      { OpFlag, 1, 1, 1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 32, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpRaw, 32, 10, -1 },
      { OpRaw, 32, 11, -1 },
      { OpRaw, 32, 12, -1 },
      { OpRaw, 32, 13, -1 },
      { OpRaw, 32, 14, -1 },
      { OpRaw, 32, 15, -1 },
      { OpRaw, 32, 16, -1 },
      { OpRaw, 32, 17, -1 },
      { OpRaw, 32, 18, -1 },
   };

   /// WheeledVehicle: WheeledVehicle::readPacketData, 0x615840 -- its parent's, then the brake and the wheels.
   static const T2DataNode sT2PrNodes_WheeledVehicle[] =
   {
      { OpEnd, 0, -1, -1 },
      { OpRaw, 32, 0, -1 },
      { OpRaw, 32, 1, -1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 32, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpRaw, 32, 10, -1 },
      { OpRaw, 32, 11, -1 },
      { OpRaw, 32, 12, -1 },
      { OpRaw, 32, 13, -1 },
      { OpRaw, 32, 14, -1 },
      { OpRaw, 32, 15, -1 },
      { OpRaw, 32, 16, -1 },
      { OpRaw, 32, 17, -1 },
      { OpFlag, 1, 18, 18 },
      { OpFlag, 1, 19, 19 },
      { OpFlag, 1, 20, 20 },
      { OpRaw, 32, 21, -1 },
      { OpRaw, 32, 22, -1 },
      { OpRaw, 32, 23, -1 },
      { OpRaw, 32, 24, -1 },
      { OpRaw, 32, 25, -1 },
      { OpRaw, 32, 26, -1 },
      { OpRaw, 32, 27, -1 },
      { OpRaw, 32, 28, -1 },
      { OpRaw, 32, 29, -1 },
      { OpRaw, 32, 30, -1 },
      { OpRaw, 32, 31, -1 },
      { OpRaw, 32, 32, -1 },
      { OpRaw, 32, 33, -1 },
      { OpRaw, 32, 34, -1 },
      { OpRaw, 32, 35, -1 },
      { OpRaw, 32, 36, -1 },
      { OpRaw, 32, 37, -1 },
   };

   static const T2DataBlockLayout sT2PacketDataRecovered[] =
   {
      { "FlyingVehicle", sT2PrNodes_FlyingVehicle, 20, NULL, 19 },
      { "HoverVehicle", sT2PrNodes_HoverVehicle, 20, NULL, 19 },
      { "Player", sT2PrNodes_Player, 23, NULL, 22 },
      { "ShapeBase", sT2PrNodes_ShapeBase, 3, NULL, 2 },
      { "Turret", sT2PrNodes_Turret, 7, NULL, 6 },
      { "Vehicle", sT2PrNodes_Vehicle, 20, NULL, 19 },
      { "WheeledVehicle", sT2PrNodes_WheeledVehicle, 39, NULL, 38 },
   };
}

#endif
