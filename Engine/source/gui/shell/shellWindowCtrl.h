// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLWINDOWCTRL_H_
#define _SHELLWINDOWCTRL_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellWindowCtrl : public GuiTextCtrl
{
   typedef GuiTextCtrl Parent;

   enum
   {
      TitleCap = 0,
      TitleMiddle = 1,
   };

   GFXTexHandle mTitleTexture;
   GFXTexHandle mCloseTexture;
   GFXTexHandle mCornerTexture;
   GFXTexHandle mFillTexture;
   GFXTexHandle mGradeTexture;

   RectI mTitleCell[ 2 ];
   RectI mCloseCell[ 3 ];

   StringTableEntry mWindowBase;

   bool mSliced;

   S32 mTitleHeight;
   S32 mMinBodyHeight;

public:
   ShellWindowCtrl();

   DECLARE_CONOBJECT( ShellWindowCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell window." );

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLWINDOWCTRL_H_
