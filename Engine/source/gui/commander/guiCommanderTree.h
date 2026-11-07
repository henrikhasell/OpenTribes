// HH: Added for Tribes 2 compatibility.
#ifndef _GUICOMMANDERTREE_H_
#define _GUICOMMANDERTREE_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif
#ifndef _GUICOMMANDERMAP_H_
#include "gui/commander/guiCommanderMap.h"
#endif

/// The list beside the commander map: the team's players and assets, its
/// waypoints and its objectives, each under a header that opens and closes.
/// What is selected here is selected on the map, and the other way about.
class ShapeBaseData;

class GuiCommanderTree : public GuiControl
{
   typedef GuiControl Parent;

public:
   enum Kind { Clients, Targets, Waypoints };

   GuiCommanderTree();

   static void initPersistFields();

   bool onAdd();
   bool onWake();
   void onSleep();
   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect);

   void onMouseDown(const GuiEvent &event);
   void onRightMouseDown(const GuiEvent &event);
   void onMouseMove(const GuiEvent &event);
   void onMouseLeave(const GuiEvent &event);

   void reset();
   bool addCategory(const char *name, const char *text, const char *kind);
   bool openCategory(const char *name, bool open);
   bool registerEntryType(const char *category, U32 type, bool control,
                          const char *icon, const ColorI &color);

   S32 getNumTargets(const char *category);
   S32 getTarget(const char *category, S32 index);
   S32 getNumSelectedTargets(const char *category);
   S32 getSelectedTarget(const char *category, S32 index);

   DECLARE_CONOBJECT(GuiCommanderTree);

private:
   enum FontColor
   {
      CategoryNormal,
      CategoryHilight,
      CategoryEmpty,
      ClientNoneEntry,
      TargetEntry,
   };

   struct EntryType
   {
      StringTableEntry category;
      U32 type;
      bool control;
      GFXTexHandle icon;
      ColorI color;
   };

   struct Entry
   {
      CommanderRef ref;
      const EntryType *type;
      GFXTexHandle icon;
      bool control;
      ShapeBaseData *dataBlock;
      /// Where its kind is listed among the others.
      S32 order;
   };

   struct Category
   {
      StringTableEntry name;
      String text;
      Kind kind;
      bool open;
      /// What is left of its slide open or shut, in milliseconds.
      S32 slide;
      Vector<Entry> entries;
   };

   /// What is under a point of the list.
   struct Hit
   {
      S32 category;
      S32 entry;        ///< -1 for the header, -2 for "None Selected"
      bool onControl;
   };

   /// @name Fields
   /// @{
   StringTableEntry mBackdropBitmapName;
   StringTableEntry mHeaderFontType;
   StringTableEntry mEntryFontType;
   StringTableEntry mClientNoneFontType;
   S32 mHeaderFontSize;
   S32 mEntryFontSize;
   S32 mClientNoneFontSize;
   Point2I mEntryIconOffset;
   Point2I mEntryTextOffset;
   StringTableEntry mHeaderBitmapName;
   Point2I mHeaderTextOffset;
   StringTableEntry mClientNoneText;
   S32 mHeaderHeight;
   S32 mEntryHeight;
   ColorI mEntryHilightColor;
   ColorI mEntrySelectColor;
   S32 mCategoryOpenTime;
   StringTableEntry mDamageColors;
   RectI mObjectControlRect;
   StringTableEntry mObjectControlBitmapName;
   /// @}

   Vector<Category> mCategories;
   Vector<EntryType> mEntryTypes;
   Hit mHover;
   U32 mLastSlideTime;

   GFXTexHandle mBackdrop;
   GFXTexHandle mHeaderBitmap;
   GFXTexHandle mControlBitmap;
   Resource<GFont> mHeaderFont;
   Resource<GFont> mEntryFont;
   Resource<GFont> mClientNoneFont;
   Map<StringTableEntry, GFXTexHandle> mMiniIcons;

   Category *findCategory(const char *name);
   const EntryType *findEntryType(U32 type) const;
   void refresh();
   static bool listedBefore(const Entry &a, const Entry &b);
   void setOpen(Category &cat, bool open);
   S32  rowsHeight(const Category &cat) const;
   S32  contentHeight() const;
   Hit  hitTest(const Point2I &local) const;
   ColorI entryColor(const Entry &entry) const;
   void entryName(const Entry &entry, char *buffer, U32 size) const;
   GFXTexHandle miniIcon(const char *name);
};

#endif
