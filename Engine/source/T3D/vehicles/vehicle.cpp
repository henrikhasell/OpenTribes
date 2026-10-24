//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/vehicles/vehicle.h"

#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "collision/clippedPolyList.h"
#include "collision/planeExtractor.h"
#include "core/stream/bitStream.h"
#include "core/dnet.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/fx/cameraFXMgr.h"
#include "ts/tsShapeInstance.h"
#include "T3D/fx/particleEmitter.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxProfile.h"
#include "sfx/sfxSource.h"
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "T3D/trigger.h"
#include "T3D/item.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxDrawUtil.h"
#include "materials/materialDefinition.h"
#include "T3D/gameBase/t2Layout.h"   // HH: Added for Tribes 2 compatibility.


namespace {

const U32 sMoveRetryCount = 3;

// Client prediction
const S32 sMaxWarpTicks = 3;           // Max warp duration in ticks
const S32 sMaxPredictionTicks = 30;    // Number of ticks to predict
const F32 sVehicleGravity = -20;

// Physics and collision constants
static F32 sRestTol = 0.5;             // % of gravity energy to be at rest
static int sRestCount = 10;            // Consecutive ticks before comming to rest

} // namespace {}

// Trigger objects that are not normally collided with.
static U32 sTriggerMask = ItemObjectType     |
                          TriggerObjectType  |
                          CorpseObjectType;

IMPLEMENT_CONOBJECT(VehicleData);

ConsoleDocClass( VehicleData,
   "@brief Base properties shared by all Vehicles (FlyingVehicle, HoverVehicle, "
   "WheeledVehicle).\n\n"
   "This datablock defines properties shared by all Vehicle types, but should "
   "not be instantiated directly. Instead, set the desired properties in the "
   "FlyingVehicleData, HoverVehicleData or WheeledVehicleData datablock.\n"

   "@section VehicleData_damage Damage\n\n"

   "The VehicleData class extends the basic energy/damage functionality provided "
   "by ShapeBaseData to include damage from collisions, as well as particle "
   "emitters activated automatically when damage levels reach user specified "
   "thresholds.\n\n"

   "The example below shows how to setup a Vehicle to:\n"
   "<ul>\n"
   "  <li>take damage when colliding with another object\n"
   "  <li>emit gray smoke particles from two locations on the Vehicle when damaged above 50%</li>\n"
   "  <li>emit black smoke particles from two locations on the Vehicle when damaged above 85%</li>\n"
   "  <li>emit bubbles when any active damage emitter point is underwater</li>\n"
   "</ul>\n\n"

   "@tsexample\n"
   "// damage from collisions\n"
   "collDamageMultiplier = 0.05;\n"
   "collDamageThresholdVel = 15;\n\n"

   "// damage levels\n"
   "damageLevelTolerance[0] = 0.5;\n"
   "damageEmitter[0] = GraySmokeEmitter;     // emitter used when damage is >= 50%\n"
   "damageLevelTolerance[1] = 0.85;\n"
   "damageEmitter[1] = BlackSmokeEmitter;    // emitter used when damage is >= 85%\n"
   "damageEmitter[2] = DamageBubbleEmitter;  // emitter used instead of damageEmitter[0:1]\n"
   "                                         // when offset point is underwater\n"
   "// emit offsets (used for all active damage level emitters)\n"
   "damageEmitterOffset[0] = \"0.5 3 1\";\n"
   "damageEmitterOffset[1] = \"-0.5 3 1\";\n"
   "numDmgEmitterAreas = 2;\n"
   "@endtsexample\n"

   "@ingroup Vehicles\n"
);

IMPLEMENT_CALLBACK( VehicleData, onEnterLiquid, void, ( Vehicle* obj, F32 coverage, const char* type ), ( obj, coverage, type ),
   "Called when the vehicle enters liquid.\n"
   "@param obj the Vehicle object\n"
   "@param coverage percentage of the vehicle's bounding box covered by the liquid\n"
   "@param type type of liquid the vehicle has entered\n" );

IMPLEMENT_CALLBACK( VehicleData, onLeaveLiquid, void, ( Vehicle* obj, const char* type ), ( obj, type ),
   "Called when the vehicle leaves liquid.\n"
   "@param obj the Vehicle object\n"
   "@param type type of liquid the vehicle has left\n" );

//----------------------------------------------------------------------------

VehicleData::VehicleData()
{
   shadowEnable = true;
   shadowSize = 256;
   shadowProjectionDistance = 14.0f;


   body.friction = 0;
   body.restitution = 1;

   minImpactSpeed = 25;
   softImpactSpeed = 25;
   hardImpactSpeed = 50;
   minRollSpeed = 0;
   // HH: Modified for Tribes 2 compatibility.
   // Retail's constructor (0x608a50) stores 0.785 itself, not a quarter of
   // pi.
   maxSteeringAngle = 0.785f;

   cameraRoll = true;
   cameraLag = 0;
   cameraDecay = 0;
   cameraOffset = 0;

   minDrag = 0;
   maxDrag = 0;
   integration = 1;
   collisionTol = 0.1f;
   contactTol = 0.1f;
   massCenter.set(0,0,0);
   massBox.set(0,0,0);

   drag = 0.7f;
   density = 4;

   jetForce = 500;
   jetEnergyDrain =  0.8f;
   minJetEnergy = 1;

   // HH: Added for Tribes 2 compatibility.
   stuckTimerTicks = 1;
   stuckTimerAngle = 180;

   // HH: Modified for Tribes 2 compatibility.
   // Retail's none is -1, as id 0 is a datablock here.
   for (S32 i = 0; i < Body::MaxSounds; i++)
   {
      body.sound[i] = 0;
      bodySoundID[i] = -1;
   }

   dustEmitter = NULL;
   dustID = -1;
   triggerDustHeight = 3.0;
   dustHeight = 1.0;

   dMemset( damageEmitterList, 0, sizeof( damageEmitterList ) );
   dMemset( damageEmitterOffset, 0, sizeof( damageEmitterOffset ) );
   dMemset( damageEmitterIDList, -1, sizeof( damageEmitterIDList ) );
   dMemset( damageLevelTolerance, 0, sizeof( damageLevelTolerance ) );
   dMemset( splashEmitterList, 0, sizeof( splashEmitterList ) );
   dMemset( splashEmitterIDList, -1, sizeof( splashEmitterIDList ) );

   numDmgEmitterAreas = 0;

   splashFreqMod = 300.0;
   splashVelEpsilon = 0.50;
   exitSplashSoundVel = 2.0;
   softSplashSoundVel = 1.0;
   medSplashSoundVel = 2.0;
   hardSplashSoundVel = 3.0;

   dMemset(waterSound, 0, sizeof(waterSound));
   dMemset(waterSoundID, -1, sizeof(waterSoundID));   // HH: Added for Tribes 2 compatibility.

   collDamageThresholdVel = 20;
   collDamageMultiplier   = 0.05f;
}


//----------------------------------------------------------------------------

