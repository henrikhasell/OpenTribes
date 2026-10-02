// HH: Added for Tribes 2 compatibility.
#ifndef _EVENTST2_H_
#define _EVENTST2_H_

class NetEvent;
class NetConnection;
class BitStream;

class EventT2
{
public:
   static bool unpack(NetEvent *evt, NetConnection *con, BitStream *stream);
};

#endif
