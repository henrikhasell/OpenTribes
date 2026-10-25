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
   bool onAdd();
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(TurretImageData);
};

class Turret : public StaticShape
{
   typedef StaticShape Parent;

   /// What a turret is doing, which its controller is sent in three bits.
   enum State
   {
      Closed,
      Activating,
      Deactivating,
      Active,
      BarrelSwap,
   };

   TurretData *mDataBlock;

   U32 mState;
   F32 mActivation;              ///< 0 closed to 1 open
   F32 mHeading;                 ///< degrees about its axis, [0, 360)
   F32 mPitch;                   ///< degrees from it; 90 is level

   StringTableEntry mInitialBarrel; ///< what is in slot 0, by name
   F32 mSkill;                   ///< 1 aims true, less aims wide
   SimObjectPtr<ShapeBase> mTarget;
   U32 mTargetMS;                ///< since it last had a target
   U32 mThinkMS;                 ///< since it last looked for one
   bool mAutoFire;               ///< it looks for targets of its own
   F32 mCapacitor;
   F32 mCapacitorRate;
   SimObjectPtr<ShapeBase> mSwapEngineer; ///< waiting for it to close

   /// What is drawn between ticks: the last tick's values, and the step
   /// back to the one before it.
   F32 mRenderActivation;
   F32 mRenderHeading;
   F32 mRenderPitch;
   F32 mActivationDelta;
   F32 mHeadingDelta;
   F32 mPitchDelta;

   TSThread *mActivateThread;
   TSThread *mElevateThread;
   TSThread *mTurnThread;

   void destroyThreads();
   void createAimThreads();
   void destroyAimThreads();
   void updateActivateThread(F32 activation);
   void setAimThreads(F32 heading, F32 pitch);
   void snapToLastUpdate();

   F32 getRestingPitch() const;
   F32 getHeadingStep();
   F32 getPitchStep();
   F32 getActivationStep();

   void updateState(bool controlled);
   void activate();
   void deactivate();
   void aim(const Move *move, bool controlled);

   U32 getThinkMS();
   F32 getAttackRadius();
   void think();
   void aimAtTarget(Move *move);
   Point3F getSkilledAim(const Point3F &start, const Point3F &end) const;
   void replaceBarrel();

public:
   Turret();

   static void initPersistFields();
   bool onAdd();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   bool mountImage(ShapeBaseImageData *image, U32 imageSlot, bool loaded,
                   NetStringHandle &skinNameHandle);
   bool unmountImage(U32 imageSlot);
   void onT2BodyRead(const T2Layout::Body &body);
   F32 getCapacitorValue();
   void processTick(const Move *move);
   void interpolateTick(F32 delta);
   void writePacketData(GameConnection *conn, BitStream *stream);
   void readPacketData(GameConnection *conn, BitStream *stream);

   void setSkill(F32 skill);
   bool setTargetObject(ShapeBase *target);
   ShapeBase *getTargetObject() const { return mTarget; }
   bool isValidTarget(ShapeBase *target);
   bool initiateBarrelSwap(ShapeBase *engineer);
   void setAutoFire(bool autoFire);
   F32  getCapacitorLevel() const { return mCapacitor; }
   void setCapacitorLevel(F32 level) { mCapacitor = level; }
   void setCapacitorRechargeRate(F32 rate) { mCapacitorRate = rate; }

   DECLARE_CONOBJECT(Turret);
};

#endif