bool VehicleData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;

   // Vehicle objects must define a collision detail
   // HH: Retail's preload (0x608d50) has no such test, but every retail
   // vehicle shape has a Collision-1 detail, and onNewDataBlock relies on it.
   if (!collisionDetails.size() || collisionDetails[0] == -1)
   {
      Con::errorf("VehicleData::preload failed: Vehicle models must define a collision-1 detail");
      return false;
   }

   // HH: Modified for Tribes 2 compatibility.
   // Retail resolves every id on server and client alike, and says nothing
   // when an impact sound is missing.
   for (S32 i = 0; i < Body::MaxSounds; i++)
      if (!body.sound[i] && bodySoundID[i] != -1)
         Sim::findObject(bodySoundID[i], body.sound[i]);

   if( !dustEmitter && dustID != -1 )
   {
      if( !Sim::findObject( dustID, dustEmitter ) )
      {
         Con::errorf( ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(dustEmitter): 0x%x", dustID );
      }
   }

   U32 i;
   for( i=0; i<VC_NUM_DAMAGE_EMITTERS; i++ )
   {
      if( !damageEmitterList[i] && damageEmitterIDList[i] != -1 )
      {
         if( !Sim::findObject( damageEmitterIDList[i], damageEmitterList[i] ) )
         {
            Con::errorf( ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(damageEmitter): 0x%x", damageEmitterIDList[i] );
         }
      }
   }

   for( i=0; i<VC_NUM_SPLASH_EMITTERS; i++ )
   {
      if( !splashEmitterList[i] && splashEmitterIDList[i] != -1 )
      {
         if( !Sim::findObject( splashEmitterIDList[i], splashEmitterList[i] ) )
         {
            Con::errorf( ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(splashEmitter): 0x%x", splashEmitterIDList[i] );
         }
      }
   }

   // HH: Added for Tribes 2 compatibility.
   for( i=0; i<MaxSounds; i++ )
   {
      if( !waterSound[i] && waterSoundID[i] != -1 &&
          !Sim::findObject( waterSoundID[i], waterSound[i] ) )
         Con::errorf( ConsoleLogEntry::General, "VehicleData::preload: Invalid packet, bad datablockId(waterSound[%d]): 0x%x", i, waterSoundID[i] );
   }

   return true;
}

// HH: Added for Tribes 2 compatibility.
// The Sim id of a datablock the last body named, or -1 where its flag said
// none.
S32 VehicleData::sentDataBlockId(U32 field)
{
   U32 id;
   return T2Layout::lastDataBlockWord(field, &id) ?
          S32(id + DataBlockObjectIdFirst) : -1;
}

// HH: Added for Tribes 2 compatibility.
// The fields VehicleData::unpackData (0x609450) reads after ShapeBaseData's.
bool VehicleData::unpackData(BitStream *stream, U32 classId)
{
   if (!Parent::unpackData(stream, classId))
      return false;

   // bodyRestitution opens the run, so a body without it is one whose layout
   // the run was not found in.
   F32 value;
   if (isUnpackedInPlace() ||
       !T2Layout::lastDataBlockField(T2Layout::T2DbBodyRestitution, &value))
      return true;

   body.restitution = value;
   T2Layout::lastDataBlockField(T2Layout::T2DbBodyFriction, &body.friction);

   static const struct { U32 field; F32 VehicleData::*member; } sFields[] =
   {
      { T2Layout::T2DbMinImpactSpeed,         &VehicleData::minImpactSpeed         },
      { T2Layout::T2DbSoftImpactSpeed,        &VehicleData::softImpactSpeed        },
      { T2Layout::T2DbHardImpactSpeed,        &VehicleData::hardImpactSpeed        },
      { T2Layout::T2DbMinRollSpeed,           &VehicleData::minRollSpeed           },
      { T2Layout::T2DbMaxSteerinAngle,        &VehicleData::maxSteeringAngle       },
      { T2Layout::T2DbMaxDrag,                &VehicleData::maxDrag                },
      { T2Layout::T2DbMinDrag,                &VehicleData::minDrag                },
      { T2Layout::T2DbJetForce,               &VehicleData::jetForce               },
      { T2Layout::T2DbJetEnergyDrain,         &VehicleData::jetEnergyDrain         },
      { T2Layout::T2DbMinJetEnergy,           &VehicleData::minJetEnergy           },
      { T2Layout::T2DbCameraOffset,           &VehicleData::cameraOffset           },
      { T2Layout::T2DbCameraLag,              &VehicleData::cameraLag              },
      { T2Layout::T2DbTriggerDustHeight,      &VehicleData::triggerDustHeight      },
      { T2Layout::T2DbDustHeight,             &VehicleData::dustHeight             },
      { T2Layout::T2DbNumDmgEmitterAreas,     &VehicleData::numDmgEmitterAreas     },
      { T2Layout::T2DbExitSplashSoundVel,     &VehicleData::exitSplashSoundVel     },
      { T2Layout::T2DbSoftSplashSoundVel,     &VehicleData::softSplashSoundVel     },
      { T2Layout::T2DbMedSplashSoundVel,      &VehicleData::medSplashSoundVel      },
      { T2Layout::T2DbHardSplashSoundVel,     &VehicleData::hardSplashSoundVel     },
      { T2Layout::T2DbSplashFreqMod,          &VehicleData::splashFreqMod          },
      { T2Layout::T2DbSplashVelEpsilon,       &VehicleData::splashVelEpsilon       },
      { T2Layout::T2DbCollDamageThresholdVel, &VehicleData::collDamageThresholdVel },
      { T2Layout::T2DbCollDamageMultiplier,   &VehicleData::collDamageMultiplier   },
   };

   for (U32 i = 0; i < sizeof(sFields) / sizeof(sFields[0]); i++)
      T2Layout::lastDataBlockField(sFields[i].field, &(this->*sFields[i].member));

   for (U32 i = 0; i < VC_NUM_DAMAGE_EMITTER_AREAS; i++)
   {
      const U32 at = T2Layout::T2DbDamageEmitterOffset + i * 3;
      T2Layout::lastDataBlockField(at,     &damageEmitterOffset[i].x);
      T2Layout::lastDataBlockField(at + 1, &damageEmitterOffset[i].y);
      T2Layout::lastDataBlockField(at + 2, &damageEmitterOffset[i].z);
   }

   for (U32 i = 0; i < VC_NUM_DAMAGE_LEVELS; i++)
      T2Layout::lastDataBlockField(T2Layout::T2DbDamageLevelTolerance + i,
                                   &damageLevelTolerance[i]);

   // Retail lets go of the sounds it had, to look the new ids up in preload,
   // but keeps the emitters it had.
   for (U32 i = 0; i < Body::MaxSounds; i++)
   {
      body.sound[i] = NULL;
      bodySoundID[i] = sentDataBlockId(T2Layout::T2DbBodySound + i);
   }

   for (U32 i = 0; i < MaxSounds; i++)
   {
      waterSound[i] = NULL;
      waterSoundID[i] = sentDataBlockId(T2Layout::T2DbWaterSound + i);
   }

   dustID = sentDataBlockId(T2Layout::T2DbDustEmitter);
   for (U32 i = 0; i < VC_NUM_DAMAGE_EMITTERS; i++)
      damageEmitterIDList[i] =
         sentDataBlockId(T2Layout::T2DbDamageEmitter + i);
   for (U32 i = 0; i < VC_NUM_SPLASH_EMITTERS; i++)
      splashEmitterIDList[i] =
         sentDataBlockId(T2Layout::T2DbSplashEmitter + i);

   return true;
}


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------

void VehicleData::initPersistFields()
{
   addField( "jetForce", TypeF32, Offset(jetForce, VehicleData),
      "@brief Additional force applied to the vehicle when it is jetting.\n\n"
      "For WheeledVehicles, the force is applied in the forward direction. For "
      "FlyingVehicles, the force is applied in the thrust direction." );
   addField( "jetEnergyDrain", TypeF32, Offset(jetEnergyDrain, VehicleData),
      "@brief Energy amount to drain for each tick the vehicle is jetting.\n\n"
      "Once the vehicle's energy level reaches 0, it will no longer be able to jet." );
   addField( "minJetEnergy", TypeF32, Offset(minJetEnergy, VehicleData),
      "Minimum vehicle energy level to begin jetting." );

   // HH: Modified for Tribes 2 compatibility.
   // Retail has neither field: its body is a unit mass about the object's
   // origin with identity inertia (0x60a2d0).
   addField( "massCenter", TypePoint3F, Offset(massCenter, VehicleData),
      "Unused. A Tribes 2 vehicle turns about its origin." );
   addField( "massBox", TypePoint3F, Offset(massBox, VehicleData),
      "Unused. A Tribes 2 vehicle's body has identity inertia." );
   addField( "bodyRestitution", TypeF32, Offset(body.restitution, VehicleData),
      "Collision 'bounciness'.\nNormally in the range 0 (not bouncy at all) to "
      "1 (100% bounciness)." );
   addField( "bodyFriction", TypeF32, Offset(body.friction, VehicleData),
      "Collision friction coefficient.\nHow well this object will slide against "
      "objects it collides with." );
   addField( "softImpactSound", TYPEID< SFXProfile >(), Offset(body.sound[Body::SoftImpactSound], VehicleData),
      "@brief Sound to play on a 'soft' impact.\n\n"
      "This sound is played if the impact speed is < hardImpactSpeed and >= "
      "softImpactSpeed.\n\n"
      "@see softImpactSpeed" );
   addField( "hardImpactSound", TYPEID< SFXProfile >(), Offset(body.sound[Body::HardImpactSound], VehicleData),
      "@brief Sound to play on a 'hard' impact.\n\n"
      "This sound is played if the impact speed >= hardImpactSpeed.\n\n"
      "@see hardImpactSpeed" );

   addField( "minImpactSpeed", TypeF32, Offset(minImpactSpeed, VehicleData),
      "Minimum collision speed for the onImpact callback to be invoked." );
   addField( "softImpactSpeed", TypeF32, Offset(softImpactSpeed, VehicleData),
      "Minimum collision speed for the softImpactSound to be played." );
   addField( "hardImpactSpeed", TypeF32, Offset(hardImpactSpeed, VehicleData),
      "Minimum collision speed for the hardImpactSound to be played." );
   addField( "minRollSpeed", TypeF32, Offset(minRollSpeed, VehicleData),
      "Unused" );
   // HH: Modified for Tribes 2 compatibility.
   // Retail registers the name misspelt (0x609b00), so a script's
   // maxSteeringAngle is a dynamic field and never reaches the engine.
   addField( "maxSteerinAngle", TypeF32, Offset(maxSteeringAngle, VehicleData),
      "Maximum yaw (horizontal) and pitch (vertical) steering angle in radians." );

   addField( "maxDrag", TypeF32, Offset(maxDrag, VehicleData),
      "Maximum drag coefficient.\nCurrently unused." );
   addField( "minDrag", TypeF32, Offset(minDrag, VehicleData),
      "Minimum drag coefficient.\nCurrently only used by FlyingVehicle." );
   // HH: Modified for Tribes 2 compatibility.
   // Retail has no such field: its tick runs the forces once (0x60a800) and
   // updatePos picks its own steps.
   addField( "integration", TypeS32, Offset(integration, VehicleData),
      "Unused. A Tribes 2 vehicle takes the forces once a tick." );
   addField( "collisionTol", TypeF32, Offset(collisionTol, VehicleData),
      "Minimum distance between objects for them to be considered as colliding." );
   addField( "contactTol", TypeF32, Offset(contactTol, VehicleData),
      "Maximum relative velocity between objects for collisions to be resolved "
      "as contacts.\nVelocities greater than this are handled as collisions." );

   addField( "cameraRoll", TypeBool, Offset(cameraRoll, VehicleData),
      "If true, the camera will roll with the vehicle. If false, the camera will "
      "always have the positive Z axis as up." );
   // HH: Modified for Tribes 2 compatibility.
   // Retail sends cameraLag with the datablock but nothing reads it, and has
   // no cameraDecay: a vehicle's camera is ShapeBase's.
   addField( "cameraLag", TypeF32, Offset(cameraLag, VehicleData),
      "Unused. Sent with the datablock, as Tribes 2 sends it." );
   addField("cameraDecay",  TypeF32, Offset(cameraDecay, VehicleData),
      "Unused." );
   addField("cameraOffset", TypeF32, Offset(cameraOffset, VehicleData),
      "Vertical (Z axis) height of the camera above the vehicle." );

   addField( "dustEmitter", TYPEID< ParticleEmitterData >(), Offset(dustEmitter, VehicleData),
      "Dust particle emitter.\n\n@see triggerDustHeight\n\n@see dustHeight");
   addField( "triggerDustHeight", TypeF32, Offset(triggerDustHeight, VehicleData),
      "@brief Maximum height above surface to emit dust particles.\n\n"
      "If the vehicle is less than triggerDustHeight above a static surface "
      "with a material that has 'showDust' set to true, the vehicle will emit "
      "particles from the dustEmitter." );
   addField( "dustHeight", TypeF32, Offset(dustHeight, VehicleData),
      "Height above ground at which to emit particles from the dustEmitter." );

   addField( "damageEmitter", TYPEID< ParticleEmitterData >(), Offset(damageEmitterList, VehicleData), VC_NUM_DAMAGE_EMITTERS,
      "@brief Array of particle emitters used to generate damage (dust, smoke etc) "
      "effects.\n\n"
      "Currently, the first two emitters (indices 0 and 1) are used when the damage "
      "level exceeds the associated damageLevelTolerance. The 3rd emitter is used "
      "when the emitter point is underwater.\n\n"
      "@see damageEmitterOffset" );
   addField( "damageEmitterOffset", TypePoint3F, Offset(damageEmitterOffset, VehicleData), VC_NUM_DAMAGE_EMITTER_AREAS,
      "@brief Object space \"x y z\" offsets used to emit particles for the "
      "active damageEmitter.\n\n"
      "@tsexample\n"
      "// damage levels\n"
      "damageLevelTolerance[0] = 0.5;\n"
      "damageEmitter[0] = SmokeEmitter;\n"
      "// emit offsets (used for all active damage level emitters)\n"
      "damageEmitterOffset[0] = \"0.5 3 1\";\n"
      "damageEmitterOffset[1] = \"-0.5 3 1\";\n"
      "numDmgEmitterAreas = 2;\n"
      "@endtsexample\n" );
   addField( "damageLevelTolerance", TypeF32, Offset(damageLevelTolerance, VehicleData), VC_NUM_DAMAGE_LEVELS,
      "@brief Damage levels (as a percentage of maxDamage) above which to begin "
      "emitting particles from the associated damageEmitter.\n\n"
      "Levels should be in order of increasing damage.\n\n"
      "@see damageEmitterOffset" );
   addField( "numDmgEmitterAreas", TypeF32, Offset(numDmgEmitterAreas, VehicleData),
      "Number of damageEmitterOffset values to use for each damageEmitter.\n\n"
      "@see damageEmitterOffset" );

   addField( "splashEmitter", TYPEID< ParticleEmitterData >(), Offset(splashEmitterList, VehicleData), VC_NUM_SPLASH_EMITTERS,
      "Array of particle emitters used to generate splash effects." );
   addField( "splashFreqMod",  TypeF32, Offset(splashFreqMod, VehicleData),
      "@brief Number of splash particles to generate based on vehicle speed.\n\n"
      "This value is multiplied by the current speed to determine how many "
      "particles to generate each frame." );
   addField( "splashVelEpsilon", TypeF32, Offset(splashVelEpsilon, VehicleData),
      "Minimum speed when moving through water to generate splash particles." );

   addField( "exitSplashSoundVelocity", TypeF32, Offset(exitSplashSoundVel, VehicleData),
      "Minimum velocity when leaving the water for the exitingWater sound to play." );
   addField( "softSplashSoundVelocity", TypeF32, Offset(softSplashSoundVel, VehicleData),
      "Minimum velocity when entering the water for the imapactWaterEasy sound "
      "to play.\n\n@see impactWaterEasy" );
   addField( "mediumSplashSoundVelocity", TypeF32, Offset(medSplashSoundVel, VehicleData),
      "Minimum velocity when entering the water for the imapactWaterMedium sound "
      "to play.\n\n@see impactWaterMedium" );
   addField( "hardSplashSoundVelocity", TypeF32, Offset(hardSplashSoundVel, VehicleData),
      "Minimum velocity when entering the water for the imapactWaterHard sound "
      "to play.\n\n@see impactWaterHard" );
   addField( "exitingWater", TYPEID< SFXProfile >(), Offset(waterSound[ExitWater], VehicleData),
      "Sound to play when exiting the water." );
   addField( "impactWaterEasy", TYPEID< SFXProfile >(), Offset(waterSound[ImpactSoft], VehicleData),
      "Sound to play when entering the water with speed >= softSplashSoundVelocity "
      "and < mediumSplashSoundVelocity." );
   addField( "impactWaterMedium", TYPEID< SFXProfile >(), Offset(waterSound[ImpactMedium], VehicleData),
      "Sound to play when entering the water with speed >= mediumSplashSoundVelocity "
      "and < hardSplashSoundVelocity." );
   addField( "impactWaterHard", TYPEID< SFXProfile >(), Offset(waterSound[ImpactHard], VehicleData),
      "Sound to play when entering the water with speed >= hardSplashSoundVelocity." );
   addField( "waterWakeSound", TYPEID< SFXProfile >(), Offset(waterSound[Wake], VehicleData),
      "Looping sound to play while moving through the water." );

   addField( "collDamageThresholdVel", TypeF32, Offset(collDamageThresholdVel, VehicleData),
      "Minimum collision velocity to cause damage to this vehicle.\nCurrently unused." );
   addField( "collDamageMultiplier", TypeF32, Offset(collDamageMultiplier, VehicleData),
      "@brief Damage to this vehicle after a collision (multiplied by collision "
      "velocity).\n\nCurrently unused." );

   // HH: Added for Tribes 2 compatibility.
   addField( "stuckTimerTicks", TypeS32, Offset(stuckTimerTicks, VehicleData),
      "Ticks a vehicle may spend tipped past stuckTimerAngle before it is destroyed." );
   addField( "stuckTimerAngle", TypeF32, Offset(stuckTimerAngle, VehicleData),
      "How far a vehicle may tip before the stuck timer counts." );

   Parent::initPersistFields();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(Vehicle);

ConsoleDocClass( Vehicle,
   "@brief Base functionality shared by all Vehicles (FlyingVehicle, HoverVehicle, "
   "WheeledVehicle).\n\n"
   "This object implements functionality shared by all Vehicle types, but should "
   "not be instantiated directly. Create a FlyingVehicle, HoverVehicle, or "
   "WheeledVehicle instead.\n"
   "@note The model used for any Vehicle must include a collision mesh at detail "
   "size -1.\n"
   "@ingroup Vehicles\n"
);

Vehicle::Vehicle()
{
   mDataBlock = 0;
   mTypeMask |= VehicleObjectType | DynamicShapeObjectType;

   mDelta.pos = Point3F(0,0,0);
   mDelta.posVec = Point3F(0,0,0);
   mDelta.warpTicks = mDelta.warpCount = 0;
   mDelta.dt = 1;
   mDelta.move = NullMove;
   mPredictionCount = 0;
   mDelta.cameraOffset.set(0,0,0);
   mDelta.cameraVec.set(0,0,0);
   mDelta.cameraRot.set(0,0,0);
   mDelta.cameraRotVec.set(0,0,0);

   mRigid.linPosition.set(0, 0, 0);
   mRigid.linVelocity.set(0, 0, 0);
   mRigid.angPosition.identity();
   mRigid.angVelocity.set(0, 0, 0);
   mRigid.linMomentum.set(0, 0, 0);
   mRigid.angMomentum.set(0, 0, 0);
   mContacts.clear();

   mSteering.set(0,0);
   mThrottle = 0;
   mJetting = false;

   dMemset( mDustEmitterList, 0, sizeof( mDustEmitterList ) );
   dMemset( mDamageEmitterList, 0, sizeof( mDamageEmitterList ) );
   dMemset( mSplashEmitterList, 0, sizeof( mSplashEmitterList ) );

   mDisableMove = false;
   mFrozen = false;   // HH: Added for Tribes 2 compatibility.
   restCount = 0;

   inLiquid = false;
   mWakeSound = NULL;
}

// HH: Modified for Tribes 2 compatibility.
// Retail's (0x60a290) is no error: it collides with nothing, and each
// subclass names its own.
U32 Vehicle::getCollisionMask()
{
   return 0;
}

Point3F Vehicle::getVelocity() const
{
   return mRigid.linVelocity;
}

//----------------------------------------------------------------------------

bool Vehicle::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // When loading from a mission script, the base SceneObject's transform
   // will have been set and needs to be transfered to the rigid body.
   mRigid.setTransform(mObjToWorld);

   // HH: Added for Tribes 2 compatibility.
   // Retail's body (0x60a338) has a mass of 1 whatever the datablock says:
   // the datablock's mass reaches it only through ShapeBase's, by which
   // applyImpulse and the subclasses' forces are divided. It passes half the
   // object box for an inertia, which 0x5c5d10 ignores, leaving identity.
   mRigid.mass = 1.0f;
   mRigid.oneOverMass = 1.0f / mRigid.mass;
   mRigid.objectInertia.identity();
   mRigid.invObjectInertia.identity();
   mRigid.updateInertialTensor();

   // Initialize interpolation vars.
   mDelta.rot[1] = mDelta.rot[0] = mRigid.angPosition;
   mDelta.pos = mRigid.linPosition;
   mDelta.posVec = Point3F(0,0,0);

   // Create Emitters on the client
   if( isClientObject() )
   {
      if( mDataBlock->dustEmitter )
      {
         for( int i=0; i<VehicleData::VC_NUM_DUST_EMITTERS; i++ )
         {
            mDustEmitterList[i] = new ParticleEmitter;
            mDustEmitterList[i]->onNewDataBlock( mDataBlock->dustEmitter, false );
            if( !mDustEmitterList[i]->registerObject() )
            {
               Con::warnf( ConsoleLogEntry::General, "Could not register dust emitter for class: %s", mDataBlock->getName() );
               delete mDustEmitterList[i];
               mDustEmitterList[i] = NULL;
            }
         }
      }

      U32 j;
      for( j=0; j<VehicleData::VC_NUM_DAMAGE_EMITTERS; j++ )
      {
         if( mDataBlock->damageEmitterList[j] )
         {
            mDamageEmitterList[j] = new ParticleEmitter;
            mDamageEmitterList[j]->onNewDataBlock( mDataBlock->damageEmitterList[j], false );
            if( !mDamageEmitterList[j]->registerObject() )
            {
               Con::warnf( ConsoleLogEntry::General, "Could not register damage emitter for class: %s", mDataBlock->getName() );
               delete mDamageEmitterList[j];
               mDamageEmitterList[j] = NULL;
            }

         }
      }

      for( j=0; j<VehicleData::VC_NUM_SPLASH_EMITTERS; j++ )
      {
         if( mDataBlock->splashEmitterList[j] )
         {
            mSplashEmitterList[j] = new ParticleEmitter;
            mSplashEmitterList[j]->onNewDataBlock( mDataBlock->splashEmitterList[j], false );
            if( !mSplashEmitterList[j]->registerObject() )
            {
               Con::warnf( ConsoleLogEntry::General, "Could not register splash emitter for class: %s", mDataBlock->getName() );
               delete mSplashEmitterList[j];
               mSplashEmitterList[j] = NULL;
            }

         }
      }
   }

   // Create a new convex.
   AssertFatal(mDataBlock->collisionDetails[0] != -1, "Error, a vehicle must have a collision-1 detail!");
   mConvex.mObject    = this;
   mConvex.pShapeBase = this;
   mConvex.hullId     = 0;
   mConvex.box        = mObjBox;
   mConvex.box.minExtents.convolve(mObjScale);
   mConvex.box.maxExtents.convolve(mObjScale);
   mConvex.findNodeTransform();

   return true;
}

void Vehicle::onRemove()
{
   U32 i=0;
   for( i=0; i<VehicleData::VC_NUM_DUST_EMITTERS; i++ )
   {
      if( mDustEmitterList[i] )
      {
         mDustEmitterList[i]->deleteWhenEmpty();
         mDustEmitterList[i] = NULL;
      }
   }

   for( i=0; i<VehicleData::VC_NUM_DAMAGE_EMITTERS; i++ )
   {
      if( mDamageEmitterList[i] )
      {
         mDamageEmitterList[i]->deleteWhenEmpty();
         mDamageEmitterList[i] = NULL;
      }
   }

   for( i=0; i<VehicleData::VC_NUM_SPLASH_EMITTERS; i++ )
   {
      if( mSplashEmitterList[i] )
      {
         mSplashEmitterList[i]->deleteWhenEmpty();
         mSplashEmitterList[i] = NULL;
      }
   }

   Parent::onRemove();
}


//----------------------------------------------------------------------------

void Vehicle::processTick(const Move* move)
{
   PROFILE_SCOPE( Vehicle_ProcessTick );

   Parent::processTick(move);

   // Warp to catch up to server
   if (mDelta.warpCount < mDelta.warpTicks)
   {
      mDelta.warpCount++;

      // Set new pos.
      mObjToWorld.getColumn(3,&mDelta.pos);
      mDelta.pos += mDelta.warpOffset;
      mDelta.rot[0] = mDelta.rot[1];
      mDelta.rot[1].interpolate(mDelta.warpRot[0],mDelta.warpRot[1],F32(mDelta.warpCount)/mDelta.warpTicks);
      setPosition(mDelta.pos,mDelta.rot[1]);
      onWarpTick();   // HH: Added for Tribes 2 compatibility.

      // Pos backstepping
      mDelta.posVec.x = -mDelta.warpOffset.x;
      mDelta.posVec.y = -mDelta.warpOffset.y;
      mDelta.posVec.z = -mDelta.warpOffset.z;
   }
   else 
   {
      if (!move) 
      {
         if (isGhost()) 
         {
            // If we haven't run out of prediction time,
            // predict using the last known move.
            if (mPredictionCount-- <= 0)
               return;
            move = &mDelta.move;
         }
         else
            move = &NullMove;
      }

      // HH: Added for Tribes 2 compatibility.
      // Retail steps a frozen vehicle not at all (0x60a965): it stays where
      // the server put it, and a ghost must not predict it off from there.
      if (mFrozen)
      {
         mDelta.pos = mRigid.linPosition;
         mDelta.posVec.set(0, 0, 0);
         mDelta.rot[1] = mDelta.rot[0] = mRigid.angPosition;
         setPosition(mRigid.linPosition, mRigid.angPosition);
         return;
      }

      // HH: Modified for Tribes 2 compatibility.
      // Retail's order (0x60a965): the working set comes before the move,
      // and the forces are summed here, once a tick, rather than inside
      // updatePos. There is no integration count to loop over.
      updateWorkingCollisionSet(getCollisionMask());

      // Process input move
      updateMove(move);

      // Save current rigid state interpolation
      mDelta.posVec = mRigid.linPosition;
      mDelta.rot[0] = mRigid.angPosition;

      mRigid.clearForces();
      updateForces(TickSec);
      updatePos(TickSec);

      // Wrap up interpolation info
      mDelta.pos     = mRigid.linPosition;
      mDelta.posVec -= mRigid.linPosition;
      mDelta.rot[1]  = mRigid.angPosition;

      // Update container database
      setPosition(mRigid.linPosition, mRigid.angPosition);
      setMaskBits(PositionMask);
      updateContainer();
   }
}

void Vehicle::interpolateTick(F32 dt)
{
   PROFILE_SCOPE( Vehicle_InterpolateTick );

   Parent::interpolateTick(dt);

   // HH: Added for Tribes 2 compatibility.
   // Retail draws a frozen vehicle where its last tick left it (0x60abe1).
   if (mFrozen)
   {
      mDelta.dt = 0;
      return;
   }

   if(dt == 0.0f)
      setRenderPosition(mDelta.pos, mDelta.rot[1]);
   else
   {
      QuatF rot;
      rot.interpolate(mDelta.rot[1], mDelta.rot[0], dt);
      Point3F pos = mDelta.pos + mDelta.posVec * dt;
      setRenderPosition(pos,rot);
   }
   mDelta.dt = dt;
}

void Vehicle::advanceTime(F32 dt)
{
   PROFILE_SCOPE( Vehicle_AdvanceTime );

   Parent::advanceTime(dt);

   updateLiftoffDust( dt );
   updateDamageSmoke( dt );
   updateFroth(dt);
}


//----------------------------------------------------------------------------

bool Vehicle::onNewDataBlock(GameBaseData* dptr,bool reload)
{
   mDataBlock = dynamic_cast<VehicleData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   // HH: Modified for Tribes 2 compatibility.
   // Retail's (0x60ace0) gives the body nothing: its mass and inertia are
   // set once in onAdd, its restitution and friction keep the rigid body's
   // own 0.3 and 0.5, and there is no massCenter to move.

   if (isGhost()) 
   {
      // Create the sound ahead of time.  This reduces runtime
      // costs and makes the system easier to understand.
      SFX_DELETE( mWakeSound );

      if ( mDataBlock->waterSound[VehicleData::Wake] )
         mWakeSound = SFX->createSource( mDataBlock->waterSound[VehicleData::Wake], &getTransform() );
   }

   return true;
}


//----------------------------------------------------------------------------

void Vehicle::getCameraParameters(F32 *min,F32* max,Point3F* off,MatrixF* rot)
{
   *min = mDataBlock->cameraMinDist;
   *max = mDataBlock->cameraMaxDist;

   off->set(0,0,mDataBlock->cameraOffset);
   rot->identity();
}

//----------------------------------------------------------------------------

void Vehicle::getVelocity(const Point3F& r, Point3F* v)
{
   mRigid.getVelocity(r, v);
}

// HH: Modified for Tribes 2 compatibility.
// Retail's (0x60ae30) arms the impulse from the object's transform at the
// datablock's +0x424, which preload zeroes (0x60900e), and divides it by
// ShapeBase's mass before the unit-mass body takes it (0x60aef0).
void Vehicle::applyImpulse(const Point3F &pos, const Point3F &impulse)
{
   Point3F origin;
   mObjToWorld.getColumn(3, &origin);
   mRigid.applyImpulse(pos - origin, impulse * mOneOverMass);
}


//----------------------------------------------------------------------------

void Vehicle::updateMove(const Move* move)
{
   PROFILE_SCOPE( Vehicle_UpdateMove );

   mDelta.move = *move;

   // Image Triggers
   if (mDamageState == Enabled) {
      setImageTriggerState(0,move->trigger[0]);
      setImageTriggerState(1,move->trigger[1]);
   }

   // Throttle
   if(!mDisableMove)
      mThrottle = move->y;

   // Steering
   if (move != &NullMove) {
      F32 y = move->yaw;
      mSteering.x = mClampF(mSteering.x + y,-mDataBlock->maxSteeringAngle,
                            mDataBlock->maxSteeringAngle);
      F32 p = move->pitch;
      mSteering.y = mClampF(mSteering.y + p,-mDataBlock->maxSteeringAngle,
                            mDataBlock->maxSteeringAngle);
   }
   else {
      mSteering.x = 0;
      mSteering.y = 0;
   }

   // Jetting flag
   if (move->trigger[3]) {
      if (!mJetting && getEnergyLevel() >= mDataBlock->minJetEnergy)
         mJetting = true;
      if (mJetting) {
         F32 newEnergy = getEnergyLevel() - mDataBlock->jetEnergyDrain;
         if (newEnergy < 0) {
            newEnergy = 0;
            mJetting = false;
         }
         setEnergyLevel(newEnergy);
      }
   }
   else
      mJetting = false;

   // HH: Moved for Tribes 2 compatibility.
   // Retail's updateMove (0x60b399), not its updatePos, tells the scripts
   // and the splash sounds about the water, before the tick's physics.
   if (isServerObject()) {

      // Water script callbacks. Retail's water also hides the vehicle's
      // heat, and leaving it restores it in full.
      if (!inLiquid && mWaterCoverage != 0.0f) {
         mDataBlock->onEnterLiquid_callback( this, mWaterCoverage, mLiquidType.c_str() );
         inLiquid = true;
         mHeat = 0.0f;
      }
      else if (inLiquid && mWaterCoverage == 0.0f) {
         mDataBlock->onLeaveLiquid_callback( this, mLiquidType.c_str() );
         inLiquid = false;
         mHeat = 1.0f;
      }
   }
   else {

      // Water volume sounds
      F32 vSpeed = getVelocity().len();
      if (!inLiquid && mWaterCoverage >= 0.8f) {
         if (vSpeed >= mDataBlock->hardSplashSoundVel)
            SFX->playOnce( mDataBlock->waterSound[VehicleData::ImpactHard], &getTransform() );
         else
            if (vSpeed >= mDataBlock->medSplashSoundVel)
               SFX->playOnce( mDataBlock->waterSound[VehicleData::ImpactMedium], &getTransform() );
         else
            if (vSpeed >= mDataBlock->softSplashSoundVel)
               SFX->playOnce( mDataBlock->waterSound[VehicleData::ImpactSoft], &getTransform() );
         inLiquid = true;
      }
      else
         if(inLiquid && mWaterCoverage < 0.8f) {
            if (vSpeed >= mDataBlock->exitSplashSoundVel)
               SFX->playOnce( mDataBlock->waterSound[VehicleData::ExitWater], &getTransform() );
         inLiquid = false;
      }
   }
}


//----------------------------------------------------------------------------

void Vehicle::setPosition(const Point3F& pos,const QuatF& rot)
{
   MatrixF mat;
   rot.setMatrix(&mat);
   mat.setColumn(3,pos);
   Parent::setTransform(mat);
}

void Vehicle::setRenderPosition(const Point3F& pos, const QuatF& rot)
{
   MatrixF mat;
   rot.setMatrix(&mat);
   mat.setColumn(3,pos);
   Parent::setRenderTransform(mat);
}

void Vehicle::setTransform(const MatrixF& newMat)
{
   mRigid.setTransform(newMat);
   Parent::setTransform(newMat);
   mRigid.atRest = false;
   mContacts.clear();
}


//-----------------------------------------------------------------------------

void Vehicle::disableCollision()
{
   Parent::disableCollision();
   for (SceneObject* ptr = getMountList(); ptr; ptr = ptr->getMountLink())
      ptr->disableCollision();
}

void Vehicle::enableCollision()
{
   Parent::enableCollision();
   for (SceneObject* ptr = getMountList(); ptr; ptr = ptr->getMountLink())
      ptr->enableCollision();
}


//----------------------------------------------------------------------------
/** Update the physics
*/

void Vehicle::updatePos(F32 dt)
{
   PROFILE_SCOPE( Vehicle_UpdatePos );

   Point3F origVelocity = mRigid.linVelocity;

   // HH: Modified for Tribes 2 compatibility.
   // processTick has already summed the forces. Retail's body (0x60b730)
   // moves in steps of half the tick, which only a contact shortens, so a
   // vehicle touching nothing integrates twice a tick, at dt / 2 each time.
   // Torque's collision response runs in each step until the solver is
   // retail's.
   const F32 step = dt * 0.5f;
   bool collided = false;
   for (U32 i = 0; i < 2; i++)
   {
      // Update collision information based on our current pos.
      if (!mRigid.atRest) {
         collided |= updateCollision(step);

         // Now that all the forces have been processed, lets
         // see if we're at rest.  Basically, if the kinetic energy of
         // the vehicles is less than some percentage of the energy added
         // by gravity for a short period, we're considered at rest.
         // This should really be part of the rigid class...
         if (mCollisionList.getCount())
         {
            F32 k = mRigid.getKineticEnergy();
            F32 G = sVehicleGravity * step;
            F32 Kg = 0.5 * mRigid.mass * G * G;
            if (k < sRestTol * Kg && ++restCount > sRestCount)
               mRigid.setAtRest();
         }
         else
            restCount = 0;
      }

      // Integrate forward
      if (!mRigid.atRest)
         mRigid.integrate(step);
   }

   // Deal with client and server scripting, sounds, etc.
   if (isServerObject()) {

      // Check triggers and other objects that we normally don't
      // collide with.  This function must be called before notifyCollision
      // as it will queue collision.
      checkTriggers();

      // Invoke the onCollision notify callback for all the objects
      // we've just hit.
      notifyCollision();

      // Server side impact script callback
      if (collided) {
         VectorF collVec = mRigid.linVelocity - origVelocity;
         F32 collSpeed = collVec.len();
         if (collSpeed > mDataBlock->minImpactSpeed)
            onImpact(collVec);
      }
   }
   else {

      // Play impact sounds on the client.
      if (collided) {
         F32 collSpeed = (mRigid.linVelocity - origVelocity).len();
         S32 impactSound = -1;
         if (collSpeed >= mDataBlock->hardImpactSpeed)
            impactSound = VehicleData::Body::HardImpactSound;
         else
            if (collSpeed >= mDataBlock->softImpactSpeed)
               impactSound = VehicleData::Body::SoftImpactSound;

         if (impactSound != -1 && mDataBlock->body.sound[impactSound] != NULL)
            SFX->playOnce( mDataBlock->body.sound[impactSound], &getTransform() );
      }
   }
}


//----------------------------------------------------------------------------

void Vehicle::updateForces(F32 /*dt*/)
{
   // Nothing here.
}


//-----------------------------------------------------------------------------
/** Update collision information
   Update the convex state and check for collisions. If the object is in
   collision, impact and contact forces are generated.
*/

bool Vehicle::updateCollision(F32 dt)
{
   PROFILE_SCOPE( Vehicle_UpdateCollision );

   // Update collision information
   MatrixF mat,cmat;
   mConvex.transform = &mat;
   mRigid.getTransform(&mat);
   cmat = mConvex.getTransform();

   mCollisionList.clear();
   CollisionState *state = mConvex.findClosestState(cmat, getScale(), mDataBlock->collisionTol);
   if (state && state->dist <= mDataBlock->collisionTol) 
   {
      //resolveDisplacement(ns,state,dt);
      mConvex.getCollisionInfo(cmat, getScale(), &mCollisionList, mDataBlock->collisionTol);
   }

   // Resolve collisions
   bool collided = resolveCollision(mRigid,mCollisionList);
   resolveContacts(mRigid,mCollisionList,dt);
   return collided;
}


//----------------------------------------------------------------------------
/** Resolve collision impacts
   Handle collision impacts, as opposed to contacts. Impulses are calculated based
   on standard collision resolution formulas.
*/
bool Vehicle::resolveCollision(Rigid&  ns,CollisionList& cList)
{
   PROFILE_SCOPE( Vehicle_ResolveCollision );

   // Apply impulses to resolve collision
   bool collided = false;
   for (S32 i = 0; i < cList.getCount(); i++) 
   {
      Collision& c = cList[i];
      if (c.distance < mDataBlock->collisionTol) 
      {
         // Velocity into surface
         Point3F v,r;
         ns.getOriginVector(c.point,&r);
         ns.getVelocity(r,&v);
         F32 vn = mDot(v,c.normal);

         // Only interested in velocities greater than sContactTol,
         // velocities less than that will be dealt with as contacts
         // "constraints".
         if (vn < -mDataBlock->contactTol) 
         {

            // Apply impulses to the rigid body to keep it from
            // penetrating the surface.
            ns.resolveCollision(cList[i].point,
               cList[i].normal);
            collided  = true;

            // Keep track of objects we collide with
            if (!isGhost() && c.object->getTypeMask() & ShapeBaseObjectType) 
            {
               ShapeBase* col = static_cast<ShapeBase*>(c.object);
               queueCollision(col,v - col->getVelocity());
            }
         }
      }
   }

   return collided;
}

//----------------------------------------------------------------------------
/** Resolve contact forces
   Resolve contact forces using the "penalty" method. Forces are generated based
   on the depth of penetration and the moment of inertia at the point of contact.
*/
bool Vehicle::resolveContacts(Rigid& ns,CollisionList& cList,F32 dt)
{
   PROFILE_SCOPE( Vehicle_ResolveContacts );

   // Use spring forces to manage contact constraints.
   bool collided = false;
   Point3F t,p(0,0,0),l(0,0,0);
   for (S32 i = 0; i < cList.getCount(); i++) 
   {
      const Collision& c = cList[i];
      if (c.distance < mDataBlock->collisionTol) 
      {

         // Velocity into the surface
         Point3F v,r;
         ns.getOriginVector(c.point,&r);
         ns.getVelocity(r,&v);
         F32 vn = mDot(v,c.normal);

         // Only interested in velocities less than mDataBlock->contactTol,
         // velocities greater than that are dealt with as collisions.
         if (mFabs(vn) < mDataBlock->contactTol) 
         {
            collided = true;

            // Penetration force. This is actually a spring which
            // will seperate the body from the collision surface.
            F32 zi = 2 * mFabs(mRigid.getZeroImpulse(r,c.normal));
            F32 s = (mDataBlock->collisionTol - c.distance) * zi - ((vn / mDataBlock->contactTol) * zi);
            Point3F f = c.normal * s;

            // Friction impulse, calculated as a function of the
            // amount of force it would take to stop the motion
            // perpendicular to the normal.
            Point3F uv = v - (c.normal * vn);
            F32 ul = uv.len();
            if (s > 0 && ul) 
            {
               uv /= -ul;
               F32 u = ul * ns.getZeroImpulse(r,uv);
               s *= mRigid.friction;
               if (u > s)
                  u = s;
               f += uv * u;
            }

            // Accumulate forces
            p += f;
            mCross(r,f,&t);
            l += t;
         }
      }
   }

   // Contact constraint forces act over time...
   ns.linMomentum += p * dt;
   ns.angMomentum += l * dt;
   ns.updateVelocity();
   return true;
}


//----------------------------------------------------------------------------

bool Vehicle::resolveDisplacement(Rigid& ns,CollisionState *state, F32 dt)
{
   PROFILE_SCOPE( Vehicle_ResolveDisplacement );

   SceneObject* obj = (state->a->getObject() == this)?
       state->b->getObject(): state->a->getObject();

   if (obj->isDisplacable() && ((obj->getTypeMask() & ShapeBaseObjectType) != 0))
   {
      // Try to displace the object by the amount we're trying to move
      Point3F objNewMom = ns.linVelocity * obj->getMass() * 1.1f;
      Point3F objOldMom = obj->getMomentum();
      Point3F objNewVel = objNewMom / obj->getMass();

      Point3F myCenter;
      Point3F theirCenter;
      getWorldBox().getCenter(&myCenter);
      obj->getWorldBox().getCenter(&theirCenter);
      if (mDot(myCenter - theirCenter, objNewMom) >= 0.0f || objNewVel.len() < 0.01)
      {
         objNewMom = (theirCenter - myCenter);
         objNewMom.normalize();
         objNewMom *= 1.0f * obj->getMass();
         objNewVel = objNewMom / obj->getMass();
      }

      obj->setMomentum(objNewMom);
      if (obj->displaceObject(objNewVel * 1.1f * dt) == true)
      {
         // Queue collision and change in velocity
         VectorF dv = (objOldMom - objNewMom) / obj->getMass();
         queueCollision(static_cast<ShapeBase*>(obj), dv);
         return true;
      }
   }

   return false;
}


//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// Retail's (0x60af50) keeps no cached box: every tick it rebuilds the list
// from the convex's box, grown by this tick's travel plus the 1.5625 m that
// 50 m/s would add, times 1.1 and 0.1 more.
void Vehicle::updateWorkingCollisionSet(const U32 mask)
{
   PROFILE_SCOPE( Vehicle_UpdateWorkingCollisionSet );

   Box3F convexBox = mConvex.getBoundingBox(getTransform(), getScale());
   const F32 len = (mRigid.linVelocity * TickSec).len();
   const F32 l = F32((len + 1.5625f) * 1.1 + 0.1);
   convexBox.minExtents -= Point3F(l, l, l);
   convexBox.maxExtents += Point3F(l, l, l);

   disableCollision();
   mConvex.updateWorkingList(convexBox, mask);
   enableCollision();
}


//----------------------------------------------------------------------------
/** Check collisions with trigger and items
   Perform a container search using the current bounding box
   of the main body, wheels are not included.  This method should
   only be called on the server.
*/
void Vehicle::checkTriggers()
{
   Box3F bbox = mConvex.getBoundingBox(getTransform(), getScale());
   gServerContainer.findObjects(bbox,sTriggerMask,findCallback,this);
}

/** The callback used in by the checkTriggers() method.
   The checkTriggers method uses a container search which will
   invoke this callback on each obj that matches.
*/
void Vehicle::findCallback(SceneObject* obj,void *key)
{
   Vehicle* vehicle = reinterpret_cast<Vehicle*>(key);
   U32 objectMask = obj->getTypeMask();

   // Check: triggers, corpses and items, basically the same things
   // that the player class checks for
   if (objectMask & TriggerObjectType) {
      Trigger* pTrigger = static_cast<Trigger*>(obj);
      pTrigger->potentialEnterObject(vehicle);
   }
   else if (objectMask & CorpseObjectType) {
      ShapeBase* col = static_cast<ShapeBase*>(obj);
      vehicle->queueCollision(col,vehicle->getVelocity() - col->getVelocity());
   }
   else if (objectMask & ItemObjectType) {
      Item* item = static_cast<Item*>(obj);
      if (vehicle != item->getCollisionObject())
         vehicle->queueCollision(item,vehicle->getVelocity() - item->getVelocity());
   }
}


//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// Vehicle::writePacketData (0x60d5a0): the body's momenta, with no atRest or
// contacts flag, and the frozen state last.
void Vehicle::writePacketData(GameConnection *connection, BitStream *stream)
{
   Parent::writePacketData(connection, stream);
   mathWrite(*stream, mSteering);

   mathWrite(*stream, mRigid.linPosition);
   mathWrite(*stream, mRigid.angPosition);
   mathWrite(*stream, mRigid.linMomentum);
   mathWrite(*stream, mRigid.angMomentum);

   stream->writeFlag(mDisableMove);
   stream->writeFlag(mFrozen);
   // Retail sets it on the connection (0x588a90), as Player's does.
   stream->setCompressionPoint(mRigid.linPosition);
}

// HH: Modified for Tribes 2 compatibility.
// Vehicle::readPacketData (0x60d740). Retail refreshes only its velocities
// from the momenta it reads (0x5c57a0), and leaves atRest and the contacts
// as they were.
void Vehicle::readPacketData(GameConnection *connection, BitStream *stream)
{
   Parent::readPacketData(connection, stream);
   mathRead(*stream, &mSteering);

   Point3F pos, linMom, angMom;
   QuatF rot;
   mathRead(*stream, &pos);
   mathRead(*stream, &rot);
   mathRead(*stream, &linMom);
   mathRead(*stream, &angMom);
   setRigidStateT2(pos, rot, linMom, angMom);

   mDisableMove = stream->readFlag();
   mFrozen = stream->readFlag();
   stream->setCompressionPoint(mRigid.linPosition);
}

// HH: Added for Tribes 2 compatibility.
// What retail's packet data (0x60d8ae) and ghost (0x60df5e) do with a body
// they are sent: keep the momenta, and take the velocities from them through
// the world inertia already stored, not one from the new rotation. With unit
// mass and identity inertia the velocities are the momenta, but for the
// low bits that R*I*R^T rounds.
void Vehicle::setRigidStateT2(const Point3F &pos, const QuatF &rot,
                              const Point3F &linMom, const Point3F &angMom)
{
   mRigid.linPosition = pos;
   mRigid.angPosition = rot;
   mRigid.linMomentum = linMom;
   mRigid.angMomentum = angMom;
   mRigid.updateVelocity();
   // Torque's own rigid body keeps a world centre of mass beside the
   // position; with massCenter inert it is the position.
   mRigid.updateCenterOfMass();
}

// HH: Added for Tribes 2 compatibility.
// Vehicle::unpackUpdate (0x60dc30). TGE's has the same shape, but retail's
// warp starts from where the object stands rather than from where it was
// being drawn, holds a frozen vehicle where the server puts it, and has no
// atRest or contacts flag to read.
void Vehicle::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   const T2Layout::Body::VehicleTail &tail = body.vehicle;
   if (!tail.present)
      return;

   mJetting = body.jetting;
   if (tail.controlled)
      return;

   // Retail scales these by the datablock the body has just named; a body
   // read before any datablock is applied has none to scale by.
   if (mDataBlock)
   {
      const F32 max = mDataBlock->maxSteeringAngle;
      mSteering.x = tail.steering[0] * 2.0f * max - max;
      mSteering.y = tail.steering[1] * 2.0f * max - max;
   }

   Move move = NullMove;
   move.pyaw   = tail.hasMoveRot[0] ? tail.moveRot[0] : 0;
   move.ppitch = tail.hasMoveRot[1] ? tail.moveRot[1] : 0;
   move.proll  = tail.hasMoveRot[2] ? tail.moveRot[2] : 0;
   move.px = tail.moveAxis[0];
   move.py = tail.moveAxis[1];
   move.pz = tail.moveAxis[2];
   move.unclamp();
   move.freeLook = tail.freeLook;
   for (U32 i = 0; i < MaxTriggerKeys; i++)
      move.trigger[i] = tail.trigger[i];
   mDelta.move = move;

   mFrozen = tail.frozen;

   if (tail.hasPosition)
   {
      const F32 speed = mRigid.linVelocity.len();
      mDelta.warpRot[0] = mRigid.angPosition;

      F32 w[10];
      dMemcpy(w, tail.words, sizeof(w));
      setRigidStateT2(tail.position, QuatF(w[0], w[1], w[2], w[3]),
                      Point3F(w[4], w[5], w[6]), Point3F(w[7], w[8], w[9]));
      mPredictionCount = sMaxPredictionTicks;

      if (isProperlyAdded() && !mFrozen)
      {
         const Point3F cp = getPosition();
         mDelta.warpOffset = mRigid.linPosition - cp;

         // Retail times the warp by a tick of exactly 1/32 s (0x7a5630).
         const F32 as = (speed + mRigid.linVelocity.len()) * 0.5f * 0.03125f;
         F32 dt;
         if (as == 0.0f || (dt = mDelta.warpOffset.len() / as) > sMaxWarpTicks)
            dt = mDelta.dt + sMaxWarpTicks;
         else
            dt = (dt <= mDelta.dt) ? mDelta.dt
                                   : mCeil(dt - mDelta.dt) + mDelta.dt;

         if (mDelta.dt != 0.0f)
         {
            mDelta.pos = cp + mDelta.warpOffset * (mDelta.dt / dt);
            mDelta.posVec = (cp - mDelta.pos) * (1.0f / mDelta.dt);
            QuatF cr;
            cr.interpolate(mDelta.rot[1], mDelta.rot[0], mDelta.dt);
            mDelta.rot[1].interpolate(cr, mRigid.angPosition, mDelta.dt / dt);
            mDelta.rot[0].extrapolate(mDelta.rot[1], cr, mDelta.dt);
         }

         mDelta.warpCount = 0;
         mDelta.warpTicks = (S32)mFloor(dt);
         if (mDelta.warpTicks)
         {
            mDelta.warpOffset = mRigid.linPosition - mDelta.pos;
            mDelta.warpOffset *= 1.0f / mDelta.warpTicks;
            mDelta.warpRot[0] = mDelta.rot[1];
            mDelta.warpRot[1] = mRigid.angPosition;
         }
      }
      else
      {
         mDelta.dt = 0;
         mDelta.pos = mRigid.linPosition;
         mDelta.posVec.set(0, 0, 0);
         mDelta.rot[1] = mDelta.rot[0] = mRigid.angPosition;
         mDelta.warpCount = mDelta.warpTicks = 0;
         setPosition(mRigid.linPosition, mRigid.angPosition);
      }
   }

   // Only a passenger is sent this: it is what the energy gauge shows.
   if (tail.hasEnergy && mDataBlock)
      setEnergyLevel(tail.energy * mDataBlock->maxEnergy);

   if (Con::getBoolVariable("$pref::Net::traceGhostVehicles", false))
      Con::printf("T2 vehicle: %s %d (ghost %d) jet %d frozen %d pos %d "
                  "(%g %g %g) warp %d steer %g %g energy %d %g",
                  getClassName(), getId(), getNetIndex(), mJetting, mFrozen,
                  tail.hasPosition, tail.position.x, tail.position.y,
                  tail.position.z, mDelta.warpTicks, mSteering.x,
                  mSteering.y, tail.hasEnergy, tail.energy);
}


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// Retail's (0x60a250) registers blowup and no variables: there is no working
// query box to tune.
void Vehicle::consoleInit()
{
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2's, at 0x0060a240. Unlike ShapeBase's it sets no mask bit; the
// damage block carries the flag when something else sends it. Our server
// does not pack that block yet, so for now only the server sees the flag.
DefineEngineMethod( Vehicle, blowup, void, (),,
   "@brief Mark the vehicle to burst into its explosion and debris.\n\n"
   "The flag goes to clients with the vehicle's next damage update.\n\n")
{
   object->setBlowApart();
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2's, at 0x00608a30 -> 0x0060e850. It sets the flag and nothing more:
// the call after it (0x5f0520) is empty, so no mask bit is set, and clients
// learn of it with the vehicle's next update.
DefineEngineMethod( Vehicle, setFrozenState, void, ( bool frozen ),,
   "@brief Hold the vehicle where it is, or let it go.\n\n"
   "A frozen vehicle takes no move and steps no physics; its clients draw it "
   "where its last tick left it.\n\n"
   "@param frozen true to freeze the vehicle, false to thaw it.\n\n")
{
   object->setFrozenState( frozen );
}

void Vehicle::initPersistFields()
{
   addField( "disableMove", TypeBool, Offset(mDisableMove, Vehicle),
      "When this flag is set, the vehicle will ignore throttle changes." );

   Parent::initPersistFields();
}


void Vehicle::mountObject(SceneObject *obj, S32 node, const MatrixF &xfm )
{
   Parent::mountObject( obj, node, xfm );

   // Clear objects off the working list that are from objects mounted to us.
   //  (This applies mostly to players...)
   for ( CollisionWorkingList* itr = mConvex.getWorkingList().wLink.mNext; 
         itr != &mConvex.getWorkingList(); 
         itr = itr->wLink.mNext) 
   {
      if (itr->mConvex->getObject() == obj) 
      {
         CollisionWorkingList* cl = itr;
         itr = itr->wLink.mPrev;
         cl->free();
      }
   }
}

//----------------------------------------------------------------------------

void Vehicle::updateLiftoffDust( F32 dt )
{
   Point3F offset( 0.0, 0.0, mDataBlock->dustHeight );
   emitDust( mDustEmitterList[ 0 ], mDataBlock->triggerDustHeight, offset,
             ( U32 )( dt * 1000 ) );
}

//----------------------------------------------------------------------------

void Vehicle::updateDamageSmoke( F32 dt )
{

   for( S32 j=VehicleData::VC_NUM_DAMAGE_LEVELS-1; j>=0; j-- )
   {
      F32 damagePercent = mDamage / mDataBlock->maxDamage;
      if( damagePercent >= mDataBlock->damageLevelTolerance[j] )
      {
         for( int i=0; i<mDataBlock->numDmgEmitterAreas; i++ )
         {
            MatrixF trans = getTransform();
            Point3F offset = mDataBlock->damageEmitterOffset[i];
            trans.mulP( offset );
            Point3F emitterPoint = offset;

            if( pointInWater(offset ) )
            {
               U32 emitterOffset = VehicleData::VC_BUBBLE_EMITTER;
               if( mDamageEmitterList[emitterOffset] )
               {
                  mDamageEmitterList[emitterOffset]->emitParticles( emitterPoint, emitterPoint, Point3F( 0.0, 0.0, 1.0 ), getVelocity(), (U32)( dt * 1000 ) );
               }
            }
            else
            {
               if( mDamageEmitterList[j] )
               {
                  mDamageEmitterList[j]->emitParticles( emitterPoint, emitterPoint, Point3F( 0.0, 0.0, 1.0 ), getVelocity(), (U32)(dt * 1000));
               }
            }
         }
         break;
      }
   }

}


//--------------------------------------------------------------------------
void Vehicle::updateFroth( F32 dt )
{
   // update bubbles
   Point3F moveDir = getVelocity();

   Point3F contactPoint;
   if( !collidingWithWater( contactPoint ) )
   {
      if ( mWakeSound )
         mWakeSound->stop();
      return;
   }

   F32 speed = moveDir.len();
   if( speed < mDataBlock->splashVelEpsilon ) speed = 0.0;

   U32 emitRate = (U32)(speed * mDataBlock->splashFreqMod * dt);

   U32 i;

   if ( mWakeSound )
   {
      if ( !mWakeSound->isPlaying() )
         mWakeSound->play();

      mWakeSound->setTransform( getTransform() );
      mWakeSound->setVelocity( getVelocity() );
   }

   for( i=0; i<VehicleData::VC_NUM_SPLASH_EMITTERS; i++ )
   {
      if( mSplashEmitterList[i] )
      {
         mSplashEmitterList[i]->emitParticles( contactPoint, contactPoint, Point3F( 0.0, 0.0, 1.0 ),
                                               moveDir, emitRate );
      }
   }

}


//--------------------------------------------------------------------------
// Returns true if vehicle is intersecting a water surface (roughly)
//--------------------------------------------------------------------------
bool Vehicle::collidingWithWater( Point3F &waterHeight )
{
   Point3F curPos = getPosition();

   F32 height = mFabs( mObjBox.maxExtents.z - mObjBox.minExtents.z );

   RayInfo rInfo;
   if( gClientContainer.castRay( curPos + Point3F(0.0, 0.0, height), curPos, WaterObjectType, &rInfo) )
   {
      waterHeight = rInfo.point;
      return true;
   }

   return false;
}

void Vehicle::setEnergyLevel(F32 energy)
{
   Parent::setEnergyLevel(energy);
   setMaskBits(EnergyMask);
}

void Vehicle::prepBatchRender( SceneRenderState *state, S32 mountedImageIndex )
{
   Parent::prepBatchRender( state, mountedImageIndex );

   if ( !gShowBoundingBox )
      return;

   if ( mountedImageIndex != -1 )
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &Vehicle::_renderMuzzleVector );
      ri->objectIndex = mountedImageIndex;
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri );
      return;
   }

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &Vehicle::_renderMassAndContacts );
   ri->type = RenderPassManager::RIT_Editor;
   state->getRenderPass()->addInst( ri );
}

