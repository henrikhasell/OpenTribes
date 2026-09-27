// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLMENUCTRL_H_
#define _SHELLMENUCTRL_H_

#ifndef _GUIPOPUPCTRL_H_
#include "gui/controls/guiPopUpCtrl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

class ShellLaunchMenu : public GuiPopUpMenuCtrl
{
   typedef GuiPopUpMenuCtrl Parent;

   ShellSkin mSkin;

   StringTableEntry mLabel;

   StringTableEntry mButtonFontType;
   S32 mButtonFontSize;
   ColorI mButtonFontColor;
   ColorI mButtonFontColorHL;

   Resource< GFont > mButtonFont;

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

   void addSeparator();
};

#endif // _SHELLMENUCTRL_H_
