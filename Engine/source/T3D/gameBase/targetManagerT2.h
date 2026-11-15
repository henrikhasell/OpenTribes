#ifndef _TARGETMANAGERT2_H_
#define _TARGETMANAGERT2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _NETSTRINGTABLE_H_
#include "sim/netStringTable.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class GameBase;
class GameConnection;
class ShapeBaseData;
class SimDataBlock;

/// The targets a Tribes 2 server keeps for its scripts (TargetManager,
/// built at 0x66c2c0).  A target is what the HUD, the sensors and the
/// commander map know an object by; the scripts allocate one per client and
/// per object whose datablock has a targetTypeTag, and hand its id to
/// GameBase.setTarget.  Targets are the server's state only; the sensor group
/// colours are the one part sent, to the connections in the group they affect.
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

   /// Retail keeps the free count, the per-group masks and the allocation
   /// mask as one run of words (manager+0x180c to +0x1a58), and allocTarget
   /// indexes the mask tables with a group it never checks.  They are kept
   /// in the same order here so an out-of-range group reads the same
   /// neighbour retail does.
   enum Block
   {
      BlockFreeCount  = 0,                             // +0x180c
      BlockZero       = 1,                             // +0x1810
      BlockGroupCount = 2,                             // +0x1814
      BlockAlwaysVis  = 3,                             // +0x1818
      BlockNeverVis   = BlockAlwaysVis + SensorGroups, // +0x1898
      BlockFriendly   = BlockNeverVis + SensorGroups,  // +0x1918
      BlockListen     = BlockFriendly + SensorGroups,  // +0x1998
      BlockAllocMask  = BlockListen + SensorGroups,    // +0x1a18
      BlockWords      = BlockAllocMask + MaskWords,    // ends at +0x1a58
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
      U32  alwaysVisMask;
      U32  neverVisMask;
      U32  friendlyMask;
      /// Nine bits: which of the datablock's hud images the target is drawn with.
      U32  renderMask;
      /// The SensorData it senses with.  Nothing on this server senses yet.
      SimObjectPtr<SimDataBlock> sensorData;
   };

   /// What a TargetInfoEvent carries, one bit each. The first five are the
   /// tags, in the order the event writes them.
   enum InfoField
   {
      FieldName        = 1 << 0,
      FieldSkin        = 1 << 1,
      FieldPrefSkin    = 1 << 2,
      FieldVoice       = 1 << 3,
      FieldType        = 1 << 4,
      FieldSensorGroup = 1 << 5,
      FieldDataBlock   = 1 << 6,
      FieldRenderMask  = 1 << 7,
      FieldVoicePitch  = 1 << 8,
      FieldAll         = 0x1ff,
   };

   /// Which of the per-target masks a group's mask is copied to.
   enum GroupMask
   {
      GroupAlwaysVis,
      GroupNeverVis,
      GroupFriendly,
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

   /// The name and skin a target is shown by, changed only when they differ
   /// (0x66ea90, 0x66ec30). Each change is sent to the connections told about
   /// the target.
   void setName(S32 id, const NetStringHandle &name);
   void setSkin(S32 id, const NetStringHandle &skin);
   /// Nine bits. A mask is sent even when it does not change (0x66fb00).
   void setRenderMask(S32 id, U32 mask);
   /// Sends a connection every target whose allocation bit is set, team
   /// targets included, each in full (0x66e3a0). Changes nothing.
   void sendTargetsTo(GameConnection *conn);
   void setGroupMask(GroupMask which, S32 group, U32 mask);

   /// The colour one viewer group draws each target of a sensor group in, for
   /// the targets in mask. The viewer group's connections are told at once
   /// (0x66f950), and a connection that joins later is told with its group.
   void setGroupColours(S32 group, U32 mask, const ColorI &colour);
   const ColorI *groupColours(S32 group) const { return mColours[group]; }

   /// The number of sensor groups the scripts count (0x66f210). Nothing on this
   /// server reads it yet: the loop that does is not ported.
   void setGroupCount(S32 count) { mBlock[BlockGroupCount] = count; }

   bool isTargetVisible(S32 id, S32 group) const;
   bool isTargetFriendly(S32 id, S32 group) const;

   Target &getTarget(S32 id) { return mTargets[id]; }

private:
   void clearTarget(Target &target);
   void takeGroupMasks(Target &target, S32 group) const;

   /// The connections told about a target: each established one that has its
   /// data blocks, where the target changes or is allocated (0x66d1c0).
   void broadcastInfo(S32 id, U32 fields);
   /// Each established connection is told a target is gone (0x66d390).
   void broadcastFree(S32 id);
   /// Each connection told about targets is first told to forget them all.
   void broadcastReset();

   U32 &freeCount()          { return mBlock[BlockFreeCount]; }
   U32 &allocMask(U32 word)  { return mBlock[BlockAllocMask + word]; }
   U32 &groupMask(U32 table, U32 group) { return mBlock[table + group]; }

   U32    mBlock[BlockWords];
   Target mTargets[MaxTargets];
   /// Row is the viewer group, entry the sensor group it sees.
   ColorI mColours[SensorGroups][SensorGroups];
};

#endif
