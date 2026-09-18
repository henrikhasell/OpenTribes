//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Load dts shapes and merge animations.
//
// `singleton`, not `datablock`: TSShapeConstructor is a plain SimObject here,
// not a SimDataBlock, so `datablock` fails with "Unable to instantiate
// non-datablock class".  singleton also makes the re-exec from onServerCreated()
// (see server/game.cs) a no-op rather than a duplicate object -- which matters
// because, unlike a datablock, this one is NOT cleared by deleteDataBlocks().
//
// The old sequenceN field syntax still works: TSShapeConstructor::_onAdd falls
// back to scanning for dynamic fields matching "sequence%d" when the shape has
// no `sequence` fields (ts/tsShapeConstruct.cpp:332-338).
singleton TSShapeConstructor(PlayerDts)
{
   baseShape = "~/data/shapes/player/player.dts";
   sequence0 = "~/data/shapes/player/player_root.dsq root";
   sequence1 = "~/data/shapes/player/player_forward.dsq run";
   sequence2 = "~/data/shapes/player/player_back.dsq back";
   sequence3 = "~/data/shapes/player/player_side.dsq side";
   sequence4 = "~/data/shapes/player/player_fall.dsq fall";
   sequence5 = "~/data/shapes/player/player_land.dsq land";
   sequence6 = "~/data/shapes/player/player_jump.dsq jump";
   sequence7 = "~/data/shapes/player/player_standjump.dsq standjump";
   sequence8 = "~/data/shapes/player/player_lookde.dsq look";
   sequence9 = "~/data/shapes/player/player_head.dsq head";
   sequence10 = "~/data/shapes/player/player_headside.dsq headside";
   sequence11 = "~/data/shapes/player/player_celwave.dsq celwave";
};

datablock PlayerData(PlayerShape)
{
   renderFirstPerson = false;
   shapeFile = "~/data/shapes/player/player.dts";

   // How far the third-person camera pulls back.  Stock tutorial.base leaves
   // this at its default of 0, which puts the camera inside the player's head --
   // pressing tab there gives you a face full of your own shoulders.  A game you
   // play by walking over things wants the third-person view to be usable, so
   // take starter.fps's 3 (server/scripts/player.cs:527).
   cameraMaxDist = 3;
};


//----------------------------------------------------------------------------
// PlayerShape Datablock methods
//----------------------------------------------------------------------------

function PlayerShape::onAdd(%this,%obj)
{
   // Called when the PlayerData datablock is first 'read' by the engine (executable)
}

function PlayerShape::onRemove(%this, %obj)
{
   if (%obj.client.player == %obj)
      %obj.client.player = 0;
}

function PlayerShape::onNewDataBlock(%this,%obj)
{
   // Called when this PlayerData datablock is assigned to an object
}
