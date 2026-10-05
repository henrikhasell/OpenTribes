// HH: Added for Tribes 2 compatibility.
#ifndef _DATABLOCKST2_H_
#define _DATABLOCKST2_H_

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _STUBST2_H_
#include "T3D/gameBase/stubsT2.h"
#endif

class DataBlockT2 : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:
   DataBlockT2();

   DECLARE_CONOBJECT(DataBlockT2);
};

#define IMPLEMENT_DATABLOCK_T2( className )                                 \
   class className : public DataBlockT2                                     \
   {                                                                        \
      typedef DataBlockT2 Parent;                                           \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
      bool unpackData(BitStream *stream, U32 classId)                     \
      {                                                                     \
         StubT2::warn("datablock", getClassName());                         \
         return Parent::unpackData(stream, classId);                      \
      }                                                                     \
   };                                                                       \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 datablock the port reads " \
                               "by its layout.\n\n@internal" )

#endif
