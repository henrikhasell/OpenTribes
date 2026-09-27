//-----------------------------------------------------------------------------
// Server-side game rules: hand each client a player to walk around with.
//
// The game itself -- the apples, the count and the win -- is not here.  It is in
// server/apple.cs, which hooks this file's callbacks through a package, so that
// one file plus its exec line is the whole feature.
//
// From tutorial.base/server/game.cs, with the spawn point snapped to the ground.
//
// These are plain redefinitions rather than a package, and initServer() execs
// this file after initBaseServer(), so each one replaces core's version of the
// same name in core/scripts/server/game.cs.
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

   // The editors write what they create to these paths and nothing else reads
   // them back, so without this a decal or a forest item made in the Mission
   // Editor is gone at the next mission load.  The paths are the editors' own
   // hardcoded defaults, not a choice this game gets to make -- see
   // $DATABLOCK_EDITOR_DEFAULT_FILENAME and friends under tools/.  They do not
   // exist until something is saved, hence the guard.
   loadEditorDatablocks();
}

function loadEditorDatablocks()
{
   %files = "art/decals/managedDecalData.cs" TAB
            "art/datablocks/managedDatablocks.cs" TAB
            "art/forest/managedItemData.cs" TAB
            "art/shapes/particles/managedParticleData.cs" TAB
            "art/shapes/particles/managedParticleEmitterData.cs";

   for (%i = 0; %i < getFieldCount(%files); %i++)
   {
      %file = getField(%files, %i);
      if (isScriptFile(%file))
         exec(%file);
   }
}

function onServerDestroyed()
{
   // This function is called as part of a server shutdown.
}


//-----------------------------------------------------------------------------

// This used to put a MissionArea back if the mission had lost one, because
// TGE's EditTSCtrl::renderMissionArea dereferenced it with no null check and
// segfaulted on the first editor frame without it.  EditTSCtrl here opens with
// `if ( !obj ) return;` (editTSCtrl.cpp:840), so the hazard is gone and so is
// the workaround -- and with it the MissionArea in orchard.mis.
function onMissionLoaded() { }

function onMissionEnded()
{
   // Called by endMission(), right before the mission is destroyed.
}

function onMissionReset() { }

// The name and description the loading screen shows.
//
// buildLoadInfo() reads either of the two blocks a mission can carry its load
// info in -- Torque 3D's LevelInfo(theLevelInfo) or TGE's
// ScriptObject(MissionInfo) -- but what it leaves behind is named after
// whichever it found, and core's version of this only ever looks for the first.
// This game's missions carry the second, so it has to look for both.
function sendLoadInfoToClient(%client)
{
   %info = theLevelInfo;
   if (!isObject(%info))
      %info = MissionInfo;

   messageClient(%client, 'MsgLoadInfo', "", %info.levelName);

   for (%i = 0; %info.desc[%i] !$= ""; %i++)
      messageClient(%client, 'MsgLoadDescripition', "", %info.desc[%i]);

   messageClient(%client, 'MsgLoadInfoDone');
}

// The clearCenterPrintAll/clearBottomPrintAll stubs this file used to carry are
// gone: core/scripts/server/centerPrint.cs defines both, and initBaseServer()
// execs it.


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
   %player.setShapeName(%this.playerName);

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

   // Snap to the ground rather than trusting the authored z, so that editing a
   // drop point cannot open the game with a silent fall and the apples out of
   // sight for the whole of it.
   //
   // getTerrainHeight() subtracts the TerrainBlock's x/y offset but does NOT add
   // its z back (terrain/terrData.cpp:1300-1302) -- unchanged from TGE -- so this
   // is only right while the terrain sits at z = 0.  In this mission it does.
   return %x SPC %y SPC (getTerrainHeight(%x SPC %y) + 1) SPC getWords(%xform, 3, 6);
}
