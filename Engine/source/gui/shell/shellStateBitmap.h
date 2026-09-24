// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSTATEBITMAP_H_
#define _SHELLSTATEBITMAP_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

/// Tribes 2's stretchable multi-state control art.
///
/// One sheet holds a grid of cells: a column per state the control has --
/// resting, showing, under the mouse -- and a row per piece it is drawn from
/// -- left cap, middle, right cap.  So a tab of any width is its column's
/// three rows, with only the middle stretched.
///
/// What divides the cells is the colour at pixel (0,0), and the sheet states
/// its own measurements in that colour rather than in a rule between every
/// cell.  A row begins with a line of separator whose length, from the left
/// edge to the first pixel that is not exactly the separator, is how wide one
/// cell is.  How tall it is follows from column zero: the run below that line
/// which is not the separator.  Each cell is then inset by a pixel on every
/// side, so the divisions never bleed into what is drawn.
///
/// gui/lnch_tab is 54x102 and measures out as nine 16x31 cells; gui/shll_menu-
/// close is 105x29 and gives one row of three 33x26, which is exactly the size
/// LaunchToolbarDlg.gui gives the button drawn from it.
class ShellStateBitmap
{
public:
   enum
   {
      MaxStates = 3,   ///< Resting, showing, under the mouse.
      MaxPieces = 3,   ///< Left cap, middle, right cap.
   };

private:
   GFXTexHandle mTexture;
   StringTableEntry mSource;

   RectI mCell[ MaxPieces ][ MaxStates ];
   S32 mPieces;

public:
   ShellStateBitmap();

   /// Measure the art `profile` names.  Cheap to repeat: it does nothing while
   /// the profile still names what it last measured.
   void update( GuiControlProfile *profile );

   /// The same for art named directly, which is how a control that carries
   /// several sheets at once -- a tab bar with a set per group of tabs -- gets
   /// the ones the profile does not name.
   void load( const char *name );

   bool isValid() const { return mPieces > 0; }

   /// How many rows the sheet actually carried.  Three is a control drawn from
   /// caps and a middle; one is a control drawn whole, which is what art the
   /// size of the control it skins gives.
   S32 getPieceCount() const { return mPieces; }

   /// The height one state was drawn at, which is what a control using this
   /// art should be tall.
   S32 getStateHeight() const { return mPieces ? mCell[ 0 ][ 0 ].extent.y : 0; }

   /// Draw one state across `bounds`: the caps at the size they were drawn and
   /// the middle stretched between them, or the single row stretched whole
   /// where that is all the sheet has.  A state past the end clamps rather
   /// than drawing nothing, because art with fewer states than a control has
   /// is common.
   void draw( S32 state, const RectI &bounds ) const;

private:
   void measure( GBitmap *bitmap );
};

#endif // _SHELLSTATEBITMAP_H_
