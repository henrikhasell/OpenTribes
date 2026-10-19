// V35 probe: exec'd on either server from GameData/Classic, never in the repo.
// Collisions run through the retail scripts; this file only moves objects
// and prints state.

function ocLog(%s)
{
   echo("OC " @ %s);
}

function ocScan(%grp, %db)
{
   for (%i = 0; %i < %grp.getCount(); %i++)
   {
      %o = %grp.getObject(%i);
      if (%o.getClassName() $= "SimGroup")
         ocScan(%o, %db);
      else if (%o.getClassName() $= "StaticShape" && $OC::found $= "" &&
               %o.getDataBlock().getName() $= %db)
         $OC::found = %o;
   }
}

function ocPlayer()
{
   return ClientGroup.getObject(0).player;
}

// Drop the player onto the top of the first StaticShape of a datablock.
// GeneratorLarge: retail runs no findEmptySeat. MPBTeleporter: retail runs
// MPBTeleporter::onCollision from the other side of the collision.
function ocDropOn(%db)
{
   $OC::found = "";
   ocScan(MissionGroup, %db);
   if ($OC::found $= "")
      ocScan(MissionCleanup, %db);
   %s = $OC::found;
   %p = ocPlayer();
   %b = %s.getWorldBox();
   %x = (getWord(%b, 0) + getWord(%b, 3)) / 2;
   %y = (getWord(%b, 1) + getWord(%b, 4)) / 2;
   %z = getWord(%b, 5) + 1.5;
   %p.setVelocity("0 0 0");
   %p.setTransform(%x SPC %y SPC %z SPC "0 0 1 0");
   ocLog("DROP " @ %db SPC "player=" @ %p SPC "shape=" @ %s SPC "team=" @ %s.team SPC "box=" @ %b SPC "xf=" @ %p.getTransform());
}

// A Wildcat on open ground near the player, spawned the pad-less way
// (V00's v00Spawn).
function ocWildcat()
{
   %p = ocPlayer();
   %team = %p.client.team;
   %pos = %p.getPosition();
   %x = getWord(%pos, 0) + 30;
   %y = getWord(%pos, 1);
   %v = ScoutVehicle.create(%team);
   %v.team = %team;
   MissionCleanup.add(%v);
   vehicleListAdd(ScoutVehicle, %v);
   %v.setTransform(%x SPC %y SPC (getTerrainHeight(%x SPC %y) + 3) SPC "0 0 1 0");
   if (%v.getTarget() != -1)
      setTargetSensorGroup(%v.getTarget(), %team);
   $OC::veh = %v;
   %p.setVelocity("0 0 0");
   %p.setTransform(%x SPC (%y - 8) SPC (getTerrainHeight(%x SPC (%y - 8)) + 1) SPC "0 0 1 0");
   ocLog("WILDCAT veh=" @ %v SPC "class=" @ %v.getClassName() SPC "mountable=" @ %v.mountable SPC
         "xf=" @ %v.getTransform() SPC "player=" @ %p.getTransform());
}

// Walk the player into the Wildcat: drop it onto the hull from just above.
function ocIntoWildcat()
{
   %p = ocPlayer();
   %b = $OC::veh.getWorldBox();
   %x = (getWord(%b, 0) + getWord(%b, 3)) / 2;
   %y = (getWord(%b, 1) + getWord(%b, 4)) / 2;
   %p.setVelocity("0 0 0");
   %p.setTransform(%x SPC %y SPC (getWord(%b, 5) + 1) SPC "0 0 1 0");
   ocLog("INTO veh=" @ $OC::veh SPC "vxf=" @ $OC::veh.getTransform() SPC "pxf=" @ %p.getTransform());
}

function ocState(%tag)
{
   %p = ocPlayer();
   %m = %p.getObjectMount();
   %n = -1;
   if (%m > 0)
      for (%i = 0; %i < 11; %i++)
         if (%m.getMountNodeObject(%i) == %p)
            %n = %i;
   ocLog("STATE " @ %tag SPC "player=" @ %p SPC "state=" @ %p.getState() SPC "mount=" @ %m SPC
         "node=" @ %n SPC "ctrl=" @ %p.client.getControlObject() SPC "xf=" @ %p.getTransform());
}

// Pass-through wrappers that print every argument the engine hands over,
// including slots past the declared ones, so a vector and a length in the
// engine's call would show up. Used where the script debugger cannot stop
// (our engine's BREAK never arrives).
package ocProbe {
function Armor::onCollision(%this, %obj, %col, %forceVehicleNode, %x5, %x6)
{
   ocLog("ARGS Armor::onCollision this=" @ %this.getName() SPC "obj=" @ %obj SPC "col=" @ %col SPC
         "colclass=" @ %col.getClassName() SPC "coldb=" @ %col.getDataBlock().getName() SPC
         "a4=<" @ %forceVehicleNode @ "> a5=<" @ %x5 @ "> a6=<" @ %x6 @ ">");
   Parent::onCollision(%this, %obj, %col, %forceVehicleNode);
}
function findEmptySeat(%vehicle, %player, %forceNode)
{
   ocLog("CALL findEmptySeat veh=" @ %vehicle SPC "class=" @ %vehicle.getClassName() SPC
         "db=" @ %vehicle.getDataBlock().getName() SPC "forceNode=<" @ %forceNode @ ">");
   return Parent::findEmptySeat(%vehicle, %player, %forceNode);
}
function MPBTeleporter::onCollision(%data, %obj, %col, %x4, %x5, %x6)
{
   ocLog("ARGS MPBTeleporter::onCollision data=" @ %data.getName() SPC "obj=" @ %obj SPC "col=" @ %col SPC
         "colclass=" @ %col.getClassName() SPC "a4=<" @ %x4 @ "> a5=<" @ %x5 @ "> a6=<" @ %x6 @ ">");
   Parent::onCollision(%data, %obj, %col);
}
};

ocLog("LOADED");
