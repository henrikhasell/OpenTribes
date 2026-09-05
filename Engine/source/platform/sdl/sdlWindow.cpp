#include "platform/sdl/sdlWindowMgr.h"
#include "platform/platform.h"
#include "gfx/gfxDevice.h"
#include "console/console.h"

#include <SDL3/SDL.h>

PlatformWindowSDL::PlatformWindowSDL()
   : mSDLWindow(NULL),
     mDevice(NULL),
     mTarget(NULL),
     mNextWindow(NULL),
     mWindowId(0),
     mCaption(""),
     mMouseLocked(false),
     mShouldLockMouse(false)
{
   mCursorController = new PlatformCursorControllerSDL(this);
}

PlatformWindowSDL::~PlatformWindowSDL()
{
   if (mOwner)
      mOwner->_unlinkWindow(this);

   if (mSDLWindow)
      SDL_DestroyWindow(mSDLWindow);
}

void PlatformWindowSDL::setVideoMode(const GFXVideoMode& mode)
{
   mVideoMode = mode;

   if (!mSDLWindow)
      return;

   SDL_SetWindowFullscreen(mSDLWindow, mode.fullScreen);
   if (!mode.fullScreen)
      SDL_SetWindowSize(mSDLWindow, mode.resolution.x, mode.resolution.y);
}

bool PlatformWindowSDL::clearFullscreen()
{
   if (!mSDLWindow)
      return false;

   SDL_SetWindowFullscreen(mSDLWindow, false);
   mVideoMode.fullScreen = false;
   return true;
}

bool PlatformWindowSDL::isFullscreen()
{
   if (!mSDLWindow)
      return false;

   return (SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_FULLSCREEN) != 0;
}

void PlatformWindowSDL::setMouseLocked(bool enable)
{
   if (!mSDLWindow)
      return;

   mShouldLockMouse = enable;
   mMouseLocked = enable;
   SDL_SetWindowRelativeMouseMode(mSDLWindow, enable);
}

bool PlatformWindowSDL::setSize(const Point2I& newSize)
{
   if (!mSDLWindow)
      return false;

   SDL_SetWindowSize(mSDLWindow, newSize.x, newSize.y);
   mVideoMode.resolution = newSize;
   return true;
}

void PlatformWindowSDL::setClientExtent(const Point2I newExtent)
{
   setSize(newExtent);
}

const Point2I PlatformWindowSDL::getClientExtent()
{
   if (!mSDLWindow)
      return mVideoMode.resolution;

   int w = 0, h = 0;
   SDL_GetWindowSize(mSDLWindow, &w, &h);
   return Point2I(w, h);
}

void PlatformWindowSDL::setBounds(const RectI& newBounds)
{
   if (!mSDLWindow)
      return;

   SDL_SetWindowPosition(mSDLWindow, newBounds.point.x, newBounds.point.y);
   SDL_SetWindowSize(mSDLWindow, newBounds.extent.x, newBounds.extent.y);
}

const RectI PlatformWindowSDL::getBounds() const
{
   if (!mSDLWindow)
      return RectI(0, 0, 0, 0);

   int x = 0, y = 0, w = 0, h = 0;
   SDL_GetWindowPosition(mSDLWindow, &x, &y);
   SDL_GetWindowSize(mSDLWindow, &w, &h);
   return RectI(x, y, w, h);
}

void PlatformWindowSDL::setPosition(const Point2I newPosition)
{
   if (mSDLWindow)
      SDL_SetWindowPosition(mSDLWindow, newPosition.x, newPosition.y);
}

const Point2I PlatformWindowSDL::getPosition()
{
   if (!mSDLWindow)
      return Point2I(0, 0);

   int x = 0, y = 0;
   SDL_GetWindowPosition(mSDLWindow, &x, &y);
   return Point2I(x, y);
}

