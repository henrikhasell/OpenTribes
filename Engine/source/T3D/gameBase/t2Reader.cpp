#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "T3D/gameBase/t2Reader.h"

using namespace T2Protocol;

/// A reader that disagrees with its layout does so every time the class is
/// read, and a warning in a decode path runs at the rate packets arrive.  One
/// line per distinct disagreement is what makes it readable -- and what keeps
/// it from looking like a hang.
static bool saidAlready(const char *what)
{
   static Vector<StringTableEntry> said(__FILE__, __LINE__);

   StringTableEntry key = StringTable->insert(what);
   for(U32 i = 0; i < said.size(); i++)
      if(said[i] == key)
         return true;

   said.push_back(key);
   return false;
}

T2Reader::T2Reader(BitStream *stream, const T2DataBlockLayout *layout,
                   bool isInitial, Env *env)
   : mStream(stream), mLayout(layout), mEnv(env), mAt(layout ? layout->root : -1),
     mInitial(isInitial), mFailed(false), mEnded(false), mDepth(0), mSteps(0)
{
   for(U32 i = 0; i < 8; i++)
      mSlots[i] = 0;
   for(U32 i = 0; i < FlagWords; i++)
      mFlagSeen[i] = mFlagValue[i] = mSelectSeen[i] = 0;
   mBranchCount = 0;
   // mSelectValue is deliberately not cleared: a byte there is read only
   // where its seen bit is set, and setting that bit is what writes it.  This
   // is the path every ghost body takes, so the 2 KB not memset is the point.
   mShape[0] = 0;
   mText[0] = 0;

   if(!mStream || !mLayout)
   {
      mFailed = true;
      mAt = -1;
      return;
   }

   advance();
}

bool T2Reader::isValue(U32 op) const
{
   switch(op)
   {
      case OpInt:
      case OpFloat:
      case OpSignedFloat:
      case OpRangedF32:
      case OpRangedU32:
      case OpColor:
      case OpRaw:
      case OpDataBlockId:
      case OpString:
      case OpPoint:
         return true;
      default:
         return false;
   }
}

T2DataOp T2Reader::op() const
{
   if(mAt < 0 || U32(mAt) >= mLayout->count)
      return OpEnd;
   return T2DataOp(mLayout->nodes[mAt].op);
}

T2DataOp T2Reader::opAt(U32 node) const
{
   if(!mLayout || node >= mLayout->count)
      return OpEnd;
   return T2DataOp(mLayout->nodes[node].op);
}

void T2Reader::noteFlag(S32 at, bool value)
{
   if(at < 0 || U32(at) >= MaxNodes)
      return;

   const U32 w = U32(at) >> 5, b = 1U << (U32(at) & 31);
   mFlagSeen[w] |= b;
   if(value)
      mFlagValue[w] |= b;
   else
      mFlagValue[w] &= ~b;   // a later pass of a loop may say the opposite
}

void T2Reader::noteSelect(S32 at, U32 value)
{
   if(at < 0 || U32(at) >= MaxNodes)
      return;

   mSelectSeen[U32(at) >> 5] |= 1U << (U32(at) & 31);

   // A select jumps through a table of 2^bits entries, so its value is narrow
   // by construction and a byte holds it.  OpReadBr reads up to a whole word
   // and there are few of them, so those go in the short list -- keyed by
   // node, and overwritten rather than appended so that a loop around one
   // cannot grow it.
   //
   // A node that has ever carried a wide value stays in the list and is
   // updated there, even when a later pass of a loop reads a narrow one: two
   // homes for one node would mean the lookup finding the stale one.
   for(U32 i = 0; i < mBranchCount; i++)
      if(mBranchNode[i] == U16(at))
      {
         mBranchValue[i] = value;
         return;
      }

   if(value <= 0xFF)
   {
      mSelectValue[U32(at)] = U8(value);
      return;
   }

   if(mBranchCount < BranchValues)
   {
      mBranchNode[mBranchCount] = U16(at);
      mBranchValue[mBranchCount] = value;
      mBranchCount++;
      return;
   }

   // Unreachable with the tables as they are -- six wide branches is the most
   // any layout has -- so rather than lose the value quietly, say the record
   // is not to be trusted for this node.
   mSelectSeen[U32(at) >> 5] &= ~(1U << (U32(at) & 31));
}

bool T2Reader::sawFlag(U32 node) const
{
   if(node >= MaxNodes)
      return false;
   return (mFlagSeen[node >> 5] & (1U << (node & 31))) != 0;
}

bool T2Reader::flag(U32 node) const
{
   if(node >= MaxNodes)
      return false;
   return (mFlagValue[node >> 5] & (1U << (node & 31))) != 0;
}

