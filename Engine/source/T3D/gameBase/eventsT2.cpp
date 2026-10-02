#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/t2Layout.h"

#define IMPLEMENT_EVENT_T2( className )                                     \
   class className : public NetEvent                                        \
   {                                                                        \
      typedef NetEvent Parent;                                              \
   public:                                                                  \
      void pack(NetConnection *, BitStream *) {}                            \
      void write(NetConnection *, BitStream *) {}                           \
      void unpack(NetConnection *, BitStream *stream)                       \
      {                                                                     \
         T2Layout::readEvent(stream, getClassName());                       \
      }                                                                     \
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
