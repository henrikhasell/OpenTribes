// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLMISCCTRL_H_
#define _SHELLMISCCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif

/// @file
/// The two controls PlayGui.gui needs that are neither shell nor HUD.
///
/// The file this pair comes from holds nine classes; the other seven -- a chat
/// tree, a vote panel, a channel list and a test control -- belong to screens
/// this port does not reach yet, and bringing them to satisfy these two would
/// be importing six hundred lines nothing asks for.  They are left where they
/// are rather than carried along.

//-----------------------------------------------------------------------------
/// A label that shows what a console expression evaluates to.
///
/// Tribes 2 puts whole expressions in these, not just variable names, which is
/// why this evaluates rather than looks up.
class GuiConsoleVariableCtrl : public GuiTextCtrl
{
   typedef GuiTextCtrl Parent;

   StringTableEntry mExpression;

public:
   GuiConsoleVariableCtrl();

   DECLARE_CONOBJECT(GuiConsoleVariableCtrl);
   static void initPersistFields();

   void onPreRender();
};

//-----------------------------------------------------------------------------
/// The container the vehicle dashboard is laid out in.
class GuiDashBoardCtrl : public GuiControl
{
   typedef GuiControl Parent;

   bool mWrap;

public:
   GuiDashBoardCtrl();

   DECLARE_CONOBJECT(GuiDashBoardCtrl);
   static void initPersistFields();

   bool resize(const Point2I &newPosition, const Point2I &newExtent) override;
};

#endif // _SHELLMISCCTRL_H_
