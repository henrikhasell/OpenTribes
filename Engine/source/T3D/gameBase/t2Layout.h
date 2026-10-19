// HH: Added for Tribes 2 compatibility.
#ifndef _T2LAYOUT_H_
#define _T2LAYOUT_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

class BitStream;

namespace T2Layout
{
   /// Eight image slots, of which item.cs fills four: weapon, auxiliary,
   /// backpack and flag.
   enum { MaxImages = 8 };

   enum { MaxThreads = 4 };

   /// ShapeBase's playAudio slots.
   enum { MaxSounds = 4 };

   struct Body
   {
      bool    hasDataBlock;         ///< the object's own datablock came with
      U32     dataBlockId;          ///< the body, as GameBase sends it
      bool    hasPosition;
      Point3F position;
      bool    hasYaw;
      F32     yaw;                  ///< radians
      bool    hasRotation;
      MatrixF rotation;             ///< the whole orientation, where the body
      bool    hasScale;
      Point3F scale;
      bool    hasVelocity;
      Point3F velocity;             ///< metres a second, world space
      bool    hasDirection;
      Point3F direction;
      bool    hasImpactNormal;
      Point3F impactNormal;
      bool    hasDamage;
      F32     damage;               ///< share of the object's maxDamage
      bool    hasDamageState;
      U32     damageState;
      bool    blowApart;
      bool    hasDamageDir;
      Point3F damageDir;
      bool    hasShield;            ///< the shield turned a hit away
      Point3F shieldNormal;
      F32     shieldEnergy;         ///< share of maxEnergy left, 0 to 1
      bool    hasExplodePoint;
      Point3F explodePoint;
      bool    hasCurrTick;
      U32     currTick;
      bool    hasSourceObject;
      U32     sourceObject;
      U32     sourceSlot;
      bool    hasVehicleObject;
      U32     vehicleObject;
      bool    hasTarget;            ///< the body changed the target
      S32     target;               ///< the target manager's id, or -1
      bool    quickSplash;
      bool    hasJetting;
      bool    jetting;              ///< the jet pack is lit
      bool    hasRotate;
      bool    rotate;               ///< the body spins about its own Z axis
      bool    hasStatic;
      bool    isStatic;             ///< it never moves, so it never simulates
      bool    hasAtRest;
      bool    atRest;               ///< it has stopped where the server left
                                    ///< it, and carries no velocity
      bool    hasWarp;
      bool    warp;                 ///< interpolate to the position rather
                                    ///< than snap to it
      bool    hasAction;
      U32     action;               ///< the shape's sequence number
      bool    actionHold;
      bool    actionAtEnd;
      bool    actionFirstPerson;
      bool    hasActionPos;
      F32     actionPos;            ///< how far through the thread, -1..1
      bool    hasInvincible;
      F32     invincibleTime;
      F32     invincibleSpeed;
      bool    hasFieldState;
      U32     fieldState;
      U32     fieldFade;
      struct Run
      {
         S32  at;
         U32  count;
      };
      Run     zonePoints;
      Run     zonePlanes;
      Run     zoneEdges;
      F32     velocityMod;
      F32     gravityMod;
      bool    hasAppliedForce;
      Point3F appliedForce;
      bool    hasZoneActive;
      bool    zoneActive;
      bool    hasCloaked;
      bool    cloaked;
      bool    hasFade;
      bool    fading;
      bool    fadeVisible;
      bool    fadeOut;
      F32     fadeTime;
      bool    hasMount;             ///< the body said what the object sits
                                    ///< on, which may be nothing
      S32     mountGhost;           ///< the mount's ghost index, or -1 to
                                    ///< get off
      U32     mountNode;            ///< the mount point on the mount

