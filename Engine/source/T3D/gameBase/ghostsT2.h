// HH: Added for Tribes 2 compatibility.
#ifndef _GHOSTST2_H_
#define _GHOSTST2_H_

class NetObject;
class NetConnection;
class BitStream;

/// The ghosts Tribes 2 sends.
///
/// A ghost's update rides in the packet with no length in front of it, so a
/// client that reads one wrongly reads the rest of the packet wrongly too.
/// Each class here takes exactly the bits Tribes 2 wrote, by the field
/// widths read out of the retail binary.
class GhostT2
{
public:
   /// Read a ghost the way Tribes 2 wrote it.  False if the class is not one
   /// the port takes this way, leaving the caller to unpack it itself.
   static bool unpackUpdate(NetObject *obj, NetConnection *con, BitStream *stream);
};

#endif