void PlatformWindowSDL::centerWindow()
{
   if (mSDLWindow)
      SDL_SetWindowPosition(mSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

Point2I PlatformWindowSDL::clientToScreen(const Point2I& point)
{
   const Point2I origin = getPosition();
   return Point2I(point.x + origin.x, point.y + origin.y);
}

Point2I PlatformWindowSDL::screenToClient(const Point2I& point)
{
   const Point2I origin = getPosition();
   return Point2I(point.x - origin.x, point.y - origin.y);
}

bool PlatformWindowSDL::setCaption(const char* cap)
{
   if (!mSDLWindow)
      return false;

   mCaption = cap;
   SDL_SetWindowTitle(mSDLWindow, cap);
   return true;
}

bool PlatformWindowSDL::isVisible()
{
   return mSDLWindow && (SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_HIDDEN) == 0;
}

bool PlatformWindowSDL::isFocused()
{
   return mSDLWindow && (SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool PlatformWindowSDL::isMinimized()
{
   return mSDLWindow && (SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_MINIMIZED) != 0;
}

bool PlatformWindowSDL::isMaximized()
{
   return mSDLWindow && (SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_MAXIMIZED) != 0;
}

void PlatformWindowSDL::minimize() { if (mSDLWindow) SDL_MinimizeWindow(mSDLWindow); }
void PlatformWindowSDL::maximize() { if (mSDLWindow) SDL_MaximizeWindow(mSDLWindow); }
void PlatformWindowSDL::hide()     { if (mSDLWindow) SDL_HideWindow(mSDLWindow); }
void PlatformWindowSDL::show()     { if (mSDLWindow) SDL_ShowWindow(mSDLWindow); }
void PlatformWindowSDL::restore()  { if (mSDLWindow) SDL_RestoreWindow(mSDLWindow); }
void PlatformWindowSDL::setFocus() { if (mSDLWindow) SDL_RaiseWindow(mSDLWindow); }

void PlatformWindowSDL::close()
{
   if (mSDLWindow)
   {
      SDL_DestroyWindow(mSDLWindow);
      mSDLWindow = NULL;
   }

   appEvent.trigger(getWindowId(), WindowDestroy);
}

//-----------------------------------------------------------------------------
// Cursor

void PlatformCursorControllerSDL::setCursorPosition(S32 x, S32 y)
{
   SDL_WarpMouseGlobal(F32(x), F32(y));
}

void PlatformCursorControllerSDL::getCursorPosition(Point2I& point)
{
   float x = 0.0f, y = 0.0f;
   SDL_GetGlobalMouseState(&x, &y);
   point.set(S32(x), S32(y));
}

void PlatformCursorControllerSDL::setCursorVisible(bool visible)
{
   if (visible)
      SDL_ShowCursor();
   else
      SDL_HideCursor();
}

bool PlatformCursorControllerSDL::isCursorVisible()
{
   return SDL_CursorVisible();
}

void PlatformCursorControllerSDL::setCursorShape(U32 cursorID)
{
   SDL_SystemCursor shape = SDL_SYSTEM_CURSOR_DEFAULT;
   switch (cursorID)
   {
      case curArrow:      shape = SDL_SYSTEM_CURSOR_DEFAULT;    break;
      case curWait:       shape = SDL_SYSTEM_CURSOR_WAIT;       break;
      case curIBeam:      shape = SDL_SYSTEM_CURSOR_TEXT;       break;
      case curResizeVert: shape = SDL_SYSTEM_CURSOR_NS_RESIZE;  break;
      case curResizeHorz: shape = SDL_SYSTEM_CURSOR_EW_RESIZE;  break;
      case curResizeAll:  shape = SDL_SYSTEM_CURSOR_MOVE;       break;
      case curResizeNESW: shape = SDL_SYSTEM_CURSOR_NESW_RESIZE; break;
      case curResizeNWSE: shape = SDL_SYSTEM_CURSOR_NWSE_RESIZE; break;
      default: break;
   }

   if (SDL_Cursor* cursor = SDL_CreateSystemCursor(shape))
      SDL_SetCursor(cursor);
}

void PlatformCursorControllerSDL::setCursorShape(const UTF8* filename, bool reload)
{
   TORQUE_UNUSED(filename);
   TORQUE_UNUSED(reload);
}

U32 PlatformCursorControllerSDL::getDoubleClickTime()   { return 500; }
S32 PlatformCursorControllerSDL::getDoubleClickWidth()  { return 4; }
S32 PlatformCursorControllerSDL::getDoubleClickHeight() { return 4; }
