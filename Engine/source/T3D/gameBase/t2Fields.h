// HH: Added for Tribes 2 compatibility.
#ifndef _T2FIELDS_H_
#define _T2FIELDS_H_

#ifndef _T2READER_H_
#include "T3D/gameBase/t2Reader.h"
#endif

/// How a class takes its Tribes 2 fields.
///
/// A ghost or a datablock does not open the stream itself.  T2Layout owns the
/// tables, so it looks the layout up, builds the cursor over it and calls
/// readT2Fields(); whatever the class does not take, it discards afterwards.
/// So a class is ported a field at a time and the packets behind it keep
/// reading in the meantime -- which is the only reason a hundred classes can
/// be ported one at a time at all.
///
///     void StaticShape::readT2Fields(T2Reader &reader)
///     {
///        Parent::readT2Fields(reader);   // innermost base first, always
///
///        T2Fields f(reader);
///        f.takeRawPoint(mT2Position);    // absent when the mask was clear
///        f.takeInt(3, mT2BeaconType);
///     }
///
/// Two things about this that are not obvious and cost a day each to learn.
///
/// **The cursor will not tell a class which field it is standing on.**  The
/// layout's branches read the stream, and T2Reader resolves every flag, select
/// and loop count itself, so a class never sees the mask bit that decided
/// whether its field is there.  All it can see is the op and the width of the
/// value the cursor stopped on.  A field is therefore recognised by its shape,
/// and every take here is tolerant: it reads only if the cursor stands on that
/// shape, and otherwise leaves the cursor alone and returns false.  false
/// means "the server did not send this one", which for an optional field is
/// the ordinary case and not an error.
///
/// **Two fields of the same shape in a row cannot be told apart.**  If a class
/// has an optional 11-bit integer followed by another one, and only the second
/// arrives, the first take() will claim it.  The stream still ends on the
/// right bit -- a take consumes exactly the node it stands on or nothing at
/// all, so nothing can desync -- but the value lands in the wrong member.
/// Where a class's fields are ambiguous by shape, take them without skipping:
/// stop at the first take() that returns false and let the rest be discarded,
/// rather than carrying on and guessing.  A class that stops early is always
/// correct, just incomplete.
///
/// Asking for a shape the cursor cannot be standing on at all -- a width no
/// node in the layout has -- is a layout disagreement rather than an absent
/// field, and T2Reader reports it against the node when the read is actually
/// attempted.  That is why these take no action on a miss.
class T2Fields
{
public:
   T2Fields(T2Reader &reader) : mReader(reader) {}

   /// Does the cursor stand on this node of the layout?
   ///
   /// For the ambiguous case the header above warns about: two optional fields
   /// of the same shape in a row, where taking by shape alone would put the
   /// second one's value in the first one's member.  The node index is the
   /// field's identity, so pair it with the take that names the shape and both
   /// have to agree:
   ///
   ///     if(f.atNode(312)) f.takeRawPoint(mT2Position);
   ///
   /// Use the shape by itself wherever it is unambiguous -- it survives the
   /// tables being regenerated and a node index does not. Here the take still
   /// checks the shape, so a regenerated table that moved this field fails on
   /// the shape rather than quietly filling the member from somewhere else.
   bool atNode(U32 node) const { return mReader.node() == node; }

