//-----------------------------------------------------------------------------
// Content that has to be registered before a mission loads.
//
// TerrainMaterial::findOrCreate() is called from TerrainFile::load, i.e. while
// TerrainBlock::onAdd is reading the .ter, so the terrain's material has to be
// in the TerrainMaterialSet before any TerrainBlock is added -- not registered
// from onServerCreated with the datablocks.
//
// onStart() execs this once, which in single player covers both halves: the
// client's ghosted TerrainBlock reads the same .ter and needs the same lookup
// to resolve.
//-----------------------------------------------------------------------------

exec("./terrains/materials.cs");
exec("./skies/materials.cs");
