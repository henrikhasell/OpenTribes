// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLDLGFRAME_H_
#define _SHELLDLGFRAME_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellDlgFrame : public GuiTextCtrl
{
   typedef GuiTextCtrl Parent;

   enum
   {
      TitleLeft = 0, TitleMiddle, TitleRight,
      BodyLeft, BodyMiddle, BodyRight,
      FootLeft, FootMiddle, FootRight,

      CellCount,
   };

   GFXTexHandle mTexture;
   RectI mCell[ CellCount ];
   bool mSliced;

public:
   ShellDlgFrame();

   DECLARE_CONOBJECT( ShellDlgFrame );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The frame a Tribes 2 shell dialog is drawn in." );

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLDLGFRAME_H_
