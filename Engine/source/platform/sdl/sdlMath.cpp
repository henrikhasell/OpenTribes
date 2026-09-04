#include "platform/platform.h"
#include "math/mMath.h"
#include "console/console.h"

extern void mInstallLibrary_C();
extern void mInstall_AMD_Math();
extern void mInstall_Library_SSE();

void Math::init(U32 properties)
{
   if (!properties)
      properties = Platform::SystemInfo.processor.properties;
   else
      properties &= Platform::SystemInfo.processor.properties;

   Con::printf("Math Init:");
   Con::printf("   Installing Standard C extensions");
   mInstallLibrary_C();

   if (properties & CPU_PROP_MMX)
   {
      Con::printf("   Installing MMX extensions");
      if (properties & CPU_PROP_3DNOW)
      {
         Con::printf("   Installing 3DNow extensions");
         mInstall_AMD_Math();
      }
   }

   if (properties & CPU_PROP_SSE)
   {
      Con::printf("   Installing SSE extensions");
      mInstall_Library_SSE();
   }

   Con::printf(" ");
}
