// HH: Added for Tribes 2 compatibility.
#ifndef _T2ARTPATH_H_
#define _T2ARTPATH_H_

#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _VOLUME_H_
#include "core/volume.h"
#endif
#ifndef _GBITMAP_H_
#include "gfx/bitmap/gBitmap.h"
#endif

/// Where a Tribes 2 art name points.
///
/// One fault, found three times.  Tribes 2 names art; Torque 3D resolves
/// paths.  The game sends "borg5.dts", "LushWorld.RockMossy",
/// "lush/skies/l4_f" -- names its ResManager found by searching the registered
/// directories, with no directory of their own and no extension.  This tree
/// has no search: it takes what it is given as a path and opens it.  So the
/// directory has to be put back, and this is the one place that knows which
/// directory each kind of name belongs to.
///
/// Three callers, all of which painted or drew nothing before:
///
///   - shapePathT2, the .dts name on a ShapeBase or a TSStatic;
///   - terrainTexturePathT2, the eight material names a terrain is painted
///     with, which were resolving to warning_material on every square;
///   - texturePathT2, the sky box faces and cloud layers a .dml names, which
///     were resolving to nothing at all.
///
/// The second one is why these do not simply prefix and hand the result to the
/// existing machinery.  `GBitmap::sFindFiles` appends each registered
/// extension with `Path::setExtension`, and `Path::_split` splits a name at
/// its *last* dot -- so "LushWorld.RockMossy" parses as the file "LushWorld"
/// with the extension "RockMossy", and asking for ".png" yields
/// "LushWorld.png".  The second half of the name is destroyed before any
/// lookup happens.  A Tribes 2 art name is not a path and cannot be passed
/// through one.
///
/// The functions below therefore build the whole string -- directory, name and
/// extension -- and only then make a Path of it, by which point the last dot
/// really is the extension and the splitting is correct.
///
/// Case is not a consideration, and that is worth saying because it looks like
/// one: 6,314 of the 10,739 entries in the shipped archives carry an uppercase
/// character, and `KeyCmp::equals<String>` in core/util/tDictionary.h compares
/// a zip entry's name with String::NoCase.  "details/lushdet1" finds
/// "textures/details/LushDet1.png".

/// A .dts name.  Every shape the game names lives under shapes/; a name that
/// already carries a directory is left alone, which is how a mod-supplied
/// path still works.
inline String shapePathT2(const char *name)
{
   String path(name);
   if(path.find('/') != String::NPos || path.find('\\') != String::NPos)
      return path;
   return String("shapes/") + path;
}

/// A .dif name, which a mission gives bare -- `interiorFile = "pmisc3.dif";`
/// -- and which arrives over the wire exactly that way.  A fourth instance of
/// the same fault, and the one that stopped a live join: the engine was asked
/// for "pmisc3.dif", answered nothing, and the server dropped the client with
/// "Unable to load interior".  Asking it for "interiors/pmisc3.dif" loads the
/// same bytes.
inline String interiorPathT2(const char *name)
{
   String path(name);
   if(path.find('/') != String::NPos || path.find('\\') != String::NPos)
      return path;
   return String("interiors/") + path;
}

/// A file named relative to the texture tree, extension and all: a .dml
/// material list, which is the one kind of art name Tribes 2 sends with its
/// extension attached.
inline String texturePathT2(const String &name)
{
   return String("textures/") + name;
}

/// A texture named without its extension, resolved against the formats this
/// build can actually load.  Returns the full path of the first that exists.
///
/// The name is composed whole before a Path is made of it, for the reason the
/// file comment gives.  This is the part that cannot be left to
/// GBitmap::sFindFiles.
inline bool findTexturePathT2(const String &name, String &out)
{
   // Some .dml files name their entries with the extension attached and most
   // do not -- of the 392 entries in the 52 shipped lists, 67 carry one.  A
   // name that is already a file is taken as it stands rather than having a
   // second extension put on the end of it.
   if(Torque::FS::IsFile(Torque::Path(texturePathT2(name))))
   {
      out = texturePathT2(name);
      return true;
   }

   for(U32 i = 0; i < GBitmap::sRegistrations.size(); i++)
   {
      const GBitmap::Registration &reg = GBitmap::sRegistrations[i];
      for(U32 j = 0; j < reg.extensions.size(); j++)
      {
         const String path = texturePathT2(name) + "." + reg.extensions[j];
         if(Torque::FS::IsFile(Torque::Path(path)))
         {
            out = path;
            return true;
         }
      }
   }

   return false;
}

/// A texture a shape names bare that lives in the skin tree.
///
/// A .dts carries its texture names without directories, and for the game's
/// own weapons and players those sit under textures/skins/ --
/// "chaingun_shot_end" is textures/skins/chaingun_shot_end.png.  Tried after
/// the plain texture tree, because a name that resolves directly under
/// textures/ should keep doing so.
inline bool findSkinTexturePathT2(const String &name, String &out)
{
   return findTexturePathT2(String("skins/") + name, out);
}

/// A terrain material name -- "LushWorld.RockMossy" -- which names a texture
/// under the terrain subtree.  The dot is part of the name, not an extension.
///
/// Two sources name the same material two ways, and both arrive for the same
/// terrain.  Ramparts.ter holds "terrain.LushWorld.RockMossy",
/// "terrain.LushWorld.DirtMossy", "LushWorld.GrassLight" and
/// "IceWorld.SnowRock"; the server sends the same four with no prefix on any
/// of them.  Taken literally the prefixed pair resolves to nothing and paints
/// warning_material, which is two of a four-material terrain drawing black.
///
/// The prefix is the directory written into the name.  Beside the texture
/// there is a textures/terrain.<name>.dml listing the five tiles Tribes 2
/// varied that material with -- terrainTiles/mossRock1 through 5 -- which a
/// TerrainMaterial has nowhere to put, taking one diffuse map.  The server's
/// form of the name is the one followed here, because the server is the
/// authority on what a terrain is painted with and because one form for both
/// sources means one material rather than two for the same ground.
///
/// Both forms have to end up as the same TerrainMaterial or a terrain gets two
/// for the same ground: the file's and the wire's, resolving to the same
/// texture, registered under different internal names so neither can find the
/// other.  A live join is what showed it -- eight materials in the set for a
/// four-material mission, and only the four the server named carrying the
/// detail texture.
inline String terrainMaterialNameT2(const char *name)
{
   if(dStrnicmp(name, "terrain.", 8) == 0)
      return String(name + 8);

   return String(name);
}

inline bool findTerrainTexturePathT2(const String &name, String &out)
{
   return findTexturePathT2(String("terrain/") + terrainMaterialNameT2(name.c_str()), out);
}

#endif
