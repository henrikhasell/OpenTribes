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
      const char *value = object->getDataField(
         StringTable->insert(field.name), NULL);
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

            U32 word = 0;
            for(U32 c = 0; c < 4; c++)
            {
               const F32 part = mClampF(rgba[c], 0.0f, 1.0f);
               word |= U32(part * 255.0f) << (c * 8);
            }
            T2Layout::publishDataBlockWord(field.slot, word);
            break;
         }

         case Word:
            T2Layout::publishDataBlockWord(field.slot, U32(dAtoi(value)));
            break;

         case Flag:
            T2Layout::publishDataBlockFlag(field.slot, dAtob(value));
            break;

         case Text:
            T2Layout::publishDataBlockString(field.slot, value);
            break;
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
