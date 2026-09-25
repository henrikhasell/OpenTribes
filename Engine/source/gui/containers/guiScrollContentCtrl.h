// HH: Added for Tribes 2 compatibility.
#ifndef _GUISCROLLCONTENTCTRL_H_
#define _GUISCROLLCONTENTCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

/// The control a Tribes 2 scroll control holds its contents in.
///
/// Torque Game Engine pairs this with a scroll control that lays it out, so
/// there it is an empty container with a default extent.  Stock GuiScrollCtrl
/// measures its child rather than sizing it -- calcChildExtents() reads the
/// extent, nothing writes it -- and Tribes 2's .gui files declare this with
/// neither position nor extent.  So the sizing has to live here: fill the
/// scroll control, and grow past it where the contents are taller, which is
/// what gives the scrollbar something to scroll.
class GuiScrollContentCtrl : public GuiControl
{
   typedef GuiControl Parent;

   /// Set while fitToParent() is resizing, so that the children answering
   /// that resize do not start it again.
   bool mFitting;

   void fitToParent();

public:
   GuiScrollContentCtrl();

   DECLARE_CONOBJECT( GuiScrollContentCtrl );

   bool onWake() override;
   void parentResized( const RectI &oldParentRect, const RectI &newParentRect ) override;
   void childResized( GuiControl *child ) override;
};

#endif // _GUISCROLLCONTENTCTRL_H_
