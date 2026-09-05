#include "platform/sdl/sdlWindowMgr.h"
#include "gfx/gfxDevice.h"
#include "core/util/journal/process.h"
#include "console/console.h"

#include <SDL3/SDL.h>

PlatformWindowManagerSDL::PlatformWindowManagerSDL()
   : mWindowListHead(NULL), mParentWindow(NULL), mNextWindowId(1)
{
   Process::notify(this, &PlatformWindowManagerSDL::_handleEvents, PROCESS_INPUT_ORDER);
}

PlatformWindowManagerSDL::~PlatformWindowManagerSDL()
{
   Process::remove(this, &PlatformWindowManagerSDL::_handleEvents);

   // The windows are not ours to delete; GuiCanvas owns the one it created and
   // each unlinks itself here as it goes.
   mWindowListHead = NULL;
}

void PlatformWindowManagerSDL::_unlinkWindow(PlatformWindowSDL* window)
{
   PlatformWindowSDL** walk = &mWindowListHead;
   while (*walk)
   {
      if (*walk == window)
      {
         *walk = static_cast<PlatformWindowSDL*>(window->getNextWindow());
         return;
      }
      walk = reinterpret_cast<PlatformWindowSDL**>(&(*walk)->mNextWindow);
   }
}

RectI PlatformWindowManagerSDL::getPrimaryDesktopArea()
{
   SDL_Rect bounds;
   const SDL_DisplayID display = SDL_GetPrimaryDisplay();

   if (!SDL_GetDisplayUsableBounds(display, &bounds))
      return RectI(0, 0, 1280, 720);

   return RectI(bounds.x, bounds.y, bounds.w, bounds.h);
}

S32 PlatformWindowManagerSDL::getDesktopBitDepth()
{
   const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
   if (!mode)
      return 32;

   return S32(SDL_BITSPERPIXEL(mode->format));
}

Point2I PlatformWindowManagerSDL::getDesktopResolution()
{
   const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
   if (!mode)
      return Point2I(1280, 720);

   return Point2I(mode->w, mode->h);
}

void PlatformWindowManagerSDL::getMonitorRegions(Vector<RectI>& regions)
{
   int count = 0;
   SDL_DisplayID* displays = SDL_GetDisplays(&count);
   if (!displays)
      return;

   for (int i = 0; i < count; ++i)
   {
      SDL_Rect bounds;
      if (SDL_GetDisplayBounds(displays[i], &bounds))
         regions.push_back(RectI(bounds.x, bounds.y, bounds.w, bounds.h));
   }

   SDL_free(displays);
}

PlatformWindow* PlatformWindowManagerSDL::createWindow(GFXDevice* device, const GFXVideoMode& mode)
{
   PlatformWindowSDL* window = new PlatformWindowSDL();

   SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
   if (mode.fullScreen)
      flags |= SDL_WINDOW_FULLSCREEN;

   window->mSDLWindow = SDL_CreateWindow("Torque", mode.resolution.x, mode.resolution.y, flags);

   if (!window->mSDLWindow)
   {
      Con::errorf("SDL_CreateWindow failed: %s", SDL_GetError());
      delete window;
      return NULL;
   }

   window->mWindowId  = mNextWindowId++;
   window->mVideoMode = mode;
   window->mDevice    = device;
   window->mTarget    = device ? device->allocWindowTarget(window) : NULL;

   window->mOwner      = this;
   window->mNextWindow = mWindowListHead;
   mWindowListHead = window;

   return window;
}

void PlatformWindowManagerSDL::getWindows(VectorPtr<PlatformWindow*>& windows)
{
   for (PlatformWindowSDL* walk = mWindowListHead; walk;
        walk = static_cast<PlatformWindowSDL*>(walk->getNextWindow()))
      windows.push_back(walk);
}

PlatformWindow* PlatformWindowManagerSDL::getFocusedWindow()
{
   for (PlatformWindowSDL* walk = mWindowListHead; walk;
        walk = static_cast<PlatformWindowSDL*>(walk->getNextWindow()))
      if (walk->isFocused())
         return walk;

   return NULL;
}

PlatformWindow* PlatformWindowManagerSDL::getWindowById(WindowId id)
{
   for (PlatformWindowSDL* walk = mWindowListHead; walk;
        walk = static_cast<PlatformWindowSDL*>(walk->getNextWindow()))
      if (walk->getWindowId() == id)
         return walk;

   return NULL;
}

PlatformWindow* PlatformWindowManagerSDL::getFirstWindow()
{
   return mWindowListHead;
}

void PlatformWindowManagerSDL::_processCmdLineArgs(const S32 argc, const char** argv)
{
   TORQUE_UNUSED(argc);
   TORQUE_UNUSED(argv);
}

void PlatformWindowManagerSDL::_handleEvents()
{
   SDL_Event event;
   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
         case SDL_EVENT_QUIT:
            Process::requestShutdown();
            break;

         case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            Process::requestShutdown();
            break;

         case SDL_EVENT_WINDOW_RESIZED:
         {
            PlatformWindow* window = getFirstWindow();
            if (window)
               window->resizeEvent.trigger(window->getWindowId(),
                                           event.window.data1, event.window.data2);
            break;
         }

         case SDL_EVENT_WINDOW_FOCUS_GAINED:
         {
            PlatformWindow* window = getFirstWindow();
            if (window)
               window->appEvent.trigger(window->getWindowId(), GainFocus);
            break;
         }

         case SDL_EVENT_WINDOW_FOCUS_LOST:
         {
            PlatformWindow* window = getFirstWindow();
            if (window)
               window->appEvent.trigger(window->getWindowId(), LoseFocus);
            break;
         }

         default:
            break;
      }
   }
}

PlatformWindowManager* CreatePlatformWindowManager()
{
   return new PlatformWindowManagerSDL();
}
