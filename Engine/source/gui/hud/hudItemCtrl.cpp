#include "gui/hud/hudItemCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "platform/platform.h"
#include "sim/netConnection.h"

IMPLEMENT_CONOBJECT(HudItemCtrl);
IMPLEMENT_CONOBJECT(HudInventory);
IMPLEMENT_CONOBJECT(HudWeapons);
IMPLEMENT_CONOBJECT(HudVehicleWeapon);
IMPLEMENT_CONOBJECT(HudScoreCtrl);
IMPLEMENT_CONOBJECT(HudNetDisplay);

//-----------------------------------------------------------------------------
/// A texture by name, or an empty handle.
///
/// The slots hold names rather than handles for the reason the header gives,
/// so the icon is looked up as it is drawn.  The texture manager caches by
/// name, so this is a hash lookup rather than a load.
static GFXTexHandle texture(StringTableEntry name)
{
   if(!name || !name[0])
      return GFXTexHandle();

   return GFXTexHandle(name, &GFXDefaultGUIProfile,
                       avar("%s() - texture (line %d)", __FUNCTION__, __LINE__));
}


//-----------------------------------------------------------------------------
HudItemCtrl::HudItemCtrl()
{
   VECTOR_SET_ASSOCIATION(mSlots);

   mActive = -1;

   mBackgroundBitmap = StringTable->insert("");
   mHighLightBitmap  = StringTable->insert("");
   mInfiniteBitmap   = StringTable->insert("");

   mIconOffset.set(0, 0);
   mAmmoOffset.set(0, 0);
   mHighLightOffset.set(0, 0);
   mBackGroundOffset.set(0, 0);
   mBackGroundSpace = 2;
}

void HudItemCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("iconOffset",       TypePoint2I, Offset(mIconOffset, HudItemCtrl));
   addField("ammoOffset",       TypePoint2I, Offset(mAmmoOffset, HudItemCtrl));
   addField("highLightOffset",  TypePoint2I, Offset(mHighLightOffset, HudItemCtrl));
   addField("backGroundOffset", TypePoint2I, Offset(mBackGroundOffset, HudItemCtrl));
   addField("backGroundSpace",  TypeS32,     Offset(mBackGroundSpace, HudItemCtrl));
}

bool HudItemCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   setBackgroundBitmap(mBackgroundBitmap);
   setHighLightBitmap(mHighLightBitmap);
   setInfiniteBitmap(mInfiniteBitmap);
   return true;
}

S32 HudItemCtrl::findSlot(const char *name) const
{
   StringTableEntry key = StringTable->insert(name ? name : "");

   for(S32 i = 0; i < S32(mSlots.size()); i++)
      if(mSlots[i].name == key)
         return i;

   return -1;
}

S32 HudItemCtrl::addSlot(const char *name)
{
   S32 at = findSlot(name);
   if(at != -1)
      return at;

   Slot slot;
   slot.name = StringTable->insert(name ? name : "");
   slot.bitmap = StringTable->insert("");
   slot.amount = -1;
   slot.infinite = false;

   mSlots.push_back(slot);
   setUpdate();
   return mSlots.size() - 1;
}

void HudItemCtrl::removeSlot(const char *name)
{
   const S32 at = findSlot(name);
   if(at == -1)
      return;

   mSlots.erase(at);

   // Keep pointing at the same item rather than the same index: dropping a
   // weapon must not silently select the one that shuffled into its place.
   if(mActive == at)
      mActive = -1;
   else if(mActive > at)
      mActive--;

   setUpdate();
}

void HudItemCtrl::clearAll()
{
   mSlots.clear();
   mActive = -1;
   setUpdate();
}

void HudItemCtrl::setSlotBitmap(const char *name, const char *bitmap)
{
   const S32 at = addSlot(name);

   mSlots[at].bitmap = StringTable->insert(bitmap ? bitmap : "", true);
   setUpdate();
}

void HudItemCtrl::setSlotAmount(const char *name, S32 amount, bool infinite)
{
   const S32 at = addSlot(name);
   mSlots[at].amount = amount;
   mSlots[at].infinite = infinite;
   setUpdate();
}

void HudItemCtrl::setActive(const char *name)
{
   mActive = findSlot(name);
   setUpdate();
}

const char *HudItemCtrl::getSlotId(S32 index) const
{
   return (index >= 0 && index < S32(mSlots.size())) ? mSlots[index].name : "";
}

const char *HudItemCtrl::getSlotBitmap(S32 index) const
{
   return (index >= 0 && index < S32(mSlots.size())) ? mSlots[index].bitmap : "";
}

const char *HudItemCtrl::getActiveItem() const
{
   return getSlotId(mActive);
}

