#include "platform/platform.h"
#include "gui/shell/shellWindowCtrl.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/core/guiCanvas.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellWindowCtrl );

ConsoleDocClass( ShellWindowCtrl,
   "@brief A Tribes 2 shell window.\n\n"

   "A titled tab across the top -- one cap drawn at each end, the second of "
   "them mirrored, with the middle stretched between -- over a body that is a "
   "gradient, a fill, and the gradient again upside down.  The title bar moves "
   "the window, the right and bottom edges size it, and the button at the far "
   "end of the title bar runs its close command.\n\n"

   "@ingroup GuiShell"
);

ShellWindowCtrl::ShellWindowCtrl()
{
   mFrameBase = StringTable->insert( "gui/window" );
   mCloseCommand = StringTable->EmptyString();

   mCanMove = true;
   mCanClose = true;

   mBorderWidth = 2;

   mSliced = false;
   mTitleHeight = 20;
   mSizingMargin = 4;

   mMode = Idle;
   mCloseOver = false;

   mCloseRect.set( 0, 0, 0, 0 );
   mCloseInset = 4;

   for ( S32 i = 0; i < 2; i++ )
      mTitleCell[ i ].set( 0, 0, 0, 0 );

   for ( S32 i = 0; i < 3; i++ )
      mCloseCell[ i ].set( 0, 0, 0, 0 );
}

void ShellWindowCtrl::initPersistFields()
{
   addField( "frameBase", TypeString, Offset( mFrameBase, ShellWindowCtrl ),
      "Stem of the title tab, close button and corner bitmaps." );
   addField( "borderWidth", TypeS32, Offset( mBorderWidth, ShellWindowCtrl ),
      "Width of the rules down the sides and under the window." );
   addField( "canMove", TypeBool, Offset( mCanMove, ShellWindowCtrl ),
      "Whether the title bar moves the window." );
   addField( "canClose", TypeBool, Offset( mCanClose, ShellWindowCtrl ),
      "Whether the title bar carries a close button." );
   addField( "closeCommand", TypeString, Offset( mCloseCommand, ShellWindowCtrl ),
      "Script run when the close button is pressed and let go." );

   Parent::initPersistFields();
}

bool ShellWindowCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   {
      char name[ 256 ];

      const char *stem = mProfile->mBitmapBase;

      if ( stem && stem[ 0 ] )
      {
         dSprintf( name, sizeof( name ), "%s_fieldfill", stem );
         mFillTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - fill", __FUNCTION__ ) );

         dSprintf( name, sizeof( name ), "%s_fieldgrade", stem );
         mGradeTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - grade", __FUNCTION__ ) );
      }

      if ( mFrameBase[ 0 ] )
      {
         dSprintf( name, sizeof( name ), "%s_titleTab", mFrameBase );
         mTitleTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - title", __FUNCTION__ ) );

         dSprintf( name, sizeof( name ), "%s_close", mFrameBase );
         mCloseTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - close", __FUNCTION__ ) );

         dSprintf( name, sizeof( name ), "%s_corner", mFrameBase );
         mCornerTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - corner", __FUNCTION__ ) );

         mSliced = mTitleTexture &&
            shellSliceGrid( mTitleTexture->getBitmap(), mTitleCell, 1, 2 ) == 2;

         if ( mCloseTexture &&
              shellSliceGrid( mCloseTexture->getBitmap(), mCloseCell, 3, 1 ) != 1 )
            mCloseTexture = NULL;
      }
   }

   if ( mSliced )
   {
      mTitleHeight = mTitleCell[ TitleCap ].extent.y;

      Point2I minExtent = getMinExtent();

      minExtent.x = getMax( minExtent.x, mTitleCell[ TitleCap ].extent.x * 2 );
      minExtent.y = getMax( minExtent.y, mTitleHeight + mBorderWidth );

      setMinExtent( minExtent );
   }

   resize( getPosition(), getExtent() );

   return true;
}

void ShellWindowCtrl::onSleep()
{
   Parent::onSleep();

   mTitleTexture = NULL;
   mCloseTexture = NULL;
   mCornerTexture = NULL;
   mFillTexture = NULL;
   mGradeTexture = NULL;

   mSliced = false;
}

bool ShellWindowCtrl::resize( const Point2I &position, const Point2I &extent )
{
   const bool changed = Parent::resize( position, extent );

   const Point2I &close = mCloseCell[ 0 ].extent;

   mCloseRect.point.set( getWidth() - close.x + mCloseInset,
                         ( mTitleHeight - close.y ) / 2 + mCloseInset );

   mCloseRect.extent.set( close.x - mCloseInset * 2, close.y - mCloseInset * 2 );

   return changed;
}

