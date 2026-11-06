// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/guiCommanderTree.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/strings/stringUnit.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "T3D/gameBase/clientTargetT2.h"
#include "T3D/gameBase/targetManagerT2.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/shapeBase.h"

IMPLEMENT_CONOBJECT(GuiCommanderTree);

ConsoleDocClass( GuiCommanderTree,
   "@brief The list of the team's players, assets, waypoints and objectives "
   "beside the commander map.\n\n@internal" );

GuiCommanderTree::GuiCommanderTree()
{
   mBackdropBitmapName = StringTable->insert("commander/gui/cmd_gradient");
   mHeaderFontType = StringTable->insert("Arial Bold");
   mEntryFontType = StringTable->insert("Arial");
   mClientNoneFontType = StringTable->insert("Arial");
   mHeaderFontSize = 14;
   mEntryFontSize = 13;
   mClientNoneFontSize = 13;
   mEntryIconOffset.set(2, 1);
   mEntryTextOffset.set(20, 3);
   mHeaderBitmapName = StringTable->insert("commander/gui/cmd_columnheadbar");
   mHeaderTextOffset.set(10, 4);
   mClientNoneText = StringTable->insert("<None>");
   mHeaderHeight = 20;
   mEntryHeight = 20;
   mEntryHilightColor.set(44, 168, 219, 100);
   mEntrySelectColor.set(255, 0, 0, 100);
   mCategoryOpenTime = 250;
   mDamageColors = StringTable->insert("0 255 0 20 255 255 0 70 255 0 0 100");
   mObjectControlRect.set(128, 2, 16, 16);
   mObjectControlBitmapName = StringTable->insert("commander/gui/cmd_control_checkbox");

   mHover.category = -1;
   mHover.entry = -1;
   mHover.onControl = false;
}

void GuiCommanderTree::initPersistFields()
{
   addField("backdropBitmapName", TypeString, Offset(mBackdropBitmapName, GuiCommanderTree), "");
   addField("headerFontType", TypeString, Offset(mHeaderFontType, GuiCommanderTree), "");
   addField("entryFontType", TypeString, Offset(mEntryFontType, GuiCommanderTree), "");
   addField("clientNoneFontType", TypeString, Offset(mClientNoneFontType, GuiCommanderTree), "");
   addField("headerFontSize", TypeS32, Offset(mHeaderFontSize, GuiCommanderTree), "");
   addField("entryFontSize", TypeS32, Offset(mEntryFontSize, GuiCommanderTree), "");
   addField("clientNoneFontSize", TypeS32, Offset(mClientNoneFontSize, GuiCommanderTree), "");
   addField("entryIconOffset", TypePoint2I, Offset(mEntryIconOffset, GuiCommanderTree), "");
   addField("entryTextOffset", TypePoint2I, Offset(mEntryTextOffset, GuiCommanderTree), "");
   addField("headerBitmapName", TypeString, Offset(mHeaderBitmapName, GuiCommanderTree), "");
   addField("headerTextOffset", TypePoint2I, Offset(mHeaderTextOffset, GuiCommanderTree), "");
   addField("clientNoneText", TypeString, Offset(mClientNoneText, GuiCommanderTree), "");
   addField("headerHeight", TypeS32, Offset(mHeaderHeight, GuiCommanderTree), "");
   addField("entryHeight", TypeS32, Offset(mEntryHeight, GuiCommanderTree), "");
   addField("entryHilightColor", TypeColorI, Offset(mEntryHilightColor, GuiCommanderTree), "");
   addField("entrySelectColor", TypeColorI, Offset(mEntrySelectColor, GuiCommanderTree), "");
   addField("categoryOpenTime", TypeS32, Offset(mCategoryOpenTime, GuiCommanderTree), "");
   addField("damageColors", TypeString, Offset(mDamageColors, GuiCommanderTree),
            "Colour and damage percentage up to which it is used, in fours.");
   addField("objectControlRect", TypeRectI, Offset(mObjectControlRect, GuiCommanderTree), "");
   addField("objectControlBitmapName", TypeString, Offset(mObjectControlBitmapName, GuiCommanderTree), "");

   Parent::initPersistFields();
}

