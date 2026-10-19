#include "platform/platform.h"
#include "T3D/gameBase/targetManagerT2.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "core/strings/stringFunctions.h"
#include "math/mMath.h"
#include "scene/sceneContainer.h"
#include "T3D/gameBase/gameBase.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/objectTypes.h"
#include "T3D/shapeBase.h"

IMPLEMENT_CO_DATABLOCK_V1(SensorData);

ConsoleDocClass( SensorData,
   "@brief What a Tribes 2 target's object detects and jams.\n\n@internal" );

SensorData::SensorData()
{
   // 0x66a180.
   detects              = true;
   detectsUsingLOS      = true;
   detectsPassiveJammed = false;
   detectsActiveJammed  = false;
   detectsCloaked       = false;
   detectionPings       = true;
   detectsFOVOnly       = false;
   detectMinVelocity    = 0.0f;
   detectRadius         = 250.0f;
   detectFOV            = 90.0f;
   jams                 = false;
   jamsOnlyGroup        = false;
   jamsUsingLOS         = false;
   jamRadius            = 0.0f;
   detectFOVPercent     = 0.0f;
   useObjectFOV         = false;

   minVelocitySquared  = 0.0f;
   detectRadiusSquared = 0.0f;
   cosHalfFOV          = 0.0f;
   jamRadiusSquared    = 0.0f;
}

bool SensorData::onAdd()
{
   if(!Parent::onAdd())
      return false;

   // 0x66a1f0, in its order: the half angle goes through float before the
   // cosine, as retail stores it.
   const F32 halfFOV = F32((F64)detectFOV * 0.5 * M_PI * (1.0 / 180.0));
   cosHalfFOV          = F32(mCos((F64)halfFOV));
   minVelocitySquared  = detectMinVelocity * detectMinVelocity;
   jamRadiusSquared    = jamRadius * jamRadius;
   detectRadiusSquared = detectRadius * detectRadius;
   return true;
}

void SensorData::initPersistFields()
{
   // 0x66a270.
   Parent::initPersistFields();

   addField("detects",              TypeBool, Offset(detects, SensorData));
   addField("detectsUsingLOS",      TypeBool, Offset(detectsUsingLOS, SensorData));
   addField("detectsActiveJammed",  TypeBool, Offset(detectsActiveJammed, SensorData));
   addField("detectsPassiveJammed", TypeBool, Offset(detectsPassiveJammed, SensorData));
   addField("detectsCloaked",       TypeBool, Offset(detectsCloaked, SensorData));
   addField("detectionPings",       TypeBool, Offset(detectionPings, SensorData));
   addField("detectsFOVOnly",       TypeBool, Offset(detectsFOVOnly, SensorData));
   addField("jams",                 TypeBool, Offset(jams, SensorData));
   addField("jamsOnlyGroup",        TypeBool, Offset(jamsOnlyGroup, SensorData));
   addField("jamsUsingLOS",         TypeBool, Offset(jamsUsingLOS, SensorData));
   addField("jamRadius",            TypeF32,  Offset(jamRadius, SensorData));
   addField("detectFOV",            TypeF32,  Offset(detectFOV, SensorData));
   addField("detectRadius",         TypeF32,  Offset(detectRadius, SensorData));
   addField("detectMinVelocity",    TypeF32,  Offset(detectMinVelocity, SensorData));
   addField("detectFOVPercent",     TypeF32,  Offset(detectFOVPercent, SensorData));
   addField("useObjectFOV",         TypeBool, Offset(useObjectFOV, SensorData));
}

//----------------------------------------------------------------------------

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
   target.visibleMask   = 0;
   target.alwaysVisMask = 0;
   target.neverVisMask  = 0;
   target.friendlyMask  = 0;
   target.sensorData    = NULL;
}

