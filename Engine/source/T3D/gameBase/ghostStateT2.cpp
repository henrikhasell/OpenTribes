// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/netClassTableT2.h"
#include "console/simBase.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "environment/sky.h"
#include "sim/netObject.h"
#include "T3D/gameBase/ghostStateT2.h"
#include "T3D/gameBase/t2Layout.h"

/// What the port keeps out of a Tribes 2 ghost body.
///
/// A node is named here only where a session says what it is.  Every field
/// below was read off a running Classic server and checked against the same
/// field on that server's own console, so the mapping is measured rather than
/// inferred from the mission file -- the mod's scripts overwrite the sky the
/// mission ships with, and the wire carries what the server holds.
///
/// The nodes left out are left walked and discarded, which is what the whole
/// body was before.

namespace
{

/// Sky, from the recovered layout's node numbers.
///
///   61 materialList   60/59/58 fogColor   57 fog volume count
///   54/53/52 SkySolidColor     41/38/35 cloudHeightPer
///   40/37/34 cloudSpeed        33/32/31 windVelocity
///   22 useSkyTextures          18 visibleDistance   17 fogDistance
///
/// The six floats a fog volume carries are read but not kept: the server this
/// was measured against sends none, so which of them is which is untested.
struct SkyState
{
   char name[ 256 ];
   F32  fogColor[ 3 ];
   F32  solidColor[ 3 ];
   F32  cloudHeight[ 3 ];
   F32  cloudSpeed[ 3 ];
   F32  wind[ 3 ];
   F32  visibleDistance;
   F32  fogDistance;
   bool skyTextures;
};

void skyField( void *ctx, const T2Layout::Field &f )
{
   SkyState &s = *static_cast< SkyState * >( ctx );

   switch( f.node )
   {
      case 61:
         dStrncpy( s.name, f.str ? f.str : "", sizeof( s.name ) - 1 );
         s.name[ sizeof( s.name ) - 1 ] = 0;
         break;

      case 60: s.fogColor[ 0 ] = f.asF32; break;
      case 59: s.fogColor[ 1 ] = f.asF32; break;
      case 58: s.fogColor[ 2 ] = f.asF32; break;

      case 54: s.solidColor[ 0 ] = f.asF32; break;
      case 53: s.solidColor[ 1 ] = f.asF32; break;
      case 52: s.solidColor[ 2 ] = f.asF32; break;

      case 41: s.cloudHeight[ 0 ] = f.asF32; break;
      case 38: s.cloudHeight[ 1 ] = f.asF32; break;
      case 35: s.cloudHeight[ 2 ] = f.asF32; break;

      case 40: s.cloudSpeed[ 0 ] = f.asF32; break;
      case 37: s.cloudSpeed[ 1 ] = f.asF32; break;
      case 34: s.cloudSpeed[ 2 ] = f.asF32; break;

      case 33: s.wind[ 0 ] = f.asF32; break;
      case 32: s.wind[ 1 ] = f.asF32; break;
      case 31: s.wind[ 2 ] = f.asF32; break;

      case 22: s.skyTextures = f.value != 0; break;
      case 18: s.visibleDistance = f.asF32; break;
      case 17: s.fogDistance = f.asF32; break;

      default: break;
   }
}

/// The names are the ones the mission file uses, because they are the names
/// the object publishes.
void setField( SimObject *obj, const char *field, const char *array,
               const char *value )
{
   obj->setDataField( StringTable->insert( field ), array, value );
}

void setF32( SimObject *obj, const char *field, const char *array, F32 value )
{
   char buf[ 32 ];
   dSprintf( buf, sizeof( buf ), "%g", value );
   setField( obj, field, array, buf );
}

void setTriple( SimObject *obj, const char *field, const F32 *v,
                const char *tail )
{
   char buf[ 96 ];
   dSprintf( buf, sizeof( buf ), "%g %g %g%s", v[ 0 ], v[ 1 ], v[ 2 ], tail );
   setField( obj, field, NULL, buf );
}

void applySky( Sky *sky, const SkyState &s )
{
   setField( sky, "materialList", NULL, s.name );
   setTriple( sky, "fogColor", s.fogColor, " 1" );
   setTriple( sky, "SkySolidColor", s.solidColor, " 1" );
   setTriple( sky, "windVelocity", s.wind, "" );

   for( U32 i = 0; i < 3; i++ )
   {
      char index[ 8 ];
      dSprintf( index, sizeof( index ), "%d", i );
      setF32( sky, "cloudHeightPer", index, s.cloudHeight[ i ] );
   }

   setF32( sky, "cloudSpeed1", NULL, s.cloudSpeed[ 0 ] );
   setF32( sky, "cloudSpeed2", NULL, s.cloudSpeed[ 1 ] );
   setF32( sky, "cloudSpeed3", NULL, s.cloudSpeed[ 2 ] );

   setF32( sky, "visibleDistance", NULL, s.visibleDistance );
   setF32( sky, "fogDistance", NULL, s.fogDistance );
   setField( sky, "useSkyTextures", NULL, s.skyTextures ? "1" : "0" );
}

} // namespace

namespace GhostStateT2
{

bool readGhost( NetObject *obj, BitStream *stream, bool isInitial )
{
   const char *className = NetClassTableT2::t2NameFor( obj->getClassName() );

   if( Sky *sky = dynamic_cast< Sky * >( obj ) )
   {
      SkyState state;
      dMemset( &state, 0, sizeof( state ) );

      if( !T2Layout::readGhost( stream, className, isInitial, skyField,
                                &state ) )
         return false;

      applySky( sky, state );
      sky->setBuiltFromLayoutT2();
      return true;
   }

   return T2Layout::readGhost( stream, className, isInitial );
}

}
