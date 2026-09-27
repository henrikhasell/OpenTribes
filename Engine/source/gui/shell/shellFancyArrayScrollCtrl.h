// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLFANCYARRAYSCROLLCTRL_H_
#define _SHELLFANCYARRAYSCROLLCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellFancyArray;
class VirtualScrollCtrl;

class ShellFancyArrayScrollCtrl : public GuiControl
{
   typedef GuiControl Parent;

   StringTableEntry mVertSpacerBitmap;
   StringTableEntry mHorzSpacerBitmap;

   bool mFixedHorizontal;

   GFXTexHandle mVertSpacer;
   GFXTexHandle mHorzSpacer;

   SimObjectPtr< ShellFancyArray > mArray;
   SimObjectPtr< VirtualScrollCtrl > mScroll;

   void arrange();

public:
   ShellFancyArrayScrollCtrl();

   DECLARE_CONOBJECT( ShellFancyArrayScrollCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The scroll a Tribes 2 browser list sits in." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void addObject( SimObject *object );
   void removeObject( SimObject *object );

   bool resize( const Point2I &position, const Point2I &extent );
   void childResized( GuiControl *child );

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLFANCYARRAYSCROLLCTRL_H_
