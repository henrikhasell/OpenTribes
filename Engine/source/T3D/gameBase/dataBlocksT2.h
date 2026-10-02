// HH: Added for Tribes 2 compatibility.
#ifndef _DATABLOCKST2_H_
#define _DATABLOCKST2_H_

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif

/// A datablock Tribes 2 has and the port has no class for.
///
/// Its body is read from the class's recovered layout rather than by a reader
/// of its own, so there is nothing to unpack here.  The class exists so the
/// net class table has something to seat, and so the server's datablock ids
/// resolve to an object.
class DataBlockT2 : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:
   DataBlockT2();

   DECLARE_CONOBJECT(DataBlockT2);
};

/// Give a Tribes 2 datablock name a class of its own.
#define IMPLEMENT_DATABLOCK_T2( className )                                 \
   class className : public DataBlockT2                                     \
   {                                                                        \
      typedef DataBlockT2 Parent;                                           \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
   };                                                                       \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 datablock the port reads " \
                               "by its layout.\n\n@internal" )

#endif