      /// What a body said about one of its object's image slots -- the
      /// weapon in the hand, the pack on the back.
      struct Image
      {
         bool   present;            ///< the body carried this slot at all;
                                    ///< the rest means nothing without it
         bool   hasDataBlock;       ///< clear is an emptied slot, not an
                                    ///< unchanged one
         U32    dataBlockId;
         bool   hasSkinName;
         String skinName;
         bool   hasSkinTag;
         U32    skinTag;            ///< a net string id, which needs the
                                    ///< connection's table to mean anything
         bool   triggerDown;
         bool   loaded;
         bool   ammo;
         bool   wet;
         bool   target;
         U32    fireCount;
         bool   fireOnInitial;      ///< an initial body's extra flag: the
                                    ///< slot was firing when the ghost was sent
      };
      Image   images[MaxImages];

      struct Thread
      {
         bool   present;
         U32    sequence;
         U32    state;
         bool   forward;
         bool   atEnd;
      };
      Thread  threads[MaxThreads];

      /// A sound slot the server started or stopped on the object.
      struct Sound
      {
         bool   present;            ///< the body changed this slot
         bool   play;
         S32    profile;            ///< an AudioProfile's id, or -1
      };
      Sound   sounds[MaxSounds];
   };

   const Body &lastBody();

   enum T2DataBlockField
   {
      T2DbJetForce,
      T2DbUnderwaterJetForce,
      T2DbUnderwaterVertJetFactor,
      T2DbJetEnergyDrain,
      T2DbUnderwaterJetEnergyDrain,
      T2DbMinJetEnergy,
      T2DbMaxJetForwardSpeed,
      T2DbMaxJetHorizontalPercentage,
      T2DbJetEmitter,
      T2DbHeatDecayPerSec,
      T2DbHeatIncreasePerSec,
      T2DbCameraMaxDist,
      T2DbImageMountPoint,
      T2DbImageCloakable,
      T2DbProjectileVelocity,
      T2DbProjectileLifetime,
      T2DbProjectileSound,
      T2DbTracerLength,
      T2DbTracerWidth,
      T2DbCrossSize,
      T2DbCrossViewAng,
      T2DbElasticity,
      T2DbFriction,
      T2DbGravityMod,
      T2DbProjectileDrag,
      T2DbProjectileDensity,
      T2DbArmingDelay,
      T2DbBlurLifetime,
      T2DbBlurWidth,
      T2DbBlurColorR,
      T2DbBlurColorG,
      T2DbBlurColorB,
      T2DbScaleX,
      T2DbScaleY,
      T2DbScaleZ,
      T2DbExplosion,
      T2DbFaceViewer,
      T2DbExplosionScaleX,
      T2DbExplosionScaleY,
      T2DbExplosionScaleZ,
      T2DbPlaySpeed,
      T2DbDelayMS,
      T2DbDelayVariance,
      T2DbExplosionLifetime,
      T2DbLifetimeVariance,
      T2DbExplosionOffset,
      T2DbExplosionTime,
      T2DbExplosionSize,
      T2DbExplosionSound,
      T2DbExplosionParticleEmitter,
      T2DbExplosionParticleDensity,
      T2DbExplosionParticleRadius,
      T2DbExplosionDebrisThetaMin,
      T2DbExplosionDebrisThetaMax,
      T2DbExplosionDebrisPhiMin,
      T2DbExplosionDebrisPhiMax,
      T2DbExplosionDebrisNum,
      T2DbExplosionDebrisNumVariance,
      T2DbExplosionDebrisVelocity,
      T2DbExplosionDebrisVelocityVariance,
      T2DbExplosionShakeCamera,
      T2DbExplosionShakeFreqX,
      T2DbExplosionShakeFreqY,
      T2DbExplosionShakeFreqZ,
      T2DbExplosionShakeAmpX,
      T2DbExplosionShakeAmpY,
      T2DbExplosionShakeAmpZ,
      T2DbExplosionShakeDuration,
      T2DbExplosionShakeRadius,
      T2DbExplosionShakeFalloff,
      T2DbExplosionDebris,
      T2DbExplosionEmitter0,
      T2DbExplosionEmitter1,
      T2DbExplosionEmitter2,
      T2DbExplosionEmitter3,
      T2DbSubExplosion0,
      T2DbSubExplosion1,
      T2DbSubExplosion2,
      T2DbSubExplosion3,
      T2DbSubExplosion4,

