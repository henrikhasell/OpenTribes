// HH: Added for Tribes 2 compatibility.
#ifndef _HUDBARCTRL_H_
#define _HUDBARCTRL_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif

/// @file
/// The gauges down the left of a Tribes 2 screen.
///
/// Energy, damage and heat are one control with three names: the same fill,
/// the same pulse when the value crosses a threshold, the same option to run
/// up rather than across.  What differs is which number they watch, and that
/// is script's business -- the server tells the client, the client calls
/// setValue, the bar draws it.

class HudBarBaseCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

protected:
   F32 mValue;
   F32 mPulseRate;
   F32 mPulseThreshold;

   bool mVerticalFill;
   bool mDisplayMounted;

   RectI mSubRegion;

   /// How bright the bar should be drawn this instant.
   ///
   /// One below the threshold, and a wave between nothing and one above it --
   /// which is what makes an energy bar flash when it is nearly empty.
   F32 pulse() const;

public:
   HudBarBaseCtrl();

   DECLARE_CONOBJECT(HudBarBaseCtrl);
   static void initPersistFields();

   void setValue(F32 value);
   F32 getValue() const { return mValue; }

   void setScriptValue(const char *value);
   const char *getScriptValue();

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The three the layouts name.  Nothing separates them but the name, which the
/// .gui files need to exist.
class HudEnergy : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;
public:
   DECLARE_CONOBJECT(HudEnergy);
};

class HudDamage : public HudBarBaseCtrl
{
   typedef HudBarBaseCtrl Parent;
public:
   DECLARE_CONOBJECT(HudDamage);
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
