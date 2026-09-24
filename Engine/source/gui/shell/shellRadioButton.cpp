#include "platform/platform.h"
#include "gui/shell/shellRadioButton.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellRadioButton );

ConsoleDocClass( ShellRadioButton,
   "@brief A Tribes 2 radio button with a label on a bar.\n\n"

   "The profile's bitmap is one column of five cells: the box unlit, the box "
   "lit, a rail, a right cap, and a glow.  The first four are laid left to "
   "right with only the rail stretched, so the label sits on a bar as wide as "
   "the control, and the glow goes back over the box while the button has the "
   "mouse or the focus.\n\n"

   "@ingroup GuiShell"
);

ShellRadioButton::ShellRadioButton()
{
   mButtonType = ButtonTypeRadio;
   mSliced = false;
}

bool ShellRadioButton::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mTexture = mProfile->mTextureObject;
   mSliced = mTexture &&
      shellSliceGrid( mTexture->getBitmap(), mCell, 1, CellCount ) == CellCount;

   if ( mSliced )
   {
      // The two caps are what the button has to carry before any of it is
      // drawn whole, so they are its minimum.  Resizing to the extent it
      // already has is what applies that.
      setMinExtent( Point2I( mCell[ BoxOff ].extent.x + mCell[ RightCap ].extent.x,
                             mCell[ BoxOff ].extent.y ) );
      resize( getPosition(), getExtent() );
   }

   return true;
}

void ShellRadioButton::onSleep()
{
   Parent::onSleep();

   mTexture = NULL;
   mSliced = false;
   resetState();
}

void ShellRadioButton::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   // The bar runs from the box to the right cap, and is as tall as the rail
   // was drawn rather than as tall as the control.  Without art there is no
   // box, and the label has the whole of the control to itself.
   const S32 barLeft = mSliced ? mCell[ BoxOff ].extent.x : 0;
   const S32 span = getWidth() - barLeft -
      ( mSliced ? mCell[ RightCap ].extent.x : 0 );
   S32 barHeight = mSliced ? mCell[ Rail ].extent.y : 0;

   if ( mSliced )
   {
      // An inactive button is the same art at half brightness; there is no
      // separate cell for it.
      if ( mActive )
         draw->clearBitmapModulation();
      else
         draw->setBitmapModulation( ColorI( 128, 128, 128 ) );

      const RectI &box = mCell[ mStateOn ? BoxOn : BoxOff ];

      Point2I at = offset;

      draw->drawBitmapSR( mTexture, at, box );
      at.x += box.extent.x;

      if ( span > 0 )
         draw->drawBitmapStretchSR( mTexture, RectI( at.x, at.y, span, barHeight ),
                                    mCell[ Rail ] );

      at.x += span;
      draw->drawBitmapSR( mTexture, at, mCell[ RightCap ] );

      if ( mActive && ( mMouseOver || isFirstResponder() ) )
         draw->drawBitmapSR( mTexture, offset, mCell[ Glow ] );
   }

   const char *text = getText();

   if ( text && text[ 0 ] && mProfile->mFont )
   {
      // Art that gave no rail leaves the label the whole height to sit in.
      if ( !barHeight )
         barHeight = getHeight();

      RectI clip( offset.x + barLeft, offset.y, span, barHeight );

      if ( clip.intersect( updateRect ) )
      {
         GFX->setClipRect( clip );

         GFont *font = mProfile->mFont;

         Point2I at( offset.x + barLeft,
                     offset.y + ( barHeight - font->getHeight() ) / 2 );

         const S32 width = font->getStrWidth( text );

         if ( span > width )
         {
            if ( mProfile->mAlignment == GuiControlProfile::RightJustify )
               at.x += span - width;
            else if ( mProfile->mAlignment == GuiControlProfile::CenterJustify )
               at.x += ( span - width ) / 2;
         }

         draw->setBitmapModulation( !mActive ? mProfile->mFontColorNA
                                    : mMouseOver ? mProfile->mFontColorHL
                                                 : mProfile->mFontColor );

         draw->drawText( font, at, text, mProfile->mFontColors );
      }
   }

   renderChildControls( offset, updateRect );
}