bool T2Reader::sawSelect(U32 node) const
{
   if(node >= MaxNodes)
      return false;
   return (mSelectSeen[node >> 5] & (1U << (node & 31))) != 0;
}

U32 T2Reader::selectValue(U32 node) const
{
   if(node >= MaxNodes)
      return 0;

   for(U32 i = 0; i < mBranchCount; i++)
      if(mBranchNode[i] == U16(node))
         return mBranchValue[i];

   return mSelectValue[node];
}

void T2Reader::failSelect(U32 node)
{
   // As failFlag: a class naming the wrong node is wrong about that one
   // field, and this is a packet path, so it is said once and the rest of the
   // body still reads.
   char what[192];
   dSprintf(what, sizeof(what), "%s: node %u is op %d, asked for a select",
            mLayout ? mLayout->name : "(none)", node, opAt(node));
   if(!saidAlready(what))
      Con::errorf("T2 layout: %s", what);
}

void T2Reader::failFlag(U32 node)
{
   // Not fail(): a class naming the wrong node is wrong about that one field,
   // and abandoning the rest of the body over it would lose the fields it did
   // get right along with every packet sharing the datagram.
   char what[192];
   dSprintf(what, sizeof(what), "%s: node %u is op %d, asked for a flag",
            mLayout ? mLayout->name : "(none)", node, opAt(node));
   if(!saidAlready(what))
      Con::errorf("T2 layout: %s", what);
}

U32 T2Reader::bits() const
{
   if(mAt < 0 || U32(mAt) >= mLayout->count)
      return 0;
   return mLayout->nodes[mAt].bits;
}

void T2Reader::fail(const char *why)
{
   mFailed = true;
   mAt = -1;

   char what[384];
   dSprintf(what, sizeof(what), "%s: %s", mLayout ? mLayout->name : "(none)", why);
   if(!saidAlready(what))
      Con::errorf("T2 layout: %s", what);
}

void T2Reader::note(const char *str)
{
   if(mShape[0] || !str)
      return;

   const U32 len = dStrlen(str);
   if(len > 4 && !dStricmp(str + len - 4, ".dts"))
   {
      dStrncpy(mShape, str, sizeof(mShape) - 1);
      mShape[sizeof(mShape) - 1] = 0;
   }
}

void T2Reader::unwind()
{
   // Nothing nested: the walk is over.
   if(!mDepth)
   {
      mAt = -1;
      return;
   }

   Frame &f = mStack[mDepth - 1];
   if(--f.remaining)
   {
      mAt = f.body;
      return;
   }

   mAt = f.ret;
   mDepth--;
}

void T2Reader::advance()
{
   while(!mFailed)
   {
      if(++mSteps > (1U << 22))
      {
         fail("spinning");
         return;
      }

      if(mAt < 0)
      {
         // Ran off the end of a body, which ends an iteration as surely as
         // OpEnd does.  At the top this is the clean finish.
         if(!mDepth)
            return;
         unwind();
         continue;
      }

      if(U32(mAt) >= mLayout->count)
      {
         char why[64];
         dSprintf(why, sizeof(why), "node %d past end (%d)", mAt, mLayout->count);
         fail(why);
         return;
      }

      const T2DataNode &n = mLayout->nodes[mAt];

      if(isValue(n.op))
         return;

      switch(n.op)
      {
         case OpEnd:
         case OpEndBody:
            mEnded = true;
            unwind();
            continue;

         case OpFlag:
         {
            // Recorded before the cursor moves, so the flag is kept against
            // the node that read it rather than wherever it branched to.
            const bool set = mStream->readFlag();
            noteFlag(mAt, set);
            mAt = set ? n.b : n.a;
            continue;
         }

         case OpInitial:
            noteFlag(mAt, mInitial);
            mAt = mInitial ? n.b : n.a;
            continue;

         case OpCountBits:
         {
            U32 v = n.bits ? mStream->readInt(n.bits) : 0;
            U32 set = 0;
            for(; v; v >>= 1)
               set += v & 1;
            mSlots[n.b & 7] = set;
            mAt = n.a;
            continue;
         }

         case OpPacketDataOf:
         {
            const U32 idx = mStream->readInt(n.bits);
            const char *cls = mEnv ? mEnv->ghostClass(idx) : NULL;
            if(!cls)
            {
               fail("reads the packet data of a ghost we cannot name");
               return;
            }
            if(!mEnv->readPacketData(mStream, cls))
            {
               fail("the packet data it is piloting would not read");
               return;
            }
            mAt = n.a;
            continue;
         }

         case OpSelect:
         {
            const U32 v = mStream->readInt(n.bits);
            if(!mLayout->select || v >= (1U << n.bits))
            {
               fail("select out of range");
               return;
            }
            noteSelect(mAt, v);
            mAt = mLayout->select[n.a + v];
            continue;
         }

         case OpReadBr:
         {
            const U32 v = n.bits ? mStream->readInt(n.bits) : 0;
            if(!mLayout->select)
            {
               fail("no select table");
               return;
            }
            if(n.b >= 0)
               mSlots[n.b & 7] = v;
            noteSelect(mAt, v);
            mAt = mLayout->select[n.a + (v ? 1 : 0)];
            continue;
         }

         case OpCount:
            mSlots[n.b & 7] = n.bits ? mStream->readInt(n.bits) : 0;
            mAt = n.a;
            continue;

         case OpLoop:
         {
            const U32 count = mSlots[n.bits & 7];
            if(count > MaxLoop)
            {
               fail("loop count out of range");
               return;
            }
            if(!count)
            {
               mAt = n.a;
               continue;
            }
            if(mDepth >= MaxDepth)
            {
               fail("nested deeper than 8");
               return;
            }
            Frame &f = mStack[mDepth++];
            f.body = n.b;
            f.ret = n.a;
            f.remaining = count;
            mAt = n.b;
            continue;
         }

         default:
         {
            char why[64];
            dSprintf(why, sizeof(why), "unknown op %d at node %d", n.op, mAt);
            fail(why);
            return;
         }
      }
   }
}