bool GuiCommanderTree::onAdd()
{
   // The scripts' onAdd registers waypoints by $CMD_WAYPOINTTYPEID.
   ClientTarget::initTypes();
   return Parent::onAdd();
}

bool GuiCommanderTree::onWake()
{
   if (!Parent::onWake())
      return false;

   const char *cache = Con::getVariable("$GUI::fontCacheDirectory");
   mHeaderFont = GFont::create(mHeaderFontType, mHeaderFontSize, cache);
   mEntryFont = GFont::create(mEntryFontType, mEntryFontSize, cache);
   mClientNoneFont = GFont::create(mClientNoneFontType, mClientNoneFontSize, cache);

   if (mBackdropBitmapName[0])
      mBackdrop.set(mBackdropBitmapName, &GFXDefaultGUIProfile, avar("%s() - backdrop", __FUNCTION__));
   if (mHeaderBitmapName[0])
      mHeaderBitmap.set(mHeaderBitmapName, &GFXDefaultGUIProfile, avar("%s() - header", __FUNCTION__));
   if (mObjectControlBitmapName[0])
      mControlBitmap.set(mObjectControlBitmapName, &GFXDefaultGUIProfile, avar("%s() - control", __FUNCTION__));

   return true;
}

void GuiCommanderTree::onSleep()
{
   mBackdrop = NULL;
   mHeaderBitmap = NULL;
   mControlBitmap = NULL;
   mMiniIcons.clear();
   Parent::onSleep();
}

//----------------------------------------------------------------------------

void GuiCommanderTree::reset()
{
   mCategories.clear();
   mEntryTypes.clear();
   mHover.category = -1;
   mHover.entry = -1;
}

GuiCommanderTree::Category *GuiCommanderTree::findCategory(const char *name)
{
   for (S32 i = 0; i < mCategories.size(); i++)
      if (!dStricmp(mCategories[i].name, name))
         return &mCategories[i];
   return NULL;
}

bool GuiCommanderTree::addCategory(const char *name, const char *text, const char *kind)
{
   if (findCategory(name))
      return false;

   Category cat;
   cat.name = StringTable->insert(name);
   cat.text = text;
   cat.open = true;
   if (!dStricmp(kind, "clients"))
      cat.kind = Clients;
   else if (!dStricmp(kind, "waypoints"))
      cat.kind = Waypoints;
   else
      cat.kind = Targets;
   mCategories.push_back(cat);
   return true;
}

bool GuiCommanderTree::openCategory(const char *name, bool open)
{
   Category *cat = findCategory(name);
   if (!cat)
      return false;
   cat->open = open;
   return true;
}

bool GuiCommanderTree::registerEntryType(const char *category, U32 type, bool control,
                                         const char *icon, const ColorI &color)
{
   if (!findCategory(category))
      return false;

   EntryType entry;
   entry.category = StringTable->insert(category);
   entry.type = type;
   entry.control = control;
   entry.color = color;
   if (icon && icon[0])
      entry.icon.set(icon, &GFXDefaultGUIProfile, avar("%s() - icon", __FUNCTION__));
   mEntryTypes.push_back(entry);
   return true;
}

const GuiCommanderTree::EntryType *GuiCommanderTree::findEntryType(U32 type) const
{
   if (!type)
      return NULL;
   for (S32 i = 0; i < mEntryTypes.size(); i++)
      if (mEntryTypes[i].type == type)
         return &mEntryTypes[i];
   return NULL;
}

GFXTexHandle GuiCommanderTree::miniIcon(const char *name)
{
   if (!name || !name[0])
      return GFXTexHandle();

   StringTableEntry key = StringTable->insert(name);
   Map<StringTableEntry, GFXTexHandle>::Iterator it = mMiniIcons.find(key);
   if (it != mMiniIcons.end())
      return it->value;

   GFXTexHandle tex(name, &GFXDefaultGUIProfile, avar("%s() - mini icon", __FUNCTION__));
   mMiniIcons.insert(key, tex);
   return tex;
}

