// HH: Added for Tribes 2 compatibility.
#ifndef _HUDCTRL_H_
#define _HUDCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

/// @file
/// The base every piece of Tribes 2's in-game HUD is built on.
///
/// A HUD control is drawn over the world rather than over a page, so what it
/// shares is not a frame and a fill but a way of being *faint*: an opacity
/// every piece is drawn through, a fill and a frame colour under it, and the
/// option to flip or centre the art inside the control.  Those six fields
/// appear on nearly every Hud* class in the .gui files, which is what says
/// they belong here rather than in each of them.
///
/// The split between float and byte colours runs *through* the HUD rather than
/// around it, and it is not a matter of taste: `fillColor` and `frameColor`
/// arrive as floats -- "0.353000 0.373000 0.933000 0.800000" -- in all 214
/// uses across the shipped .gui files, and every other colour field on a Hud
/// class arrives as bytes, "3 255 110 255".  Declaring one of the byte ones
/// TypeColorF does not merely dim it: ColorF::operator ColorI multiplies by
/// 255 without clamping, so 3,255,110 comes back 253,1,146 and the sensor
/// ping at the centre of the compass draws magenta.  It read as a missing
/// texture and was not one.

class HudCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   ColorF mFillColor;
   ColorF mFrameColor;
   F32    mOpacity;

   bool mAutoCenter;
   bool mAutoResize;
   bool mFlipHorizontal;
   bool mFlipVertical;

   StringTableEntry mBitmapName;
   GFXTexHandle mTexture;

   /// The flip flags as the draw utility wants them.
   GFXBitmapFlip flipFlags() const;

   RectI artRect(const Point2I &offset, const Point2I &size) const;

   /// `colour` faded by this control's opacity, in either convention.
   ColorI faded(const ColorF &colour) const;
   ColorI faded(const ColorI &colour) const;

public:
   HudCtrl();

   DECLARE_CONOBJECT(HudCtrl);
   static void initPersistFields();

   bool onWake();

   void onPreRender();

   void setBitmap(const char *name);
   GFXTexHandle getTexture() const { return mTexture; }

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// A bitmap on the HUD, and nothing else.
class HudBitmapCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   DECLARE_CONOBJECT(HudBitmapCtrl);
};

//-----------------------------------------------------------------------------
/// A bitmap that pulses, and changes colour to say the sensor is jammed or
/// pinging.
class HudPulsingBitmap : public HudCtrl
{
   typedef HudCtrl Parent;

   ColorI mColor;
   ColorI mJamColor;
   ColorI mPingColor;

   bool mJam;
   bool mPing;
   F32  mPulse;
   F32  mPulseRate;

   /// Where in the pulse we are, in milliseconds since it started.
   U32 mPulseStart;

public:
   HudPulsingBitmap();

   DECLARE_CONOBJECT(HudPulsingBitmap);
   static void initPersistFields();

   /// Restart the pulse, which is what script calls this for.
   void update();

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The reticle.
class HudCrosshair : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   HudCrosshair();

   DECLARE_CONOBJECT(HudCrosshair);

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The match clock, counting whatever script last set.
class HudClock : public HudCtrl
{
   typedef HudCtrl Parent;

   RectI mSubRegion;

   /// Seconds on the clock, and when that was true.
   S32 mTime;
   U32 mSetAt;

public:
   HudClock();

   DECLARE_CONOBJECT(HudClock);
   static void initPersistFields();

   /// Set the clock going from `seconds`.  A negative value stops it.
   void setTime(S32 seconds);

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// Containers the HUD groups things in.  A fancy one draws a panel behind its
/// children; a horizontal one only lays them out.
class HudFancyCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   HudFancyCtrl();

   DECLARE_CONOBJECT(HudFancyCtrl);
};

class HudHorzCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   DECLARE_CONOBJECT(HudHorzCtrl);

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The little balloon that explains what a control is for.
///
/// The most-used control in Tribes 2's whole layout -- seventy-two of them in
/// PlayGui alone -- because every screen carries one per thing that can be
/// pointed at.  It draws only when script has given it something to say.
class HudHelpTag : public HudCtrl
{
   typedef HudCtrl Parent;

   bool mFlip;
   StringTableEntry mText;

public:
   HudHelpTag();

   DECLARE_CONOBJECT(HudHelpTag);
   static void initPersistFields();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDCTRL_H_
