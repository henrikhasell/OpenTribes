// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/turretT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/moveManager.h"
#include "T3D/gameBase/t2Layout.h"
#include "ts/tsShapeInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(TurretData);

ConsoleDocClass( TurretData,
   "@brief A Tribes 2 turret's base.\n\n@internal" );

ImplementEnumType( TurretPrimaryAxis,
   "@brief The axis a Tribes 2 turret aims about.\n\n@internal" )
   { TurretData::YAxis,    "yaxis"    },
   { TurretData::RevYAxis, "revyaxis" },
   { TurretData::ZAxis,    "zaxis"    },
   { TurretData::RevZAxis, "revzaxis" },
EndImplementEnumType;

/// 0x00653d90.
TurretData::TurretData()
   : thetaMin(45.0f),
     thetaMax(135.0f),
     thetaNull(-1.0f),
     primaryAxis(ZAxis),
     neverUpdateControl(false),
     maxCapacitorEnergy(0.0f),
     capacitorRechargeRate(0.0f),
     activateSequence(-1),
     elevateSequence(-1),
     turnSequence(-1)
{
}

void TurretData::initPersistFields()
{
   addField("thetaMin",              TypeF32,  Offset(thetaMin,              TurretData));
   addField("thetaMax",              TypeF32,  Offset(thetaMax,              TurretData));
   addField("thetaNull",             TypeF32,  Offset(thetaNull,             TurretData));
   addField("primaryAxis",           TYPEID< TurretPrimaryAxis >(),
            Offset(primaryAxis, TurretData));
   addField("neverUpdateControl",    TypeBool, Offset(neverUpdateControl,    TurretData));
   addField("maxCapacitorEnergy",    TypeF32,  Offset(maxCapacitorEnergy,    TurretData));
   addField("capacitorRechargeRate", TypeF32,  Offset(capacitorRechargeRate, TurretData));

   Parent::initPersistFields();
}

/// 0x00653ed0: a turret base without the three sequences it aims with is
/// refused, and its pitch is held to what the shape can do.
bool TurretData::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(!mShape)
   {
      errorStr = String::ToString("TurretData(%s)::onAdd: must have a shape: "
                                  "%s not found", getName(), shapeName);
      return false;
   }

   activateSequence = mShape->findSequence("activate");
   elevateSequence = mShape->findSequence("elevate");
   turnSequence = mShape->findSequence("turn");
   if(activateSequence == -1 || elevateSequence == -1 || turnSequence == -1)
   {
      errorStr = String::ToString("TurretData(%s)::onAdd: shape %s does not "
                                  "have all sequences necessary (turn, "
                                  "elevate, activate)", getName(), shapeName);
      return false;
   }

   if(thetaMin < 0.0f || thetaMin > 90.0f)
   {
      Con::warnf("TurretData(%s): thetaMin must be in [0, 90]", getName());
      thetaMin = thetaMin < 0.0f ? 0.0f : 90.0f;
   }
   if(thetaMax < 90.0f || thetaMax > 180.0f)
   {
      Con::warnf("TurretData(%s): thetaMax must be in [90, 180]", getName());
      thetaMax = thetaMax < 90.0f ? 90.0f : 180.0f;
   }

   return true;
}

bool TurretData::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   if(isUnpackedInPlace())
      return true;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbThetaMin, &value))
      thetaMin = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbThetaMax, &value))
      thetaMax = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbThetaNull, &value))
      thetaNull = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbNeverUpdateControl, &word))
      neverUpdateControl = word != 0;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbPrimaryAxis, &word))
      primaryAxis = S32(word);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbMaxCapacitorEnergy, &value))
      maxCapacitorEnergy = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbCapacitorRechargeRate, &value))
      capacitorRechargeRate = value;

   return true;
}

IMPLEMENT_CO_DATABLOCK_V1(TurretImageData);

ConsoleDocClass( TurretImageData,
   "@brief A Tribes 2 turret's barrel.\n\n@internal" );

