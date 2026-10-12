// HH: Added for Tribes 2 compatibility.
#ifndef _T2LAYOUT_H_
#define _T2LAYOUT_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

class BitStream;

namespace T2Layout
{
   /// Eight image slots, of which item.cs fills four: weapon, auxiliary,
   /// backpack and flag.
   enum { MaxImages = 8 };

   struct Body
   {
      bool    hasPosition;
      Point3F position;
      bool    hasYaw;
      F32     yaw;                  ///< radians
      bool    hasRotation;
      MatrixF rotation;             ///< the whole orientation, where the body
      bool    hasScale;
      Point3F scale;
      bool    hasVelocity;
      Point3F velocity;             ///< metres a second, world space
      bool    hasDirection;
      Point3F direction;            ///< a unit vector, where the body says
                                    ///< which way without saying how fast
      bool    hasDamage;
      F32     damage;               ///< share of the object's maxDamage

      /// What a body said about one of its object's image slots -- the
      /// weapon in the hand, the pack on the back.
      struct Image
      {
         bool   present;            ///< the body carried this slot at all;
                                    ///< the rest means nothing without it
         bool   hasDataBlock;       ///< clear is an emptied slot, not an
                                    ///< unchanged one
         U32    dataBlockId;
         bool   hasSkinName;
         String skinName;
         bool   hasSkinTag;
         U32    skinTag;            ///< a net string id, which needs the
                                    ///< connection's table to mean anything
         bool   triggerDown;
         bool   loaded;
         bool   ammo;
         bool   wet;
         bool   target;
         U32    fireCount;
      };
      Image   images[MaxImages];
   };

   const Body &lastBody();

   enum T2DataBlockField
   {
      T2DbJetForce,
      T2DbUnderwaterJetForce,
      T2DbUnderwaterVertJetFactor,
      T2DbJetEnergyDrain,
      T2DbUnderwaterJetEnergyDrain,
      T2DbMinJetEnergy,
      T2DbMaxJetForwardSpeed,
      T2DbMaxJetHorizontalPercentage,
      T2DbHeatDecayPerSec,
      T2DbHeatIncreasePerSec,
      T2DbCameraMaxDist,
      T2DbImageMountPoint,
      T2DbProjectileVelocity,
      T2DbProjectileLifetime,
      T2DbTracerLength,
      T2DbTracerWidth,
      T2DbCrossSize,
      T2DbElasticity,
      T2DbFriction,
      T2DbGravityMod,

      T2DbFieldCount
   };

   /// How many states a Tribes 2 image datablock can carry, which is what a
   /// way out of one is wide enough to name.
   enum { MaxImageStates = 32 };

      /// No sequence name is on the wire: retail's client finds one by looking the
      /// state's own name up in the shape.
   struct ImageState
   {
      /// The ways out of a state, in the order the body carries them, which
      /// is Torque's own: each pair is the false arm and then the true one.
      enum
      {
         NotLoaded, Loaded, NoAmmo, Ammo, NoTarget, Target, NotWet, Wet,
         TriggerUp, TriggerDown, Timeout,

         TransitionCount
      };

      String name;
      S32    transition[TransitionCount]; ///< a state, or -1 for no way out
      bool   hasTimeout;
      F32    timeout;
      bool   waitForTimeout;
      bool   fire;
         /// Of the 211 states of the game's 45 images only the chaingun's Fire sets
         /// this, and retail has it ejecting a shell and carrying a flash alike.
      bool   ejectShell;
      bool   direction;          ///< play the sequence forwards
      U32    loaded;             ///< ShapeBaseImageData::StateData::LoadedState
      U32    spin;               ///< ...::SpinState
      U32    recoil;             ///< ...::RecoilState
   };

   /// The states the last datablock read, in the order it read them.
   const Vector<ImageState> &lastImageStates();

   bool lastDataBlockField(U32 field, F32 *out);

   /// The same read as the bits it arrived as, for a field Tribes 2 writes
   /// as a whole word rather than a float.
   bool lastDataBlockWord(U32 field, U32 *out);

   bool readDataBlock(BitStream *stream, U32 classId);

   bool readGhost(BitStream *stream, const char *className,
                  bool isInitial = false);
   bool readEvent(BitStream *stream, const char *className);

   typedef const char *(*GhostClassFn)(void *ctx, U32 ghostIndex);

   bool readPacketData(BitStream *stream, const char *className,
                       GhostClassFn resolve = NULL, void *ctx = NULL);

   bool writeDataBlock(BitStream *stream, U32 classId);

   bool writeGhost(BitStream *stream, const char *className,
                   bool isInitial = false);
   bool writeEvent(BitStream *stream, const char *className);

   bool writePacketData(BitStream *stream, const char *className,
                        GhostClassFn resolve = NULL, void *ctx = NULL);

   const char *lastShapeFile();

   U32 firstDataBlockId();

   /// Every datablock id the last body named, in the order it named them.
   const Vector<U32> &lastDataBlockIds();

   const char *firstString();

   bool firstWord32(U32 *out);

   const Vector<String> &lastStrings();

   /// Every 32-bit word the last ghost body read, in the order it read them.
   const Vector<U32> &lastWords();

   /// A Tribes 2 resource name is a bare file name; Torque wants a path.
   String resourcePath(const char *name, const char *dir);
}

#endif
