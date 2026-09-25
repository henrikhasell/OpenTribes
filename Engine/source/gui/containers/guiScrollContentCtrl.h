// HH: Added for Tribes 2 compatibility.
#ifndef _GUISCROLLCONTENTCTRL_H_
#define _GUISCROLLCONTENTCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiScrollCtrl;

/// The control a Tribes 2 scroll control holds its contents in.
///
/// A scroll control keeps exactly one of these and puts everything else
/// inside it, so what is scrolled is always a grandchild.  This control is a
/// fixed window over that: it stays the size of the scroll control's content
/// rect, and a request to move or resize it is passed on to what it holds.
/// Scrolling therefore moves the contents behind a window that never moves,
/// which is what lets the contents be taller than the control they sit in.
class GuiScrollContentCtrl : public GuiControl
{
   typedef GuiControl Parent;

public:
   GuiScrollContentCtrl();

   DECLARE_CONOBJECT( GuiScrollContentCtrl );

   /// The control being scrolled, or NULL while this is empty.
   GuiControl *getContents();

   /// Sit over the given rect, which is the scroll control's content rect.
   void setWindow( const Point2I &position, const Point2I &extent );

   /// Tell the scroll control that the contents moved under their own steam.
   void contentsMoved();

   bool onWake() override;
   bool resize( const Point2I &newPosition, const Point2I &newExtent ) override;
   void childResized( GuiControl *child ) override;
};

/// The scroll control scrolling \a ctrl, and the offset that puts a rect in
/// \a ctrl's own coordinates into that scroll control's client space.
///
/// Client space starts at the top left of what is being scrolled, so the
/// position of the content control's own child is the scroll offset and is
/// not part of the offset -- counting it would ask to scroll by where we
/// already are, twice over.
GuiScrollCtrl *findScrollAbove( GuiControl *ctrl, Point2I &offset );

#endif // _GUISCROLLCONTENTCTRL_H_