/// 0x00654280.
TurretImageData::TurretImageData()
   : activationMS(1000),
     deactivateDelayMS(1000),
     thinkTimeMS(100),
     degPerSecTheta(45.0f),
     degPerSecPhi(180.0f),
     attackRadius(40.0f),
     dontFireInsideDamageRadius(false),
     damageRadius(0.0f),
     useCapacitor(false),
     yawVariance(0.5f),
     pitchVariance(0.5f)
{
}

void TurretImageData::initPersistFields()
{
   addField("activationMS",               TypeS32,  Offset(activationMS,               TurretImageData));
   addField("deactivateDelayMS",          TypeS32,  Offset(deactivateDelayMS,          TurretImageData));
   addField("thinkTimeMS",                TypeS32,  Offset(thinkTimeMS,                TurretImageData));
   addField("degPerSecTheta",             TypeF32,  Offset(degPerSecTheta,             TurretImageData));
   addField("degPerSecPhi",               TypeF32,  Offset(degPerSecPhi,               TurretImageData));
   addField("attackRadius",               TypeF32,  Offset(attackRadius,               TurretImageData));
   addField("dontFireInsideDamageRadius", TypeBool, Offset(dontFireInsideDamageRadius, TurretImageData));
   addField("damageRadius",               TypeF32,  Offset(damageRadius,               TurretImageData));
   addField("useCapacitor",               TypeBool, Offset(useCapacitor,               TurretImageData));
   addField("yawVariance",                TypeF32,  Offset(yawVariance,                TurretImageData));
   addField("pitchVariance",              TypeF32,  Offset(pitchVariance,              TurretImageData));

   Parent::initPersistFields();
}

/// 0x00654850, after ShapeBaseImageData's.
bool TurretImageData::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   // A local server's own, which keeps what the scripts gave it, as
   // ShapeBaseData::isUnpackedInPlace says of a shape's.
   if(isProperlyAdded() && !dynamic_cast<NetConnection *>(getGroup()))
      return true;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbActivationMS, &word))
      activationMS = S32(word << 5);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbDeactivateDelayMS, &word))
      deactivateDelayMS = S32(word << 5);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbDegPerSecTheta, &word))
      degPerSecTheta = F32(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbDegPerSecPhi, &word))
      degPerSecPhi = F32(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbDontFireInsideDamageRadius,
                                  &word))
      dontFireInsideDamageRadius = word != 0;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbDamageRadius, &value))
      damageRadius = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbUseCapacitor, &word))
      useCapacitor = word != 0;

   return true;
}

IMPLEMENT_CO_NETOBJECT_V1(Turret);

ConsoleDocClass( Turret,
   "@brief A Tribes 2 turret.\n\n@internal" );

/// 0x00654980: closed, level and facing its own forward.
Turret::Turret()
   : mDataBlock(NULL),
     mState(Closed),
     mActivation(0.0f),
     mHeading(0.0f),
     mPitch(90.0f),
     mTargetMS(0),
     mCapacitor(0.0f),
     mCapacitorRate(0.0f),
     mRenderActivation(0.0f),
     mRenderHeading(0.0f),
     mRenderPitch(90.0f),
     mActivationDelta(0.0f),
     mHeadingDelta(0.0f),
     mPitchDelta(0.0f),
     mActivateThread(NULL),
     mElevateThread(NULL),
     mTurnThread(NULL)
{
   mTypeMask |= TurretObjectType;
}

void Turret::destroyThreads()
{
   if(mShapeInstance)
   {
      mShapeInstance->destroyThread(mActivateThread);
      mShapeInstance->destroyThread(mElevateThread);
      mShapeInstance->destroyThread(mTurnThread);
   }

   mActivateThread = NULL;
   mElevateThread = NULL;
   mTurnThread = NULL;
}

bool Turret::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   // A new shape takes the old one's threads with it.
   destroyThreads();

   mDataBlock = dynamic_cast<TurretData *>(dptr);
   return mDataBlock && Parent::onNewDataBlock(dptr, reload);
}

