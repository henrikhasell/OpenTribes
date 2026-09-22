// HH: imported from TGE
// (drawing goes through GFXDrawUtil rather than dgl, and childMargin is gone
// because GuiScrollCtrl already declares it)
//
// The pieces of a part are measured off its separator rows rather than taken
// as an even division of its height.  Tribes 2's art is not evenly divided --
// shll_scroll_vertbar is 9, 4 and 9 rows in a 25 pixel image -- so dividing it
// in three drew each piece a row early and stretched a separator through the
// middle of every thumb.  Everything the bars measure comes off the same
// bands, which is why the thumb's floor is set here too.

#include "platform/platform.h"
#include "gui/shell/shellScrollCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(ShellScrollCtrl);
IMPLEMENT_CONOBJECT(VirtualScrollCtrl);
IMPLEMENT_CONOBJECT(ShellFancyArrayScrollCtrl);
IMPLEMENT_CONOBJECT(VirtualScrollContentCtrl);

ConsoleDocClass( ShellScrollCtrl,
   "@brief A scroll control drawn in Tribes 2's shell skin.\n\n"

   "Scrolls exactly as GuiScrollCtrl does; the bars are drawn from the family "
   "of files under the profile's bitmapBase.\n\n"

   "@ingroup GuiContainers"
);

ConsoleDocClass( VirtualScrollCtrl,
   "@brief A ShellScrollCtrl by another name.\n\n"
   "@ingroup GuiContainers"
);

ConsoleDocClass( ShellFancyArrayScrollCtrl,
   "@brief A ShellScrollCtrl that Tribes 2's layouts keep to one axis.\n\n"
   "@ingroup GuiContainers"
);

ConsoleDocClass( VirtualScrollContentCtrl,
   "@brief A GuiScrollContentCtrl by another name.\n\n"
   "@ingroup GuiContainers"
);

//-----------------------------------------------------------------------------
ShellScrollCtrl::ShellScrollCtrl()
{
   mFieldBase = StringTable->insert("");
}

void ShellScrollCtrl::initPersistFields()
{
   Parent::initPersistFields();

   // Drawn by the ShellFieldCtrl these sit inside rather than here, but the
   // layouts set it and a field nothing declares is a console error on every
   // one of them.
   addField("fieldBase", TypeFilename, Offset(mFieldBase, ShellScrollCtrl),
      "Stem of the bitmap family for the field drawn around the scroller.");
}

bool ShellScrollCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   mSkin.setBase(mProfile->mBitmapBase);

   // GuiScrollCtrl takes the bar's thickness and the arrows' length from its
   // profile's bitmap array, which a shell profile does not have -- it names a
   // family instead.  Measure them off that family, or the bars come out one
   // pixel wide and the arrows have no room.
   const S32 track = mSkin.part("_vertfield");
   if(track != -1 && mSkin.texture(track))
      mScrollBarThickness = mSkin.texture(track).getWidth() / TrackStates;

   const S32 arrows = mSkin.part("_vertbuttons");
   if(arrows != -1 && mSkin.texture(arrows))
      mScrollBarArrowBtnLength = mSkin.cell(arrows, ButtonStates, 0, 2, 0).extent.y;

   // The floor under every thumb, and the same two caps drawBar() will put on
   // one.  Left alone it keeps a value measured off art this control does not
   // use, and a short thumb then draws its caps over each other.
   const S32 thumb = mSkin.part("_vertbar");
   if(thumb != -1 && mSkin.texture(thumb))
      mBaseThumbSize = mSkin.cell(thumb, BarStates, 0, 3, 0).extent.y +
                       mSkin.cell(thumb, BarStates, 0, 3, 2).extent.y;

   computeSizes();
   return true;
}

bool ShellScrollCtrl::hasScrollArt() const
{
   return mProfile->mBitmapBase && mProfile->mBitmapBase[0];
}

void ShellScrollCtrl::drawBar(const char *part, S32 states, const RectI &bounds, bool vertical)
{
   const S32 slot = mSkin.part(part);
   if(slot == -1 || bounds.extent.x <= 0 || bounds.extent.y <= 0)
      return;

   GFXTexHandle art = mSkin.texture(slot);
   if(!art)
      return;

   // State 0 throughout: nothing here tracks which part the mouse is over
   // closely enough to light one piece and not another, and a bar drawn in its
   // resting state is right far more often than it is wrong.
   const RectI head = mSkin.cell(slot, states, 0, 3, 0);
   const RectI body = mSkin.cell(slot, states, 0, 3, 1);
   const RectI tail = mSkin.cell(slot, states, 0, 3, 2);

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if(vertical)
   {
      const S32 cap = getMin(head.extent.y, bounds.extent.y / 2);
      const S32 mid = bounds.extent.y - cap * 2;
      const S32 x = bounds.point.x, w = bounds.extent.x;

      drawer->drawBitmapStretchSR(art, RectI(x, bounds.point.y, w, cap), head);
      if(mid > 0)
         drawer->drawBitmapStretchSR(art, RectI(x, bounds.point.y + cap, w, mid), body);
      drawer->drawBitmapStretchSR(art, RectI(x, bounds.point.y + cap + getMax(mid, 0), w, cap), tail);
   }
   else
   {
      // The horizontal art is a column too -- its three pieces are stacked in
      // the image and used left, middle, right.
      const S32 cap = getMin(head.extent.x, bounds.extent.x / 2);
      const S32 mid = bounds.extent.x - cap * 2;
      const S32 y = bounds.point.y, h = bounds.extent.y;

      drawer->drawBitmapStretchSR(art, RectI(bounds.point.x, y, cap, h), head);
      if(mid > 0)
         drawer->drawBitmapStretchSR(art, RectI(bounds.point.x + cap, y, mid, h), body);
      drawer->drawBitmapStretchSR(art, RectI(bounds.point.x + cap + getMax(mid, 0), y, cap, h), tail);
   }
}

