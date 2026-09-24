#include "platform/platform.h"
#include "gui/shell/shellPaneCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"

IMPLEMENT_CONOBJECT( ShellPaneCtrl );

ConsoleDocClass( ShellPaneCtrl,
   "@brief The frame a Tribes 2 shell screen sits in.\n\n"

   "A title bar across the top carrying the screen's name, rails down the "
   "sides, and a lit-from-above interior its children are drawn over.  Every "
   "shell layout is one of these with the screen inside it.\n\n"

   "@ingroup GuiShell"
);

ShellPaneCtrl::ShellPaneCtrl()
{
   mText = StringTable->insert( "" );
   mNoTitleBar = false;
   mHaveTitleCells = false;

   setExtent( 200, 100 );
}

void ShellPaneCtrl::initPersistFields()
{
   addField( "text", TypeCaseString, Offset( mText, ShellPaneCtrl ),
      "The name shown in the title bar." );
   addField( "noTitleBar", TypeBool, Offset( mNoTitleBar, ShellPaneCtrl ),
      "Draw the frame without a title bar, for a pane that is part of a larger one." );

   Parent::initPersistFields();
}

void ShellPaneCtrl::setText( const char *text )
{
   mText = StringTable->insert( text ? text : "", true );
   setUpdate();
}

void ShellPaneCtrl::updateSkin()
{
   mSkin.setBase( mProfile ? mProfile->mBitmapBase : NULL );

   if ( mHaveTitleCells )
      return;

   GFXTexHandle tab = mSkin.get( "_titletab" );
   if ( !tab )
      return;

   // Two cells stacked: the chamfered left end, and the length that runs
   // between the ends.
   mHaveTitleCells = shellSliceGrid( tab->getBitmap(), mTitleCell, 1, 2 ) == 2;
}

S32 ShellPaneCtrl::titleHeight()
{
   if ( mNoTitleBar || !mHaveTitleCells )
      return 0;

   return mTitleCell[ 1 ].extent.y;
}

void ShellPaneCtrl::drawTitleBar( const Point2I &offset )
{
   const S32 height = titleHeight();
   if ( !height )
      return;

   GFXTexHandle tab = mSkin.get( "_titletab" );
   GFXTexHandle rail = mSkin.get( "_frame_end" );
   if ( !tab )
      return;

   // The bar sits between the rails rather than over them.
   const S32 railWidth = rail ? S32( rail->getWidth() ) : 0;
   const S32 capWidth = mTitleCell[ 0 ].extent.x;

   const S32 span = getWidth() - ( capWidth + railWidth ) * 2;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   S32 x = offset.x + railWidth;

   draw->drawBitmapSR( tab, Point2I( x, offset.y ), mTitleCell[ 0 ] );
   x += capWidth;

   if ( span > 0 )
   {
      draw->drawBitmapStretchSR( tab, RectI( x, offset.y, span, height ), mTitleCell[ 1 ] );
      x += span;
   }

   // The right end is the left one turned round; there is no separate art.
   draw->drawBitmapSR( tab, Point2I( x, offset.y ), mTitleCell[ 0 ], GFXBitmapFlip_X );

   if ( mProfile->mFont && mText && mText[ 0 ] )
   {
      const S32 textWidth = mProfile->mFont->getStrWidth( mText );
      const S32 textHeight = mProfile->mFont->getHeight();

      const Point2I at( offset.x + ( getWidth() - textWidth ) / 2,
                        offset.y + ( height - textHeight ) / 2 );

      draw->setBitmapModulation( mProfile->mFontColor );
      draw->drawText( mProfile->mFont, at, mText );
      draw->clearBitmapModulation();
   }
}

void ShellPaneCtrl::drawFrame( const Point2I &offset )
{
   GFXTexHandle rail = mSkin.get( "_frame_end" );
   GFXTexHandle fill = mSkin.get( "_fieldfill" );
   GFXTexHandle grade = mSkin.get( "_fieldgrade" );

   if ( !rail || !fill || !grade )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const S32 railWidth = S32( rail->getWidth() );
   const S32 gradeHeight = S32( grade->getHeight() );

   const S32 top = titleHeight();
   const S32 insideWidth = getWidth() - railWidth * 2;
   const S32 insideHeight = getHeight() - top;

   // The interior is three bands, not a fill with a gradient over it: the
   // gradient runs down from under the title, the fill carries the middle, and
   // the gradient runs again at the bottom turned over, so the panel is lit
   // from both edges towards its centre.  A panel too short to hold both
   // gradients gets the two of them and no fill.
   if ( insideWidth > 0 && insideHeight > 0 )
   {
      const bool roomForFill = insideHeight > gradeHeight * 2;

      S32 y = offset.y + top;
      const S32 x = offset.x + railWidth;

      const S32 first = roomForFill ? gradeHeight : insideHeight / 2;
      draw->drawBitmapStretch( grade, RectI( x, y, insideWidth, first ) );
      y += first;

      if ( roomForFill )
      {
         const S32 middle = insideHeight - gradeHeight * 2;
         draw->drawBitmapStretch( fill, RectI( x, y, insideWidth, middle ) );
         y += middle;
      }

      const S32 last = roomForFill ? gradeHeight : insideHeight - first;
      draw->drawBitmapStretch( grade, RectI( x, y, insideWidth, last ), GFXBitmapFlip_Y );
   }
}

bool ShellPaneCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   updateSkin();
   return true;
}

void ShellPaneCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   updateSkin();

   drawFrame( offset );
   drawTitleBar( offset );

   renderChildControls( offset, updateRect );
}

DefineEngineMethod( ShellPaneCtrl, setTitle, void, ( const char *title ), ,
   "@brief Change the name shown in the title bar.\n\n@ingroup GuiShell" )
{
   object->setText( title );
}