bool GuiCommanderTree::listedBefore(const Entry &a, const Entry &b)
{
   if (a.order != b.order)
      return a.order < b.order;

   char na[256], nb[256];
   if (!T2Targets::getTargetName(a.ref.target, na, sizeof(na)))
      na[0] = 0;
   if (!T2Targets::getTargetName(b.ref.target, nb, sizeof(nb)))
      nb[0] = 0;
   return dStricmp(na, nb) < 0;
}

/// Every target of this client's team, filed by its registered type or by
/// its datablock's cmdCategory, in the order of their ids; then this
/// client's own waypoints and tasks.
void GuiCommanderTree::refresh()
{
   for (S32 i = 0; i < mCategories.size(); i++)
      mCategories[i].entries.clear();

   const U32 mine = T2Targets::clientSensorGroup();
   for (S32 target = TargetManagerT2::TeamTargets; target < TargetManagerT2::MaxTargets; target++)
   {
      if (!T2Targets::isAllocated(target) || T2Targets::sensorGroup(target) != mine)
         continue;

      ShapeBaseData *data = dynamic_cast<ShapeBaseData *>(T2Targets::dataBlock(target));
      if (!data)
         if (ShapeBase *shape = GuiCommanderMap::ghostOf(target))
            data = dynamic_cast<ShapeBaseData *>(shape->getDataBlock());

      const EntryType *type = findEntryType(T2Targets::typeTag(target));
      Category *cat = NULL;
      if (type)
         cat = findCategory(type->category);
      else if (data && data->cmdCategory && data->cmdCategory[0])
         cat = findCategory(data->cmdCategory);
      if (!cat)
         continue;

      Entry entry;
      entry.ref = CommanderRef(target);
      entry.type = type;
      entry.icon = type && type->icon ? type->icon
                                      : miniIcon(data ? data->cmdMiniIconName : NULL);
      entry.control = type ? type->control : (data && data->canControl);
      entry.dataBlock = data;
      entry.order = 0;
      cat->entries.push_back(entry);
   }

   // Retail lists each kind of asset together, the kinds in the order they
   // first turn up and by name within each; players stay as they come.
   for (S32 i = 0; i < mCategories.size(); i++)
   {
      if (mCategories[i].kind != Targets)
         continue;

      Vector<Entry> &entries = mCategories[i].entries;
      for (S32 a = 0; a < entries.size(); a++)
      {
         entries[a].order = a;
         for (S32 b = 0; b < a; b++)
            if (entries[b].dataBlock == entries[a].dataBlock)
            {
               entries[a].order = entries[b].order;
               break;
            }
      }

      for (S32 a = 1; a < entries.size(); a++)
         for (S32 b = a; b > 0 && listedBefore(entries[b], entries[b - 1]); b--)
         {
            const Entry swapped = entries[b];
            entries[b] = entries[b - 1];
            entries[b - 1] = swapped;
         }
   }

   const Vector<ClientTarget *> &own = ClientTarget::typed();
   for (S32 i = 0; i < own.size(); i++)
   {
      const EntryType *type = findEntryType(own[i]->getType());
      Category *cat = type ? findCategory(type->category) : NULL;
      if (!cat)
         continue;

      Entry entry;
      entry.ref = CommanderRef(-1, own[i]->getId());
      entry.type = type;
      entry.icon = type->icon;
      entry.control = type->control;
      entry.dataBlock = NULL;
      entry.order = 0;
      cat->entries.push_back(entry);
   }
}

S32 GuiCommanderTree::contentHeight() const
{
   S32 height = 0;
   for (S32 i = 0; i < mCategories.size(); i++)
   {
      const Category &cat = mCategories[i];
      height += mHeaderHeight;
      if (cat.open)
         height += mEntryHeight * (cat.entries.size() + (cat.kind == Clients ? 1 : 0));
   }
   return height;
}