void TargetManagerT2::reset()
{
   // 0x66c4a0.
   for(U32 i = 0; i < BlockWords; i++)
      mBlock[i] = 0;
   freeCount() = MaxTargets;
   for(U32 i = 0; i < MaxTargets; i++)
   {
      clearTarget(mTargets[i]);
      // A free leaves what the sensors last found; the id's next
      // allocTarget clears it (0x66d0a3), and so does a reset.
      mTargets[i].sensorFlags = 0;
   }

   // 0x671d00: no group sees anything until the sensors next run.
   dMemset(mGroupVisible, 0, sizeof(mGroupVisible));

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
   target.sensorFlags = 0;
   target.visibleMask = 0;

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

   // 0x66d433: no group keeps seeing the id, or a later owner would
   // inherit what was seen of this one.
   for(U32 g = 0; g < SensorGroups; g++)
      mGroupVisible[g][id >> 5] &= ~(1u << (id & 31));

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

void TargetManagerT2::setGroupMask(U32 table, S32 group, U32 mask)
{
   // 0x66df80.  The team targets sit in the allocation mask, so the group's
   // own target takes the mask too.
   for(U32 word = 0; word < MaskWords; word++)
   {
      if(allocMask(word) == 0)
         continue;
      for(U32 bit = 0; bit < 32; bit++)
      {
         if(!(allocMask(word) & (1u << bit)))
            continue;
         Target &target = mTargets[word * 32 + bit];
         if(target.sensorGroup != group)
            continue;
         if(table == BlockNeverVis)
            target.neverVisMask = mask;
         else if(table == BlockFriendly)
            target.friendlyMask = mask;
         else
            target.alwaysVisMask = mask;
      }
   }
   groupMask(table, group) = mask;
}

bool TargetManagerT2::isVisible(S32 id, S32 group) const
{
   const Target &target = mTargets[id];
   const U32 bit = 1u << group;
   if(target.neverVisMask & bit)
      return false;
   if(target.alwaysVisMask & bit)
      return true;
   return (target.visibleMask & bit) != 0;
}

namespace
{

/// Where a sensor looks from, and where a target is looked at (0x672061): a
/// player's eye, anything else the centre of its world box.
Point3F sensorPoint(GameBase *obj)
{
   if(obj->getTypeMask() & PlayerObjectType)
   {
      if(ShapeBase *shape = dynamic_cast<ShapeBase*>(obj))
      {
         MatrixF eye;
         shape->getEyeTransform(&eye);
         return eye.getPosition();
      }
   }
   return obj->getWorldBox().getCenter();
}

/// Whether terrain, an interior or another shape stands between the two
/// (0x672833, mask 0x80c).  Neither end blocks it, nor what the target is
/// mounted on, so a pilot is seen through his own vehicle.
bool lineIsClear(GameBase *target, GameBase *sensor,
                 const Point3F &from, const Point3F &to)
{
   SceneObject *mount = NULL;
   target->disableCollision();
   sensor->disableCollision();
   if(target->getTypeMask() & ShapeBaseObjectType)
      mount = target->getObjectMount();
   if(mount)
      mount->disableCollision();

   RayInfo info;
   const bool hit = gServerContainer.castRay(from, to,
      TerrainObjectType | InteriorObjectType | ShapeBaseObjectType, &info);

   target->enableCollision();
   sensor->enableCollision();
   if(mount)
      mount->enableCollision();
   return !hit;
}

}

void TargetManagerT2::updateSensors()
{
   // 0x671ef0.  A tick takes one more target than a thirty-second of those
   // in use, carrying on after the last one it took; ids with no object
   // cost nothing.  Retail walks the cursor unmasked and masks it on use.
   const U32 budget = ((MaxTargets - freeCount()) >> 5) + 1;
   const U32 start  = mBlock[BlockSensorCursor];
   U32 last = start;
   U32 done = 0;

   for(U32 cursor = start + 1; cursor - start < MaxTargets; cursor++)
   {
      const U32 id   = cursor & (MaxTargets - 1);
      const U32 word = id >> 5;
      if((id & 31) == 0 && allocMask(word) == 0)
      {
         cursor += 31;
         continue;
      }
      if(!(allocMask(word) & (1u << (id & 31))) || !mTargets[id].object)
         continue;

      updateTarget(id);
      last = cursor;
      if(++done >= budget)
         break;
   }

   mBlock[BlockSensorCursor] = last;
}

void TargetManagerT2::updateTarget(U32 id)
{
   Target &target = mTargets[id];
   GameBase *obj = target.object;
   const Point3F targetPos = sensorPoint(obj);

   // The groups that saw it, kept apart by the hardest thing each sensor
   // sees through, so a jammed or cloaked target can drop the rest.
   U32  seenActive = 0, seenCloaked = 0, seenPassive = 0, seenPlain = 0;
   bool pinged = false;
   bool activeJammed = false;
   bool enemyJammed = false;

   for(U32 s = 0; s < MaxTargets; s++)
   {
      const U32 word = s >> 5;
      if((s & 31) == 0 && allocMask(word) == 0)
      {
         s += 31;
         continue;
      }
      if(!(allocMask(word) & (1u << (s & 31))))
         continue;

      Target &sensor = mTargets[s];
      GameBase *sensorObj = sensor.object;
      SensorData *data = sensor.sensorData;
      if(!sensorObj || !data)
         continue;

      // A jammer always jams what carries it.
      if(s == id)
      {
         if(data->jams)
            activeJammed = true;
         continue;
      }

      ShapeBase *sensorShape = dynamic_cast<ShapeBase*>(sensorObj);
      if(!sensorShape || sensorShape->getDamageState() != ShapeBase::Enabled)
         continue;

      // x86 masks a shift to five bits; a group past 31 wraps as it does.
      const U32 groupBit = 1u << (sensor.sensorGroup & 31);

      // A group the target is always or never shown to only jams it.
      bool jamOnly = false;
      if((target.alwaysVisMask | target.neverVisMask) & groupBit)
      {
         if(!data->jams)
            continue;
         jamOnly = true;
      }

      const Point3F sensorPos = sensorPoint(sensorObj);
      bool losTested = false;
      bool losClear  = false;

      if(!jamOnly && data->detects)
      {
         U32 &seen = data->detectsActiveJammed  ? seenActive  :
                     data->detectsCloaked       ? seenCloaked :
                     data->detectsPassiveJammed ? seenPassive : seenPlain;

         bool skipSensor = false;
         do
         {
            // Already seen by this group, and nothing left to ping.
            if((seen & groupBit) && pinged == data->detectionPings)
               break;

            const Point3F d = targetPos - sensorPos;
            const F64 xx = (F64)d.x * d.x;
            const F64 yy = (F64)d.y * d.y;
            const F64 zz = (F64)d.z * d.z;

            // Retail drops a sensor sitting on its target, jamming and all.
            if(xx <= 0.0001 && yy <= 0.0001 && zz <= 0.0001)
            {
               skipSensor = true;
               break;
            }

            // Only the horizontal distance is held to detectRadius
            // (0x672476): the height difference never enters it.
            if(xx + yy > data->detectRadiusSquared)
               break;

            if(data->detectMinVelocity != 0.0f)
            {
               const Point3F v = obj->getVelocity();
               const F64 speed2 = (F64)v.y * v.y + (F64)v.x * v.x + (F64)v.z * v.z;
               if(speed2 < data->minVelocitySquared)
                  break;
            }

            if(data->detectsFOVOnly)
            {
               MatrixF eye;
               sensorShape->getEyeTransform(&eye);
               Point3F forward;
               eye.getColumn(1, &forward);
               Point3F dir = d;
               dir.normalize();

               F64 dot = (F64)dir.y * forward.y + (F64)dir.x * forward.x +
                         (F64)dir.z * forward.z;
               if(dot > 1.0)
                  dot = 1.0;
               const F32 cosine = dot > -1.0 ? F32(dot) : -1.0f;

               if(data->useObjectFOV)
               {
                  const F32 fov  = F32((F64)sensorShape->getCameraFov() *
                                       M_PI * (1.0 / 180.0));
                  const F32 half = fov * 0.5f;
                  if(cosine < F32(mCos((F64)half)))
                     break;

                  // How much of the view the target fills, in percent.
                  if(data->detectFOVPercent != 0.0f)
                  {
                     const F64 dx = (F64)targetPos.x - sensorPos.x;
                     const F64 dy = (F64)targetPos.y - sensorPos.y;
                     const F64 dz = (F64)targetPos.z - sensorPos.z;
                     const F32 distance = F32(mSqrtD(dx * dx + dy * dy + dz * dz));
                     const F32 tangent  = F32(mTan((F64)half));
                     const F64 percent  = (F64)obj->getWorldSphere().radius /
                                          ((F64)distance * tangent) * 100.0;
                     if(percent < data->detectFOVPercent)
                        break;
                  }
               }
               else if(cosine < data->cosHalfFOV)
                  break;
            }

            if(data->detectsUsingLOS)
            {
               losTested = true;
               losClear  = lineIsClear(obj, sensorObj, sensorPos, targetPos);
               if(!losClear)
                  break;
            }

            seen |= groupBit;

            // A pinging sensor pings anything not friendly to it.
            if(data->detectionPings &&
               !(sensor.friendlyMask & (1u << (target.sensorGroup & 31))))
               pinged = true;
         }
         while(false);

         if(skipSensor)
            continue;
      }

      if(!data->jams)
         continue;
      if(activeJammed && enemyJammed)
         continue;

      const bool sameGroup = sensor.sensorGroup == target.sensorGroup;
      if(sameGroup)
      {
         if(activeJammed)
            continue;
      }
      else if(enemyJammed && data->jamsOnlyGroup)
         continue;

      const F64 dx = (F64)targetPos.x - sensorPos.x;
      const F64 dy = (F64)targetPos.y - sensorPos.y;
      const F64 dz = (F64)targetPos.z - sensorPos.z;
      if(dz * dz + dy * dy + dx * dx > data->jamRadiusSquared)
         continue;

      if(data->jamsUsingLOS)
      {
         if(!losTested)
            losClear = lineIsClear(obj, sensorObj, sensorPos, targetPos);
         if(!losClear)
            continue;
      }

      // An enemy jammer that keeps to its own group still clears what a
      // friendly one set (0x672a80); retail does exactly this.
      if(sameGroup)
         activeJammed = true;
      else
      {
         activeJammed = !data->jamsOnlyGroup;
         enemyJammed  = true;
      }
   }

   bool cloaked = false;
   bool passiveJammed = false;
   ShapeBase *shape = dynamic_cast<ShapeBase*>(obj);
   if(shape)
   {
      cloaked       = shape->getCloakedState();
      passiveJammed = shape->isPassiveJammed();
   }

   U32 visible;
   if(activeJammed)
      visible = seenActive;
   else if(cloaked)
      visible = seenActive | seenCloaked;
   else if(passiveJammed)
      visible = seenActive | seenCloaked | seenPassive;
   else
      visible = seenActive | seenCloaked | seenPassive | seenPlain;
   visible = (visible | target.alwaysVisMask) & ~target.neverVisMask;
   target.visibleMask = visible;

   target.sensorFlags = 0;
   if(pinged)
      target.sensorFlags |= Pinged;
   if(activeJammed)
      target.sensorFlags |= ActiveJammed;
   if(enemyJammed)
   {
      target.sensorFlags |= EnemyJammed;
      if(shape)
         shape->forceUncloak("jammed");
   }

   // Only the groups the scripts count (setSensorGroupCount) are kept.
   for(U32 g = 0; g < mBlock[BlockGroupCount]; g++)
   {
      if((visible >> g) & 1)
         mGroupVisible[g][id >> 5] |= 1u << (id & 31);
      else
         mGroupVisible[g][id >> 5] &= ~(1u << (id & 31));
   }
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

//----------------------------------------------------------------------------
// What a target shows to each sensor group.

namespace
{

typedef U32 TargetManagerT2::Target::*TargetMask;

S32 readTargetMask(const char *idArg, const char *fn, TargetMask mask)
{
   const S32 id = validateTarget(dAtoi(idArg), fn, false);
   if(id == -1)
      return 0;
   return (S32)(TargetManagerT2::server()->getTarget(id).*mask);
}

void writeTargetMask(const char *idArg, const char *maskArg, const char *fn,
                   TargetMask mask)
{
   // Retail's setters pass the getters' check (0x66f290), so a team
   // target's masks can be changed.  A mask only takes effect when the
   // sensors next reach the target.
   const S32 id = validateTarget(dAtoi(idArg), fn, false);
   if(id == -1)
      return;
   TargetManagerT2::server()->getTarget(id).*mask = (U32)dAtoi(maskArg);
}

S32 readGroupMask(const char *groupArg, const char *fn, U32 table)
{
   const S32 group = validateSensorGroup(dAtoi(groupArg), fn);
   if(group == -1)
      return 0;
   return (S32)TargetManagerT2::server()->getGroupMask(table, group);
}

void writeGroupMask(const char *groupArg, const char *maskArg, const char *fn,
                  U32 table)
{
   const S32 group = validateSensorGroup(dAtoi(groupArg), fn);
   if(group == -1)
      return;
   const U32 mask = (U32)dAtoi(maskArg);
   if(table == TargetManagerT2::BlockListen)
      TargetManagerT2::server()->setListenMask(group, mask);
   else
      TargetManagerT2::server()->setGroupMask(table, group, mask);
}

}

ConsoleFunction(getTargetAlwaysVisMask, S32, 2, 2, "getTargetAlwaysVisMask(target)")
{
   // 0x66f250.
   return readTargetMask(argv[1], "cGetTargetAlwaysVisMask",
                        &TargetManagerT2::Target::alwaysVisMask);
}

ConsoleFunction(setTargetAlwaysVisMask, void, 3, 3, "setTargetAlwaysVisMask(target, mask)")
{
   // 0x66f290.
   writeTargetMask(argv[1], argv[2], "cSetTargetAlwaysVisMask",
                 &TargetManagerT2::Target::alwaysVisMask);
}

ConsoleFunction(getTargetNeverVisMask, S32, 2, 2, "getTargetNeverVisMask(target)")
{
   // 0x66f2e0.
   return readTargetMask(argv[1], "cGetTargetNeverVisMask",
                        &TargetManagerT2::Target::neverVisMask);
}

ConsoleFunction(setTargetNeverVisMask, void, 3, 3, "setTargetNeverVisMask(target, mask)")
{
   // 0x66f320.
   writeTargetMask(argv[1], argv[2], "cSetTargetNeverVisMask",
                 &TargetManagerT2::Target::neverVisMask);
}

ConsoleFunction(getTargetFriendlyMask, S32, 2, 2, "getTargetFriendlyMask(target)")
{
   // 0x66f370.
   return readTargetMask(argv[1], "cGetTargetFriendlyMask",
                        &TargetManagerT2::Target::friendlyMask);
}

ConsoleFunction(setTargetFriendlyMask, void, 3, 3, "setTargetFriendlyMask(target, mask)")
{
   // 0x66f3b0.
   writeTargetMask(argv[1], argv[2], "cSetTargetFriendlyMask",
                 &TargetManagerT2::Target::friendlyMask);
}

ConsoleFunction(getSensorGroupAlwaysVisMask, S32, 2, 2, "getSensorGroupAlwaysVisMask(sensorGroup)")
{
   // 0x66f400.
   return readGroupMask(argv[1], "cGetSensorGroupAlwaysVisMask",
                       TargetManagerT2::BlockAlwaysVis);
}

ConsoleFunction(setSensorGroupAlwaysVisMask, void, 3, 3, "setSensorGroupAlwaysVisMask(sensorGroup, mask)")
{
   // 0x66f440.
   writeGroupMask(argv[1], argv[2], "cSetSensorGroupAlwaysVisMask",
                TargetManagerT2::BlockAlwaysVis);
}

ConsoleFunction(getSensorGroupNeverVisMask, S32, 2, 2, "getSensorGroupNeverVisMask(sensorGroup)")
{
   // 0x66f490.
   return readGroupMask(argv[1], "cGetSensorGroupNeverVisMask",
                       TargetManagerT2::BlockNeverVis);
}

ConsoleFunction(setSensorGroupNeverVisMask, void, 3, 3, "setSensorGroupNeverVisMask(sensorGroup, mask)")
{
   // 0x66f4d0.
   writeGroupMask(argv[1], argv[2], "cSetSensorGroupNeverVisMask",
                TargetManagerT2::BlockNeverVis);
}

ConsoleFunction(getSensorGroupFriendlyMask, S32, 2, 2, "getSensorGroupFriendlyMask(sensorGroup)")
{
   // 0x66f520.
   return readGroupMask(argv[1], "cGetSensorGroupFriendlyMask",
                       TargetManagerT2::BlockFriendly);
}

ConsoleFunction(setSensorGroupFriendlyMask, void, 3, 3, "setSensorGroupFriendlyMask(sensorGroup, mask)")
{
   // 0x66f560.
   writeGroupMask(argv[1], argv[2], "cSetSensorGroupFriendlyMask",
                TargetManagerT2::BlockFriendly);
}

ConsoleFunction(getSensorGroupListenMask, S32, 2, 2, "getSensorGroupListenMask(sensorGroup)")
{
   // 0x66f5b0.
   return readGroupMask(argv[1], "cGetSensorGroupListenMask",
                       TargetManagerT2::BlockListen);
}

ConsoleFunction(setSensorGroupListenMask, void, 3, 3, "setSensorGroupListenMask(sensorGroup, mask)")
{
   // 0x66f5f0.  Unlike the other three, the group's targets keep theirs.
   writeGroupMask(argv[1], argv[2], "cSetSensorGroupListenMask",
                TargetManagerT2::BlockListen);
}

ConsoleFunction(isTargetFriendly, bool, 3, 3, "isTargetFriendly(target, sensorGroup)")
{
   // 0x66f640.
   const S32 id = validateTarget(dAtoi(argv[1]), "cIsTargetFriendly", false);
   if(id == -1)
      return false;
   const S32 group = validateSensorGroup(dAtoi(argv[2]), "cIsTargetFriendly");
   if(group == -1)
      return false;
   return (TargetManagerT2::server()->getTarget(id).friendlyMask >> group) & 1;
}

ConsoleFunction(isTargetVisible, bool, 3, 3, "isTargetVisible(target, sensorGroup)")
{
   // 0x66f6b0.
   const S32 id = validateTarget(dAtoi(argv[1]), "cIsTargetVisible", false);
   if(id == -1)
      return false;
   const S32 group = validateSensorGroup(dAtoi(argv[2]), "cIsTargetVisible");
   if(group == -1)
      return false;
   return TargetManagerT2::server()->isVisible(id, group);
}

ConsoleFunction(getSensorGroupCount, S32, 1, 1, "getSensorGroupCount()")
{
   // 0x66f200.
   return TargetManagerT2::server()->getGroupCount();
}

ConsoleFunction(setSensorGroupCount, void, 2, 2, "setSensorGroupCount(count)")
{
   // 0x66f210.  Every group may be counted: 0 to 32.
   const S32 count = dAtoi(argv[1]);
   if(count < 0 || count > TargetManagerT2::SensorGroups)
   {
      Con::errorf("TargetManager::cSetSensorGroupCount: invalid group count [%d]", count);
      return;
   }
   TargetManagerT2::server()->setGroupCount(count);
}

ConsoleFunction(setTargetSensorData, void, 3, 3, "setTargetSensorData(targetId, sensorData)")
{
   // 0x66f720.  Only the range is checked, silently: an unallocated target
   // takes a sensor too.  Anything but a SensorData clears it, which is how
   // the scripts pass 0.
   const S32 id = dAtoi(argv[1]);
   if(id < 0 || id >= TargetManagerT2::MaxTargets)
      return;
   TargetManagerT2::server()->getTarget(id).sensorData =
      dynamic_cast<SensorData*>(Sim::findObject(argv[2]));
}

ConsoleFunction(getTargetSensorData, S32, 2, 2, "getTargetSensorData(targetId)")
{
   // 0x66f7b0.
   const S32 id = dAtoi(argv[1]);
   if(id < 0 || id >= TargetManagerT2::MaxTargets)
      return -1;
   SensorData *data = TargetManagerT2::server()->getTarget(id).sensorData;
   return data ? data->getId() : -1;
}

ConsoleMethod(ShapeBase, setPassiveJammed, void, 3, 3, "obj.setPassiveJammed(true|false)")
{
   // 0x5f1d50.  Only the server's sensors read it, so a ghost ignores it.
   if(object->isGhost())
      return;
   object->setPassiveJammed(dAtob(argv[2]));
}

ConsoleMethod(ShapeBase, isPassiveJammed, bool, 2, 2, "obj.isPassiveJammed()")
{
   // 0x5f1d80.
   return object->isPassiveJammed();
}
