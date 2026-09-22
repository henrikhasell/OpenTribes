// HH: imported from TGE
// (only the two classes this file was brought over for -- the tabs, the radios
// and the tab group are still to come)

#ifndef _SHELLBUTTONCTRL_H_
#define _SHELLBUTTONCTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _SHELLSTATEBITMAP_H_
#include "gui/shell/shellStateBitmap.h"
#endif

/// @file
/// The things in Tribes 2's shell you press.
///
/// All of them draw the same way -- one image holding every state, stretched
/// between two fixed caps, with the label centred over it -- so they differ in
/// which state they pick and in what pressing them means.  GuiButtonBaseCtrl
/// already owns the second half of that, so these are rendering classes.

//-----------------------------------------------------------------------------
/// The shell's ordinary button.
class ShellBitmapButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

protected:
   ShellStateBitmap mArt;

   bool mSimpleStyle;
   S32  mDirection;

   /// Which band of the art the control's current state calls for.
   virtual S32 artState();

public:
   ShellBitmapButton();

   DECLARE_CONOBJECT(ShellBitmapButton);
   static void initPersistFields();

   bool onWake() override;
   void onRender(Point2I offset, const RectI &updateRect) override;
};

//-----------------------------------------------------------------------------
/// A button that stays down.
class ShellToggleButton : public ShellBitmapButton
{
   typedef ShellBitmapButton Parent;

protected:
   S32 artState() override;

public:
   ShellToggleButton();

   DECLARE_CONOBJECT(ShellToggleButton);
};

#endif // _SHELLBUTTONCTRL_H_
