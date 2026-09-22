// HH: imported from TGE
// (drawing goes through GFXDrawUtil rather than dgl, and the extent comes from
// getExtent rather than mBounds)
//
// The title tab's two halves are taken off its separator row rather than as
// half the image, which is a pixel either way -- dlg_titletab is two bands of
// twenty-eight in a fifty-eight pixel image -- but it is the same measurement
// every other part of the skin is read with, so there is no reason to guess
// here.

#include "platform/platform.h"
#include "gui/shell/shellPaneCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(ShellPaneCtrl);

ConsoleDocClass( ShellPaneCtrl,
   "@brief The frame Tribes 2's shell puts around a screen.\n\n"

   "A teal border with a tab in its top-left corner carrying a title, drawn "
   "from the family of files under the profile's bitmapBase.\n\n"

   "@ingroup GuiContainers"
);

//-----------------------------------------------------------------------------
ShellPaneCtrl::ShellPaneCtrl()
{
   mText = StringTable->insert("");
   mNoTitleBar = false;
   setExtent(200, 100);
}

void ShellPaneCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("text",       TypeCaseString, Offset(mText, ShellPaneCtrl),
      "What goes on the tab in the pane's top-left corner.");
   addField("noTitleBar", TypeBool,       Offset(mNoTitleBar, ShellPaneCtrl),
      "Whether the pane is drawn without that tab.");
}

bool ShellPaneCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   mSkin.setBase(mProfile->mBitmapBase);
   return true;
}

void ShellPaneCtrl::setText(const char *text)
{
   mText = StringTable->insert(text ? text : "", true);
   setUpdate();
}

S32 ShellPaneCtrl::titleHeight()
{
   if(mNoTitleBar || !mText || !mText[0])
      return 0;

   const S32 tab = mSkin.part("_titletab");
   if(tab == -1 || !mSkin.texture(tab))
      return mProfile->mFont ? S32(mProfile->mFont->getHeight()) + 6 : 0;

   // The tab art stacks its two ends, so one of them is a band of the image.
   return mSkin.cell(tab, 1, 0, 2, 0).extent.y;
}

RectI ShellPaneCtrl::contentRect(const RectI &bounds)
{
   GFXTexHandle edge = mSkin.get("_frame_edge");
   const S32 inset = edge ? shellArtWidth(edge) / 2 : 4;
   const S32 top = titleHeight();

   RectI r(bounds.point.x + inset, bounds.point.y + top + inset,
           bounds.extent.x - inset * 2, bounds.extent.y - top - inset * 2);

   if(r.extent.x < 0) r.extent.x = 0;
   if(r.extent.y < 0) r.extent.y = 0;
   return r;
}

