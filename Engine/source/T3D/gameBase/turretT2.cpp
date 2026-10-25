// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/turretT2.h"

#include "collision/collision.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "math/mRandom.h"
#include "scene/sceneContainer.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/moveManager.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/gameBase/targetManagerT2.h"
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

/// 0x00654450: the times are held to [32, 5000] ms, opening and closing
/// rounded down to a tick and thinking up to one; the rates are held to
/// [1, 1080] whole degrees a second and the reach to [10, 1000] metres.
bool TurretImageData::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if(activationMS < 32 || activationMS > 5000)
   {
      Con::warnf("TurretImageData(%s)::onAdd: activationMS must be in the "
                 "range [%d, 5000]", getName(), 32);
      activationMS = activationMS < 32 ? 32 : 5000;
   }
   activationMS &= ~31;
   if(deactivateDelayMS < 32 || deactivateDelayMS > 5000)
   {
      Con::warnf("TurretImageData(%s)::onAdd: deactivateDelayMS must be in "
                 "the range [%d, 5000]", getName(), 32);
      deactivateDelayMS = deactivateDelayMS < 32 ? 32 : 5000;
   }
   deactivateDelayMS &= ~31;
   if(thinkTimeMS < 32 || thinkTimeMS > 5000)
   {
      Con::warnf("TurretImageData(%s)::onAdd: thinkTimeMS must be in the "
                 "range [%d, 5000]", getName(), 32);
      thinkTimeMS = thinkTimeMS < 32 ? 32 : 5000;
   }
   thinkTimeMS = (thinkTimeMS + 31) & ~31;

   if(degPerSecTheta < 1.0f || degPerSecTheta > 1080.0f)
   {
      Con::warnf("TurretImageData(%s)::onAdd: degPerSecTheta must be in the "
                 "range [1, 1080]", getName());
      degPerSecTheta = degPerSecTheta < 1.0f ? 1.0f : 1080.0f;
   }
   degPerSecTheta = F32(S32(degPerSecTheta));
   if(degPerSecPhi < 1.0f || degPerSecPhi > 1080.0f)
   {
      Con::warnf("TurretImageData(%s)::onAdd: degPerSecPhi must be in the "
                 "range [1, 1080]", getName());
      degPerSecPhi = degPerSecPhi < 1.0f ? 1.0f : 1080.0f;
   }
   degPerSecPhi = F32(S32(degPerSecPhi));
   if(attackRadius < 10.0f || attackRadius > 1000.0f)
   {
      Con::warnf("TurretImageData(%s)::onAdd: attackRadius must be in the "
                 "range [10, 1000]", getName());
      attackRadius = attackRadius < 10.0f ? 10.0f : 1000.0f;
   }

   return true;
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

/// 0x00654980: closed, level and facing its own forward, aiming true and
/// looking for targets of its own.
Turret::Turret()
   : mDataBlock(NULL),
     mState(Closed),
     mActivation(0.0f),
     mHeading(0.0f),
     mPitch(90.0f),
     mInitialBarrel(StringTable->EmptyString()),
     mSkill(1.0f),
     mTargetMS(0),
     mThinkMS(0),
     mAutoFire(true),
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
     mTurnThread(NULL),
     mBarrelDamageThread(NULL),
     mBarrelDeployThread(NULL)
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

void Turret::initPersistFields()
{
   addField("initialBarrel", TypeString, Offset(mInitialBarrel, Turret));

   Parent::initPersistFields();
}

/// 0x00654c60: the server mounts the barrel a mission names, unloaded.
bool Turret::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if(isServerObject() && mInitialBarrel && mInitialBarrel[0])
   {
      ShapeBaseImageData *barrel = NULL;
      if(Sim::findObject(mInitialBarrel, barrel))
      {
         NetStringHandle skin;
         mountImage(barrel, 0, false, skin);
      }
   }
   mThinkMS = 0;

   return true;
}