      T2DbDebrisElasticity,
      T2DbDebrisFriction,
      T2DbDebrisNumBounces,
      T2DbDebrisBounceVariance,
      T2DbDebrisMinSpinSpeed,
      T2DbDebrisMaxSpinSpeed,
      T2DbDebrisExplodeOnMaxBounce,
      T2DbDebrisStaticOnMaxBounce,
      T2DbDebrisSnapOnMaxBounce,
      T2DbDebrisLifetime,
      T2DbDebrisLifetimeVariance,
      T2DbDebrisVelocity,
      T2DbDebrisVelocityVariance,
      T2DbDebrisFade,
      T2DbDebrisUseRadiusMass,
      T2DbDebrisBaseRadius,
      T2DbDebrisGravModifier,
      T2DbDebrisTerminalVelocity,
      T2DbDebrisIgnoreWater,
      T2DbDebrisEmitter0,
      T2DbDebrisEmitter1,
      T2DbDebrisExplosion,

      T2DbAudioDescription,
      T2DbAudioVolume,
      T2DbAudioLooping,
      T2DbAudio3D,
      T2DbAudioChannel,
      T2DbAudioReferenceDistance,
      T2DbAudioMaxDistance,
      T2DbAudioConeInsideAngle,
      T2DbAudioConeOutsideAngle,
      T2DbAudioConeOutsideVolume,

      T2DbMass,
      T2DbDrag,
      T2DbDensity,
      T2DbMaxEnergy,

      T2DbShapeExplosion,
      T2DbShapeUnderwaterExplosion,
      T2DbShapeDebris,
      T2DbShieldEffectLifetimeMS,
      T2DbShieldEffectScaleX,
      T2DbShieldEffectScaleY,
      T2DbShieldEffectScaleZ,

      T2DbComputeCRC,
      T2DbShapeCRC,
      T2DbShapeFile,
      T2DbCameraMinDist,
      T2DbCameraDefaultFov,
      T2DbCameraMinFov,
      T2DbCameraMaxFov,
      T2DbDebrisShapeName,
      T2DbCmdCategory,
      T2DbCmdMiniIconName,
      T2DbCmdIcon,
      T2DbObserveThroughObject,
      T2DbIsInvincible,
      T2DbRenderWhenDestroyed,
      T2DbInheritEnergyFromMount,
      T2DbFirstPersonOnly,
      T2DbUseEyePoint,

      T2DbItemFriction,
      T2DbItemElasticity,
      T2DbItemSticky,
      T2DbItemGravityMod,
      T2DbItemMaxVelocity,
      T2DbItemLightType,
      T2DbItemLightColorR,
      T2DbItemLightColorG,
      T2DbItemLightColorB,
      T2DbItemLightColorA,
      T2DbItemLightTime,
      T2DbItemLightRadius,
      T2DbItemLightOnlyStatic,