/// 0x00655f60: the aim is set as it arrives, with nothing to smooth it.
void Turret::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   if(body.hasPowered)
      setPowered(body.powered);

   if(body.hasTurretAim && mDataBlock)
   {
      mHeading = body.turretHeading * 360.0f;
      mPitch = mDataBlock->thetaMin +
               body.turretPitch * (mDataBlock->thetaMax - mDataBlock->thetaMin);
      mActivation = body.turretActivation;
   }
}

/// The base aims only once it is fully open.
void Turret::createAimThreads()
{
   TSShapeInstance *shape = mShapeInstance;
   if(!shape)
      return;

   if(!mElevateThread)
   {
      mElevateThread = shape->addThread();
      shape->setSequence(mElevateThread, mDataBlock->elevateSequence, 0.0f);
   }
   if(!mTurnThread)
   {
      mTurnThread = shape->addThread();
      shape->setSequence(mTurnThread, mDataBlock->turnSequence, 0.0f);
   }
}

void Turret::destroyAimThreads()
{
   if(mShapeInstance)
   {
      mShapeInstance->destroyThread(mElevateThread);
      mShapeInstance->destroyThread(mTurnThread);
   }

   mElevateThread = NULL;
   mTurnThread = NULL;
}

/// The base opens as far as the activation says.
void Turret::updateActivateThread(F32 activation)
{
   TSShapeInstance *shape = mShapeInstance;
   if(!shape)
      return;

   if(activation != 0.0f)
   {
      if(!mActivateThread)
      {
         mActivateThread = shape->addThread();
         shape->setSequence(mActivateThread, mDataBlock->activateSequence, 0.0f);
      }
      shape->setPos(mActivateThread, activation);
   }
   else if(mActivateThread)
   {
      shape->destroyThread(mActivateThread);
      mActivateThread = NULL;
   }
}

/// 0x006560b0: the pitch held to the datablock's range and the heading to
/// a single turn, each a share of its sequence.
void Turret::setAimThreads(F32 heading, F32 pitch)
{
   TSShapeInstance *shape = mShapeInstance;
   if(!shape)
      return;

   pitch = mClampF(pitch, mDataBlock->thetaMin, mDataBlock->thetaMax);
   while(heading >= 360.0f)
      heading -= 360.0f;
   while(heading < 0.0f)
      heading += 360.0f;

   if(mTurnThread)
      shape->setPos(mTurnThread, heading / 360.0f);
   if(mElevateThread)
      shape->setPos(mElevateThread, pitch / 180.0f);
}

/// 0x00655948, where a client that does not control the turret takes what
/// the server last sent, with nothing to draw between ticks.
void Turret::snapToLastUpdate()
{
   mRenderActivation = mActivation;
   mRenderHeading = mHeading;
   mRenderPitch = mPitch;
   mActivationDelta = 0.0f;
   mHeadingDelta = 0.0f;
   mPitchDelta = 0.0f;

   if(mActivation == 1.0f)
      createAimThreads();
   else
      destroyAimThreads();
}

/// 0x00656350.
F32 Turret::getRestingPitch() const
{
   return mDataBlock->thetaNull == -1.0f ? 90.0f : mDataBlock->thetaNull;
}

/// 0x006562e0, 0x00656300 and 0x00656320: how far the barrel in slot 0
/// lets it turn, pitch and open in a tick, or retail's defaults without one.
F32 Turret::getHeadingStep()
{
   TurretImageData *barrel = dynamic_cast<TurretImageData *>(getMountedImage(0));
   return (barrel ? barrel->degPerSecPhi : 180.0f) * 0.032f;
}

F32 Turret::getPitchStep()
{
   TurretImageData *barrel = dynamic_cast<TurretImageData *>(getMountedImage(0));
   return (barrel ? barrel->degPerSecTheta : 45.0f) * 0.032f;
}

F32 Turret::getActivationStep()
{
   TurretImageData *barrel = dynamic_cast<TurretImageData *>(getMountedImage(0));
   return (barrel ? 1000.0f / F32(barrel->activationMS) : 1.0f) * 0.032f;
}

