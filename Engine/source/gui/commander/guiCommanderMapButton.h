// HH: Added for Tribes 2 compatibility.
#ifndef _GUICOMMANDERMAPBUTTON_H_
#define _GUICOMMANDERMAPBUTTON_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

/// A button of the commander map's bars: one bitmap cut four wide -- off,
/// on, then each again under the mouse -- answering only within mouseRegion.
class GuiCommanderMapButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

protected:
   enum { Off, On, OffLit, OnLit, NumCells };

   StringTableEntry mBitmapName;
   RectI mMouseRegion;
   GFXTexHandle mTexture;
   RectI mCells[NumCells];

   /// The cell for how the button stands now.
   virtual U32 currentCell() const;

public:
   GuiCommanderMapButton();

   static void initPersistFields();

   bool onWake();
   void onSleep();
   bool pointInControl(const Point2I &parentCoordPoint);
   void onRender(Point2I offset, const RectI &updateRect);

   DECLARE_CONOBJECT(GuiCommanderMapButton);
};

/// The same, standing on or off until it is pressed again.
class GuiCommanderMapCheckbox : public GuiCommanderMapButton
{
   typedef GuiCommanderMapButton Parent;

public:
   GuiCommanderMapCheckbox();

   DECLARE_CONOBJECT(GuiCommanderMapCheckbox);
};

#endif
