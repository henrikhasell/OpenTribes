// HH: Added for Tribes 2 compatibility.
#ifndef _MATERIALPROPERTYMAP_H_
#define _MATERIALPROPERTYMAP_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

/// What Tribes 2's scripts attach to a texture name through
/// `addMaterialMapping`.
class MaterialPropertyMap
{
public:
   struct Entry
   {
      bool              hasColor;
      ColorF            color;
      F32               emissive;
      bool              hasSound;
      S32               sound;
      bool              hasEnvironment;
      StringTableEntry  envTexture;
      F32               envFactor;

      Entry()
         : hasColor(false), color(1.0f, 1.0f, 1.0f, 1.0f), emissive(0.0f),
           hasSound(false), sound(-1),
           hasEnvironment(false), envTexture(NULL), envFactor(0.0f) {}
   };

   static MaterialPropertyMap *get();

   /// `props` are property strings of the form "<kind>: <values>".
   void add(const char *textureName, const char *const *props, U32 count);

   /// Case insensitive: the scripts write lower case, the interiors upper.
   const Entry *find(const char *textureName) const;

   U32 size() const { return mEntries.size(); }

private:
   Map<String, Entry> mEntries;

   static String key(const char *textureName);
};

#endif
