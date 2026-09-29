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
  "StationFXPersonalData", "StationFXVehicleData", "TSShapeConstructor",
  "TargetProjectileData", "TracerProjectileData", "TriggerData",
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

