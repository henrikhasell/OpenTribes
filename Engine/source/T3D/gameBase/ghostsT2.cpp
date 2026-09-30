// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netObject.h"

/// The objects Tribes 2 ghosts that the port has never heard of.
///
/// A ghost's update is read from the class's recovered layout (see
/// t2Layout.h), so these carry no reading of their own -- they exist so the
/// net class table has something to seat at their id and the ghost stream has
/// something to hang an index on.

#define IMPLEMENT_GHOST_T2( className )                                     \
   class className : public NetObject                                       \
   {                                                                        \
      typedef NetObject Parent;                                             \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
   };                                                                       \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                    \
   ConsoleDocClass( className, "@brief A Tribes 2 object the port reads by " \
                               "its layout.\n\n@internal" )

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
