#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netObject.h"
#include "T3D/gameBase/gameBase.h"
#include "T3D/gameBase/stubsT2.h"

#define IMPLEMENT_GHOST_T2( className )                                     \
   class className : public GameBase                                        \
   {                                                                        \
      typedef GameBase Parent;                                              \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
      bool unpackUpdate(BitStream *stream, bool isInitial)                \
      {                                                                     \
         StubT2::warn("ghost", getClassName());                             \
         return Parent::unpackUpdate(stream, isInitial);                  \
      }                                                                     \
   };                                                                       \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                    \
   ConsoleDocClass( className, "@brief A Tribes 2 object the port reads by " \
                               "its layout.\n\n@internal" )

IMPLEMENT_GHOST_T2( AIObjective );
IMPLEMENT_GHOST_T2( AudioEmitter );
IMPLEMENT_GHOST_T2( BeaconObject );
IMPLEMENT_GHOST_T2( BombProjectile );
IMPLEMENT_GHOST_T2( ELFProjectile );
IMPLEMENT_GHOST_T2( FireballAtmosphere );
IMPLEMENT_GHOST_T2( FlareProjectile );
IMPLEMENT_GHOST_T2( ParticleEmissionDummy );
IMPLEMENT_GHOST_T2( RepairProjectile );
IMPLEMENT_GHOST_T2( SeekerProjectile );
IMPLEMENT_GHOST_T2( ShockLanceProjectile );
IMPLEMENT_GHOST_T2( Shockwave );
IMPLEMENT_GHOST_T2( StationFXPersonal );
