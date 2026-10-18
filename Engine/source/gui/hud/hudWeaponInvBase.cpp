// HH: Added for Tribes 2 compatibility.
#include "gui/hud/hudWeaponInvBase.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"

IMPLEMENT_CONOBJECT(HudWeaponInvBase);
IMPLEMENT_CONOBJECT(HudWeapons);

/// $WeaponsHudData's TargetingLaser, which Tribes 2 keeps at the bottom of
/// the column however late it arrives (0x004ff624).
static const S32 sTargetingLaser = 9;

//-----------------------------------------------------------------------------
HudWeaponInvBase::HudWeaponInvBase()
{
   VECTOR_SET_ASSOCIATION(mSlots);

   for(S32 i = 0; i < MaxItems; i++)
      mItemBitmapNames[i] = StringTable->insert("");

   mNumItems = 0;
   mActive = -1;

   mBackgroundName = StringTable->insert("");
   mHighLightName  = StringTable->insert("");
   mInfiniteName   = StringTable->insert("");

   mAmmoOffset.set(0, 0);
   mBackGroundOffset.set(0, 0);
   mHighLightOffset.set(0, 0);
   mIconOffset.set(0, 0);
   mBackGroundSpace = 0;

   mCellSize.set(0, 0);
   mCellOffset.set(0, 0);
}

void HudWeaponInvBase::initPersistFields()
{
   Parent::initPersistFields();

   addField("ammoOffset",       TypePoint2I, Offset(mAmmoOffset, HudWeaponInvBase));
   addField("backGroundOffset", TypePoint2I, Offset(mBackGroundOffset, HudWeaponInvBase));
   addField("highLightOffset",  TypePoint2I, Offset(mHighLightOffset, HudWeaponInvBase));
   addField("iconOffset",       TypePoint2I, Offset(mIconOffset, HudWeaponInvBase));
   addField("backGroundSpace",  TypeS32,     Offset(mBackGroundSpace, HudWeaponInvBase));
}

GFXTexHandle HudWeaponInvBase::loadArt(const char *name, StringTableEntry &outName)
{
   outName = StringTable->insert(name ? name : "", true);
   if(!outName[0])
      return GFXTexHandle();

   return GFXTexHandle(outName, &GFXDefaultGUIProfile,
                       avar("%s() - art (line %d)", __FUNCTION__, __LINE__));
}

/// 0x004fff00: the cell grows, axis by axis, to the furthest any piece of art
/// reaches, and remembers the offset of the piece that reached it.
void HudWeaponInvBase::fitCell(GFXTexHandle &art, const Point2I &offset)
{
   const S32 w = art ? art->getBitmapWidth() : 0;
   const S32 h = art ? art->getBitmapHeight() : 0;

   if(w + offset.x > mCellSize.x + mCellOffset.x)
   {
      mCellSize.x = w;
      mCellOffset.x = offset.x;
   }

   if(h + offset.y > mCellSize.y + mCellOffset.y)
   {
      mCellSize.y = h;
      mCellOffset.y = offset.y;
   }
}

S32 HudWeaponInvBase::findSlot(S32 item) const
{
   for(S32 i = 0; i < S32(mSlots.size()); i++)
      if(mSlots[i].item == item)
         return i;

   return -1;
}

//-----------------------------------------------------------------------------
/// 0x004ffb40: the first icon given for an index stays, and every one taken
/// counts towards mNumItems whether its art loaded or not.
void HudWeaponInvBase::setItemBitmap(S32 item, const char *name)
{
   if(!name || !name[0] || item < 0 || item >= MaxItems)
      return;

   if(mItemBitmaps[item] || mNumItems >= MaxItems)
      return;

   mItemBitmaps[item] = loadArt(name, mItemBitmapNames[item]);
   fitCell(mItemBitmaps[item], mIconOffset);
   mNumItems++;
}

const char *HudWeaponInvBase::getItemBitmap(S32 item) const
{
   if(item < 0 || item >= mNumItems || !mItemBitmaps[item])
      return "";

   return mItemBitmapNames[item];
}

/// 0x004ff5d0.  The control is resized to fit only here; dropping a weapon
/// leaves it as tall as it was.
void HudWeaponInvBase::addItem(S32 item, S32 count)
{
   if(item < 0 || item >= mNumItems || findSlot(item) != -1)
      return;

   Slot slot;
   slot.item = item;
   slot.count = count;

   if(!mSlots.empty() && mSlots.last().item == sTargetingLaser)
      mSlots.insert(mSlots.size() - 1, slot);
   else
      mSlots.push_back(slot);

   const Point2I extent = itemsExtent() + mCellOffset;
   if(extent != getExtent())
      resize(getPosition(), extent);
}

