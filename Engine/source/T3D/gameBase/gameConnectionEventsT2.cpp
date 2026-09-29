// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"

/// The events a Tribes 2 server sends about its sensor and target lists.
///
/// A client that cannot read one of these cannot read the rest of the packet
/// either, so each is here to take its own bits off the wire even where the
/// port has nothing yet to do with what it says.

//----------------------------------------------------------------------------

class ResetClientTargetsEvent : public NetEvent
{
   typedef NetEvent Parent;

public:
   ResetClientTargetsEvent() { mGuaranteeType = GuaranteedOrdered; }

   void pack(NetConnection *, BitStream *) {}
   void write(NetConnection *, BitStream *) {}
   void unpack(NetConnection *, BitStream *) {}
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(ResetClientTargetsEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(ResetClientTargetsEvent);

ConsoleDocClass( ResetClientTargetsEvent,
   "@brief Tells a client to forget every target it holds.\n\n"
   "@internal" );
