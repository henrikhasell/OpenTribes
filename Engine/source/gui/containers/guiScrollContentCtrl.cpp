#include "platform/platform.h"
#include "gui/containers/guiScrollContentCtrl.h"

#include "console/engineAPI.h"
#include "gui/containers/guiScrollCtrl.h"

IMPLEMENT_CONOBJECT( GuiScrollContentCtrl );

ConsoleDocClass( GuiScrollContentCtrl,
   "@brief The control a Tribes 2 scroll control holds its contents in.\n\n"

   "Fills the scroll control and grows past it where the contents are taller, "
   "so that there is something to scroll.\n\n"

   "@ingroup GuiContainers"
);

GuiScrollContentCtrl::GuiScrollContentCtrl()
{
   // GuiControl's 64x64 would clip the contents into a corner on the frame
   // before the first fitToParent().
   setExtent( 100, 100 );
}

void GuiScrollContentCtrl::fitToParent()
{
   GuiControl *parent = getParent();

   if ( !parent )
      return;

   GuiScrollCtrl *scroll = dynamic_cast< GuiScrollCtrl* >( parent );
   Point2I extent = scroll ? scroll->getContentExtent() : parent->getExtent();

   for ( iterator i = begin(); i != end(); i++ )
   {
      GuiControl *child = dynamic_cast< GuiControl* >( *i );

      if ( child )
         extent.y = getMax( extent.y, child->getPosition().y + child->getExtent().y );
   }

   if ( extent != getExtent() )
      resize( getPosition(), extent );
}

bool GuiScrollContentCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   fitToParent();
   return true;
}

void GuiScrollContentCtrl::parentResized( const RectI &oldParentRect, const RectI &newParentRect )
{
   Parent::parentResized( oldParentRect, newParentRect );
   fitToParent();
}

void GuiScrollContentCtrl::childResized( GuiControl *child )
{
   Parent::childResized( child );
   fitToParent();
}
