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
#include "gui/shell/shellScrollCtrl.h"

/// The list the drop-down opens.  Tribes 2 marks the entry under the mouse
/// and the chosen one with a bar of art rather than a block of colour, which
/// is what the two bar bitmaps on the menu are for.
class ShellPopupTextList : public GuiPopupTextListCtrl
{
   typedef GuiPopupTextListCtrl Parent;

   GFXTexHandle mRollover;
   GFXTexHandle mSelected;

public:
   ShellPopupTextList( ShellPopupMenu *menu, const char *rollover, const char *selected )
      : GuiPopupTextListCtrl( menu )
   {
      if ( rollover && rollover[ 0 ] )
         mRollover.set( rollover, &GFXDefaultPersistentProfile,
                        avar( "%s() - rollover", __FUNCTION__ ) );

      if ( selected && selected[ 0 ] )
         mSelected.set( selected, &GFXDefaultPersistentProfile,
                        avar( "%s() - selected", __FUNCTION__ ) );
   }

   void onRenderCell( Point2I offset, Point2I cell, bool selected, bool mouseOver ) override
   {
      Point2I size;
      getCellSize( size );

      GFXDrawUtil *draw = GFX->getDrawUtil();

      GFXTexHandle bar = mouseOver ? mRollover : ( selected ? mSelected : GFXTexHandle() );

      if ( bar )
      {
         draw->clearBitmapModulation();
         draw->drawBitmapStretch( bar, RectI( offset, size ) );
      }

      ColorI fontColor;
      mPopUpCtrl->getFontColor( fontColor, mList[ cell.y ].id, selected, mouseOver );

      // The profile these menus use says nothing about a margin -- its
      // textOffset is "0 0" -- and an entry drawn on the panel's edge is not
      // what stock shows, which holds them five pixels in.
      const S32 margin = mProfile->mTextOffset.x ? mProfile->mTextOffset.x : 5;

      draw->setBitmapModulation( fontColor );
      draw->drawText( mFont, Point2I( offset.x + margin, offset.y ),
                      mList[ cell.y ].text );
      draw->clearBitmapModulation();
   }
};

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

void ShellPopupMenu::addChildren()
{
   // Torque builds the drop-down out of a plain scroll control skinned from
   // GuiScrollProfile, which is Torque's own chrome and draws nothing a shell
   // screen would recognise -- the entries came out over bare background.
   // Tribes 2 hands both children the menu's own profile and skins the
   // container with the same art the closed control uses.
   mTl = new ShellPopupTextList( this, mRolloverBarBitmap, mSelectedBarBitmap );
   mTl->setControlProfile( mProfile );
   mTl->setField( "noDuplicates", "false" );

   ShellScrollCtrl *scroll = new ShellScrollCtrl;

   scroll->setControlProfile( mProfile );

   // A shell scroll control skins itself off a stem, and the one it defaults
   // to is the entry field's border -- a different control's.  Point it at
   // the menu's own art, which is the texture Tribes 2 hands its drop-down's
   // container, and the panel gets the pulldown's fill and edges.
   scroll->setField( "fieldBase", mButtonBitmap );
   scroll->setField( "hScrollBar", "AlwaysOff" );
   scroll->setField( "vScrollBar", "dynamic" );

   mSc = scroll;

   mBackground = new GuiPopUpBackgroundCtrl( this, mTl );
}

void ShellPopupMenu::onAction()
{
   Parent::onAction();

   GuiControl *parent = getParent();

   if ( !mSc || !mTl || !mBackground || !parent )
      return;

   // gui/shll_pulldown carries the control's glow as clear pixels down its
   // outside edges, so the button the player sees is smaller than the control
   // holding it: its face starts eight columns in, stops ten short, and
   // leaves twelve rows clear underneath.  Tribes 2 hangs the list off the
   // face rather than off the control, which is what keeps it no wider than
   // the button and joined to its bottom edge instead of floating below it.
   enum { FaceLeft = 8, FaceRight = 10, FaceBottom = 12 };

   const Point2I at =
      mBackground->globalToLocalCoord( parent->localToGlobalCoord( getPosition() ) );

   RectI bounds = mSc->getBounds();

   // Which way the menu opened is Torque's decision and stands; the list just
   // meets whichever edge of the button it was put against.
   const bool above = bounds.point.y < at.y;

   // The container insets its own frame inside its bounds, so the bounds have
   // to be let out by that much for the frame to land on the button's face
   // rather than beside it.
   const S32 frame = mSc->getBorderThickness();

   bounds.extent.x = getMax( getWidth() - FaceLeft - FaceRight + frame * 2, 1 );
   bounds.point.x = at.x + FaceLeft - frame;
   bounds.point.y = above ? at.y - bounds.extent.y + FaceBottom
                          : at.y + getHeight() - FaceBottom;

   mSc->resize( bounds.point, bounds.extent );

   // The rows were sized for the width Torque picked, so they have to be told
   // about the narrower one or they run out past the panel.
   const S32 inner = mSc->getContentExtent().x - mSc->getChildMargin().x * 2;

   if ( inner > 0 )
   {
      Point2I cell;
      mTl->getCellSize( cell );
      cell.x = inner;

      // Resizing the list drops whatever was selected, and which entry that
      // is has already been settled -- including the reversal a menu that
      // opens upward does -- so put it back rather than work it out again.
      const Point2I selected = mTl->getSelectedCell();

      mTl->setCellSize( cell );
      mTl->setWidth( inner );

      if ( selected.y >= 0 )
         mTl->setSelectedCell( selected );
   }
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
