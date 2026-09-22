// HH: imported from TGE
// (drawing goes through GFXDrawUtil rather than dgl, the extent is set through
// setExtent rather than mBounds, and the fields carry doc strings)

#include "platform/platform.h"
#include "gui/shell/shellButtonCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(ShellBitmapButton);
IMPLEMENT_CONOBJECT(ShellToggleButton);

ConsoleDocClass( ShellBitmapButton,
   "@brief A button drawn in Tribes 2's shell skin.\n\n"

   "Behaves as any button does; the art is one image holding every state, "
   "stretched between two fixed caps.\n\n"

   "@ingroup GuiButtons"
);

ConsoleDocClass( ShellToggleButton,
   "@brief A ShellBitmapButton that stays down.\n\n"
   "@ingroup GuiButtons"
);

//-----------------------------------------------------------------------------
ShellBitmapButton::ShellBitmapButton()
{
   mSimpleStyle = false;
   mDirection = 0;
   setExtent(128, 38);
}

void ShellBitmapButton::initPersistFields()
{
   Parent::initPersistFields();

   addField("simpleStyle", TypeBool, Offset(mSimpleStyle, ShellBitmapButton),
      "Whether the button is drawn without its frame.");
   addField("direction",   TypeS32,  Offset(mDirection, ShellBitmapButton),
      "Which way an arrow button points.");
}

bool ShellBitmapButton::onWake()
{
   if(!Parent::onWake())
      return false;

   mArt.update(mProfile);

   // fixedExtent means the art decides how tall the control is; the layout
   // only gets a say in the width.
   if(mProfile->mFixedExtent && mArt.isValid())
      setExtent(getExtent().x, mArt.getStateHeight());

   return true;
}

S32 ShellBitmapButton::artState()
{
   if(!mActive)
      return 0;
   if(mDepressed)
      return 2;
   if(mMouseOver)
      return 1;
   return 0;
}

void ShellBitmapButton::onRender(Point2I offset, const RectI &updateRect)
{
   mArt.update(mProfile);

   const RectI bounds(offset, getExtent());

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if(mArt.isValid())
   {
      drawer->setBitmapModulation(ColorI(255, 255, 255, 255));
      mArt.draw(artState(), bounds);
      drawer->clearBitmapModulation();
   }

   if(mProfile->mFont && mButtonText && mButtonText[0])
   {
      ColorI color = mProfile->mFontColor;
      if(!mActive)
         color = mProfile->mFontColorNA;
      else if(mDepressed || mStateOn)
         color = mProfile->mFontColorSEL;
      else if(mMouseOver)
         color = mProfile->mFontColorHL;

      drawer->setBitmapModulation(color);
      renderJustifiedText(offset + mProfile->mTextOffset,
                          getExtent() - mProfile->mTextOffset, mButtonText);
      drawer->clearBitmapModulation();
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
ShellToggleButton::ShellToggleButton()
{
   mButtonType = ButtonTypeCheck;
}

S32 ShellToggleButton::artState()
{
   if(!mActive)
      return 0;

   // Down is down whether the mouse is holding it there or the last click
   // left it there.
   if(mDepressed || mStateOn)
      return 2;
   if(mMouseOver)
      return 1;
   return 0;
}
