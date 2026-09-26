#include "platform/platform.h"
#include "gui/shell/shellFancyArrayScrollCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/shell/shellFancyArray.h"
#include "gui/shell/virtualScrollCtrl.h"

IMPLEMENT_CONOBJECT( ShellFancyArrayScrollCtrl );

ConsoleDocClass( ShellFancyArrayScrollCtrl,
   "@brief The scroll a Tribes 2 browser list sits in.\n\n"

   "Holds a VirtualScrollCtrl and a ShellFancyArray side by side and keeps "
   "them in step: the bar is sized from the array's rows and the array is "
   "scrolled by the bar.  The spacers fill the corner the two leave.\n\n"

   "@ingroup GuiShell"
);

ShellFancyArrayScrollCtrl::ShellFancyArrayScrollCtrl()
{
   mVertSpacerBitmap = StringTable->insert( "gui/shll_vertspacer" );
   mHorzSpacerBitmap = StringTable->insert( "gui/shll_horzspacer" );

   mFixedHorizontal = false;
}

void ShellFancyArrayScrollCtrl::initPersistFields()
{
   addField( "fixedHorizontal", TypeBool,
      Offset( mFixedHorizontal, ShellFancyArrayScrollCtrl ),
      "Never scroll across, however wide the columns come out." );
   addField( "vertSpacerBitmap", TypeString,
      Offset( mVertSpacerBitmap, ShellFancyArrayScrollCtrl ),
      "Art filling the strip beside the bar." );
   addField( "horzSpacerBitmap", TypeString,
      Offset( mHorzSpacerBitmap, ShellFancyArrayScrollCtrl ),
      "Art filling the strip under the array." );

   Parent::initPersistFields();
}

bool ShellFancyArrayScrollCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   if ( mVertSpacerBitmap && mVertSpacerBitmap[ 0 ] )
      mVertSpacer.set( mVertSpacerBitmap, &GFXDefaultPersistentProfile,
                       avar( "%s() - vert", __FUNCTION__ ) );

   if ( mHorzSpacerBitmap && mHorzSpacerBitmap[ 0 ] )
      mHorzSpacer.set( mHorzSpacerBitmap, &GFXDefaultPersistentProfile,
                       avar( "%s() - horz", __FUNCTION__ ) );

   arrange();

   return true;
}

void ShellFancyArrayScrollCtrl::onSleep()
{
   Parent::onSleep();

   mVertSpacer = NULL;
   mHorzSpacer = NULL;
}

void ShellFancyArrayScrollCtrl::addObject( SimObject *object )
{
   Parent::addObject( object );

   if ( ShellFancyArray *array = dynamic_cast< ShellFancyArray * >( object ) )
   {
      mArray = array;
      mArrayMargin = getExtent() - array->getExtent();

      if ( mFixedHorizontal )
         array->setFitColumns( true );
   }
   else if ( VirtualScrollCtrl *scroll = dynamic_cast< VirtualScrollCtrl * >( object ) )
   {
      mScroll = scroll;
      mScrollPos = scroll->getPosition();
      mScrollMargin = getExtent() - scroll->getPosition() - scroll->getExtent();
   }

   arrange();
}

void ShellFancyArrayScrollCtrl::removeObject( SimObject *object )
{
   if ( object == ( SimObject * )mArray )
      mArray = NULL;
   else if ( object == ( SimObject * )mScroll )
      mScroll = NULL;

   Parent::removeObject( object );
}

void ShellFancyArrayScrollCtrl::arrange()
{
   if ( !mArray )
      return;

   if ( !mScroll )
      return;

   mScroll->resize( mScrollPos, getExtent() - mScrollPos - mScrollMargin );

   mArray->resize( mArray->getPosition(), getExtent() - mArrayMargin );

   if ( GuiControl *content = mScroll->size() ? dynamic_cast< GuiControl * >( ( *mScroll )[ 0 ] ) : NULL )
   {
      const S32 wanted = mArray->getHeaderHeight() + mArray->contentHeight();

      if ( content->getHeight() != wanted )
         content->resize( content->getPosition(), Point2I( content->getWidth(), wanted ) );
   }

   mArray->setScrollOffset( mScroll->getChildRelPos().y );
}

bool ShellFancyArrayScrollCtrl::resize( const Point2I &position, const Point2I &extent )
{
   const bool changed = Parent::resize( position, extent );
   arrange();
   return changed;
}

void ShellFancyArrayScrollCtrl::childResized( GuiControl *child )
{
   Parent::childResized( child );
   arrange();
}

void ShellFancyArrayScrollCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   if ( mVertSpacer && mArray )
   {
      draw->drawBitmapStretch( mVertSpacer,
         RectI( offset.x + mArray->getWidth(),
                offset.y + mArray->getGlowOffset(),
                S32( mVertSpacer->getBitmapWidth() ),
                mArray->headerSpan() - 1 ) );
   }

   if ( mHorzSpacer && mArray && !mFixedHorizontal )
   {
      draw->drawBitmapStretch( mHorzSpacer,
         RectI( offset.x + mArray->getGlowOffset(),
                offset.y + mArray->getHeight(),
                mArray->pinnedWidth() - 1,
                S32( mHorzSpacer->getBitmapHeight() ) ) );
   }

   renderChildControls( offset, updateRect );
}
