#include "platform/platform.h"
#include "gui/shell/shellTextList.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( ShellTextList );

ConsoleDocClass( ShellTextList,
   "@brief A Tribes 2 list whose rows highlight with art.\n\n"

   "The profile's stem carries two bars -- _rol for the row under the mouse "
   "and _act for the row that is chosen -- each stretched across the cell.  "
   "An inactive row gets neither, and its text is drawn in fontColorNA.\n\n"

   "@ingroup GuiShell"
);

bool ShellTextList::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mSkin.setBase( mProfile->mBitmapBase );
   return true;
}

void ShellTextList::onSleep()
{
   Parent::onSleep();
   mSkin.clear();
}

void ShellTextList::onCellSelected( Point2I cell )
{
   Parent::onCellSelected( cell );
   setUpdate();
}

void ShellTextList::onRenderCell( Point2I offset, Point2I cell, bool selected, bool mouseOver )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   const Entry &entry = mList[ cell.y ];

   if ( entry.active )
   {
      GFXTexHandle bar;
      if ( selected )
         bar = mSkin.get( "_act" );
      else if ( mouseOver )
         bar = mSkin.get( "_rol" );

      if ( bar )
      {
         draw->clearBitmapModulation();
         draw->drawBitmapStretch( bar, RectI( offset, mCellSize ) );
      }
   }

   ColorI color;
   if ( !entry.active )
      color = mProfile->mFontColorNA;
   else if ( selected )
      color = mProfile->mFontColorSEL;
   else if ( mouseOver )
      color = mProfile->mFontColorHL;
   else
      color = mProfile->mFontColor;

   const char *text = entry.text;

   for ( U32 index = 0; index < mColumnOffsets.size(); index++ )
   {
      const char *nextCol = dStrchr( text, '\t' );

      if ( mColumnOffsets[ index ] >= 0 )
      {
         const U32 length = nextCol ? U32( nextCol - text ) : dStrlen( text );

         draw->setBitmapModulation( color );
         draw->drawTextN( mFont, Point2I( offset.x + 4 + mColumnOffsets[ index ], offset.y ),
                          text, length, mProfile->mFontColors );
      }

      if ( !nextCol )
         break;

      text = nextCol + 1;
   }
}
