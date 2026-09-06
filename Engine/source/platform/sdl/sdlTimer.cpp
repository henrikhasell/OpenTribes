#include "platform/platform.h"
#include "platform/platformTimer.h"
#include "core/stringTable.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

class SDLTimer : public PlatformTimer
{
public:
   SDLTimer() : mLast(SDL_GetTicks()) {}

   // Elapsed since the last reset(), not since the last call. TimeManager reads
   // this twice per frame -- once to decide how long to sleep, once for the
   // frame delta -- and resets explicitly afterwards. Consuming the time on
   // read made the second read return only the sleep, so the frame delta
   // collapsed to near zero and the whole game ran in slow motion.
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