void HudItemCtrl::setBackgroundBitmap(const char *name)
{
   mBackgroundBitmap = StringTable->insert(name ? name : "", true);
   mBackground = mBackgroundBitmap[0] ? texture(mBackgroundBitmap)
                                      : GFXTexHandle();
   setUpdate();
}

void HudItemCtrl::setHighLightBitmap(const char *name)
{
   mHighLightBitmap = StringTable->insert(name ? name : "", true);
   mHighLight = mHighLightBitmap[0] ? texture(mHighLightBitmap)
                                    : GFXTexHandle();
   setUpdate();
}

void HudItemCtrl::setInfiniteBitmap(const char *name)
{
   mInfiniteBitmap = StringTable->insert(name ? name : "", true);
   mInfinite = mInfiniteBitmap[0] ? texture(mInfiniteBitmap)
                                  : GFXTexHandle();
   setUpdate();
}

S32 HudItemCtrl::slotStep(bool vertical) const
{
   // A slot is as big as the plate drawn behind it, and backGroundSpace is the
   // gap left between two of them.  With no plate the icons are the slots, and
   // the control's short side is how big one of those is.
   S32 size = vertical ? getExtent().x : getExtent().y;

   if(mBackground)
      size = vertical ? mBackground->getBitmapHeight() : mBackground->getBitmapWidth();

   return getMax(size + mBackGroundSpace, 1);
}

RectI HudItemCtrl::slotRect(S32 index) const
{
   // The row runs whichever way the control is longer.  Tribes 2 has the
   // inventory down one edge and the weapons across another, and neither says
   // which it is beyond its own shape.
   const bool vertical = getExtent().y > getExtent().x;
   const S32 step = slotStep(vertical);

   return vertical
      ? RectI(0, index * step, getExtent().x, step)
      : RectI(index * step, 0, step, getExtent().y);
}

void HudItemCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const U8 alpha = U8(mClampF(mOpacity, 0.f, 1.f) * 255.f);

   for(S32 i = 0; i < S32(mSlots.size()); i++)
   {
      RectI slot = slotRect(i);
      slot.point += offset;

      draw->setBitmapModulation(ColorI(255, 255, 255, alpha));

      if(mBackground)
         draw->drawBitmap(mBackground, slot.point + mBackGroundOffset);

      if(i == mActive && mHighLight)
         draw->drawBitmap(mHighLight, slot.point + mHighLightOffset);

      if(mSlots[i].bitmap[0])
      {
         GFXTexHandle icon = texture(mSlots[i].bitmap);
         if(icon)
            draw->drawBitmap(icon, slot.point + mIconOffset);
      }

      // A count of -1 is "this slot has no number", which is how a pack
      // differs from a magazine.
      if(mSlots[i].infinite && mInfinite)
         draw->drawBitmap(mInfinite, slot.point + mAmmoOffset);
      else if(mSlots[i].amount >= 0 && mProfile->mFont)
      {
         char buf[16];
         dSprintf(buf, sizeof(buf), "%d", mSlots[i].amount);

         draw->setBitmapModulation(mProfile->mFontColor);
         draw->drawText(mProfile->mFont, slot.point + mAmmoOffset, buf);
      }

      draw->clearBitmapModulation();
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudScoreCtrl::HudScoreCtrl()
{
   mVerticalInset.set(0, 0);
   mHorizontalInset.set(0, 0);
   mHorzSeparator = false;
}

void HudScoreCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("verticalInset",   TypePoint2I, Offset(mVerticalInset, HudScoreCtrl));
   addField("horizontalInset", TypePoint2I, Offset(mHorizontalInset, HudScoreCtrl));
}

void HudScoreCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());

   draw->drawRectFill(bounds, faded(mFillColor));

   if(mHorzSeparator)
   {
      const S32 y = offset.y + getExtent().y / 2;
      draw->drawLine(offset.x + mHorizontalInset.x, y,
                  offset.x + getExtent().x - mHorizontalInset.y, y,
                  faded(mFrameColor));
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudNetDisplay::HudNetDisplay()
{
   VECTOR_SET_ASSOCIATION(mHistory);

   mInfoCallback = StringTable->insert("");
   mHistorySize = 64;
   mUpdatePeriod = 500;
   mRenderGraph = true;
   mLastSample = 0;
}

void HudNetDisplay::initPersistFields()
{
   Parent::initPersistFields();

   addField("infoCallback", TypeCaseString, Offset(mInfoCallback, HudNetDisplay));
   addField("historySize",  TypeS32,        Offset(mHistorySize, HudNetDisplay));
   addField("updatePeriod", TypeS32,        Offset(mUpdatePeriod, HudNetDisplay));
   addField("renderGraph",  TypeBool,       Offset(mRenderGraph, HudNetDisplay));
}

void HudNetDisplay::init()
{
   mHistory.clear();
   mLastSample = 0;
}

void HudNetDisplay::uninit()
{
   mHistory.clear();
}

void HudNetDisplay::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());
   draw->drawRectFill(bounds, faded(mFillColor));

   // One sample per period, of whatever the connection says the round trip is.
   const U32 now = Platform::getVirtualMilliseconds();
   if(mUpdatePeriod > 0 && now - mLastSample >= U32(mUpdatePeriod))
   {
      mLastSample = now;

      F32 ping = 0.f;
      if(NetConnection *conn = NetConnection::getConnectionToServer())
         ping = conn->getRoundTripTime();

      mHistory.push_back(ping);
      while(S32(mHistory.size()) > mHistorySize)
         mHistory.pop_front();
   }

   if(mRenderGraph && mHistory.size() > 1)
   {
      // Scaled to the worst sample in the window, so the shape of the trace
      // stays readable whatever the connection is doing.
      F32 worst = 1.f;
      for(S32 i = 0; i < S32(mHistory.size()); i++)
         worst = getMax(worst, mHistory[i]);

      const ColorI line = faded(mFrameColor);
      const S32 step = getMax(getExtent().x / mHistorySize, 1);

      for(S32 i = 1; i < S32(mHistory.size()); i++)
      {
         const S32 x0 = offset.x + (i - 1) * step;
         const S32 x1 = offset.x + i * step;
         const S32 y0 = offset.y + getExtent().y
                      - S32(mHistory[i - 1] / worst * F32(getExtent().y));
         const S32 y1 = offset.y + getExtent().y
                      - S32(mHistory[i] / worst * F32(getExtent().y));

         draw->drawLine(x0, y0, x1, y1, line);
      }
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
// The script interface.
//
// Tribes 2's scripts call the same operations by two sets of names -- the
// inventory row's and the weapons row's -- on what is one control, so each is
// registered under both.  The duplicates are not tidiness waiting to happen:
// scripts/hud.cs uses one vocabulary and the shell screens use the other.

DefineEngineMethod(HudItemCtrl, clearAll, void, (), ,
   "@brief Empty the row.\n\n@ingroup GuiHud")
{
   object->clearAll();
}

DefineEngineMethod(HudItemCtrl, reset, void, (), ,
   "@brief Empty the row.\n\n@ingroup GuiHud")
{
   object->clearAll();
}

// The amount is optional because Tribes 2 sends both forms:
// clientCmdSetWeaponsHudItem and clientCmdSetInventoryHudItem pass the slot
// *and* what is in it, while the screens that only lay a row out name the slot
// alone.  A slot that is already there keeps its count when it is named again,
// which is why this adds first and only then sets.
DefineEngineMethod(HudItemCtrl, addInventory, void,
   (const char *item, S32 amount), (-1),
   "@brief Add an inventory slot, optionally with a count.\n\n@ingroup GuiHud")
{
   object->addSlot(item);
   if(amount != -1)
      object->setSlotAmount(item, amount);
}

DefineEngineMethod(HudItemCtrl, addWeapon, void,
   (const char *weapon, S32 ammo), (-1),
   "@brief Add a weapon slot, optionally with an ammo count.\n\n@ingroup GuiHud")
{
   object->addSlot(weapon);
   if(ammo != -1)
      object->setSlotAmount(weapon, ammo);
}

DefineEngineMethod(HudItemCtrl, removeInventory, void, (const char *item), ,
   "@brief Drop a slot.\n\n@ingroup GuiHud")
{
   object->removeSlot(item);
}

DefineEngineMethod(HudItemCtrl, removeWeapon, void, (const char *weapon), ,
   "@brief Drop a slot.\n\n@ingroup GuiHud")
{
   object->removeSlot(weapon);
}

DefineEngineMethod(HudItemCtrl, setInventoryBitmap, void,
   (const char *item, const char *bitmap), ,
   "@brief Give a slot its icon.\n\n@ingroup GuiHud")
{
   object->setSlotBitmap(item, bitmap);
}

DefineEngineMethod(HudItemCtrl, setWeaponBitmap, void,
   (const char *weapon, const char *bitmap), ,
   "@brief Give a slot its icon.\n\n@ingroup GuiHud")
{
   object->setSlotBitmap(weapon, bitmap);
}

DefineEngineMethod(HudItemCtrl, setAmount, void, (const char *item, S32 amount), ,
   "@brief Set the number under a slot.\n\n@ingroup GuiHud")
{
   object->setSlotAmount(item, amount);
}

DefineEngineMethod(HudItemCtrl, setAmmo, void,
   (const char *weapon, S32 amount, bool infinite), (false),
   "@brief Set the ammo under a slot.\n\n@ingroup GuiHud")
{
   object->setSlotAmount(weapon, amount, infinite);
}

DefineEngineMethod(HudItemCtrl, setActiveInventory, void, (const char *item), ,
   "@brief Light this slot as the one in hand.\n\n@ingroup GuiHud")
{
   object->setActive(item);
}

DefineEngineMethod(HudItemCtrl, setActiveWeapon, void, (const char *weapon), ,
   "@brief Light this slot as the one in hand.\n\n@ingroup GuiHud")
{
   object->setActive(weapon);
}

DefineEngineMethod(HudItemCtrl, getActiveItem, const char *, (), ,
   "@brief Which slot is lit.\n\n@ingroup GuiHud")
{
   return object->getActiveItem();
}

DefineEngineMethod(HudItemCtrl, getSlotCount, S32, (), ,
   "@brief How many slots the row has.\n\n@ingroup GuiHud")
{
   return object->getSlotCount();
}

DefineEngineMethod(HudItemCtrl, getNumSlots, S32, (), ,
   "@brief How many slots the row has.\n\n@ingroup GuiHud")
{
   return object->getSlotCount();
}

DefineEngineMethod(HudItemCtrl, getNumItems, S32, (), ,
   "@brief How many slots the row has.\n\n@ingroup GuiHud")
{
   return object->getSlotCount();
}

DefineEngineMethod(HudItemCtrl, getSlotId, const char *, (S32 index), ,
   "@brief What script calls the slot at this index.\n\n@ingroup GuiHud")
{
   return object->getSlotId(index);
}

DefineEngineMethod(HudItemCtrl, getItemBitmap, const char *, (S32 index), ,
   "@brief The icon of the slot at this index.\n\n@ingroup GuiHud")
{
   return object->getSlotBitmap(index);
}

// HH: Added for Tribes 2 compatibility.
// hud.cs spells this setBackGroundBitmap, which the console resolves to the
// same name; a second binding for it is what a debug build asserts on.
DefineEngineMethod(HudItemCtrl, setBackgroundBitmap, void, (const char *bitmap), ,
   "@brief The plate drawn behind every slot.\n\n@ingroup GuiHud")
{
   object->setBackgroundBitmap(bitmap);
}

DefineEngineMethod(HudItemCtrl, getBackgroundBitmap, const char *, (), ,
   "@brief The plate drawn behind every slot.\n\n@ingroup GuiHud")
{
   return object->getBackgroundBitmap();
}

DefineEngineMethod(HudItemCtrl, setHighLightBitmap, void, (const char *bitmap), ,
   "@brief The mark on the slot in hand.\n\n@ingroup GuiHud")
{
   object->setHighLightBitmap(bitmap);
}

DefineEngineMethod(HudItemCtrl, getHighLightBitmap, const char *, (), ,
   "@brief The mark on the slot in hand.\n\n@ingroup GuiHud")
{
   return object->getHighLightBitmap();
}

DefineEngineMethod(HudItemCtrl, setInfiniteAmmoBitmap, void, (const char *bitmap), ,
   "@brief Drawn instead of a number when a slot never runs out.\n\n@ingroup GuiHud")
{
   object->setInfiniteBitmap(bitmap);
}

DefineEngineMethod(HudItemCtrl, setInfiniteAmountBitmap, void, (const char *bitmap), ,
   "@brief Drawn instead of a number when a slot never runs out.\n\n@ingroup GuiHud")
{
   object->setInfiniteBitmap(bitmap);
}

DefineEngineMethod(HudItemCtrl, getInfiniteBitmap, const char *, (), ,
   "@brief Drawn instead of a number when a slot never runs out.\n\n@ingroup GuiHud")
{
   return object->getInfiniteBitmap();
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudScoreCtrl, setSeparators, void, (bool on), ,
   "@brief Draw the rule between the two teams.\n\n@ingroup GuiHud")
{
   object->setSeparators(on);
}

DefineEngineMethod(HudScoreCtrl, enableHorzSeparator, void, (), ,
   "@brief Draw the rule between the two teams.\n\n@ingroup GuiHud")
{
   object->setSeparators(true);
}

DefineEngineMethod(HudScoreCtrl, disableHorzSeparator, void, (), ,
   "@brief Stop drawing the rule between the two teams.\n\n@ingroup GuiHud")
{
   object->setSeparators(false);
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudNetDisplay, init, void, (), ,
   "@brief Start the trace over.\n\n@ingroup GuiHud")
{
   object->init();
}

DefineEngineMethod(HudNetDisplay, uninit, void, (), ,
   "@brief Throw the trace away.\n\n@ingroup GuiHud")
{
   object->uninit();
}

DefineEngineMethod(HudNetDisplay, updateToggles, void, (), ,
   "@brief Re-read what to show.\n\n@ingroup GuiHud")
{
   object->setUpdate();
}

DefineEngineMethod(HudNetDisplay, getPrefs, const char *, (), ,
   "@brief What is being shown.  Nothing reads this yet.\n\n@ingroup GuiHud")
{
   return "";
}
