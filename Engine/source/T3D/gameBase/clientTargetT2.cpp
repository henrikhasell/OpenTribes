// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/clientTargetT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"
#include "sim/netStringTable.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/targetsT2.h"

IMPLEMENT_CONOBJECT(ClientTarget);

ConsoleDocClass( ClientTarget,
   "@brief A server target or a place the commander map gives orders "
   "about.\n\n@internal" );

Vector<ClientTarget *> ClientTarget::smTyped(__FILE__, __LINE__);
// A client's own targets are numbered on from the server's 512.
S32 ClientTarget::smNextLocalId = 512;

namespace
{
   const char *sTypeNames[ClientTarget::NumKinds] =
   {
      "CMDMAP__LocationType", "CMDMAP__WayPointType",
      "CMDMAP__PotentialTaskType", "CMDMAP__AssignedTaskType",
   };
   const char *sTypeVars[ClientTarget::NumKinds] =
   {
      "$CMD_LOCATIONTYPEID", "$CMD_WAYPOINTTYPEID",
      "$CMD_POTENTIALTASKTYPEID", "$CMD_ASSIGNEDTASKTYPEID",
   };
   NetStringHandle sTypes[ClientTarget::NumKinds];
}

void ClientTarget::initTypes()
{
   if (sTypes[0].isValidString() || !gNetStringTable)
      return;

   for (U32 i = 0; i < NumKinds; i++)
   {
      sTypes[i] = NetStringHandle(sTypeNames[i]);
      Con::setIntVariable(sTypeVars[i], sTypes[i].getIndex());
   }
}

U32 ClientTarget::typeId(Kind kind)
{
   initTypes();
   return sTypes[kind].getIndex();
}

ClientTarget::ClientTarget()
{
   mServerTarget = -1;
   mLocalId = -1;
   mType = 0;
   mPosition.zero();
   mText = StringTable->EmptyString();
}

S32 ClientTarget::getTargetId() const
{
   return mLocalId != -1 ? mLocalId : mServerTarget;
}

bool ClientTarget::makeTyped(U32 type, const char *text)
{
   if (mLocalId == -1)
   {
      mLocalId = smNextLocalId++;
      smTyped.push_back(this);
   }

   mType = type;
   mText = StringTable->insert(text ? text : "");
   return true;
}

void ClientTarget::onRemove()
{
   if (mLocalId != -1)
      for (S32 i = 0; i < smTyped.size(); i++)
         if (smTyped[i] == this)
         {
            smTyped.erase(i);
            break;
         }

   Parent::onRemove();
}

//----------------------------------------------------------------------------

/// What the next order from this client is about: a flag for a target, then
/// its id, or the place it is at.
class SetServerTargetEvent : public NetEvent
{
   typedef NetEvent Parent;

   S32 mTarget;
   Point3F mPosition;

public:
   enum { TargetBits = 9 };

   SetServerTargetEvent(S32 target = -1, const Point3F &pos = Point3F::Zero)
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = target;
      mPosition = pos;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      if (stream->writeFlag(mTarget >= 0))
         stream->writeInt(mTarget, TargetBits);
      else
      {
         stream->write(mPosition.x);
         stream->write(mPosition.y);
         stream->write(mPosition.z);
      }
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = -1;
      if (stream->readFlag())
         mTarget = stream->readInt(TargetBits);
      else
      {
         stream->read(&mPosition.x);
         stream->read(&mPosition.y);
         stream->read(&mPosition.z);
      }
   }
   void process(NetConnection *) {}

   DECLARE_CONOBJECT(SetServerTargetEvent);
};

IMPLEMENT_CO_NETEVENT_V1(SetServerTargetEvent);

ConsoleDocClass( SetServerTargetEvent,
   "@brief Tells a server what a client's next order is about.\n\n@internal" );

void ClientTarget::sendToServer()
{
   NetConnection *conn = NetConnection::getConnectionToServer();
   if (!conn)
      return;

   const bool server = mServerTarget >= 0 && T2Targets::isAllocated(mServerTarget);
   conn->postNetEvent(new SetServerTargetEvent(server ? mServerTarget : -1, mPosition));
}

//----------------------------------------------------------------------------

DefineEngineMethod( ClientTarget, getTargetId, S32, (), ,
   "The server's target, -1 for a place, or this client's own id for one." )
{
   return object->getTargetId();
}

DefineEngineMethod( ClientTarget, sendToServer, void, (), ,
   "Tell the server what the next order is about." )
{
   object->sendToServer();
}

DefineEngineMethod( ClientTarget, createWaypoint, bool, (const char *text), ,
   "Make this place a waypoint called text." )
{
   if (object->isTyped())
   {
      Con::errorf("ClientTarget::cCreateWaypoint: target already typed");
      return false;
   }
   return object->makeTyped(ClientTarget::typeId(ClientTarget::WayPoint), text);
}

DefineEngineMethod( ClientTarget, addPotentialTask, bool, (), ,
   "Show this task on the commander map until it is answered." )
{
   if (object->isTyped() &&
       object->getType() != ClientTarget::typeId(ClientTarget::PotentialTask))
      return true;
   return object->makeTyped(ClientTarget::typeId(ClientTarget::PotentialTask),
                            object->getText());
}

DefineEngineFunction( createClientTarget, S32, (S32 targetId, Point3F pos), (-1, Point3F::Zero),
   "createClientTarget(targetId, <x y z>)" )
{
   ClientTarget::initTypes();

   ClientTarget *target = new ClientTarget;
   target->setServerTarget(targetId);
   target->setPosition(pos);
   target->setType(targetId >= 0 ? T2Targets::typeTag(targetId)
                                 : ClientTarget::typeId(ClientTarget::Location));
   if (!target->registerObject())
   {
      delete target;
      return 0;
   }
   return target->getId();
}

//----------------------------------------------------------------------------

DefineEngineMethod( GameConnection, scopeCommanderMap, void, (bool scope), ,
   "Scope every object this client's team knows of, for the commander map." )
{
   object->setScopingCommanderMap(scope);
}

DefineEngineMethod( GameConnection, isScopingCommanderMap, bool, (), ,
   "Whether this client is scoping for the commander map." )
{
   return object->isScopingCommanderMap();
}
