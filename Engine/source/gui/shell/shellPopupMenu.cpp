#include "platform/platform.h"
#include "gui/shell/shellPopupMenu.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellPopupMenu );

ConsoleDocClass( ShellPopupMenu,
   "@brief A Tribes 2 shell drop-down.\n\n"

   "The closed control is three pieces off buttonBitmap -- a left cap, a "
   "stretched middle and a right cap -- with the current entry's text across "
   "it.  noButtonStyle collapses it to nothing, for a menu that is only ever "
   "opened from somewhere else.\n\n"

   "@ingroup GuiShell"
);

ShellPopupMenu::ShellPopupMenu()
{
   mButtonBitmap = StringTable->insert( "gui/shll_pulldown" );
   mRolloverBarBitmap = StringTable->insert( "gui/shll_pulldownbar_rol" );
   mSelectedBarBitmap = StringTable->insert( "gui/shll_pulldownbar_act" );

   mNoButtonStyle = false;
   mSliced = false;
}

void ShellPopupMenu::initPersistFields()
{
   addField( "buttonBitmap", TypeString, Offset( mButtonBitmap, ShellPopupMenu ),
      "Art for the closed control, cut three wide and three tall." );
   addField( "rolloverBarBitmap", TypeString, Offset( mRolloverBarBitmap, ShellPopupMenu ),
      "Art behind the entry the mouse is over." );
   addField( "selectedBarBitmap", TypeString, Offset( mSelectedBarBitmap, ShellPopupMenu ),
      "Art behind the chosen entry." );
   addField( "noButtonStyle", TypeBool, Offset( mNoButtonStyle, ShellPopupMenu ),
      "Draw nothing and take no room when closed." );

   Parent::initPersistFields();
}

bool ShellPopupMenu::onWake()
{
   if ( !Parent::onWake() )
      return false;

   if ( mButtonBitmap && mButtonBitmap[ 0 ] )
   {
      mTexture.set( mButtonBitmap, &GFXDefaultPersistentProfile,
                    avar( "%s() - mTexture", __FUNCTION__ ) );

      mSliced = mTexture &&
         shellSliceGrid( mTexture->getBitmap(), mCell, StateCount, 3 ) == 3;
   }

   if ( mNoButtonStyle )
   {
      setMinExtent( Point2I( 0, 0 ) );
      resize( getPosition(), Point2I( 0, 0 ) );
   }
   else if ( mSliced )
   {
      setMinExtent( Point2I(
         cell( LeftCap, Resting ).extent.x + cell( RightCap, Resting ).extent.x,
         cell( LeftCap, Resting ).extent.y ) );

      resize( getPosition(), getExtent() );
   }

   return true;
}

void ShellPopupMenu::onSleep()
{
   Parent::onSleep();

   mTexture = NULL;
   mSliced = false;
}

void ShellPopupMenu::onRender( Point2I offset, const RectI &updateRect )
{
   if ( !mSliced )
   {
      Parent::onRender( offset, updateRect );
      return;
   }

   GFXDrawUtil *draw = GFX->getDrawUtil();

   const bool lit = mActive && ( mMouseOver || isFirstResponder() );
   const S32 state = lit ? Highlight : Resting;

   const ColorI &color = !mActive ? mProfile->mFontColorNA
                         : mMouseOver ? mProfile->mFontColorHL
                                      : mProfile->mFontColor;

   draw->clearBitmapModulation();

   const S32 left = cell( LeftCap, Resting ).extent.x;
   const S32 span = getWidth() - left - cell( RightCap, Resting ).extent.x;

   Point2I at = offset;

   draw->drawBitmapSR( mTexture, at, cell( LeftCap, state ) );
   at.x += left;

   if ( span > 0 )
      draw->drawBitmapStretchSR( mTexture,
         RectI( at.x, at.y, span, cell( Middle, state ).extent.y ),
         cell( Middle, state ) );

   at.x += span;
   draw->drawBitmapSR( mTexture, at, cell( RightCap, state ) );

   const char *text = getText();

   if ( !text || !text[ 0 ] || span <= 0 || !mProfile->mFont )
      return;

   RectI clip( offset.x + left, offset.y, span, getHeight() );

   if ( !clip.intersect( updateRect ) )
      return;

   GFX->setClipRect( clip );

   GFont *font = mProfile->mFont;

   Point2I where( offset.x + left,
                  offset.y + ( getHeight() - S32( font->getHeight() ) ) / 2 );

   const S32 width = font->getStrWidth( text );

   if ( span > width )
   {
      if ( mProfile->mAlignment == GuiControlProfile::RightJustify )
         where.x += span - width;
      else if ( mProfile->mAlignment == GuiControlProfile::CenterJustify )
         where.x += ( span - width ) / 2;
   }

   draw->setBitmapModulation( color );
   draw->drawText( font, where, text, mProfile->mFontColors );
}
