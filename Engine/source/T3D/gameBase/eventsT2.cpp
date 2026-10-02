#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/eventsT2.h"
#include "T3D/gameBase/t2Layout.h"

#define IMPLEMENT_EVENT_T2( className )                                     \
   class className : public NetEvent                                        \
   {                                                                        \
      typedef NetEvent Parent;                                              \
   public:                                                                  \
      void pack(NetConnection *, BitStream *) {}                            \
      void write(NetConnection *, BitStream *) {}                           \
      void unpack(NetConnection *, BitStream *) {}                          \
      void process(NetConnection *) {}                                      \
      DECLARE_CONOBJECT(className);                                         \
   };                                                                       \
   IMPLEMENT_CO_NETEVENT_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 event the port reads by "  \
                               "its layout.\n\n@internal" )

IMPLEMENT_EVENT_T2( SetServerTargetEvent );
IMPLEMENT_EVENT_T2( SimTargetAudioEvent );
IMPLEMENT_EVENT_T2( SimVoiceStreamEvent );
IMPLEMENT_EVENT_T2( TargetInfoEvent );
IMPLEMENT_EVENT_T2( TargetToEvent );

static const char *const sByHand[] =
{
   "SimDataBlockEvent", "RemoteCommandEvent", "NetStringEvent",
   "GhostAlwaysObjectEvent", "GhostingMessageEvent", "CRCChallengeEvent",
   "SetMissionCRCEvent", "SimpleMessageEvent", "LightningStrikeEvent",
   "Sim2DAudioEvent", "Sim3DAudioEvent"
};

bool EventT2::unpack( NetEvent *evt, NetConnection *, BitStream *stream )
{
   const char *name = evt->getClassName();

   for( U32 i = 0; i < sizeof( sByHand ) / sizeof( sByHand[ 0 ] ); i++ )
      if( !dStrcmp( name, sByHand[ i ] ) )
         return false;

   return T2Layout::readEvent( stream, name );
}
