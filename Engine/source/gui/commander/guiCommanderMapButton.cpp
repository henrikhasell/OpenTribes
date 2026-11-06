// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/guiCommanderMapButton.h"

#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(GuiCommanderMapButton);
IMPLEMENT_CONOBJECT(GuiCommanderMapCheckbox);

ConsoleDocClass( GuiCommanderMapButton,
   "@brief A button of the commander map's bars.\n\n@internal" );
ConsoleDocClass( GuiCommanderMapCheckbox,
   "@brief A toggle of the commander map's bars.\n\n@internal" );

GuiCommanderMapButton::GuiCommanderMapButton()
{
   mBitmapName = StringTable->EmptyString();
   mMouseRegion.set(0, 0, 0, 0);
}

GuiCommanderMapCheckbox::GuiCommanderMapCheckbox()
{
   mButtonType = ButtonTypeCheck;
}

void GuiCommanderMapButton::initPersistFields()
{
   addField("bitmap", TypeFilename, Offset(mBitmapName, GuiCommanderMapButton),
            "Art cut four wide: off, on, and each under the mouse.");
   addField("mouseRegion", TypeRectI, Offset(mMouseRegion, GuiCommanderMapButton),
            "The part of the control that answers the mouse.");
   Parent::initPersistFields();
}

bool GuiCommanderMapButton::onWake()
{
   if (!Parent::onWake())
      return false;

   if (mBitmapName && mBitmapName[0])
      mTexture.set(mBitmapName, &GFXDefaultGUIProfile, avar("%s() - mTexture", __FUNCTION__));

   if (!mTexture)
   {
      Con::errorf("%s: failed to load bitmap", getClassName());
      return true;
   }

   const S32 width = mTexture->getBitmapWidth() / NumCells;
   for (U32 i = 0; i < NumCells; i++)
      mCells[i].set(i * width, 0, width, mTexture->getBitmapHeight());

   return true;
}

void GuiCommanderMapButton::onSleep()
{
   mTexture = NULL;
   Parent::onSleep();
}

bool GuiCommanderMapButton::pointInControl(const Point2I &parentCoordPoint)
{
   if (!mMouseRegion.isValidRect())
      return Parent::pointInControl(parentCoordPoint);

   const Point2I local = parentCoordPoint - getPosition();
   return mMouseRegion.pointInRect(local);
}

U32 GuiCommanderMapButton::currentCell() const
{
   const bool on = mButtonType == ButtonTypeCheck ? mStateOn : mDepressed;
   const bool lit = mMouseOver && mActive;
   return on ? (lit ? OnLit : On) : (lit ? OffLit : Off);
}

void GuiCommanderMapButton::onRender(Point2I offset, const RectI &updateRect)
{
   if (mTexture)
   {
      GFXDrawUtil *draw = GFX->getDrawUtil();
      draw->clearBitmapModulation();
      draw->drawBitmapSR(mTexture, offset, mCells[currentCell()]);
   }

   renderChildControls(offset, updateRect);
}
