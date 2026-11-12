#include "platform/platform.h"
#include "console/console.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"
#include "T3D/gameBase/stubsT2.h"
#include "console/simBase.h"
#include "core/strings/stringFunctions.h"
#include "math/mMathFn.h"
#include "T3D/gameBase/t2Layout.h"

namespace StubT2
{

static Vector<StringTableEntry> sWarned(__FILE__, __LINE__);

void publish(SimObject *object, const Field *fields, U32 count)
{
   for(U32 i = 0; i < count; i++)
   {
      const Field &field = fields[i];
      // A constant is not a field of the object at all: its literal is what
      // every body retail writes carries there.
      if(field.kind == Constant)
      {
         T2Layout::publishDataBlockWhole(field.slot, dAtof(field.name));
         continue;
      }

      const char *value = object->getDataField(
         StringTable->insert(field.name), NULL);
      if(!value || !value[0])
         value = field.fallback;
      if(!value || !value[0])
         continue;

      switch(field.kind)
      {
         case Whole:
            T2Layout::publishDataBlockWhole(field.slot, dAtof(value));
            break;

         case Colour:
         {
            // Four floats in the script, four bytes on the wire.
            F32 rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            dSscanf(value, "%g %g %g %g", &rgba[0], &rgba[1], &rgba[2], &rgba[3]);

            // A colour's bytes are rounded, where a float scaled into a
            // bit field is truncated: retail sends 128 for an alpha of 0.5,
            // and ScoutChaingunHit's second colour is the body that says so.
            U32 word = 0;
            for(U32 c = 0; c < 4; c++)
               word |= T2Layout::t2ColorByte(rgba[c]) << (c * 8);
            T2Layout::publishDataBlockWord(field.slot, word);
            break;
         }

         case Vector3:
         {
            F32 xyz[3] = { 0.0f, 0.0f, 0.0f };
            dSscanf(value, "%g %g %g", &xyz[0], &xyz[1], &xyz[2]);
            for(U32 c = 0; c < 3; c++)
               T2Layout::publishDataBlockWhole(field.slot + c, xyz[c]);
            break;
         }

         case Word:
            T2Layout::publishDataBlockWord(field.slot, U32(dAtoi(value)));
            break;

         case Word32:
            T2Layout::publishDataBlockWord(field.slot, U32(dAtoui(value, 0)));
            break;

         case Ticks:
            T2Layout::publishDataBlockWord(field.slot,
               T2Layout::t2TickMs(dAtoi(value),
                                  field.fallback ? dAtoi(field.fallback) : 0));
            break;

         case Flag:
            T2Layout::publishDataBlockFlag(field.slot, dAtob(value));
            break;

         case Text:
            T2Layout::publishDataBlockString(field.slot, value);
            break;

         case DataBlock:
         {
            SimObject *named = Sim::findObject(value);
            if(named)
               T2Layout::publishDataBlockWord(field.slot,
                  named->getId() - DataBlockObjectIdFirst);
            else
               Con::warnf("StubT2::publish - %s names no object '%s' for %s",
                  object->getName(), value, field.name);
            break;
         }
      }
   }
}

void warn(const char *kind, const char *className)
{
   char what[192];
   dSprintf(what, sizeof(what), "%s %s", kind,
            className ? className : "(unnamed)");

   StringTableEntry key = StringTable->insert(what);
   for(U32 i = 0; i < sWarned.size(); i++)
      if(sWarned[i] == key)
         return;
   sWarned.push_back(key);

   Con::warnf("Stub Tribes 2 %s -- read by its layout, acting on defaults.",
              what);
}

}
