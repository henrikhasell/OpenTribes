// HH: imported from TGE
// (drawing goes through GFXDrawUtil rather than dgl, and the profile's art is
// mTextureObject rather than mTextureHandle)

#include "platform/platform.h"
#include "gui/shell/shellStateBitmap.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "console/console.h"

ShellStateBitmap::ShellStateBitmap()
{
   mStateCount = 0;
}

S32 ShellStateBitmap::getStateWidth() const
{
   if(!mStateCount)
      return 0;

   // From the first piece to the last, which for a single-piece state is just
   // that piece.
   const RectI &first = mPiece[0][0];
   const RectI &last  = mPiece[0][mPieces[0] - 1];
   return last.point.x + last.extent.x - first.point.x;
}

void ShellStateBitmap::update(GuiControlProfile *profile)
{
   // A profile that names a family does not also have a single bitmap, and
   // what it appears to have is not its own: GuiControlProfile copies
   // mBitmapName off GuiDefaultProfile, which Tribes 2's editor profiles point
   // at "gui/darkWindow".  Every shell profile therefore inherits a window
   // frame it never asked for, and drawing it puts that frame behind every
   // control in the game.
   const bool hasFamily = profile && profile->mBitmapBase && profile->mBitmapBase[0];

   GFXTexHandle texture = (profile && !hasFamily) ? profile->mTextureObject : GFXTexHandle();

   if(texture.getPointer() == mTexture.getPointer())
      return;

   mTexture = texture;
   mStateCount = 0;

   const GBitmap *bmp = mTexture.getBitmap();
   if(!bmp || bmp->getWidth() < 1 || bmp->getHeight() < 1)
      return;

   const S32 w = S32(bmp->getWidth());
   const S32 h = S32(bmp->getHeight());

   ColorI separator;
   bmp->getColor(0, 0, separator);

   // Rows first, read off column zero.  That is Torque's own bitmap-array
   // convention and it is the only test that works here: a separator row is
   // transparent all the way across but not the *same* transparent -- the
   // magenta is only exact where the artist drew it, and in the middle of the
   // row the same fully-clear pixels carry other colours entirely.
   S32 y = 0;
   while(y < h && mStateCount < MaxStates)
   {
      ColorI c;

      bmp->getColor(0, y, c);
      if(c == separator)
      {
         y++;
         continue;
      }

      const S32 top = y;
      while(y < h)
      {
         bmp->getColor(0, y, c);
         if(c == separator)
            break;
         y++;
      }

      const S32 state = mStateCount++;
      const S32 height = y - top;

      // Then this state's own pieces, which are the runs of columns that draw
      // anything at all.  Here the test has to be the alpha rather than the
      // colour, for the same reason: what separates two pieces is clear, and
      // clear is all it reliably is.
      S32 pieces = 0;
      S32 x = 0;
      while(x < w)
      {
         bool blank = true;
         for(S32 yy = top; yy < top + height && blank; yy++)
         {
            bmp->getColor(x, yy, c);
            blank = (c.alpha == 0);
         }

         if(blank)
         {
            x++;
            continue;
         }

         const S32 left = x;
         while(x < w)
         {
            blank = true;
            for(S32 yy = top; yy < top + height && blank; yy++)
            {
               bmp->getColor(x, yy, c);
               blank = (c.alpha == 0);
            }
            if(blank)
               break;
            x++;
         }

         if(pieces < MaxPieces)
            mPiece[state][pieces++] = RectI(left, top, x - left, height);
         else
            mPiece[state][MaxPieces - 1].extent.x =
               x - mPiece[state][MaxPieces - 1].point.x;
      }

      // One piece or three; two is not a shape this art comes in, and taking
      // the span of both is closer than dropping one.
      if(pieces == 2)
      {
         mPiece[state][0].extent.x = mPiece[state][1].point.x + mPiece[state][1].extent.x
                                   - mPiece[state][0].point.x;
         pieces = 1;
      }

      if(!pieces)
      {
         mPiece[state][0] = RectI(0, top, w, height);
         pieces = 1;
      }

      mPieces[state] = pieces;
   }
}

void ShellStateBitmap::draw(S32 state, const RectI &bounds) const
{
   if(!mStateCount || bounds.extent.x <= 0 || bounds.extent.y <= 0)
      return;

   state = mClamp(state, 0, mStateCount - 1);

   GFXDrawUtil *drawer = GFX->getDrawUtil();
   GFXTextureObject *art = mTexture.getPointer();

   if(mPieces[state] == 1)
   {
      drawer->drawBitmapStretchSR(art, bounds, mPiece[state][0]);
      return;
   }

   const RectI &left  = mPiece[state][0];
   const RectI &right = mPiece[state][2];

   // The middle piece is not a strip -- the artist drew a whole rounded shape
   // there, the same shape as the caps -- so stretching it whole gives a blob
   // in the centre of the button.  What stretches is its cross-section: the
   // one column through its middle, which is the flat part the caps join onto.
   // Taking that turns the three pieces into a bar with rounded ends, which is
   // what a Tribes 2 button is.
   const RectI &whole = mPiece[state][1];
   const RectI mid(whole.point.x + whole.extent.x / 2, whole.point.y, 1, whole.extent.y);

   // Never let the two caps overlap: on a control narrower than the art, give
   // each of them half and drop the middle.
   const S32 lw = getMin(left.extent.x,  bounds.extent.x / 2);
   const S32 rw = getMin(right.extent.x, bounds.extent.x - lw);
   const S32 mw = bounds.extent.x - lw - rw;

   const S32 y = bounds.point.y, bh = bounds.extent.y;

   drawer->drawBitmapStretchSR(art, RectI(bounds.point.x, y, lw, bh), left);
   if(mw > 0)
      drawer->drawBitmapStretchSR(art, RectI(bounds.point.x + lw, y, mw, bh), mid);
   drawer->drawBitmapStretchSR(art, RectI(bounds.point.x + lw + getMax(mw, 0), y, rw, bh), right);
}
