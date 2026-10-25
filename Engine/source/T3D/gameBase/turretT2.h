// HH: Added for Tribes 2 compatibility.
#ifndef _TURRETT2_H_
#define _TURRETT2_H_

#ifndef _STATICSHAPE_H_
#include "T3D/staticShape.h"
#endif

class TSThread;

/// A Tribes 2 turret's base: the shape that opens, turns and elevates, and
/// carries its barrel as the image in slot 0.
struct TurretData : public StaticShapeData
{
   typedef StaticShapeData Parent;

   /// The axis a turret aims about, which is how it is mounted.
   enum PrimaryAxis
   {
      YAxis,
      RevYAxis,
      ZAxis,
      RevZAxis,
   };

   F32  thetaMin;                ///< degrees from straight up the barrel
   F32  thetaMax;                ///< can pitch between
   F32  thetaNull;               ///< where it rests; -1 is horizontal
   S32  primaryAxis;
   bool neverUpdateControl;      ///< it never aims: a deployed camera
   F32  maxCapacitorEnergy;
   F32  capacitorRechargeRate;

   S32  activateSequence;
   S32  elevateSequence;
   S32  turnSequence;

   TurretData();

   static void initPersistFields();
   bool preload(bool server, String &errorStr);
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(TurretData);
};

typedef TurretData::PrimaryAxis TurretPrimaryAxis;
DefineEnumType( TurretPrimaryAxis );

/// A turret's barrel, and how fast the turret aims and opens with it in
/// slot 0.
struct TurretImageData : public ShapeBaseImageData
{
   typedef ShapeBaseImageData Parent;

   S32  activationMS;
   S32  deactivateDelayMS;
   S32  thinkTimeMS;
   F32  degPerSecTheta;          ///< how fast it pitches
   F32  degPerSecPhi;            ///< how fast it turns
   F32  attackRadius;
   bool dontFireInsideDamageRadius;
   F32  damageRadius;
   bool useCapacitor;
   F32  yawVariance;
   F32  pitchVariance;

   TurretImageData();

   static void initPersistFields();
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(TurretImageData);
};

class Turret : public StaticShape
{
   typedef StaticShape Parent;

   TurretData *mDataBlock;

   F32 mActivation;              ///< 0 closed to 1 open
   F32 mHeading;                 ///< degrees about its axis, [0, 360)
   F32 mPitch;                   ///< degrees from it; 90 is level

   TSThread *mActivateThread;
   TSThread *mElevateThread;
   TSThread *mTurnThread;

   void destroyThreads();
   void updateThreads();

public:
   Turret();

   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void processTick(const Move *move);

   DECLARE_CONOBJECT(Turret);
};

#endif
