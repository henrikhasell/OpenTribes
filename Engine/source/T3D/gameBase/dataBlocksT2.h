// HH: Added for Tribes 2 compatibility.
#ifndef _DATABLOCKST2_H_
#define _DATABLOCKST2_H_

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

class BitStream;

/// A datablock Tribes 2 has and the port has no class for yet.
///
/// A datablock's contents ride inside one event with no length in front of
/// them, so a client that cannot read one cannot read anything after it.
/// Rather than stand up a class per name, these read by a program: the list
/// of field widths taken off the retail binary, held in the wire's own order.
/// Each value read is kept, so what was read can be written back unchanged.
class DataBlockT2 : public SimDataBlock
{
   typedef SimDataBlock Parent;

protected:
   Vector<U32>              mValue;
   Vector<StringTableEntry> mString;

   /// The field widths this class puts on the wire, in order.
   virtual const U16 *getReadProgram() const;

public:
   /// One step of a read program.
   enum Op
   {
      OpEnd,         ///< no more fields
      OpFlag,        ///< one bit
      OpInt,         ///< an unsigned field, width follows
      OpFloat,       ///< a float held in a field, width follows
      OpRaw,         ///< whole bytes, count follows
      OpString,      ///< a string
      OpBlock,       ///< the id of another datablock
      OpIf,          ///< a bit, then that many program words if it was set
      OpLoop,        ///< a count, then a length, then that many words, so often
      OpTimes,       ///< a field holding how many times, its width and bias, then a length
      OpIfKept       ///< a length, taken if the bit last read was set
   };

   DataBlockT2();

   void packData(BitStream *stream);
   void unpackData(BitStream *stream);

   DECLARE_CONOBJECT(DataBlockT2);

   /// Read a datablock the port has a class for but writes differently.
   ///
   /// The part of the chain that is ported reads as it stands; the rest is
   /// taken off by the field widths Tribes 2 wrote.  False if the class is
   /// not one of those, leaving the caller to unpack it itself.
   static bool unpackDelta(SimDataBlock *obj, BitStream *stream);
};

/// Give a Tribes 2 datablock name a class that reads by program.
#define IMPLEMENT_DATABLOCK_T2( className, program )                        \
   class className : public DataBlockT2                                     \
   {                                                                        \
      typedef DataBlockT2 Parent;                                           \
      const U16 *getReadProgram() const { return program; }                 \
   public:                                                                  \
      DECLARE_CONOBJECT(className);                                         \
   };                                                                       \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                    \
   ConsoleDocClass( className, "@brief A Tribes 2 datablock read by its "   \
                               "field widths.\n\n@internal" )

#endif
