#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simDatablock.h"
#include "T3D/gameBase/dataBlocksT2.h"

IMPLEMENT_CONOBJECT(DataBlockT2);

// HH: Modified for Tribes 2 compatibility.
// Six classes that were stubs here now read their own fields, in
// dataBlocksMiscT2.cpp: the Tribes 2 datablocks this port has no Torque class
// of its own for.  A name must not appear in both places -- the macro below
// declares the class as well as registering it.

ConsoleDocClass( DataBlockT2,
   "@brief A Tribes 2 datablock the port reads by its layout.\n\n@internal" );

DataBlockT2::DataBlockT2()
{
}

IMPLEMENT_DATABLOCK_T2( AudioEnvironment );
IMPLEMENT_DATABLOCK_T2( AudioSampleEnvironment );
IMPLEMENT_DATABLOCK_T2( BombProjectileData );
IMPLEMENT_DATABLOCK_T2( ELFProjectileData );
IMPLEMENT_DATABLOCK_T2( EffectProfile );
IMPLEMENT_DATABLOCK_T2( EnergyProjectileData );
IMPLEMENT_DATABLOCK_T2( FireballAtmosphereData );
IMPLEMENT_DATABLOCK_T2( FlareProjectileData );
IMPLEMENT_DATABLOCK_T2( ForceFieldBareData );
IMPLEMENT_DATABLOCK_T2( GrenadeProjectileData );
IMPLEMENT_DATABLOCK_T2( LinearFlareProjectileData );
IMPLEMENT_DATABLOCK_T2( LinearProjectileData );
IMPLEMENT_DATABLOCK_T2( ParticleEmissionDummyData );
IMPLEMENT_DATABLOCK_T2( RepairProjectileData );
IMPLEMENT_DATABLOCK_T2( SeekerProjectileData );
IMPLEMENT_DATABLOCK_T2( ShockLanceProjectileData );
IMPLEMENT_DATABLOCK_T2( ShockwaveData );
IMPLEMENT_DATABLOCK_T2( SniperProjectileData );
IMPLEMENT_DATABLOCK_T2( StationFXPersonalData );
IMPLEMENT_DATABLOCK_T2( StationFXVehicleData );
IMPLEMENT_DATABLOCK_T2( TargetProjectileData );
IMPLEMENT_DATABLOCK_T2( TracerProjectileData );
IMPLEMENT_DATABLOCK_T2( TurretData );
IMPLEMENT_DATABLOCK_T2( TurretImageData );
