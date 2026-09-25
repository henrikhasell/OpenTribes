#include "platform/platform.h"
#include "gui/shell/shellSliderCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellSliderCtrl );

ConsoleDocClass( ShellSliderCtrl,
   "@brief A Tribes 2 shell slider.\n\n"

   "Shares its art with the shell's scroll control: the track is the "
   "horizontal field and the thumb the two caps of the horizontal bar, butted "
   "together with nothing between them.\n\n"

   "@ingroup GuiShell"
);

ShellSliderCtrl::ShellSliderCtrl()
{
   mSliced = false;
   mUsePlusMinus = false;
   mLabelWidth = 0;
   mInset = 4;
}

void ShellSliderCtrl::initPersistFields()
{
   addField( "usePlusMinus", TypeBool, Offset( mUsePlusMinus, ShellSliderCtrl ),
      "Label the ends of the track LOW and HIGH." );

   Parent::initPersistFields();
}

bool ShellSliderCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   const char *stem = mProfile->mBitmapBase;

   if ( stem && stem[ 0 ] )
   {
      char name[ 256 ];

      dSprintf( name, sizeof( name ), "%s_horzfield", stem );
      mTrackTexture.set( name, &GFXDefaultPersistentProfile,
                         avar( "%s() - track", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_horzbar", stem );
      mThumbTexture.set( name, &GFXDefaultPersistentProfile,
                         avar( "%s() - thumb", __FUNCTION__ ) );

      mSliced = mTrackTexture && mThumbTexture
         && shellSliceGrid( mTrackTexture->getBitmap(), mTrackCell, TrackStates, 3 ) == 3
         && shellSliceGrid( mThumbTexture->getBitmap(), mThumbCell, ThumbStates, 3 ) == 3;
   }

   mLabelWidth = 0;

   if ( mSliced )
   {
      // The thumb is the two caps with nothing between them, and they overlap
      // the track by the inset at each end.
      mThumbSize.set(
         thumbPiece( LeftCap, Resting ).extent.x + thumbPiece( RightCap, Resting ).extent.x
            - mInset * 2,
         thumbPiece( LeftCap, Resting ).extent.y );

      setMinExtent( Point2I( mThumbSize.x * 2, mThumbSize.y ) );
      resize( getPosition(), getExtent() );
   }

   if ( mUsePlusMinus && mProfile->mFont )
      mLabelWidth = S32( mProfile->mFont->getStrWidth( "HIGH" ) ) + mInset;

   return true;
}

void ShellSliderCtrl::onSleep()
{
   Parent::onSleep();

   mTrackTexture = NULL;
   mThumbTexture = NULL;
   mSliced = false;
}

S32 ShellSliderCtrl::thumbState() const
{
   if ( mDepressed )
      return Pressed;

   if ( mMouseOver )
      return Highlight;

   return Resting;
}

void ShellSliderCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   if ( !mSliced )
   {
      Parent::onRender( offset, updateRect );
      return;
   }

   GFXDrawUtil *draw = GFX->getDrawUtil();

   if ( mUsePlusMinus && mProfile->mFont )
   {
      GFont *font = mProfile->mFont;

      const S32 y = offset.y + ( getHeight() - S32( font->getHeight() ) ) / 2;

      draw->setBitmapModulation( mProfile->mFontColor );
      draw->drawText( font, Point2I( offset.x + mInset, y ), "LOW", mProfile->mFontColors );

      draw->drawText( font,
         Point2I( offset.x + getWidth() - mInset - S32( font->getStrWidth( "HIGH" ) ), y ),
         "HIGH", mProfile->mFontColors );
   }

   draw->clearBitmapModulation();

   const RectI &left = trackPiece( LeftCap, mActive );
   const RectI &mid = trackPiece( Middle, mActive );
   const RectI &right = trackPiece( RightCap, mActive );

   Point2I at( offset.x + mLabelWidth, offset.y + mInset );

   draw->drawBitmapSR( mTrackTexture, at, left );
   at.x += left.extent.x;

   const S32 span = getWidth() - left.extent.x - right.extent.x - mLabelWidth * 2;

   if ( span > 0 )
      draw->drawBitmapStretchSR( mTrackTexture,
         RectI( at.x, at.y, span, mid.extent.y ), mid );

   at.x += span;
   draw->drawBitmapSR( mTrackTexture, at, right );

   if ( mActive )
   {
      const S32 state = thumbState();

      const RectI &capLeft = thumbPiece( LeftCap, state );
      const RectI &capRight = thumbPiece( RightCap, state );

      at.set( offset.x + mThumb.point.x + mLabelWidth - 1,
              offset.y + mThumb.point.y );

      draw->drawBitmapSR( mThumbTexture, at, capLeft );
      at.x += capLeft.extent.x;

      draw->drawBitmapSR( mThumbTexture, at, capRight );
   }

   renderChildControls( offset, updateRect );
}
