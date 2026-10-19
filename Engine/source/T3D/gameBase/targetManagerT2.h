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

class GameBase;
class ShapeBaseData;

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

   /// Retail keeps the free count, the per-group masks and the allocation
   /// mask as one run of words (manager+0x180c to +0x1a58), and allocTarget
   /// indexes the mask tables with a group it never checks.  They are kept
   /// in the same order here so an out-of-range group reads the same
   /// neighbour retail does.
   enum Block
   {
      BlockFreeCount  = 0,                             // +0x180c
      BlockZero       = 1,                             // +0x1810, +0x1814
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

   Target &getTarget(S32 id) { return mTargets[id]; }

private:
   void clearTarget(Target &target);
   void takeGroupMasks(Target &target, S32 group) const;

   U32 &freeCount()          { return mBlock[BlockFreeCount]; }
   U32 &allocMask(U32 word)  { return mBlock[BlockAllocMask + word]; }
   U32 &groupMask(U32 table, U32 group) { return mBlock[table + group]; }

   U32    mBlock[BlockWords];
   Target mTargets[MaxTargets];
};

#endif
