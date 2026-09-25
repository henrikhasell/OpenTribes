#include "platform/platform.h"
#include "gui/shell/shellMenuCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"

IMPLEMENT_CONOBJECT( ShellLaunchMenu );

ConsoleDocClass( ShellLaunchMenu,
   "@brief The LAUNCH menu at the left of Tribes 2's launch bar.\n\n"

   "A drop-down whose closed state is a fixed button with a fixed label, "
   "rather than a box showing the last thing picked.  Its art is the "
   "profile's bitmapBase and the two lit versions beside it.\n\n"

   "@ingroup GuiShell"
);

ShellLaunchMenu::ShellLaunchMenu()
{
   mLabel = NULL;
   mButtonFontType = NULL;
   mButtonFontSize = 0;
   // Transparent stands for "not given", so that a profile that has said what
   // colour its text is does not have to say it twice.
   mButtonFontColor.set( 0, 0, 0, 0 );
   mButtonFontColorHL.set( 0, 0, 0, 0 );
}

void ShellLaunchMenu::initPersistFields()
{
   addField( "buttonFontType", TypeString, Offset( mButtonFontType, ShellLaunchMenu ),
      "Font for the label on the closed menu." );
   addField( "buttonFontSize", TypeS32, Offset( mButtonFontSize, ShellLaunchMenu ),
      "Size of the label on the closed menu." );
   addField( "buttonFontColor", TypeColorI, Offset( mButtonFontColor, ShellLaunchMenu ),
      "Colour of the label on the closed menu." );
   addField( "buttonFontColorHL", TypeColorI, Offset( mButtonFontColorHL, ShellLaunchMenu ),
      "Colour of the label while the mouse is over the menu." );

   Parent::initPersistFields();
}

bool ShellLaunchMenu::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Before anything has been added to the menu, and so before the first
   // clear() writes over it.
   mLabel = StringTable->insert( mText ? mText : "", true );
   return true;
}

bool ShellLaunchMenu::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mSkin.setBase( mProfile->mBitmapBase );

   if ( !mButtonFontColor.alpha )
      mButtonFontColor = mProfile->mFontColor;
   if ( !mButtonFontColorHL.alpha )
      mButtonFontColorHL = mProfile->mFontColorHL;

   if ( mButtonFontType && mButtonFontType[ 0 ] && mButtonFontSize > 0 )
      mButtonFont = GFont::create( mButtonFontType, mButtonFontSize,
                                   Con::getVariable( "$GUI::fontCacheDirectory" ) );

   return true;
}

GFont *ShellLaunchMenu::buttonFont()
{
   return mButtonFont ? (GFont*)mButtonFont : (GFont*)mProfile->mFont;
}

void ShellLaunchMenu::addSeparator()
{
   // Tribes 2 draws a rule here.  Torque's popup has no row that cannot be
   // picked, so this is a row with an id no handler matches.
   addEntry( "", -1 );
}

void ShellLaunchMenu::onRender( Point2I offset, const RectI &updateRect )
{
   mSkin.setBase( mProfile->mBitmapBase );

   // Resting, under the mouse, and held open.  Each is a whole image rather
   // than a band of one sheet.
   const char *suffix = "";
   if ( mInAction )
      suffix = "_act";
   else if ( mMouseOver )
      suffix = "_rol";

   GFXTexHandle art = mSkin.get( suffix );
   if ( !art )
      art = mSkin.get( "" );

   GFXDrawUtil *draw = GFX->getDrawUtil();

   if ( art )
   {
      // At the size it was drawn, where the control starts.  The art and the
      // control are both 115x56 here, so stretching looked the same -- but the
      // art is what decides the size, not the layout.
      draw->clearBitmapModulation();
      draw->drawBitmap( art, offset );
   }

   GFont *font = buttonFont();

   if ( font && mLabel && mLabel[ 0 ] )
   {
      const ColorI color = mMouseOver ? mButtonFontColorHL : mButtonFontColor;

      const Point2I textOffset = offset + mProfile->mTextOffset;
      const Point2I textExtent = getExtent() - mProfile->mTextOffset;

      S32 x = textOffset.x;
      if ( mProfile->mAlignment == GuiControlProfile::CenterJustify )
         x += ( textExtent.x - font->getStrWidth( mLabel ) ) / 2;
      else if ( mProfile->mAlignment == GuiControlProfile::RightJustify )
         x += textExtent.x - font->getStrWidth( mLabel );

      draw->setBitmapModulation( color );
      draw->drawText( font, Point2I( x, textOffset.y ), mLabel );
      draw->clearBitmapModulation();
   }

   renderChildControls( offset, updateRect );
}

//-----------------------------------------------------------------------------
DefineEngineMethod( ShellLaunchMenu, add, void, ( S32 id, const char *text ), ,
   "@brief Put a row on the end of the menu.\n\n"

   "Tribes 2 gives the id first, where Torque's own popup gives the text "
   "first.\n\n"

   "@ingroup GuiShell" )
{
   object->addEntry( text, id );
}

DefineEngineMethod( ShellLaunchMenu, addSeparator, void, (), ,
   "@brief Put a divider on the end of the menu.\n\n@ingroup GuiShell" )
{
   object->addSeparator();
}
