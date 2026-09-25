// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLMENUCTRL_H_
#define _SHELLMENUCTRL_H_

#ifndef _GUIPOPUPCTRL_H_
#include "gui/controls/guiPopUpCtrl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// The LAUNCH menu at the left of Tribes 2's launch bar.
///
/// GuiPopUpMenuCtrl owns the list, the selection and the script interface.
/// What is different here is the closed control, which is not a bordered box
/// showing what is selected but a fixed button carrying a fixed label: the
/// menu says LAUNCH whatever was last picked from it.  The art is a family --
/// the profile's bitmapBase, plus "_rol" and "_act" -- rather than one sheet.
class ShellLaunchMenu : public GuiPopUpMenuCtrl
{
   typedef GuiPopUpMenuCtrl Parent;

   ShellSkin mSkin;

   /// The label, kept apart from GuiTextCtrl's text because the popup writes
   /// what is selected into that and clear() empties it.  Tribes 2's own
   /// ShellLaunchMenu is not a popup at all, so nothing there touches its
   /// text; this is the same label by another route.
   StringTableEntry mLabel;

   StringTableEntry mButtonFontType;
   S32 mButtonFontSize;
   ColorI mButtonFontColor;
   ColorI mButtonFontColorHL;

   Resource< GFont > mButtonFont;

   /// The label's font, which is the profile's until buttonFontType names
   /// another.
   GFont *buttonFont();

public:
   ShellLaunchMenu();

   DECLARE_CONOBJECT( ShellLaunchMenu );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The LAUNCH menu on Tribes 2's launch bar." );

   static void initPersistFields();

   bool onAdd();
   bool onWake();
   void onRender( Point2I offset, const RectI &updateRect );

   /// A row that divides the menu rather than being picked from it.
   void addSeparator();
};

#endif // _SHELLMENUCTRL_H_
