// HH: Added for Tribes 2 compatibility.
#ifndef _VIRTUALSCROLLCTRL_H_
#define _VIRTUALSCROLLCTRL_H_

#ifndef _GUISCROLLCONTENTCTRL_H_
#include "gui/containers/guiScrollContentCtrl.h"
#endif
#ifndef _SHELLSCROLLCTRL_H_
#include "gui/shell/shellScrollCtrl.h"
#endif

class VirtualScrollContentCtrl : public GuiScrollContentCtrl
{
   typedef GuiScrollContentCtrl Parent;

public:
   DECLARE_CONOBJECT( VirtualScrollContentCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The content control a VirtualScrollCtrl makes for itself." );
};

class VirtualScrollCtrl : public ShellScrollCtrl
{
   typedef ShellScrollCtrl Parent;

   SimObjectPtr< VirtualScrollContentCtrl > mContent;

public:
   DECLARE_CONOBJECT( VirtualScrollCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 scroll control that makes its own content." );

   bool onAdd();
   void addObject( SimObject *object );
};

#endif // _VIRTUALSCROLLCTRL_H_
