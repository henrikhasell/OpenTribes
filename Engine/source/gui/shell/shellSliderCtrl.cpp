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

S32 ShellSliderCtrl::trackSpan() const
{
   return getMax( getWidth() - mLabelWidth * 2 - mThumbSize.x, 0 );
}

S32 ShellSliderCtrl::thumbLeft() const
{
   const F32 range = mRange.y - mRange.x;
   const F32 along = range > 0.0f ? ( mValue - mRange.x ) / range : 0.0f;

   return mLabelWidth + S32( mClampF( along, 0.0f, 1.0f ) * trackSpan() + 0.5f );
}

F32 ShellSliderCtrl::valueAt( S32 x ) const
{
   const S32 span = trackSpan();
   if ( span <= 0 )
      return mRange.x;

   const F32 along = F32( x - mLabelWidth - mThumbSize.x / 2 ) / F32( span );

   return mRange.x + mClampF( along, 0.0f, 1.0f ) * ( mRange.y - mRange.x );
}

void ShellSliderCtrl::onSleep()
{
   Parent::onSleep();

   mTrackTexture = NULL;
   mThumbTexture = NULL;
   mSliced = false;
}

void ShellSliderCtrl::onMouseDown( const GuiEvent &event )
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   mouseLock();
   setFirstResponder();
   mDepressed = true;

   setValue( valueAt( globalToLocalCoord( event.mousePoint ).x ), true );
}

void ShellSliderCtrl::onMouseDragged( const GuiEvent &event )
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   mMouseDragged = true;

   setValue( valueAt( globalToLocalCoord( event.mousePoint ).x ), true );
}

void ShellSliderCtrl::onMouseUp( const GuiEvent &event )
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   mouseUnlock();

   mDepressed = false;
   mMouseDragged = false;

   setValue( valueAt( globalToLocalCoord( event.mousePoint ).x ), true );
   execConsoleCallback();
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

      at.set( offset.x + thumbLeft() - mInset,
              offset.y + ( getHeight() - mThumbSize.y ) / 2 );

      draw->drawBitmapSR( mThumbTexture, at, capLeft );
      at.x += capLeft.extent.x;

      draw->drawBitmapSR( mThumbTexture, at, capRight );
   }

   renderChildControls( offset, updateRect );
}
