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
/// Energy, damage and heat are one control with three names, which retail asks
/// for its value once per render through a virtual of its own (vtable +0x110,
/// called at 0x00500f26).

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

   virtual ColorI barColor();

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
   ColorI barColor();
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

/// The bar under a bomber's or a tank's energy gauge (vCapBar), filled from
/// the capacitor of the turret mounted on the vehicle.
class HudCapacitor : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;
public:
   DECLARE_CONOBJECT(HudCapacitor);

   F32 barValue();
};

//-----------------------------------------------------------------------------
/// The corners that show how much of the view the next zoom level will fill.
/// calcZoomFOV sizes it and shows it for five seconds each time the level is
/// cycled while not zoomed in.
class HudZoom : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   HudZoom();

   DECLARE_CONOBJECT(HudZoom);

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDBARCTRL_H_
