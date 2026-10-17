// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/targetsT2.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "core/stream/bitStream.h"
#include "math/mMatrix.h"
#include "sfx/sfxDescription.h"
#include "sfx/sfxProfile.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxSystem.h"
#include "sim/netStringTable.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameBase.h"

namespace TargetsT2
{
   struct Target
   {
      enum { NumTags = 5, VoiceTag = 3 };

      NetStringHandle         tags[NumTags];
      S32                     sensorGroup;
      S32                     renderMask;
      F32                     voicePitch;
      SimObjectPtr<GameBase>  object;
      SimObjectPtr<SFXSource> audio;

      Target() : sensorGroup(0), renderMask(0), voicePitch(1.0f) {}
   };

   static Target sTargets[MaxTargets];

   static Target *getTarget(S32 id)
   {
      return id >= 0 && id < MaxTargets ? &sTargets[id] : NULL;
   }

   void setObjectTarget(GameBase *object, S32 previous, S32 target)
   {
      Target *old = getTarget(previous);
      if(old && old->object == object)
         old->object = NULL;

      Target *now = getTarget(target);
      if(now && target >= FirstObjectTarget)
         now->object = object;
   }
}

using namespace TargetsT2;

//----------------------------------------------------------------------------

class TargetInfoEvent : public NetEvent
{
   typedef NetEvent Parent;

   enum
   {
      TagBits = ConnectionStringTable::EntryBitSize,
      NoTag = ConnectionStringTable::EntryCount,
      SensorGroupBits = 5,
      RenderMaskBits = 9,
      PitchBits = 7,
      NoDataBlock = -2,
   };

   S32 mTarget;
   S32 mTags[Target::NumTags];
   S32 mSensorGroup;
   S32 mDataBlock;
   S32 mRenderMask;
   F32 mVoicePitch;

public:
   TargetInfoEvent()
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = 0;
      for(U32 i = 0; i < Target::NumTags; i++)
         mTags[i] = -1;
      mSensorGroup = -1;
      mDataBlock = -1;
      mRenderMask = -1;
      mVoicePitch = -1.0f;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->writeInt(mTarget, TargetBits);
      for(U32 i = 0; i < Target::NumTags; i++)
         if(stream->writeFlag(mTags[i] != -1) &&
            stream->writeFlag(mTags[i] != NoTag))
            stream->writeInt(mTags[i], TagBits);