/// 0x004ff790
void HudWeaponInvBase::removeItem(S32 item)
{
   if(item >= mNumItems)
      return;

   const S32 at = findSlot(item);
   if(at == -1)
      return;

   mSlots.erase(at);

   if(mActive == item)
      mActive = -1;
}

/// 0x004ff880
void HudWeaponInvBase::setItemCount(S32 item, S32 count)
{
   if(item >= mNumItems)
      return;

   const S32 at = findSlot(item);
   if(at != -1)
      mSlots[at].count = count;
}

/// 0x004ff830: an index that is not in the list lights nothing.
void HudWeaponInvBase::setActiveItem(S32 item)
{
   mActive = (item < mNumItems && findSlot(item) != -1) ? item : -1;
}

S32 HudWeaponInvBase::getSlotId(S32 slot) const
{
   return (slot >= 0 && slot < S32(mSlots.size())) ? mSlots[slot].item : -1;
}

S32 HudWeaponInvBase::getSlotCount(S32 slot) const
{
   return (slot >= 0 && slot < S32(mSlots.size())) ? mSlots[slot].count : -1;
}

/// 0x004ffe00: the list goes, the icon table stays.
void HudWeaponInvBase::clearAll()
{
   mSlots.clear();
   mActive = -1;
}

/// 0x004ffe50: the list and the icon table both go.
void HudWeaponInvBase::reset()
{
   clearAll();

   for(S32 i = 0; i < MaxItems; i++)
   {
      mItemBitmaps[i] = NULL;
      mItemBitmapNames[i] = StringTable->insert("");
   }

   mNumItems = 0;
}

//-----------------------------------------------------------------------------
// 0x004ffc20, 0x004ffd60 and 0x004ffcc0.  The infinity mark is measured at
// iconOffset, not ammoOffset, which is where it is drawn.
void HudWeaponInvBase::setBackgroundBitmap(const char *name)
{
   mBackground = loadArt(name, mBackgroundName);
   fitCell(mBackground, mBackGroundOffset);
}

void HudWeaponInvBase::setHighLightBitmap(const char *name)
{
   mHighLight = loadArt(name, mHighLightName);
   fitCell(mHighLight, mHighLightOffset);
}

void HudWeaponInvBase::setInfiniteBitmap(const char *name)
{
   mInfinite = loadArt(name, mInfiniteName);
   fitCell(mInfinite, mIconOffset);
}

