// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTABBUTTON_H_
#define _SHELLTABBUTTON_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellTabButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

   enum
   {
      Resting = 0,
      Selected = 1,
      Highlight = 2,

      StateCount = 3,
   };

   enum
   {
      LeftCap = 0,
      Middle = 1,
      RightCap = 2,

      PieceCount = 3,
   };

   GFXTexHandle mTexture;
   RectI mCell[ PieceCount * StateCount ];
   bool mSliced;

   const RectI &cell( S32 piece, S32 state ) const
   {
      return mCell[ piece * StateCount + state ];
   }

   S32 artState();

public:
   ShellTabButton();

   DECLARE_CONOBJECT( ShellTabButton );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "One tab of a Tribes 2 shell tab strip." );

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLTABBUTTON_H_
