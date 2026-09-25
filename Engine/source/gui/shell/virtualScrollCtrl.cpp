#include "platform/platform.h"
#include "gui/shell/virtualScrollCtrl.h"

#include "console/console.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT( VirtualScrollContentCtrl );
IMPLEMENT_CONOBJECT( VirtualScrollCtrl );

ConsoleDocClass( VirtualScrollContentCtrl,
   "@brief The content control a VirtualScrollCtrl makes for itself.\n\n"

   "@ingroup GuiShell"
);

ConsoleDocClass( VirtualScrollCtrl,
   "@brief A Tribes 2 scroll control that makes its own content.\n\n"

   "Anything added to it is put inside that content control rather than "
   "beside it, so a .gui can list what scrolls as if it were a direct "
   "child.\n\n"

   "@ingroup GuiShell"
);

bool VirtualScrollCtrl::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   VirtualScrollContentCtrl *content = new VirtualScrollContentCtrl;

   if ( !content->registerObject() )
   {
      Con::errorf( "Failed to add virtual content control to VirtualScrollCtrl!" );
      delete content;
      return true;
   }

   addObject( content );
   return true;
}

void VirtualScrollCtrl::addObject( SimObject *object )
{
   VirtualScrollContentCtrl *content = dynamic_cast< VirtualScrollContentCtrl * >( object );

   if ( content )
   {
      if ( mContent )
         mContent->deleteObject();

      Parent::addObject( object );
      mContent = content;
      return;
   }

   if ( !mContent )
      return;

   mContent->addObject( object );
   computeSizes();
}
