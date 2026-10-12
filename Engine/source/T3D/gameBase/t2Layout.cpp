#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "console/netClassTable.h"
#include "math/mPoint3.h"
#include "math/mQuat.h"
#include "math/mMathFn.h"
#include "T3D/gameBase/t2Layout.h"

namespace T2Layout
{
#include "T3D/gameBase/t2DataBlockTables.h"
#include "T3D/gameBase/t2GhostTablesRecovered.h"
#include "T3D/gameBase/t2EventTables.h"
#include "T3D/gameBase/t2EventTablesRecovered.h"
#include "T3D/gameBase/t2PacketDataTables.h"
#include "T3D/gameBase/t2PacketDataTablesRecovered.h"
#include "T3D/gameBase/t2GhostFields.h"
#include "T3D/gameBase/t2DataBlockFields.h"

using namespace T2Protocol;

static GhostClassFn sResolve = NULL;
static void *sResolveCtx = NULL;
static char sShapeName[256] = { 0 };
static U32 sDataBlockId = 0;
static Vector<U32> sDataBlockIds(__FILE__, __LINE__);
static char sFirstString[256] = { 0 };
static U32 sFirstWord32 = 0;
static bool sHaveFirstWord32 = false;
static Vector<String> sStrings(__FILE__, __LINE__);
static Vector<U32> sWords(__FILE__, __LINE__);
static Body sBody;

static const T2GhostFieldRow *sRows[16] = { NULL };
static U32 sRowCount = 0;
static const char *sFieldsFor = NULL;

static const T2DataBlockFieldRow *sDbRows[16] = { NULL };
static U32 sDbRowCount = 0;
static S32 sDbRowsFor = -1;
static const void *sDbLayout = NULL;
static F32 sDbValues[T2DbFieldCount] = { 0.0f };
static U32 sDbWords[T2DbFieldCount] = { 0 };
static bool sDbHas[T2DbFieldCount] = { false };

static S32 sTransformAt = -1;
static S32 sAffineAt = -1;
static S32 sScaleAt = -1;
static bool sQuatNegative = false;
static F32 sSpeed = 0.0f;
static const void *sFieldsLayout = NULL;

const Body &lastBody()
{
   return sBody;
}

bool lastDataBlockField(U32 field, F32 *out)
{
   if(field >= T2DbFieldCount || !sDbHas[field])
      return false;
   *out = sDbValues[field];
   return true;
}

bool lastDataBlockWord(U32 field, U32 *out)
{
   if(field >= T2DbFieldCount || !sDbHas[field])
      return false;
   *out = sDbWords[field];
   return true;
}

const char *lastShapeFile()
{
   return sShapeName[0] ? sShapeName : NULL;
}

U32 firstDataBlockId()
{
   return sDataBlockId;
}

const Vector<U32> &lastDataBlockIds()
{
   return sDataBlockIds;
}

const char *firstString()
{
   return sFirstString[0] ? sFirstString : NULL;
}

bool firstWord32(U32 *out)
{
   if(!sHaveFirstWord32)
      return false;
   *out = sFirstWord32;
   return true;
}

const Vector<String> &lastStrings()
{
   return sStrings;
}

const Vector<U32> &lastWords()
{
   return sWords;
}

String resourcePath(const char *name, const char *dir)
{
   String path(name);
   if(path.find('/') != String::NPos || path.find('\\') != String::NPos)
      return path;
   return String(dir) + path;
}

static void noteString(const char *str)
{
   if(sShapeName[0] || !str)
      return;

   const U32 len = dStrlen(str);
   if(len > 4 && !dStricmp(str + len - 4, ".dts"))
   {
      dStrncpy(sShapeName, str, sizeof(sShapeName) - 1);
      sShapeName[sizeof(sShapeName) - 1] = 0;
   }
}

/// Reading takes a node's value off the wire; writing puts a default one on
/// it at the same width. Every branch below therefore asks for its value
/// through one of these two and then treats it the same way, so the writer
/// cannot drift away from the reader's idea of how wide a node is.
static U32 xferInt(BitStream *stream, U32 bits, bool write)
{
   if(!bits)
      return 0;

   if(write)
   {
      stream->writeInt(0, bits);
      return 0;
   }

   return stream->readInt(bits);
}

static F32 signedFloatOf(U32 word, U32 bits)
{
   const U32 mask = (1 << bits) - 1;
   return (F32)(word & mask) * 2.0f / (F32)mask - 1.0f;
}

static Point3F normalVector(U32 word, U32 bits)
{
   const F32 phi   = signedFloatOf(word, bits + 1) * M_PI_F;
   const F32 theta = signedFloatOf(word >> (bits + 1), bits) * M_HALFPI_F;

   return Point3F(mSin(phi) * mCos(theta), mCos(phi) * mCos(theta),
                  mSin(theta));
}

static bool xferFlag(BitStream *stream, bool write)
{
   if(write)
   {
      stream->writeFlag(false);
      return false;
   }

   return stream->readFlag();
}

static const T2GhostFieldRow *fieldAt(const T2DataBlockLayout *layout, S32 at)
{
   if(!sRowCount || layout != sFieldsLayout)
      return NULL;

   for(U32 i = 0; i < sRowCount; i++)
      if(sRows[i]->node == at)
         return sRows[i];

   return NULL;
}

static const T2DataBlockFieldRow *dbFieldAt(const T2DataBlockLayout *layout,
                                            S32 at)
{
   if(!sDbRowCount || layout != sDbLayout)
      return NULL;

   for(U32 i = 0; i < sDbRowCount; i++)
      if(sDbRows[i]->node == at)
         return sDbRows[i];

   return NULL;
}

/// The nodes of one image slot, in the order the walk reaches them.
enum T2ImagePart
{
   T2ImageMask,                     ///< the flag that says the body carried
                                    ///< this slot
   T2ImageHasBlock,
   T2ImageBlockId,
   T2ImageHasSkin,
   T2ImageSkinKind,
   T2ImageSkinName,
   T2ImageSkinTag,
   T2ImageTrigger,
   T2ImageLoaded,
   T2ImageAmmo,
   T2ImageWet,
   T2ImageTarget,
   T2ImageFireCount,
   T2ImageInitial,
   T2ImageExtra,                    ///< one more flag, on an initial body only

