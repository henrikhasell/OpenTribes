#ifndef _SDL_WINDOW_H_
#define _SDL_WINDOW_H_

#include "windowManager/platformWindow.h"
#include "windowManager/platformCursorController.h"
#include "gfx/gfxStructs.h"
#include "gfx/gfxTarget.h"

struct SDL_Window;
class PlatformWindowManagerSDL;

class PlatformCursorControllerSDL : public PlatformCursorController
{
public:
   PlatformCursorControllerSDL(PlatformWindow* owner) : PlatformCursorController(owner) {}

   void setCursorPosition(S32 x, S32 y);
   void getCursorPosition(Point2I& point);
   void setCursorVisible(bool visible);
   bool isCursorVisible();
   void setCursorShape(U32 cursorID);
   void setCursorShape(const UTF8* filename, bool reload);

   U32 getDoubleClickTime();
   S32 getDoubleClickWidth();
   S32 getDoubleClickHeight();
};

class PlatformWindowSDL : public PlatformWindow
{
   friend class PlatformWindowManagerSDL;

public:
   PlatformWindowSDL();
   virtual ~PlatformWindowSDL();

   GFXDevice*        getGFXDevice() { return mDevice; }
   GFXWindowTarget*  getGFXTarget() { return mTarget; }

   void              setVideoMode(const GFXVideoMode& mode);
   const GFXVideoMode& getVideoMode() { return mVideoMode; }
   bool              clearFullscreen();
   bool              isFullscreen();

   void              setMouseLocked(bool enable);
   bool              isMouseLocked() const { return mMouseLocked; }
   bool              shouldLockMouse() const { return mShouldLockMouse; }

   WindowId          getWindowId() { return mWindowId; }
   PlatformWindow*   getNextWindow() const { return mNextWindow; }

   bool              setSize(const Point2I& newSize);
   void              setClientExtent(const Point2I newExtent);
   const Point2I     getClientExtent();
   void              setBounds(const RectI& newBounds);
   const RectI       getBounds() const;
   void              setPosition(const Point2I newPosition);
   const Point2I     getPosition();
   void              centerWindow();

   Point2I           clientToScreen(const Point2I& point);
   Point2I           screenToClient(const Point2I& point);

   bool              setCaption(const char* cap);
   const char*       getCaption() { return mCaption; }

   bool              isOpen()      { return mSDLWindow != NULL; }
   bool              isVisible();
   bool              isFocused();
   bool              isMinimized();
   bool              isMaximized();

   void              minimize();
   void              maximize();
   void              hide();
   void              show();
   void              close();
   void              restore();
   void              setFocus();
   void              setKeyboardTranslation(const bool enabled);

   void*             getPlatformDrawable() const { return mSDLWindow; }
   SDL_Window*       getSDLWindow() const { return mSDLWindow; }

   PlatformWindowManagerSDL* mOwner;

private:
   SDL_Window*        mSDLWindow;
   GFXDevice*         mDevice;
   GFXWindowTargetRef mTarget;
   GFXVideoMode       mVideoMode;
   PlatformWindow*    mNextWindow;
   WindowId           mWindowId;
   const char*        mCaption;
   bool               mMouseLocked;
   bool               mShouldLockMouse;
};

#endif // _SDL_WINDOW_H_
