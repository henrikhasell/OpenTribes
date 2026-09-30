// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/eventsT2.h"

/// The events Tribes 2 sends.
///
/// An event rides in the packet with no length in front of it, so a client
/// that reads one wrongly reads the rest of the packet wrongly too.  The
/// events the port acts on are unpacked by hand; the rest take exactly the
/// bits Tribes 2 wrote, by the field widths read out of the retail binary
/// by scratchpad/genprog.py.

/// Give a name Tribes 2 sends and the port has no class for an event of its
/// own, whose contents are read by their field widths.
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
   ConsoleDocClass( className, "@brief A Tribes 2 event read by its field " \
                               "widths.\n\n@internal" )

static const U16 OpEnd    = DataBlockT2::OpEnd;
static const U16 OpFlag   = DataBlockT2::OpFlag;
static const U16 OpInt    = DataBlockT2::OpInt;
static const U16 OpFloat  = DataBlockT2::OpFloat;
static const U16 OpRaw    = DataBlockT2::OpRaw;
static const U16 OpString = DataBlockT2::OpString;
static const U16 OpBlock  = DataBlockT2::OpBlock;
static const U16 OpIf     = DataBlockT2::OpIf;
static const U16 OpIfKept = DataBlockT2::OpIfKept;
static const U16 OpLoop   = DataBlockT2::OpLoop;
static const U16 OpTimes  = DataBlockT2::OpTimes;

static const U16 sProgCRCChallengeResponseEvent[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgFogChallengeEvent[] = { OpInt, 16, OpEnd };
static const U16 sProgGravityEvent[] = { OpRaw, 4, OpEnd };
static const U16 sProgLightningStrikeEvent[] = { OpIf, 1, OpEnd, OpInt, 11, OpFloat, 10, OpFloat, 10, OpIf, 3, OpInt, 11, OpEnd, OpEnd };
static const U16 sProgPathManagerEvent[] = { OpIf, 1, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgRemoveClientTargetTypeEvent[] = { OpInt, 2, OpEnd };
static const U16 sProgResetClientTargetsEvent[] = { OpFlag, OpEnd };
static const U16 sProgSensorGroupColorEvent[] = { OpInt, 5, OpRaw, 4, OpIf, 9, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpEnd, OpEnd };
static const U16 sProgSetObjectActiveImageEvent[] = { OpInt, 10, OpInt, 4, OpEnd };
static const U16 sProgSetSensorGroupEvent[] = { OpInt, 5, OpEnd };
static const U16 sProgSetServerTargetEvent[] = { OpIf, 3, OpInt, 9, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgSim2DAudioEvent[] = { OpBlock, OpEnd };
static const U16 sProgSim3DAudioEvent[] = { OpBlock, OpIf, 12, OpFloat, 0, OpFloat, 0, OpFloat, 0, OpRaw, 4, OpIf, 1, OpEnd, OpEnd, OpEnd };
static const U16 sProgSimTargetAudioEvent[] = { OpInt, 9, OpInt, 10, OpIf, 3, OpInt, 11, OpEnd, OpIf, 1, OpEnd, OpFlag, OpEnd };
static const U16 sProgSimVoiceStreamEvent[] = { OpInt, 5, OpInt, 6, OpInt, 2, OpRaw, 4, OpInt, 10, OpIf, 3, OpInt, 5, OpEnd, OpRaw, 4, OpInt, 7, OpInt, 2, OpInt, 2, OpRaw, 4, OpIfKept, 8, OpFlag, OpInt, 5, OpInt, 4, OpInt, 7, OpEnd, OpIf, 3, OpInt, 5, OpEnd, OpRaw, 4, OpEnd };
static const U16 sProgSimpleMessageEvent[] = { OpString, OpEnd };
static const U16 sProgTargetFreeEvent[] = { OpInt, 9, OpEnd };
static const U16 sProgTargetInfoEvent[] = { OpInt, 9, OpIf, 6, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpIf, 3, OpInt, 5, OpEnd, OpIf, 6, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpIf, 3, OpInt, 9, OpEnd, OpIf, 3, OpFloat, 7, OpEnd, OpEnd };
static const U16 sProgTargetToEvent[] = { OpEnd };

IMPLEMENT_EVENT_T2( SetServerTargetEvent );
IMPLEMENT_EVENT_T2( SimTargetAudioEvent );
IMPLEMENT_EVENT_T2( SimVoiceStreamEvent );
IMPLEMENT_EVENT_T2( TargetInfoEvent );
IMPLEMENT_EVENT_T2( TargetToEvent );

static const struct EventEntry
{
   const char *name;
   const U16  *program;
} sEventTable[] =
{
   { "CRCChallengeResponseEvent", sProgCRCChallengeResponseEvent },
   { "FogChallengeEvent", sProgFogChallengeEvent },
   { "GravityEvent", sProgGravityEvent },
   { "LightningStrikeEvent", sProgLightningStrikeEvent },
   { "PathManagerEvent", sProgPathManagerEvent },
   { "RemoveClientTargetTypeEvent", sProgRemoveClientTargetTypeEvent },
   { "ResetClientTargetsEvent", sProgResetClientTargetsEvent },
   { "SensorGroupColorEvent", sProgSensorGroupColorEvent },
   { "SetObjectActiveImageEvent", sProgSetObjectActiveImageEvent },
   { "SetSensorGroupEvent", sProgSetSensorGroupEvent },
   { "SetServerTargetEvent", sProgSetServerTargetEvent },
   { "Sim2DAudioEvent", sProgSim2DAudioEvent },
   { "Sim3DAudioEvent", sProgSim3DAudioEvent },
   { "SimTargetAudioEvent", sProgSimTargetAudioEvent },
   { "SimVoiceStreamEvent", sProgSimVoiceStreamEvent },
   { "SimpleMessageEvent", sProgSimpleMessageEvent },
   { "TargetFreeEvent", sProgTargetFreeEvent },
   { "TargetInfoEvent", sProgTargetInfoEvent },
   { "TargetToEvent", sProgTargetToEvent },
};

bool EventT2::unpack( NetEvent *evt, NetConnection *con, BitStream *stream )
{
   const char *name = evt->getClassName();
   for( U32 i = 0; i < sizeof( sEventTable ) / sizeof( sEventTable[ 0 ] ); i++ )
   {
      if( dStrcmp( name, sEventTable[ i ].name ) )
         continue;

      Vector< U32 >              value;
      Vector< StringTableEntry > string;
      if( Con::getBoolVariable( "$T2::traceEvents" ) )
         Con::printf( "EVT %s at %d", name, stream->getCurPos() );
      DataBlockT2::runProgram( stream, sEventTable[ i ].program, value, string );
      return true;
   }

   return false;
}
