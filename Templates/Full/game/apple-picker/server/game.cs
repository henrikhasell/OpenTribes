//-----------------------------------------------------------------------------
// Server-side game rules: hand each client a player to walk around with.
//
// The game itself -- the apples, the count and the win -- is not here.  It is in
// server/apple.cs, which hooks this file's callbacks through a package, so that
// one file plus its exec line is the whole feature.
//
// From tutorial.base/server/game.cs, with the spawn point snapped to the ground
// and the two print-clearing stubs added.
//-----------------------------------------------------------------------------

function onServerCreated()
{
   $Server::GameType = "apple-picker";
   $Server::MissionType = "None";

   // Datablocks MUST be (re)loaded here rather than at init time: every
   // destroyServer() ends with deleteDataBlocks(), so anything registered in
   // initServer() is already gone the next time a mission loads.
   exec("./camera.cs");
   exec("./player.cs");
   exec("./editor.cs");
}

function onServerDestroyed()
{
   // This function is called as part of a server shutdown.
}


//-----------------------------------------------------------------------------

function onMissionLoaded()
{
   // Defensive: EditTSCtrl::renderMissionArea (editTSCtrl.cc:265) dereferences
   // Sim::findObject("MissionArea") with no null check whenever a TerrainBlock
   // named "Terrain" exists, and EditorGui.gui sets renderMissionArea = 1.  If
   // someone deletes the MissionArea in the editor and saves, every later boot
   // would segfault on the first editor frame.  Put one back.
   if (isObject(Terrain) && !isObject(MissionArea))
   {
      warn("apple-picker: mission has a Terrain but no MissionArea -- recreating it.");
      %area = new MissionArea(MissionArea) {
         area = "-1024 -1024 2048 2048";
         flightCeiling = "300";
         flightCeilingRange = "20";
      };
      MissionGroup.add(%area);
   }
}

function onMissionEnded()
{
   // Called by endMission(), right before the mission is destroyed.
}

function onMissionReset() { }

// common/server/missionLoad.cs:23-24 clears on-screen prints at the top of
// every load.  This mod's HUD is a plain GuiTextCtrl rather than the starter
// kits' centerPrint dialogs, so these are no-ops -- but they have to exist, or
// every mission load logs two "Unable to find function" errors.  Stock
// tutorial.base does exactly that.
function clearCenterPrintAll() { }
function clearBottomPrintAll() { }


//-----------------------------------------------------------------------------
// Dealing with client connections.

function GameConnection::onClientEnterGame(%this)
{
   // Every client gets a camera object.  It is not what they control -- the
   // Player below is -- but "alt c" swaps between the two, and server/editor.cs's
   // serverCmdDropCameraAtPlayer needs it to exist.
   %this.camera = new Camera() {
      dataBlock = Observer;
   };
   MissionCleanup.add(%this.camera);
   %this.camera.scopeToClient(%this);

   // Create a player object.
   %spawnPoint = pickSpawnPoint();
   %this.createPlayer(%spawnPoint);
}

function GameConnection::onClientLeaveGame(%this)
{
   if (isObject(%this.camera))
      %this.camera.delete();
   if (isObject(%this.player))
      %this.player.delete();
}


//-----------------------------------------------------------------------------

function GameConnection::createPlayer(%this, %spawnPoint)
{
   if (%this.player > 0)  {
      // The client should not have a player currently assigned.  Assigning a
      // new one could result in a player ghost.
      error( "Attempting to create an angus ghost!" );
   }

   // Create the player object
   %player = new Player() {
      dataBlock = PlayerShape;
      client = %this;
   };
   MissionCleanup.add(%player);

   // Player setup...
   %player.setTransform(%spawnPoint);
   %player.setShapeName(%this.name);

   // Update the camera to start with the player
   %this.camera.setTransform(%player.getEyeTransform());

   // Give the client control of the player
   %this.player = %player;
   %this.setControlObject(%player);
}


//-----------------------------------------------------------------------------

function pickSpawnPoint()
{
   %group = nameToID("MissionGroup/PlayerDropPoints");
   if (%group == -1 || %group.getCount() == 0)
   {
      error("apple-picker: no MissionGroup/PlayerDropPoints -- spawning at the origin");
      return "0 0" SPC (getTerrainHeight("0 0") + 1) SPC "0 0 1 0";
   }

   // Pick the first object in the drop point group and use its location.
   %xform = %group.getObject(0).getTransform();
   %x = getWord(%xform, 0);
   %y = getWord(%xform, 1);

   // Snap to the ground rather than trusting the authored z.  Stock flat.mis
   // drops the player at z = 166 over a terrain that is flat at z = 0, so the
   // game would open with a silent 166-metre fall.  (It does no damage --
   // impact damage goes through PlayerData::onImpact and nothing defines one --
   // but it is not a start, and the apples are out of sight for the whole of it.)
   //
   // getTerrainHeight() subtracts the TerrainBlock's x/y offset but does NOT add
   // its z back (terrain/terrData.cc:1222-1237), so this is only right while the
   // terrain sits at z = 0.  In this mission it does.
   return %x SPC %y SPC (getTerrainHeight(%x SPC %y) + 1) SPC getWords(%xform, 3, 6);
}
