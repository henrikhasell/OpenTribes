#include "platform/sdl/sdlWindowMgr.h"
#include "gfx/gfxDevice.h"
#include "core/util/journal/process.h"
#include "console/console.h"

#include <SDL3/SDL.h>

static U32 sdlModifiers()
{
   const SDL_Keymod mod = SDL_GetModState();
   U32 out = 0;

   if( mod & SDL_KMOD_LSHIFT ) out |= SI_LSHIFT;
   if( mod & SDL_KMOD_RSHIFT ) out |= SI_RSHIFT;
   if( mod & SDL_KMOD_LCTRL )  out |= SI_LCTRL;
   if( mod & SDL_KMOD_RCTRL )  out |= SI_RCTRL;
   if( mod & SDL_KMOD_LALT )   out |= SI_LALT;
   if( mod & SDL_KMOD_RALT )   out |= SI_RALT;

   return out;
}

/// SDL scancode to the engine's key codes. The letter and digit runs are
/// contiguous in both, so those map arithmetically.
static U16 sdlToTorqueKey( SDL_Scancode code )
{
   static_assert( KEY_Z - KEY_A == 25, "KEY_A..KEY_Z must stay contiguous" );
   static_assert( KEY_9 - KEY_1 ==  8, "KEY_1..KEY_9 must stay contiguous" );
   static_assert( KEY_F12 - KEY_F1 == 11, "KEY_F1..KEY_F12 must stay contiguous" );

   if( code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z )
      return U16( KEY_A + ( code - SDL_SCANCODE_A ) );

   if( code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9 )
      return U16( KEY_1 + ( code - SDL_SCANCODE_1 ) );

   if( code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F12 )
      return U16( KEY_F1 + ( code - SDL_SCANCODE_F1 ) );

   switch( code )
   {
      case SDL_SCANCODE_0:            return KEY_0;
      case SDL_SCANCODE_BACKSPACE:    return KEY_BACKSPACE;
      case SDL_SCANCODE_TAB:          return KEY_TAB;
      case SDL_SCANCODE_RETURN:       return KEY_RETURN;
      case SDL_SCANCODE_ESCAPE:       return KEY_ESCAPE;
      case SDL_SCANCODE_SPACE:        return KEY_SPACE;
      case SDL_SCANCODE_PAGEDOWN:     return KEY_PAGE_DOWN;
      case SDL_SCANCODE_PAGEUP:       return KEY_PAGE_UP;
      case SDL_SCANCODE_END:          return KEY_END;
      case SDL_SCANCODE_HOME:         return KEY_HOME;
      case SDL_SCANCODE_LEFT:         return KEY_LEFT;
      case SDL_SCANCODE_UP:           return KEY_UP;
      case SDL_SCANCODE_RIGHT:        return KEY_RIGHT;
      case SDL_SCANCODE_DOWN:         return KEY_DOWN;
      case SDL_SCANCODE_INSERT:       return KEY_INSERT;
      case SDL_SCANCODE_DELETE:       return KEY_DELETE;
      case SDL_SCANCODE_LSHIFT:
      case SDL_SCANCODE_RSHIFT:       return KEY_SHIFT;
      case SDL_SCANCODE_LCTRL:
      case SDL_SCANCODE_RCTRL:        return KEY_CONTROL;
      case SDL_SCANCODE_LALT:
      case SDL_SCANCODE_RALT:         return KEY_ALT;
      case SDL_SCANCODE_CAPSLOCK:     return KEY_CAPSLOCK;
      case SDL_SCANCODE_PAUSE:        return KEY_PAUSE;
      case SDL_SCANCODE_MINUS:        return KEY_MINUS;
      case SDL_SCANCODE_EQUALS:       return KEY_EQUALS;
      case SDL_SCANCODE_LEFTBRACKET:  return KEY_LBRACKET;
      case SDL_SCANCODE_RIGHTBRACKET: return KEY_RBRACKET;
      case SDL_SCANCODE_BACKSLASH:    return KEY_BACKSLASH;
      case SDL_SCANCODE_SEMICOLON:    return KEY_SEMICOLON;
      case SDL_SCANCODE_APOSTROPHE:   return KEY_APOSTROPHE;
      case SDL_SCANCODE_COMMA:        return KEY_COMMA;
      case SDL_SCANCODE_PERIOD:       return KEY_PERIOD;
      case SDL_SCANCODE_SLASH:        return KEY_SLASH;
      case SDL_SCANCODE_GRAVE:        return KEY_TILDE;
      default:                        return 0;
   }
}

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

         case SDL_EVENT_WINDOW_FOCUS_GAINED:
         case SDL_EVENT_WINDOW_FOCUS_LOST:
         {
            PlatformWindow* window = getWindowById( event.window.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window )
               break;

            const ApplicationMessage action =
               ( event.type == SDL_EVENT_WINDOW_FOCUS_GAINED ) ? GainFocus : LoseFocus;

            window->appEvent.trigger( window->getWindowId(), action );
            break;
         }

         case SDL_EVENT_WINDOW_RESIZED:
         {
            PlatformWindow* window = getFirstWindow();
            if (window)
               window->resizeEvent.trigger(window->getWindowId(),
                                           event.window.data1, event.window.data2);
            break;
         }

         case SDL_EVENT_MOUSE_MOTION:
         {
            PlatformWindow* window = getWindowById( event.motion.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window )
               break;

            // Relative mode reports deltas, which is what the engine wants
            // while the mouse is locked for camera control.
            const bool relative = window->isMouseLocked();
            const S32 x = relative ? S32( event.motion.xrel ) : S32( event.motion.x );
            const S32 y = relative ? S32( event.motion.yrel ) : S32( event.motion.y );

            window->mouseEvent.trigger( window->getWindowId(), sdlModifiers(), x, y, relative );
            break;
         }

         case SDL_EVENT_MOUSE_BUTTON_DOWN:
         case SDL_EVENT_MOUSE_BUTTON_UP:
         {
            PlatformWindow* window = getWindowById( event.button.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window )
               break;

            const U32 action = ( event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ) ? SI_MAKE : SI_BREAK;

            // WindowInputGenerator::handleMouseButton adds KEY_BUTTON0 itself,
            // so this passes the zero-based index. Sending the offset code here
            // made the engine compute KEY_BUTTON0 + 256, which is no button.
            static const U16 sButtonOrder[] = { 0, 2, 1 };

            const U16 sdlButton = U16( event.button.button - 1 );
            const U16 button = sdlButton < 3 ? sButtonOrder[ sdlButton ] : sdlButton;

            window->buttonEvent.trigger( window->getWindowId(), sdlModifiers(), action, button );
            break;
         }

         case SDL_EVENT_MOUSE_WHEEL:
         {
            PlatformWindow* window = getWindowById( event.wheel.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window )
               break;

            // The engine expects wheel deltas in notches of WheelDelta.
            const S32 dx = S32( event.wheel.x ) * 120;
            const S32 dy = S32( event.wheel.y ) * 120;

            window->wheelEvent.trigger( window->getWindowId(), sdlModifiers(), dx, dy );
            break;
         }

         case SDL_EVENT_KEY_DOWN:
         case SDL_EVENT_KEY_UP:
         {
            PlatformWindow* window = getWindowById( event.key.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window )
               break;

            const U16 key = sdlToTorqueKey( event.key.scancode );
            if( !key )
               break;

            U32 action = ( event.type == SDL_EVENT_KEY_DOWN ) ? SI_MAKE : SI_BREAK;
            if( event.type == SDL_EVENT_KEY_DOWN && event.key.repeat )
               action = SI_REPEAT;

            window->keyEvent.trigger( window->getWindowId(), sdlModifiers(), action, key );
            break;
         }

         case SDL_EVENT_TEXT_INPUT:
         {
            PlatformWindow* window = getWindowById( event.text.windowID );
            if( !window )
               window = getFirstWindow();
            if( !window || !window->getKeyboardTranslation() )
               break;

            for( const char* c = event.text.text; c && *c; ++ c )
               window->charEvent.trigger( window->getWindowId(), sdlModifiers(), U16( U8( *c ) ) );
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
