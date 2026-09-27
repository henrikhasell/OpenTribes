// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLRADIOBUTTON_H_
#define _SHELLRADIOBUTTON_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

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
