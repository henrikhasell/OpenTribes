#include "platform/platform.h"
#include "T3D/gameBase/targetManagerT2.h"

#include "console/console.h"
#include "console/simBase.h"
#include "console/simDatablock.h"
#include "core/strings/stringFunctions.h"
#include "T3D/gameBase/gameBase.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/shapeBase.h"

TargetManagerT2::TargetManagerT2()
{
   reset();
}

TargetManagerT2 *TargetManagerT2::server()
{
   // Retail builds its manager once at startup (0x66fcb0) and keeps it for
   // the life of the process.  Never destroyed: its handles would outlive
   // the string table and the objects they point at.
   static TargetManagerT2 *sServer = new TargetManagerT2;
   return sServer;
}

void TargetManagerT2::clearTarget(Target &target)
{
   target.name     = NetStringHandle();
   target.skin     = NetStringHandle();
   target.prefSkin = NetStringHandle();
   target.voice    = NetStringHandle();
   target.type     = NetStringHandle();
   target.voicePitch  = 1.0f;
   target.sensorGroup = 0;
   target.allocated   = false;
   target.object      = NULL;
   target.dataBlock   = NULL;
   target.alwaysVisMask = 0;
   target.neverVisMask  = 0;
   target.friendlyMask  = 0;
   target.renderMask    = 0;
   target.sensorData    = NULL;
}

void TargetManagerT2::reset()
{
   // 0x66c4a0.
   for(U32 i = 0; i < BlockWords; i++)
      mBlock[i] = 0;
   freeCount() = MaxTargets;
   for(U32 i = 0; i < MaxTargets; i++)
      clearTarget(mTargets[i]);

   // The team targets take the first word whole, so allocation starts at
   // 32, yet they stay unallocated: the validator lets them be read.
   freeCount() -= TeamTargets;
   allocMask(0) = 0xffffffff;

   for(U32 i = 0; i < SensorGroups; i++)
   {
      mTargets[i].sensorGroup = i;
      groupMask(BlockFriendly, i)  = 1u << i;
      groupMask(BlockAlwaysVis, i) = 1u << i;
      groupMask(BlockListen, i)    = 1u << i;
      groupMask(BlockNeverVis, i)  = 0;
   }
}

void TargetManagerT2::resetTargets()
{
   // 0x66d830.  Retail first sends each client connection the event that
   // clears its copy of the targets (0x66d610); that is V34e.  Each object
   // then loses its target as setTargetInfo(NULL) has it (0x5e37e0), so an
   // object outliving the reset is not left naming an id reused later.
   broadcastReset();
   for(U32 i = 0; i < MaxTargets; i++)
      if(GameBase *obj = mTargets[i].object)
         obj->mTarget = -1;

   reset();
}

void TargetManagerT2::takeGroupMasks(Target &target, S32 group) const
{
   // 0x66d070 and 0x66dcc0: each mask is read at its table + group * 4.
   // allocTarget never checks the group, so one outside 0..31 reads the
   // neighbouring table, as it does here.  Past the run retail reads the
   // per-group records below it or target 0 above it; those are not
   // modelled and read as 0.
   const auto read = [&](S32 table) -> U32
   {
      const S64 index = (S64)table + group;
      return index >= 0 && index < BlockWords ? mBlock[index] : 0;
   };
   target.alwaysVisMask = read(BlockAlwaysVis);
   target.neverVisMask  = read(BlockNeverVis);
   target.friendlyMask  = read(BlockFriendly);
}

S32 TargetManagerT2::allocTarget(const NetStringHandle &name,
                                 const NetStringHandle &skin,
                                 const NetStringHandle &voice,
                                 const NetStringHandle &type,
                                 S32 sensorGroup, S32 dataBlockId,
                                 F32 voicePitch,
                                 const NetStringHandle &prefSkin)
{
   // 0x66ceb0.
   if(freeCount() == 0)
      return -1;

   S32 id = -1;
   for(U32 word = 0; word < MaskWords && id == -1; word++)
   {
      if(allocMask(word) == 0xffffffff)
         continue;
      for(U32 bit = 0; bit < 32; bit++)
         if(!(allocMask(word) & (1u << bit)))
         {
            id = word * 32 + bit;
            allocMask(word) |= 1u << bit;
            break;
         }
   }
   freeCount()--;

   Target &target = mTargets[id];
   target.allocated   = true;
   target.name        = name;
   target.skin        = skin;
   target.voice       = voice;
   target.type        = type;
   target.prefSkin    = prefSkin;
   target.sensorGroup = sensorGroup;
   target.voicePitch  = voicePitch;
   takeGroupMasks(target, sensorGroup);

   ShapeBaseData *data = NULL;
   if(dataBlockId != -1 && dataBlockId != -2)
      Sim::findObject(dataBlockId, data);
   target.dataBlock = data;

   broadcastInfo(id, FieldAll);
   return id;
}

