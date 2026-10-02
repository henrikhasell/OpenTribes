#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simDatablock.h"
#include "T3D/gameBase/dataBlocksT2.h"

IMPLEMENT_CONOBJECT(DataBlockT2);

ConsoleDocClass( DataBlockT2,
   "@brief A Tribes 2 datablock the port reads by its layout.\n\n@internal" );

DataBlockT2::DataBlockT2()
{
}

IMPLEMENT_DATABLOCK_T2( TSShapeConstructorT2 );
IMPLEMENT_DATABLOCK_T2( CommanderIconData );
IMPLEMENT_DATABLOCK_T2( CannedChatItem );
IMPLEMENT_DATABLOCK_T2( SensorData );
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
IMPLEMENT_DATABLOCK_T2( JetEffectData );
IMPLEMENT_DATABLOCK_T2( LinearFlareProjectileData );
IMPLEMENT_DATABLOCK_T2( LinearProjectileData );
IMPLEMENT_DATABLOCK_T2( ParticleEmissionDummyData );
IMPLEMENT_DATABLOCK_T2( RepairProjectileData );
IMPLEMENT_DATABLOCK_T2( RunningLightData );
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
