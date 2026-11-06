// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/guiCommanderMapPopupMenu.h"

#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/containers/guiScrollCtrl.h"
#include "gui/core/guiCanvas.h"

IMPLEMENT_CONOBJECT(GuiCommanderMapPopupMenu);

ConsoleDocClass( GuiCommanderMapPopupMenu,
   "@brief The commander map's list of orders.\n\n@internal" );

namespace
{
   const char *sBackdrop = "commander/gui/cmd_gradient";
   const ColorI sHilight(44, 168, 219, 100);
   enum { TextMargin = 4 };

   /// Text with its colour codes left out, which is what is measured.
   S32 shownWidth(GFont *font, const char *text)
   {
      char plain[256];
      U32 n = 0;
      for (const char *c = text; *c && n < sizeof(plain) - 1; c++)
         if (U8(*c) >= 0x20)
            plain[n++] = *c;
      plain[n] = 0;
      return font->getStrWidth(plain);
   }

   class CommanderPopupList : public GuiPopupTextListCtrl
   {
      typedef GuiPopupTextListCtrl Parent;

   public:
      CommanderPopupList(GuiPopUpMenuCtrl *menu) : GuiPopupTextListCtrl(menu) {}

      /// Only the rows: the gradient behind them is the scroll's.
      void onRender(Point2I offset, const RectI &updateRect) override
      {
         for (S32 i = 0; i < mList.size(); i++)
            onRenderCell(Point2I(offset.x, offset.y + i * mCellSize.y), Point2I(0, i),
                         i == mSelectedCell.y, i == mMouseOverCell.y);
      }

      void onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver) override
      {
         Point2I size;
         getCellSize(size);
         GFXDrawUtil *draw = GFX->getDrawUtil();
         if (mouseOver)
            draw->drawRectFill(RectI(offset, size), sHilight);

         draw->setBitmapModulation(mProfile->mFontColors[0]);
         draw->drawText(mFont, Point2I(offset.x + TextMargin, offset.y), mList[cell.y].text,
                        mProfile->mFontColors);
         draw->clearBitmapModulation();
      }
   };

   /// The gradient behind the list and the profile's border round it.
   class CommanderPopupScroll : public GuiScrollCtrl
   {
      typedef GuiScrollCtrl Parent;
      GFXTexHandle mBackdrop;

   public:
      void onRender(Point2I offset, const RectI &updateRect)
      {
         GFXDrawUtil *draw = GFX->getDrawUtil();
         const RectI box(offset, getExtent());

         if (!mBackdrop)
            mBackdrop.set(sBackdrop, &GFXDefaultGUIProfile, avar("%s() - backdrop", __FUNCTION__));

         if (mProfile->mOpaque)
            draw->drawRectFill(box, mProfile->mFillColor);
         if (mBackdrop)
         {
            draw->clearBitmapModulation();
            draw->drawBitmapStretch(mBackdrop, box);
         }

         renderChildControls(offset, updateRect);

         if (mProfile->mBorder)
            draw->drawRect(box, mProfile->mBorderColor);
      }
   };
}

GuiCommanderMapPopupMenu::GuiCommanderMapPopupMenu()
{
}

void GuiCommanderMapPopupMenu::addChildren()
{
   mTl = new CommanderPopupList(this);
   mTl->setControlProfile(mProfile);
   mTl->setField("noDuplicates", "false");

   CommanderPopupScroll *scroll = new CommanderPopupScroll;
   scroll->setControlProfile(mProfile);
   scroll->setField("hScrollBar", "alwaysOff");
   scroll->setField("vScrollBar", "alwaysOff");
   mSc = scroll;

   mBackground = new GuiPopUpBackgroundCtrl(this, mTl);
}

/// Opens just below and right of where it was put, as wide as its longest
/// order and as tall as all of them.
void GuiCommanderMapPopupMenu::onAction()
{
   Parent::onAction();

   GuiControl *parent = getParent();
   if (!mSc || !mTl || !mBackground || !parent || !mProfile->mFont)
      return;

   GFont *font = mProfile->mFont;
   S32 width = 0;
   for (S32 i = 0; i < mEntries.size(); i++)
      width = getMax(width, shownWidth(font, mEntries[i].buf));
   width += TextMargin * 2;

   // The list's own rows, which are two taller than the font.
   const S32 row = font->getHeight() + 2;
   const S32 border = mSc->getBorderThickness();
   const Point2I size(width + border * 2, row * mEntries.size() + border * 2);

   Point2I at = mBackground->globalToLocalCoord(parent->localToGlobalCoord(getPosition()));
   const Point2I room = mBackground->getExtent();
   if (at.x + size.x > room.x)
      at.x = getMax(room.x - size.x, 0);
   if (at.y + size.y > room.y)
      at.y = getMax(room.y - size.y, 0);

   mTl->setSize(Point2I(1, mEntries.size()));
   mTl->setCellSize(Point2I(width, row));
   mTl->setExtent(Point2I(width, row * mEntries.size()));
   mSc->resize(at, size);

   // Whatever the scroll keeps for itself inside its border comes on top.
   const Point2I inner = mSc->getContentExtent();
   const Point2I missing(getMax(width - inner.x, 0), getMax(row * mEntries.size() - inner.y, 0));
   if (missing.x || missing.y)
      mSc->resize(at, size + missing);
}

void GuiCommanderMapPopupMenu::onRender(Point2I, const RectI &)
{
}
