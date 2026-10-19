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

#ifndef _WHEELEDVEHICLE_H_
#define _WHEELEDVEHICLE_H_

#ifndef _VEHICLE_H_
#include "T3D/vehicles/vehicle.h"
#endif

#ifndef _CLIPPEDPOLYLIST_H_
#include "collision/clippedPolyList.h"
#endif

class ParticleEmitter;
class ParticleEmitterData;


//----------------------------------------------------------------------------

struct WheeledVehicleData: public VehicleData 
{
   typedef VehicleData Parent;

   enum Constants 
   {
      MaxWheels = 8,
      MaxWheelBits = 3
   };

   enum Sounds 
   {
      JetSound,
      EngineSound,
      SquealSound,
      WheelImpactSound,
      MaxSounds,
   };
   // HH: Modified for Tribes 2 compatibility.
   SFXProfile* sound[MaxSounds];
   S32 soundID[MaxSounds];

   ParticleEmitterData* tireEmitter;
   S32 tireEmitterID;               // HH: Added for Tribes 2 compatibility.

   // HH: Modified for Tribes 2 compatibility.
   // Tribes 2 has no tire or spring datablocks: the vehicle's own datablock
   // carries one tire and one spring for every wheel.
   F32 tireFriction;
   F32 tireRestitution;
   F32 tireRadius;
   F32 tireLateralForce;
   F32 tireLateralDamping;
   F32 tireLateralRelaxation;
   F32 tireLongitudinalForce;
   F32 tireLongitudinalDamping;
   F32 tireLongitudinalRelaxation;  ///< registered as tireLogitudinalRelaxation
   F32 staticLoadScale;
   F32 springForce;
   F32 springDamping;
   F32 antiSwayForce;
   F32 antiRockForce;
   F32 maxWheelSpeed;
   F32 engineTorque;
   F32 brakeTorque;                 ///< registered as breakTorque
   F32 stabilizerForce;
   F32 gyroForce;
   F32 gyroDamping;

   // HH: Modified for Tribes 2 compatibility.
   // What preload (0x611f30) finds of each wheel in the vehicle's own shape,
   // in the order of Tribes 2's 0x48-byte record.
   struct Wheel 
   {
      S32 steers;                ///< a turn%d sequence was found
      S32 opposite;              ///< the wheel across from this one, or -1
      F32 springForce;
      F32 springDamping;
      F32 spare;                 ///< 0.5, which retail's wheel code never reads
      Point3F hubPos;            ///< unmirrored pos, raised by tireRadius
      Point3F pos;               ///< ground%d at the end of spring%d
      Point3F springTravel;      ///< 0, 0 and ground%d's rise to its start
      S32 groundNode;
      S32 springSequence;
      S32 wheelSequence;
      S32 turnSequence;
   } wheel[MaxWheels];
   U32 wheelCount;
   ClippedPolyList rigidBody;    // Extracted from shape
   S32 tailLightSequence;        // HH: Modified for Tribes 2 compatibility.

   //
   WheeledVehicleData();
   DECLARE_CONOBJECT(WheeledVehicleData);
   static void initPersistFields();
   bool preload(bool, String &errorStr);
   bool mirrorWheel(Wheel* we);
   // HH: Added for Tribes 2 compatibility.
   bool unpackData(BitStream *stream, U32 classId) override;
};


//----------------------------------------------------------------------------

class WheeledVehicle: public Vehicle
{
   typedef Vehicle Parent;

   enum MaskBits 
   {
      WheelMask    = Parent::NextFreeMask << 0,
      NextFreeMask = Parent::NextFreeMask << 1
   };

   WheeledVehicleData* mDataBlock;

   bool mBraking;
   TSThread* mTailLightThread;
   SFXSource* mJetSound;
   SFXSource* mEngineSound;
   SFXSource* mSquealSound;

   struct Wheel 
   {
      WheeledVehicleData::Wheel* data;

      F32 extension;          // Spring extension (0-1)
      F32 avel;               // Angular velocity
      F32 apos;               // Anuglar position (client side only)
      F32 Dy,Dx;              // Current tire deformation

      struct Surface 
      {
         bool contact;        // Wheel is touching a surface
         Point3F normal;      // Surface normal
         BaseMatInstance* material; // Surface material
         Point3F pos;         // Point of contact
         SceneObject* object; // Object in contact with
      } surface;

      F32 steering;           // Wheel steering scale
      bool powered;           // Powered by engine
      bool slipping;          // Traction on last tick
      F32 torqueScale;        // Max torque % applied to wheel (0-1)
      F32 slip;               // Amount of wheel slip (0-1)
      SimObjectPtr<ParticleEmitter> emitter;
   };
   Wheel mWheel[WheeledVehicleData::MaxWheels];

   //
   bool onNewDataBlock( GameBaseData *dptr, bool reload );
   void processTick(const Move *move);
   void updateMove(const Move *move);
   void updateForces(F32 dt);

   // Client sounds
   void updateEngineSound(F32 level);
   void updateSquealSound(F32 level);
   void updateJetSound();

   virtual U32 getCollisionMask();

public:
   DECLARE_CONOBJECT(WheeledVehicle);
   static void initPersistFields();

   WheeledVehicle();
   ~WheeledVehicle();

   bool onAdd();
   void onRemove();
   void advanceTime(F32 dt);

   S32 getWheelCount();
   Wheel *getWheel(U32 index) {return &mWheel[index];}
   void setWheelSteering(S32 wheel,F32 steering);
   void setWheelPowered(S32 wheel,bool powered);

   void writePacketData(GameConnection * conn, BitStream *stream);
   void readPacketData(GameConnection * conn, BitStream *stream);

   // HH: Added for Tribes 2 compatibility.
   void getT2Subject(T2Layout::Subject *subject,
                     NetConnection *conn) const override;
};


#endif
