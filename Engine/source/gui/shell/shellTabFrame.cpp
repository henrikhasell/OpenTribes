#include "platform/platform.h"
#include "gui/shell/shellTabFrame.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( ShellTabFrame );

ConsoleDocClass( ShellTabFrame,
   "@brief The panel a Tribes 2 tab bar stands on.\n\n"

   "Two rails with chamfered outer corners and a line stretched between them.  "
   "The rails fade down the panel, which is what joins the open tab to what it "
   "opened.  setAltColor swaps the whole thing to the second palette its art "
   "comes in.\n\n"

   "@ingroup GuiShell"
);

ShellTabFrame::ShellTabFrame()
{
   mIsVertical = false;
   mUseCloseButton = false;
   mEdgeInset = 0;
   mUseAltColor = false;

   setExtent( 200, 100 );
}

void ShellTabFrame::initPersistFields()
{
   addField( "isVertical", TypeBool, Offset( mIsVertical, ShellTabFrame ),
      "Rails across the top and bottom rather than down the sides." );
   addField( "useCloseButton", TypeBool, Offset( mUseCloseButton, ShellTabFrame ),
      "Draw the close button in the corner of the panel." );
   addField( "edgeInset", TypeS32, Offset( mEdgeInset, ShellTabFrame ),
      "How far the line between the rails sits inside them." );

   Parent::initPersistFields();
}

bool ShellTabFrame::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mSkin.setBase( mProfile->mBitmapBase );
   return true;
}

void ShellTabFrame::setAltColor( bool alt )
{
   if ( alt == mUseAltColor )
      return;

   mUseAltColor = alt;
   setUpdate();
}

GFXTexHandle ShellTabFrame::part( const char *name )
{
   if ( !mUseAltColor )
      return mSkin.get( name );

   char suffix[ 32 ];
   dSprintf( suffix, sizeof( suffix ), "%sa", name );

   return mSkin.get( suffix );
}

void ShellTabFrame::onRender( Point2I offset, const RectI &updateRect )
{
   mSkin.setBase( mProfile->mBitmapBase );

   GFXTexHandle rail = part( "grad" );
   GFXTexHandle line = part( "gradedge" );

   if ( rail && line )
   {
      GFXDrawUtil *draw = GFX->getDrawUtil();
      draw->clearBitmapModulation();

      const S32 railWidth = S32( rail->getBitmapWidth() );
      const S32 railHeight = S32( rail->getBitmapHeight() );

      Point2I at = offset;

      if ( mIsVertical )
      {
         const S32 span = getHeight() - railHeight * 2;

         draw->drawBitmap( rail, at );
         at.y += railHeight;

         if ( span > 0 )
            draw->drawBitmapStretch( line,
               RectI( at.x, at.y, S32( line->getBitmapWidth() ), span ) );

         at.y += span;

         draw->drawBitmap( rail, at, GFXBitmapFlip_Y );
      }
      else
      {
         const S32 span = getWidth() - railWidth * 2;

         draw->drawBitmap( rail, at );
         at.x += railWidth;
         at.y += mEdgeInset;

         if ( span > 0 )
            draw->drawBitmapStretch( line,
               RectI( at.x, at.y, span, S32( line->getBitmapHeight() ) ) );

         at.x += span;
         at.y -= mEdgeInset;

         draw->drawBitmap( rail, at, GFXBitmapFlip_X );
      }
   }

   renderChildControls( offset, updateRect );
}

DefineEngineMethod( ShellTabFrame, setAltColor, void, ( bool alt ), ,
   "@brief Draw the panel in its second palette.\n\n"

   "Which one that is comes from the art rather than from a colour: the "
   "profile's stem carries a second copy of every piece.\n\n"

   "@ingroup GuiShell" )
{
   object->setAltColor( alt );
}
