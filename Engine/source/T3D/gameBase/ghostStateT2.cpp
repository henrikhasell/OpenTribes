// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/netClassTableT2.h"
#include "console/simBase.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "environment/sky.h"
#include "environment/sun.h"
#include "math/mMathFn.h"
#include "terrain/terrData.h"
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

/// TerrainBlock, from the recovered layout's node numbers.
///
///   2 terrainFile   3 detailTexture   4 squareSize
///
/// Checked against the mission the server was running: Pyroclasm.ter,
/// details/lavadet1 and 8, which is what Pyroclasm.mis carries.  The empty
/// square list after them is read and dropped.
struct TerrainState
{
   char file[ 256 ];
   char detail[ 256 ];
   U32  squareSize;
};

void terrainField( void *ctx, const T2Layout::Field &f )
{
   TerrainState &s = *static_cast< TerrainState * >( ctx );

   switch( f.node )
   {
      case 2:
         dStrncpy( s.file, f.str ? f.str : "", sizeof( s.file ) - 1 );
         s.file[ sizeof( s.file ) - 1 ] = 0;
         break;

      case 3:
         dStrncpy( s.detail, f.str ? f.str : "", sizeof( s.detail ) - 1 );
         s.detail[ sizeof( s.detail ) - 1 ] = 0;
         break;

      case 4: s.squareSize = f.value; break;

      default: break;
   }
}

/// Sun, from the recovered layout's node numbers.
///
///   3/4/5 direction   6/7/8/9 color   10/11/12/13 ambient
///
/// Checked against the mission the server was running: direction
/// 0.57735 0.57735 -0.57735, colour 0.8 and ambient 0.6, every component.
struct SunState
{
   F32 direction[ 3 ];
   F32 color[ 4 ];
   F32 ambient[ 4 ];
};

void sunField( void *ctx, const T2Layout::Field &f )
{
   SunState &s = *static_cast< SunState * >( ctx );

   if( f.node >= 3 && f.node <= 5 )
      s.direction[ f.node - 3 ] = f.asF32;
   else if( f.node >= 6 && f.node <= 9 )
      s.color[ f.node - 6 ] = f.asF32;
   else if( f.node >= 10 && f.node <= 13 )
      s.ambient[ f.node - 10 ] = f.asF32;
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

void applySun( Sun *sun, const SunState &s )
{
   // Tribes 2 names a direction where this engine's Sun names an azimuth and
   // an elevation.  Inverted off _conformLights, which builds the direction
   // back out of them as -getVectorFromAngles(azimuth, elevation), so the two
   // agree by construction rather than by the wording of either field.
   const F32 lz = mClampF( s.direction[ 2 ], -1.0f, 1.0f );
   const F32 elevation = mRadToDeg( mAsin( lz ) );
   F32 azimuth = mRadToDeg( mAtan2( s.direction[ 0 ], -s.direction[ 1 ] ) );
   while( azimuth < 0.0f )
      azimuth += 360.0f;

   setF32( sun, "azimuth", NULL, azimuth );
   setF32( sun, "elevation", NULL, elevation );

   char buf[ 96 ];
   dSprintf( buf, sizeof( buf ), "%g %g %g %g", s.color[ 0 ], s.color[ 1 ],
             s.color[ 2 ], s.color[ 3 ] );
   setField( sun, "color", NULL, buf );

   dSprintf( buf, sizeof( buf ), "%g %g %g %g", s.ambient[ 0 ], s.ambient[ 1 ],
             s.ambient[ 2 ], s.ambient[ 3 ] );
   setField( sun, "ambient", NULL, buf );
}

void applyTerrain( TerrainBlock *terrain, const TerrainState &s )
{
   // Tribes 2 names the file alone and keeps its terrains in one directory,
   // the way it keeps its textures in one.
   String path = s.file;
   if( path.isNotEmpty() && !path.startsWith( "terrains/" ) )
      path = String::ToString( "terrains/%s", s.file );

   setField( terrain, "terrainFile", NULL, path );
   setField( terrain, "detailTexture", NULL, s.detail );
   setF32( terrain, "squareSize", NULL, F32( s.squareSize ) );

   // The ghost carries no transform.  Every Tribes 2 mission centres its
   // terrain on the origin, which is where a square size and the block's
   // fixed 256 squares put it.
   const F32 half = 256.0f * F32( s.squareSize ) * 0.5f;
   char buf[ 64 ];
   dSprintf( buf, sizeof( buf ), "%g %g 0", -half, -half );
   setField( terrain, "position", NULL, buf );
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

   if( Sun *sun = dynamic_cast< Sun * >( obj ) )
   {
      SunState state;
      dMemset( &state, 0, sizeof( state ) );

      if( !T2Layout::readGhost( stream, className, isInitial, sunField,
                                &state ) )
         return false;

      applySun( sun, state );
      sun->setBuiltFromLayoutT2();
      return true;
   }

   if( TerrainBlock *terrain = dynamic_cast< TerrainBlock * >( obj ) )
   {
      TerrainState state;
      dMemset( &state, 0, sizeof( state ) );

      if( !T2Layout::readGhost( stream, className, isInitial, terrainField,
                                &state ) )
         return false;

      applyTerrain( terrain, state );
      terrain->setBuiltFromLayoutT2();
      return true;
   }

   return T2Layout::readGhost( stream, className, isInitial );
}

}
