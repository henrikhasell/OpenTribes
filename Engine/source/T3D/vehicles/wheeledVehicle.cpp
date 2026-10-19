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
#include "core/stream/bitStream.h"
#include "core/dnet.h"
#include "T3D/gameBase/gameConnection.h"
#include "ts/tsShapeInstance.h"
#include "T3D/fx/particleEmitter.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxProfile.h"   // HH: Modified for Tribes 2 compatibility.
#include "sfx/sfxSource.h"
#include "sfx/sfxTypes.h"
#include "scene/sceneManager.h"
#include "core/resourceManager.h"
#include "materials/materialDefinition.h"
#include "materials/baseMatInstance.h"
#include "lighting/lightQuery.h"
#include "T3D/gameBase/gravityT2.h"   // HH: Added for Tribes 2 compatibility.
#include "T3D/gameBase/t2Layout.h"    // HH: Added for Tribes 2 compatibility.


// Collision masks are used to determin what type of objects the
// wheeled vehicle will collide with.
static U32 sClientCollisionMask =
      TerrainObjectType    | InteriorObjectType       |
      PlayerObjectType     | StaticShapeObjectType    |
      VehicleObjectType    | VehicleBlockerObjectType;

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
// Retail's constructor (0x611c60). It leaves springDamping as it finds it.
WheeledVehicleData::WheeledVehicleData()
{
   tireEmitter = 0;
   tireEmitterID = -1;
   tireFriction = 0.3f;
   tireRestitution = 1;
   tireRadius = 0.6f;
   tireLateralForce = 1000;
   tireLateralDamping = 100;
   tireLateralRelaxation = 1;
   tireLongitudinalForce = 1000;
   tireLongitudinalDamping = 100;
   tireLongitudinalRelaxation = 1;
   maxWheelSpeed = 40;
   engineTorque = 1;
   brakeTorque = 1;
   staticLoadScale = 1;
   springForce = 0.6f;
   springDamping = 0;
   antiSwayForce = 1;
   antiRockForce = 0;
   stabilizerForce = 0;
   gyroForce = 0;
   gyroDamping = 0;
   tailLightSequence = -1;
   wheelCount = 0;

   for (S32 i = 0; i < MaxSounds; i++)
   {
      sound[i] = 0;
      soundID[i] = -1;
   }
}