void ShellPaneCtrl::drawFrame(const RectI &bounds)
{
   GFXTexHandle corner = mSkin.get("_frame_edge");
   GFXTexHandle run    = mSkin.get("_frame_end");
   GFXTexHandle fill   = mSkin.get("_fieldfill");
   GFXTexHandle grade  = mSkin.get("_fieldgrade");

   if(!corner && !run)
      return;

   const S32 cw = corner ? shellArtWidth(corner)  : shellArtWidth(run);
   const S32 ch = corner ? shellArtHeight(corner) : cw;

   const S32 x0 = bounds.point.x, x1 = x0 + bounds.extent.x;
   const S32 y0 = bounds.point.y, y1 = y0 + bounds.extent.y;

   const S32 midW = bounds.extent.x - cw * 2;
   const S32 midH = bounds.extent.y - ch * 2;
   if(midW < 0 || midH < 0)
      return;

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   // The interior first, so the frame sits over its edge.
   RectI inside(x0 + cw, y0 + ch, midW, midH);
   if(fill)
      shellTileBitmap(inside, fill);

   // The gradient is a tall thin strip: it runs down from the top of the
   // panel, which is what gives the shell its lit-from-above look.
   if(grade && inside.extent.y > 0)
   {
      const S32 gh = getMin(shellArtHeight(grade), inside.extent.y);
      drawer->drawBitmapStretchSR(grade, RectI(inside.point.x, inside.point.y, inside.extent.x, gh),
                                  RectI(0, 0, shellArtWidth(grade), gh));
   }

   if(run)
   {
      // The one long piece the art gives is vertical, so it draws the two
      // sides as it is and the top and bottom flipped onto their side by
      // stretching the corner rather than distorting a gradient.
      drawer->drawBitmapStretch(run, RectI(x0, y0 + ch, cw, midH));
      drawer->drawBitmapStretch(run, RectI(x1 - cw, y0 + ch, cw, midH), GFXBitmapFlip_X);
   }

   if(corner)
   {
      drawer->drawBitmapStretch(corner, RectI(x0 + cw, y0, midW, ch));
      drawer->drawBitmapStretch(corner, RectI(x0 + cw, y1 - ch, midW, ch), GFXBitmapFlip_Y);

      drawer->drawBitmap(corner, Point2I(x0, y0));
      drawer->drawBitmap(corner, Point2I(x1 - cw, y0), GFXBitmapFlip_X);
      drawer->drawBitmap(corner, Point2I(x0, y1 - ch), GFXBitmapFlip_Y);
      drawer->drawBitmap(corner, Point2I(x1 - cw, y1 - ch), GFXBitmapFlip_XY);
   }
}

void ShellPaneCtrl::drawTitleTab(const RectI &bounds)
{
   const S32 th = titleHeight();
   if(!th)
      return;

   // Wide enough for the title and no wider: the tab is a tab, not a bar.
   S32 textWidth = 0;
   if(mProfile->mFont)
      textWidth = mProfile->mFont->getStrWidth((const UTF8*)mText);

   const S32 slot = mSkin.part("_titletab");
   GFXTexHandle tab = slot == -1 ? GFXTexHandle() : mSkin.texture(slot);
   const S32 cap = shellArtWidth(tab);

   RectI tabRect(bounds.point.x, bounds.point.y,
                 getMin(textWidth + cap * 2 + 20, bounds.extent.x), th);

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if(tab)
   {
      // The two bands in the art are the tab's two ends.
      const RectI left  = mSkin.cell(slot, 1, 0, 2, 0);
      const RectI right = mSkin.cell(slot, 1, 0, 2, 1);

      drawer->drawBitmapStretchSR(tab, RectI(tabRect.point.x, tabRect.point.y, cap, th), left);
      drawer->drawBitmapStretchSR(tab, RectI(tabRect.point.x + tabRect.extent.x - cap,
                                             tabRect.point.y, cap, th), right);

      GFXTexHandle fill = mSkin.get("_fieldfill");
      if(fill && tabRect.extent.x > cap * 2)
         shellTileBitmap(RectI(tabRect.point.x + cap, tabRect.point.y,
                               tabRect.extent.x - cap * 2, th), fill);
   }

   if(mProfile->mFont && mText[0])
   {
      drawer->setBitmapModulation(mProfile->mFontColor);
      const S32 ty = tabRect.point.y + (th - S32(mProfile->mFont->getHeight())) / 2;
      drawer->drawText(mProfile->mFont, Point2I(tabRect.point.x + cap + 4, ty), mText);
      drawer->clearBitmapModulation();
   }
}

void ShellPaneCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   mSkin.setBase(mProfile->mBitmapBase);

   const RectI bounds(offset, getExtent());

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   drawer->setBitmapModulation(ColorI(255, 255, 255, 255));
   drawFrame(bounds);
   drawTitleTab(bounds);
   drawer->clearBitmapModulation();

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
DefineEngineMethod( ShellPaneCtrl, setTitle, void, ( const char *title ),,
   "What goes on the tab in the pane's top-left corner." )
{
   object->setText(title);
}

DefineEngineMethod( ShellPaneCtrl, getTitle, const char *, (),,
   "What is on the tab in the pane's top-left corner." )
{
   return object->getText();
}
