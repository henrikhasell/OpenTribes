#include "platform/platform.h"
#include "gui/shell/shellTextEditCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellTextEditCtrl );

ConsoleDocClass( ShellTextEditCtrl,
   "@brief A Tribes 2 shell entry field.\n\n"

   "The profile's bitmap is a grid two wide and three tall: a left cap, a "
   "middle and a right cap, each in a resting and a lit state.  The caps are "
   "laid at their own size and the middle stretched between them, and the text "
   "is drawn into what the caps leave.\n\n"

   "@ingroup GuiShell"
);

ShellTextEditCtrl::ShellTextEditCtrl()
{
   mSliced = false;
   mGlowOffset.set( 9, 9 );
}

void ShellTextEditCtrl::initPersistFields()
{
   addField( "glowOffset", TypePoint2I, Offset( mGlowOffset, ShellTextEditCtrl ),
      "How far inside the field the text sits.  Only the y is read." );

   Parent::initPersistFields();
}

bool ShellTextEditCtrl::onWake()
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

void ShellTextEditCtrl::onSleep()
{
   Parent::onSleep();

   mTexture = NULL;
   mSliced = false;
}

void ShellTextEditCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   const S32 state = ( mActive && isFirstResponder() ) ? Focused : Resting;

   S32 inset = 0;
   S32 span = getWidth();

   if ( mSliced )
   {
      if ( mActive )
         draw->clearBitmapModulation();
      else
         draw->setBitmapModulation( ColorI( 128, 128, 128 ) );

      inset = cell( LeftCap, Resting ).extent.x;
      span = getWidth() - inset - cell( RightCap, Resting ).extent.x;

      Point2I at = offset;

      draw->drawBitmapSR( mTexture, at, cell( LeftCap, state ) );
      at.x += cell( LeftCap, state ).extent.x;

      if ( span > 0 )
         draw->drawBitmapStretchSR( mTexture,
            RectI( at.x, at.y, span, cell( Middle, state ).extent.y ),
            cell( Middle, state ) );

      at.x += span;
      draw->drawBitmapSR( mTexture, at, cell( RightCap, state ) );
   }

   RectI text( offset.x + inset, offset.y + mGlowOffset.y,
               span, getHeight() - mGlowOffset.y * 2 );

   RectI clip = text;

   if ( clip.intersect( updateRect ) )
   {
      GFX->setClipRect( clip );
      drawText( text, isFirstResponder() );
   }

   if ( mProfile->mBorder )
      renderBorder( RectI( offset, getExtent() ), mProfile );
}
