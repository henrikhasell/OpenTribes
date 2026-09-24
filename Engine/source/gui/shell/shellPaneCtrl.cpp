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

   renderChildControls( offset, updateRect );
}

DefineEngineMethod( ShellPaneCtrl, setTitle, void, ( const char *title ), ,
   "@brief Change the name shown in the title bar.\n\n@ingroup GuiShell" )
{
   object->setText( title );
}