//----------------------------------------------------------------------------
// HH: Modified for Tribes 2 compatibility.
// Retail's preload (0x611e30). The wheels are the vehicle shape's own: each
// i under eight with a ground%d node and a spring%d sequence is one, placed
// where the node sits at either end of the sequence. Tribes 2 has no hub%d.
bool WheeledVehicleData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;

   // A temporary shape instance is created so that we can
   // animate the shape and extract wheel information.
   TSShapeInstance* si = new TSShapeInstance(mShape, false);

   // Retail resolves these on server and client alike, and silently.
   for (S32 i = 0; i < MaxSounds; i++)
      if (!sound[i] && soundID[i] != -1)
         Sim::findObject(soundID[i], sound[i]);

   if (!tireEmitter && tireEmitterID != -1)
      Sim::findObject(tireEmitterID, tireEmitter);

   // Extract wheel information from the shape
   TSThread* thread = si->addThread();
   Wheel* wp = wheel;
   char buff[16];
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

      // The hub is taken before mirroring, so a mirrored wheel keeps its own
      // x here and takes its partner's, negated, in pos.
      wp->hubPos.set(wp->pos.x, wp->pos.y, wp->pos.z + tireRadius);
      if (!mirrorWheel(wp))
         wp->springTravel.set(0, 0, start.z - wp->pos.z);

      dSprintf(buff,sizeof(buff),"turn%d",i);
      wp->turnSequence = mShape->findSequence(buff);
      dSprintf(buff,sizeof(buff),"wheel%d",i);
      wp->wheelSequence = mShape->findSequence(buff);
      wp->spare = 0.5f;
      wp->springForce = springForce;
      wp->springDamping = springDamping;
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
// Retail (0x612330) lines up the spring travel too.
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
// Retail's names (0x612400), misspellings and all: the scripts'
// tireLongitudinalRelaxation is only a dynamic field.
void WheeledVehicleData::initPersistFields()
{
   addField( "jetSound", TYPEID< SFXProfile >(), Offset(sound[JetSound], WheeledVehicleData),
      "Looping sound played when the vehicle is jetting." );
   addField( "engineSound", TYPEID< SFXProfile >(), Offset(sound[EngineSound], WheeledVehicleData),
      "@brief Looping engine sound.\n\n"
      "The pitch is dynamically adjusted based on the current engine RPM" );
   addField("squeelSound", TYPEID< SFXProfile >(), Offset(sound[SquealSound], WheeledVehicleData),
      "@brief Looping sound played while any of the wheels is slipping.\n\n"
      "The volume is dynamically adjusted based on how much the wheels are slipping." );
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
// VehicleData's. Retail keeps the sounds and emitter it had, which preload
// then leaves as they are.
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

   tireEmitterID = sentDataBlockId(T2Layout::T2DbTireEmitter);
   for (U32 i = 0; i < MaxSounds; i++)
      soundID[i] = sentDataBlockId(T2Layout::T2DbVehicleSound + i);

   return true;
}


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
   mJetSound = NULL;
   mEngineSound = NULL;
   mSquealSound = NULL;
   mTailLightThread = 0;

   for (S32 i = 0; i < WheeledVehicleData::MaxWheels; i++) {
      mWheel[i].Dy = mWheel[i].Dx = 0;
      mWheel[i].steering = 0;
      mWheel[i].powered = true;
      mWheel[i].slipping = false;
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
      for (Wheel* wheel = mWheel; wheel < wend; wheel++) {
         if (!wheel->emitter.isNull())
            wheel->emitter->deleteWhenEmpty();
      }
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

   // Set inertial tensor, default for the vehicle is sphere
   if (mDataBlock->massBox.x > 0 && mDataBlock->massBox.y > 0 && mDataBlock->massBox.z > 0)
      mRigid.setObjectInertia(mDataBlock->massBox);
   else
      mRigid.setObjectInertia(mObjBox.maxExtents - mObjBox.minExtents);

   // Initialize the wheels...
   for (S32 i = 0; i < mDataBlock->wheelCount; i++) 
   {
      Wheel* wheel = &mWheel[i];
      wheel->data = &mDataBlock->wheel[i];

      wheel->surface.contact = false;
      wheel->surface.object  = NULL;
      wheel->avel = 0;
      wheel->apos = 0;
      wheel->extension = 1;
      wheel->slip = 0;
      wheel->torqueScale = 0;       // HH: Added for Tribes 2 compatibility.

      wheel->emitter = 0;

      // Steering on the front tires by default
      if (wheel->data->pos.y > 0)
         wheel->steering = 1;

      // Each wheel get's it's own particle emitter
      if( mDataBlock->tireEmitter && isGhost() )
      {
         wheel->emitter = new ParticleEmitter;
         wheel->emitter->onNewDataBlock( mDataBlock->tireEmitter, false );
         wheel->emitter->registerObject();
      }
   }

   // Brake light sequence
   // HH: Modified for Tribes 2 compatibility.
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

      if ( mDataBlock->sound[WheeledVehicleData::EngineSound] )
         mEngineSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::EngineSound], &getTransform() );

      if ( mDataBlock->sound[WheeledVehicleData::SquealSound] )
         mSquealSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::SquealSound], &getTransform() );

      if ( mDataBlock->sound[WheeledVehicleData::JetSound] )
         mJetSound = SFX->createSource( mDataBlock->sound[WheeledVehicleData::JetSound], &getTransform() );
   }

   scriptOnNewDataBlock();
   return true;
}


//----------------------------------------------------------------------------

S32 WheeledVehicle::getWheelCount()
{
   // Return # of hubs defined on the car body
   return mDataBlock? mDataBlock->wheelCount: 0;
}

void WheeledVehicle::setWheelSteering(S32 wheel,F32 steering)
{
   AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels,"Wheel index out of bounds");
   mWheel[wheel].steering = mClampF(steering,-1,1);
   setMaskBits(WheelMask);
}

void WheeledVehicle::setWheelPowered(S32 wheel,bool powered)
{
   AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels,"Wheel index out of bounds");
   mWheel[wheel].powered = powered;
   setMaskBits(WheelMask);
}

//----------------------------------------------------------------------------

void WheeledVehicle::processTick(const Move* move)
{
   Parent::processTick(move);
}

void WheeledVehicle::updateMove(const Move* move)
{
   Parent::updateMove(move);

   // Brake on trigger
   mBraking = move->trigger[2];

   // Set the tail brake light thread direction based on the brake state.
   if (mTailLightThread)
      mShapeInstance->setTimeScale(mTailLightThread, mBraking? 1.0f : -1.0f);
}