   T2ImageParts
};


struct T2ImageMap
{
   const T2DataBlockLayout *layout;
   U32                      slots;  ///< the longest run found, for the log
   Vector<S16>              part;   ///< node -> slot * T2ImageParts + part,
                                    ///< or -1
};

static Vector<T2ImageMap *> sImageMaps(__FILE__, __LINE__);
static const T2ImageMap *sImages = NULL;

static const T2DataNode *nodeAt(const T2DataBlockLayout *layout, S32 at)
{
   return at >= 0 && U32(at) < layout->count ? &layout->nodes[at] : NULL;
}

/// A flag the body keeps the value of but does not branch on: both arms of it
/// go to the same place.
static bool isBareFlag(const T2DataNode *n)
{
   return n && n->op == OpFlag && n->bits == 1 && n->a == n->b;
}

   /// An image slot is fifteen nodes of one particular shape, in the order two
   /// independent readings of ShapeBase::unpackUpdate agree on.
static bool matchImageSlot(const T2DataBlockLayout *layout, S32 top,
                           S16 *parts)
{
   const T2DataNode *mask = nodeAt(layout, top);
   if(!mask || mask->op != OpFlag || mask->bits != 1 || mask->a == mask->b)
      return false;

   /// Where a slot the body left out carries on, which is where the slot it
   /// did carry has to end up as well.
   const S32 after = mask->a;

   const T2DataNode *hasBlock = nodeAt(layout, mask->b);
   if(!hasBlock || hasBlock->op != OpFlag || hasBlock->bits != 1)
      return false;

   const T2DataNode *blockId = nodeAt(layout, hasBlock->b);
   if(!blockId || blockId->op != OpDataBlockId || blockId->bits != 11 ||
      blockId->a != hasBlock->a)
      return false;

   const T2DataNode *hasSkin = nodeAt(layout, hasBlock->a);
   if(!hasSkin || hasSkin->op != OpFlag || hasSkin->bits != 1)
      return false;

   const T2DataNode *skinKind = nodeAt(layout, hasSkin->b);
   if(!skinKind || skinKind->op != OpFlag || skinKind->bits != 1)
      return false;

   const T2DataNode *skinName = nodeAt(layout, skinKind->a);
   if(!skinName || skinName->op != OpString || skinName->a != hasSkin->a)
      return false;

   const T2DataNode *skinTag = nodeAt(layout, skinKind->b);
   if(!skinTag || skinTag->op != OpRaw || skinTag->bits != 10 ||
      skinTag->a != hasSkin->a)
      return false;

   S32 flag[5];
   S32 walk = hasSkin->a;
   for(U32 i = 0; i < 5; i++)
   {
      const T2DataNode *n = nodeAt(layout, walk);
      if(!isBareFlag(n))
         return false;
      flag[i] = walk;
      walk = n->a;
   }

   const T2DataNode *fireCount = nodeAt(layout, walk);
   if(!fireCount || fireCount->op != OpRaw || fireCount->bits != 3)
      return false;
   const S32 fireCountAt = walk;

   const T2DataNode *initial = nodeAt(layout, fireCount->a);
   if(!initial || initial->op != OpInitial || initial->a != after)
      return false;

   const T2DataNode *extra = nodeAt(layout, initial->b);
   if(!isBareFlag(extra) || extra->a != after)
      return false;

   parts[T2ImageMask]      = S16(top);
   parts[T2ImageHasBlock]  = S16(mask->b);
   parts[T2ImageBlockId]   = S16(hasBlock->b);
   parts[T2ImageHasSkin]   = S16(hasBlock->a);
   parts[T2ImageSkinKind]  = S16(hasSkin->b);
   parts[T2ImageSkinName]  = S16(skinKind->a);
   parts[T2ImageSkinTag]   = S16(skinKind->b);
   parts[T2ImageTrigger]   = S16(flag[0]);
   parts[T2ImageLoaded]    = S16(flag[1]);
   parts[T2ImageAmmo]      = S16(flag[2]);
   parts[T2ImageWet]       = S16(flag[3]);
   parts[T2ImageTarget]    = S16(flag[4]);
   parts[T2ImageFireCount] = S16(fireCountAt);
   parts[T2ImageInitial]   = S16(fireCount->a);
   parts[T2ImageExtra]     = S16(initial->b);
   return true;
}

