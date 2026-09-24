// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLPANECTRL_H_
#define _SHELLPANECTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// The frame Tribes 2's shell puts around a screen.
///
/// A title bar across the top with the screen's name centred in it, side rails
/// down the edges, and a lit-from-above interior the screen's own controls sit
/// in.  It is the root of every shell layout: GameGui.gui is one of these with
/// everything else inside it.
///
/// Its art is a family the profile names by its stem -- "gui/shll" for the
/// shell's frames -- which the control completes with the five suffixes it
/// knows.  A stem that resolves to none of them draws nothing rather than
/// drawing wrong.
class ShellPaneCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   StringTableEntry mText;
   bool mNoTitleBar;

   ShellSkin mSkin;

   void updateSkin();

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
