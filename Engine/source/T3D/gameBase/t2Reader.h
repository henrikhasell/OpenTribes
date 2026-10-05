// HH: Added for Tribes 2 compatibility.
#ifndef _T2READER_H_
#define _T2READER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#include "T3D/gameBase/t2DataBlockTables.h"

class BitStream;

/// A cursor over one Tribes 2 bit layout.
///
/// The layout graph decides the path -- its branches read the stream, never the
/// caller -- so a class cannot drive the walk itself.  It drives the values
/// instead: the cursor resolves every branch, loop and count on its own and
/// stops on each field, which the caller takes with the read of the matching
/// type.  Asking for the wrong type is the mistake worth catching, and it is
/// reported against the node that disagreed rather than silently returning a
/// field's worth of some other field.
///
/// A caller that stops early is not an error.  finish() reads out the rest and
/// throws it away, which is what lets a class be ported a field at a time
/// without the packets behind it being lost in the meantime.
class T2Reader
{
public:
   /// What the stream cannot say.  Only the piloted-vehicle case needs it: the
   /// width of what follows depends on which ghost an index names, so the
   /// connection has to be asked.
   struct Env
   {
      virtual ~Env() {}
      virtual const char *ghostClass(U32 ghostIndex) { return NULL; }
      virtual bool readPacketData(BitStream *stream, const char *className)
      { return false; }
   };

   T2Reader(BitStream *stream, const T2Protocol::T2DataBlockLayout *layout,
            bool isInitial = false, Env *env = NULL);

   /// True once every field has been taken or the walk has failed.
   bool atEnd() const { return mAt < 0 || mFailed; }

   /// Is this an initial body, the one a ghost arrives with?
   ///
   /// A class needs this where a field is on the initial body only and the
   /// layout does not say so -- GameBase's datablock id and target id are
   /// both in that shape, and taking them off an update ate the first field
   /// of whatever class was reading after it.
   bool isInitial() const { return mInitial; }
   bool failed() const { return mFailed; }

   /// The op of the field the cursor stands on, or OpEnd at the end.
   T2Protocol::T2DataOp op() const;
   /// The width of that field, where the layout fixes it.
   U32 bits() const;

   /// Which node of the layout the cursor stands on, or NoNode at the end.
   ///
   /// A field's op and width are not always enough to name it: two optional
   /// fields of the same shape in a row cannot be told apart by shape, so if
   /// only the second is sent the first take claims it and the value lands in
   /// the wrong member.  The node index is the field's identity and settles
   /// it.  Prefer the shape, which survives the tables being regenerated, and
   /// reach for this only where the shape is ambiguous -- and then check the
   /// shape as well, so a table that has moved underneath fails loudly here
   /// rather than quietly filling a member with the wrong field.
   enum { NoNode = 0xFFFFFFFF };
   U32 node() const { return mAt < 0 ? U32(NoNode) : U32(mAt); }

   /// The op of any node of the layout, not just the one being stood on.
   T2Protocol::T2DataOp opAt(U32 node) const;

   /// Was the flag at this node read on the way here, and what did it say?
   ///
   /// A flag is control flow: the cursor reads it to choose the next node and
   /// the walk moves on, so it is never a field anyone stops on and a class
   /// asking only for fields can read no boolean the protocol sends.  That is
   /// most of the protocol to leave on the floor -- 9,674 of the 16,575 nodes
   /// across every table are flags -- and a world where nothing is powered,
   /// alarmed, open or destroyed is not a simulating world.
   ///
   /// So every flag is recorded as it is read, by the node that read it.  From
   /// construction, which matters: the cursor runs ahead to the first field
   /// before a class is ever called, and the flags it passes on the way would
   /// otherwise be spent before anyone could ask.
   ///
   /// Recorded as it is read, though -- not in advance.  A flag behind the
   /// last field of a body has not been read while a class is still taking
   /// fields, so flags are taken after the fields and not before.  See the
   /// note on T2Fields::takeFlag, which is where that bites.
   ///
   /// sawFlag() is false for a flag on a branch the body did not take -- it was
   /// not sent rather than sent clear, and those are different.  A flag inside
   /// a loop keeps what the last iteration said.
   bool sawFlag(U32 node) const;
   bool flag(U32 node) const;

   /// Report a class asking for a flag at a node that holds something else.
   /// Deduped like every other disagreement here: this is a packet path.
   void failFlag(U32 node);