bool ShellWindowCtrl::loadCursors()
{
   if ( mArrowCursor.isNull() )
   {
      mArrowCursor = dynamic_cast< GuiCursor * >( Sim::findObject( "DefaultCursor" ) );
      mLeftRightCursor = dynamic_cast< GuiCursor * >( Sim::findObject( "LeftRightCursor" ) );
      mUpDownCursor = dynamic_cast< GuiCursor * >( Sim::findObject( "UpDownCursor" ) );
      mDiagonalCursor = dynamic_cast< GuiCursor * >( Sim::findObject( "DiagLeftCursor" ) );
   }

   return !mArrowCursor.isNull() && !mLeftRightCursor.isNull() &&
          !mUpDownCursor.isNull() && !mDiagonalCursor.isNull();
}

S32 ShellWindowCtrl::sizingAt( const Point2I &local ) const
{
   if ( local.y < mTitleHeight )
      return Idle;

   if ( mCornerTexture &&
        local.x > getWidth() - S32( mCornerTexture->getBitmapWidth() ) &&
        local.y > getHeight() - S32( mCornerTexture->getBitmapHeight() ) )
      return SizingWidth | SizingHeight;

   S32 mode = Idle;

   if ( local.x > getWidth() - mSizingMargin )
      mode |= SizingWidth;

   if ( local.y > getHeight() - mSizingMargin )
      mode |= SizingHeight;

   return mode;
}

void ShellWindowCtrl::selectWindow()
{
   GuiControl *parent = getParent();

   if ( parent )
      parent->pushObjectToBack( this );

   setFirstResponder( getFirstResponder() );
}

void ShellWindowCtrl::onMouseDown( const GuiEvent &event )
{
   setUpdate();

   mOrigBounds = getBounds();
   mMouseDownPosition = event.mousePoint;

   const Point2I local = globalToLocalCoord( event.mousePoint );

   selectWindow();

   if ( local.y < mTitleHeight )
   {
      if ( mCloseRect.pointInRect( local ) && mCanClose )
         mMode = Closing;
      else if ( mCanMove )
         mMode = Moving;
   }
   else
      mMode = sizingAt( local );

   if ( mMode != Idle )
      mouseLock();
}

void ShellWindowCtrl::onMouseDragged( const GuiEvent &event )
{
   GuiControl *parent = getParent();
   GuiCanvas *root = getRoot();

   if ( !parent || !root )
      return;

   const Point2I moved = event.mousePoint - mMouseDownPosition;

   RectI bounds = getBounds();
   bool changed = false;

   if ( mMode == Moving )
   {
      bounds.point.x = mClamp( mOrigBounds.point.x + moved.x, 0,
                               parent->getWidth() - getWidth() );
      bounds.point.y = mClamp( mOrigBounds.point.y + moved.y, 0,
                               parent->getHeight() - getHeight() );

      changed = true;
   }
   else
   {
      if ( mMode & SizingWidth )
      {
         bounds.extent.x = getMax( getMin( mOrigBounds.extent.x + moved.x,
                                           parent->getWidth() ), getMinExtent().x );
         changed = true;
      }

      if ( mMode & SizingHeight )
      {
         bounds.extent.y = getMax( getMin( mOrigBounds.extent.y + moved.y,
                                           parent->getHeight() ), getMinExtent().y );
         changed = true;
      }
   }

   if ( !changed )
      return;

   root->addUpdateRegion( parent->localToGlobalCoord( getPosition() ), getExtent() );

   resize( bounds.point, bounds.extent );
}

void ShellWindowCtrl::onMouseUp( const GuiEvent &event )
{
   mouseUnlock();

   if ( !getParent() )
      return;

   if ( mMode == Closing && mCloseCommand[ 0 ] &&
        mCloseRect.pointInRect( globalToLocalCoord( event.mousePoint ) ) )
      Con::evaluate( mCloseCommand, false );

   mMode = Idle;
}

void ShellWindowCtrl::onMouseMove( const GuiEvent &event )
{
   Parent::onMouseMove( event );

   const bool over = mCloseRect.pointInRect( globalToLocalCoord( event.mousePoint ) );

   if ( over != mCloseOver )
   {
      mCloseOver = over;

      setUpdateRegion( mCloseRect.point - Point2I( mCloseInset, mCloseInset ),
                       mCloseRect.extent + Point2I( mCloseInset, mCloseInset ) * 2 );
   }
}

void ShellWindowCtrl::onMouseEnter( const GuiEvent &event )
{
   Parent::onMouseEnter( event );

   onMouseMove( event );
}

void ShellWindowCtrl::onMouseLeave( const GuiEvent &event )
{
   Parent::onMouseLeave( event );

   if ( mCloseOver )
   {
      mCloseOver = false;

      setUpdateRegion( mCloseRect.point - Point2I( mCloseInset, mCloseInset ),
                       mCloseRect.extent + Point2I( mCloseInset, mCloseInset ) * 2 );
   }
}