void GuiCommanderTree::onPreRender()
{
   refresh();

   // Tall enough for everything, and never shorter than the scroll it is in,
   // so the backdrop reaches the bottom.
   S32 height = contentHeight();
   if (GuiControl *parent = getParent())
      height = getMax(height, parent->getHeight());
   if (height != getHeight())
      setExtent(getWidth(), height);

   Parent::onPreRender();
}

GuiCommanderTree::Hit GuiCommanderTree::hitTest(const Point2I &local) const
{
   Hit hit;
   hit.category = -1;
   hit.entry = -1;
   hit.onControl = false;

   S32 y = 0;
   for (S32 i = 0; i < mCategories.size(); i++)
   {
      const Category &cat = mCategories[i];
      if (local.y >= y && local.y < y + mHeaderHeight)
      {
         hit.category = i;
         return hit;
      }
      y += mHeaderHeight;
      if (!cat.open)
         continue;

      if (cat.kind == Clients)
      {
         if (local.y >= y && local.y < y + mEntryHeight)
         {
            hit.category = i;
            hit.entry = -2;
            return hit;
         }
         y += mEntryHeight;
      }

      for (S32 e = 0; e < cat.entries.size(); e++, y += mEntryHeight)
         if (local.y >= y && local.y < y + mEntryHeight)
         {
            hit.category = i;
            hit.entry = e;
            hit.onControl = cat.entries[e].control &&
               RectI(mObjectControlRect.point + Point2I(0, y), mObjectControlRect.extent).pointInRect(local);
            return hit;
         }
   }

   return hit;
}

/// A listed shape's icon shows its damage: each colour of damageColors up
/// to its percentage. Anything not in scope keeps its registered colour.
ColorI GuiCommanderTree::entryColor(const Entry &entry) const
{
   ShapeBase *shape = entry.ref.target >= 0 ? GuiCommanderMap::ghostOf(entry.ref.target) : NULL;
   if (!shape)
      return entry.type ? entry.type->color : ColorI(255, 255, 255, 255);

   const F32 damage = shape->getDamageValue() * 100.0f;
   const U32 count = StringUnit::getUnitCount(mDamageColors, " \t") / 4;
   ColorI color(255, 255, 255, 255);
   for (U32 i = 0; i < count; i++)
   {
      color.set(dAtoi(StringUnit::getUnit(mDamageColors, i * 4 + 0, " \t")),
                dAtoi(StringUnit::getUnit(mDamageColors, i * 4 + 1, " \t")),
                dAtoi(StringUnit::getUnit(mDamageColors, i * 4 + 2, " \t")), 255);
      if (damage <= dAtof(StringUnit::getUnit(mDamageColors, i * 4 + 3, " \t")))
         break;
   }
   return color;
}

void GuiCommanderTree::entryName(const Entry &entry, char *buffer, U32 size) const
{
   buffer[0] = 0;
   if (ClientTarget *local = entry.ref.getLocal())
      dStrncpy(buffer, local->getText(), size);
   else
      T2Targets::getTargetName(entry.ref.target, buffer, size);
   buffer[size - 1] = 0;
}

