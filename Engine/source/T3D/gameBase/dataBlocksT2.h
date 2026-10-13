// HH: Added for Tribes 2 compatibility.
#ifndef _DATABLOCKST2_H_
#define _DATABLOCKST2_H_

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _STUBST2_H_
#include "T3D/gameBase/stubsT2.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _RESOURCE_H_
#include "core/resource.h"
#endif

class DataBlockT2 : public GameBaseData
{
   typedef GameBaseData Parent;

public:
   DataBlockT2();

   StringTableEntry  shapeName;
   Resource<TSShape> mShape;

   virtual bool namesGameBaseShape() const { return false; }

      /// Whether the class Tribes 2 has in this one's place derives from
      /// ShapeBaseData, which is the whole of what the file checksum sums
      /// (ResManager's walk at 0x004401a0 casts to it and skips everything
      /// else). A stub standing in for anything else must answer false, or
      /// our answer carries files the server's never counted.
   virtual bool isShapeBaseData() const { return false; }

   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(DataBlockT2);
};

#define IMPLEMENT_DATABLOCK_T2_SHAPE( className, shaped )                   \
   class className : public DataBlockT2                                     \
   {                                                                        \
      typedef DataBlockT2 Parent;                                           \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
      bool namesGameBaseShape() const { return shaped; }                    \
      bool isShapeBaseData() const { return shaped; }                       \
      bool unpackData(BitStream *stream, U32 classId)                     \
      {                                                                     \
         StubT2::warn("datablock", getClassName());                         \
         return Parent::unpackData(stream, classId);                      \
      }                                                                     \
   };                                                                       \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 datablock the port reads " \
                               "by its layout.\n\n@internal" )

#define IMPLEMENT_DATABLOCK_T2( className )                                 \
   IMPLEMENT_DATABLOCK_T2_SHAPE( className, false )

#endif
