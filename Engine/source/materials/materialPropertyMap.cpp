// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "materials/materialPropertyMap.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/strings/stringFunctions.h"
#include "core/strings/stringUnit.h"
#include "console/engineAPI.h"

MaterialPropertyMap *MaterialPropertyMap::get()
{
   static MaterialPropertyMap sMap;
   return &sMap;
}

String MaterialPropertyMap::key(const char *textureName)
{
   String k(textureName);
   return String::ToLower(k);
}

void MaterialPropertyMap::add(const char *textureName,
                              const char *const *props, U32 count)
{
   if(!textureName || !textureName[0])
      return;

   Entry e;
   Map<String, Entry>::Iterator existing = mEntries.find(key(textureName));
   if(existing != mEntries.end())
      e = existing->value;

   for(U32 i = 0; i < count; i++)
   {
      const char *prop = props[i];
      if(!prop || !prop[0])
         continue;

      const char *colon = dStrchr(prop, ':');
      if(!colon)
      {
         Con::warnf("addMaterialMapping: %s has no kind in %s",
                    textureName, prop);
         continue;
      }

      char kind[32];
      const U32 len = getMin(U32(colon - prop), U32(sizeof(kind) - 1));
      dStrncpy(kind, prop, len);
      kind[len] = 0;

      const char *values = colon + 1;
      while(*values == ' ')
         values++;

      if(!dStricmp(kind, "color"))
      {
         F32 c[NumColors * 4] = { 0.0f };
         dSscanf(values, "%g %g %g %g %g %g %g %g",
                 &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7]);
         e.hasColor = true;
         for(U32 j = 0; j < NumColors; j++)
            e.color[j].set(c[j * 4], c[j * 4 + 1], c[j * 4 + 2], c[j * 4 + 3]);
      }
      else if(!dStricmp(kind, "sound"))
      {
         e.hasSound = true;
         e.sound = dAtoi(values);
      }
      else if(!dStricmp(kind, "environment"))
      {
         // A texture to reflect, then how strongly.
         char tex[256];
         F32 factor = 0.0f;
         if(dSscanf(values, "%255s %g", tex, &factor) >= 1)
         {
            e.hasEnvironment = true;
            e.envTexture = StringTable->insert(tex);
            e.envFactor = factor;
         }
      }
      else
      {
         static StringTableEntry sWarned = NULL;
         StringTableEntry k = StringTable->insert(kind);
         if(k != sWarned)
         {
            sWarned = k;
            Con::warnf("addMaterialMapping: no use for a '%s' property yet",
                       kind);
         }
      }
   }

   mEntries[key(textureName)] = e;
}

const MaterialPropertyMap::Entry *
MaterialPropertyMap::find(const char *textureName) const
{
   if(!textureName || !textureName[0])
      return NULL;
   Map<String, Entry>::ConstIterator i = mEntries.find(key(textureName));
   return i == mEntries.end() ? NULL : &i->value;
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2's own addMaterialMapping, which `scripts/*PropMap.cs` calls with a
// texture name and one or more property strings.  Torque 3D's function of this
// name took a Material object's name instead, so the property string was
// stored as one: a surface with a mapping then resolved to no material at all
// and never reached the fallback that loads the texture itself.
DefineEngineFunction( addMaterialMapping, void,
   (const char *textureName, const char *a, const char *b, const char *c),
   ("", "", ""),
   "@brief Attach Tribes 2 material properties to a texture name.\n\n"
   "Each property is \"<kind>: <values>\" -- color, sound or environment.\n\n"
   "@ingroup Materials" )
{
   const char *props[3] = { a, b, c };
   U32 count = 0;
   for(U32 i = 0; i < 3; i++)
      if(props[i] && props[i][0])
         count = i + 1;

   MaterialPropertyMap::get()->add(textureName, props, count);
}

DefineEngineFunction( getMaterialPropertyCount, S32, (),,
   "@brief How many texture names carry Tribes 2 material properties.\n\n"
   "@ingroup Materials" )
{
   return S32(MaterialPropertyMap::get()->size());
}

DefineEngineFunction( getMaterialSound, S32, (const char *textureName),,
   "@brief The surface sound Tribes 2 maps to a texture, or -1.\n\n"
   "@ingroup Materials" )
{
   const MaterialPropertyMap::Entry *e =
      MaterialPropertyMap::get()->find(textureName);
   return (e && e->hasSound) ? e->sound : -1;
}
