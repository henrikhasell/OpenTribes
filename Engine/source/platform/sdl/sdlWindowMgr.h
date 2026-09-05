#ifndef _SDL_WINDOWMGR_H_
#define _SDL_WINDOWMGR_H_

#include "windowManager/platformWindowMgr.h"
#include "platform/sdl/sdlWindow.h"

class PlatformWindowManagerSDL : public PlatformWindowManager
{
public:
   PlatformWindowManagerSDL();
   virtual ~PlatformWindowManagerSDL();

   RectI          getPrimaryDesktopArea();
   S32            getDesktopBitDepth();
   Point2I        getDesktopResolution();
   void           getMonitorRegions(Vector<RectI>& regions);

   PlatformWindow* createWindow(GFXDevice* device, const GFXVideoMode& mode);
   void            getWindows(VectorPtr<PlatformWindow*>& windows);
   PlatformWindow* getFocusedWindow();
   PlatformWindow* getWindowById(WindowId id);
   PlatformWindow* getFirstWindow();

   void  setParentWindow(void* newParent) { mParentWindow = newParent; }
   void* getParentWindow()                { return mParentWindow; }

   void  lowerCurtain() {}
   void  raiseCurtain() {}

   void  _processCmdLineArgs(const S32 argc, const char** argv);

   /// Windows are owned by whoever created them -- GuiCanvas deletes its own --
   /// so they unlink here on destruction to keep this list free of dead entries.
   void  _unlinkWindow(PlatformWindowSDL* window);

protected:
   void  _handleEvents();

   PlatformWindowSDL* mWindowListHead;
   void*              mParentWindow;
   WindowId           mNextWindowId;
   bool               mLastFocusState;
};

#endif // _SDL_WINDOWMGR_H_
