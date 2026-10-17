// HH: Added for Tribes 2 compatibility.
// The world's one gravity, the event that carries it to a client, and the
// pair of console functions that set and read it.
#include "platform/platform.h"
#include "T3D/gameBase/gravityT2.h"

#include "console/engineAPI.h"
#include "console/simBase.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"

F32 gWorldGravity = -20.0f;

//----------------------------------------------------------------------------

class GravityEvent : public NetEvent
{
   typedef NetEvent Parent;
   F32 mGravity;

public:
   GravityEvent(F32 gravity = 0.0f)
   {
      mGuaranteeType = GuaranteedOrdered;
      mGravity = gravity;
   }

   void pack(NetConnection *, BitStream *stream) { stream->write(mGravity); }
   void write(NetConnection *, BitStream *stream) { stream->write(mGravity); }
   void unpack(NetConnection *, BitStream *stream) { stream->read(&mGravity); }

      // What retail's own process does (0x005ff5e0): the mission's gravity
      // into the one global every falling thing reads. Classic's server.cs
      // runs at -26.9 rather than the engine's own -20, and a client that
      // ignored this predicts its own fall against the wrong number.
   void process(NetConnection *) { gWorldGravity = mGravity; }

   DECLARE_CONOBJECT(GravityEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(GravityEvent);

ConsoleDocClass( GravityEvent,
   "@brief Gives a client the mission's gravity.\n\n@internal" );

void postWorldGravity(NetConnection *conn)
{
   if (conn)
      conn->postNetEvent(new GravityEvent(gWorldGravity));
}

//----------------------------------------------------------------------------

// What retail's setGravity does (0x005f9df0): set the global, then tell every
// client of this server, so the clients predict their fall at the rate the
// server simulates it.
DefineEngineFunction( setGravity, void, ( F32 gravity ),,
   "@brief Set the world's gravity and send it to every connected client.\n\n"
   "@ingroup GameObjects\n")
{
   gWorldGravity = gravity;

   SimGroup *clients = Sim::getClientGroup();
   for (SimGroup::iterator itr = clients->begin(); itr != clients->end(); itr++)
      postWorldGravity(dynamic_cast<NetConnection *>(*itr));
}

DefineEngineFunction( getGravity, F32, (),,
   "@brief Get the world's gravity.\n\n"
   "@ingroup GameObjects\n")
{
   return gWorldGravity;
}
