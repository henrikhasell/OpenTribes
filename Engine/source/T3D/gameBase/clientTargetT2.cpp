// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/clientTargetT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"
#include "sim/netStringTable.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/targetManagerT2.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/shapeBase.h"
#include "gui/commander/guiCommanderMap.h"

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

namespace
{
   enum { TargetBits = 9 };

   void writeTarget(BitStream *stream, S32 target)
   {
      if (stream->writeFlag(target != -1))
         stream->writeInt(target, TargetBits);
   }
   S32 readTarget(BitStream *stream)
   {
      return stream->readFlag() ? S32(stream->readInt(TargetBits)) : -1;
   }

   void writePosition(BitStream *stream, const Point3F &pos)
   {
      stream->write(pos.x);
      stream->write(pos.y);
      stream->write(pos.z);
   }
   void readPosition(BitStream *stream, Point3F *pos)
   {
      stream->read(&pos->x);
      stream->read(&pos->y);
      stream->read(&pos->z);
   }
}

/// What the next order from this client is about: a flag and the target's id
/// where there is one, then the place, which is sent either way.
class SetServerTargetEvent : public NetEvent
{
   typedef NetEvent Parent;

   S32 mTarget;
   Point3F mPosition;

public:
   SetServerTargetEvent(S32 target = -1, const Point3F &pos = Point3F::Zero)
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = target;
      mPosition = pos;
   }

   void pack(NetConnection *, BitStream *stream)
   {
      writeTarget(stream, mTarget);
      writePosition(stream, mPosition);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = readTarget(stream);
      readPosition(stream, &mPosition);
   }
   void process(NetConnection *conn)
   {
      GameConnection *client = dynamic_cast<GameConnection *>(conn);
      if (client && !client->isConnectionToServer())
         client->setServerTarget(mTarget, mPosition);
   }

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

/// A task for this client: its target, the place unless the client already
/// has the target's object to take it from, and whether the task is now the
/// client's own or one it may take.
class TargetToEvent : public NetEvent
{
   typedef NetEvent Parent;

   S32 mTarget;
   Point3F mPosition;
   bool mAssign;

public:
   TargetToEvent(S32 target = -1, const Point3F &pos = Point3F::Zero,
                 bool assign = false)
   {
      mGuaranteeType = GuaranteedOrdered;
      mTarget = target;
      mPosition = pos;
      mAssign = assign;
   }

   void pack(NetConnection *conn, BitStream *stream)
   {
      S32 ghost = -1;
      if (mTarget >= 0 && mTarget < TargetManagerT2::MaxTargets)
         if (GameBase *obj = TargetManagerT2::server()->getTarget(mTarget).object)
            ghost = conn->getGhostIndex(obj);

      writeTarget(stream, mTarget);
      if (stream->writeFlag(ghost == -1))
         writePosition(stream, mPosition);
      stream->writeFlag(mAssign);
   }
   void write(NetConnection *conn, BitStream *stream) { pack(conn, stream); }
   void unpack(NetConnection *, BitStream *stream)
   {
      mTarget = readTarget(stream);
      if (stream->readFlag())
         readPosition(stream, &mPosition);
      else if (ShapeBase *shape = GuiCommanderMap::ghostOf(mTarget))
         mPosition = shape->getWorldBox().getCenter();
      mAssign = stream->readFlag();
   }
   void process(NetConnection *conn)
   {
      ClientTarget *target = new ClientTarget;
      target->setServerTarget(mTarget);
      target->setPosition(mPosition);
      if (!target->registerObject())
      {
         delete target;
         return;
      }
      conn->addObject(target);

      const ClientTarget::Kind kind = mAssign ? ClientTarget::AssignedTask
                                              : ClientTarget::PotentialTask;
      target->makeTyped(ClientTarget::typeId(kind), "");
      Con::executef(target, "onAdd", mAssign ? "AssignedTask" : "PotentialTask");
   }

   DECLARE_CONOBJECT(TargetToEvent);
};

IMPLEMENT_CO_NETEVENT_V1(TargetToEvent);

ConsoleDocClass( TargetToEvent,
   "@brief Gives a client a task from its server.\n\n@internal" );

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

DefineEngineMethod( ClientTarget, setText, void, (const char *text), ,
   "Set what the map and the HUD say this target is." )
{
   object->setText(text);
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

DefineEngineMethod( GameConnection, getTargetId, S32, (), ,
   "The target this client's next order is about, -1 for a place." )
{
   return object->isConnectionToServer() ? -1 : object->getServerTarget();
}

DefineEngineMethod( GameConnection, setTargetId, void, (S32 targetId), ,
   "Make this client's next order about targetId." )
{
   if (targetId < -1 || targetId >= TargetManagerT2::MaxTargets)
   {
      Con::errorf("GameConnection::cSetTargetId: invalid target id [%d]", targetId);
      return;
   }
   object->setServerTarget(targetId, object->getServerTargetPos());
}

DefineEngineMethod( GameConnection, getTargetPos, const char *, (), ,
   "The place this client's next order is about." )
{
   if (object->isConnectionToServer())
      return "";
   const Point3F &pos = object->getServerTargetPos();
   char *ret = Con::getReturnBuffer(128);
   dSprintf(ret, 128, "%f %f %f", pos.x, pos.y, pos.z);
   return ret;
}

DefineEngineMethod( GameConnection, sendTargetTo, void, (GameConnection *conn, bool assign), ,
   "Give conn this client's order target as a task, its own if assign." )
{
   const S32 target = object->getServerTarget();
   if (!conn || target < -1 || target >= TargetManagerT2::MaxTargets ||
       object->isConnectionToServer())
      return;
   conn->postNetEvent(new TargetToEvent(target, object->getServerTargetPos(), assign));
}