void TargetManagerT2::freeTarget(S32 id)
{
   // 0x66d390.  The object loses its target as GameBase::setTargetInfo(NULL)
   // has it (0x5e37e0); any other object still naming the id keeps it.
   Target &target = mTargets[id];
   if(GameBase *obj = target.object)
      obj->mTarget = -1;

   broadcastFree(id);
   clearTarget(target);

   const U32 bit = 1u << (id & 31);
   if(allocMask(id >> 5) & bit)
   {
      allocMask(id >> 5) &= ~bit;
      freeCount()++;
   }
}

void TargetManagerT2::setSensorGroup(S32 id, S32 group)
{
   // 0x66dcc0.
   Target &target = mTargets[id];
   if(target.sensorGroup == group)
      return;

   target.sensorGroup = group;
   takeGroupMasks(target, group);
   broadcastInfo(id, FieldSensorGroup);
}

void TargetManagerT2::setName(S32 id, const NetStringHandle &name)
{
   // 0x66ea90 and 0x66d9a0.
   Target &target = mTargets[id];
   if(target.name == name)
      return;

   target.name = name;
   broadcastInfo(id, FieldName);
}

void TargetManagerT2::setSkin(S32 id, const NetStringHandle &skin)
{
   // 0x66ec30 and 0x66da70.
   Target &target = mTargets[id];
   if(target.skin == skin)
      return;

   target.skin = skin;
   broadcastInfo(id, FieldSkin);
}

void TargetManagerT2::setRenderMask(S32 id, U32 mask)
{
   // 0x66fb00 and 0x66ddf0.  Retail sends it even when nothing changed.
   mTargets[id].renderMask = mask & 0x1ff;
   broadcastInfo(id, FieldRenderMask);
}

void TargetManagerT2::sendTargetsTo(GameConnection *conn)
{
   // 0x66e3a0 and 0x66ccc0.  Nothing is sent to a connection not yet told
   // about targets, and the scripts ask for it once its data blocks are in.
   if(!conn->isEstablished() || !conn->getReceivedDataBlocks())
      return;

   for(S32 id = 0; id < MaxTargets; id++)
      if(allocMask(id >> 5) & (1u << (id & 31)))
         conn->sendTargetInfo(id, FieldAll);
}

void TargetManagerT2::broadcastInfo(S32 id, U32 fields)
{
   SimGroup *clients = Sim::getClientGroup();
   for(S32 i = 0; i < clients->size(); i++)
      if(GameConnection *conn = dynamic_cast<GameConnection *>((*clients)[i]))
         if(conn->isEstablished() && conn->getReceivedDataBlocks())
            conn->sendTargetInfo(id, fields);
}

void TargetManagerT2::broadcastFree(S32 id)
{
   SimGroup *clients = Sim::getClientGroup();
   for(S32 i = 0; i < clients->size(); i++)
      if(GameConnection *conn = dynamic_cast<GameConnection *>((*clients)[i]))
         conn->sendTargetFree(id);
}

void TargetManagerT2::broadcastReset()
{
   SimGroup *clients = Sim::getClientGroup();
   for(S32 i = 0; i < clients->size(); i++)
      if(GameConnection *conn = dynamic_cast<GameConnection *>((*clients)[i]))
         if(conn->isEstablished() && conn->getReceivedDataBlocks())
            conn->sendResetTargets(false);
}

void TargetManagerT2::setTargetObject(S32 id, GameBase *obj)
{
   // 0x66d910.
   mTargets[id].object = obj;
}

void TargetManagerT2::setGroupMask(GroupMask which, S32 group, U32 mask)
{
   // 0x66df80, then 0x66e020, 0x66e060 or 0x66e0a0: every target in the
   // group takes the new mask, team targets too, and so does each one
   // allocated into it later.
   static const S32 tables[] = { BlockAlwaysVis, BlockNeverVis, BlockFriendly };
   for(U32 id = 0; id < MaxTargets; id++)
   {
      Target &target = mTargets[id];
      if(!(allocMask(id >> 5) & (1u << (id & 31))) || target.sensorGroup != group)
         continue;
      if(which == GroupAlwaysVis)
         target.alwaysVisMask = mask;
      else if(which == GroupNeverVis)
         target.neverVisMask = mask;
      else
         target.friendlyMask = mask;
   }
   groupMask(tables[which], group) = mask;
}

