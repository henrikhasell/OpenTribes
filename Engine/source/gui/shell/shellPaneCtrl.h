// HH: imported from TGE
// (only ShellPaneCtrl -- the window, the dialog frame and the field control
// that share the file there are still to come)

#ifndef _SHELLPANECTRL_H_
#define _SHELLPANECTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// @file
/// The frames Tribes 2's shell puts around everything.
///
/// A pane is a teal border with a tab in its top-left corner carrying a title,
/// and it is what every screen in the game sits inside.  The border is built
/// out of a family of images the profile names by its stem -- "gui/shll" for
/// the shell's own frames, "gui/dlg" for a dialog's smaller ones -- so the two
/// differ by one string and share every line of code below.

//-----------------------------------------------------------------------------
class ShellPaneCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   StringTableEntry mText;
   bool mNoTitleBar;

   ShellSkin mSkin;

   /// How tall the title tab is, which is the height of its own art.  Content
   /// starts below it, so it has to be known before anything is drawn.
   S32 titleHeight();

   /// The rectangle the frame encloses, in the same space as `bounds`.
   RectI contentRect(const RectI &bounds);

   void drawFrame(const RectI &bounds);
   void drawTitleTab(const RectI &bounds);

public:
   ShellPaneCtrl();

   DECLARE_CONOBJECT(ShellPaneCtrl);
   static void initPersistFields();

   bool onWake() override;
   void onRender(Point2I offset, const RectI &updateRect) override;

   void setText(const char *text);
   const char *getText() const { return mText; }
};

#endif // _SHELLPANECTRL_H_
