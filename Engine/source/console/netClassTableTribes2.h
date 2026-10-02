// HH: Added for Tribes 2 compatibility.
#ifndef _NETCLASSTABLETRIBES2_H_
#define _NETCLASSTABLETRIBES2_H_

#define NET_CLASS_TABLE_TRIBES2_OBJECTS( X ) \
   X( AIObjective ) \
   X( AudioEmitter ) \
   X( BeaconObject ) \
   X( BombProjectile ) \
   X( Camera ) \
   X( Debris ) \
   X( ELFProjectile ) \
   X( EnergyProjectile ) \
   X( FireballAtmosphere ) \
   X( FlareProjectile ) \
   X( FlyingVehicle ) \
   X( ForceFieldBare ) \
   X( GameBase ) \
   X( GrenadeProjectile ) \
   X( HoverVehicle ) \
   X( InteriorInstance ) \
   X( Item ) \
   X( Lightning ) \
   X( LinearFlareProjectile ) \
   X( LinearProjectile ) \
   X( Marker ) \
   X( MissionArea ) \
   X( MissionMarker ) \
   X( ParticleEmissionDummy ) \
   X( PhysicalZone ) \
   X( Player ) \
   X( Precipitation ) \
   X( Projectile ) \
   X( RepairProjectile ) \
   X( ScopeAlwaysShape ) \
   X( SeekerProjectile ) \
   X( ShapeBase ) \
   X( ShockLanceProjectile ) \
   X( Shockwave ) \
   X( SimpleNetObject ) \
   X( Sky ) \
   X( SniperProjectile ) \
   X( SpawnSphere ) \
   X( Splash ) \
   X( StaticShape ) \
   X( StationFXPersonal ) \
   X( StationFXVehicle ) \
   X( Sun ) \
   X( TSStatic ) \
   X( TargetProjectile ) \
   X( TerrainBlock ) \
   X( TracerProjectile ) \
   X( Trigger ) \
   X( Turret ) \
   X( VehicleBlocker ) \
   X( WaterBlock ) \
   X( WayPoint ) \
   X( WheeledVehicle )

#define NET_CLASS_TABLE_TRIBES2_DATABLOCKS( X ) \
   X( AudioDescription ) \
   X( AudioEnvironment ) \
   X( AudioProfile ) \
   X( AudioSampleEnvironment ) \
   X( BombProjectileData ) \
   X( CameraData ) \
   X( CannedChatItem ) \
   X( CommanderIconData ) \
   X( DebrisData ) \
   X( DecalData ) \
   X( ELFProjectileData ) \
   X( EffectProfile ) \
   X( EnergyProjectileData ) \
   X( ExplosionData ) \
   X( FireballAtmosphereData ) \
   X( FlareProjectileData ) \
   X( FlyingVehicleData ) \
   X( ForceFieldBareData ) \
   X( GameBaseData ) \
   X( GrenadeProjectileData ) \
   X( HoverVehicleData ) \
   X( ItemData ) \
   X( JetEffectData ) \
   X( LightningData ) \
   X( LinearFlareProjectileData ) \
   X( LinearProjectileData ) \
   X( MissionMarkerData ) \
   X( ParticleData ) \
   X( ParticleEmissionDummyData ) \
   X( ParticleEmitterData ) \
   X( PlayerData ) \
   X( PrecipitationData ) \
   X( ProjectileData ) \
   X( RepairProjectileData ) \
   X( RunningLightData ) \
   X( SeekerProjectileData ) \
   X( SensorData ) \
   X( ShapeBaseData ) \
   X( ShapeBaseImageData ) \
   X( ShockLanceProjectileData ) \
   X( ShockwaveData ) \
   X( SimDataBlock ) \
   X( SniperProjectileData ) \
   X( SplashData ) \
   X( StaticShapeData ) \
   X( StationFXPersonalData ) \
   X( StationFXVehicleData ) \
   X( TSShapeConstructorT2 ) \
   X( TargetProjectileData ) \
   X( TracerProjectileData ) \
   X( TriggerData ) \
   X( TurretData ) \
   X( TurretImageData ) \
   X( WheeledVehicleData )

#define NET_CLASS_TABLE_TRIBES2_EVENTS( X ) \
   X( CRCChallengeEvent ) \
   X( CRCChallengeResponseEvent ) \
   X( FogChallengeEvent ) \
   X( GhostAlwaysObjectEvent ) \
   X( GhostingMessageEvent ) \
   X( GravityEvent ) \
   X( LightningStrikeEvent ) \
   X( NetStringEvent ) \
   X( PathManagerEvent ) \
   X( RemoteCommandEvent ) \
   X( RemoveClientTargetTypeEvent ) \
   X( ResetClientTargetsEvent ) \
   X( SensorGroupColorEvent ) \
   X( SetMissionCRCEvent ) \
   X( SetObjectActiveImageEvent ) \
   X( SetSensorGroupEvent ) \
   X( SetServerTargetEvent ) \
   X( Sim2DAudioEvent ) \
   X( Sim3DAudioEvent ) \
   X( SimDataBlockEvent ) \
   X( SimTargetAudioEvent ) \
   X( SimVoiceStreamEvent ) \
   X( SimpleMessageEvent ) \
   X( TargetFreeEvent ) \
   X( TargetInfoEvent ) \
   X( TargetToEvent )

#ifndef NET_CLASS_TABLE_OBJECT_NAMES
#define NET_CLASS_TABLE_OBJECT_NAMES    NET_CLASS_TABLE_TRIBES2_OBJECTS
#endif
#ifndef NET_CLASS_TABLE_DATABLOCK_NAMES
#define NET_CLASS_TABLE_DATABLOCK_NAMES NET_CLASS_TABLE_TRIBES2_DATABLOCKS
#endif
#ifndef NET_CLASS_TABLE_EVENT_NAMES
#define NET_CLASS_TABLE_EVENT_NAMES     NET_CLASS_TABLE_TRIBES2_EVENTS
#endif

#define NET_CLASS_TABLE_OBJECT_BITS     7
#define NET_CLASS_TABLE_DATABLOCK_BITS  7
#define NET_CLASS_TABLE_EVENT_BITS      6

#define NET_CLASS_TABLE_GHOST_STATES    4

#endif