bool Turret::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   // A new shape takes the old one's threads with it.
   destroyThreads();

   mDataBlock = dynamic_cast<TurretData *>(dptr);
   return mDataBlock && Parent::onNewDataBlock(dptr, reload);
}

/// 0x00654df0 and 0x00654e30: initialBarrel names whatever was last mounted,
/// in any slot, and nothing once one is taken off.
bool Turret::mountImage(ShapeBaseImageData *image, U32 imageSlot, bool loaded,
                        NetStringHandle &skinNameHandle)
{
   if(Parent::mountImage(image, imageSlot, loaded, skinNameHandle))
      mInitialBarrel = image->getName() ? image->getName()
                                        : StringTable->EmptyString();
   return true;
}

bool Turret::unmountImage(U32 imageSlot)
{
   if(Parent::unmountImage(imageSlot))
      mInitialBarrel = StringTable->EmptyString();
   return true;
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

namespace
{

/// What a turret's sight of its target is blocked by (0x1000c).
const U32 LOSMask = TerrainObjectType | InteriorObjectType | VehicleObjectType;

/// GameBase::getSensorGroup (0x005e3490): an object with no target is in
/// none.
S32 getSensorGroup(const GameBase *obj)
{
   const S32 target = obj->getTarget();
   return target == -1 ? 0 : TargetManagerT2::server()->getTarget(target).sensorGroup;
}

}

/// 0x00656280 and 0x006562b0: how often the barrel in slot 0 looks for a
/// target and how far, or retail's defaults without one.
U32 Turret::getThinkMS()
{
   TurretImageData *barrel = dynamic_cast<TurretImageData *>(getMountedImage(0));
   return barrel ? U32(barrel->thinkTimeMS) : 200;
}

F32 Turret::getAttackRadius()
{
   TurretImageData *barrel = dynamic_cast<TurretImageData *>(getMountedImage(0));
   return barrel ? barrel->attackRadius : 40.0f;
}

/// 0x00656380: anything outside (0, 1] aims true.
void Turret::setSkill(F32 skill)
{
   mSkill = skill > 0.0f && skill <= 1.0f ? skill : 1.0f;
}

/// 0x006563d0: a new target, or the same one again, is had from now.
bool Turret::setTargetObject(ShapeBase *target)
{
   mTarget = target;
   mTargetMS = 0;
   return target != NULL;
}

/// 0x00656940: one that is not to look for targets drops the one it has.
void Turret::setAutoFire(bool autoFire)
{
   mAutoFire = autoFire;
   if(!mAutoFire)
      setTargetObject(NULL);
}

/// 0x006564c0: a target the turret can see, reach and hit, on another side.
bool Turret::isValidTarget(ShapeBase *target)
{
   if(!target || target->getDamageValue() >= 1.0f)
      return false;
   if(target->getTarget() == -1 || getTarget() == -1)
      return false;

   ShapeBaseImageData *image = getMountedImage(0);
   if(image && image->isSeeker && target->getHeat() < image->minSeekHeat)
      return false;

   const Point3F centre = target->getWorldBox().getCenter();
   if(image)
   {
      const F32 dist = (centre - getPosition()).len();
      TurretImageData *barrel = dynamic_cast<TurretImageData *>(image);
      if(barrel && barrel->dontFireInsideDamageRadius &&
         dist <= barrel->damageRadius)
         return false;
      if(image->minTargetingDistance != 0.0f &&
         dist <= image->minTargetingDistance)
         return false;
   }

   TargetManagerT2 *targets = TargetManagerT2::server();
   if(!targets->isTargetVisible(target->getTarget(), getSensorGroup(this)) ||
      targets->isTargetFriendly(getTarget(), getSensorGroup(target)))
      return false;

   MatrixF muzzle;
   getMuzzleTransform(0, &muzzle);
   const Point3F start = muzzle.getPosition();
   if((centre - start).len() > getAttackRadius())
      return false;

   target->disableCollision();
   disableCollision();
   RayInfo ray;
   const bool blocked = gServerContainer.castRay(start, centre, LOSMask, &ray);
   target->enableCollision();
   enableCollision();
   return !blocked;
}

/// 0x00656a90: a target it can no longer have is dropped, and the scripts
/// are asked for another; with one, it opens.
void Turret::think()
{
   mThinkMS = 0;

   if(mTarget && !isValidTarget(mTarget))
      mTarget = NULL;
   if(!mTarget)
      Con::executef(mDataBlock, "selectTarget", getIdString());

   if(!mTarget)
      return;
   if(mState == Closed)
      mState = Activating;
   else if(mState != BarrelSwap)
      mState = mActivation == 1.0f ? Active : Activating;
}

/// 0x00656b50: a turret short of full skill aims at a point beside its
/// target, off the line to it by up to a share of its distance.
Point3F Turret::getSkilledAim(const Point3F &start, const Point3F &end) const
{
   const Point3F away = start - end;
   Point3F across = mCross(away, Point3F(0.0f, 0.0f, 1.0f));
   if(across.isZero())
      return end;
   across.normalize();
   Point3F up = mCross(away, across);
   if(up.isZero())
      return end;
   up.normalize();

   const F32 error = mSkill == 1.0f ? 0.0f : (1.0f - mSkill) * 0.2f + 0.04f;
   const F32 spread = away.len() * error * 0.1f;
   const F32 sideSign = gRandGen.randF() < 0.5f ? -1.0f : 1.0f;
   const F32 side = spread * gRandGen.randF() * sideSign;
   const F32 upSign = gRandGen.randF() < 0.5f ? -1.0f : 1.0f;
   const F32 rise = spread * gRandGen.randF() * upSign;
   return end + across * side + up * rise;
}

/// 0x00656e80: an uncontrolled turret's own move, in degrees, toward its
/// target, which it loses once anything stands between them.  Retail leads
/// the target by the slot-0 barrel's projectile and fires once the lead is
/// good and the aim is within yawVariance and pitchVariance; this server's
/// projectiles are not its own yet (projectilesT2.cpp is the client's), so
/// this one aims where the target is and pulls no trigger.
void Turret::aimAtTarget(Move *move)
{
   if(!mTarget)
      return;

   MatrixF muzzle;
   getMuzzleTransform(0, &muzzle);
   const Point3F start = muzzle.getPosition();
   Point3F end = mTarget->getWorldBox().getCenter();

   mTarget->disableCollision();
   disableCollision();
   if(mSkill < 1.0f)
      end = getSkilledAim(start, end);
   RayInfo ray;
   const bool blocked = gServerContainer.castRay(start, end, LOSMask, &ray);
   mTarget->enableCollision();
   enableCollision();
   if(blocked)
   {
      mTargetMS = 0;
      mTarget = NULL;
      return;
   }

   // The way to it as the turret is mounted.
   Point3F dir = end - start;
   getWorldTransform().mulV(dir);
   dir.normalizeSafe();

   F32 heading = 0.0f;
   F32 pitch = 0.0f;
   switch(mDataBlock->primaryAxis)
   {
      case TurretData::ZAxis:
         heading = mRadToDeg(mAtan2(dir.x, dir.y));
         pitch = 90.0f - mRadToDeg(mAtan2(dir.z, mSqrt(dir.x * dir.x + dir.y * dir.y)));
         break;
      case TurretData::RevZAxis:
         heading = 360.0f - mRadToDeg(mAtan2(dir.x, dir.y));
         pitch = 90.0f + mRadToDeg(mAtan2(dir.z, mSqrt(dir.x * dir.x + dir.y * dir.y)));
         break;
      case TurretData::YAxis:
      case TurretData::RevYAxis:
         heading = mRadToDeg(mAtan2(-dir.x, dir.z));
         pitch = 90.0f - mRadToDeg(mAtan2(dir.y, mSqrt(dir.z * dir.z + dir.x * dir.x)));
         break;
   }

   // The short way round from where it points.
   if(heading < 0.0f)
      heading += 360.0f;
   const F32 straight = mFabs(heading - mHeading);
   if(mFabs(heading - 360.0f - mHeading) < straight)
      heading -= 360.0f;
   else if(mFabs(heading + 360.0f - mHeading) < straight)
      heading += 360.0f;

   move->yaw = heading - mHeading;
   move->pitch = pitch - mPitch;
}

/// 0x00656970: once it has closed, the scripts swap the barrel for the
/// engineer who asked.
void Turret::replaceBarrel()
{
   Con::executef(mDataBlock, "replaceCallback", getIdString(),
                 mSwapEngineer->getIdString());
   mSwapEngineer = NULL;
}

/// 0x006569e0: a closed turret, or one with no barrel, swaps at once; an
/// open one closes first.
bool Turret::initiateBarrelSwap(ShapeBase *engineer)
{
   if(!engineer)
      return false;

   // Retail reports each swap on the console, as an error.
   mSwapEngineer = engineer;
   if(!getMountedImage(0) || mState == Closed)
   {
      replaceBarrel();
      Con::errorf("instant replace");
   }
   else
   {
      mState = BarrelSwap;
      Con::errorf("active replace");
   }
   return true;
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

         // One left to itself looks for a target every thinkTimeMS.
         if(!controlled && mAutoFire && getMountedImage(0))
         {
            mThinkMS += TickMs;
            if(mThinkMS >= getThinkMS())
               think();
         }

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
               if(!controlled && isServerObject())
                  aimAtTarget(&ownMove);
               aim(move, controlled);
               break;
            case BarrelSwap:
               // Only the server knows the engineer swapping the barrel; it
               // closes for them and then has the scripts swap it, and
               // anyone else just closes.
               if(mSwapEngineer)
               {
                  deactivate();
                  if(mState == Closed)
                     replaceBarrel();
               }
               else
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
      {
         mTarget = NULL;
         mSwapEngineer = NULL;
      }
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

/// 0x00655b50: the barrel deploys at half its sequence's speed.
void Turret::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(mBarrelDeployThread)
      mBarrelDeployThread->getShapeInstance()->advanceTime(dt * 0.5f,
                                                           mBarrelDeployThread);
}

/// 0x00654fe0: the barrel shows the base's damage, up to its full sequence.
void Turret::updateDamageLevel()
{
   Parent::updateDamageLevel();

   if(mBarrelDamageThread)
      mBarrelDamageThread->getShapeInstance()->setPos(
         mBarrelDamageThread, getMin(getDamageValue(), 1.0f));
}

/// The same thread at the end of its shape's list, where any thread before it
/// on the same nodes wins.
static TSThread *readdThread(TSShapeInstance *shape, TSThread *thread)
{
   const S32 sequence = thread->getSeqIndex();
   const F32 pos = shape->getPos(thread);
   const F32 timeScale = shape->getTimeScale(thread);

   shape->destroyThread(thread);
   thread = shape->addThread();
   shape->setSequence(thread, sequence, pos);
   shape->setTimeScale(thread, timeScale);
   return thread;
}

/// 0x00654e60: a ghost's new barrel gets its own damage and deploy threads,
/// the deploy from its start. Retail does this for any slot, and a later
/// slot's threads take over from an earlier one's.
void Turret::setImage(U32 imageSlot, ShapeBaseImageData *imageData,
                      NetStringHandle &skinNameHandle, bool loaded, bool ammo,
                      bool triggerDown, bool altTriggerDown, bool motion,
                      bool genericTrigger0, bool genericTrigger1,
                      bool genericTrigger2, bool genericTrigger3, bool target)
{
   MountedImage &image = mMountedImageList[imageSlot];

   // A ghost body sets every image it carries, but retail's sets one only
   // when its datablock or skin changes (0x005ef769), so only that restarts
   // the deploy.
   const bool changed = isGhost() &&
                        (image.dataBlock != imageData ||
                         image.skinNameHandle != skinNameHandle);

   TSShapeInstance *shape =
      image.shapeInstance[ShapeBaseImageData::StandardImageShape];
   if(changed && shape)
   {
      if(mBarrelDamageThread && mBarrelDamageThread->getShapeInstance() == shape)
      {
         shape->destroyThread(mBarrelDamageThread);
         mBarrelDamageThread = NULL;
      }
      if(mBarrelDeployThread && mBarrelDeployThread->getShapeInstance() == shape)
      {
         shape->destroyThread(mBarrelDeployThread);
         mBarrelDeployThread = NULL;
      }
   }

   Parent::setImage(imageSlot, imageData, skinNameHandle, loaded, ammo,
                    triggerDown, altTriggerDown, motion, genericTrigger0,
                    genericTrigger1, genericTrigger2, genericTrigger3, target);

   shape = image.shapeInstance[ShapeBaseImageData::StandardImageShape];
   if(!changed || !shape)
      return;

   const S32 damage = shape->getShape()->findSequence("damage");
   if(damage != -1)
   {
      mBarrelDamageThread = shape->addThread();
      shape->setSequence(mBarrelDamageThread, damage, 0.0f);
      updateDamageLevel();
   }

   const S32 deploy = shape->getShape()->findSequence("deploy");
   if(deploy != -1)
   {
      mBarrelDeployThread = shape->addThread();
      shape->setSequence(mBarrelDeployThread, deploy, 0.0f);

      // The image's own threads come first and hold the barrel at its first
      // sequence, which is the deploy, closed. Retail's sort of equal
      // priorities puts them behind these; ours keeps the order they were
      // added in, so they are added again.
      const U32 i = ShapeBaseImageData::StandardImageShape;
      if(image.animThread[i])
         image.animThread[i] = readdThread(shape, image.animThread[i]);
      if(image.flashThread[i])
         image.flashThread[i] = readdThread(shape, image.flashThread[i]);
   }
}

//----------------------------------------------------------------------------

// Registered at 0x00654b10.  Every object a turret is handed is looked up by
// its id, and only a shape will do.

namespace
{

ShapeBase *findShape(const char *id)
{
   ShapeBase *shape = NULL;
   Sim::findObject(SimObjectId(dAtoi(id)), shape);
   return shape;
}

}

ConsoleMethod(Turret, setSkill, void, 3, 3, "turret.setSkill(skill)")
{
   object->setSkill(dAtof(argv[2]));
}

ConsoleMethod(Turret, setTargetObject, bool, 3, 3, "turret.setTargetObject(obj)")
{
   return object->setTargetObject(findShape(argv[2]));
}

ConsoleMethod(Turret, clearTarget, void, 2, 2, "turret.clearTarget()")
{
   object->setTargetObject(NULL);
}

ConsoleMethod(Turret, getTargetObject, S32, 2, 2, "turret.getTargetObject()")
{
   ShapeBase *target = object->getTargetObject();
   return target ? target->getId() : 0;
}

ConsoleMethod(Turret, isValidTarget, bool, 3, 3, "turret.isValidTarget(obj)")
{
   ShapeBase *target = findShape(argv[2]);
   return target && object->isValidTarget(target);
}

ConsoleMethod(Turret, initiateBarrelSwap, bool, 3, 3, "turret.initiateBarrelSwap(engineer)")
{
   return object->initiateBarrelSwap(findShape(argv[2]));
}

ConsoleMethod(Turret, setAutoFire, void, 3, 3, "turret.setAutoFire(bool)")
{
   object->setAutoFire(!dStricmp(argv[2], "true") || dAtoi(argv[2]) > 0);
}

ConsoleMethod(Turret, setCapacitorRechargeRate, void, 3, 3, "turret.setCapacitorRechargeRate(rate)")
{
   object->setCapacitorRechargeRate(dAtof(argv[2]));
}

ConsoleMethod(Turret, getCapacitorLevel, S32, 2, 2, "turret.getCapacitorLevel()")
{
   return S32(object->getCapacitorLevel());
}

ConsoleMethod(Turret, setCapacitorLevel, void, 3, 3, "turret.setCapacitorLevel(level)")
{
   object->setCapacitorLevel(dAtof(argv[2]));
}
