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
#include "T3D/vehicles/wheeledVehicle.h"

#include "math/mMath.h"
#include "math/mathIO.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "collision/clippedPolyList.h"
#include "collision/planeExtractor.h"
#include "collision/extrudedPolyList.h"   // HH: Added for Tribes 2 compatibility.
#include "math/mPolyhedron.h"             // HH: Added for Tribes 2 compatibility.
#include "core/stream/bitStream.h"
#include "core/dnet.h"
#include "T3D/gameBase/gameConnection.h"
#include "ts/tsShapeInstance.h"
#include "T3D/fx/particleEmitter.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxTrack.h"
#include "sfx/sfxProfile.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTypes.h"
#include "scene/sceneManager.h"
#include "core/resourceManager.h"
#include "materials/materialDefinition.h"
#include "materials/baseMatInstance.h"
#include "lighting/lightQuery.h"
#include "T3D/gameBase/gravityT2.h"   // HH: Added for Tribes 2 compatibility.


// HH: Modified for Tribes 2 compatibility.
// Tribes 2's mask (0x7a6658), on server and client alike: it adds the force
// fields to Torque's.
static U32 sCollisionMask =
      TerrainObjectType    | InteriorObjectType       |
      PlayerObjectType     | StaticShapeObjectType    |
      VehicleObjectType    | VehicleBlockerObjectType |
      ForceFieldObjectType;

// Gravity constant
// HH: Modified for Tribes 2 compatibility.
static F32 &sWheeledVehicleGravity = gWorldGravity;

// Misc. sound constants
static F32 sMinSquealVolume = 0.05f;
static F32 sIdleEngineVolume = 0.2f;


//----------------------------------------------------------------------------
// Wheeled Vehicle Data Block
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(WheeledVehicleData);

ConsoleDocClass( WheeledVehicleData,
   "@brief Defines the properties of a WheeledVehicle.\n\n"
   "@ingroup Vehicles\n"
);

// HH: Modified for Tribes 2 compatibility.
// Retail's defaults (0x611c60). It never sets springDamping; it is zeroed here
// so that a datablock which leaves it out does not run on whatever the heap
// held.
WheeledVehicleData::WheeledVehicleData()
{
   tireEmitter = 0;
   tireEmitterID = -1;
   maxWheelSpeed = 40;
   engineTorque = 1;
   brakeTorque = 1;
   tireFriction = 0.3f;
   tireRestitution = 1;
   tireRadius = 0.6f;
   tireLateralForce = 1000;
   tireLateralDamping = 100;
   tireLateralRelaxation = 1;
   tireLongitudinalForce = 1000;
   tireLongitudinalDamping = 100;
   tireLongitudinalRelaxation = 1;
   staticLoadScale = 1;
   springForce = 0.6f;
   springDamping = 0;
   antiSwayForce = 1;
   antiRockForce = 0;
   stabilizerForce = 0;
   gyroForce = 0;
   gyroDamping = 0;
   wheelCount = 0;
   tailLightSequence = -1;

   for (S32 i = 0; i < MaxSounds; i++) {
      sound[i] = 0;
      soundID[i] = -1;
   }
}


//----------------------------------------------------------------------------
/** Load the vehicle shape
   Loads and extracts information from the vehicle shape.

   Wheel Nodes
      ground#        The tire's foot

   Wheel Sequences
      spring#        Wheel spring motion: time 0 = wheel fully extended,
                     time 1 = fully compressed
      wheel#         Wheel spin
      turn#          Wheel steering: time 0 = full right, 0.5 = center

   Other Sequences
      taillight      Brake light, time 0 = off, 1 = braking

   A wheel needs its ground node and its spring sequence; the others are
   optional.
*/
bool WheeledVehicleData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;

   // A temporary shape instance is created so that we can
   // animate the shape and extract wheel information.
   TSShapeInstance* si = new TSShapeInstance(mShape, false);

   // HH: Modified for Tribes 2 compatibility.
   // Retail (0x611ea0) resolves these on server and client alike, and
   // silently.
   for (S32 i = 0; i < MaxSounds; i++)
      if (!sound[i] && soundID[i] != -1)
         Sim::findObject(soundID[i], sound[i]);

   if (!tireEmitter && tireEmitterID != -1)
      Sim::findObject(tireEmitterID, tireEmitter);

   // HH: Modified for Tribes 2 compatibility.
   // Retail's wheels (0x611f42): each has a ground%d node, whose place at
   // either end of its spring%d sequence gives the spring's travel.
   TSThread* thread = si->addThread();
   Wheel* wp = wheel;
   char buff[10];
   for (S32 i = 0; i < MaxWheels; i++) {
      dSprintf(buff,sizeof(buff),"ground%d",i);
      wp->groundNode = mShape->findNode(buff);
      dSprintf(buff,sizeof(buff),"spring%d",i);
      wp->springSequence = mShape->findSequence(buff);
      if (wp->springSequence == -1 || wp->groundNode == -1)
         continue;

      Point3F start;
      si->setSequence(thread,wp->springSequence,0);
      si->animate();
      si->mNodeTransforms[wp->groundNode].getColumn(3, &start);
      si->setPos(thread,1);
      si->animate();
      si->mNodeTransforms[wp->groundNode].getColumn(3, &wp->pos);

      wp->hubPos = wp->pos;
      wp->hubPos.z += tireRadius;

      // A wheel across from an earlier one takes its travel; any other
      // travels straight up or down to where its spring starts.
      if (!mirrorWheel(wp))
         wp->springTravel.set(0, 0, start.z - wp->pos.z);

      dSprintf(buff,sizeof(buff),"turn%d",i);
      wp->turnSequence = mShape->findSequence(buff);
      dSprintf(buff,sizeof(buff),"wheel%d",i);
      wp->wheelSequence = mShape->findSequence(buff);
      wp->steers = wp->turnSequence != -1;
      wp++;
   }
   wheelCount = wp - wheel;

   tailLightSequence = mShape->findSequence("taillight");

   // Extract collision planes from shape collision detail level
   if (collisionDetails[0] != -1) {
      MatrixF imat(1);
      SphereF sphere;
      sphere.center = mShape->center;
      sphere.radius = mShape->radius;
      PlaneExtractorPolyList polyList;
      polyList.mPlaneList = &rigidBody.mPlaneList;
      polyList.setTransform(&imat, Point3F(1,1,1));
      si->buildPolyList(&polyList,collisionDetails[0]);
   }

   delete si;
   return true;
}