//----------------------------------------------------------------------------

void WheeledVehicle::advanceTime(F32 dt)
{
   PROFILE_SCOPE( WheeledVehicle_AdvanceTime );

   Parent::advanceTime(dt);

   // Update wheel angular position and slip, this is a client visual
   // feature only, it has no affect on the physics.
   F32 slipTotal = 0;
   F32 torqueTotal = 0;

   // HH: Modified for Tribes 2 compatibility.
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
   }

   // Update the sounds based on wheel slip and torque output
   updateSquealSound(slipTotal / mDataBlock->wheelCount);
   updateEngineSound(sIdleEngineVolume + (1 - sIdleEngineVolume) *
      (1 - (torqueTotal / mDataBlock->wheelCount)));
   updateJetSound();

   // Animate the tail light. The direction of the thread is
   // set based on vehicle braking.
   if (mTailLightThread)
      mShapeInstance->advanceTime(dt,mTailLightThread);
}


//----------------------------------------------------------------------------
/** Update the rigid body forces on the vehicle
   This method calculates the forces acting on the body, including gravity,
   suspension & tire forces.
*/
// HH: Modified for Tribes 2 compatibility.
// Torque's tire and spring forces came from WheeledVehicleTire and
// WheeledVehicleSpring datablocks, which Tribes 2 has none of, so none are
// applied here.
void WheeledVehicle::updateForces(F32 dt)
{
   PROFILE_SCOPE( WheeledVehicle_UpdateForces );

   // Get the current matrix and extact vectors
   MatrixF currMatrix;
   mRigid.getTransform(&currMatrix);

   Point3F by;
   currMatrix.getColumn(1,&by);

   // Integrate forces, we'll do this ourselves here instead of
   // relying on the rigid class which does it during movement.
   mRigid.force.set(0, 0, 0);
   mRigid.torque.set(0, 0, 0);

   // Jet Force
   if (mJetting)
      mRigid.force += by * mDataBlock->jetForce;

   // Container drag & buoyancy
   mRigid.force  += Point3F(0, 0, -mBuoyancy * sWheeledVehicleGravity * mRigid.mass);
   mRigid.force  -= mRigid.linVelocity * mDrag;
   mRigid.torque -= mRigid.angMomentum * mDrag;

   // If we've added anything other than gravity, then we're no
   // longer at rest. Could test this a little more efficiently...
   if (mRigid.atRest && (mRigid.force.len() || mRigid.torque.len()))
      mRigid.atRest = false;

   // Gravity
   mRigid.force += Point3F(0, 0, sWheeledVehicleGravity * mRigid.mass);

   // Integrate and update velocity
   mRigid.linMomentum += mRigid.force * dt;
   mRigid.angMomentum += mRigid.torque * dt;
   mRigid.updateVelocity();

   // Since we've already done all the work, just need to clear this out.
   mRigid.force.set(0, 0, 0);
   mRigid.torque.set(0, 0, 0);

   // If we're still atRest, make sure we're not accumulating anything
   if (mRigid.atRest)
      mRigid.setAtRest();
}


