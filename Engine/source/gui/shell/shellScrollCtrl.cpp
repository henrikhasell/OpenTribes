#include "platform/platform.h"
#include "gui/shell/shellScrollCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"

IMPLEMENT_CONOBJECT( ShellScrollCtrl );

ConsoleDocClass( ShellScrollCtrl,
   "@brief A Tribes 2 shell scroll container.\n\n"

   "Three sheets off the profile's stem for each direction -- the two arrows, "
   "the thumb and the track -- each carrying a resting, a pressed, a lit and a "
   "disabled state.  The arrows and the thumb are wider than the track they "
   "run in and hang over it on both sides.\n\n"

   "@ingroup GuiShell"
);

ShellScrollCtrl::ShellScrollCtrl()
{
   mFieldBase = StringTable->EmptyString();
   mOverhang = 4;
   mBorderThickness = 4;
}

void ShellScrollCtrl::initPersistFields()
{
   addField( "fieldBase", TypeString, Offset( mFieldBase, ShellScrollCtrl ),
      "Stem for the nine pieces of the frame drawn around the content." );

   Parent::initPersistFields();
}

bool ShellScrollCtrl::loadBar( Bar &bar, const char *stem, const char *which )
{
   char name[ 256 ];

   dSprintf( name, sizeof( name ), "%s_%sfield", stem, which );
   bar.track.set( name, &GFXDefaultPersistentProfile, avar( "%s() - track", __FUNCTION__ ) );

   dSprintf( name, sizeof( name ), "%s_%sbuttons", stem, which );
   bar.buttons.set( name, &GFXDefaultPersistentProfile, avar( "%s() - buttons", __FUNCTION__ ) );

   dSprintf( name, sizeof( name ), "%s_%sbar", stem, which );
   bar.thumb.set( name, &GFXDefaultPersistentProfile, avar( "%s() - thumb", __FUNCTION__ ) );

   bar.ready = bar.track && bar.buttons && bar.thumb
      && shellSliceGrid( bar.track->getBitmap(), bar.trackCell, 2, 3 ) == 3
      && shellSliceGrid( bar.buttons->getBitmap(), bar.buttonCell, StateCount, 2 ) == 2
      && shellSliceGrid( bar.thumb->getBitmap(), bar.thumbCell, StateCount, 3 ) == 3;

   return bar.ready;
}

bool ShellScrollCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   const char *stem = mProfile->mBitmapBase;

   if ( stem && stem[ 0 ] )
   {
      loadBar( mVert, stem, "vert" );
      loadBar( mHorz, stem, "horz" );

      char name[ 256 ];
      dSprintf( name, sizeof( name ), "%s_scale", stem );

      mScaleTexture.set( name, &GFXDefaultPersistentProfile,
                         avar( "%s() - scale", __FUNCTION__ ) );

      if ( mScaleTexture &&
           shellSliceGrid( mScaleTexture->getBitmap(), mScaleCell, StateCount, 1 ) != 1 )
         mScaleTexture = NULL;
   }

   mFieldSkin.setBase( mFieldBase );

   if ( mVert.ready )
   {
      mBaseThumbSize = mVert.thumbPiece( 0, Resting ).extent.y
                     + mVert.thumbPiece( 2, Resting ).extent.y
                     - mOverhang * 2;

      mScrollBarThickness = mVert.trackPiece( 1, true ).extent.x;
      mScrollBarArrowBtnLength = mVert.button( 0, Resting ).extent.y - mOverhang * 2;

      computeSizes();
   }

   return true;
}

void ShellScrollCtrl::onSleep()
{
   Parent::onSleep();

   mVert = Bar();
   mHorz = Bar();

   mScaleTexture = NULL;
   mFieldSkin.clear();
}

S32 ShellScrollCtrl::artState( Region region, bool enabled )
{
   if ( !enabled )
      return Disabled;

   if ( mHitRegion != region )
      return Resting;

   return mStateDepressed ? Pressed : Highlight;
}

