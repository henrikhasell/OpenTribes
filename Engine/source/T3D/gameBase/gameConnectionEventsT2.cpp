// HH: Added for Tribes 2 compatibility.
// The events a Tribes 2 server opens a session with.  A client that cannot
// read one cannot read the rest of the packet either, so each takes its own
// bits off the wire even where the port does nothing with them.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/shapeBase.h"
#include "T3D/player.h"
#include "core/crc.h"
#include "core/stream/fileStream.h"
#include "ts/tsShape.h"
#include "ts/tsMaterialList.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "sfx/sfxDescription.h"
#include "sfx/sfxProfile.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxSystem.h"


//----------------------------------------------------------------------------

class ResetClientTargetsEvent : public NetEvent
{
   typedef NetEvent Parent;
   bool mAll;

public:
   ResetClientTargetsEvent(bool all = false)
   {
      mGuaranteeType = GuaranteedOrdered;
      mAll = all;
   }

   void pack(NetConnection *, BitStream *stream) { stream->writeFlag(mAll); }
   void write(NetConnection *, BitStream *stream) { stream->writeFlag(mAll); }
   void unpack(NetConnection *, BitStream *stream) { mAll = stream->readFlag(); }
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(ResetClientTargetsEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(ResetClientTargetsEvent);

ConsoleDocClass( ResetClientTargetsEvent,
   "@brief Tells a client to forget every target it holds.\n\n@internal" );

//----------------------------------------------------------------------------

class RemoveClientTargetTypeEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mType;

public:
   RemoveClientTargetTypeEvent(U32 type = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mType = type;
   }

   void pack(NetConnection *, BitStream *stream) { stream->writeInt(mType, TypeBits); }
   void write(NetConnection *, BitStream *stream) { stream->writeInt(mType, TypeBits); }
   void unpack(NetConnection *, BitStream *stream) { mType = stream->readInt(TypeBits); }
   void process(NetConnection *) {}

   enum { TypeBits = 2 };

   DECLARE_CONOBJECT(RemoveClientTargetTypeEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(RemoveClientTargetTypeEvent);

ConsoleDocClass( RemoveClientTargetTypeEvent,
   "@brief Tells a client to forget one kind of target.\n\n@internal" );

//----------------------------------------------------------------------------

class SetSensorGroupEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mGroup;

public:
   SetSensorGroupEvent(U32 group = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mGroup = group;
   }

   void pack(NetConnection *, BitStream *stream) { stream->writeInt(mGroup, GroupBits); }
   void write(NetConnection *, BitStream *stream) { stream->writeInt(mGroup, GroupBits); }
   void unpack(NetConnection *, BitStream *stream) { mGroup = stream->readInt(GroupBits); }
   void process(NetConnection *) {}

   enum { GroupBits = 5 };

   DECLARE_CONOBJECT(SetSensorGroupEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(SetSensorGroupEvent);

ConsoleDocClass( SetSensorGroupEvent,
   "@brief Tells a client which sensor group it belongs to.\n\n@internal" );

//----------------------------------------------------------------------------

class SensorGroupColorEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mGroup;
   U32 mColor;

public:
   SensorGroupColorEvent(U32 group = 0, U32 color = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mGroup = group;
      mColor = color;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->writeInt(mGroup, SetSensorGroupEvent::GroupBits);
      stream->write(mColor);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      T2Layout::readEvent(stream, "SensorGroupColorEvent");
   }
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(SensorGroupColorEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(SensorGroupColorEvent);

ConsoleDocClass( SensorGroupColorEvent,
   "@brief Gives a client the colour to draw a sensor group in.\n\n@internal" );

//----------------------------------------------------------------------------

class TargetFreeEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mTarget;

public:
   TargetFreeEvent(U32 target = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = target;
   }

   void pack(NetConnection *, BitStream *stream) { stream->writeInt(mTarget, TargetBits); }
   void write(NetConnection *, BitStream *stream) { stream->writeInt(mTarget, TargetBits); }
   void unpack(NetConnection *, BitStream *stream) { mTarget = stream->readInt(TargetBits); }
   void process(NetConnection *) {}

   enum { TargetBits = 9 };

   DECLARE_CONOBJECT(TargetFreeEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(TargetFreeEvent);

ConsoleDocClass( TargetFreeEvent,
   "@brief Tells a client one target is gone.\n\n@internal" );

//----------------------------------------------------------------------------

class SetObjectActiveImageEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mGhost;
   U32 mSlot;

public:
   SetObjectActiveImageEvent(U32 ghost = 0, U32 slot = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mGhost = ghost;
      mSlot = slot;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->writeInt(mGhost, NetConnection::GhostIdBitSize);
      stream->writeInt(mSlot, SlotBits);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      mGhost = stream->readInt(NetConnection::GhostIdBitSize);
      mSlot = stream->readInt(SlotBits);
   }
   void process(NetConnection *) {}

   enum { SlotBits = 4 };

   DECLARE_CONOBJECT(SetObjectActiveImageEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(SetObjectActiveImageEvent);

ConsoleDocClass( SetObjectActiveImageEvent,
   "@brief Tells a client which image slot an object is holding up.\n\n@internal" );

//----------------------------------------------------------------------------

class CRCChallengeResponseEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mSeed;
   U32 mSum;
   U32 mLength;

public:
   CRCChallengeResponseEvent(U32 seed = 0, U32 sum = 0, U32 length = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mSeed = seed;
      mSum = sum;
      mLength = length;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->write(mSeed);
      stream->write(mSum);
      stream->write(mLength);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      stream->read(&mSeed);
      stream->read(&mSum);
      stream->read(&mLength);
   }
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(CRCChallengeResponseEvent);
};

IMPLEMENT_CO_SERVEREVENT_V1(CRCChallengeResponseEvent);

ConsoleDocClass( CRCChallengeResponseEvent,
   "@brief A client's answer to a checksum challenge.\n\n@internal" );

//----------------------------------------------------------------------------

class CRCChallengeEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32  mSeed;
   U32  mStart;
   U32  mLength;
   bool mWhole;

public:
   CRCChallengeEvent()
   {
      mGuaranteeType = GuaranteedOrdered;
      mSeed = mStart = mLength = 0;
      mWhole = false;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      stream->write(mSeed);
      stream->write(mStart);
      stream->write(mLength);
      stream->writeFlag(mWhole);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      stream->read(&mSeed);
      stream->read(&mStart);
      stream->read(&mLength);
      mWhole = stream->readFlag();
   }

   static bool sum(const char *path, U32 &crc, U32 &total)
   {
      FileStream *file = FileStream::createAndOpen(path, Torque::FS::File::Read);
      if(!file)
         return false;

      crc = CRC::calculateCRCStream(file, crc);
      total += file->getStreamSize();
      delete file;
      return true;
   }

   void process(NetConnection *conn)
   {
      U32 crc = mSeed;
      U32 total = 0;

      for(U32 id = DataBlockObjectIdFirst; id <= DataBlockObjectIdLast; id++)
      {
         StringTableEntry   name  = NULL;
         const TSShape     *shape = NULL;

         if(ShapeBaseData *data = dynamic_cast<ShapeBaseData *>(Sim::findObject(id)))
         {
            name  = data->shapeName;
            shape = data->mShape;
         }
         else if(DataBlockT2 *stub = dynamic_cast<DataBlockT2 *>(Sim::findObject(id)))
         {
            if(!stub->isShapeBaseData())
               continue;

            name  = stub->shapeName;
            shape = stub->mShape;
         }

         if(!shape)
            continue;

         if(!sum(T2Layout::resourcePath(name, "shapes/"), crc, total))
            continue;

         if(!mWhole || dynamic_cast<PlayerData *>(Sim::findObject(id)))
            continue;

         const TSMaterialList *materials = shape->materialList;
         for(U32 i = 0; materials && i < materials->size(); i++)
         {
            const String &texture = materials->getMaterialName(i);
            if(!sum(T2Layout::resourcePath(texture + ".png", "textures/"), crc, total))
               sum(T2Layout::resourcePath(texture + ".bm8", "textures/"), crc, total);
         }
      }

      conn->postNetEvent(new CRCChallengeResponseEvent(crc + total, total, mLength));
   }

   DECLARE_CONOBJECT(CRCChallengeEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(CRCChallengeEvent);

ConsoleDocClass( CRCChallengeEvent,
   "@brief Asks a client to checksum part of its install.\n\n@internal" );

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

class FogChallengeEvent : public NetEvent
{
   typedef NetEvent Parent;
   U32 mKey;

public:
   FogChallengeEvent(U32 key = 0)
   {
      mGuaranteeType = GuaranteedOrdered;
      mKey = key;
   }

   void pack(NetConnection *, BitStream *stream) { stream->writeInt(mKey, KeyBits); }
   void write(NetConnection *, BitStream *stream) { stream->writeInt(mKey, KeyBits); }
   void unpack(NetConnection *, BitStream *stream) { mKey = stream->readInt(KeyBits); }
   void process(NetConnection *) {}

   enum { KeyBits = 16 };

   DECLARE_CONOBJECT(FogChallengeEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(FogChallengeEvent);

ConsoleDocClass( FogChallengeEvent,
   "@brief Asks a client how far it can see through fog.\n\n@internal" );

//----------------------------------------------------------------------------

namespace
{
   /// What a client keeps of one target: the strings the server tagged for
   /// it, which TargetInfoEvent sends in this order, and its voice's pitch.
   struct ClientTarget
   {
      enum Tag { Name, Skin, PrefSkin, Voice, Type, NumTags };

      NetStringHandle tags[NumTags];
      F32 voicePitch;
      U32 sensorGroup;

      ClientTarget() : voicePitch(1.0f), sensorGroup(0) {}
   };

   enum { TargetBits = 9, NumTargets = 1 << TargetBits };

   ClientTarget sClientTargets[NumTargets];
}

U32 T2Targets::sensorGroup(S32 target)
{
   if (target < 0 || target >= NumTargets)
      return 0;
   return sClientTargets[target].sensorGroup;
}

void T2Targets::applySkin(ShapeBase *shape)
{
   const S32 target = shape->getT2Target();
   if (target < 0 || target >= NumTargets)
      return;

   shape->setTargetSkin(sClientTargets[target].tags[ClientTarget::Skin]);
}

class TargetInfoEvent : public NetEvent
{
   typedef NetEvent Parent;

   enum
   {
      SensorGroupBits = 5,
      DataBlockBits = 11,
      RenderMaskBits = 9,
      VoicePitchBits = 7,
   };

   U32 mTarget;
   bool mTagHere[ClientTarget::NumTags];
   U32 mTag[ClientTarget::NumTags];
   bool mVoicePitchHere;
   U32 mVoicePitch;
   bool mSensorGroupHere;
   U32 mSensorGroup;

public:
   TargetInfoEvent()
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = 0;
      for (U32 i = 0; i < ClientTarget::NumTags; i++)
      {
         mTagHere[i] = false;
         mTag[i] = 0;
      }
      mVoicePitchHere = false;
      mVoicePitch = 0;
      mSensorGroupHere = false;
      mSensorGroup = 0;
   }

   void pack(NetConnection *, BitStream *) {}
   void write(NetConnection *, BitStream *) {}
   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = stream->readInt(TargetBits);

      // Each tag is a flag for a change, then a flag for a string at all.
      for (U32 i = 0; i < ClientTarget::NumTags; i++)
         if ((mTagHere[i] = stream->readFlag()))
            mTag[i] = stream->readFlag()
                    ? stream->readInt(ConnectionStringTable::EntryBitSize) + 1
                    : 0;

      if ((mSensorGroupHere = stream->readFlag()))
         mSensorGroup = stream->readInt(SensorGroupBits);
      if (stream->readFlag() && stream->readFlag())
         stream->readInt(DataBlockBits);
      if (stream->readFlag())
         stream->readInt(RenderMaskBits);
      if ((mVoicePitchHere = stream->readFlag()))
         mVoicePitch = stream->readInt(VoicePitchBits);
   }
   void process(NetConnection *conn)
   {
      ClientTarget &target = sClientTargets[mTarget];
      const NetStringHandle wasSkin = target.tags[ClientTarget::Skin];

      for (U32 i = 0; i < ClientTarget::NumTags; i++)
         if (mTagHere[i])
            target.tags[i] = mTag[i] ? conn->translateRemoteStringId(mTag[i] - 1)
                                     : NetStringHandle();

      // A target's skin can change under a ghost already in scope: a flag
      // taken from a stand keeps its object and is reskinned where it stands.
      if (target.tags[ClientTarget::Skin] != wasSkin)
         reskinGhostsWearing(conn);

      // allocTarget takes a pitch from 0.5 to 2, sent as a share of that.
      if (mSensorGroupHere)
         target.sensorGroup = mSensorGroup;

      if (mVoicePitchHere)
         target.voicePitch = 0.5f + 1.5f * F32(mVoicePitch) /
                                    F32((1 << VoicePitchBits) - 1);
   }

   DECLARE_CONOBJECT(TargetInfoEvent);

private:
   /// Hand the new skin to every ghost already wearing this target.
   void reskinGhostsWearing(NetConnection *conn) const
   {
      if (!conn->isGhostingTo())
         return;

      for (S32 i = 0; i < NetConnection::MaxGhostCount; i++)
      {
         ShapeBase *shape = dynamic_cast<ShapeBase *>(conn->resolveGhost(i));
         if (shape && shape->getT2Target() == S32(mTarget))
            T2Targets::applySkin(shape);
      }
   }
};

IMPLEMENT_CO_CLIENTEVENT_V1(TargetInfoEvent);

ConsoleDocClass( TargetInfoEvent,
   "@brief Tells a client what one target is called and how it "
   "sounds.\n\n@internal" );

//----------------------------------------------------------------------------

class SimTargetAudioEvent : public NetEvent
{
   typedef NetEvent Parent;

   enum { DataBlockBits = 11 };

   U32 mTarget;
   U32 mFile;
   bool mDescriptionHere;
   U32 mDescription;
   bool mPositionHere;
   Point3F mPosition;

public:
   SimTargetAudioEvent()
   {
      mGuaranteeType = Unguaranteed;
      mTarget = 0;
      mFile = 0;
      mDescriptionHere = false;
      mDescription = 0;
      mPositionHere = false;
      mPosition.set(0.0f, 0.0f, 0.0f);
   }

   void pack(NetConnection *, BitStream *) {}
   void write(NetConnection *, BitStream *) {}
   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = stream->readInt(TargetBits);
      mFile = stream->readInt(ConnectionStringTable::EntryBitSize);
      if ((mDescriptionHere = stream->readFlag()))
         mDescription = stream->readInt(DataBlockBits);
      if ((mPositionHere = stream->readFlag()))
         stream->readCompressedPoint(&mPosition, 0.01f);

      // playTargetAudio's last argument, which says nothing about the sound.
      stream->readFlag();
   }
   void process(NetConnection *conn)
   {
      const ClientTarget &target = sClientTargets[mTarget];
      const char *voice = target.tags[ClientTarget::Voice].getString();
      const char *file = conn->translateRemoteStringId(mFile).getString();
      if (!voice || !voice[0] || !file || !file[0])
         return;

      SFXDescription *description = NULL;
      if (mDescriptionHere)
         Sim::findObject(DataBlockObjectIdFirst + mDescription, description);
      if (!description)
         return;

      char path[256];
      dSprintf(path, sizeof(path), "voice/%s/%s.wav", voice, file);

      SFXProfile *profile = new SFXProfile(description,
                                           StringTable->insert(path), true);
      if (!profile->registerObject())
      {
         delete profile;
         return;
      }
      profile->setAutoDelete(true);

      // A line is spoken by a shape, so it is heard from wherever that shape
      // has got to: a taunt from a player who jets away recedes with him.
      if (ShapeBase *speaker = findSpeaker(conn))
      {
         speaker->playVoiceAudio(profile, target.voicePitch);
         return;
      }

      // With no shape to speak it, the server says where it was spoken.  A
      // line with neither is one this client cannot place, and a sound at the
      // world origin is further from the truth than silence.
      if (!mPositionHere)
         return;

      MatrixF transform(true);
      transform.setColumn(3, mPosition);

      SFXSource *source = SFX->createSource(profile, &transform);
      if (!source)
         return;

      source->setPitch(target.voicePitch);
      source->play();
      SFX->deleteWhenStopped(source);
   }

   DECLARE_CONOBJECT(SimTargetAudioEvent);

private:
   /// The shape wearing this target, which any GameBase can do: a turret and
   /// a vehicle speak as well as a player.
   ShapeBase *findSpeaker(NetConnection *conn) const
   {
      if (!conn->isGhostingTo())
         return NULL;

      for (S32 i = 0; i < NetConnection::MaxGhostCount; i++)
      {
         ShapeBase *shape = dynamic_cast<ShapeBase *>(conn->resolveGhost(i));
         if (shape && shape->getT2Target() == S32(mTarget))
            return shape;
      }
      return NULL;
   }
};

IMPLEMENT_CO_CLIENTEVENT_V1(SimTargetAudioEvent);

ConsoleDocClass( SimTargetAudioEvent,
   "@brief Plays a line from a target's own voice pack.\n\n@internal" );
