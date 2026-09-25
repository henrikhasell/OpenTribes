// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTEXTEDITCTRL_H_
#define _SHELLTEXTEDITCTRL_H_

#ifndef _GUITEXTEDITCTRL_H_
#include "gui/controls/guiTextEditCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellTextEditCtrl : public GuiTextEditCtrl
{
   typedef GuiTextEditCtrl Parent;

   enum
   {
      Resting = 0,
      Focused = 1,

      StateCount = 2,
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

   Point2I mGlowOffset;

   const RectI &cell( S32 piece, S32 state ) const
   {
      return mCell[ piece * StateCount + state ];
   }

public:
   ShellTextEditCtrl();

   DECLARE_CONOBJECT( ShellTextEditCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell entry field." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLTEXTEDITCTRL_H_
