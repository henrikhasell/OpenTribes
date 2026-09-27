// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLPANECTRL_H_
#define _SHELLPANECTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

class ShellPaneCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   StringTableEntry mText;
   bool mNoTitleBar;

   ShellSkin mSkin;

   RectI mTitleCell[ 2 ];
   bool mHaveTitleCells;

   void updateSkin();

   S32 titleHeight();

   void drawTitleBar( const Point2I &offset );
   void drawFrame( const Point2I &offset );

public:
   ShellPaneCtrl();

   DECLARE_CONOBJECT( ShellPaneCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The frame a Tribes 2 shell screen sits in." );

   static void initPersistFields();

   bool onWake();
   void onRender( Point2I offset, const RectI &updateRect );

   void setText( const char *text );
   const char *getText() const { return mText; }
};

#endif // _SHELLPANECTRL_H_
