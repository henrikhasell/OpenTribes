// HH: Added for Tribes 2 compatibility.
#ifndef _GUICOMMANDERMAPPOPUPMENU_H_
#define _GUICOMMANDERMAPPOPUPMENU_H_

#ifndef _GUIPOPUPCTRL_H_
#include "gui/controls/guiPopUpCtrl.h"
#endif

/// The orders the commander map offers for what was right-clicked: a list
/// that opens where the mouse is, each order's hotkey picked out in the
/// profile's second colour.  The control itself takes no room and draws
/// nothing.
class GuiCommanderMapPopupMenu : public GuiPopUpMenuCtrl
{
   typedef GuiPopUpMenuCtrl Parent;

public:
   GuiCommanderMapPopupMenu();

   void addChildren() override;
   void onAction() override;
   void onRender(Point2I offset, const RectI &updateRect);

   DECLARE_CONOBJECT(GuiCommanderMapPopupMenu);
};

#endif