void GuiCommanderTree::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   GuiCommanderMap *map = GuiCommanderMap::get();
   const S32 width = getWidth();

   draw->clearBitmapModulation();
   if (mBackdrop)
      draw->drawBitmapStretch(mBackdrop, RectI(offset, getExtent()));
   else
      draw->drawRectFill(RectI(offset, getExtent()), ColorI(0, 0, 0, 255));

   S32 y = offset.y;
   for (S32 i = 0; i < mCategories.size(); i++)
   {
      const Category &cat = mCategories[i];

      draw->clearBitmapModulation();
      if (mHeaderBitmap)
         draw->drawBitmapStretch(mHeaderBitmap, RectI(offset.x, y, width, mHeaderHeight));

      if (mHeaderFont)
      {
         const bool hover = mHover.category == i && mHover.entry == -1;
         const ColorI &color = mProfile->mFontColors[cat.entries.empty() ? CategoryEmpty
                                                      : hover ? CategoryHilight : CategoryNormal];
         draw->setBitmapModulation(color);
         draw->drawText(mHeaderFont, Point2I(offset.x + mHeaderTextOffset.x, y + mHeaderTextOffset.y),
                        cat.text.c_str());
      }
      y += mHeaderHeight;

      if (!cat.open)
         continue;

      if (cat.kind == Clients)
      {
         if (mHover.category == i && mHover.entry == -2)
            draw->drawRectFill(RectI(offset.x, y, width, mEntryHeight - 1), mEntryHilightColor);
         if (mClientNoneFont)
         {
            const S32 w = mClientNoneFont->getStrWidth(mClientNoneText);
            draw->setBitmapModulation(mProfile->mFontColors[ClientNoneEntry]);
            draw->drawText(mClientNoneFont, Point2I(offset.x + (width - w) / 2, y + mEntryTextOffset.y),
                           mClientNoneText);
         }
         y += mEntryHeight;
      }

      for (S32 e = 0; e < cat.entries.size(); e++, y += mEntryHeight)
      {
         const Entry &entry = cat.entries[e];
         const RectI row(offset.x, y, width, mEntryHeight - 1);

         if (map && map->isSelected(entry.ref))
            draw->drawRectFill(row, mEntrySelectColor);
         else if (mHover.category == i && mHover.entry == e)
            draw->drawRectFill(row, mEntryHilightColor);

         if (entry.icon)
         {
            draw->setBitmapModulation(entryColor(entry));
            draw->drawBitmap(entry.icon, Point2I(offset.x + mEntryIconOffset.x, y + mEntryIconOffset.y));
         }

         if (mEntryFont)
         {
            char name[256];
            entryName(entry, name, sizeof(name));
            draw->setBitmapModulation(mProfile->mFontColors[TargetEntry]);
            draw->drawText(mEntryFont, Point2I(offset.x + mEntryTextOffset.x, y + mEntryTextOffset.y), name);
         }

         if (entry.control && mControlBitmap)
         {
            // The art is the box unticked then ticked; nothing here is ever
            // left controlled from the list, so it is always unticked.
            const S32 cell = mControlBitmap->getBitmapWidth() / 2;
            draw->clearBitmapModulation();
            draw->drawBitmapStretchSR(mControlBitmap,
               RectI(offset.x + mObjectControlRect.point.x, y + mObjectControlRect.point.y,
                     mObjectControlRect.extent.x, mObjectControlRect.extent.y),
               RectI(0, 0, cell, mControlBitmap->getBitmapHeight()));
         }
      }
   }

   draw->clearBitmapModulation();
   renderChildControls(offset, updateRect);
}

//----------------------------------------------------------------------------

void GuiCommanderTree::onMouseMove(const GuiEvent &event)
{
   mHover = hitTest(globalToLocalCoord(event.mousePoint));
}

void GuiCommanderTree::onMouseLeave(const GuiEvent &event)
{
   mHover.category = -1;
   mHover.entry = -1;
   Parent::onMouseLeave(event);
}

void GuiCommanderTree::onMouseDown(const GuiEvent &event)
{
   const Hit hit = hitTest(globalToLocalCoord(event.mousePoint));
   if (hit.category < 0)
      return;

   Category &cat = mCategories[hit.category];
   GuiCommanderMap *map = GuiCommanderMap::get();

   if (hit.entry == -1)
   {
      cat.open = !cat.open;
      Con::executef(this, "onCategoryOpen", cat.name, Con::getIntArg(cat.open));
      return;
   }

   if (!map)
      return;

   // "None Selected" orders the whole team.
   if (hit.entry == -2)
   {
      for (S32 e = 0; e < cat.entries.size(); e++)
         map->select(cat.entries[e].ref, false);
      return;
   }

   const Entry &entry = cat.entries[hit.entry];
   if (hit.onControl)
   {
      Con::executef(this, "controlObject", Con::getIntArg(entry.ref.target));
      return;
   }

   if (event.modifier & (SI_SHIFT | SI_CTRL))
      map->select(entry.ref, !map->isSelected(entry.ref));
   else
   {
      const CommanderRef ref = entry.ref;
      for (S32 i = map->getSelection().size() - 1; i >= 0; i--)
         if (map->getSelection()[i] != ref)
            map->select(map->getSelection()[i], false);
      map->select(ref, true);

      if (event.mouseClickCount > 1)
         map->followLastSelected();
   }
}

