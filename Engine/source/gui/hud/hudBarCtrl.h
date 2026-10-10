// HH: Added for Tribes 2 compatibility.
#ifndef _HUDBARCTRL_H_
#define _HUDBARCTRL_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif

class Camera;
class ShapeBase;

/// @file
/// The gauges down the left of a Tribes 2 screen.
///
/// Energy, damage and heat are one control with three names: the same fill,
/// the same pulse when the number they watch passes a line, the same option to
/// run up rather than across.  What differs is which number they watch, and
/// each of them goes and reads it: nothing in Tribes 2's shipped scripts ever
/// sets one of these bars.  Retail asks the control for its value once per
/// render, through a virtual of the bar's own (vtable +0x110, called at
/// 0x00500f26), and each subclass answers from the connection's control
/// object -- or, where that object is the observer camera, from the player or
/// vehicle the camera is watching.
///
/// `value`, `pulseThreshold` and `verticalFill` are read by nothing in retail
/// but the base class's own answer, which is what the two net-traffic bars in
/// PlayGui.gui use; they are kept because the .gui files set them.

class HudBarBaseCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

protected:
   F32 mValue;
   F32 mPulseRate;
   F32 mPulseThreshold;

   bool mVerticalFill;
   bool mDisplayMounted;

      /// Retail decides this inside barValue (0x00501959 for damage,
      /// 0x00501b7d for heat).
   bool mWarning;

   RectI mSubRegion;

   /// How bright the bar should be drawn this instant.
   ///
   F32 pulse() const;

   static ShapeBase *controlObject();

   static ShapeBase *watchedObject(Camera *camera);

   static ShapeBase *mountedObject(ShapeBase *object);

public:
   HudBarBaseCtrl();

   DECLARE_CONOBJECT(HudBarBaseCtrl);
   static void initPersistFields();

   virtual F32 barValue();

   void setValue(F32 value);

   void setScriptValue(const char *value);
   const char *getScriptValue();

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
class HudEnergy : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;
public:
   DECLARE_CONOBJECT(HudEnergy);

   F32 barValue();
};

class HudDamage : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;
public:
   DECLARE_CONOBJECT(HudDamage);

   F32 barValue();
};

/// Heat has one field of its own: the level at which it starts warning.
class HudHeat : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;

   F32 mHeatWarning;

public:
   HudHeat();

   DECLARE_CONOBJECT(HudHeat);
   static void initPersistFields();

   F32 barValue();
};

//-----------------------------------------------------------------------------
/// The frame drawn around the view while a weapon is zoomed.
class HudZoom : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   HudZoom();

   DECLARE_CONOBJECT(HudZoom);
};

#endif // _HUDBARCTRL_H_