   /// Was the select at this node resolved on the way here, and what did it
   /// read?
   ///
   /// The same problem as a flag and the same answer.  A flag is one bit of
   /// control flow; OpSelect reads several and jumps through a table, and
   /// OpReadBr reads a word and branches on whether it was zero -- and a
   /// walked layout models a field as one of those whenever the binary
   /// branched on its value.  So these are fields too, swallowed the same way:
   /// 985 OpSelect nodes and 28 OpReadBr across the tables, which is every
   /// damage state, every image's fire count, every mount node and the
   /// whole-file flag of a CRC challenge.
   ///
   /// Recorded as read, from construction, exactly as flags are -- so the
   /// same rule bites: **take selects after the fields**, because the cursor
   /// moves only when a field is taken and a select behind the last field of
   /// a body has not been resolved while a class is still taking fields.
   ///
   /// sawSelect() is false for a select on a branch the body did not take.
   /// One inside a loop keeps what the last iteration read.
   bool sawSelect(U32 node) const;
   U32 selectValue(U32 node) const;

   /// Report a class asking for a select at a node that holds something else.
   void failSelect(U32 node);


   U32         readInt();     ///< OpInt, OpRaw, OpRangedU32, OpDataBlockId
   F32         readFloat();   ///< OpFloat, OpSignedFloat, OpRangedF32
   ColorI      readColor();   ///< OpColor
   Point3F     readPoint();   ///< OpPoint
   const char *readString();  ///< OpString

   /// Read out and discard whatever is left.  True if the layout ran to its
   /// end with nothing unaccounted for.
   bool finish();

   /// The last .dts named by a string field, which is how a shape is found.
   const char *shapeFile() const { return mShape[0] ? mShape : NULL; }

private:
   enum { MaxDepth = 8, MaxLoop = 4096, MaxSteps = 65536 };

   /// The largest layout is TurretImageData at 1,292 nodes.  Recording flags
   /// by node rather than as a list of them costs two bits per node of the
   /// layout instead of a word per flag read -- 512 bytes of stack, no
   /// allocation, and no cap that could overrun: one walk resolves up to 740
   /// decisions.  This is the path every ghost body takes, and TSStatic alone
   /// arrives 575 times in one session.
   enum { MaxNodes = 2048, FlagWords = MaxNodes / 32 };

   /// A select's value does not fit the two bitsets a flag needs, so it is
   /// kept by node as a byte: OpSelect jumps through a table of 2^bits
   /// entries, which bounds its width -- the widest in any layout is six bits
   /// -- and a byte per node of the largest layout is 2 KB of stack against
   /// the 2.8 KB a list of what was read would need.  Measured, because the
   /// list looked cheaper until it was: one TurretImageData body resolves 351
   /// distinct selects, so a list would have to be at least that long and
   /// could still be overrun, while by-node cannot be.
   ///
   /// Only the seen bitset is cleared per body.  A value byte is read only
   /// where its seen bit is set, and setting that bit is what writes it.
   ///
   /// OpReadBr is the exception and reads up to a full word, so those few go
   /// in a short list instead -- there are 28 of them in all the tables and at
   /// most 6 in any one layout, so BranchValues of 16 cannot be reached.
   enum { BranchValues = 16 };

   struct Frame
   {
      S16 body;      ///< the node the body starts at
      S16 ret;       ///< where to go when the last iteration ends
      U32 remaining; ///< iterations still to run, this one included
   };

   bool isValue(U32 op) const;
   /// Resolve control flow until the cursor stands on a field or the walk ends.
   void advance();
   /// Leave the innermost body, taking the next iteration or returning.
   void unwind();
   void fail(const char *why);
   bool want(U32 op, const char *asked);
   void step();
   void note(const char *str);
   void noteFlag(S32 at, bool value);
   void noteSelect(S32 at, U32 value);

   BitStream *mStream;
   const T2Protocol::T2DataBlockLayout *mLayout;
   Env *mEnv;
   S32 mAt;
   bool mInitial;
   bool mFailed;
   bool mEnded;   ///< stopped on OpEnd/OpEndBody rather than running off
   U32 mSlots[8];
   Frame mStack[MaxDepth];
   U32 mDepth;
   U32 mSteps;
   char mShape[256];
   char mText[256];   ///< BitStream::readString writes 256
   U32 mFlagSeen[FlagWords];
   U32 mFlagValue[FlagWords];
   U32 mSelectSeen[FlagWords];
   U8  mSelectValue[MaxNodes];
   U16 mBranchNode[BranchValues];
   U32 mBranchValue[BranchValues];
   U32 mBranchCount;
};

#endif
