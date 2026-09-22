// HH: imported from TGE
// (the art is measured through GFXTexHandle rather than TextureHandle, the
// childMargin field is dropped because GuiScrollCtrl already has one, and
// hasScrollArt() replaces the bitmap-array test that would otherwise keep the
// overrides below from ever being called)

#ifndef _SHELLSCROLLCTRL_H_
#define _SHELLSCROLLCTRL_H_

#ifndef _GUISCROLLCTRL_H_
#include "gui/containers/guiScrollCtrl.h"
#endif
#ifndef _GUISCROLLCONTENTCTRL_H_
#include "gui/containers/guiScrollContentCtrl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// @file
/// Scrolling, in the shell's skin.
///
/// Everything a scroll bar does -- the thumb, the arrows, paging, the wheel,
/// deciding whether a bar is needed at all -- GuiScrollCtrl already does, and
/// does it against rectangles it works out itself.  What Tribes 2 changes is
/// only what is drawn into those rectangles, and GuiScrollCtrl makes each of
/// those a virtual, so these classes are the four overrides and nothing else.
///
/// The art is a family under the profile's stem: _vertfield and _horzfield are
/// the tracks, _vertbar and _horzbar the thumbs, _vertbuttons and _horzbuttons
/// the arrows.  Each is itself in three parts stacked vertically -- for a
/// thumb, the two caps and the middle; for the arrows, the two ends.

//-----------------------------------------------------------------------------
class ShellScrollCtrl : public GuiScrollCtrl
{
   typedef GuiScrollCtrl Parent;

protected:
   ShellSkin mSkin;

   StringTableEntry mFieldBase;

   /// How many states each kind of part is drawn in.
   ///
   /// Every image in the family is a grid: its columns are the states a part
   /// can be in and its rows the pieces it is built from.  Across the family
   /// the counts are
   ///
   ///     _vertfield   36x21   3 states x 3 pieces
   ///     _vertbar    104x25   4 states x 3 pieces
   ///     _vertbuttons 104x54  4 states x 2 pieces
   ///     _horzfield   12x57   3 states x 3 pieces
   ///     _horzbar     36x81   4 states x 3 pieces
   ///     _horzbuttons 104x54  4 states x 2 pieces
   ///     _scale      104x27   4 states x 1 piece
   ///
   /// -- three states for the parts that only sit there, four for the ones a
   /// mouse can be over, pressing or find disabled.  The three pieces of a bar
   /// are its two caps and the run between them; the two of a button are the
   /// two directions it can point.  ShellSkin::cell() finds the rows; only the
   /// number of states has to be said here, there being nothing in the image
   /// that marks where one ends.
   enum
   {
      TrackStates  = 3,
      BarStates    = 4,
      ButtonStates = 4,
   };

   /// Draw a three-piece bar along `bounds`, capping the ends and stretching
   /// the middle.
   void drawBar(const char *part, S32 states, const RectI &bounds, bool vertical);

   /// Draw one piece of a part at whatever size `bounds` asks for.
   void drawPiece(const char *part, S32 states, S32 pieces, S32 piece, const RectI &bounds);

   bool hasScrollArt() const override;

public:
   ShellScrollCtrl();

   DECLARE_CONOBJECT(ShellScrollCtrl);
   static void initPersistFields();

   bool onWake() override;

   void drawBorder(const Point2I &offset, bool isFirstResponder) override;
   void drawVScrollBar(const Point2I &offset) override;
   void drawHScrollBar(const Point2I &offset) override;
   void drawScrollCorner(const Point2I &offset) override;
};

//-----------------------------------------------------------------------------
/// A scroller whose content is taller than any one control.
///
/// Tribes 2 distinguishes this from ShellScrollCtrl in its layouts but not in
/// anything it does with one; both scroll their children.
class VirtualScrollCtrl : public ShellScrollCtrl
{
   typedef ShellScrollCtrl Parent;

public:
   DECLARE_CONOBJECT(VirtualScrollCtrl);
};

//-----------------------------------------------------------------------------
/// A scroller that keeps its bar to one axis.
class ShellFancyArrayScrollCtrl : public ShellScrollCtrl
{
   typedef ShellScrollCtrl Parent;

   bool mFixedHorizontal;
   StringTableEntry mHorzSpacerBitmap;
   StringTableEntry mVertSpacerBitmap;

public:
   ShellFancyArrayScrollCtrl();

   DECLARE_CONOBJECT(ShellFancyArrayScrollCtrl);
   static void initPersistFields();
};

//-----------------------------------------------------------------------------
/// The content control the virtual scrollers hold, which differs from the one
/// the others hold only in the name Tribes 2's layouts give it.
class VirtualScrollContentCtrl : public GuiScrollContentCtrl
{
   typedef GuiScrollContentCtrl Parent;

public:
   DECLARE_CONOBJECT(VirtualScrollContentCtrl);
};

#endif // _SHELLSCROLLCTRL_H_
