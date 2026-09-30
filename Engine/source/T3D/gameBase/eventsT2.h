// HH: Added for Tribes 2 compatibility.
#ifndef _EVENTST2_H_
#define _EVENTST2_H_

class NetEvent;
class NetConnection;
class BitStream;

/// The events Tribes 2 sends.
///
/// An event rides in the packet with no length in front of it, so a client
/// that reads one wrongly reads the rest of the packet wrongly too.  The
/// events the port acts on are unpacked by hand; the rest take exactly the
/// bits Tribes 2 wrote, by the field widths read out of the retail binary.
class EventT2
{
public:
   /// Read an event the way Tribes 2 wrote it.  False if the port unpacks
   /// this one itself.
   static bool unpack(NetEvent *evt, NetConnection *con, BitStream *stream);
};

#endif
