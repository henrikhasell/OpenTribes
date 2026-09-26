// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLPOPUPMENU_H_
#define _SHELLPOPUPMENU_H_

#ifndef _GUIPOPUPCTRL_H_
#include "gui/controls/guiPopUpCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellPopupMenu : public GuiPopUpMenuCtrl
{
   typedef GuiPopUpMenuCtrl Parent;

   enum
   {
      Resting = 0,
      Highlight = 2,

      StateCount = 3,
   };

   enum
   {
      LeftCap = 0,
      Middle = 1,
      RightCap = 2,
   };

   StringTableEntry mButtonBitmap;
   StringTableEntry mRolloverBarBitmap;
   StringTableEntry mSelectedBarBitmap;

   bool mNoButtonStyle;

   GFXTexHandle mTexture;
   RectI mCell[ 3 * StateCount ];
   bool mSliced;

   const RectI &cell( S32 piece, S32 state ) const
   {
      return mCell[ piece * StateCount + state ];
   }

public:
   ShellPopupMenu();

   DECLARE_CONOBJECT( ShellPopupMenu );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell drop-down." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void addChildren() override;
   void onAction() override;

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLPOPUPMENU_H_
