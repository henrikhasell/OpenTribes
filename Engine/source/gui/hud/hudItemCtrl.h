// HH: Added for Tribes 2 compatibility.
#ifndef _HUDITEMCTRL_H_
#define _HUDITEMCTRL_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

/// @file
/// The two rows of icons a Tribes 2 player reads mid-fight.
///
/// The inventory along one edge and the weapons along another are the same
/// control: a row of slots, each an icon with a count under it, one of them
/// lit as the one in hand.  What script does with them is identical too --
/// clear the row, add a slot, give it a bitmap, set its number, say which is
/// active -- so they are one class with two names and two vocabularies for
/// the same calls, because the scripts use both.

//-----------------------------------------------------------------------------
class HudItemCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

protected:
   /// A slot holds the *name* of its icon rather than a handle to it.
   ///
   /// Torque's Vector never constructs what it holds -- it grows with realloc
   /// and assigns over whatever was in the new memory -- so a GFXTexHandle in
   /// one takes its assignment operator into uninitialised bytes and unlocks a
   /// pointer that was never a texture.  The texture manager caches by name
   /// anyway, so looking the icon up as it is drawn costs a hash lookup and
   /// keeps the slot a plain old struct.
   struct Slot
   {
      StringTableEntry name;      ///< What script calls this slot.
      StringTableEntry bitmap;
      S32 amount;                 ///< -1 for "do not draw a number".
      bool infinite;
   };

   Vector<Slot> mSlots;
   S32 mActive;

   StringTableEntry mBackgroundBitmap;
   StringTableEntry mHighLightBitmap;
   StringTableEntry mInfiniteBitmap;

   GFXTexHandle mBackground;
   GFXTexHandle mHighLight;
   GFXTexHandle mInfinite;

   Point2I mIconOffset;
   Point2I mAmmoOffset;
   Point2I mHighLightOffset;
   Point2I mBackGroundOffset;

   /// The gap between two slots, on top of however big a slot is.  A single
   /// number in the .gui files -- "backGroundSpace = 2" -- not a point.
   S32 mBackGroundSpace;

   /// How far apart two slots start, measured off the background art.
   S32 slotStep(bool vertical) const;

   /// Where slot `index` is drawn, relative to the control.
   RectI slotRect(S32 index) const;

   S32 findSlot(const char *name) const;

public:
   HudItemCtrl();

   DECLARE_CONOBJECT(HudItemCtrl);
   static void initPersistFields();

   bool onWake();

   //-- what script does to a row -------------------------------------------
   S32  addSlot(const char *name);
   void removeSlot(const char *name);
   void clearAll();

   void setSlotBitmap(const char *name, const char *bitmap);
   void setSlotAmount(const char *name, S32 amount, bool infinite = false);
   void setActive(const char *name);

   S32 getSlotCount() const { return mSlots.size(); }
   const char *getSlotId(S32 index) const;
   const char *getSlotBitmap(S32 index) const;
   const char *getActiveItem() const;

   void setBackgroundBitmap(const char *name);
   void setHighLightBitmap(const char *name);
   void setInfiniteBitmap(const char *name);

   const char *getBackgroundBitmap() const { return mBackgroundBitmap; }
   const char *getHighLightBitmap() const  { return mHighLightBitmap; }
   const char *getInfiniteBitmap() const   { return mInfiniteBitmap; }

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The packs and grenades row.
class HudInventory : public HudItemCtrl
{
   typedef HudItemCtrl Parent;
public:
   DECLARE_CONOBJECT(HudInventory);
};

/// The weapons row.
class HudWeapons : public HudItemCtrl
{
   typedef HudItemCtrl Parent;
public:
   DECLARE_CONOBJECT(HudWeapons);
};

/// The weapons row a vehicle's gunner sees.
class HudVehicleWeapon : public HudItemCtrl
{
   typedef HudItemCtrl Parent;
public:
   DECLARE_CONOBJECT(HudVehicleWeapon);
};

//-----------------------------------------------------------------------------
/// The objective panel: the two team names, their scores and where the flags
/// are.
///
/// Every field on it is a tagged string id rather than the text itself, which
/// is how a server that has already sent the string sends the line that uses
/// it in a few bits.  A container, because the layout nests the labels inside.
class HudScoreCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

   Point2I mVerticalInset;
   Point2I mHorizontalInset;

   bool mHorzSeparator;

public:
   HudScoreCtrl();

   DECLARE_CONOBJECT(HudScoreCtrl);
   static void initPersistFields();

   void setSeparators(bool on) { mHorzSeparator = on; setUpdate(); }

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The connection graph -- ping, packet loss and the rest -- that Tribes 2
/// draws over the corner of the screen when it is asked to.
class HudNetDisplay : public HudCtrl
{
   typedef HudCtrl Parent;

   StringTableEntry mInfoCallback;
   S32 mHistorySize;
   S32 mUpdatePeriod;
   bool mRenderGraph;

   Vector<F32> mHistory;
   U32 mLastSample;

public:
   HudNetDisplay();

   DECLARE_CONOBJECT(HudNetDisplay);
   static void initPersistFields();

   void init();
   void uninit();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDITEMCTRL_H_
