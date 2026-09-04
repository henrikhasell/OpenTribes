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
      const U64 now = SDL_GetTicks();
      const S32 elapsed = S32(now - mLast);
      mLast = now;
      return elapsed;
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
