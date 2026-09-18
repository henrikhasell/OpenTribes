//-----------------------------------------------------------------------------
// Surface properties for orchard.ter's one material.
//
// The name matched is the *basename* of the material path stored inside the
// .ter (TerrainBlock::onAdd, terrain/terrData.cc:836), so this is
// apple-picker/data/terrains/checkerboard2 -- the same tutorial checkerboard the
// mission has always used.
//
// This entry is currently INERT, and deliberately so.  It is what
// Player::updateActionThread (game/player.cc:2098) looks up before it draws a
// foot puff or picks a footstep sound, and it skips the whole block when the
// lookup fails -- but PlayerData(PlayerShape) in server/player.cs names no
// footPuffEmitter and no footstepSound datablocks yet, so there is nothing for
// the entry to colour.  It is here so that adding either later is one line
// rather than an afternoon working out why the dust never appears.
//
// "color: r g b a0 a1" is one RGB with the alpha at each end of the particle's
// life (dgl/materialPropertyMap.cc:140).  "sound:" is an index into
// playFootstepSound's switch, not a datablock name: 0 soft, 1 hard, 2 metal,
// 3 snow.
//-----------------------------------------------------------------------------

addMaterialMapping( "checkerboard2" , "sound: 1" , "color: 0.46 0.36 0.26 0.4 0.0" );
