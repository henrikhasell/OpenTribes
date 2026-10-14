#include "platform/platform.h"
#include "gui/containers/guiScrollContentCtrl.h"

#include "console/engineAPI.h"
#include "gui/containers/guiScrollCtrl.h"

IMPLEMENT_CONOBJECT( GuiScrollContentCtrl );

ConsoleDocClass( GuiScrollContentCtrl,
   "@brief The control a Tribes 2 scroll control holds its contents in.\n\n"

   "A fixed window the size of the scroll control's content rect.  Moving or "
   "resizing it moves or resizes what it holds instead, so the contents scroll "
   "behind a window that stays put.\n\n"

   "@ingroup GuiContainers"
);

GuiScrollContentCtrl::GuiScrollContentCtrl()
{
}

GuiControl *GuiScrollContentCtrl::getContents()
{
   return size() ? dynamic_cast< GuiControl* >( front() ) : NULL;
}

void GuiScrollContentCtrl::setWindow( const Point2I &position, const Point2I &extent )
{
   mNotifyChildrenResized = false;

   Parent::resize( position, extent );

   mNotifyChildrenResized = true;
}

void GuiScrollContentCtrl::contentsMoved()
{
   // Moving without resizing does not reach a parent, so the scroll control
   // would go on believing the contents are where it last put them.
   if ( GuiScrollCtrl *scroll = dynamic_cast< GuiScrollCtrl* >( getParent() ) )
      scroll->computeSizes();
}

bool GuiScrollContentCtrl::resize( const Point2I &newPosition, const Point2I &newExtent )
{
   GuiControl *contents = getContents();

   if ( !contents )
      return Parent::resize( newPosition, newExtent );

   const RectI bounds = getBounds();

   Point2I delta = newPosition - bounds.point;

   if ( !delta.isZero() )
      contents->setPosition( contents->getPosition() + delta );

   if ( newExtent != bounds.extent )
      for ( iterator i = begin(); i != end(); i++ )
         static_cast< GuiControl* >( *i )->parentResized( bounds,
            RectI( bounds.point, newExtent ) );

   return false;
}

bool GuiScrollContentCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   if ( GuiScrollCtrl *scroll = dynamic_cast< GuiScrollCtrl* >( getParent() ) )
      scroll->computeSizes();

   return true;
}

void GuiScrollContentCtrl::childResized( GuiControl *child )
{
   Parent::childResized( child );

   // What is scrolled sits inside this control, so the scroll control cannot
   // see it change on its own.  Go through childResized rather than straight
   // to computeSizes, or this walks back in while it is already resizing.
   if ( GuiScrollCtrl *scroll = dynamic_cast< GuiScrollCtrl* >( getParent() ) )
      scroll->childResized( this );
}

GuiScrollCtrl *findScrollAbove( GuiControl *ctrl, Point2I &offset )
{
   offset.set( 0, 0 );

   GuiControl *child = ctrl;

   for ( GuiControl *walk = ctrl->getParent(); walk; walk = walk->getParent() )
   {
      if ( GuiScrollContentCtrl *window = dynamic_cast< GuiScrollContentCtrl* >( walk ) )
         return dynamic_cast< GuiScrollCtrl* >( window->getParent() );

      if ( GuiScrollCtrl *scroll = dynamic_cast< GuiScrollCtrl* >( walk ) )
         return scroll;

      offset += child->getPosition();
      child = walk;
   }

   return NULL;
}
