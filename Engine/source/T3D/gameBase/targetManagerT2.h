#ifndef _TARGETMANAGERT2_H_
#define _TARGETMANAGERT2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _NETSTRINGTABLE_H_
#include "sim/netStringTable.h"
#endif

class GameBase;
class ShapeBaseData;

/// What a target's object sees and jams (SensorData, 0x66a180).  Tribes 2
/// derives it straight from SimDataBlock and sends none of it, so a client
/// holds the defaults; only the server's target manager reads it.
class SensorData : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:
   bool detects;
   bool detectsUsingLOS;
   bool detectsPassiveJammed;
   bool detectsActiveJammed;
   bool detectsCloaked;
   bool detectionPings;
   bool detectsFOVOnly;
   F32  detectMinVelocity;
   F32  detectRadius;
   F32  detectFOV;
   bool jams;
   bool jamsOnlyGroup;
   bool jamsUsingLOS;
   F32  jamRadius;
   F32  detectFOVPercent;
   bool useObjectFOV;

   /// Worked out once in onAdd (0x66a1f0), so a field changed afterwards
   /// leaves them as they were.
   F32  minVelocitySquared;
   F32  detectRadiusSquared;
   F32  cosHalfFOV;
   F32  jamRadiusSquared;

   SensorData();

   bool onAdd();
   static void initPersistFields();

   DECLARE_CONOBJECT(SensorData);
};

/// The targets a Tribes 2 server keeps for its scripts (TargetManager,
/// built at 0x66c2c0).  A target is what the HUD, the sensors and the
/// commander map know an object by; the scripts allocate one per client and
/// per object whose datablock has a targetTypeTag, and hand its id to
/// GameBase.setTarget.  This is the server's state only: nothing here is sent.
class TargetManagerT2
{
public:
   enum Constants
   {
      MaxTargets   = 512,
      /// Targets 0 to 31 belong to the sensor groups of the same number. They
      /// are never allocated and nothing may change or free them.
      TeamTargets  = 32,
      SensorGroups = 32,
      MaskWords    = MaxTargets / 32,
   };

   /// Retail keeps the free count, the sensor cursor, the group count, the
   /// per-group masks and the allocation mask as one run of words
   /// (manager+0x180c to +0x1a58), and allocTarget indexes the mask tables
   /// with a group it never checks.  They are kept in the same order here so
   /// an out-of-range group reads the same neighbour retail does.
   enum Block
   {
      BlockFreeCount     = 0,                             // +0x180c
      BlockSensorCursor  = 1,                             // +0x1810
      BlockGroupCount    = 2,                             // +0x1814
      BlockAlwaysVis     = 3,                             // +0x1818
      BlockNeverVis      = BlockAlwaysVis + SensorGroups, // +0x1898
      BlockFriendly      = BlockNeverVis + SensorGroups,  // +0x1918
      BlockListen        = BlockFriendly + SensorGroups,  // +0x1998
      BlockAllocMask     = BlockListen + SensorGroups,    // +0x1a18
      BlockWords         = BlockAllocMask + MaskWords,    // ends at +0x1a58
   };

   /// What the last sensor pass found about a target (target+0x3c).  Kept
   /// for the target events; no script reads it.
   enum SensorFlags
   {
      Pinged        = 1 << 1,
      ActiveJammed  = 1 << 2,
      EnemyJammed   = 1 << 3,
   };

   struct Target
   {
      NetStringHandle name;
      NetStringHandle skin;
      NetStringHandle prefSkin;
      NetStringHandle voice;
      NetStringHandle type;
      F32  voicePitch;
      S32  sensorGroup;
      bool allocated;
      SimObjectPtr<GameBase>      object;
      SimObjectPtr<ShapeBaseData> dataBlock;
      /// The groups the last sensor pass let see it.
      U32  visibleMask;
      U32  alwaysVisMask;
      U32  neverVisMask;
      U32  friendlyMask;
      U32  sensorFlags;
      SimObjectPtr<SensorData>    sensorData;
   };

   TargetManagerT2();

   static TargetManagerT2 *server();

   void reset();
   /// Takes every target from its object, then resets (0x66d830).
   void resetTargets();

   /// The lowest free id past the team targets, or -1 once all are in use.
   S32  allocTarget(const NetStringHandle &name, const NetStringHandle &skin,
                    const NetStringHandle &voice, const NetStringHandle &type,
                    S32 sensorGroup, S32 dataBlockId, F32 voicePitch,
                    const NetStringHandle &prefSkin);
   void freeTarget(S32 id);
   void setSensorGroup(S32 id, S32 group);
   void setTargetObject(S32 id, GameBase *obj);

   /// Sets a sensor group's always-visible, never-visible or friendly mask,
   /// and the same mask on every allocated target in the group (0x66df80).
   void setGroupMask(U32 table, S32 group, U32 mask);
   U32  getGroupMask(U32 table, S32 group) const { return mBlock[table + group]; }
   void setListenMask(S32 group, U32 mask) { groupMask(BlockListen, group) = mask; }

   S32  getGroupCount() const   { return (S32)mBlock[BlockGroupCount]; }
   void setGroupCount(S32 count) { mBlock[BlockGroupCount] = (U32)count; }

   /// 0x66e130: never-visible wins, then always-visible, then the sensors.
   bool isVisible(S32 id, S32 group) const;

   /// Runs some of the targets past every sensor, once a tick before the
   /// objects tick (0x671ef0, from the server's process list at 0x6023aa).
   void updateSensors();

   /// Whether the last pass let a group see a target, kept per group the
   /// way the scoping and the turrets read it (manager+0xc).
   bool isGroupVisible(S32 group, S32 id) const
   {
      return (mGroupVisible[group][id >> 5] >> (id & 31)) & 1;
   }

   Target &getTarget(S32 id) { return mTargets[id]; }

private:
   void clearTarget(Target &target);
   void takeGroupMasks(Target &target, S32 group) const;
   void updateTarget(U32 id);

   U32 &freeCount()          { return mBlock[BlockFreeCount]; }
   U32 &allocMask(U32 word)  { return mBlock[BlockAllocMask + word]; }
   U32 &groupMask(U32 table, U32 group) { return mBlock[table + group]; }

   U32    mBlock[BlockWords];
   Target mTargets[MaxTargets];
   U32    mGroupVisible[SensorGroups][MaskWords];
};

#endif
