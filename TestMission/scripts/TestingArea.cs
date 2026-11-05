// Exec'd by TestingArea.mis, so it holds only while that mission runs. The
// package goes with the game's own at the mission's end.

package TestingArea {

function DefaultGame::missionLoadDone(%game)
{
   Parent::missionLoadDone(%game);
   $TestingArea::TimeLimit = $Host::TimeLimit;
   $Host::TimeLimit = 0;
}

function DefaultGame::deactivatePackages(%game)
{
   Parent::deactivatePackages(%game);
   deactivatePackage(TestingArea);
   $Host::TimeLimit = $TestingArea::TimeLimit;
}

// No score ends the game. Retail's check never uses its default limit, as an
// unset CTF_scoreLimit times the points per cap is 0, so any score won.
function CTFGame::checkScoreLimit(%game, %team)
{
}

// A dedicated server resets when its last player drops, which loads another
// mission over this one.
function GameConnection::onDrop(%client, %reason)
{
   %resetting = $resettingServer;
   $resettingServer = true;
   Parent::onDrop(%client, %reason);
   $resettingServer = %resetting;
}

// "night" is the mission's own lighting.
function setLighting(%preset)
{
   if(!isObject(Sun))
      return;
   switch$(%preset)
   {
      case "night":
         replaceSun("0.2 0.2 0.2 1", "0.2 0.2 0.2 1");
      case "full":
         replaceSun("1 1 1 1", "1 1 1 1");
      case "none":
         replaceSun("0 0 0 1", "0 0 0 1");
      case "day":
         replaceSun("0.7 0.7 0.7 1", "0.5 0.5 0.5 1");
      case "sunrise":
         replaceSun("0.9 0.6 0.35 1", "0.45 0.35 0.3 1");
      case "dusk":
         replaceSun("0.15 0.2 0.4 1", "0.1 0.12 0.25 1");
      default:
         error("setLighting: unknown preset \"" @ %preset @ "\", use night, full, none, day, sunrise or dusk.");
         return;
   }
   messageAll('MsgClient', '\c2Lighting set to %1.', %preset);
}

// A Sun sends its fields to clients only when it is ghosted, so a changed one
// is replaced by a new one.
function replaceSun(%color, %ambient)
{
   %old = Sun.getId();
   %new = new Sun() {
      position = %old.position;
      rotation = %old.rotation;
      scale = %old.scale;
      direction = %old.direction;
      color = %color;
      ambient = %ambient;
      texture[0] = %old.texture[0];
      texture[1] = %old.texture[1];
      texture[2] = %old.texture[2];
      texture[3] = %old.texture[3];
      texture[4] = %old.texture[4];
      lensFlareScale = %old.lensFlareScale;
      lensFlareIntensity = %old.lensFlareIntensity;
      frontFlareSize = %old.frontFlareSize;
      backFlareSize = %old.backFlareSize;
      flareColor = %old.flareColor;
      locked = %old.locked;
   };
   %old.delete();
   %new.setName("Sun");
   MissionGroup.add(%new);

   // reghostClients();

   // Give the new Sun time to reach the clients before they light with it.
   schedule(2000, 0, relightClients);
}

// A Sun is sent to a client only when its ghosting starts, as the mission
// loads, so it is started over, as serverCmdMissionStartPhase2Done starts it.
// function reghostClients()
// {
//    %count = ClientGroup.getCount();
//    for(%i = 0; %i < %count; %i++)
//    {
//       %client = ClientGroup.getObject(%i);
//       if(%client.isAIControlled())
//          continue;
//       %client.resetGhosting();
//       %client.activateGhosting();
//       %client.camera.scopeToClient(%client);
//    }
// }

// SetLighting is for clients that do not take a Sun's fields from its ghost.
function relightClients()
{
   %count = ClientGroup.getCount();
   for(%i = 0; %i < %count; %i++)
   {
      %client = ClientGroup.getObject(%i);
      if(!%client.isAIControlled())
      {
         commandToClient(%client, 'SetLighting', Sun.color, Sun.ambient);
         commandToClient(%client, 'RelightMission');
      }
   }
}

function serverCmdSetLighting(%client, %preset)
{
   setLighting(%preset);
}

};

activatePackage(TestingArea);
