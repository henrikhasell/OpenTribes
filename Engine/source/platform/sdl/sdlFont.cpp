// Phase 5 replaces this with an SDL3_ttf implementation of PlatformFont.

#include "platform/platform.h"
#include "platform/platformFont.h"

PlatformFont* createPlatformFont(const char* name, U32 size, U32 charset)
{
   TORQUE_UNUSED(name);
   TORQUE_UNUSED(size);
   TORQUE_UNUSED(charset);
   return NULL;
}
