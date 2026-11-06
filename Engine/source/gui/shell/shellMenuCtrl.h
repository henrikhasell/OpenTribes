// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLMENUCTRL_H_
#define _SHELLMENUCTRL_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

class ShellLaunchList;
class ShellVWipeCtrl;

class ShellLaunchMenu : public GuiTextCtrl
{
   typedef GuiTextCtrl Parent;

public:
   struct Entry
   {
      StringTableEntry text;
      S32 id;
      bool separator;
   };

private:
   ShellSkin mSkin;

   Vector< Entry > mEntries;

   StringTableEntry mButtonFontType;
   S32 mButtonFontSize;
   ColorI mButtonFontColor;
   ColorI mButtonFontColorHL;

   StringTableEntry mRolloverBarBitmap;
   StringTableEntry mSelectedBarBitmap;
   StringTableEntry mSeparatorBitmap;
   StringTableEntry mVWipeFillBitmap;
   StringTableEntry mVWipeBarBitmap;

   Resource< GFont > mButtonFont;

   bool mOpen;
   bool mOver;

   GuiControl *mBackground;
   ShellVWipeCtrl *mWipe;
   ShellLaunchList *mList;

   GFont *buttonFont();
   GFXTexHandle art();

public:
   ShellLaunchMenu();

   DECLARE_CONOBJECT( ShellLaunchMenu );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The LAUNCH menu on Tribes 2's launch bar." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );

   void onMouseDown( const GuiEvent &event );
   void onMouseEnter( const GuiEvent &event );
   void onMouseLeave( const GuiEvent &event );
   bool onKeyDown( const GuiEvent &event );

   void clear();
   void addEntry( const char *text, S32 id );
   void addSeparator();

   const Vector< Entry > &entries() const { return mEntries; }

   /// Draws the pressed button with its label, over the dimmed screen.
   void renderPressed( Point2I offset );

   void open();
   void onWipeDone();
   void close( S32 id );
};

#endif // _SHELLMENUCTRL_H_
