//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This file contains shape declarations and functions used by the mission
// editor. The mission editor invokes datablock create methods when it
// wants to create an object of that type.

//-----------------------------------------------------------------------------
// Declare a static shape create method.  This allows DTS objects to be loaded
// using the StaticShape simulation object.

function StaticShapeData::create(%data)
{
   %obj = new StaticShape() {
      dataBlock = %data;
   };
   return %obj;
}


//-----------------------------------------------------------------------------
// The two marker datablocks this file used to declare -- WayPointMarker and
// SpawnSphereMarker, both pointing at data/shapes/markers/octahedron.dts -- are
// gone, and so is that shape.  It is a DTS version 18 file and this engine
// rejects anything below 19 (ts/tsShape.cpp:1549), so both datablocks failed to
// preload, and with SpawnSphereMarker missing the SpawnSphere in orchard.mis
// then failed to register and the game spawned at the origin.
//
// core/art/datablocks/markers.cs declares both under the same names against
// core/art/shapes/octahedron.dts, which is the same octahedron at version 24,
// and createServer() execs it before onServerCreated() gets here.
//
// MissionMarkerData::create() went with them.  The editor's Library pane builds
// "<class>::create(<name>)" for every shape datablock it lists
// (worldEditor/scripts/editors/creator.ed.cs), so the three markers core
// declares have no create method -- but neither do they in the stock Full
// template, so this is upstream behaviour rather than something the move to a
// standalone project introduced.

//-----------------------------------------------------------------------------
// Misc. server commands used for editing.
//
// core/scripts/server/commands.cs defines all three of these as well, but its
// versions do not set the camera mode the way TGE's did.  These are exec'd
// from onServerCreated(), after initBaseServer(), so they win.
//
// syncEditorGui() stands in for the clientCmdSyncEditorGui() that core calls
// at the end of each: it lives in the tools directory, which a player build
// does not load, and calling it when absent logs "Unable to find function".
//-----------------------------------------------------------------------------

function syncEditorGui()
{
   if (isFunction("clientCmdSyncEditorGui"))
      clientCmdSyncEditorGui();
}

function serverCmdToggleCamera(%client)
{
   if ($Server::ServerType $= "SinglePlayer") {
      %control = %client.getControlObject();
      if (%control == %client.player) {
         %control = %client.camera;
         %control.mode = toggleCameraFly;
      }
      else {
         %control = %client.player;
         %control.mode = observerFly;
      }
      %client.setControlObject(%control);
      syncEditorGui();
   }
}

function serverCmdDropPlayerAtCamera(%client)
{
   if (!%client.player.isMounted())
      %client.player.setTransform(%client.camera.getTransform());
   %client.player.setVelocity("0 0 0");
   %client.setControlObject(%client.player);
   syncEditorGui();
}

function serverCmdDropCameraAtPlayer(%client)
{
   %client.camera.setTransform(%client.player.getEyeTransform());
   %client.camera.setVelocity("0 0 0");
   %client.setControlObject(%client.camera);
   syncEditorGui();
}


function dropFreakinCameraAtPlayer()
{
   $dropcameracount++;
   %cl = ClientGroup.getObject(0);
   if (%cl.camera) dropCameraAtPlayer(1);
   else if ($dropcameracount<100) schedule(100,0,dropFreakinCameraAtPlayer);
}

