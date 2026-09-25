#include "platform/platform.h"
#include "gui/shell/shellDlgFrame.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellDlgFrame );

ConsoleDocClass( ShellDlgFrame,
   "@brief The frame a Tribes 2 shell dialog is drawn in.\n\n"

   "One column of nine cells off the profile's bitmap: a titled row, a body "
   "and a foot, each as a left piece, a middle and a right.  The control's "
   "text is the title, centred on the bar.\n\n"

   "@ingroup GuiShell"
);

ShellDlgFrame::ShellDlgFrame()
{
   mSliced = false;
}

bool ShellDlgFrame::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mTexture = mProfile->mTextureObject;
   mSliced = mTexture &&
      shellSliceGrid( mTexture->getBitmap(), mCell, 1, CellCount ) == CellCount;

   return true;
}

void ShellDlgFrame::onSleep()
{
   Parent::onSleep();

   mTexture = NULL;
   mSliced = false;
}

void ShellDlgFrame::onRender( Point2I offset, const RectI &updateRect )
{
   if ( mSliced )
   {
      GFXDrawUtil *draw = GFX->getDrawUtil();
      draw->clearBitmapModulation();

      const S32 span = getWidth() - mCell[ TitleLeft ].extent.x
                                  - mCell[ TitleRight ].extent.x;

      const S32 body = getHeight() - mCell[ TitleMiddle ].extent.y
                                   - mCell[ FootMiddle ].extent.y;

      Point2I at = offset;

      draw->drawBitmapSR( mTexture, at, mCell[ TitleLeft ] );
      at.x += mCell[ TitleLeft ].extent.x;

      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, span, mCell[ TitleMiddle ].extent.y ), mCell[ TitleMiddle ] );

      at.x += span;
      draw->drawBitmapSR( mTexture, at, mCell[ TitleRight ] );

      const char *text = getText();

      if ( text && text[ 0 ] && mProfile->mFont )
      {
         GFont *font = mProfile->mFont;

         Point2I title(
            offset.x + ( getWidth() - S32( font->getStrWidth( text ) ) ) / 2,
            offset.y + ( mCell[ TitleMiddle ].extent.y - S32( font->getHeight() ) ) / 2 + 4 );

         draw->setBitmapModulation( mProfile->mFontColor );
         draw->drawText( font, title, text, mProfile->mFontColors );
         draw->clearBitmapModulation();
      }

      at.set( offset.x, offset.y + mCell[ TitleLeft ].extent.y );

      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, mCell[ BodyLeft ].extent.x, body ), mCell[ BodyLeft ] );

      at.x += mCell[ BodyLeft ].extent.x;

      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, span, body ), mCell[ BodyMiddle ] );

      at.x += span;

      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, mCell[ BodyRight ].extent.x, body ), mCell[ BodyRight ] );

      at.set( offset.x, at.y + body );

      draw->drawBitmapSR( mTexture, at, mCell[ FootLeft ] );
      at.x += mCell[ FootLeft ].extent.x;

      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, span, mCell[ FootMiddle ].extent.y ), mCell[ FootMiddle ] );

      at.x += span;
      draw->drawBitmapSR( mTexture, at, mCell[ FootRight ] );
   }

   renderChildControls( offset, updateRect );
}
