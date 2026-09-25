#include "platform/platform.h"
#include "gui/shell/shellTabButton.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellTabButton );

ConsoleDocClass( ShellTabButton,
   "@brief One tab of a Tribes 2 shell tab strip.\n\n"

   "The profile's bitmap is a grid three wide and three tall: a left cap, a "
   "middle and a right cap, each resting, selected and lit.  Which of the "
   "three is open is script's to say -- setValue latches a tab and nothing in "
   "the control does.\n\n"

   "@ingroup GuiShell"
);

ShellTabButton::ShellTabButton()
{
   mButtonType = ButtonTypePush;
   mSliced = false;
}

bool ShellTabButton::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mTexture = mProfile->mTextureObject;
   mSliced = mTexture &&
      shellSliceGrid( mTexture->getBitmap(), mCell, StateCount, PieceCount ) == PieceCount;

   if ( mSliced )
   {
      setMinExtent( Point2I(
         cell( LeftCap, Resting ).extent.x + cell( RightCap, Resting ).extent.x,
         cell( LeftCap, Resting ).extent.y ) );

      resize( getPosition(), getExtent() );
   }

   return true;
}

void ShellTabButton::onSleep()
{
   Parent::onSleep();

   mTexture = NULL;
   mSliced = false;
   resetState();
}

S32 ShellTabButton::artState()
{
   if ( mStateOn )
      return Selected;

   if ( mActive && ( isFirstResponder() || mMouseOver ) )
      return Highlight;

   return Resting;
}

void ShellTabButton::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   S32 left = 0;
   S32 span = getWidth();
   S32 barHeight = 0;

   if ( mSliced )
   {
      const S32 state = artState();

      left = cell( LeftCap, Resting ).extent.x;
      span = getWidth() - left - cell( RightCap, Resting ).extent.x;
      barHeight = cell( LeftCap, Resting ).extent.y;

      if ( mActive )
         draw->clearBitmapModulation();
      else
         draw->setBitmapModulation( ColorI( 128, 128, 128 ) );

      Point2I at = offset;

      draw->drawBitmapSR( mTexture, at, cell( LeftCap, state ) );
      at.x += cell( LeftCap, state ).extent.x;

      if ( span > 0 )
         draw->drawBitmapStretchSR( mTexture,
            RectI( at.x, at.y, span, barHeight ), cell( Middle, state ) );

      at.x += span;
      draw->drawBitmapSR( mTexture, at, cell( RightCap, state ) );
   }

   const char *text = getText();

   if ( text && text[ 0 ] && mProfile->mFont )
   {
      if ( !barHeight )
         barHeight = getHeight();

      RectI clip( offset.x + left, offset.y, span, barHeight );

      if ( clip.intersect( updateRect ) )
      {
         GFX->setClipRect( clip );

         GFont *font = mProfile->mFont;

         Point2I at( offset.x + left,
                     offset.y + ( barHeight - font->getHeight() ) / 2 - 1 );

         const S32 width = font->getStrWidth( text );

         if ( span > width )
         {
            if ( mProfile->mAlignment == GuiControlProfile::RightJustify )
               at.x += span - width;
            else if ( mProfile->mAlignment == GuiControlProfile::CenterJustify )
               at.x += ( span - width ) / 2;
         }

         if ( mStateOn )
            at += Point2I( 1, 1 );

         draw->setBitmapModulation( !mActive ? mProfile->mFontColorNA
                                    : mStateOn ? mProfile->mFontColorSEL
                                    : mMouseOver ? mProfile->mFontColorHL
                                                 : mProfile->mFontColor );

         draw->drawText( font, at, text, mProfile->mFontColors );
      }
   }
}
