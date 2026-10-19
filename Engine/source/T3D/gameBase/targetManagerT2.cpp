#include "platform/platform.h"
#include "T3D/gameBase/targetManagerT2.h"

#include "console/console.h"
#include "console/simBase.h"
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

   return id;
}

void TargetManagerT2::freeTarget(S32 id)
{
   // 0x66d390.  The object loses its target as GameBase::setTargetInfo(NULL)
   // has it (0x5e37e0); any other object still naming the id keeps it.
   Target &target = mTargets[id];
   if(GameBase *obj = target.object)
      obj->mTarget = -1;

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
}

void TargetManagerT2::setTargetObject(S32 id, GameBase *obj)
{
   // 0x66d910.
   mTargets[id].object = obj;
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
   object->setSensorGroup(dAtoi(argv[2]));
}

ConsoleMethod(GameConnection, getSensorGroup, S32, 2, 2, "conn.getSensorGroup()")
{
   // 0x5fdcd0.
   return object->getSensorGroup();
}