//----------------------------------------------------------------------------
/** Update engine sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateEngineSound(F32 level)
{
   if ( !mEngineSound )
      return;

   if ( !mEngineSound->isPlaying() )
      mEngineSound->play();

   mEngineSound->setTransform( getTransform() );
   mEngineSound->setVelocity( getVelocity() );
   //mEngineSound->setVolume( level );

   // Adjust pitch
   F32 pitch = ((level-sIdleEngineVolume) * 1.3f);
   if (pitch < 0.4f)  
      pitch = 0.4f;

   mEngineSound->setPitch( pitch );
}


//----------------------------------------------------------------------------
/** Update wheel skid sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateSquealSound(F32 level)
{
   if ( !mSquealSound )
      return;

   if ( level < sMinSquealVolume ) 
   {
      mSquealSound->stop();
      return;
   }

   if ( !mSquealSound->isPlaying() )
      mSquealSound->play();

   mSquealSound->setTransform( getTransform() );
   mSquealSound->setVolume( level );
}


//----------------------------------------------------------------------------
/** Update jet sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateJetSound()
{
   if ( !mJetSound )
      return;

   if ( !mJetting ) 
   {
      mJetSound->stop();
      return;
   }

   if ( !mJetSound->isPlaying() )
      mJetSound->play();

   mJetSound->setTransform( getTransform() );
}


//----------------------------------------------------------------------------

U32 WheeledVehicle::getCollisionMask()
{
   return sClientCollisionMask;
}


//----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// WheeledVehicle::writePacketData (0x6157a0): Vehicle's, the brake, then each
// wheel's angular velocity and tire deflection. Retail sends no slipping flag.
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

// HH: Modified for Tribes 2 compatibility.
// WheeledVehicle::readPacketData (0x615840), the mirror of the write.
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
static U32 t2WheelCountOf(U32 dataBlockId)
{
   WheeledVehicleData *data = NULL;
   return Sim::findObject(dataBlockId, data) && data ? data->wheelCount : 0;
}

void WheeledVehicle::getT2Subject(T2Layout::Subject *subject,
                                  NetConnection *conn) const
{
   Parent::getT2Subject(subject, conn);
   subject->wheelCount = t2WheelCountOf;
}

// HH: Added for Tribes 2 compatibility.
// WheeledVehicle::unpackUpdate (0x615a70) keeps the brake and the wheels'
// words as they come, and its controller is sent neither: it has them from
// its own packet data. Retail stores them once a first body's datablock has
// been taken and its onNewDataBlock has cleared the wheels, so they are kept
// here after the parent's read has done the same.
bool WheeledVehicle::unpackUpdate(BitStream *stream, bool isInitial)
{
   if (!Parent::unpackUpdate(stream, isInitial))
      return false;

   const T2Layout::Body::VehicleTail &tail = T2Layout::lastBody().vehicle;
   if (tail.hasBraking)
      mBraking = tail.braking;

   if (!tail.hasWheels || !mDataBlock)
      return true;

   // The walk counted the wheels from the datablock this one has now.
   const U32 count = getMin(tail.wheelCount, mDataBlock->wheelCount);
   for (U32 i = 0; i < count; i++)
   {
      dMemcpy(&mWheel[i].avel, &tail.wheels[i][0], sizeof(F32));
      dMemcpy(&mWheel[i].Dy,   &tail.wheels[i][1], sizeof(F32));
      dMemcpy(&mWheel[i].Dx,   &tail.wheels[i][2], sizeof(F32));
   }
   return true;
}


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
// Console Methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

DefineEngineMethod( WheeledVehicle, setWheelSteering, bool, ( S32 wheel, F32 steering ),,
   "@brief Set how much the wheel is affected by steering.\n\n"
   "The steering factor controls how much the wheel is rotated by the vehicle "
   "steering. For example, most cars would have their front wheels set to 1.0, "
   "and their rear wheels set to 0 since only the front wheels should turn.\n\n"
   "Negative values will turn the wheel in the opposite direction to the steering "
   "angle.\n"
   "@param wheel index of the wheel to set (hub node #)\n"
   "@param steering steering factor from -1 (full inverse) to 1 (full)\n"
   "@return true if successful, false if failed\n\n" )
{
   if ( wheel >= 0 && wheel < object->getWheelCount() ) {
      object->setWheelSteering( wheel, steering );
      return true;
   }
   else
      Con::warnf("setWheelSteering: wheel index %d out of bounds, vehicle has %d hubs",
         wheel, object->getWheelCount());
   return false;
}

DefineEngineMethod( WheeledVehicle, setWheelPowered, bool, ( S32 wheel, bool powered ),,
   "@brief Set whether the wheel is powered (has torque applied from the engine).\n\n"
   "A rear wheel drive car for example would set the front wheels to false, "
   "and the rear wheels to true.\n"
   "@param wheel index of the wheel to set (hub node #)\n"
   "@param powered flag indicating whether to power the wheel or not\n"
   "@return true if successful, false if failed\n\n" )
{
   if ( wheel >= 0 && wheel < object->getWheelCount() ) {
      object->setWheelPowered( wheel, powered );
      return true;
   }
   else
      Con::warnf("setWheelPowered: wheel index %d out of bounds, vehicle has %d hubs",
         wheel, object->getWheelCount());
   return false;
}

DefineEngineMethod( WheeledVehicle, getWheelCount, S32, (),,
   "@brief Get the number of wheels on this vehicle.\n"
   "@return the number of wheels (equal to the number of hub nodes defined in the model)\n\n" )
{
   return object->getWheelCount();
}
