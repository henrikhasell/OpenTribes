// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/turretT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
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
     mActivation(0.0f),
     mHeading(0.0f),
     mPitch(90.0f),
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

/// 0x00655948, where a client takes what the server last sent: the base
/// opens as far as the activation says, and aims only once it is open.
void Turret::updateThreads()
{
   TSShapeInstance *shape = mShapeInstance;
   if(!shape)
      return;

   if(mActivation == 1.0f)
   {
      if(!mElevateThread || !mTurnThread)
      {
         mElevateThread = shape->addThread();
         mTurnThread = shape->addThread();
         shape->setSequence(mElevateThread, mDataBlock->elevateSequence, 0.0f);
         shape->setSequence(mTurnThread, mDataBlock->turnSequence, 0.0f);
      }
   }
   else if(mElevateThread || mTurnThread)
   {
      shape->destroyThread(mElevateThread);
      shape->destroyThread(mTurnThread);
      mElevateThread = NULL;
      mTurnThread = NULL;
   }

   if(mActivation != 0.0f)
   {
      if(!mActivateThread)
      {
         mActivateThread = shape->addThread();
         shape->setSequence(mActivateThread, mDataBlock->activateSequence, 0.0f);
      }
      shape->setPos(mActivateThread, mActivation);
   }
   else if(mActivateThread)
   {
      shape->destroyThread(mActivateThread);
      mActivateThread = NULL;
   }

   // 0x006560b0: the pitch held to the datablock's range and the heading to
   // a single turn, each a share of its sequence.
   const F32 pitch = mClampF(mPitch, mDataBlock->thetaMin,
                             mDataBlock->thetaMax);
   F32 heading = mHeading;
   while(heading >= 360.0f)
      heading -= 360.0f;
   while(heading < 0.0f)
      heading += 360.0f;

   if(mTurnThread)
      shape->setPos(mTurnThread, heading / 360.0f);
   if(mElevateThread)
      shape->setPos(mElevateThread, pitch / 180.0f);
}

/// 0x006553c0, a client's half of it: what the server last sent sets the
/// threads it draws. A server's own turret does not aim yet.
void Turret::processTick(const Move *move)
{
   Parent::processTick(move);

   if(isHidden() || !mDataBlock || mDataBlock->neverUpdateControl)
      return;

   if(isGhost())
      updateThreads();
}