void TargetManagerT2::setGroupColours(S32 group, U32 mask, const ColorI &colour)
{
   // 0x66f950, with 0x66ca50 telling the connections. Only the viewer group's
   // own connections are sent its row, and a connection that joins later is
   // sent it with its group.
   for(U32 i = 0; i < SensorGroups; i++)
      if(mask & (1u << i))
         mColours[group][i] = colour;

   SimGroup *clients = Sim::getClientGroup();
   for(S32 i = 0; i < clients->size(); i++)
      if(GameConnection *conn = dynamic_cast<GameConnection *>((*clients)[i]))
         if(conn->getSensorGroup() == group)
            conn->sendSensorGroupColours(group, mask, mColours[group]);
}

bool TargetManagerT2::isTargetVisible(S32 id, S32 group) const
{
   // 0x66e130.  Past what a group is never or always shown, retail asks
   // what its sensors see of the target; nothing on this server senses
   // yet, so nothing else is seen.
   const Target &target = mTargets[id];
   const U32 bit = 1u << group;
   if(target.neverVisMask & bit)
      return false;
   return (target.alwaysVisMask & bit) != 0;
}

bool TargetManagerT2::isTargetFriendly(S32 id, S32 group) const
{
   // 0x66e100.
   return (mTargets[id].friendlyMask & (1u << group)) != 0;
}

//----------------------------------------------------------------------------

// Defined here, beside the manager they keep in step with.

S32 GameBase::getTarget() const
{
   return mTarget;
}

void GameBase::setTarget(S32 target)
{
   // 0x5e2fa0.  Only targets past the team ones learn their object.
   if(mTarget == target)
      return;

   mTarget = target;
   if(mTarget >= TargetManagerT2::TeamTargets)
      TargetManagerT2::server()->setTargetObject(mTarget, this);
}

//----------------------------------------------------------------------------

namespace
{

/// The checks every per-target function makes first (0x66e180). A getter
/// passes mutate false; even a getter fails on an unallocated target past
/// the team ones.
S32 validateTarget(S32 id, const char *fn, bool mutate)
{
   if(id < 0 || id >= TargetManagerT2::MaxTargets)
   {
      Con::errorf("TargetManager::%s: invalid target index [%d]", fn, id);
      return -1;
   }
   if(mutate && id < TargetManagerT2::TeamTargets)
   {
      Con::errorf("TargetManager::%s: cannot change attribute on team target [%d]", fn, id);
      return -1;
   }
   if(!TargetManagerT2::server()->getTarget(id).allocated &&
      id >= TargetManagerT2::TeamTargets)
   {
      Con::errorf("TargetManager::%s: cannot change attribute on unallocated target [%d]", fn, id);
      return -1;
   }
   return id;
}

S32 validateSensorGroup(S32 group, const char *fn)
{
   // 0x66e220.
   if(group < 0 || group >= TargetManagerT2::SensorGroups)
   {
      Con::errorf("TargetManager::%s: invalid sensor group [%d]", fn, group);
      return -1;
   }
   return group;
}

/// A tagged string names its entry after the tag byte; retail reads any
/// other non-empty string as an entry number too.
NetStringHandle tagArg(const char *arg)
{
   U32 index = 0;
   if(arg[0] == StringTagPrefixByte)
      index = dAtoi(arg + 1);
   else if(arg[0])
      index = dAtoi(arg);
   return NetStringHandle(index);
}

}

ConsoleFunction(allocTarget, S32, 8, 9,
   "allocTarget(nameTag, skinTag, voiceTag, typeTag, sensorGroup, dataBlockId, voicePitch, [prefskin])")
{
   // 0x66e3f0.
   NetStringHandle name  = tagArg(argv[1]);
   NetStringHandle skin  = tagArg(argv[2]);
   NetStringHandle voice = tagArg(argv[3]);
   NetStringHandle type  = tagArg(argv[4]);
   const S32 sensorGroup = dAtoi(argv[5]);

   // Anything that is not a datablock is no datablock at all.
   S32 dataBlockId = -1;
   SimDataBlock *data = NULL;
   if(Sim::findObject(dAtoi(argv[6]), data))
      dataBlockId = data->getId();

   F32 voicePitch = dAtof(argv[7]);
   if(voicePitch < 0.5f || voicePitch > 2.0f)
      voicePitch = 1.0f;

   NetStringHandle prefSkin;
   if(argc == 9)
      prefSkin = tagArg(argv[8]);

   return TargetManagerT2::server()->allocTarget(name, skin, voice, type,
      sensorGroup, dataBlockId, voicePitch, prefSkin);
}