/// 0x00655040: a controlled turret opens and stays open. One left to itself
/// opens for a target, and closes once it has had none for the barrel's
/// deactivateDelayMS, or at once with no barrel.
void Turret::updateState(bool controlled)
{
   if(controlled)
   {
      mTarget = NULL;
      if(mState == Closed)
         mState = Activating;
      else if(mState == Activating || mState == Deactivating)
      {
         if(mActivation == 1.0f)
         {
            createAimThreads();
            mState = Active;
         }
         else
            mState = Activating;
      }
      return;
   }

   ShapeBaseImageData *image = getMountedImage(0);
   if(!image)
   {
      mTarget = NULL;
      if(mState == Closed || mState == Deactivating)
         destroyAimThreads();
      else
         mState = Deactivating;
      return;
   }

   switch(mState)
   {
      case Active:
      {
         TurretImageData *barrel = dynamic_cast<TurretImageData *>(image);
         const U32 delay = barrel ? U32(barrel->deactivateDelayMS) : 1000;
         if(!mTarget && mTargetMS > delay)
            mState = Deactivating;
         break;
      }
      case Deactivating:
         if(mTarget)
         {
            if(mActivation == 1.0f)
            {
               createAimThreads();
               mState = Active;
            }
            else
               mState = Activating;
         }
         break;
      case Closed:
         destroyAimThreads();
         if(mTarget)
            mState = Activating;
         break;
      default:
         break;
   }
}

/// 0x00657c50: the barrel is loaded as the base opens, and it rests while it
/// does.
void Turret::activate()
{
   setImageLoadedState(0, true);

   mActivation += getActivationStep();
   if(mActivation >= 1.0f)
   {
      createAimThreads();
      mActivation = 1.0f;
      mState = Active;
   }

   mRenderHeading = 0.0f;
   mHeadingDelta = 0.0f;
   mRenderPitch = getRestingPitch();
   mPitchDelta = 0.0f;
   mRenderActivation = mActivation;
   mActivationDelta = 0.0f;
}

/// 0x00657d70: it turns home the short way and levels, and only then
/// unloads the barrel and closes.
void Turret::deactivate()
{
   bool home = true;

   if(mHeading == 0.0f)
   {
      mRenderHeading = mHeading;
      mHeadingDelta = 0.0f;
   }
   else
   {
      home = false;
      const F32 step = getHeadingStep();
      if(mHeading > 180.0f)
      {
         mHeading += step;
         mHeadingDelta = -step;
      }
      else
      {
         mHeading -= step;
         mHeadingDelta = step;
      }
      mRenderHeading = mHeading;
      if(mHeading >= 360.0f || mHeading <= 0.0f)
         mHeading = 0.0f;
   }

   if(mPitch == 90.0f)
      mPitchDelta = 0.0f;
   else
   {
      home = false;
      const F32 step = getPitchStep();
      const F32 offset = 90.0f - mPitch;
      const F32 sign = offset / mFabs(offset);
      mPitch += sign * step;
      mPitchDelta = -sign * step;
      mRenderPitch = mPitch;
      if((sign < 0.0f && mPitch <= 90.0f) || (sign > 0.0f && mPitch >= 90.0f))
         mPitch = 90.0f;
   }

   if(home)
   {
      destroyAimThreads();
      setImageLoadedState(0, false);

      const F32 step = getActivationStep();
      mActivation -= step;
      mActivationDelta = step;
      mRenderActivation = mActivation;
      if(mActivation <= 0.0f)
      {
         mActivation = 0.0f;
         mState = Closed;
      }
   }
   else
   {
      mRenderActivation = mActivation;
      mActivationDelta = 0.0f;
   }
}