//----------------------------------------------------------------------------
/** Find a matching lateral wheel
   Looks for a matching wheeling mirrored along the Y axis, within some
   tolerance (current 0.5m), if one is found, the two wheels are lined up.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x612330) lines up the spring's travel too.
bool WheeledVehicleData::mirrorWheel(Wheel* we)
{
   we->opposite = -1;
   for (Wheel* wp = wheel; wp != we; wp++)
      if (mFabs(wp->pos.y - we->pos.y) < 0.5) 
      {
         we->pos.x = -wp->pos.x;
         we->pos.y = wp->pos.y;
         we->pos.z = wp->pos.z;
         we->springTravel = wp->springTravel;
         we->opposite = wp - wheel;
         wp->opposite = we - wheel;
         return true;
      }
   return false;
}


//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// Retail's fields (0x612400), misspellings and all: a script's squealSound,
// brakeTorque or tireLongitudinalRelaxation is a dynamic field there.
void WheeledVehicleData::initPersistFields()
{
   addField( "jetSound", TYPEID< SFXProfile >(), Offset(sound[JetSound], WheeledVehicleData),
      "Looping sound played when the vehicle is jetting." );
   addField( "engineSound", TYPEID< SFXProfile >(), Offset(sound[EngineSound], WheeledVehicleData),
      "Looping engine sound." );
   addField("squeelSound", TYPEID< SFXProfile >(), Offset(sound[SquealSound], WheeledVehicleData),
      "Looping sound played while the wheels are slipping." );
   addField("WheelImpactSound", TYPEID< SFXProfile >(), Offset(sound[WheelImpactSound], WheeledVehicleData),
      "Sound played when the wheels impact the ground.\nCurrently unused." );

   addField("tireRadius", TypeF32, Offset(tireRadius, WheeledVehicleData));
   addField("tireFriction", TypeF32, Offset(tireFriction, WheeledVehicleData));
   addField("tireRestitution", TypeF32, Offset(tireRestitution, WheeledVehicleData));
   addField("tireLateralForce", TypeF32, Offset(tireLateralForce, WheeledVehicleData));
   addField("tireLateralDamping", TypeF32, Offset(tireLateralDamping, WheeledVehicleData));
   addField("tireLateralRelaxation", TypeF32, Offset(tireLateralRelaxation, WheeledVehicleData));
   addField("tireLongitudinalForce", TypeF32, Offset(tireLongitudinalForce, WheeledVehicleData));
   addField("tireLongitudinalDamping", TypeF32, Offset(tireLongitudinalDamping, WheeledVehicleData));
   addField("tireLogitudinalRelaxation", TypeF32, Offset(tireLongitudinalRelaxation, WheeledVehicleData));

   addField("tireEmitter",TYPEID< ParticleEmitterData >(), Offset(tireEmitter, WheeledVehicleData),
      "ParticleEmitterData datablock used to generate particles from each wheel "
      "when the vehicle is moving and the wheel is in contact with the ground.");
   addField("maxWheelSpeed", TypeF32, Offset(maxWheelSpeed, WheeledVehicleData),
      "@brief Maximum linear velocity of each wheel.\n\n"
      "This caps the maximum speed of the vehicle." );
   addField("engineTorque", TypeF32, Offset(engineTorque, WheeledVehicleData),
      "@brief Torque available from the engine at 100% throttle.\n\n"
      "This controls vehicle acceleration. ie. how fast it will reach maximum speed." );
   addField("breakTorque", TypeF32, Offset(brakeTorque, WheeledVehicleData),
      "@brief Torque applied when braking.\n\n"
      "This controls how fast the vehicle will stop when the brakes are applied." );
   addField("staticLoadScale", TypeF32, Offset(staticLoadScale, WheeledVehicleData));
   addField("springForce", TypeF32, Offset(springForce, WheeledVehicleData));
   addField("springDamping", TypeF32, Offset(springDamping, WheeledVehicleData));
   addField("antiSwayForce", TypeF32, Offset(antiSwayForce, WheeledVehicleData));
   addField("antiRockForce", TypeF32, Offset(antiRockForce, WheeledVehicleData));
   addField("stabilizerForce", TypeF32, Offset(stabilizerForce, WheeledVehicleData));
   addField("gyroForce", TypeF32, Offset(gyroForce, WheeledVehicleData));
   addField("gyroDamping", TypeF32, Offset(gyroDamping, WheeledVehicleData));

   Parent::initPersistFields();
}

// HH: Added for Tribes 2 compatibility.
// The fields WheeledVehicleData::unpackData (0x6128c0) reads after
// VehicleData's.
bool WheeledVehicleData::unpackData(BitStream *stream, U32 classId)
{
   if (!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   if (isUnpackedInPlace() ||
       !T2Layout::lastDataBlockField(T2Layout::T2DbTireFriction, &value))
      return true;

   static const struct { U32 field; F32 WheeledVehicleData::*member; } sFields[] =
   {
      { T2Layout::T2DbTireFriction,               &WheeledVehicleData::tireFriction               },
      { T2Layout::T2DbTireRestitution,            &WheeledVehicleData::tireRestitution            },
      { T2Layout::T2DbTireRadius,                 &WheeledVehicleData::tireRadius                 },
      { T2Layout::T2DbTireLateralForce,           &WheeledVehicleData::tireLateralForce           },
      { T2Layout::T2DbTireLateralDamping,         &WheeledVehicleData::tireLateralDamping         },
      { T2Layout::T2DbTireLateralRelaxation,      &WheeledVehicleData::tireLateralRelaxation      },
      { T2Layout::T2DbTireLongitudinalForce,      &WheeledVehicleData::tireLongitudinalForce      },
      { T2Layout::T2DbTireLongitudinalDamping,    &WheeledVehicleData::tireLongitudinalDamping    },
      { T2Layout::T2DbTireLongitudinalRelaxation, &WheeledVehicleData::tireLongitudinalRelaxation },
      { T2Layout::T2DbSpringForce,                &WheeledVehicleData::springForce                },
      { T2Layout::T2DbSpringDamping,              &WheeledVehicleData::springDamping              },
      { T2Layout::T2DbAntiSwayForce,              &WheeledVehicleData::antiSwayForce              },
      { T2Layout::T2DbAntiRockForce,              &WheeledVehicleData::antiRockForce              },
      { T2Layout::T2DbMaxWheelSpeed,              &WheeledVehicleData::maxWheelSpeed              },
      { T2Layout::T2DbEngineTorque,               &WheeledVehicleData::engineTorque               },
      { T2Layout::T2DbBrakeTorque,                &WheeledVehicleData::brakeTorque                },
      { T2Layout::T2DbStaticLoadScale,            &WheeledVehicleData::staticLoadScale            },
      { T2Layout::T2DbStabilizerForce,            &WheeledVehicleData::stabilizerForce            },
      { T2Layout::T2DbGyroForce,                  &WheeledVehicleData::gyroForce                  },
      { T2Layout::T2DbGyroDamping,                &WheeledVehicleData::gyroDamping                },
   };

   for (U32 i = 0; i < sizeof(sFields) / sizeof(sFields[0]); i++)
      T2Layout::lastDataBlockField(sFields[i].field, &(this->*sFields[i].member));

   // Retail keeps the ids, and preload looks them up.
   tireEmitterID = sentDataBlockId(T2Layout::T2DbTireEmitter);
   for (U32 i = 0; i < MaxSounds; i++)
      soundID[i] = sentDataBlockId(T2Layout::T2DbVehicleSound + i);

   return true;
}


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
// Wheeled Vehicle Class
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(WheeledVehicle);

ConsoleDocClass( WheeledVehicle,
   "@brief A wheeled vehicle.\n"
   "@ingroup Vehicles\n"
);

WheeledVehicle::WheeledVehicle()
{
   mDataBlock = 0;
   mBraking = false;
   mWheelContact = false;
   mJetSound = NULL;
   mEngineSound = NULL;
   mSquealSound = NULL;
   mTailLightThread = 0;

   for (S32 i = 0; i < WheeledVehicleData::MaxWheels; i++) {
      Wheel& wheel = mWheel[i];
      wheel.data = NULL;
      wheel.extension = 1;
      wheel.avel = wheel.apos = wheel.Dy = wheel.Dx = 0;
      wheel.surface.contact = false;
      wheel.surface.object = NULL;
      wheel.turnThread = wheel.springThread = wheel.wheelThread = 0;
      wheel.torqueScale = wheel.slip = 0;
   }
}

WheeledVehicle::~WheeledVehicle()
{
}

void WheeledVehicle::initPersistFields()
{
   Parent::initPersistFields();
}


//----------------------------------------------------------------------------

bool WheeledVehicle::onAdd()
{
   if(!Parent::onAdd())
      return false;

   addToScene();
   if (isServerObject())
      scriptOnAdd();
   return true;
}

void WheeledVehicle::onRemove()
{
   // Delete the wheel resources
   if (mDataBlock != NULL)  {
      Wheel* wend = &mWheel[mDataBlock->wheelCount];
      for (Wheel* wheel = mWheel; wheel < wend; wheel++)
         if (!wheel->emitter.isNull())
            wheel->emitter->deleteWhenEmpty();
   }

   // Stop the sounds
   SFX_DELETE( mJetSound );
   SFX_DELETE( mEngineSound );
   SFX_DELETE( mSquealSound );

   //
   scriptOnRemove();
   removeFromScene();
   Parent::onRemove();
}


//----------------------------------------------------------------------------

bool WheeledVehicle::onNewDataBlock(GameBaseData* dptr, bool reload)
{
   // Delete any existing wheel resources if we're switching
   // datablocks.
   if (mDataBlock) 
   {
      Wheel* wend = &mWheel[mDataBlock->wheelCount];
      for (Wheel* wheel = mWheel; wheel < wend; wheel++) 
      {
         if (!wheel->emitter.isNull()) 
         {
            wheel->emitter->deleteWhenEmpty();
            wheel->emitter = 0;
         }
      }
   }

   // Load up the new datablock
   mDataBlock = dynamic_cast<WheeledVehicleData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr,reload))
      return false;

   // HH: Modified for Tribes 2 compatibility.
   // Retail's (0x612f40) leaves the body alone: its inertia stays the
   // identity Vehicle::onAdd gives it, even when the datablock changes.

   // Initialize the wheels...
   for (U32 i = 0; i < mDataBlock->wheelCount; i++) 
   {
      Wheel* wheel = &mWheel[i];
      wheel->data = &mDataBlock->wheel[i];

      wheel->surface.contact = false;
      wheel->surface.object  = NULL;
      wheel->avel = 0;
      wheel->apos = 0;
      wheel->extension = 1;
      wheel->slip = 0;

      // HH: Modified for Tribes 2 compatibility.
      // A wheel steers, rides its spring and spins by its own sequences,
      // each from time 0 (0x612fed).
      wheel->turnThread = 0;
      if (wheel->data->turnSequence != -1) {
         wheel->turnThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(wheel->turnThread,wheel->data->turnSequence,0);
      }
      wheel->springThread = 0;
      if (wheel->data->springSequence != -1) {
         wheel->springThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(wheel->springThread,wheel->data->springSequence,0);
      }
      wheel->wheelThread = 0;
      if (wheel->data->wheelSequence != -1) {
         wheel->wheelThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(wheel->wheelThread,wheel->data->wheelSequence,0);
      }

      // Each wheel get's it's own particle emitter
      wheel->emitter = 0;
      if( mDataBlock->tireEmitter && isGhost() )
      {
         wheel->emitter = new ParticleEmitter;
         wheel->emitter->onNewDataBlock( mDataBlock->tireEmitter, false );
         wheel->emitter->registerObject();
      }
   }

   // Tail light sequence
   if (mDataBlock->tailLightSequence != -1) {
      mTailLightThread = mShapeInstance->addThread();
      mShapeInstance->setSequence(mTailLightThread,mDataBlock->tailLightSequence,0);
   }
   else
      mTailLightThread = 0;

   if (isGhost()) 
   {
      // Create the sounds ahead of time.  This reduces runtime
      // costs and makes the system easier to understand.

      SFX_DELETE( mEngineSound );
      SFX_DELETE( mSquealSound );
      SFX_DELETE( mJetSound );

      // HH: Modified for Tribes 2 compatibility.
      // Retail (0x6131ba) starts the engine here and keeps it running; the
      // squeal and the jet start and stop as they are needed.
      if ( mDataBlock->sound[WheeledVehicleData::EngineSound] )
      {
         mEngineSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::EngineSound], &getTransform() );
         if ( mEngineSound )
            mEngineSound->play();
      }
   }

   scriptOnNewDataBlock();
   return true;
}


//----------------------------------------------------------------------------

S32 WheeledVehicle::getWheelCount()
{
   // HH: Modified for Tribes 2 compatibility.
   return mDataBlock? mDataBlock->wheelCount: 0;
}

//----------------------------------------------------------------------------

void WheeledVehicle::processTick(const Move* move)
{
   Parent::processTick(move);
}

// HH: Added for Tribes 2 compatibility.
// The wheels follow the body on each warp tick (0x6132f0).
void WheeledVehicle::onWarpTick()
{
   extendWheels();
}

void WheeledVehicle::updateMove(const Move* move)
{
   Parent::updateMove(move);

   // HH: Modified for Tribes 2 compatibility.
   // Retail (0x613b44) brakes while the throttle is against the way the
   // wheels turn, not on a trigger, and lets go once it no longer is or the
   // wheels have stopped.
   F32 avel = 0;
   for (U32 i = 0; i < mDataBlock->wheelCount; i++)
      avel += mWheel[i].avel;
   if (mFabs(avel * mDataBlock->tireRadius) < 0.02f)
      avel = 0;

   if (mBraking) {
      if (avel == 0 || mThrottle == 0 ||
          (mThrottle > 0 && avel > 0) || (mThrottle < 0 && avel < 0))
         mBraking = false;
   }
   else if ((mThrottle > 0 && avel < 0) || (mThrottle < 0 && avel > 0))
      mBraking = true;

   // Set the tail brake light thread direction based on the brake state.
   if (mTailLightThread)
      mShapeInstance->setTimeScale(mTailLightThread, mBraking? 1.0f : -1.0f);

   // HH: Added for Tribes 2 compatibility.
   // The wheels are extended here, before the forces are summed, and a
   // vehicle with a wheel on something has no pitch steering.
   extendWheels();
   if (mWheelContact)
      mSteering.y = 0;
}


//----------------------------------------------------------------------------

void WheeledVehicle::advanceTime(F32 dt)
{
   PROFILE_SCOPE( WheeledVehicle_AdvanceTime );

   Parent::advanceTime(dt);

   // Animate the tail light. The direction of the thread is
   // set based on vehicle braking.
   if (mTailLightThread)
      mShapeInstance->advanceTime(dt,mTailLightThread);

   // HH: Modified for Tribes 2 compatibility.
   // Retail spins no wheel and plays no wheel sound for a frozen vehicle
   // (0x61332f).
   if (!mFrozen) {
      // Update wheel angular position and slip, this is a client visual
      // feature only, it has no affect on the physics.
      F32 slipTotal = 0;
      F32 torqueTotal = 0;

      Wheel* wend = &mWheel[mDataBlock->wheelCount];
      for (Wheel* wheel = mWheel; wheel < wend; wheel++) {
         // Update angular position
         wheel->apos += (wheel->avel * dt) / M_2PI;
         wheel->apos -= mFloor(wheel->apos);
         if (wheel->apos < 0)
            wheel->apos = 1 - wheel->apos;

         // Keep track of largest slip
         slipTotal += wheel->slip;
         torqueTotal += wheel->torqueScale;

         updateWheelParticles(wheel, dt);
      }

      // Update the sounds based on wheel slip and torque output, shared
      // over the wheels Tribes 2 finds.
      if (mDataBlock->wheelCount) {
         updateSquealSound(slipTotal / mDataBlock->wheelCount);
         updateEngineSound(sIdleEngineVolume + (1 - sIdleEngineVolume) *
            (1 - (torqueTotal / mDataBlock->wheelCount)));
      }
      updateJetSound();
   }

   updateWheelThreads();
}


//----------------------------------------------------------------------------
/** Update the rigid body forces on the vehicle
   This method calculates the forces acting on the body, including gravity,
   suspension & tire forces.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x613db0). The body's mass is 1 (Vehicle::onAdd), so what it sums
// are accelerations: the drag, jet and tire forces are divided by ShapeBase's
// mass, the springs by four fifths of it, and gravity is added as it is.
// Every arm is measured from the body's origin, and the wheels' forces turn
// the body about it from where each wheel's foot is fully compressed.
void WheeledVehicle::updateForces(F32 dt)
{
   PROFILE_SCOPE( WheeledVehicle_UpdateForces );

   const U32 wheelCount = mDataBlock->wheelCount;
   const F32 tireRadius = mDataBlock->tireRadius;

   // Get the current matrix and extact vectors
   MatrixF currMatrix;
   mRigid.getTransform(&currMatrix);

   Point3F bx,by,bz,origin;
   currMatrix.getColumn(0,&bx);
   currMatrix.getColumn(1,&by);
   currMatrix.getColumn(2,&bz);
   currMatrix.getColumn(3,&origin);

   const F32 springScale = 1 / (mMass * 0.8f);
   const F32 maxAvel = mDataBlock->maxWheelSpeed / tireRadius;
   const F32 aMomentum = wheelCount ? mMass / wheelCount : mMass;

   // Drag, and the anti-rock force damps the spin
   Point3F force  = -(mRigid.linVelocity * mDataBlock->minDrag) * mOneOverMass;
   Point3F torque = -(mRigid.angMomentum * mDataBlock->antiRockForce) * mOneOverMass;

   // Steering angles from current steering wheel position
   F32 quadraticSteering = -(mSteering.x * mFabs(mSteering.x));
   F32 cosSteering,sinSteering;
   mSinCos(quadraticSteering, sinSteering, cosSteering);

   // The vehicle's weight, shared over the wheels on something, is what
   // limits each tire's grip.
   U32 contactCount = 0;
   for (U32 i = 0; i < wheelCount; i++)
      if (mWheel[i].surface.contact)
         contactCount++;
   F32 staticLoad = 0;
   if (contactCount)
      staticLoad = -sWheeledVehicleGravity * mMass * mDataBlock->staticLoadScale / contactCount;
   const F32 staticLoadSq = staticLoad * staticLoad;

   // Calculate Engine and brake torque values used later by in
   // wheel calculations. With the throttle off, the engine brakes with
   // its own torque.
   F32 engineTorque,brakeVel;
   if (mBraking) 
   {
      brakeVel = (mFabs(mThrottle) * mDataBlock->brakeTorque / aMomentum) * dt;
      engineTorque = 0;
   }
   else if (mThrottle)
   {
      engineTorque = mDataBlock->engineTorque * mThrottle;
      brakeVel = 0;
      // Double the engineTorque to help out the jets
      if (mThrottle > 0 && mJetting)
         engineTorque += mDataBlock->engineTorque;
   }
   else
   {
      brakeVel = (mDataBlock->engineTorque / aMomentum) * dt;
      engineTorque = 0;
   }

   // Gravity and the jet
   Point3F accel(0, 0, sWheeledVehicleGravity);
   if (mJetting)
      accel += by * (mDataBlock->jetForce * mOneOverMass);

   Point3F wheelForce[WheeledVehicleData::MaxWheels];
   for (U32 i = 0; i < wheelCount; i++)  
   {
      Wheel* wheel = &mWheel[i];
      wheelForce[i].set(0, 0, 0);

      if (!wheel->surface.contact) 
      {
         // Wheel not in contact with the ground: it spins on as it was.
         wheel->torqueScale = 0;
         wheel->slip = 0;

         // Relax the tire deformation
         wheel->Dy += (-mDataBlock->tireLongitudinalRelaxation *
                       mFabs(wheel->avel) * wheel->Dy) * dt;
         wheel->Dx += (-mDataBlock->tireLateralRelaxation *
                       mFabs(wheel->avel) * wheel->Dx) * dt;
         continue;
      }

      // First, let's compute the wheel's position, and worldspace velocity
      Point3F pos, r, localVel;
      currMatrix.mulP(wheel->data->pos, &pos);
      r = pos - origin;
      mRigid.getVelocity(r, &localVel);

      // Spring force & damping
      F32 spring = (1 - wheel->extension) * mDataBlock->springForce;
      F32 damping = -mDot(bz, localVel) * mDataBlock->springDamping;
      if (damping < 0)
         damping = 0;

      // Anti-sway force based on difference in suspension extension
      F32 antiSway = 0;
      if (wheel->data->opposite != -1)
      {
         Wheel* oppositeWheel = &mWheel[wheel->data->opposite];
         if (oppositeWheel->surface.contact)
            antiSway = (oppositeWheel->extension - wheel->extension) *
               mDataBlock->antiSwayForce;
         if (antiSway < 0)
            antiSway = 0;
      }

      // Spring forces act straight up
      wheelForce[i] += bz * ((spring + damping + antiSway) * springScale);

      // Tire direction vectors perpendicular to surface normal
      Point3F wheelXVec = bx;
      if (wheel->data->steers)
         wheelXVec = bx * cosSteering + by * sinSteering;
      Point3F tireX, tireY;
      mCross(wheel->surface.normal, wheelXVec, &tireY);
      tireY.normalize();
      mCross(tireY, wheel->surface.normal, &tireX);
      tireX.normalize();

      // Velocity of tire at the surface contact
      Point3F wheelContact, wheelVelocity;
      wheelContact = wheel->surface.pos - origin;
      mRigid.getVelocity(wheelContact, &wheelVelocity);

      F32 xVelocity = mDot(tireX, wheelVelocity);
      F32 yVelocity = mDot(tireY, wheelVelocity);

      // Tires act as springs and generate lateral and longitudinal
      // forces to move the vehicle. These distortion/spring forces
      // are what convert wheel angular velocity into forces that
      // act on the rigid body.

      // Longitudinal tire deformation force
      F32 ddy = (wheel->avel * tireRadius - yVelocity) -
         mDataBlock->tireLongitudinalRelaxation *
         mFabs(wheel->avel) * wheel->Dy;
      wheel->Dy += ddy * dt;
      F32 Fy = (mDataBlock->tireLongitudinalDamping * ddy +
         mDataBlock->tireLongitudinalForce * wheel->Dy);

      // Lateral tire deformation force
      F32 ddx = xVelocity - mDataBlock->tireLateralRelaxation *
         mFabs(wheel->avel) * wheel->Dx;
      wheel->Dx += ddx * dt;
      F32 Fx = -(mDataBlock->tireLateralDamping * ddx +
         mDataBlock->tireLateralForce * wheel->Dx);

      // Adjust tire forces based on friction. A surface the wheel is not
      // above gives no grip at all; on any other, retail's limit squares
      // the friction and the slope on both sides, so it is the static
      // load alone.
      F32 nz = wheel->surface.normal.z;
      if (nz > 0)
      {
         F32 Fn = staticLoad * mDataBlock->tireFriction * nz;
         Fn *= Fn;
         F32 limit = Fn * staticLoadSq;
         F32 Fw = (Fy * Fy + Fx * Fx) * Fn;
         if (Fw > limit) 
         {
            F32 K = mSqrt(limit / Fw);
            Fy *= K;
            Fx *= K;
            wheel->Dy *= K;
            wheel->Dx *= K;
         }
      }
      else
         Fx = Fy = 0;

      // Tire forces act through the tire direction vectors parallel
      // to the surface.
      wheelForce[i] += tireY * (Fy * mOneOverMass) + tireX * (Fx * mOneOverMass);

      // Adjust the wheel's angular velocity based on engine torque
      // and tire deformation forces.
      F32 absAvel = mFabs(wheel->avel);
      wheel->torqueScale = (absAvel > maxAvel) ? 0 : 1 - (absAvel / maxAvel);
      wheel->avel += (((wheel->torqueScale * engineTorque) - Fy *
         tireRadius) / aMomentum) * dt;

      // Adjust the wheel's angular velocity based on brake torque.
      // This is done after avel update to make sure we come to a
      // complete stop.
      if (brakeVel > mFabs(wheel->avel))
         wheel->avel = 0;
      else
         if (wheel->avel > 0)
            wheel->avel -= brakeVel;
         else
            wheel->avel += brakeVel;
   }

   // Each wheel's force turns the body from its compressed foot.
   Point3F wheelTorque(0, 0, 0);
   for (U32 i = 0; i < wheelCount; i++)
   {
      Point3F pos, t;
      currMatrix.mulP(mWheel[i].data->pos, &pos);
      mCross(pos - origin, wheelForce[i], &t);
      wheelTorque += t;
      accel += wheelForce[i];
   }

   // Container drag & buoyancy
   accel.z += -mBuoyancy * sWheeledVehicleGravity;
   accel -= mRigid.linVelocity * mDrag;
   wheelTorque -= mRigid.angMomentum * mDrag;

   mRigid.force = force + accel;
   mRigid.torque = torque + wheelTorque;

   // If we've added anything other than gravity, then we're no
   // longer at rest. Could test this a little more efficiently...
   if (mRigid.atRest && (mRigid.force.len() || mRigid.torque.len()))
      mRigid.atRest = false;
}


//----------------------------------------------------------------------------

// HH: Added for Tribes 2 compatibility.
// Container::buildCollisionList (0x58ea00), which Torque 3D no longer has:
// what the polyhedron meets on its way from start to end.
static void tireCollisionCallback(SceneObject* obj, void* key)
{
   SceneContainer::CallbackInfo* info = reinterpret_cast<SceneContainer::CallbackInfo*>(key);
   obj->buildPolyList(info->context, info->polyList, info->boundingBox, info->boundingSphere);
}

static bool buildTireCollisionList(SceneContainer* container,
                                   const Polyhedron& polyhedron,
                                   const Point3F& start, const Point3F& end,
                                   const VectorF& velocity, U32 mask,
                                   CollisionList* collisionList)
{
   VectorF vector = end - start;
   if (mFabs(vector.x) + mFabs(vector.y) + mFabs(vector.z) == 0)
      return false;

   Point3F min(1e10, 1e10, 1e10);
   Point3F max(-1e10, -1e10, -1e10);
   for (U32 i = 0; i < polyhedron.pointList.size(); i++)
   {
      min.setMin(polyhedron.pointList[i]);
      max.setMax(polyhedron.pointList[i]);
   }

   SceneContainer::CallbackInfo info;
   info.context = PLC_Collision;
   info.key = NULL;
   info.boundingBox.minExtents = info.boundingBox.maxExtents = Point3F(0, 0, 0);
   info.boundingBox.minExtents.setMin(vector);
   info.boundingBox.maxExtents.setMax(vector);
   info.boundingBox.minExtents += min;
   info.boundingBox.maxExtents += max;
   info.boundingSphere.center = (info.boundingBox.minExtents + info.boundingBox.maxExtents) * 0.5;
   info.boundingSphere.radius = (info.boundingBox.maxExtents - info.boundingSphere.center).len();

   static ExtrudedPolyList sPolyList;
   sPolyList.extrude(polyhedron, vector);
   sPolyList.setVelocity(velocity);
   sPolyList.setCollisionList(collisionList);
   info.polyList = &sPolyList;

   container->findObjects(info.boundingBox, mask, tireCollisionCallback, &info);
   sPolyList.adjustCollisionTime();
   return collisionList->getCount() != 0;
}

/** Extend the wheels
   The wheels are extended until they contact a surface. The extension
   is instantaneous.  The wheels are extended before force calculations and
   also on each warp tick.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x614e10) sweeps a box the size of the tire from one spring's
// travel above the compressed foot down to the extended one, and takes the
// face most against the travel. Where nothing is in the way, a ray from the
// hub to the compressed foot finds anything the tire is already inside.
void WheeledVehicle::extendWheels()
{
   PROFILE_SCOPE( WheeledVehicle_ExtendWheels );

   disableCollision();
   mWheelContact = false;

   MatrixF currMatrix;
   mRigid.getTransform(&currMatrix);

   const U32 mask = sCollisionMask & ~PlayerObjectType;
   const F32 radius = mDataBlock->tireRadius;
   const Box3F tireBox(Point3F(-radius * 0.5f, -radius, 0),
                       Point3F(radius * 0.5f, radius, radius * 2));
   static Polyhedron sTirePolyhedron;

   Wheel* wend = &mWheel[mDataBlock->wheelCount];
   for (Wheel* wheel = mWheel; wheel < wend; wheel++) 
   {
      wheel->extension = 1;

      Point3F sp, travel;
      currMatrix.mulP(wheel->data->pos, &sp);
      currMatrix.mulV(wheel->data->springTravel, &travel);
      const Point3F start = sp - travel * wheel->extension;
      const Point3F end = sp + travel * wheel->extension;

      MatrixF tireMat = currMatrix;
      tireMat.setPosition(start);
      sTirePolyhedron.buildBox(tireMat, tireBox, true);

      CollisionList collisions;
      if (buildTireCollisionList(mContainer, sTirePolyhedron, start, end,
                                 travel * 2, mask, &collisions)) 
      {
         // Half way is the compressed foot, the end the extended one.
         const F32 t = collisions.getTime();
         if (t > 0.5f)
            wheel->extension *= (t - 0.5f) * 2;
         else
            wheel->extension = 0;

         wheel->surface.contact = true;
         wheel->surface.pos     = sp + travel * wheel->extension;
         wheel->surface.object  = collisions[0].object;

         const Collision* best = &collisions[0];
         F32 bestDot = 1e30f;
         for (S32 c = 0; c < collisions.getCount(); c++)
         {
            const F32 d = mDot(collisions[c].normal, travel);
            if (d < bestDot)
            {
               bestDot = d;
               best = &collisions[c];
            }
         }
         wheel->surface.normal = best->normal;
         mWheelContact = true;
      }
      else 
      {
         Point3F hub;
         currMatrix.mulP(wheel->data->hubPos, &hub);

         RayInfo rInfo;
         if (mContainer->castRay(hub, sp, mask, &rInfo)) 
         {
            wheel->extension = 0;
            wheel->surface.contact = true;
            wheel->surface.normal  = rInfo.normal;
            wheel->surface.pos     = rInfo.point;
            wheel->surface.object  = rInfo.object;
            mWheelContact = true;
         }
         else 
         {
            wheel->surface.contact = false;
            wheel->surface.object  = NULL;
         }
      }
   }
   enableCollision();
}


//----------------------------------------------------------------------------
/** Update wheel steering and suspension threads.
   These animations are purely cosmetic and this method is only invoked
   on the client.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x6154f0): time 0 of the spring sequence hangs the wheel out and 1
// has it compressed, the turn sequence is full right at 0 and straight ahead
// at 0.5, and the spin sequence follows the wheel's angular position.
void WheeledVehicle::updateWheelThreads()
{
   Wheel* wend = &mWheel[mDataBlock->wheelCount];
   for (Wheel* wheel = mWheel; wheel < wend; wheel++) 
   {
      if (wheel->springThread)
         mShapeInstance->setPos(wheel->springThread,1 - getMin(wheel->extension, 1.0f));

      if (wheel->turnThread) {
         F32 t = (mSteering.x * mFabs(mSteering.x)) / mDataBlock->maxSteeringAngle;
         mShapeInstance->setPos(wheel->turnThread,0.5 - t * 0.5);
      }

      if (wheel->wheelThread)
         mShapeInstance->setPos(wheel->wheelThread,wheel->apos);
   }
}

//----------------------------------------------------------------------------
/** Update wheel particles effects
   These animations are purely cosmetic and this method is only invoked
   on the client.  Particles are emitted as long as the moving.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x613402) raises dust from a tire on terrain alone, half a metre
// above its foot, in the colours the scripts' property map gives the
// terrain's first material, and as much as the vehicle's speed says.
void WheeledVehicle::updateWheelParticles(Wheel* wheel, F32 dt)
{
   Point3F vel = Parent::getVelocity();
   F32 speed = vel.len();

   // Don't bother if we're not moving.
   if (speed <= 1.0f || wheel->emitter.isNull() || !wheel->surface.object ||
       !(wheel->surface.object->getTypeMask() & TerrainObjectType))
      return;

   if (const MaterialPropertyMap::Entry *e = groundDustProperties(wheel->surface.object))
   {
      ColorF colorList[ ParticleData::PDC_NUM_KEYS ];

      for( U32 x = 0; x < getMin( U32( MaterialPropertyMap::NumColors ), U32( ParticleData::PDC_NUM_KEYS ) ); ++ x )
         colorList[ x ] = e->color[ x ];
      for( U32 x = MaterialPropertyMap::NumColors; x < ParticleData::PDC_NUM_KEYS; ++ x )
         colorList[ x ].set( 1.0, 1.0, 1.0, 0.0 );

      wheel->emitter->setColors( colorList );
   }

   Point3F axis = vel;
   axis.normalize();
   wheel->emitter->emitParticles( wheel->surface.pos + Point3F(0, 0, 0.5f), true,
      axis, vel, (U32)(speed * (1 / 15.0) * 1000 * dt));
}


//----------------------------------------------------------------------------
/** Update engine sound
   This method is only invoked by clients.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x6155f0) sets the engine's volume, not its pitch.
void WheeledVehicle::updateEngineSound(F32 level)
{
   if ( !mEngineSound )
      return;

   mEngineSound->setTransform( getTransform() );
   mEngineSound->setVolume( level );
}


//----------------------------------------------------------------------------
/** Update wheel skid sound
   This method is only invoked by clients.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x615630) starts the squeal when it is loud enough and lets it go
// when it is not.
void WheeledVehicle::updateSquealSound(F32 level)
{
   if ( !mDataBlock->sound[WheeledVehicleData::SquealSound] )
      return;

   if ( level < sMinSquealVolume ) 
   {
      SFX_DELETE( mSquealSound );
      return;
   }

   if ( !mSquealSound )
   {
      mSquealSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::SquealSound], &getTransform() );
      if ( !mSquealSound )
         return;
      mSquealSound->play();
   }

   mSquealSound->setTransform( getTransform() );
   mSquealSound->setVolume( level );
}


//----------------------------------------------------------------------------
/** Update jet sound
   This method is only invoked by clients.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's (0x6156e0) starts the jet with the jetting and lets it go after.
void WheeledVehicle::updateJetSound()
{
   if ( !mDataBlock->sound[WheeledVehicleData::JetSound] )
      return;

   if ( !mJetting ) 
   {
      SFX_DELETE( mJetSound );
      return;
   }

   if ( !mJetSound )
   {
      mJetSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::JetSound], &getTransform() );
      if ( !mJetSound )
         return;
      mJetSound->play();
   }

   mJetSound->setTransform( getTransform() );
}


//----------------------------------------------------------------------------

U32 WheeledVehicle::getCollisionMask()
{
   return sCollisionMask;
}


//----------------------------------------------------------------------------
/** Build a collision polylist
   The polylist is filled with polygons representing the collision volume
   and the wheels.
*/
// HH: Modified for Tribes 2 compatibility.
// Retail's wheel boxes (0x613740) are a little bigger than the ones it
// sweeps, and stand on each tire's foot at its extension.
bool WheeledVehicle::buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
   PROFILE_SCOPE( WheeledVehicle_BuildPolyList );

   // Parent will take care of body collision.
   Parent::buildPolyList(context, polyList,box,sphere);

   // Add wheels as boxes.
   const F32 radius = mDataBlock->tireRadius;
   Box3F wbox;
   wbox.minExtents.x = -(wbox.maxExtents.x = radius * 0.5f + 0.02f);
   wbox.minExtents.y = -(wbox.maxExtents.y = radius + 0.02f);
   wbox.minExtents.z = 0;
   wbox.maxExtents.z = radius * 2;

   Wheel* wend = &mWheel[mDataBlock->wheelCount];
   for (Wheel* wheel = mWheel; wheel < wend; wheel++) {
      MatrixF mat = mObjToWorld;

      Point3F sp,vec;
      mObjToWorld.mulP(wheel->data->pos,&sp);
      mObjToWorld.mulV(wheel->data->springTravel,&vec);
      Point3F ep = sp + (vec * wheel->extension);
      mat.setColumn(3,ep);
      polyList->setTransform(&mat,Point3F(1,1,1));
      polyList->addBox(wbox);
   }
   return !polyList->isEmpty();
}