void GuiCommanderTree::onRightMouseDown(const GuiEvent &event)
{
   const Hit hit = hitTest(globalToLocalCoord(event.mousePoint));
   if (hit.category < 0 || hit.entry < 0)
      return;

   if (GuiCommanderMap *map = GuiCommanderMap::get())
      map->issueCommand(mCategories[hit.category].entries[hit.entry].ref, event.mousePoint);
}

//----------------------------------------------------------------------------

S32 GuiCommanderTree::getNumTargets(const char *category)
{
   refresh();
   Category *cat = findCategory(category);
   return cat ? cat->entries.size() : 0;
}

static S32 entryTarget(const CommanderRef &ref)
{
   if (ClientTarget *local = ref.getLocal())
      return local->getTargetId();
   return ref.target;
}

S32 GuiCommanderTree::getTarget(const char *category, S32 index)
{
   refresh();
   Category *cat = findCategory(category);
   if (!cat || index < 0 || index >= cat->entries.size())
      return -1;
   return entryTarget(cat->entries[index].ref);
}

S32 GuiCommanderTree::getNumSelectedTargets(const char *category)
{
   refresh();
   Category *cat = findCategory(category);
   GuiCommanderMap *map = GuiCommanderMap::get();
   if (!cat || !map)
      return 0;

   S32 count = 0;
   for (S32 i = 0; i < cat->entries.size(); i++)
      count += map->isSelected(cat->entries[i].ref);
   return count;
}

S32 GuiCommanderTree::getSelectedTarget(const char *category, S32 index)
{
   refresh();
   Category *cat = findCategory(category);
   GuiCommanderMap *map = GuiCommanderMap::get();
   if (!cat || !map)
      return -1;

   for (S32 i = 0; i < cat->entries.size(); i++)
      if (map->isSelected(cat->entries[i].ref) && index-- == 0)
         return entryTarget(cat->entries[i].ref);
   return -1;
}

//----------------------------------------------------------------------------

DefineEngineMethod( GuiCommanderTree, addCategory, bool, (const char *name, const char *text, const char *kind), ,
   "commanderTree.addCategory(name, displayText, type)" )
{
   return object->addCategory(name, text, kind);
}

DefineEngineMethod( GuiCommanderTree, openCategory, bool, (const char *name, bool open), ,
   "commanderTree.openCategory(name, bool)" )
{
   return object->openCategory(name, open);
}

DefineEngineMethod( GuiCommanderTree, registerEntryType, bool,
   (const char *category, S32 type, bool control, const char *icon, ColorI color), ,
   "commanderTree.registerEntryType(category, id, control, icon, color)" )
{
   return object->registerEntryType(category, U32(type), control, icon, color);
}

DefineEngineMethod( GuiCommanderTree, reset, void, (), ,
   "commanderTree.reset()" )
{
   object->reset();
}

DefineEngineMethod( GuiCommanderTree, getNumTargets, S32, (const char *category), ,
   "commanderTree.getNumTargets(category)" )
{
   return object->getNumTargets(category);
}

DefineEngineMethod( GuiCommanderTree, getTarget, S32, (const char *category, S32 index), ,
   "commanderTree.getTarget(category, idx)" )
{
   return object->getTarget(category, index);
}

DefineEngineMethod( GuiCommanderTree, getNumSelectedTargets, S32, (const char *category), ,
   "commanderTree.getNumSelectedTargets(category)" )
{
   return object->getNumSelectedTargets(category);
}

DefineEngineMethod( GuiCommanderTree, getSelectedTarget, S32, (const char *category, S32 index), ,
   "commanderTree.getSelectedTarget(category, idx)" )
{
   return object->getSelectedTarget(category, index);
}
