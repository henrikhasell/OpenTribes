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

// HH: A stub that knows which of its script's fields go on which of its
// layout's nodes, and so has a body to write even though it declares no
// fields of its own. The table is named sT2<className>Fields.
#define IMPLEMENT_DATABLOCK_T2_WRITES( className )                          \
   class className : public DataBlockT2                                      \
   {                                                                         \
      typedef DataBlockT2 Parent;                                            \
   public:                                                                   \
      DECLARE_CONOBJECT(className);                                          \
      bool unpackData(BitStream *stream, U32 classId)                        \
      {                                                                      \
         StubT2::warn("datablock", getClassName());                          \
         return Parent::unpackData(stream, classId);                         \
      }                                                                      \
      void publishT2Data()                                                   \
      {                                                                      \
         Parent::publishT2Data();                                            \
         StubT2::publish(this, sT2##className##Fields,                       \
            sizeof(sT2##className##Fields) /                                 \
            sizeof(sT2##className##Fields[0]));                              \
      }                                                                      \
   };                                                                        \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                      \
   ConsoleDocClass( className, "@brief A Tribes 2 datablock the port reads "  \
                               "by its layout and writes from its script's "  \
                               "own fields.\n\n@internal" )

#endif