   /// The slots run one into the next: a slot's unread arm is the next slot's
   /// first node.
static const T2ImageMap *resolveImages(const T2DataBlockLayout *layout)
{
   for(U32 i = 0; i < sImageMaps.size(); i++)
      if(sImageMaps[i]->layout == layout)
         return sImageMaps[i];

   T2ImageMap *map = new T2ImageMap;
   map->layout = layout;
   map->slots = 0;
   map->part.setSize(layout->count);
   for(U32 i = 0; i < layout->count; i++)
      map->part[i] = -1;

   Vector<S16> parts(__FILE__, __LINE__);
   parts.setSize(layout->count * T2ImageParts);

   Vector<bool> isSlot(__FILE__, __LINE__);
   isSlot.setSize(layout->count);
   for(U32 i = 0; i < layout->count; i++)
      isSlot[i] = matchImageSlot(layout, S32(i), &parts[i * T2ImageParts]);

   for(U32 i = 0; i < layout->count; i++)
   {
      if(!isSlot[i])
         continue;

      // A run's first block is one no other block runs into.
      bool head = true;
      for(U32 j = 0; j < layout->count && head; j++)
         if(isSlot[j] && layout->nodes[j].a == S32(i))
            head = false;
      if(!head)
         continue;

      U32 slot = 0;
      for(S32 at = S32(i);
          slot < MaxImages && at >= 0 && U32(at) < layout->count && isSlot[at];
          at = layout->nodes[at].a, slot++)
         for(U32 p = 0; p < T2ImageParts; p++)
            map->part[parts[at * T2ImageParts + p]] =
               S16(slot * T2ImageParts + p);

      if(slot > map->slots)
         map->slots = slot;
   }

   if(map->slots)
      Con::printf("T2 layout: %s carries %d image slots", layout->name,
                  map->slots);

   sImageMaps.push_back(map);
   return map;
}

/// Which slot and field this node is, where it is one of an image slot's.
static bool imageAt(const T2DataBlockLayout *layout, S32 at, U32 *slot,
                    U32 *part)
{
   if(!sImages || sImages->layout != layout || at < 0 ||
      U32(at) >= sImages->part.size())
      return false;

   const S16 what = sImages->part[at];
   if(what < 0)
      return false;

   *slot = U32(what) / T2ImageParts;
   *part = U32(what) % T2ImageParts;
   return true;
}

static bool runGraph(BitStream *stream, const T2DataBlockLayout *layout,
                     S32 at, U32 *slots, U32 depth, bool isInitial,
                     bool write)
{
   if(depth > 8)
   {
      Con::warnf("T2 layout: %s nested deeper than 8", layout->name);
      return false;
   }

   char buf[512];
   U32 guard = 0;

   while(at >= 0 && U32(at) < layout->count)
   {
      if(++guard > 65536)
      {
         Con::warnf("T2 layout: %s spinning at node %d", layout->name, at);
         return false;
      }

      const T2DataNode &n = layout->nodes[at];

      switch(n.op)
      {
         case OpEnd:
         case OpEndBody:
            return true;

         case OpFlag:
         {
            const bool set = xferFlag(stream, write);
            if(!write)
            {
               const T2GhostFieldRow *row = fieldAt(layout, at);
               if(row && row->field == T2FieldQuatSign)
                  sQuatNegative = set;
               if(row && row->field == T2FieldVelocityHere)
               {
                  sBody.hasVelocity = true;
                  sBody.velocity.set(0.0f, 0.0f, 0.0f);
               }

               U32 slot, part;
               if(imageAt(layout, at, &slot, &part))
               {
                  Body::Image &image = sBody.images[slot];
                  switch(part)
                  {
                     case T2ImageMask:    image.present     = set; break;
                     case T2ImageTrigger: image.triggerDown = set; break;
                     case T2ImageLoaded:  image.loaded      = set; break;
                     case T2ImageAmmo:    image.ammo        = set; break;
                     case T2ImageWet:     image.wet         = set; break;
                     case T2ImageTarget:  image.target      = set; break;
                     default:                                      break;
                  }
               }
            }
            at = set ? n.b : n.a;
            continue;
         }

         case OpInitial:
            at = isInitial ? n.b : n.a;
            continue;

         case OpCountBits:
         {
            U32 v = xferInt(stream, n.bits, write);
            U32 set = 0;
            for(; v; v >>= 1)
               set += v & 1;
            slots[n.b & 7] = set;
            at = n.a;
            continue;
         }

         case OpPacketDataOf:
         {
            const U32 idx = xferInt(stream, n.bits, write);
            const char *cls = sResolve ? sResolve(sResolveCtx, idx) : NULL;
            if(!cls)
            {
               Con::warnf("T2 layout: %s %s the packet data of ghost %d, "
                          "which we cannot name", layout->name,
                          write ? "writes" : "reads", idx);
               return false;
            }
            if(write)
            {
               if(!writePacketData(stream, cls, sResolve, sResolveCtx))
                  return false;
            }
            else if(!readPacketData(stream, cls, sResolve, sResolveCtx))
               return false;
            at = n.a;
            continue;
         }

         case OpSelect:
         {
            const U32 v = xferInt(stream, n.bits, write);
            if(!layout->select || v >= (1U << n.bits))
            {
               Con::warnf("T2 layout: %s node %d select %u out of range",
                          layout->name, at, v);
               return false;
            }
            at = layout->select[n.a + v];
            continue;
         }

         case OpReadBr:
         {
            const U32 v = xferInt(stream, n.bits, write);
            if(!layout->select)
            {
               Con::warnf("T2 layout: %s node %d has no select table",
                          layout->name, at);
               return false;
            }
            if(n.b >= 0)
               slots[n.b & 7] = v;
            at = layout->select[n.a + (v ? 1 : 0)];
            continue;
         }

         case OpCount:
            slots[n.b & 7] = xferInt(stream, n.bits, write);
            break;

         case OpLoop:
         {
            const U32 count = slots[n.bits & 7];
            if(count > 4096)
            {
               Con::warnf("T2 layout: %s node %d loop count %u",
                          layout->name, at, count);
               return false;
            }
            for(U32 i = 0; i < count; i++)
               if(!runGraph(stream, layout, n.b, slots, depth + 1, isInitial,
                            write))
                  return false;
            at = n.a;
            continue;
         }

         case OpDataBlockId:
         {
            const U32 dbId = xferInt(stream, n.bits, write);
            U32 slot, part;
            const bool isImage = !write && imageAt(layout, at, &slot, &part) &&
                                 part == T2ImageBlockId;

               // An image slot names a ShapeBaseImageData, never the object's own
               // datablock.
            if(!sDataBlockId && !isImage)
               sDataBlockId = dbId;
            if(isImage)
            {
               sBody.images[slot].hasDataBlock = true;
               sBody.images[slot].dataBlockId = dbId;
            }
            if(!write)
               sDataBlockIds.push_back(dbId);
            break;
         }

         case OpFloat:
         case OpSignedFloat:
         case OpRangedF32:
         case OpRangedU32:
         case OpColor:
            xferInt(stream, n.bits, write);
            break;

         case OpInt:
         case OpRaw:
         {
            const T2GhostFieldRow *row = write ? NULL : fieldAt(layout, at);

            if(row)
               switch(row->field)
               {
                  case T2FieldTransform: sTransformAt = sWords.size(); break;
                  case T2FieldAffine:    sAffineAt    = sWords.size(); break;
                  case T2FieldScale:     sScaleAt     = sWords.size(); break;
                  default:                                             break;
               }

            const U32 word = xferInt(stream, n.bits, write);
            if(n.bits == 32 && !write)
            {
               if(!sHaveFirstWord32)
               {
                  sHaveFirstWord32 = true;
                  sFirstWord32 = word;
               }
               sWords.push_back(word);
            }
            if(!write && n.bits == 32)
            {
               const T2DataBlockFieldRow *dbRow = dbFieldAt(layout, at);
               if(dbRow)
               {
                  dMemcpy(&sDbValues[dbRow->field], &word, sizeof(F32));
                  sDbWords[dbRow->field] = word;
                  sDbHas[dbRow->field] = true;
               }
            }
            if(row && row->field == T2FieldYaw && n.bits)
            {
               sBody.hasYaw = true;
               sBody.yaw = (F32)word / (F32)((1 << n.bits) - 1) * M_2PI_F;
            }
            if(row && row->field == T2FieldDamage && n.bits)
            {
               // readFloat(n) again: the share of the object's maxDamage.
               sBody.hasDamage = true;
               sBody.damage = (F32)word / (F32)((1 << n.bits) - 1);
            }
            if(row && row->field == T2FieldSpeed)
               sSpeed = (F32)word / 32.0f;
            if(row && row->field == T2FieldDirection)
               sBody.velocity = normalVector(word, 10) * sSpeed;

            if(!write)
            {
               U32 slot, part;
               if(imageAt(layout, at, &slot, &part))
               {
                  if(part == T2ImageSkinTag)
                  {
                     sBody.images[slot].hasSkinTag = true;
                     sBody.images[slot].skinTag = word;
                  }
                  else if(part == T2ImageFireCount)
                     sBody.images[slot].fireCount = word;
               }
            }
            break;
         }

         case OpPoint:
         {
            Point3F p(0.0f, 0.0f, 0.0f);
            if(write)
               stream->writeCompressedPoint(p, 0.01f);
            else
            {
               stream->readCompressedPoint(&p, 0.01f);
               const T2GhostFieldRow *row = fieldAt(layout, at);
               if(row && row->field == T2FieldPosition)
               {
                  sBody.hasPosition = true;
                  sBody.position = p;
               }
            }
            break;
         }

         case OpString:
            if(write)
               stream->writeString("");
            else
            {
               stream->readString(buf);
               noteString(buf);
               sStrings.push_back(String(buf));
               if(!sFirstString[0])
               {
                  dStrncpy(sFirstString, buf, sizeof(sFirstString) - 1);
                  sFirstString[sizeof(sFirstString) - 1] = 0;
               }

               U32 slot, part;
               if(imageAt(layout, at, &slot, &part) &&
                  part == T2ImageSkinName)
               {
                  sBody.images[slot].hasSkinName = true;
                  sBody.images[slot].skinName = buf;
               }
            }
            break;

         default:
            Con::warnf("T2 layout: %s unknown op %d at node %d",
                       layout->name, n.op, at);
            return false;
      }
      at = n.a;
   }

   if(at >= 0)
      Con::warnf("T2 layout: %s node %d past end (%d)", layout->name, at,
                 layout->count);
   return at < 0;
}

static const T2DataBlockLayout *findNamed(const char *name,
                                          const T2DataBlockLayout *table,
                                          U32 count)
{
   if(!name)
      return NULL;

   for(U32 i = 0; i < count; i++)
      if(!dStrcmp(table[i].name, name))
         return &table[i];

   return NULL;
}

static bool runNamed(BitStream *stream, const char *name,
                     const T2DataBlockLayout *table, U32 count,
                     bool *found = NULL, bool isInitial = false,
                     bool write = false)
{
   const T2DataBlockLayout *layout = findNamed(name, table, count);
   if(!layout)
      return false;

   if(found)
      *found = true;

   U32 slots[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
   return runGraph(stream, layout, layout->root, slots, 0, isInitial, write);
}

#define ELEMENTS( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

static Vector<StringTableEntry> sUnimplemented(__FILE__, __LINE__);

static void warnUnimplemented(const char *kind, const char *name, bool write)
{
   char what[192];
   dSprintf(what, sizeof(what), "%s %s", kind, name ? name : "(unnamed)");

   char keyed[256];
   dSprintf(keyed, sizeof(keyed), "%s %s", write ? "write" : "read", what);

   StringTableEntry key = StringTable->insert(keyed);
   for(U32 i = 0; i < sUnimplemented.size(); i++)
      if(sUnimplemented[i] == key)
         return;
   sUnimplemented.push_back(key);

   Con::errorf("No Tribes 2 layout for %s -- the rest of the %s is lost.",
               what, write ? "body we are writing" : "packet");
}

static void resolveDataBlockFields(U32 classId, const T2DataBlockLayout *layout)
{
   if(S32(classId) == sDbRowsFor)
      return;

   sDbRowsFor = S32(classId);
   sDbRowCount = 0;
   sDbLayout = layout;

   for(U32 i = 0; i < ELEMENTS(sT2DataBlockFields); i++)
   {
      const T2DataBlockFieldRow &row = sT2DataBlockFields[i];
      if(dStrcmp(row.cls, layout->name))
         continue;

      if(row.node < 0 || U32(row.node) >= layout->count)
      {
         Con::errorf("T2 datablock field: %s node %d is past the end of its "
                     "layout -- this class is acting on its defaults.",
                     layout->name, row.node);
         continue;
      }

      const T2DataNode &n = layout->nodes[row.node];
      if(n.op != row.op || n.bits != row.bits)
      {
         Con::errorf("T2 datablock field: %s node %d is op %d of %d bits, not "
                     "the op %d of %d bits it was named against -- the layout "
                     "moved under the field, so this class is acting on its "
                     "defaults.", layout->name, row.node, n.op, n.bits,
                     row.op, row.bits);
         continue;
      }

      if(sDbRowCount >= ELEMENTS(sDbRows))
      {
         Con::errorf("T2 datablock field: %s names more fields than there is "
                     "room for; node %d is being ignored.", layout->name,
                     row.node);
         continue;
      }

      sDbRows[sDbRowCount++] = &row;
   }
}

static bool runDataBlock(BitStream *stream, U32 classId, bool write)
{
   AssertFatal(ELEMENTS(sT2DataBlocks) == NetClassTable::DataBlockCount,
      "The datablock layouts are not one per net class id!");

   sShapeName[0] = 0;
   sDataBlockIds.clear();
   sStrings.clear();
   if(classId >= ELEMENTS(sT2DataBlocks))
   {
      char id[32];
      dSprintf(id, sizeof(id), "class id %u", classId);
      warnUnimplemented("datablock", id, write);
      return false;
   }

   const T2DataBlockLayout *layout = &sT2DataBlocks[classId];
   resolveDataBlockFields(classId, layout);
   for(U32 i = 0; i < T2DbFieldCount; i++)
      sDbHas[i] = false;

   U32 slots[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
   return runGraph(stream, layout, layout->root, slots, 0, false, write);
}

static bool isRawRun(const T2DataBlockLayout *layout, S32 at, U32 words)
{
   for(U32 i = 0; i < words; i++)
   {
      if(at < 0 || U32(at) >= layout->count)
         return false;

      const T2DataNode &n = layout->nodes[at];
      if(n.op != OpRaw || n.bits != 32)
         return false;

      at = n.a;
   }

   return true;
}

static void resolveGhostFields(const char *className)
{
   if(className == sFieldsFor)
      return;

   sFieldsFor = className;
   sRowCount = 0;
   sFieldsLayout = NULL;
   sImages = NULL;

   const T2DataBlockLayout *layout =
      findNamed(className, sT2GhostsRecovered, ELEMENTS(sT2GhostsRecovered));
   if(!layout)
      return;

   sFieldsLayout = layout;
   sImages = resolveImages(layout);

   for(U32 i = 0; i < ELEMENTS(sT2GhostFields); i++)
   {
      const T2GhostFieldRow &row = sT2GhostFields[i];
      if(dStrcmp(row.cls, className))
         continue;

      if(row.node < 0 || U32(row.node) >= layout->count)
      {
         Con::errorf("T2 field: %s node %d is past the end of its layout -- "
                     "the transform of this class is not being read.",
                     className, row.node);
         continue;
      }

      const T2DataNode &n = layout->nodes[row.node];
      if(n.op != row.op || n.bits != row.bits)
      {
         Con::errorf("T2 field: %s node %d is op %d of %d bits, not the op %d "
                     "of %d bits it was named against -- the layout moved "
                     "under the field, so the transform of this class is not "
                     "being read.", className, row.node, n.op, n.bits,
                     row.op, row.bits);
         continue;
      }

      const U32 words = t2FieldWords(row.field);
      if(words && !isRawRun(layout, row.node, words))
      {
         Con::errorf("T2 field: %s node %d does not start %d words of float "
                     "-- the layout moved under the field, so the transform "
                     "of this class is not being read.", className, row.node,
                     words);
         continue;
      }

      if(sRowCount >= ELEMENTS(sRows))
      {
         Con::errorf("T2 field: %s names more fields than there is room for; "
                     "node %d is being ignored.", className, row.node);
         continue;
      }

      sRows[sRowCount++] = &row;
   }
}

static F32 wordFloat(U32 at)
{
   F32 f;
   dMemcpy(&f, &sWords[at], sizeof(f));
   return f;
}

static void assembleBody()
{
   if(sTransformAt >= 0 && sWords.size() >= U32(sTransformAt) + 16)
   {
      F32 *m = sBody.rotation;
      for(U32 i = 0; i < 16; i++)
         m[i] = wordFloat(sTransformAt + i);

      sBody.hasRotation = true;
      sBody.position = sBody.rotation.getPosition();
      sBody.hasPosition = true;
   }
   else if(sAffineAt >= 0 && sWords.size() >= U32(sAffineAt) + 6)
   {
      QuatF q;
      q.x = wordFloat(sAffineAt + 3);
      q.y = wordFloat(sAffineAt + 4);
      q.z = wordFloat(sAffineAt + 5);
      q.w = mSqrt(1.0f - getMin(q.x * q.x + q.y * q.y + q.z * q.z, 1.0f));
      if(sQuatNegative)
         q.w = -q.w;

      q.setMatrix(&sBody.rotation);
      sBody.hasRotation = true;
      sBody.position.set(wordFloat(sAffineAt), wordFloat(sAffineAt + 1),
                         wordFloat(sAffineAt + 2));
      sBody.hasPosition = true;
   }

   if(sScaleAt >= 0 && sWords.size() >= U32(sScaleAt) + 3)
   {
      sBody.scale.set(wordFloat(sScaleAt), wordFloat(sScaleAt + 1),
                      wordFloat(sScaleAt + 2));
      sBody.hasScale = true;
   }
}

static bool runGhost(BitStream *stream, const char *className,
                     bool isInitial, bool write)
{
   sDataBlockId = 0;
   sFirstString[0] = 0;
   sHaveFirstWord32 = false;
   sFirstWord32 = 0;
   sStrings.clear();
   sWords.clear();
   sBody.hasPosition = false;
   sBody.hasYaw = false;
   sBody.hasRotation = false;
   sBody.hasScale = false;
   sBody.hasVelocity = false;
   sBody.hasDamage = false;
   for(U32 i = 0; i < MaxImages; i++)
   {
      Body::Image &image = sBody.images[i];
      image.present = false;
      image.hasDataBlock = false;
      image.dataBlockId = 0;
      image.hasSkinName = false;
      image.skinName = String::EmptyString;
      image.hasSkinTag = false;
      image.skinTag = 0;
      image.triggerDown = false;
      image.loaded = false;
      image.ammo = false;
      image.wet = false;
      image.target = false;
      image.fireCount = 0;
   }
   sTransformAt = -1;
   sAffineAt = -1;
   sScaleAt = -1;
   sQuatNegative = false;
   sSpeed = 0.0f;
   resolveGhostFields(className);
   bool found = false;
   const bool ok = runNamed(stream, className, sT2GhostsRecovered,
                            ELEMENTS(sT2GhostsRecovered), &found, isInitial,
                            write);
   if(!found)
      warnUnimplemented("ghost", className, write);
   if(ok && !write)
      assembleBody();
   return ok;
}

static bool runEvent(BitStream *stream, const char *className, bool write)
{
   bool found = false;
   const bool ok = runNamed(stream, className, sT2EventsRecovered,
                            ELEMENTS(sT2EventsRecovered), &found, false,
                            write);
   if(found)
      return ok;

   const bool shipped = runNamed(stream, className, sT2Events,
                                 ELEMENTS(sT2Events), &found, false, write);
   if(!found)
      warnUnimplemented("event", className, write);
   return shipped;
}

static bool runPacketData(BitStream *stream, const char *className,
                          GhostClassFn resolve, void *ctx, bool write)
{
   sResolve = resolve;
   sResolveCtx = ctx;

   bool found = false;
   bool ok = runNamed(stream, className, sT2PacketDataRecovered,
                      ELEMENTS(sT2PacketDataRecovered), &found, false, write);
   if(!found)
      ok = runNamed(stream, className, sT2PacketData,
                    ELEMENTS(sT2PacketData), &found, false, write);
   if(!found)
      warnUnimplemented("control object", className, write);
   return ok;
}

bool readDataBlock(BitStream *stream, U32 classId)
{
   return runDataBlock(stream, classId, false);
}

bool readGhost(BitStream *stream, const char *className, bool isInitial)
{
   return runGhost(stream, className, isInitial, false);
}

bool readEvent(BitStream *stream, const char *className)
{
   return runEvent(stream, className, false);
}

bool readPacketData(BitStream *stream, const char *className,
                    GhostClassFn resolve, void *ctx)
{
   return runPacketData(stream, className, resolve, ctx, false);
}

bool writeDataBlock(BitStream *stream, U32 classId)
{
   return runDataBlock(stream, classId, true);
}

bool writeGhost(BitStream *stream, const char *className, bool isInitial)
{
   return runGhost(stream, className, isInitial, true);
}

bool writeEvent(BitStream *stream, const char *className)
{
   return runEvent(stream, className, true);
}

bool writePacketData(BitStream *stream, const char *className,
                     GhostClassFn resolve, void *ctx)
{
   return runPacketData(stream, className, resolve, ctx, true);
}

}
