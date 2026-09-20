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
// diffuseSize is the world size one repeat of the texture covers --
// terrRender.cpp:259 tiles it getWorldBlockSize()/diffuseSize times across the
// block.  64, not 8: TGE did not put one checkerboard on each terrain square.
// Blender::blend splits the source into an 8x8 grid of cells --
// TEXELS_PER_SOURCE_EDGE is 256 and SQUARES_PER_MIPMAP_EDGE_LOG2 is
// 10 - TerrainTextureMipLevel = 3, so eight cells of 32px a side -- and hands
// each terrain square one cell.  A repeat spans 8 squares, which at
// squareSize 8 is 64 metres.
//
// Note when changing any of this: the composited base texture is cached beside
// the mission as orchard_basetex.dds and is NOT invalidated by an edit here.
// Delete it to see the change.
//-----------------------------------------------------------------------------

new TerrainMaterial()
{
   internalName = "apple-picker/data/terrains/checkerboard2";
   diffuseMap = "apple-picker/data/terrains/checkerboard2";
   diffuseSize = "64";
};