   /// Take a boolean the layout branches on, named by the node that reads it.
   ///
   /// A flag is not a field: the cursor reads it to pick the next node, so it
   /// is never something to stand on and the takes below can never see one.
   /// Flags are most of this protocol -- 9,674 of 16,575 nodes -- so without
   /// this a class can read no boolean at all, and a map where nothing is
   /// powered, alarmed or destroyed is not a simulating one.
   ///
   ///     f.takeFlag(14, mT2ShowTerrainInside);
   ///
   /// False means the body never reached that flag, which is different from
   /// reaching it and finding it clear -- an untaken branch leaves the member
   /// alone rather than setting it false.
   ///
   /// **Take flags after the fields, at the end of readT2Fields.**  A flag is
   /// readable only once the cursor has gone past it, and the cursor only
   /// moves when a field is taken.  A flag sitting behind the last field of a
   /// body -- which is where a layout usually puts the ones that are always
   /// sent -- has therefore not been read yet while a class is still taking
   /// fields, and asking for it early returns false and leaves the member at
   /// its default.
   ///
   /// That failure is quiet and shaped by the body: an update with no fields
   /// in it has the cursor run to the end looking for one, so the flag *is*
   /// recorded and the value looks right, while an initial body full of
   /// fields stops short and the same code reads the default.  It was found
   /// by scripting every path through a layout; no recorded session would
   /// have shown it, because whether it shows depends on which body arrives.
   /// If a class needs one exit for this, split the field reading out into a
   /// helper and take the flags after it returns.
   ///
   /// Naming a node that is not a flag is a mistake about the layout rather
   /// than an absent value, so it is reported against the node.  That is what
   /// makes a regenerated table that moved the flag fail here instead of
   /// quietly leaving the member at its default forever.
   bool takeFlag(U32 node, bool &out)
   {
      // A lookup that found nothing, rather than a node that holds the wrong
      // thing.  Classes resolve a flag by name through T2Layout::ghostNode and
      // pass the result straight here, and a layout that names nothing -- any
      // walked one, and every datablock, since the names are a ghost table --
      // answers NoNode.  That is this flag not being on offer, which is the
      // ordinary case the take below returns false for, not a disagreement
      // worth a line in the log.  Reported, it was nineteen classes each
      // claiming "node 4294967295 is op 0" on a live join.
      if(node == U32(T2Reader::NoNode))
         return false;

      const T2Protocol::T2DataOp op = mReader.opAt(node);
      if(op != T2Protocol::OpFlag && op != T2Protocol::OpInitial)
      {
         mReader.failFlag(node);
         return false;
      }
      if(!mReader.sawFlag(node))
         return false;

      out = mReader.flag(node);
      return true;
   }

   /// takeFlag and takeSelect for a node a layout may not name at all.
   ///
   /// Those two report a node that holds something else, which is right: a
   /// class naming the wrong node is wrong about the layout.  But a layout
   /// that simply does not name this field gives NoNode, and that is an
   /// absent name rather than a mistake -- asking anyway would put a
   /// complaint on a path that runs as often as packets arrive.  A reader
   /// that resolves its nodes by name wants these.
   /// @{
   bool takeFlagNamed(U32 node, bool &out)
   {
      if(node == U32(T2Reader::NoNode))
         return false;
      return takeFlag(node, out);
   }

   bool takeSelectNamed(U32 node, U32 &out)
   {
      if(node == U32(T2Reader::NoNode))
         return false;
      return takeSelect(node, out);
   }

   bool takeSelectNamed(U32 node, S32 &out)
   {
      if(node == U32(T2Reader::NoNode))
         return false;
      return takeSelect(node, out);
   }
   /// @}

   /// Take a value the layout branches on through a jump table, named by the
   /// node that read it.
   ///
   /// takeFlag's problem one size up.  A walked layout models a field as a
   /// select whenever the binary branched on its value, and then the cursor
   /// resolves it and no class can see it: that is how ShapeBase's damage
   /// state, every image's fire count and every mount node came to be
   /// unreadable in the layouts that were not written by hand, and how a CRC
   /// challenge's whole-file flag is lost.  985 nodes of the tables are
   /// selects and 28 more are the branch-on-a-word kind.
   ///
   ///     f.takeSelect(45, mT2PrimaryAxis);
   ///
   /// False means the body never reached it, which is not the same as
   /// reaching it and reading zero.
   ///
   /// **Take selects after the fields**, for the reason takeFlag gives: the
   /// cursor moves only when a field is taken, so one sitting behind the last
   /// field of a body has not been resolved while a class is still taking
   /// fields.  The failure is quiet and depends on which body arrives.
   bool takeSelect(U32 node, U32 &out)
   {
      const T2Protocol::T2DataOp op = mReader.opAt(node);
      if(op != T2Protocol::OpSelect && op != T2Protocol::OpReadBr)
      {
         mReader.failSelect(node);
         return false;
      }
      if(!mReader.sawSelect(node))
         return false;

      out = mReader.selectValue(node);
      return true;
   }

   bool takeSelect(U32 node, S32 &out)
   {
      U32 v;
      if(!takeSelect(node, v))
         return false;
      out = S32(v);
      return true;
   }

   /// Does the cursor stand on a field of this shape?  Ops are matched as
   /// T2Reader matches them: OpInt also accepts OpRaw, OpDataBlockId and
   /// OpRangedU32, and OpFloat also accepts OpSignedFloat and OpRangedF32.
   /// A width of 0 accepts any width.
   bool at(T2Protocol::T2DataOp op, U32 bits = 0) const
   {
      if(mReader.atEnd())
         return false;
      if(bits && mReader.bits() != bits)
         return false;

      const U32 got = mReader.op();
      if(got == U32(op))
         return true;
      if(op == T2Protocol::OpInt)
         return got == T2Protocol::OpRaw || got == T2Protocol::OpDataBlockId ||
                got == T2Protocol::OpRangedU32;
      if(op == T2Protocol::OpFloat)
         return got == T2Protocol::OpSignedFloat ||
                got == T2Protocol::OpRangedF32;
      return false;
   }

