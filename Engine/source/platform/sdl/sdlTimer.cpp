#include "platform/platform.h"
#include "platform/platformTimer.h"
#include "core/stringTable.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

class SDLTimer : public PlatformTimer
{
public:
   SDLTimer() : mLast(SDL_GetTicks()) {}

   const S32 getElapsedMs()
   {
      return S32( SDL_GetTicks() - mLast );
   }

   void reset() { mLast = SDL_GetTicks(); }

private:
   U64 mLast;
};

PlatformTimer* PlatformTimer::create()
{
   return new SDLTimer();
}

StringTableEntry osGetTemporaryDirectory()
{
   const char* tmp = getenv("TMPDIR");
   return StringTable->insert(tmp && *tmp ? tmp : "/tmp");
}