/// 0x006555cf: a controller's move turns it at 40 degrees a radian, and an
/// aim of its own is in degrees; either is held to the barrel's rates.
void Turret::aim(const Move *move, bool controlled)
{
   F32 yaw, pitch;
   if(controlled)
   {
      yaw = mFabs(40.0f * move->yaw);
      pitch = mFabs(40.0f * move->pitch);
   }
   else
   {
      yaw = mFabs(move->yaw);
      pitch = mFabs(move->pitch);
      if(!mTarget)
         mTargetMS += TickMs;
   }

   yaw = getMin(yaw, getHeadingStep());
   if(move->yaw < 0.0f)
      yaw = -yaw;
   pitch = getMin(pitch, getPitchStep());
   if(move->pitch < 0.0f)
      pitch = -pitch;

   mHeading += yaw;
   mPitch += pitch;
   mRenderHeading = mHeading;
   mHeadingDelta = -yaw;
   mRenderPitch = mPitch;
   mPitchDelta = -pitch;
   mRenderActivation = 1.0f;
   mActivationDelta = 0.0f;

   while(mHeading >= 360.0f)
      mHeading -= 360.0f;
   while(mHeading < 0.0f)
      mHeading += 360.0f;
   mPitch = mClampF(mPitch, mDataBlock->thetaMin, mDataBlock->thetaMax);
}

/// 0x006553c0. The server, and the client controlling the turret, run it
/// from moves; any other client shows what the server last sent.
void Turret::processTick(const Move *move)
{
   Parent::processTick(move);

   if(isHidden() || !mDataBlock || mDataBlock->neverUpdateControl)
      return;

   const bool controlled = move != NULL;
   if(!controlled && isGhost())
      snapToLastUpdate();
   else
   {
      // A turret no one controls aims by a move of its own.
      Move ownMove = NullMove;
      if(!move)
         move = &ownMove;

      if(mCapacitor < mDataBlock->maxCapacitorEnergy)
         mCapacitor = mClampF(mCapacitor + mCapacitorRate, 0.0f,
                              mDataBlock->maxCapacitorEnergy);

      if(mDamageState == Enabled)
      {
         updateState(controlled);

         switch(mState)
         {
            case Closed:
               mActivation = mRenderActivation = 0.0f;
               mHeading = mRenderHeading = 0.0f;
               mPitch = mRenderPitch = getRestingPitch();
               mActivationDelta = mHeadingDelta = mPitchDelta = 0.0f;
               break;
            case Activating:
               activate();
               break;
            case Deactivating:
               deactivate();
               break;
            case Active:
               aim(move, controlled);
               break;
            case BarrelSwap:
               // Only the server knows the engineer swapping the barrel; it
               // closes for them, and anyone else just closes.
               mState = Deactivating;
               break;
         }

         // Only an open turret fires, and only the first two triggers;
         // the scripts fire the rest from onTrigger.
         const bool active = mState == Active;
         setImageTriggerState(0, active && move->trigger[0]);
         setImageTriggerState(1, active && move->trigger[1]);
      }
      else
         mTarget = NULL;
   }

   updateActivateThread(mActivation);
   setAimThreads(mHeading, mPitch);

   // Its muzzle and eye ride the aim, so they follow it each tick.
   if((controlled || isServerObject()) && mShapeInstance)
      mShapeInstance->animate();
}

/// 0x00655b90: back from this tick toward the last by the deltas.
void Turret::interpolateTick(F32 delta)
{
   Parent::interpolateTick(delta);

   if(isHidden() || !mDataBlock || !mShapeInstance)
      return;

   if(mActivateThread)
      mShapeInstance->setPos(mActivateThread,
                             mRenderActivation + delta * mActivationDelta);
   setAimThreads(mRenderHeading + delta * mHeadingDelta,
                 mRenderPitch + delta * mPitchDelta);
   mShapeInstance->animate();
}

/// 0x00655ce0, after ShapeBase's energy.
void Turret::writePacketData(GameConnection *conn, BitStream *stream)
{
   Parent::writePacketData(conn, stream);

   stream->writeInt(mState, 3);
   stream->write(mActivation);
   stream->write(mHeading);
   stream->write(mPitch);
}

/// 0x00655d70: what is drawn starts again from what was sent.
void Turret::readPacketData(GameConnection *conn, BitStream *stream)
{
   Parent::readPacketData(conn, stream);

   mState = stream->readInt(3);
   stream->read(&mActivation);
   stream->read(&mHeading);
   stream->read(&mPitch);

   mRenderActivation = mActivation;
   mRenderHeading = mHeading;
   mRenderPitch = mPitch;
   mActivationDelta = 0.0f;
   mHeadingDelta = 0.0f;
   mPitchDelta = 0.0f;
}
