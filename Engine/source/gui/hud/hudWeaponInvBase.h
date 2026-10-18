// HH: Added for Tribes 2 compatibility.
#ifndef _HUDWEAPONINVBASE_H_
#define _HUDWEAPONINVBASE_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

/// @file
/// The column of weapons down the right of a Tribes 2 screen, and the class
/// Tribes 2 builds it on.
///
/// The control holds two things script fills separately.  When a mission
/// starts the server sends an icon for every weapon the game knows, by index
/// into $WeaponsHudData; that is a table, and none of it is shown.  What is
/// shown is the list addWeapon builds as the player picks weapons up -- one
/// index and one ammo count per entry, drawn in the order they arrived.

//-----------------------------------------------------------------------------
/// HudWeaponInvBase (Tribes2.exe 0x004ff070): the table, the list, and how an
/// entry is drawn.  Where the next entry goes and how big the list is are the
/// subclass's to say.
class HudWeaponInvBase : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   /// How many icons the table holds.
   enum { MaxItems = 32 };

protected:
   struct Slot
   {
      S32 item;                   ///< Index into the icon table.
      S32 count;                  ///< Negative for "never runs out".
   };

   Vector<Slot> mSlots;

   /// The table is a plain array because a GFXTexHandle in a Vector is
   /// assigned over memory that was never constructed.
   GFXTexHandle mItemBitmaps[MaxItems];
   StringTableEntry mItemBitmapNames[MaxItems];

   /// How many icons script has set, which is what bounds every index.
   S32 mNumItems;

   S32 mActive;

   GFXTexHandle mBackground;
   GFXTexHandle mHighLight;
   GFXTexHandle mInfinite;

   StringTableEntry mBackgroundName;
   StringTableEntry mHighLightName;
   StringTableEntry mInfiniteName;

   Point2I mAmmoOffset;
   Point2I mBackGroundOffset;
   Point2I mHighLightOffset;
   Point2I mIconOffset;
   S32 mBackGroundSpace;

   /// The biggest piece of art in an entry, and the offset it is drawn at.
   Point2I mCellSize;
   Point2I mCellOffset;

   void fitCell(GFXTexHandle &art, const Point2I &offset);
   GFXTexHandle loadArt(const char *name, StringTableEntry &outName);

   S32 findSlot(S32 item) const;

   /// Move `cursor` from one entry to the next.
   virtual void advance(Point2I &cursor) {}

   /// How big the list is, before mCellOffset.
   virtual Point2I itemsExtent() const { return Point2I(0, 0); }

public:
   HudWeaponInvBase();

   DECLARE_CONOBJECT(HudWeaponInvBase);
   static void initPersistFields();

   //-- the icon table ------------------------------------------------------
   void setItemBitmap(S32 item, const char *name);
   const char *getItemBitmap(S32 item) const;
   S32 getNumItems() const { return mNumItems; }

   //-- the list ------------------------------------------------------------
   void addItem(S32 item, S32 count);
   void removeItem(S32 item);
   void setItemCount(S32 item, S32 count);
   void setActiveItem(S32 item);
   S32 getActiveItem() const { return mActive; }

   S32 getNumSlots() const { return mSlots.size(); }
   S32 getSlotId(S32 slot) const;
   S32 getSlotCount(S32 slot) const;

   void clearAll();
   void reset();

   //-- the art -------------------------------------------------------------
   void setBackgroundBitmap(const char *name);
   void setHighLightBitmap(const char *name);
   void setInfiniteBitmap(const char *name);

   const char *getBackgroundBitmap() const { return mBackground ? mBackgroundName : ""; }
   const char *getHighLightBitmap() const  { return mHighLight ? mHighLightName : ""; }
   const char *getInfiniteBitmap() const   { return mInfinite ? mInfiniteName : ""; }

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The weapons column: entries stacked downwards, one background's height
/// and backGroundSpace apart.
class HudWeapons : public HudWeaponInvBase
{
   typedef HudWeaponInvBase Parent;

protected:
   void advance(Point2I &cursor);
   Point2I itemsExtent() const;

public:
   DECLARE_CONOBJECT(HudWeapons);
};

#endif // _HUDWEAPONINVBASE_H_
