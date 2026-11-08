// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/guiCommanderTV.h"

#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mRandom.h"

IMPLEMENT_CONOBJECT(GuiCommanderTV);

ConsoleDocClass( GuiCommanderTV,
   "@brief The commander map's view through a teammate or an "
   "asset.\n\n@internal" );

GuiCommanderTV::GuiCommanderTV()
{
   mFrameBitmapName = StringTable->insert("commander/gui/cmd_tv_frame");
   mStaticBitmapName = StringTable->insert("commander/gui/cmd_tv_static");
}

void GuiCommanderTV::initPersistFields()
{
   addField("frameBitmap", TypeString, Offset(mFrameBitmapName, GuiCommanderTV),
            "Drawn over the whole screen, view or static.");
   addField("staticBitmap", TypeString, Offset(mStaticBitmapName, GuiCommanderTV),
            "Wider than the screen, and shown a random stretch of at a time.");
   Parent::initPersistFields();
}

bool GuiCommanderTV::onWake()
{
   if (!Parent::onWake())
      return false;

   if (mFrameBitmapName[0])
      mFrame.set(mFrameBitmapName, &GFXDefaultGUIProfile, avar("%s() - frame", __FUNCTION__));
   if (mStaticBitmapName[0])
      mStatic.set(mStaticBitmapName, &GFXDefaultGUIProfile, avar("%s() - static", __FUNCTION__));
   return true;
}

void GuiCommanderTV::onSleep()
{
   mFrame = NULL;
   mStatic = NULL;
   Parent::onSleep();
}

void GuiCommanderTV::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI screen(offset, getExtent());

   if (dAtob(getDataField(StringTable->insert("attached"), NULL)))
   {
      // The world only: the game's own filters belong to the main view.
      GuiTSCtrl::onRender(offset, updateRect);
   }
   else
   {
      draw->drawRectFill(screen, ColorI(0, 0, 0, 255));
      if (mStatic)
      {
         static MRandomLCG random;
         const S32 width = getMin(S32(mStatic->getBitmapWidth()), getMax(getWidth(), 1));
         const S32 spare = mStatic->getBitmapWidth() - width;
         const S32 x = spare > 0 ? random.randI(0, spare) : 0;
         draw->clearBitmapModulation();
         draw->drawBitmapStretchSR(mStatic, screen,
                                   RectI(x, 0, width, mStatic->getBitmapHeight()));
      }
      renderChildControls(offset, updateRect);
   }

   GFX->setClipRect(updateRect);
   if (mFrame)
   {
      draw->clearBitmapModulation();
      draw->drawBitmapStretch(mFrame, screen);
   }
}