      T2DbMinLookAngle,
      T2DbMaxLookAngle,
      T2DbMaxFreelookAngle,
      T2DbMaxTimeScale,
      T2DbMaxStepHeight,
      T2DbRunForce,
      T2DbRunEnergyDrain,
      T2DbMinRunEnergy,
      T2DbMaxForwardSpeed,
      T2DbMaxBackwardSpeed,
      T2DbMaxSideSpeed,
      T2DbMaxUnderwaterForwardSpeed,
      T2DbMaxUnderwaterBackwardSpeed,
      T2DbMaxUnderwaterSideSpeed,
      T2DbRunSurfaceAngle,
      T2DbRecoverDelay,
      T2DbRecoverRunForceScale,
      T2DbJumpForce,
      T2DbJumpEnergyDrain,
      T2DbMinJumpEnergy,
      T2DbMinJumpSpeed,
      T2DbMaxJumpSpeed,
      T2DbJumpSurfaceAngle,
      T2DbJumpDelay,
      T2DbHorizMaxSpeed,
      T2DbHorizResistSpeed,
      T2DbHorizResistFactor,
      T2DbUpMaxSpeed,
      T2DbUpResistSpeed,
      T2DbUpResistFactor,
      T2DbSplashVelocity,
      T2DbSplashAngle,
      T2DbSplashFreqMod,
      T2DbSplashVelEpsilon,
      T2DbBubbleEmitTime,
      T2DbMedSplashSoundVel,
      T2DbHardSplashSoundVel,
      T2DbExitSplashSoundVel,
      T2DbFootSplashHeight,
      T2DbMinImpactSpeed,
      T2DbPlayerSound,
      T2DbPlayerSoundLast = T2DbPlayerSound + 31,
      T2DbBoxSizeX,
      T2DbBoxSizeY,
      T2DbBoxSizeZ,
      T2DbFootPuffEmitter,
      T2DbFootPuffNumParts,
      T2DbFootPuffRadius,
      T2DbDecalOffset,
      T2DbDustEmitter,
      T2DbGroundImpactMinSpeed,
      T2DbGroundImpactShakeFreqX,
      T2DbGroundImpactShakeFreqY,
      T2DbGroundImpactShakeFreqZ,
      T2DbGroundImpactShakeAmpX,
      T2DbGroundImpactShakeAmpY,
      T2DbGroundImpactShakeAmpZ,
      T2DbGroundImpactShakeDuration,
      T2DbGroundImpactShakeFalloff,

      T2DbEjectionPeriodMS,
      T2DbPeriodVarianceMS,
      T2DbEjectionVelocity,
      T2DbVelocityVariance,
      T2DbEjectionOffset,
      T2DbThetaMin,
      T2DbThetaMax,
      T2DbPhiReferenceVel,
      T2DbPhiVariance,
      T2DbOverrideAdvance,
      T2DbOrientParticles,
      T2DbOrientOnVelocity,
      T2DbEmitterLifetimeMS,
      T2DbEmitterLifetimeVarianceMS,
      T2DbUseEmitterSizes,
      T2DbUseEmitterColors,

      T2DbDragCoefficient,
      T2DbWindCoefficient,
      T2DbGravityCoefficient,
      T2DbInheritedVelFactor,
      T2DbConstantAcceleration,
      T2DbParticleLifetimeMS,
      T2DbParticleLifetimeVarianceMS,
      T2DbSpinSpeed,
      T2DbSpinRandomMin,
      T2DbSpinRandomMax,
      T2DbUseInvAlpha,
      T2DbParticleColorR,
      T2DbParticleColorG,
      T2DbParticleColorB,
      T2DbParticleColorA,
      T2DbParticleSize,
      T2DbParticleTime,

      T2DbFieldFadeMS,
      T2DbFieldBaseTranslucency,
      T2DbFieldPowerOffTranslucency,
      T2DbFieldTeamPermiable,
      T2DbFieldOtherPermiable,
      T2DbFieldColor,
      T2DbFieldPowerOffColor,
      T2DbFieldFramesPerSec,
      T2DbFieldNumFrames,
      T2DbFieldScrollSpeed,
      T2DbFieldUMapping,
      T2DbFieldVMapping,

      T2DbFieldCount
   };

   inline F32 t2Float(U32 word, U32 bits)
   {
      const U32 mask = bits >= 32 ? 0xffffffffu : (1u << bits) - 1;
      return mask ? F32(word & mask) / F32(mask) : 0.0f;
   }

   inline F32 t2SignedFloat(U32 word, U32 bits)
   {
      return t2Float(word, bits) * 2.0f - 1.0f;
   }