//-----------------------------------------------------------------------------
/// 0x004ff310.  An entry whose icon never loaded is skipped without leaving
/// a gap, and the highlight is drawn over the icon at the entry's corner;
/// highLightOffset only sizes the cell.
void HudWeaponInvBase::onRender(Point2I offset, const RectI &updateRect)
{
   if(mNumItems == 0)
   {
      renderChildControls(offset, updateRect);
      return;
   }

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const F32 alpha = mClampF(mOpacity, 0.f, 1.f);
   const ColorI plain(ColorF(1.f, 1.f, 1.f, alpha));
   const ColorI count(ColorF(0.631f, 0.827f, 0.996f, alpha));

   draw->setBitmapModulation(plain);

   Point2I cursor = offset;
   for(S32 i = 0; i < S32(mSlots.size()); i++)
   {
      const Slot &slot = mSlots[i];
      if(slot.item < 0 || slot.item >= mNumItems || !mItemBitmaps[slot.item])
         continue;

      if(mBackground)
         draw->drawBitmap(mBackground, cursor + mBackGroundOffset);

      draw->drawBitmap(mItemBitmaps[slot.item], cursor + mIconOffset);

      if(mHighLight && mActive == slot.item)
         draw->drawBitmap(mHighLight, cursor);

      if(slot.count >= 0)
      {
         char buf[10];
         dSprintf(buf, sizeof(buf), "%d", slot.count);

         // 0x004ff8c0: centred on a field three zeroes wide, except that one
         // digit sits one zero in rather than one and a half.
         if(GFont *font = mProfile->mFont)
         {
            const S32 digits = dStrlen(buf);
            const F32 shift = digits == 1 ? 1.f : 3.f / F32(digits) - 1.f;
            const Point2I centre(S32(F32(font->getStrWidth((const UTF8 *)"0")) * shift), 0);

            draw->setBitmapModulation(count);
            draw->drawText(font, cursor + mAmmoOffset + centre, buf);
            draw->setBitmapModulation(plain);
         }
      }
      else if(mInfinite)
         draw->drawBitmap(mInfinite, cursor + mAmmoOffset);

      advance(cursor);
   }

   draw->clearBitmapModulation();
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
/// 0x00500130
void HudWeapons::advance(Point2I &cursor)
{
   cursor.y += mBackGroundSpace + (mBackground ? S32(mBackground->getBitmapHeight()) : 0);
}

/// 0x00500160
Point2I HudWeapons::itemsExtent() const
{
   return Point2I(mCellSize.x, (mCellSize.y + mBackGroundSpace) * S32(mSlots.size()));
}

//-----------------------------------------------------------------------------
// The script interface, registered where Tribes 2 registers it: the queries
// recordings.cs saves a demo's HUD with on the base, and what hud.cs drives
// the column with on HudWeapons.

DefineEngineMethod(HudWeaponInvBase, getBackgroundBitmap, const char *, (), ,
   "@brief The plate drawn behind every entry.\n\n@ingroup GuiHud")
{
   return object->getBackgroundBitmap();
}

DefineEngineMethod(HudWeaponInvBase, getHighLightBitmap, const char *, (), ,
   "@brief The mark on the entry in hand.\n\n@ingroup GuiHud")
{
   return object->getHighLightBitmap();
}

DefineEngineMethod(HudWeaponInvBase, getInfiniteBitmap, const char *, (), ,
   "@brief Drawn instead of a number when an entry never runs out.\n\n@ingroup GuiHud")
{
   return object->getInfiniteBitmap();
}

DefineEngineMethod(HudWeaponInvBase, getNumItems, S32, (), ,
   "@brief How many icons the table holds.\n\n@ingroup GuiHud")
{
   return object->getNumItems();
}

DefineEngineMethod(HudWeaponInvBase, getItemBitmap, const char *, (S32 idx), ,
   "@brief The icon at this index of the table.\n\n@ingroup GuiHud")
{
   return object->getItemBitmap(idx);
}

DefineEngineMethod(HudWeaponInvBase, getActiveItem, S32, (), ,
   "@brief The index lit as the one in hand, or -1.\n\n@ingroup GuiHud")
{
   return object->getActiveItem();
}

DefineEngineMethod(HudWeaponInvBase, getNumSlots, S32, (), ,
   "@brief How many entries the list has.\n\n@ingroup GuiHud")
{
   return object->getNumSlots();
}

DefineEngineMethod(HudWeaponInvBase, getSlotId, S32, (S32 slot), ,
   "@brief The table index of this entry.\n\n@ingroup GuiHud")
{
   return object->getSlotId(slot);
}

DefineEngineMethod(HudWeaponInvBase, getSlotCount, S32, (S32 slot), ,
   "@brief The count under this entry.\n\n@ingroup GuiHud")
{
   return object->getSlotCount(slot);
}

DefineEngineMethod(HudWeaponInvBase, reset, void, (), ,
   "@brief Empty the list and the icon table.\n\n@ingroup GuiHud")
{
   object->reset();
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudWeapons, setWeaponBitmap, void, (S32 slot, const char *bitmap), ,
   "@brief Give the table its icon for this weapon.\n\n@ingroup GuiHud")
{
   object->setItemBitmap(slot, bitmap);
}

DefineEngineMethod(HudWeapons, setBackGroundBitmap, void, (const char *bitmap), ,
   "@brief The plate drawn behind every entry.\n\n@ingroup GuiHud")
{
   object->setBackgroundBitmap(bitmap);
}

DefineEngineMethod(HudWeapons, setHighLightBitmap, void, (const char *bitmap), ,
   "@brief The mark on the entry in hand.\n\n@ingroup GuiHud")
{
   object->setHighLightBitmap(bitmap);
}

DefineEngineMethod(HudWeapons, setInfiniteAmmoBitmap, void, (const char *bitmap), ,
   "@brief Drawn instead of a number when a weapon never runs out.\n\n@ingroup GuiHud")
{
   object->setInfiniteBitmap(bitmap);
}

DefineEngineMethod(HudWeapons, addWeapon, void, (S32 weaponNum, S32 ammoAmount), ,
   "@brief Put a weapon in the column with its ammo; -1 is infinite.\n\n@ingroup GuiHud")
{
   object->addItem(weaponNum, ammoAmount);
}

DefineEngineMethod(HudWeapons, removeWeapon, void, (S32 weaponNum), ,
   "@brief Take a weapon out of the column.\n\n@ingroup GuiHud")
{
   object->removeItem(weaponNum);
}

DefineEngineMethod(HudWeapons, setAmmo, void, (S32 weaponNum, S32 ammoCount), ,
   "@brief Set the ammo under a weapon.\n\n@ingroup GuiHud")
{
   object->setItemCount(weaponNum, ammoCount);
}

DefineEngineMethod(HudWeapons, setActiveWeapon, void, (S32 weaponNum), ,
   "@brief Light this weapon as the one in hand.\n\n@ingroup GuiHud")
{
   object->setActiveItem(weaponNum);
}

DefineEngineMethod(HudWeapons, clearAll, void, (), ,
   "@brief Empty the column, keeping the icon table.\n\n@ingroup GuiHud")
{
   object->clearAll();
}
