// HH: imported from TGE
// (the art is held as a GFXTexHandle rather than a TextureHandle)

#ifndef _SHELLSTATEBITMAP_H_
#define _SHELLSTATEBITMAP_H_

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
/// Tribes 2's stretchable multi-state control art.
///
/// A shell button is one image holding every state it has -- normal, rollover,
/// depressed -- stacked vertically and separated by a row of the colour in the
/// top-left corner, which is Torque's own bitmap-array convention.  What is not
/// Torque's convention is what a state contains: a left cap, a middle and a
/// right cap laid side by side, so one 54-pixel image draws a button of any
/// width by stretching only the middle.
///
/// Where those three sit is measured rather than assumed.  Nothing in the art
/// marks the divisions -- the columns between the pieces are simply clear --
/// so a state's pieces are the runs of columns that are not clear, and a state
/// with three of them is three pieces while a state with one is one image
/// stretched.  Both occur in the same bitmap: shll_button's resting and
/// pressed states are three pieces each and its rollover state, which is a
/// filled bar, is one.
class ShellStateBitmap
{
   enum { MaxStates = 8, MaxPieces = 3 };

   GFXTexHandle mTexture;

   RectI mPiece[MaxStates][MaxPieces];
   S32   mPieces[MaxStates];       ///< 1 or 3, per state.
   S32   mStateCount;

public:
   ShellStateBitmap();

   /// Measure `profile`'s bitmap.  Cheap to repeat: it does nothing once the
   /// texture it last measured is still the one the profile holds.
   void update(GuiControlProfile *profile);

   bool isValid() const { return mStateCount > 0; }
   S32 getStateCount() const { return mStateCount; }

   /// The height one state was drawn at, which is what a control using this
   /// art should be tall.
   S32 getStateHeight() const { return mStateCount ? mPiece[0][0].extent.y : 0; }

   /// The width of a state drawn at its own size -- what a control whose art
   /// is a single square, like a checkbox, should be wide.
   S32 getStateWidth() const;

   /// Draw one state across `bounds`, capping the ends and stretching the
   /// middle.  A state index past the end clamps rather than drawing nothing,
   /// because art with fewer states than a control has is common -- a button
   /// with no separate disabled frame should still draw.
   void draw(S32 state, const RectI &bounds) const;
};

#endif // _SHELLSTATEBITMAP_H_