ConsoleFunction(freeTarget, void, 2, 2, "freeTarget(targetId)")
{
   // 0x66e960.
   const S32 id = validateTarget(dAtoi(argv[1]), "cFreeTarget", true);
   if(id == -1)
      return;
   TargetManagerT2::server()->freeTarget(id);
}

ConsoleFunction(resetTargets, void, 1, 1, "resetTargets()")
{
   // 0x66e2c0.  The scripts' resetTargetManager, at every mission load and
   // when the server goes down.
   TargetManagerT2::server()->resetTargets();
}

ConsoleFunction(getTargetSensorGroup, S32, 2, 2, "getTargetSensorGroup(targetId)")
{
   // 0x66f0a0.
   const S32 id = validateTarget(dAtoi(argv[1]), "cGetTargetSensorGroup", false);
   if(id == -1)
      return -1;
   return TargetManagerT2::server()->getTarget(id).sensorGroup;
}

ConsoleFunction(setTargetSensorGroup, void, 3, 3, "setTargetSensorGroup(targetId, sensorGroup)")
{
   // 0x66f0e0.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetSensorGroup", true);
   if(id == -1)
      return;
   const S32 group = validateSensorGroup(dAtoi(argv[2]), "cSetTargetSensorGroup");
   if(group == -1)
      return;
   TargetManagerT2::server()->setSensorGroup(id, group);
}

ConsoleFunction(setTargetSensorData, void, 3, 3, "setTargetSensorData(targetId, sensorData)")
{
   // 0x66f720.  Anything that is not a datablock is no sensor at all.
   const S32 id = dAtoi(argv[1]);
   if(id < 0 || id >= TargetManagerT2::MaxTargets)
      return;
   SimDataBlock *data = NULL;
   Sim::findObject(argv[2], data);
   TargetManagerT2::server()->getTarget(id).sensorData = data;
}

ConsoleFunction(setTargetAlwaysVisMask, void, 3, 3, "setTargetAlwaysVisMask(target, mask)")
{
   // 0x66f290.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetAlwaysVisMask", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->getTarget(id).alwaysVisMask = dAtoui(argv[2]);
}

ConsoleFunction(setTargetNeverVisMask, void, 3, 3, "setTargetNeverVisMask(target, mask)")
{
   // 0x66f320.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetNeverVisMask", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->getTarget(id).neverVisMask = dAtoui(argv[2]);
}

ConsoleFunction(setTargetFriendlyMask, void, 3, 3, "setTargetFriendlyMask(target, mask)")
{
   // 0x66f3b0.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetFriendlyMask", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->getTarget(id).friendlyMask = dAtoui(argv[2]);
}

ConsoleFunction(setSensorGroupAlwaysVisMask, void, 3, 3, "setSensorGroupAlwaysVisMask(sensorGroup, mask)")
{
   // 0x66f440.
   const S32 group = validateSensorGroup(dAtoi(argv[1]), "cSetSensorGroupAlwaysVisMask");
   if(group == -1)
      return;
   TargetManagerT2::server()->setGroupMask(TargetManagerT2::GroupAlwaysVis, group, dAtoui(argv[2]));
}

ConsoleFunction(setSensorGroupFriendlyMask, void, 3, 3, "setSensorGroupFriendlyMask(sensorGroup, mask)")
{
   // 0x66f560.
   const S32 group = validateSensorGroup(dAtoi(argv[1]), "cSetSensorGroupFriendlyMask");
   if(group == -1)
      return;
   TargetManagerT2::server()->setGroupMask(TargetManagerT2::GroupFriendly, group, dAtoui(argv[2]));
}

ConsoleFunction(setSensorGroupColor, void, 4, 4, "setSensorGroupColor(sensorGroup, groupMask, color)")
{
   // 0x66f950.  The colour is "r g b a" as the script gives it. A value it
   // leaves off is 0, and alpha is 255. Retail's value there is not established.
   const S32 group = validateSensorGroup(dAtoi(argv[1]), "cSetSensorGroupColor");
   if(group == -1)
      return;

   S32 r = 0, g = 0, b = 0, a = 255;
   dSscanf(argv[3], "%d %d %d %d", &r, &g, &b, &a);
   TargetManagerT2::server()->setGroupColours(group, dAtoui(argv[2]),
      ColorI((U8)r, (U8)g, (U8)b, (U8)a));
}

