// HH: imported from TGE
// (drawing goes through GFXDrawUtil rather than dgl, and a missing part is a
// null GFXTexHandle rather than an Optional TextureHandle)

#include "gui/shell/shellSkin.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTextureManager.h"
#include "core/util/safeDelete.h"
#include "console/console.h"

ShellSkin::ShellSkin()
{
   mBase = NULL;
   mCount = 0;
}

void ShellSkin::clear()
{
   for(U32 i = 0; i < mCount; i++)
   {
      mSuffix[i] = NULL;
      mTexture[i] = NULL;
   }
   mCount = 0;
}

void ShellSkin::setBase(StringTableEntry base)
{
   // StringTable pointers compare by identity, which is the point of it.
   if(base == mBase)
      return;

   mBase = base;
   clear();
}

S32 ShellSkin::part(const char *suffix)
{
   if(!mBase || !mBase[0])
      return -1;

   StringTableEntry key = StringTable->insert(suffix);

   for(U32 i = 0; i < mCount; i++)
      if(mSuffix[i] == key)
         return S32(i);

   if(mCount == MaxParts)
   {
      Con::errorf("ShellSkin: more than %d pieces asked of '%s'", MaxParts, mBase);
      return -1;
   }

   char name[512];
   dSprintf(name, sizeof(name), "%s%s", mBase, suffix);

   const U32 slot = mCount++;
   mSuffix[slot] = key;
   // A skin part that this base does not have is normal -- Tribes 2's skins
   // are sparse, and shellDrawFrame() is written to degrade around a missing
   // piece -- so the handle simply stays null.
   mTexture[slot] = GFXTexHandle(name, &GFXDefaultGUIProfile, avar("%s() - %s", __FUNCTION__, name));

   return S32(slot);
}

GFXTexHandle ShellSkin::get(const char *suffix)
{
   const S32 slot = part(suffix);
   return slot == -1 ? GFXTexHandle() : mTexture[slot];
}

//-----------------------------------------------------------------------------

void shellDrawFrame(const RectI &bounds, GFXTexHandle *p)
{
   // A missing corner is a zero-sized one, which keeps the arithmetic below
   // honest: the edges then run the full span and the frame degrades to a
   // stretched middle rather than to nothing.
   const S32 lw = p[0] ? p[0].getWidth()  : (p[3] ? p[3].getWidth()  : 0);
   const S32 rw = p[2] ? p[2].getWidth()  : (p[5] ? p[5].getWidth()  : 0);
   const S32 th = p[0] ? p[0].getHeight() : (p[1] ? p[1].getHeight() : 0);
   const S32 bh = p[6] ? p[6].getHeight() : (p[7] ? p[7].getHeight() : 0);

   // Never let the corners eat more than the control has.
   const S32 left   = getMin(lw, bounds.extent.x);
   const S32 right  = getMin(rw, bounds.extent.x - left);
   const S32 top    = getMin(th, bounds.extent.y);
   const S32 bottom = getMin(bh, bounds.extent.y - top);

   const S32 midW = bounds.extent.x - left - right;
   const S32 midH = bounds.extent.y - top - bottom;

   const S32 x0 = bounds.point.x, x1 = x0 + left, x2 = x1 + midW;
   const S32 y0 = bounds.point.y, y1 = y0 + top,  y2 = y1 + midH;

   const RectI cells[9] = {
      RectI(x0, y0, left,  top),    RectI(x1, y0, midW, top),    RectI(x2, y0, right, top),
      RectI(x0, y1, left,  midH),   RectI(x1, y1, midW, midH),   RectI(x2, y1, right, midH),
      RectI(x0, y2, left,  bottom), RectI(x1, y2, midW, bottom), RectI(x2, y2, right, bottom),
   };

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   for(U32 i = 0; i < 9; i++)
   {
      if(!p[i] || cells[i].extent.x <= 0 || cells[i].extent.y <= 0)
         continue;

      drawer->drawBitmapStretch(p[i], cells[i]);
   }
}

void shellDrawFrame(const RectI &bounds, ShellSkin &skin,
                    const char *tl, const char *tm, const char *tr,
                    const char *ml, const char *mm, const char *mr,
                    const char *bl, const char *bm, const char *br)
{
   const char *names[9] = { tl, tm, tr, ml, mm, mr, bl, bm, br };

   // Resolve every slot before taking a reference to any of them: part() can
   // grow the array, and growing it would move whatever we already held.
   S32 slots[9];
   for(U32 i = 0; i < 9; i++)
      slots[i] = names[i] ? skin.part(names[i]) : -1;

   GFXTexHandle pieces[9];
   for(U32 i = 0; i < 9; i++)
      if(slots[i] != -1)
         pieces[i] = skin.texture(slots[i]);

   shellDrawFrame(bounds, pieces);
}

void shellDrawFrame(const RectI &bounds, ShellSkin &skin)
{
   shellDrawFrame(bounds, skin, "_TL", "_TM", "_TR",
                                "_ML", "_MM", "_MR",
                                "_BL", "_BM", "_BR");
}

//-----------------------------------------------------------------------------

void shellTileBitmap(const RectI &bounds, GFXTexHandle texture)
{
   if(!texture)
      return;

   const S32 tw = texture.getWidth();
   const S32 th = texture.getHeight();
   if(tw <= 0 || th <= 0)
      return;

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   // Below this a "tile" carries no pattern to repeat -- the shell's fills are
   // two pixels square, a flat translucent wash -- and repeating one over a
   // six-hundred-pixel panel is a hundred thousand draw calls for the same
   // result one stretch gives.
   enum { SmallestWorthTiling = 8 };

   const bool tileX = tw >= SmallestWorthTiling;
   const bool tileY = th >= SmallestWorthTiling;

   if(!tileX && !tileY)
   {
      drawer->drawBitmapStretch(texture, bounds);
      return;
   }

   if(!tileX)
   {
      for(S32 y = bounds.point.y; y < bounds.point.y + bounds.extent.y; y += th)
      {
         const S32 h = getMin(th, bounds.point.y + bounds.extent.y - y);
         drawer->drawBitmapStretchSR(texture, RectI(bounds.point.x, y, bounds.extent.x, h),
                                     RectI(0, 0, tw, h));
      }
      return;
   }

   if(!tileY)
   {
      for(S32 x = bounds.point.x; x < bounds.point.x + bounds.extent.x; x += tw)
      {
         const S32 w = getMin(tw, bounds.point.x + bounds.extent.x - x);
         drawer->drawBitmapStretchSR(texture, RectI(x, bounds.point.y, w, bounds.extent.y),
                                     RectI(0, 0, w, th));
      }
      return;
   }

   for(S32 y = bounds.point.y; y < bounds.point.y + bounds.extent.y; y += th)
   {
      for(S32 x = bounds.point.x; x < bounds.point.x + bounds.extent.x; x += tw)
      {
         // Clip the last tile in each direction rather than letting it hang
         // over the edge: these are gradients, and a half tile that runs on is
         // visible as a seam.
         RectI cell(x, y, getMin(tw, bounds.point.x + bounds.extent.x - x),
                          getMin(th, bounds.point.y + bounds.extent.y - y));

         if(cell.extent.x == tw && cell.extent.y == th)
            drawer->drawBitmap(texture, cell.point);
         else
            drawer->drawBitmapStretchSR(texture, cell, RectI(0, 0, cell.extent.x, cell.extent.y));
      }
   }
}
