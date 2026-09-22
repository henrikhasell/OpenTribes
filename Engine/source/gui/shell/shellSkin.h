// HH: imported from TGE

#ifndef _SHELLSKIN_H_
#define _SHELLSKIN_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif

/// @file
/// Loading and drawing the bitmap families Tribes 2's shell is skinned from.
///
/// A shell control does not have one bitmap, it has a stem.  The profile
/// carries `bitmapBase` -- "gui/shll_field", "gui/shll", "gui/shll_scroll" --
/// and the control appends the suffixes it knows about: nine of them for a
/// frame's corners and edges, seven for a scrollbar's parts.  So the loading
/// is per-control and shared, and that is what this is.
///
/// Nothing here decides what a control looks like.  It answers two questions:
/// where is the texture called `<base><suffix>`, and how do you stretch a
/// nine-piece frame over an arbitrary rectangle without distorting the corners.

//-----------------------------------------------------------------------------
/// A lazily-loaded set of textures sharing a stem.
///
/// Load() is cheap to call repeatedly -- the texture manager caches, and a
/// suffix that resolves to nothing stays null and simply does not draw, which
/// is what lets one skin serve controls that use different parts of it.
class ShellSkin
{
   enum { MaxParts = 16, MaxPieces = 8 };

   StringTableEntry mBase;
   StringTableEntry mSuffix[MaxParts];
   GFXTexHandle     mTexture[MaxParts];
   U32              mCount;

   /// Where the separator rows put the pieces of each part, once measured.
   bool mMeasured[MaxParts];
   U32  mPieceCount[MaxParts];
   S32  mPieceTop[MaxParts][MaxPieces];
   S32  mPieceBottom[MaxParts][MaxPieces];

   void measure(S32 slot);

public:
   ShellSkin();

   /// Point the skin at a stem.  Does nothing if it is already there, so a
   /// control can call this from onRender without paying for it.
   void setBase(StringTableEntry base);

   StringTableEntry getBase() const { return mBase; }

   /// Load `<base><suffix>` and return its slot, or -1 if there is no stem.
   /// The slot stays valid until the base changes.
   S32 part(const char *suffix);

   GFXTexHandle &texture(S32 slot) { return mTexture[slot]; }

   /// The texture for a suffix, or a null handle.  Convenience over part().
   GFXTexHandle get(const char *suffix);

   /// Where one cell of a part lives.
   ///
   /// A part is a grid: its rows are the pieces it is built from and its
   /// columns are the states it can be in.  The rows are delimited by lines in
   /// the separator colour -- the colour of the top-left pixel, the same
   /// convention GuiControlProfile::constructBitmapArray() reads -- because
   /// they are not all the same height.  The columns have no separators and
   /// are an even division.
   ///
   /// Art with fewer separated rows than `pieces` is divided evenly instead,
   /// which covers a part drawn as a plain grid and a part with no separators
   /// at all.
   RectI cell(S32 slot, S32 states, S32 state, S32 pieces, S32 piece);

   void clear();
};

//-----------------------------------------------------------------------------
// Drawing.

/// Stretch a nine-piece frame over `bounds`.
///
/// The corners are drawn at their own size, the edges are stretched along the
/// one axis they run in, and the middle fills what is left.  A null piece is
/// skipped rather than substituted, so a frame with no middle draws as a
/// border over whatever is already there.
///
/// The nine are given in reading order: top-left, top-middle, top-right, then
/// the middle row, then the bottom.
void shellDrawFrame(const RectI &bounds, GFXTexHandle *pieces);

/// The same, naming the pieces by the suffixes Tribes 2 gives them.
void shellDrawFrame(const RectI &bounds, ShellSkin &skin,
                    const char *tl, const char *tm, const char *tr,
                    const char *ml, const char *mm, const char *mr,
                    const char *bl, const char *bm, const char *br);

/// The _TL.._BR family, which is how most of the shell's frames are named.
void shellDrawFrame(const RectI &bounds, ShellSkin &skin);

/// Repeat a texture across `bounds` rather than stretching it.
///
/// The shell's fills are one- and two-pixel strips meant to tile; stretching
/// them turns a gradient into a smear.
void shellTileBitmap(const RectI &bounds, GFXTexHandle texture);

#endif // _SHELLSKIN_H_
