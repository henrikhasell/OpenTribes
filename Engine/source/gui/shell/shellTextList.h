// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTEXTLIST_H_
#define _SHELLTEXTLIST_H_

#ifndef _GUITEXTLISTCTRL_H_
#include "gui/controls/guiTextListCtrl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// Tribes 2's list of rows.
///
/// A row is drawn from art rather than from a filled border: the profile's
/// stem carries `_rol` and `_act`, one bar the width of the list stretched
/// under the row the mouse is on and one under the row that is chosen.  There
/// is no grid in either of them -- the whole bitmap is the bar, which is why
/// nothing here slices.
class ShellTextList : public GuiTextListCtrl
{
   typedef GuiTextListCtrl Parent;

   ShellSkin mSkin;

protected:
   void onCellSelected( Point2I cell );

public:
   DECLARE_CONOBJECT( ShellTextList );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 list whose rows highlight with art." );

   bool onWake();
   void onSleep();

   void onRenderCell( Point2I offset, Point2I cell, bool selected, bool mouseOver );
};

#endif // _SHELLTEXTLIST_H_
