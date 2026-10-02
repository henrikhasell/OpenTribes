#include "platform/platform.h"
#include "console/console.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "core/util/tVector.h"
#include "T3D/gameBase/stubsT2.h"

namespace StubT2
{

static Vector<StringTableEntry> sWarned(__FILE__, __LINE__);

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