void ShellWindowCtrl::getCursor( GuiCursor *&cursor, bool &showCursor,
                                 const GuiEvent &event )
{
   if ( !loadCursors() )
      return;

   const S32 mode = mMode != Idle
      ? mMode : sizingAt( globalToLocalCoord( event.mousePoint ) );

   switch ( mode & ( SizingWidth | SizingHeight ) )
   {
      case SizingWidth:
         cursor = mLeftRightCursor;
         break;

      case SizingHeight:
         cursor = mUpDownCursor;
         break;

      case SizingWidth | SizingHeight:
         cursor = mDiagonalCursor;
         break;

      default:
         cursor = mArrowCursor;
         break;
   }
}

void ShellWindowCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const S32 cap = mSliced ? mTitleCell[ TitleCap ].extent.x : 0;
   const S32 span = getWidth() - cap * 2;

   if ( mSliced )
   {
      Point2I at = offset;

      draw->drawBitmapSR( mTitleTexture, at, mTitleCell[ TitleCap ] );
      at.x += cap;

      if ( span > 0 )
         draw->drawBitmapStretchSR( mTitleTexture,
            RectI( at.x, at.y, span, mTitleHeight ), mTitleCell[ TitleMiddle ] );

      at.x += span;

      draw->drawBitmapSR( mTitleTexture, at, mTitleCell[ TitleCap ], GFXBitmapFlip_X );

      const char *text = getText();

      if ( text && text[ 0 ] && span > 0 && mProfile->mFont )
      {
         const RectI saved = GFX->getClipRect();

         RectI clip( offset.x + cap, offset.y, span, mTitleHeight );

         if ( clip.intersect( updateRect ) )
         {
            GFX->setClipRect( clip );

            GFont *font = mProfile->mFont;

            const S32 width = S32( font->getStrWidth( text ) );

            Point2I where( offset.x + cap + ( span > width ? ( span - width ) / 2 : 0 ),
                           offset.y + ( mTitleHeight - S32( font->getHeight() ) ) / 2 );

            draw->setBitmapModulation( mProfile->mFontColor );
            draw->drawText( font, where, text, mProfile->mFontColors );
            draw->clearBitmapModulation();
         }

         GFX->setClipRect( saved );
      }
   }

   if ( mCanClose && mCloseTexture )
   {
      const S32 cell = mCloseOver ? ( mMode == Closing ? 1 : 2 ) : 0;

      draw->drawBitmapSR( mCloseTexture,
         offset + mCloseRect.point - Point2I( mCloseInset, mCloseInset ),
         mCloseCell[ cell ] );
   }

   if ( mGradeTexture && mFillTexture && getWidth() > mBorderWidth * 2 )
   {
      const S32 grade = S32( mGradeTexture->getBitmapHeight() );

      S32 body = getHeight() - mTitleHeight - mBorderWidth;

      const bool roomy = body > grade * 2;

      RectI at( offset.x + mBorderWidth, offset.y + mTitleHeight,
                getWidth() - mBorderWidth * 2, roomy ? grade : body / 2 );

      draw->drawBitmapStretch( mGradeTexture, at );

      at.point.y += at.extent.y;

      if ( roomy )
      {
         body -= grade * 2;

         at.extent.y = body;

         draw->drawBitmapStretch( mFillTexture, at );

         at.point.y += body;
         at.extent.y = grade;
      }
      else
         at.extent.y = body - at.extent.y;

      draw->drawBitmapStretch( mGradeTexture, at, GFXBitmapFlip_Y );
   }

   if ( mBorderWidth > 0 )
   {
      RectI at( offset.x, offset.y + mTitleHeight,
                mBorderWidth + 1, getHeight() - mTitleHeight + 1 );

      draw->drawRectFill( at, mProfile->mBorderColor );

      at.point.x += getWidth() - mBorderWidth;

      if ( mCornerTexture )
         at.extent.y -= S32( mCornerTexture->getBitmapHeight() );

      draw->drawRectFill( at, mProfile->mBorderColor );

      at.point.set( offset.x + mBorderWidth, offset.y + getHeight() - mBorderWidth );
      at.extent.set( getWidth() - mBorderWidth + 1, mBorderWidth + 1 );

      if ( mCornerTexture )
         at.extent.x -= S32( mCornerTexture->getBitmapWidth() );

      draw->drawRectFill( at, mProfile->mBorderColor );
   }

   if ( mCornerTexture )
   {
      draw->clearBitmapModulation();
      draw->drawBitmap( mCornerTexture,
         offset + getExtent() - Point2I( S32( mCornerTexture->getBitmapWidth() ),
                                         S32( mCornerTexture->getBitmapHeight() ) ) );
   }

   renderChildControls( offset, updateRect );
}