void Vehicle::_renderMassAndContacts( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   GFXStateBlockDesc desc;
   desc.setBlend(false, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   desc.setZReadWrite(false,true);
   desc.fillMode = GFXFillWireframe;

   // Render the mass center.   
   GFX->getDrawUtil()->drawCube(desc, Point3F(0.1f,0.1f,0.1f),mDataBlock->massCenter, ColorI(255, 255, 255), &mRenderObjToWorld);

   // Now render all the contact points.
   for (int i = 0; i < mCollisionList.getCount(); i++)
   {
      const Collision& collision = mCollisionList[i];
      GFX->getDrawUtil()->drawCube(desc, Point3F(0.05f,0.05f,0.05f),collision.point, ColorI(0, 0, 255));
   }

   // Finally render the normals as one big batch.
   PrimBuild::begin(GFXLineList, mCollisionList.getCount() * 2);
   for (int i = 0; i < mCollisionList.getCount(); i++)
   {
      const Collision& collision = mCollisionList[i];
      PrimBuild::color3f(1, 1, 1);
      PrimBuild::vertex3fv(collision.point);
      PrimBuild::vertex3fv(collision.point + collision.normal * 0.05f);
   }
   PrimBuild::end();

   // Build and render the collision polylist which is returned
   // in the server's world space.
   ClippedPolyList polyList;
   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].set(getWorldBox().minExtents,VectorF(-1,0,0));
   polyList.mPlaneList[1].set(getWorldBox().minExtents,VectorF(0,-1,0));
   polyList.mPlaneList[2].set(getWorldBox().minExtents,VectorF(0,0,-1));
   polyList.mPlaneList[3].set(getWorldBox().maxExtents,VectorF(1,0,0));
   polyList.mPlaneList[4].set(getWorldBox().maxExtents,VectorF(0,1,0));
   polyList.mPlaneList[5].set(getWorldBox().maxExtents,VectorF(0,0,1));
   Box3F dummyBox;
   SphereF dummySphere;
   buildPolyList(PLC_Collision, &polyList, dummyBox, dummySphere);
   //polyList.render();
}

void Vehicle::_renderMuzzleVector( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   const U32 index = ri->objectIndex;

   AssertFatal( index > 0 && index < MaxMountedImages, "Vehicle::_renderMuzzleVector() - Bad object index!" );
   AssertFatal( mMountedImageList[index].dataBlock, "Vehicle::_renderMuzzleVector() - Bad object index!" );

	Point3F muzzlePoint, muzzleVector, endpoint;
	getMuzzlePoint(index, &muzzlePoint);
	getMuzzleVector(index, &muzzleVector);
	endpoint = muzzlePoint + muzzleVector * 250;

   if (mSolidSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(false, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      desc.setZReadWrite(false);
      mSolidSB = GFX->createStateBlock(desc);
   }

   GFX->setStateBlock(mSolidSB);

   PrimBuild::begin(GFXLineList, 2);

   PrimBuild::color4f(0, 1, 0, 1);
   PrimBuild::vertex3fv(muzzlePoint);
	PrimBuild::vertex3fv(endpoint);

   PrimBuild::end();
}