ConsoleFunction(setSensorGroupCount, void, 2, 2, "setSensorGroupCount(count)")
{
   // 0x66f210.  Zero up to every group, team zero included, is valid.
   const S32 count = dAtoi(argv[1]);
   if(count < 0 || count > TargetManagerT2::SensorGroups)
   {
      Con::errorf("TargetManager::cSetSensorGroupCount: invalid group count [%d]", count);
      return;
   }
   TargetManagerT2::server()->setGroupCount(count);
}

ConsoleFunction(setTargetName, void, 3, 3, "setTargetName(targetId, nameTag)")
{
   // 0x66ea90.  Team targets are named too; the checks let them be changed.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetName", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->setName(id, tagArg(argv[2]));
}

ConsoleFunction(setTargetSkin, void, 3, 3, "setTargetSkin(targetId, skinTag)")
{
   // 0x66ec30.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetSkin", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->setSkin(id, tagArg(argv[2]));
}

ConsoleFunction(getTargetRenderMask, S32, 2, 2, "getTargetRenderMask(targetId)")
{
   // 0x66fab0.
   const S32 id = validateTarget(dAtoi(argv[1]), "cGetTargetRender", false);
   if(id == -1)
      return -1;
   return TargetManagerT2::server()->getTarget(id).renderMask;
}

ConsoleFunction(setTargetRenderMask, void, 3, 3, "setTargetRenderMask(targetId, mask)")
{
   // 0x66fb00.
   const S32 id = validateTarget(dAtoi(argv[1]), "cSetTargetRender", false);
   if(id == -1)
      return;
   TargetManagerT2::server()->setRenderMask(id, dAtoui(argv[2]));
}

ConsoleFunction(sendTargetsToClient, void, 2, 2, "sendTargetsToClient(client)")
{
   // 0x66e3a0.  The scripts send each client its targets once its data
   // blocks are in.
   GameConnection *conn = NULL;
   if(!Sim::findObject(argv[1], conn))
      return;
   TargetManagerT2::server()->sendTargetsTo(conn);
}

ConsoleFunction(resetClientTargets, void, 3, 3, "resetClientTargets(client, tasksOnly)")
{
   // 0x66e2e0.  tasksOnly asks for the commander's tasks and waypoints, which
   // a client does not keep, so the client clears nothing for it.
   GameConnection *conn = NULL;
   if(!Sim::findObject(argv[1], conn))
      return;
   conn->sendResetTargets(dAtob(argv[2]));
}

ConsoleMethod(GameBase, setTarget, void, 3, 3, "obj.setTarget(targetId)")
{
   // 0x5e3570.  A ghost's target is the server's to say.
   if(object->isGhost())
      return;

   const S32 id = dAtoi(argv[2]);
   if(id < -1 || id >= TargetManagerT2::MaxTargets)
   {
      Con::errorf("GameBase::cSetTargetId: invalid target id [%s]", argv[2]);
      return;
   }
   object->setTarget(id);
}

ConsoleMethod(GameBase, getTarget, S32, 2, 2, "obj.getTarget()")
{
   // 0x5e35d0.
   return object->getTarget();
}

ConsoleMethod(GameConnection, setSensorGroup, void, 3, 3, "conn.setSensorGroup(groupId)")
{
   // 0x5fdc90.  Unchecked: the scripts pass team numbers.  The connection
   // to a server is not one a server can place in a group.
   if(object == GameConnection::getConnectionToServer())
      return;
   object->assignSensorGroup(dAtoi(argv[2]));
}

ConsoleMethod(GameConnection, getSensorGroup, S32, 2, 2, "conn.getSensorGroup()")
{
   // 0x5fdcd0.
   return object->getSensorGroup();
}

ConsoleMethod(GameConnection, getReceivedDataBlocks, bool, 2, 2, "conn.getReceivedDataBlocks()")
{
   return object->getReceivedDataBlocks();
}

ConsoleMethod(GameConnection, setReceivedDataBlocks, void, 3, 3, "conn.setReceivedDataBlocks(received)")
{
   object->setReceivedDataBlocks(dAtob(argv[2]));
}
