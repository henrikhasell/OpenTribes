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

      T2DbFieldCount
   };

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
