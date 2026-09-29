// HH: Added for Tribes 2 compatibility.
#include "console/netClassTableT2.h"
#include "console/consoleObject.h"

namespace NetClassTableT2
{

const char *const Objects[] =
{
  "AIObjective", "AudioEmitter", "BeaconObject", "BombProjectile",
  "Camera", "Debris", "ELFProjectile", "EnergyProjectile",
  "FireballAtmosphere", "FlareProjectile", "FlyingVehicle",
  "ForceFieldBare", "GameBase", "GrenadeProjectile", "HoverVehicle",
  "InteriorInstance", "Item", "Lightning", "LinearFlareProjectile",
  "LinearProjectile", "Marker", "MissionArea", "MissionMarker",
  "ParticleEmissionDummy", "PhysicalZone", "Player", "Precipitation",
  "Projectile", "RepairProjectile", "ScopeAlwaysShape", "SeekerProjectile",
  "ShapeBase", "ShockLanceProjectile", "Shockwave", "SimpleNetObject",
  "Sky", "SniperProjectile", "SpawnSphere", "Splash", "StaticShape",
  "StationFXPersonal", "StationFXVehicle", "Sun", "TSStatic",
  "TargetProjectile", "TerrainBlock", "TracerProjectile", "Trigger",
  "Turret", "VehicleBlocker", "WaterBlock", "WayPoint", "WheeledVehicle"
};

const char *const DataBlocks[] =
{
  "AudioDescription", "AudioEnvironment", "AudioProfile",
  "AudioSampleEnvironment", "BombProjectileData", "CameraData",
  "CannedChatItem", "CommanderIconData", "DebrisData", "DecalData",
  "ELFProjectileData", "EffectProfile", "EnergyProjectileData",
  "ExplosionData", "FireballAtmosphereData", "FlareProjectileData",
  "FlyingVehicleData", "ForceFieldBareData", "GameBaseData",
  "GrenadeProjectileData", "HoverVehicleData", "ItemData", "JetEffectData",
  "LightningData", "LinearFlareProjectileData", "LinearProjectileData",
  "MissionMarkerData", "ParticleData", "ParticleEmissionDummyData",
  "ParticleEmitterData", "PlayerData", "PrecipitationData",
  "ProjectileData", "RepairProjectileData", "RunningLightData",
  "SeekerProjectileData", "SensorData", "ShapeBaseData",
  "ShapeBaseImageData", "ShockLanceProjectileData", "ShockwaveData",
  "SimDataBlock", "SniperProjectileData", "SplashData", "StaticShapeData",
  "StationFXPersonalData", "StationFXVehicleData",
  "TSShapeConstructor", "TargetProjectileData",
  "TracerProjectileData", "TriggerData",
  "TurretData", "TurretImageData", "WheeledVehicleData"
};

const char *const Events[] =
{
  "CRCChallengeEvent", "CRCChallengeResponseEvent", "FogChallengeEvent",
  "GhostAlwaysObjectEvent", "GhostingMessageEvent", "GravityEvent",
  "LightningStrikeEvent", "NetStringEvent", "PathManagerEvent",
  "RemoteCommandEvent", "RemoveClientTargetTypeEvent",
  "ResetClientTargetsEvent", "SensorGroupColorEvent", "SetMissionCRCEvent",
  "SetObjectActiveImageEvent", "SetSensorGroupEvent",
  "SetServerTargetEvent", "Sim2DAudioEvent", "Sim3DAudioEvent",
  "SimDataBlockEvent", "SimTargetAudioEvent", "SimVoiceStreamEvent",
  "SimpleMessageEvent", "TargetFreeEvent", "TargetInfoEvent",
  "TargetToEvent"
};

const U32 ObjectCount    = sizeof( Objects )    / sizeof( Objects[ 0 ] );
const U32 DataBlockCount = sizeof( DataBlocks ) / sizeof( DataBlocks[ 0 ] );
const U32 EventCount     = sizeof( Events )     / sizeof( Events[ 0 ] );

const U32 ObjectBits    = 7;
const U32 DataBlockBits = 7;
const U32 EventBits     = 6;

/// The datablocks whose packing has been ported.
static const char *const sPortedDataBlocks[] =
{
   "CommanderIconData", "SensorData", "CannedChatItem", "SimDataBlock",
   "GameBaseData", "ShapeBaseData", "MissionMarkerData", "AudioDescription",
   "AudioEnvironment", "AudioProfile", "AudioSampleEnvironment",
   "BombProjectileData", "ELFProjectileData", "EffectProfile",
   "EnergyProjectileData", "FireballAtmosphereData", "FlareProjectileData",
   "ForceFieldBareData", "GrenadeProjectileData", "JetEffectData",
   "LinearFlareProjectileData", "LinearProjectileData",
   "ParticleEmissionDummyData", "RepairProjectileData", "RunningLightData",
   "SeekerProjectileData", "ShockLanceProjectileData", "ShockwaveData",
   "SniperProjectileData", "StationFXPersonalData", "StationFXVehicleData",
   "TargetProjectileData", "TracerProjectileData", "TurretData",
   "TurretImageData", "CameraData", "DebrisData", "DecalData",
   "ExplosionData", "FlyingVehicleData", "HoverVehicleData", "ItemData",
   "LightningData", "ParticleData", "ParticleEmitterData", "PlayerData",
   "PrecipitationData", "ProjectileData", "ShapeBaseImageData",
   "SplashData", "StaticShapeData", "TriggerData", "WheeledVehicleData",
   "TSShapeConstructor"
};

/// The port keeps a TSShapeConstructor of its own that is no datablock, so
/// Tribes 2's carries a name of its own and is matched up here.
static const char *const sAliases[][ 2 ] =
{
   { "TSShapeConstructorT2", "TSShapeConstructor" }
};

const char *t2NameFor( const char *className )
{
   for( U32 i = 0; i < sizeof( sAliases ) / sizeof( sAliases[ 0 ] ); i++ )
      if( !dStrcmp( className, sAliases[ i ][ 0 ] ) )
         return sAliases[ i ][ 1 ];

   return className;
}

bool isPorted( U32 classType, const char *className )
{
   if( classType == NetClassTypeEvent || classType == NetClassTypeObject )
      return true;

   if( classType == NetClassTypeDataBlock )
   {
      for( U32 i = 0; i < sizeof( sPortedDataBlocks ) / sizeof( sPortedDataBlocks[ 0 ] ); i++ )
         if( !dStrcmp( className, sPortedDataBlocks[ i ] ) )
            return true;
   }

   return false;
}

const char *const *tableFor( U32 classType, U32 *countOut, U32 *bitsOut )
{
   switch( classType )
   {
      case NetClassTypeObject:
         *countOut = ObjectCount;    *bitsOut = ObjectBits;    return Objects;
      case NetClassTypeDataBlock:
         *countOut = DataBlockCount; *bitsOut = DataBlockBits; return DataBlocks;
      case NetClassTypeEvent:
         *countOut = EventCount;     *bitsOut = EventBits;     return Events;
   }
   *countOut = 0;
   *bitsOut = 0;
   return NULL;
}

}

