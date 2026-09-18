//-----------------------------------------------------------------------------
// Content that has to be registered before a mission loads.
//
// The material property map is engine-wide and read by TerrainBlock::onAdd, so
// it has to be filled in before any TerrainBlock is added -- not from
// onServerCreated with the datablocks.  onStart() execs this on both halves,
// which is also what the client's ghosted TerrainBlock needs.
//-----------------------------------------------------------------------------

exec("./terrains/propertyMap.cs");
