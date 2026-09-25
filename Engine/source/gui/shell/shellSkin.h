// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSKIN_H_
#define _SHELLSKIN_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

class GBitmap;

/// Cut `bitmap` into a `cols` by `rows` grid of cells, writing `cols * rows`
/// rects in reading order.  Returns how many rows it got, which can be fewer
/// than asked for: art the size of the control it skins is one row of states
/// and nothing else, and that is not a failure.
///
/// The sheet states its own measurements in the colour at pixel (0,0) rather
/// than ruling every cell.  A row opens with a run of exactly that colour, and
/// how far the run goes is how wide one cell is; the run below it down column
/// zero that is not that colour is how tall.  Each cell is then inset by a
/// pixel all round, so a division never bleeds into what is drawn.
S32 shellSliceGrid( GBitmap *bitmap, RectI *out, S32 cols, S32 rows );

/// A set of textures sharing a stem.
///
/// A shell control does not always have one bitmap; it has a stem.  The
/// profile carries `bitmapBase` -- "gui/launch_btn" -- and the control appends
/// the suffixes it knows about, "" and "_rol" and "_act".  A suffix that
/// resolves to nothing stays null and simply does not draw, which is what lets
/// one stem serve controls that use different parts of it.
class ShellSkin
{
   enum { MaxParts = 8 };

   StringTableEntry mBase;
   StringTableEntry mSuffix[ MaxParts ];
   GFXTexHandle     mTexture[ MaxParts ];
   U32              mCount;

public:
   ShellSkin();

   /// Point the skin at a stem.  Does nothing if it is already there, so a
   /// control can call this from onRender without paying for it.
   void setBase( StringTableEntry base );

   /// The texture for a suffix, loading it the first time it is asked for.
   GFXTexHandle get( const char *suffix );

   void clear();
};

#endif // _SHELLSKIN_H_