//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// WheeledVehicle::writePacketData (0x6157a0) and readPacketData (0x615840):
// three floats a wheel and no slipping flag.
void WheeledVehicle::writePacketData(GameConnection *connection, BitStream *stream)
{
   Parent::writePacketData(connection, stream);
   stream->writeFlag(mBraking);

   Wheel* wend = &mWheel[mDataBlock->wheelCount];
   for (Wheel* wheel = mWheel; wheel < wend; wheel++) 
   {
      stream->write(wheel->avel);
      stream->write(wheel->Dy);
      stream->write(wheel->Dx);
   }
}

void WheeledVehicle::readPacketData(GameConnection *connection, BitStream *stream)
{
   Parent::readPacketData(connection, stream);
   mBraking = stream->readFlag();

   Wheel* wend = &mWheel[mDataBlock->wheelCount];
   for (Wheel* wheel = mWheel; wheel < wend; wheel++) 
   {
      stream->read(&wheel->avel);
      stream->read(&wheel->Dy);
      stream->read(&wheel->Dx);
   }

   // Rigid state is transmitted by the parent...
   setPosition(mRigid.linPosition,mRigid.angPosition);
   mDelta.pos = mRigid.linPosition;
   mDelta.rot[1] = mRigid.angPosition;
}

// HH: Added for Tribes 2 compatibility.
static U32 wheelCountOf(U32 dataBlockId)
{
   WheeledVehicleData *data = NULL;
   return Sim::findObject(dataBlockId, data) && data ? data->wheelCount : 0;
}

