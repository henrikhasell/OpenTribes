// SDL3 half of the GL device, replacing gfxGLDevice.win.cpp's WGL code.

#include "platform/platform.h"
#include "platform/sdl/sdlWindowMgr.h"
#include "gfx/gl/gfxGLDevice.h"
#include "gfx/gl/gfxGLWindowTarget.h"
#include "gfx/gl/gfxGLEnumTranslate.h"
#include "gfx/gfxCardProfile.h"
#include "gfx/gfxStringEnumTranslate.h"
#include "windowManager/platformWindowMgr.h"
#include "console/console.h"

#include <SDL3/SDL.h>

extern void loadGLCore();
extern void loadGLExtensions(void* context);

void GFXGLDevice::enumerateAdapters(Vector<GFXAdapter*>& adapterList)
{
   GFXAdapter* toAdd = new GFXAdapter();
   toAdd->mIndex = 0;
   toAdd->mType  = OpenGL;
   toAdd->mCreateDeviceInstanceDelegate = mCreateDeviceInstance;

   dStrcpy(toAdd->mName, "OpenGL");

   int count = 0;
   SDL_DisplayID* displays = SDL_GetDisplays(&count);
   if (displays)
   {
      int modeCount = 0;
      SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displays[0], &modeCount);
      if (modes)
      {
         for (int i = 0; i < modeCount; ++i)
         {
            GFXVideoMode mode;
            mode.resolution.set(modes[i]->w, modes[i]->h);
            mode.bitDepth    = SDL_BITSPERPIXEL(modes[i]->format);
            mode.refreshRate = U32(modes[i]->refresh_rate);
            mode.fullScreen  = true;
            toAdd->mAvailableModes.push_back(mode);
         }
         SDL_free(modes);
      }
      SDL_free(displays);
   }

   adapterList.push_back(toAdd);
}

void GFXGLDevice::enumerateVideoModes()
{
   mVideoModes.clear();

   int count = 0;
   SDL_DisplayID* displays = SDL_GetDisplays(&count);
   if (!displays)
      return;

   int modeCount = 0;
   SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displays[0], &modeCount);
   if (modes)
   {
      for (int i = 0; i < modeCount; ++i)
      {
         GFXVideoMode mode;
         mode.resolution.set(modes[i]->w, modes[i]->h);
         mode.bitDepth    = SDL_BITSPERPIXEL(modes[i]->format);
         mode.refreshRate = U32(modes[i]->refresh_rate);
         mode.fullScreen  = true;
         mVideoModes.push_back(mode);
      }
      SDL_free(modes);
   }

   SDL_free(displays);
}

void GFXGLDevice::init(const GFXVideoMode& mode, PlatformWindow* window)
{
   AssertFatal(window, "GFXGLDevice::init - no window specified");

   PlatformWindowSDL* sdlWindow = dynamic_cast<PlatformWindowSDL*>(window);
   AssertFatal(sdlWindow, "GFXGLDevice::init - window is not a PlatformWindowSDL");

   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   SDL_GLContext context = SDL_GL_CreateContext(sdlWindow->getSDLWindow());
   if (!context)
   {
      Con::errorf("SDL_GL_CreateContext failed: %s", SDL_GetError());
      return;
   }

   mContext = context;

   if (!SDL_GL_MakeCurrent(sdlWindow->getSDLWindow(), context))
      Con::errorf("SDL_GL_MakeCurrent failed: %s", SDL_GetError());

   loadGLCore();
   loadGLExtensions(context);

   SDL_GL_SetSwapInterval(0);

   initGLState();
   mProjectionMatrix.identity();

   mInitialized = true;
   deviceInited();
}

bool GFXGLDevice::beginSceneInternal()
{
   // HH: Modified for Tribes 2 compatibility.
   mCanCurrentlyRender = true;
   glGetError();
   return true;
}

U32 GFXGLDevice::getTotalVideoMemory()
{
   return 0;
}

GFXWindowTarget* GFXGLDevice::allocWindowTarget(PlatformWindow* window)
{
   if (!mContext)
      init(window->getVideoMode(), window);

   GFXGLWindowTarget* target = new GFXGLWindowTarget(window, this);
   target->registerResourceWithDevice(this);
   target->mContext = mContext;

   return target;
}

GFXFence* GFXGLDevice::_createPlatformSpecificFence()
{
   return NULL;
}

//-----------------------------------------------------------------------------

void GFXGLWindowTarget::makeActive()
{
   PlatformWindowSDL* window = dynamic_cast<PlatformWindowSDL*>(getWindow());
   if (window && mContext)
      SDL_GL_MakeCurrent(window->getSDLWindow(), static_cast<SDL_GLContext>(mContext));
}

bool GFXGLWindowTarget::present()
{
   PlatformWindowSDL* window = dynamic_cast<PlatformWindowSDL*>(getWindow());
   if (!window)
      return false;

   SDL_GL_SwapWindow(window->getSDLWindow());
   return true;
}

void GFXGLWindowTarget::_teardownCurrentMode()
{
}

void GFXGLWindowTarget::_setupNewMode()
{
}
