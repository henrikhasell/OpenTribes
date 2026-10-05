#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "T3D/gameBase/t2Undescribed.h"

namespace T2Undescribed
{

enum { MaxNoted = 64 };
static const char *sNoted[MaxNoted];
static U32 sCount = 0;

void note(const char *className)
{
   if(!className)
      return;

   for(U32 i = 0; i < sCount; i++)
      if(sNoted[i] == className || !dStrcmp(sNoted[i], className))
         return;

   if(sCount < MaxNoted)
      sNoted[sCount++] = StringTable->insert(className);
}

U32 count()
{
   return sCount;
}

const char *at(U32 i)
{
   return i < sCount ? sNoted[i] : "";
}

}

DefineEngineFunction( t2Undescribed, const char *, (),,
   "@brief The Tribes 2 datablock classes that read a body with no field "
   "names in the table, and so fell back to recognising fields by the shape "
   "of a run.\n\n"
   "Space separated, empty when there are none.  A fallback is correct and "
   "worse: a body sending two of nine optional floats files none of them.\n\n"
   "@internal" )
{
   if( !T2Undescribed::count() )
      return "";

   U32 size = 1;
   for( U32 i = 0; i < T2Undescribed::count(); i++ )
      size += dStrlen( T2Undescribed::at( i ) ) + 1;

   char *out = Con::getReturnBuffer( size );
   out[0] = 0;
   for( U32 i = 0; i < T2Undescribed::count(); i++ )
   {
      if( i )
         dStrcat( out, " " );
      dStrcat( out, T2Undescribed::at( i ) );
   }
   return out;
}
