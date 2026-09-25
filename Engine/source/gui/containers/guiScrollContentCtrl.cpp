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
   mFitting = false;

   // GuiControl's 64x64 would clip the contents into a corner on the frame
   // before the first fitToParent().
   setExtent( 100, 100 );
}

void GuiScrollContentCtrl::fitToParent()
{
   // Growing to fit the contents resizes them in turn -- a .gui gives the
   // text control vertSizing "height", so it follows this control's height
   // just as this control follows its text's.  Each answers the other and
   // neither settles, which overflows the stack rather than converging.  The
   // children resizing is part of this pass, so it must not start another.
   if ( mFitting )
      return;

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

   if ( extent == getExtent() )
      return;

   // A .gui gives the contents vertSizing "height", so resizing this control
   // stretches them to match -- which would overwrite the height their own
   // reflow just worked out and leave nothing to measure next time.  Their
   // width should follow ours; their height is theirs.  So put the heights
   // back afterwards, and this grows and shrinks with the contents instead of
   // ratcheting upwards.
   Vector< S32 > heights;

   for ( iterator i = begin(); i != end(); i++ )
   {
      GuiControl *child = dynamic_cast< GuiControl* >( *i );
      heights.push_back( child ? child->getHeight() : 0 );
   }

   mFitting = true;

   resize( getPosition(), extent );

   U32 at = 0;

   for ( iterator i = begin(); i != end(); i++, at++ )
   {
      GuiControl *child = dynamic_cast< GuiControl* >( *i );

      if ( child && heights[ at ] && child->getHeight() != heights[ at ] )
         child->resize( child->getPosition(),
                        Point2I( child->getWidth(), heights[ at ] ) );
   }

   mFitting = false;

   // The scroll control works its bar out from how tall this is, so it has to
   // be told the answer changed.
   if ( scroll )
      scroll->computeSizes();
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