void WheeledVehicle::getT2Subject(T2Layout::Subject *subject,
                                  NetConnection *conn) const
{
   Parent::getT2Subject(subject, conn);
   subject->wheelCount = wheelCountOf;
}

// HH: Added for Tribes 2 compatibility.
// WheeledVehicle::unpackUpdate (0x615a70) takes the brake and the wheels'
// spin and tire deflection as they come, so a vehicle someone else drives
// turns its wheels as the server's do. Its controller is sent neither: it has
// them from its own packet data. The datablock the body named, and the wheels
// its onNewDataBlock cleared, are already in place.
void WheeledVehicle::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   const T2Layout::Body::VehicleTail &tail = body.vehicle;
   if (tail.hasBraking)
      mBraking = tail.braking;

   if (!tail.hasWheels || !mDataBlock)
      return;

   const U32 count = getMin(tail.wheelCount, mDataBlock->wheelCount);
   for (U32 i = 0; i < count; i++)
   {
      dMemcpy(&mWheel[i].avel, &tail.wheels[i][0], sizeof(F32));
      dMemcpy(&mWheel[i].Dy,   &tail.wheels[i][1], sizeof(F32));
      dMemcpy(&mWheel[i].Dx,   &tail.wheels[i][2], sizeof(F32));
   }
}


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
// Console Methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

DefineEngineMethod( WheeledVehicle, getWheelCount, S32, (),,
   "@brief Get the number of wheels on this vehicle.\n"
   "@return the number of wheels (a ground node and a spring sequence each)\n\n" )
{
   return object->getWheelCount();
}
