//-----------------------------------------------------------------------------
// Surface properties for orchard.ter's one material.
//
// This replaces TGE's data/terrains/propertyMap.cs.  addMaterialMapping() still
// exists but means something else entirely now -- it maps a texture name to a
// Material (materials/materialManager.cpp:460), not a texture name to a
// footstep sound and a dust colour -- so the old call was not portable, and the
// surface properties it carried have no engine-wide table to live in any more.
// Footstep sounds and foot puffs are fields on PlayerData in this engine.
//
// What is portable is the material itself.  Without this file the terrain still
// renders: TerrainMaterial::findOrCreate() sees that the name stored in the
// .ter is a path to a readable texture and builds a material out of it.  But
// that fallback gets diffuseSize 500, which stretches one checkerboard over 500
// metres and leaves the ground a flat pale blue.
//
// internalName has to be the string stored inside orchard.ter verbatim -- that
// is what findOrCreate() looks the material up by, and it finds this one first
// and skips the fallback.
//
// diffuseSize is the world size one repeat of the texture covers, so 8 is one
// checkerboard per terrain square, which is how TGE applied it.
//
// Note when changing any of this: the composited base texture is cached beside
// the mission as orchard_basetex.dds and is NOT invalidated by an edit here.
// Delete it to see the change.
//-----------------------------------------------------------------------------

new TerrainMaterial()
{
   internalName = "apple-picker/data/terrains/checkerboard2";
   diffuseMap = "apple-picker/data/terrains/checkerboard2";
   diffuseSize = "8";
};