void ShellScrollCtrl::drawVScrollBar( const Point2I &offset )
{
   if ( !mVert.ready )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   Point2I at( offset.x + mUpArrowRect.point.x - mOverhang,
               offset.y + mUpArrowRect.point.y - mOverhang );

   draw->drawBitmapSR( mVert.buttons, at,
      mVert.button( 0, artState( UpArrow, mVBarEnabled ) ) );

   const RectI &top = mVert.trackPiece( 0, mVBarEnabled );
   const RectI &mid = mVert.trackPiece( 1, mVBarEnabled );
   const RectI &bot = mVert.trackPiece( 2, mVBarEnabled );

   at.x += mOverhang;
   at.y += mOverhang + mScrollBarArrowBtnLength;

   draw->drawBitmapSR( mVert.track, at, top );
   at.y += top.extent.y;

   const S32 span = mDownArrowRect.point.y - mUpArrowRect.point.y
                  - mScrollBarArrowBtnLength - top.extent.y - bot.extent.y;

   if ( span > 0 )
      draw->drawBitmapStretchSR( mVert.track,
         RectI( at.x, at.y, mid.extent.x, span ), mid );

   at.y += span;
   draw->drawBitmapSR( mVert.track, at, bot );

   if ( mVBarEnabled )
   {
      const S32 state = artState( VertThumb, true );

      const RectI &capTop = mVert.thumbPiece( 0, state );
      const RectI &body = mVert.thumbPiece( 1, state );
      const RectI &capBot = mVert.thumbPiece( 2, state );

      at.x = offset.x + mUpArrowRect.point.x - mOverhang;
      at.y = offset.y + mVThumbPos - mOverhang;

      draw->drawBitmapSR( mVert.thumb, at, capTop );
      at.y += capTop.extent.y;

      const S32 body_h = mVThumbSize - capTop.extent.y - capBot.extent.y
                       + mOverhang * 2;

      if ( body_h > 0 )
         draw->drawBitmapStretchSR( mVert.thumb,
            RectI( at.x, at.y, body.extent.x, body_h ), body );

      at.y += body_h;
      draw->drawBitmapSR( mVert.thumb, at, capBot );
   }

   at.set( offset.x + mDownArrowRect.point.x - mOverhang,
           offset.y + mDownArrowRect.point.y - mOverhang );

   draw->drawBitmapSR( mVert.buttons, at,
      mVert.button( 1, artState( DownArrow, mVBarEnabled ) ) );
}

void ShellScrollCtrl::drawHScrollBar( const Point2I &offset )
{
   if ( !mHorz.ready )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   Point2I at( offset.x + mLeftArrowRect.point.x - mOverhang,
               offset.y + mLeftArrowRect.point.y - mOverhang );

   draw->drawBitmapSR( mHorz.buttons, at,
      mHorz.button( 0, artState( LeftArrow, mHBarEnabled ) ) );

   const RectI &left = mHorz.trackPiece( 0, mHBarEnabled );
   const RectI &mid = mHorz.trackPiece( 1, mHBarEnabled );
   const RectI &right = mHorz.trackPiece( 2, mHBarEnabled );

   at.x += mOverhang + mScrollBarArrowBtnLength;
   at.y += mOverhang;

   draw->drawBitmapSR( mHorz.track, at, left );
   at.x += left.extent.x;

   const S32 span = mRightArrowRect.point.x - mLeftArrowRect.point.x
                  - mScrollBarArrowBtnLength - left.extent.x - right.extent.x;

   if ( span > 0 )
      draw->drawBitmapStretchSR( mHorz.track,
         RectI( at.x, at.y, span, mid.extent.y ), mid );

   at.x += span;
   draw->drawBitmapSR( mHorz.track, at, right );

   if ( mHBarEnabled )
   {
      const S32 state = artState( HorizThumb, true );

      const RectI &capLeft = mHorz.thumbPiece( 0, state );
      const RectI &body = mHorz.thumbPiece( 1, state );
      const RectI &capRight = mHorz.thumbPiece( 2, state );

      at.x = offset.x + mHThumbPos - mOverhang;
      at.y = offset.y + mLeftArrowRect.point.y - mOverhang;

      draw->drawBitmapSR( mHorz.thumb, at, capLeft );
      at.x += capLeft.extent.x;

      const S32 body_w = mHThumbSize - capLeft.extent.x - capRight.extent.x
                       + mOverhang * 2;

      if ( body_w > 0 )
         draw->drawBitmapStretchSR( mHorz.thumb,
            RectI( at.x, at.y, body_w, body.extent.y ), body );

      at.x += body_w;
      draw->drawBitmapSR( mHorz.thumb, at, capRight );
   }

   at.set( offset.x + mRightArrowRect.point.x - mOverhang,
           offset.y + mRightArrowRect.point.y - mOverhang );

   draw->drawBitmapSR( mHorz.buttons, at,
      mHorz.button( 1, artState( RightArrow, mHBarEnabled ) ) );
}

