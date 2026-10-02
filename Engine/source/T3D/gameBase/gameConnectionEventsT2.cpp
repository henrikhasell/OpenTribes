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

class GravityEvent : public NetEvent
{
   typedef NetEvent Parent;
   F32 mGravity;

public:
   GravityEvent(F32 gravity = 0.0f)
   {
      mGuaranteeType = GuaranteedOrdered;
      mGravity = gravity;
   }

   void pack(NetConnection *, BitStream *stream) { stream->write(mGravity); }
   void write(NetConnection *, BitStream *stream) { stream->write(mGravity); }
   void unpack(NetConnection *, BitStream *stream) { stream->read(&mGravity); }
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(GravityEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(GravityEvent);

ConsoleDocClass( GravityEvent,
   "@brief Gives a client the mission's gravity.\n\n@internal" );

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

   void process(NetConnection *conn)
   {
      conn->postNetEvent(new CRCChallengeResponseEvent(mSeed, 0, mLength));
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
