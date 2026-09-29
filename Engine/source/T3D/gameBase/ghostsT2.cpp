// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netObject.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/ghostsT2.h"

/// The ghosts Tribes 2 sends.
///
/// A ghost's update rides in the packet with no length in front of it, so a
/// client that reads one wrongly reads the rest of the packet wrongly too.
/// Each class here takes exactly the bits Tribes 2 wrote, by the field
/// widths read out of the retail binary.

/// Give a name Tribes 2 ghosts and the port has no class for an object of
/// its own, whose update is read by its field widths.
#define IMPLEMENT_GHOST_T2( className )                                     \
   class className : public NetObject                                       \
   {                                                                        \
      typedef NetObject Parent;                                             \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
   };                                                                       \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                    \
   ConsoleDocClass( className, "@brief A Tribes 2 ghost read by its field " \
                               "widths.\n\n@internal" )

// --- generated read programs ---

// HH: Added for Tribes 2 compatibility.
// Short names for the program words, which are read out of the retail binary
// by scratchpad/genprog.py -- one entry per field, in the wire's order.
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

static const U16 sProgAIObjective[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgAudioEmitter[] = { OpFlag, OpIf, 1, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 1, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 1, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 1, OpEnd, OpString, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpIf, 1, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgBeaconObject[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 21, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpFlag, OpIf, 3, OpInt, 2, OpEnd, OpEnd };
static const U16 sProgBombProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 50, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpInt, 12, OpIf, 6, OpIfKept, 3, OpRaw, 1, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgCamera[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgDebris[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpEnd };
static const U16 sProgELFProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 10, OpIf, 7, OpInt, 11, OpInt, 3, OpInt, 11, OpEnd, OpEnd, OpEnd };
static const U16 sProgEnergyProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 82, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpInt, 12, OpRaw, 4, OpIf, 63, OpIfKept, 3, OpRaw, 1, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpIf, 3, OpInt, 11, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd, OpEnd, OpEnd };
static const U16 sProgFireballAtmosphere[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgFlareProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 50, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpInt, 12, OpIf, 6, OpIfKept, 3, OpRaw, 1, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgFlyingVehicle[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgForceFieldBare[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 2, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgGameBase[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpEnd };
static const U16 sProgGrenadeProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 50, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpInt, 12, OpIf, 6, OpIfKept, 3, OpRaw, 1, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgHoverVehicle[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpInt, 3, OpEnd };
static const U16 sProgInteriorInstance[] = { OpIf, 23, OpRaw, 4, OpString, OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpString, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 9, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpFlag, OpIf, 4, OpIf, 1, OpEnd, OpEnd, OpIf, 2, OpString, OpEnd, OpIf, 9, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd, OpEnd };
static const U16 sProgItem[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 13, OpFlag, OpFlag, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd, OpIf, 3, OpInt, 10, OpEnd, OpIf, 6, OpIf, 1, OpEnd, OpRaw, 4, OpEnd, OpIf, 76, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 58, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIfKept, 1, OpEnd, OpIfKept, 1, OpEnd, OpIfKept, 1, OpEnd, OpIfKept, 1, OpEnd, OpIfKept, 1, OpEnd, OpIfKept, 1, OpEnd, OpRaw, 4, OpIfKept, 19, OpString, OpIfKept, 9, OpIfKept, 6, OpIfKept, 3, OpRaw, 4, OpEnd, OpEnd, OpEnd, OpString, OpIfKept, 1, OpEnd, OpRaw, 4, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd, OpRaw, 4, OpEnd };
static const U16 sProgLightning[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 37, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgLinearFlareProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 36, OpIf, 28, OpInt, 15, OpInt, 14, OpInt, 9, OpIf, 14, OpInt, 10, OpInt, 3, OpIf, 7, OpInt, 8, OpInt, 8, OpInt, 7, OpEnd, OpEnd, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd };
static const U16 sProgLinearProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 36, OpIf, 28, OpInt, 15, OpInt, 14, OpInt, 9, OpIf, 14, OpInt, 10, OpInt, 3, OpIf, 7, OpInt, 8, OpInt, 8, OpInt, 7, OpEnd, OpEnd, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd };
static const U16 sProgMarker[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgMissionArea[] = { OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgMissionMarker[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 21, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgParticleEmissionDummy[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpEnd };
static const U16 sProgPhysicalZone[] = { OpIf, 48, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpEnd, OpFlag, OpEnd };
static const U16 sProgPlayer[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 3, OpInt, 3, OpEnd, OpIf, 20, OpInt, 8, OpFlag, OpFlag, OpFlag, OpIf, 3, OpInt, 6, OpEnd, OpIfKept, 1, OpEnd, OpIfKept, 4, OpIfKept, 1, OpEnd, OpEnd, OpEnd, OpIf, 3, OpInt, 8, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgPrecipitation[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 26, OpRaw, 4, OpRaw, 4, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd, OpIf, 6, OpRaw, 1, OpIfKept, 1, OpEnd, OpEnd, OpIf, 5, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpEnd };
static const U16 sProgRepairProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 10, OpIf, 7, OpInt, 11, OpInt, 3, OpInt, 11, OpEnd, OpEnd, OpEnd };
static const U16 sProgScopeAlwaysShape[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 23, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpFlag, OpEnd };
static const U16 sProgSeekerProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 43, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpIf, 12, OpIf, 3, OpInt, 11, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 1, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgShapeBase[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgShockLanceProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 3, OpInt, 11, OpEnd, OpIf, 21, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 5, OpInt, 11, OpInt, 3, OpEnd, OpEnd, OpEnd };
static const U16 sProgShockwave[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 13, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgSimpleNetObject[] = { OpString, OpEnd };
static const U16 sProgSky[] = { OpIf, 74, OpString, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 3, 6, OpString, OpRaw, 4, OpRaw, 4, OpEnd, OpRaw, 1, OpRaw, 4, OpLoop, 3, 3, OpRaw, 4, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 11, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd, OpIf, 3, OpRaw, 1, OpEnd, OpIf, 6, OpRaw, 1, OpIfKept, 1, OpEnd, OpEnd, OpIf, 15, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 4, OpLoop, 3, 3, OpRaw, 4, OpEnd, OpEnd, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 9, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgSniperProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 25, OpFloat, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpFlag, OpIf, 6, OpInt, 11, OpInt, 3, OpFlag, OpEnd, OpEnd, OpIf, 6, OpInt, 11, OpInt, 3, OpFlag, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpEnd };
static const U16 sProgSpawnSphere[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 9, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgSplash[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgStaticShape[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 23, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpFlag, OpEnd };
static const U16 sProgStationFXPersonal[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpEnd };
static const U16 sProgStationFXVehicle[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 6, OpIf, 3, OpInt, 11, OpEnd, OpEnd, OpEnd };
static const U16 sProgSun[] = { OpIf, 9, OpLoop, 5, 5, OpString, OpIfKept, 1, OpEnd, OpEnd, OpEnd, OpIf, 39, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgTSStatic[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpString, OpEnd };
static const U16 sProgTargetProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 22, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 6, OpInt, 11, OpInt, 3, OpFlag, OpEnd, OpEnd, OpIf, 6, OpInt, 11, OpInt, 3, OpFlag, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpEnd };
static const U16 sProgTerrainBlock[] = { OpIf, 11, OpRaw, 4, OpString, OpString, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 5, OpRaw, 4, OpRaw, 4, OpEnd, OpEnd };
static const U16 sProgTracerProjectile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 36, OpIf, 28, OpInt, 15, OpInt, 14, OpInt, 9, OpIf, 14, OpInt, 10, OpInt, 3, OpIf, 7, OpInt, 8, OpInt, 8, OpInt, 7, OpEnd, OpEnd, OpIf, 3, OpInt, 10, OpEnd, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd, OpInt, 15, OpInt, 14, OpFlag, OpEnd };
static const U16 sProgTrigger[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgTurret[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 21, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpFlag, OpIf, 3, OpFloat, 8, OpEnd, OpIf, 1, OpEnd, OpEnd };
static const U16 sProgVehicleBlocker[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgWaterBlock[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpString, OpString, OpLoop, 2, 2, OpString, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sProgWayPoint[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpIf, 19, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpString, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 2, OpFlag, OpEnd, OpEnd };
static const U16 sProgWheeledVehicle[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 6, OpIf, 3, OpInt, 9, OpEnd, OpEnd, OpIf, 1, OpEnd, OpFlag, OpIf, 1, OpEnd, OpEnd };

IMPLEMENT_GHOST_T2( AIObjective );
IMPLEMENT_GHOST_T2( AudioEmitter );
IMPLEMENT_GHOST_T2( BeaconObject );
IMPLEMENT_GHOST_T2( BombProjectile );
IMPLEMENT_GHOST_T2( ELFProjectile );
IMPLEMENT_GHOST_T2( EnergyProjectile );
IMPLEMENT_GHOST_T2( FireballAtmosphere );
IMPLEMENT_GHOST_T2( FlareProjectile );
IMPLEMENT_GHOST_T2( ForceFieldBare );
IMPLEMENT_GHOST_T2( GrenadeProjectile );
IMPLEMENT_GHOST_T2( LinearFlareProjectile );
IMPLEMENT_GHOST_T2( LinearProjectile );
IMPLEMENT_GHOST_T2( ParticleEmissionDummy );
IMPLEMENT_GHOST_T2( RepairProjectile );
IMPLEMENT_GHOST_T2( SeekerProjectile );
IMPLEMENT_GHOST_T2( ShockLanceProjectile );
IMPLEMENT_GHOST_T2( Shockwave );
IMPLEMENT_GHOST_T2( SniperProjectile );
IMPLEMENT_GHOST_T2( StationFXPersonal );
IMPLEMENT_GHOST_T2( StationFXVehicle );
IMPLEMENT_GHOST_T2( TargetProjectile );
IMPLEMENT_GHOST_T2( TracerProjectile );
IMPLEMENT_GHOST_T2( Turret );

static const struct GhostEntry
{
   const char *name;
   const U16  *program;
} sGhostTable[] =
{
   { "AIObjective", sProgAIObjective },
   { "AudioEmitter", sProgAudioEmitter },
   { "BeaconObject", sProgBeaconObject },
   { "BombProjectile", sProgBombProjectile },
   { "Camera", sProgCamera },
   { "Debris", sProgDebris },
   { "ELFProjectile", sProgELFProjectile },
   { "EnergyProjectile", sProgEnergyProjectile },
   { "FireballAtmosphere", sProgFireballAtmosphere },
   { "FlareProjectile", sProgFlareProjectile },
   { "FlyingVehicle", sProgFlyingVehicle },
   { "ForceFieldBare", sProgForceFieldBare },
   { "GameBase", sProgGameBase },
   { "GrenadeProjectile", sProgGrenadeProjectile },
   { "HoverVehicle", sProgHoverVehicle },
   { "InteriorInstance", sProgInteriorInstance },
   { "Item", sProgItem },
   { "Lightning", sProgLightning },
   { "LinearFlareProjectile", sProgLinearFlareProjectile },
   { "LinearProjectile", sProgLinearProjectile },
   { "Marker", sProgMarker },
   { "MissionArea", sProgMissionArea },
   { "MissionMarker", sProgMissionMarker },
   { "ParticleEmissionDummy", sProgParticleEmissionDummy },
   { "PhysicalZone", sProgPhysicalZone },
   { "Player", sProgPlayer },
   { "Precipitation", sProgPrecipitation },
   { "Projectile", sProgProjectile },
   { "RepairProjectile", sProgRepairProjectile },
   { "ScopeAlwaysShape", sProgScopeAlwaysShape },
   { "SeekerProjectile", sProgSeekerProjectile },
   { "ShapeBase", sProgShapeBase },
   { "ShockLanceProjectile", sProgShockLanceProjectile },
   { "Shockwave", sProgShockwave },
   { "SimpleNetObject", sProgSimpleNetObject },
   { "Sky", sProgSky },
   { "SniperProjectile", sProgSniperProjectile },
   { "SpawnSphere", sProgSpawnSphere },
   { "Splash", sProgSplash },
   { "StaticShape", sProgStaticShape },
   { "StationFXPersonal", sProgStationFXPersonal },
   { "StationFXVehicle", sProgStationFXVehicle },
   { "Sun", sProgSun },
   { "TSStatic", sProgTSStatic },
   { "TargetProjectile", sProgTargetProjectile },
   { "TerrainBlock", sProgTerrainBlock },
   { "TracerProjectile", sProgTracerProjectile },
   { "Trigger", sProgTrigger },
   { "Turret", sProgTurret },
   { "VehicleBlocker", sProgVehicleBlocker },
   { "WaterBlock", sProgWaterBlock },
   { "WayPoint", sProgWayPoint },
   { "WheeledVehicle", sProgWheeledVehicle },
};
// --- end generated read programs ---

bool GhostT2::unpackUpdate( NetObject *obj, NetConnection *con, BitStream *stream )
{
   const char *name = obj->getClassName();
   for( U32 i = 0; i < sizeof( sGhostTable ) / sizeof( sGhostTable[ 0 ] ); i++ )
   {
      if( dStrcmp( name, sGhostTable[ i ].name ) )
         continue;

      Vector< U32 >              value;
      Vector< StringTableEntry > string;
      if( Con::getBoolVariable( "$T2::traceGhosts" ) )
         Con::printf( "GHOST %s at %d", name, stream->getCurPos() );
      DataBlockT2::runProgram( stream, sGhostTable[ i ].program, value, string );
      return true;
   }

   return false;
}