void ShellScrollCtrl::drawScrollCorner( const Point2I &offset )
{
   if ( !mScaleTexture )
      return;

   Point2I at( offset.x + mRightArrowRect.point.x + mRightArrowRect.extent.x - mOverhang,
               offset.y + mRightArrowRect.point.y - mOverhang );

   GFX->getDrawUtil()->clearBitmapModulation();
   GFX->getDrawUtil()->drawBitmapSR( mScaleTexture, at, mScaleCell[ Disabled ] );
}

S32 ShellScrollCtrl::borderThickness() const
{
   return mBorderThickness;
}

void ShellScrollCtrl::calcScrollRects()
{
   const S32 inset = borderThickness();
   const S32 bar = mScrollBarThickness;
   const S32 arrow = mScrollBarArrowBtnLength;

   if ( mHasHScrollBar )
   {
      const S32 y = getHeight() - inset - bar - 1;

      mLeftArrowRect.set( inset, y, arrow, bar );

      mRightArrowRect.set( getWidth() - inset - ( mHasVScrollBar ? bar : 0 ) - arrow,
                           y, arrow, bar );

      mHTrackRect.set( inset + arrow, y,
                       mRightArrowRect.point.x - inset - arrow, bar );
   }

   if ( mHasVScrollBar )
   {
      const S32 x = getWidth() - inset - bar;

      mUpArrowRect.set( x, inset, bar, arrow );

      mDownArrowRect.set( x,
         getHeight() - inset - ( mHasHScrollBar ? bar + 1 : 0 ) - arrow, bar, arrow );

      mVTrackRect.set( x, inset + arrow, bar,
                       mDownArrowRect.point.y - inset - arrow );
   }
}

void ShellScrollCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   drawBorder( offset, isFirstResponder() );

   Parent::onRender( offset, updateRect );
}

void ShellScrollCtrl::drawBorder( const Point2I &offset, bool isFirstResponder )
{
   static const char *suffix[ 9 ] =
   {
      "_TL", "_TM", "_TR",
      "_ML", "_MM", "_MR",
      "_BL", "_BM", "_BR"
   };

   if ( !mFieldBase || !mFieldBase[ 0 ] )
      return;

   GFXTexHandle part[ 9 ];

   for ( U32 i = 0; i < 9; i++ )
   {
      part[ i ] = mFieldSkin.get( suffix[ i ] );

      if ( !part[ i ] )
         return;
   }

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const S32 inset = borderThickness();

   Point2I extent( getWidth() - inset * 2, getHeight() - inset * 2 );

   if ( mHasVScrollBar )
      extent.x -= mScrollBarThickness;

   if ( mHasHScrollBar )
      extent.y -= mScrollBarThickness + 1;

   const S32 left = S32( part[ 0 ]->getBitmapWidth() );
   const S32 right = S32( part[ 2 ]->getBitmapWidth() );
   const S32 span = extent.x - left - right;

   const S32 topH = S32( part[ 0 ]->getBitmapHeight() );
   const S32 botH = S32( part[ 6 ]->getBitmapHeight() );
   const S32 midH = extent.y - topH - botH;

   Point2I at( offset.x + inset, offset.y + inset );

   for ( U32 row = 0; row < 3; row++ )
   {
      const S32 h = row == 0 ? topH : row == 1 ? midH : botH;

      if ( h <= 0 )
      {
         at.y += h;
         continue;
      }

      at.x = offset.x + inset;

      draw->drawBitmapStretch( part[ row * 3 ], RectI( at.x, at.y, left, h ) );
      at.x += left;

      if ( span > 0 )
         draw->drawBitmapStretch( part[ row * 3 + 1 ], RectI( at.x, at.y, span, h ) );

      at.x += span;
      draw->drawBitmapStretch( part[ row * 3 + 2 ], RectI( at.x, at.y, right, h ) );

      at.y += h;
   }
}
