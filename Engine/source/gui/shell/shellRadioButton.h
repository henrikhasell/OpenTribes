// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLRADIOBUTTON_H_
#define _SHELLRADIOBUTTON_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

/// Tribes 2's radio button.
///
/// The art is one column of five cells rather than a grid of states: the box
/// unlit, the box lit, a rail, a right cap, and a glow over the whole of the
/// box.  The first four are laid left to right with only the rail stretched,
/// so the label sits on a bar as wide as the control; the glow is drawn back
/// over the box when the button has the mouse or the focus.
///
/// The two caps are what the button must be to draw at all, so waking one
/// sets its minimum extent from them -- 26 by 27 for stock art, which is what
/// every Tribes 2 .gui gives a radio button.
class ShellRadioButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

   enum
   {
      BoxOff = 0,
      BoxOn = 1,
      Rail = 2,
      RightCap = 3,
      Glow = 4,

      CellCount = 5,
   };

   GFXTexHandle mTexture;
   RectI mCell[ CellCount ];
   bool mSliced;

public:
   ShellRadioButton();

   DECLARE_CONOBJECT( ShellRadioButton );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 radio button with a label on a bar." );

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLRADIOBUTTON_H_
