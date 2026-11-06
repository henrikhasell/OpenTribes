// HH: Added for Tribes 2 compatibility.
#ifndef _GUICOMMANDERTV_H_
#define _GUICOMMANDERTV_H_

#ifndef _GAMETSCTRL_H_
#include "T3D/gameTSCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

/// The commander map's little screen: the view from whatever the server put
/// this client's camera on, or static while it has put it on nothing.  The
/// scripts keep whether it is on something in its "attached" field.
class GuiCommanderTV : public GameTSCtrl
{
   typedef GameTSCtrl Parent;

   StringTableEntry mFrameBitmapName;
   StringTableEntry mStaticBitmapName;
   GFXTexHandle mFrame;
   GFXTexHandle mStatic;

public:
   GuiCommanderTV();

   static void initPersistFields();

   bool onWake();
   void onSleep();
   void onRender(Point2I offset, const RectI &updateRect);

   DECLARE_CONOBJECT(GuiCommanderTV);
};

#endif