void ShellScrollCtrl::drawPiece(const char *part, S32 states, S32 pieces, S32 piece,
                                const RectI &bounds)
{
   const S32 slot = mSkin.part(part);
   if(slot == -1 || bounds.extent.x <= 0 || bounds.extent.y <= 0)
      return;

   GFXTexHandle art = mSkin.texture(slot);
   if(!art)
      return;

   GFX->getDrawUtil()->drawBitmapStretchSR(art, bounds, mSkin.cell(slot, states, 0, pieces, piece));
}

void ShellScrollCtrl::drawBorder(const Point2I &, bool)
{
   // The shell's scrollers sit inside a ShellFieldCtrl that has already drawn
   // the frame around them; a border here would double it.
}

void ShellScrollCtrl::drawVScrollBar(const Point2I &offset)
{
   mSkin.setBase(mProfile->mBitmapBase);

   GFXDrawUtil *drawer = GFX->getDrawUtil();
   drawer->setBitmapModulation(ColorI(255, 255, 255, 255));

   RectI track = mVTrackRect;
   track.point += offset;
   drawBar("_vertfield", TrackStates, track, true);

   if(mVBarEnabled && mHasVScrollBar)
   {
      RectI thumb(mVTrackRect.point.x, mVTrackRect.point.y + mVThumbPos,
                  mVTrackRect.extent.x, mVThumbSize);
      thumb.point += offset;
      drawBar("_vertbar", BarStates, thumb, true);
   }

   RectI up = mUpArrowRect, down = mDownArrowRect;
   up.point += offset;
   down.point += offset;
   drawPiece("_vertbuttons", ButtonStates, 2, 0, up);
   drawPiece("_vertbuttons", ButtonStates, 2, 1, down);

   drawer->clearBitmapModulation();
}

void ShellScrollCtrl::drawHScrollBar(const Point2I &offset)
{
   mSkin.setBase(mProfile->mBitmapBase);

   GFXDrawUtil *drawer = GFX->getDrawUtil();
   drawer->setBitmapModulation(ColorI(255, 255, 255, 255));

   RectI track = mHTrackRect;
   track.point += offset;
   drawBar("_horzfield", TrackStates, track, false);

   if(mHBarEnabled && mHasHScrollBar)
   {
      RectI thumb(mHTrackRect.point.x + mHThumbPos, mHTrackRect.point.y,
                  mHThumbSize, mHTrackRect.extent.y);
      thumb.point += offset;
      drawBar("_horzbar", BarStates, thumb, false);
   }

   RectI left = mLeftArrowRect, right = mRightArrowRect;
   left.point += offset;
   right.point += offset;
   drawPiece("_horzbuttons", ButtonStates, 2, 0, left);
   drawPiece("_horzbuttons", ButtonStates, 2, 1, right);

   drawer->clearBitmapModulation();
}

void ShellScrollCtrl::drawScrollCorner(const Point2I &)
{
   // Nothing lives in the corner in Tribes 2's shell.
}

//-----------------------------------------------------------------------------
ShellFancyArrayScrollCtrl::ShellFancyArrayScrollCtrl()
{
   mFixedHorizontal = false;
   mHorzSpacerBitmap = StringTable->insert("");
   mVertSpacerBitmap = StringTable->insert("");
}

void ShellFancyArrayScrollCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("fixedHorizontal",  TypeBool,     Offset(mFixedHorizontal, ShellFancyArrayScrollCtrl),
      "Whether the horizontal extent is the control's rather than its child's.");
   addField("horzSpacerBitmap", TypeFilename, Offset(mHorzSpacerBitmap, ShellFancyArrayScrollCtrl),
      "Bitmap drawn between the rows of the array this scrolls.");
   addField("vertSpacerBitmap", TypeFilename, Offset(mVertSpacerBitmap, ShellFancyArrayScrollCtrl),
      "Bitmap drawn between the columns of the array this scrolls.");
}