   /// How many states a Tribes 2 image datablock can carry, which is what a
   /// way out of one is wide enough to name.
   enum { MaxImageStates = 32 };

      /// No sequence name is on the wire: retail's client finds one by looking the
      /// state's own name up in the shape.
   struct ImageState
   {
      /// The ways out of a state, in the order the body carries them, which
      /// is Torque's own: each pair is the false arm and then the true one.
      enum
      {
         NotLoaded, Loaded, NoAmmo, Ammo, NoTarget, Target, NotWet, Wet,
         TriggerUp, TriggerDown, Timeout,

         TransitionCount
      };

      String name;
      S32    transition[TransitionCount]; ///< a state, or -1 for no way out
      bool   hasTimeout;
      F32    timeout;
      bool   waitForTimeout;
      bool   fire;
         /// Of the 211 states of the game's 45 images only the chaingun's Fire sets
         /// this, and retail has it ejecting a shell and carrying a flash alike.
      bool   ejectShell;
      bool   direction;          ///< play the sequence forwards
      U32    loaded;             ///< ShapeBaseImageData::StateData::LoadedState
      U32    spin;               ///< ...::SpinState
      U32    recoil;             ///< ...::RecoilState
      S32    sound;              ///< an AudioProfile, or -1 for a silent state
   };

   /// The states the last datablock read, in the order it read them.
   const Vector<ImageState> &lastImageStates();

   bool lastDataBlockField(U32 field, F32 *out);

   /// The same read as the bits it arrived as, for a field Tribes 2 writes
   /// as a whole word rather than a float.
   bool lastDataBlockWord(U32 field, U32 *out);

   const Vector<U32> &lastDataBlockValues(U32 field);

   /// A field the last datablock sent as a string, or NULL where it sent none.
   const char *lastDataBlockString(U32 field);

   /// What a body leaves out because retail reads it off the object and the
   /// connection rather than the stream. A walk without one takes the object
   /// for nobody's control object, with no wheels.
   struct Subject
   {
      Subject() : isControlObject(false), dataBlockId(0), wheelCount(NULL) {}

      /// The connection's control object is this object.
      bool isControlObject;
      /// The datablock the object already has, for a body that names none.
      U32  dataBlockId;
      /// The wheels a datablock has. The walk asks with the id the body named
      /// where it named one, because a first body names it before the wheels.
      U32 (*wheelCount)(U32 dataBlockId);
   };

   bool readDataBlock(BitStream *stream, U32 classId);

   bool readGhost(BitStream *stream, const char *className,
                  bool isInitial = false, const Subject *subject = NULL);
   bool readEvent(BitStream *stream, const char *className);

   /// The class of the object in a ghost slot, and its subject, for a layout
   /// that goes on to read that object's packet data.
   typedef const char *(*GhostClassFn)(void *ctx, U32 ghostIndex,
                                       Subject *subject);

   bool readPacketData(BitStream *stream, const char *className,
                       GhostClassFn resolve = NULL, void *ctx = NULL,
                       const Subject *subject = NULL);

   bool writeDataBlock(BitStream *stream, U32 classId);

   bool writeGhost(BitStream *stream, const char *className,
                   bool isInitial = false, const Subject *subject = NULL);
   bool writeEvent(BitStream *stream, const char *className);

   bool writePacketData(BitStream *stream, const char *className,
                        GhostClassFn resolve = NULL, void *ctx = NULL,
                        const Subject *subject = NULL);

   const char *lastShapeFile();

   /// Every datablock id the last body named, in the order it named them.
   const Vector<U32> &lastDataBlockIds();

   const char *firstString();

   bool firstWord32(U32 *out);

   const Vector<String> &lastStrings();

   /// Every 32-bit word the last ghost body read, in the order it read them.
   const Vector<U32> &lastWords();

   /// A Tribes 2 resource name is a bare file name; Torque wants a path.
   String resourcePath(const char *name, const char *dir);
}

#endif
