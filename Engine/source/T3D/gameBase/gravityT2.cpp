// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/gravityT2.h"

#include "console/engineAPI.h"
#include "console/simBase.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"

F32 gWorldGravity = -20.0f;

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