      if(stream->writeFlag(mSensorGroup != -1))
         stream->writeInt(mSensorGroup, SensorGroupBits);
      if(stream->writeFlag(mDataBlock != -1) &&
         stream->writeFlag(mDataBlock != NoDataBlock))
         stream->writeInt(mDataBlock - DataBlockObjectIdFirst,
                          DataBlockObjectIdBitSize);
      if(stream->writeFlag(mRenderMask != -1))
         stream->writeInt(mRenderMask, RenderMaskBits);
      if(stream->writeFlag(mVoicePitch != -1.0f))
         stream->writeFloat((mVoicePitch - 0.5f) / 1.5f, PitchBits);
   }

   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }

   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = stream->readInt(TargetBits);
      for(U32 i = 0; i < Target::NumTags; i++)
         if(stream->readFlag())
            mTags[i] = stream->readFlag() ? S32(stream->readInt(TagBits))
                                          : S32(NoTag);

      if(stream->readFlag())
         mSensorGroup = stream->readInt(SensorGroupBits);
      if(stream->readFlag())
         mDataBlock = stream->readFlag()
                      ? S32(stream->readInt(DataBlockObjectIdBitSize) +
                            DataBlockObjectIdFirst)
                      : S32(NoDataBlock);
      if(stream->readFlag())
         mRenderMask = stream->readInt(RenderMaskBits);
      if(stream->readFlag())
         mVoicePitch = stream->readFloat(PitchBits) * 1.5f + 0.5f;
   }

   void process(NetConnection *conn)
   {
      Target *target = getTarget(mTarget);
      if(!target)
         return;

      for(U32 i = 0; i < Target::NumTags; i++)
      {
         if(mTags[i] == -1)
            continue;

         target->tags[i] = mTags[i] == NoTag
                           ? NetStringHandle()
                           : conn->translateRemoteStringId(mTags[i]);
      }

      if(mSensorGroup != -1)
         target->sensorGroup = mSensorGroup;
      if(mRenderMask != -1)
         target->renderMask = mRenderMask;
      if(mVoicePitch != -1.0f)
         target->voicePitch = mVoicePitch;
   }

   DECLARE_CONOBJECT(TargetInfoEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(TargetInfoEvent);

ConsoleDocClass( TargetInfoEvent,
   "@brief Tells a client what a target is called and how it sounds.\n\n"
   "@internal" );

//----------------------------------------------------------------------------

class SimTargetAudioEvent : public NetEvent
{
   typedef NetEvent Parent;

   enum { FileTagBits = ConnectionStringTable::EntryBitSize };

   S32     mTarget;
   U32     mFileTag;
   S32     mDescription;
   bool    mHasPosition;
   Point3F mPosition;
   bool    mUpdate;

public:
   SimTargetAudioEvent()
   {
      mGuaranteeType = Guaranteed;
      mTarget = 0;
      mFileTag = 0;
      mDescription = -1;
      mHasPosition = false;
      mPosition.set(0.0f, 0.0f, 0.0f);
      mUpdate = false;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->writeInt(mTarget, TargetBits);
      stream->writeInt(mFileTag, FileTagBits);
      if(stream->writeFlag(mDescription != -1))
         stream->writeInt(mDescription - DataBlockObjectIdFirst,
                          DataBlockObjectIdBitSize);
      if(stream->writeFlag(mHasPosition))
         stream->writeCompressedPoint(mPosition, 0.5f);
      stream->writeFlag(mUpdate);
   }

   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }

   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = stream->readInt(TargetBits);
      mFileTag = stream->readInt(FileTagBits);
      mDescription = stream->readFlag()
                     ? S32(stream->readInt(DataBlockObjectIdBitSize) +
                           DataBlockObjectIdFirst)
                     : -1;
      mHasPosition = stream->readFlag();
      if(mHasPosition)
         stream->readCompressedPoint(&mPosition, 0.5f);
      mUpdate = stream->readFlag();
   }

   void process(NetConnection *conn)
   {
      Target *target = getTarget(mTarget);
      if(!target || mDescription == -1 || !SFX)
         return;

      SFXDescription *description =
         dynamic_cast<SFXDescription *>(Sim::findObject(mDescription));
      if(!description)
         return;

      if(!mHasPosition)
      {
         if(!target->object)
            return;
         mPosition = target->object->getPosition();
      }

      const NetStringHandle file = conn->translateRemoteStringId(mFileTag);
      const char *fileName = file.getString();
      const char *voice = target->tags[Target::VoiceTag].getString();
      if(!fileName || !fileName[0] || !voice || !voice[0])
         return;

      char path[256];
      dSprintf(path, sizeof(path), "voice/%s/%s.wav", voice, fileName);

      if(Con::getBoolVariable("$pref::Net::traceTargetAudio", false))
         Con::printf("T2 target audio: target %d %s at %g %g %g pitch %g%s",
                     mTarget, path, mPosition.x, mPosition.y, mPosition.z,
                     target->voicePitch, mUpdate ? " (update)" : "");

      if(mUpdate && target->audio)
         SFX->stopAndDeleteSource(target->audio);

      SFXProfile *profile =
         new SFXProfile(description, StringTable->insert(path), true);
      if(!profile->registerObject())
      {
         delete profile;
         return;
      }
      profile->setAutoDelete(true);

      MatrixF transform(true);
      transform.setPosition(mPosition);
      SFXSource *source = SFX->createSource(profile, &transform);
      if(!source)
         return;

      if(target->voicePitch != 1.0f)
         source->setPitch(target->voicePitch);

      if(mUpdate)
         target->audio = source;

      source->play();
      SFX->deleteWhenStopped(source);
   }

   DECLARE_CONOBJECT(SimTargetAudioEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(SimTargetAudioEvent);

ConsoleDocClass( SimTargetAudioEvent,
   "@brief Plays a line in a target's voice, where that target is.\n\n"
   "@internal" );