void T2Reader::step()
{
   mAt = mLayout->nodes[mAt].a;
   advance();
}

bool T2Reader::want(U32 op, const char *asked)
{
   if(mAt < 0 || mFailed)
   {
      char why[96];
      dSprintf(why, sizeof(why), "asked for %s past the end of the layout", asked);
      fail(why);
      return false;
   }

   const U32 got = mLayout->nodes[mAt].op;
   if(got == op)
      return true;

   // OpRaw and OpDataBlockId are integers of a stated width, and so are the
   // ranged ops once the range is gone: reading them as one is right.
   if(op == OpInt && (got == OpRaw || got == OpDataBlockId || got == OpRangedU32))
      return true;
   if(op == OpFloat && (got == OpSignedFloat || got == OpRangedF32))
      return true;

   char why[128];
   dSprintf(why, sizeof(why), "node %d is op %d, asked for %s", mAt, got, asked);
   fail(why);
   return false;
}

U32 T2Reader::readInt()
{
   if(!want(OpInt, "an integer"))
      return 0;

   const U32 bits = mLayout->nodes[mAt].bits;
   const U32 v = bits ? mStream->readInt(bits) : 0;
   step();
   return v;
}

F32 T2Reader::readFloat()
{
   if(!want(OpFloat, "a float"))
      return 0.0f;

   const T2DataNode &n = mLayout->nodes[mAt];
   F32 v = 0.0f;
   if(n.bits)
      v = n.op == OpSignedFloat ? mStream->readSignedFloat(n.bits)
                                : mStream->readFloat(n.bits);
   step();
   return v;
}

ColorI T2Reader::readColor()
{
   if(!want(OpColor, "a colour"))
      return ColorI(255, 255, 255, 255);

   const U32 bits = mLayout->nodes[mAt].bits;
   const U32 v = bits ? mStream->readInt(bits) : 0;
   step();

   // Narrower than a word and there is no telling which channels are there,
   // so the value stands for the lot.
   if(bits < 32)
      return ColorI(v & 0xff, v & 0xff, v & 0xff, 255);

   return ColorI(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff);
}

Point3F T2Reader::readPoint()
{
   if(!want(OpPoint, "a point"))
      return Point3F(0.0f, 0.0f, 0.0f);

   Point3F p(0.0f, 0.0f, 0.0f);
   mStream->readCompressedPoint(&p, 0.01f);
   step();
   return p;
}

const char *T2Reader::readString()
{
   if(!want(OpString, "a string"))
      return "";

   mStream->readString(mText);
   note(mText);
   step();
   return mText;
}

bool T2Reader::finish()
{
   while(!mFailed && mAt >= 0)
   {
      const T2DataNode &n = mLayout->nodes[mAt];
      switch(n.op)
      {
         case OpString:
         {
            char buf[256];
            mStream->readString(buf);
            note(buf);
            break;
         }

         case OpPoint:
         {
            Point3F p;
            mStream->readCompressedPoint(&p, 0.01f);
            break;
         }

         default:
            if(n.bits)
               mStream->readInt(n.bits);
            break;
      }
      step();
   }

   return !mFailed;
}