   bool takeInt(U32 bits, U32 &out)
   {
      if(!at(T2Protocol::OpInt, bits))
         return false;
      out = mReader.readInt();
      return true;
   }

   bool takeInt(U32 bits, S32 &out)
   {
      U32 v;
      if(!takeInt(bits, v))
         return false;
      out = S32(v);
      return true;
   }

   bool takeInt(U32 bits, bool &out)
   {
      U32 v;
      if(!takeInt(bits, v))
         return false;
      out = v != 0;
      return true;
   }

   /// A reference to another datablock, by the id it was sent under.  Matched
   /// on the op alone rather than through takeInt, because a datablock id and
   /// an ordinary integer of the same width are not interchangeable: the
   /// layouts type this one, and a class that asked for an integer would
   /// happily take the field after it instead.
   bool takeDataBlockId(U32 &out)
   {
      if(!at(T2Protocol::OpDataBlockId))
         return false;
      out = mReader.readInt();
      return true;
   }

   /// A run of independently-optional datablock references -- the shape a
   /// datablock's list of emitters, explosions and sounds arrives in, a flag
   /// and an id each.
   ///
   /// The cursor hides the flags, so a run with holes in it cannot be told
   /// from a shorter run: if nine references are declared and seven arrive,
   /// nothing in the stream says which two are missing.  So this reports how
   /// many it took and leaves the filing to the caller, which should assign
   /// them only when the whole run arrived.  The ids of a short run are lost
   /// rather than mis-filed, which is the right way round.
   U32 takeDataBlockRun(U32 *ids, U32 count)
   {
      U32 got = 0;
      while(got < count && takeDataBlockId(ids[got]))
         got++;
      return got;
   }

   /// A fraction or a signed fraction of the stated width.  Which of the two
   /// the layout says it is decides how the bits are scaled, so a class asks
   /// for a float and gets the one the layout names.
   bool takeFloat(U32 bits, F32 &out)
   {
      if(!at(T2Protocol::OpFloat, bits))
         return false;
      out = mReader.readFloat();
      return true;
   }

   bool takeColor(ColorI &out)
   {
      if(!at(T2Protocol::OpColor))
         return false;
      out = mReader.readColor();
      return true;
   }

   /// The compressed point the layout types as one.  A position written as
   /// three machine words is takeRawPoint() instead.
   bool takePoint(Point3F &out)
   {
      if(!at(T2Protocol::OpPoint))
         return false;
      out = mReader.readPoint();
      return true;
   }

   bool takeString(char *out, U32 size)
   {
      if(!at(T2Protocol::OpString))
         return false;
      dStrncpy(out, mReader.readString(), size - 1);
      out[size - 1] = 0;
      return true;
   }

   /// A 32-bit word the layout could not type.  Wherever the field is a
   /// coordinate or any other float, that word is the float's bits: the game
   /// wrote it with mathWrite rather than a quantised form.
   bool takeRawFloat(F32 &out)
   {
      U32 word;
      if(!takeInt(32, word))
         return false;
      dMemcpy(&out, &word, sizeof(out));
      return true;
   }

   /// Three of those in a row, which is how the game writes a Point3F it has
   /// not compressed.  False if the first is absent, and false with the first
   /// one or two taken if the layout disagrees part way -- the bits read were
   /// still that node's own, so the stream stays where the layout says it is.
   bool takeRawPoint(Point3F &out)
   {
      return takeRawFloat(out.x) && takeRawFloat(out.y) && takeRawFloat(out.z);
   }

   /// Take the field the cursor stands on and throw it away.  For a field a
   /// class has to step over to reach the ones after it: the alternative is
   /// stopping, which gives up everything behind it.
   bool skip()
   {
      if(mReader.atEnd())
         return false;

      switch(mReader.op())
      {
         case T2Protocol::OpColor:  mReader.readColor();  return true;
         case T2Protocol::OpPoint:  mReader.readPoint();  return true;
         case T2Protocol::OpString: mReader.readString(); return true;

         case T2Protocol::OpFloat:
         case T2Protocol::OpSignedFloat:
         case T2Protocol::OpRangedF32:
            mReader.readFloat();
            return true;

         default:
            mReader.readInt();
            return true;
      }
   }

private:
   T2Reader &mReader;
};

#endif
