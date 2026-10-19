// V00 server helpers: exec'd on the retail dedicated server, never in the repo.
// Every vehicle path goes through the retail scripts (create, onCollision,
// vehicleDeploy, applyDamage); this file only picks the objects and prints state.

function v00Log(%s)
{
   echo("V00 " @ %s);
}

// The one human client: everything else in ClientGroup is a bot.
function v00Client()
{
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %c = ClientGroup.getObject(%i);
      if (!%c.isAIControlled())
         return %c;
   }
   return 0;
}

function v00ScanPads(%grp)
{
   for (%i = 0; %i < %grp.getCount(); %i++)
   {
      %o = %grp.getObject(%i);
      if (%o.getClassName() $= "SimGroup")
         v00ScanPads(%o);
      else if (%o.getClassName() $= "StaticShape" && %o.getDataBlock().getName() $= "StationVehiclePad")
      {
         $V00::pad[%o.team] = %o;
         v00Log("PAD team=" @ %o.team SPC "pad=" @ %o SPC "station=" @ %o.station SPC
                "padxf=" @ %o.getTransform() SPC "stxf=" @ %o.station.getTransform() SPC
                "ready=" @ %o.ready);
      }
   }
}

function v00Pads()
{
   v00ScanPads(MissionGroup);
}

// Ground height under x y, plus a lift.
function v00At(%x, %y, %lift)
{
   return %x SPC %y SPC (getTerrainHeight(%x SPC %y) + %lift);
}

// The plan's pad-less spawn: <Data>::create(%team), a fixed transform, and the
// bookkeeping createVehicle does (cleanup, vehicle list, sensor group).
function v00Spawn(%block, %team, %pos, %angleDeg)
{
   %obj = %block.create(%team);
   if (!isObject(%obj))
   {
      v00Log("SPAWN-FAILED " @ %block);
      return 0;
   }
   %obj.team = %team;
   MissionCleanup.add(%obj);
   vehicleListAdd(%block, %obj);
   %obj.setTransform(%pos SPC "0 0 1" SPC mDegToRad(%angleDeg));
   if (%obj.getTarget() != -1)
      setTargetSensorGroup(%obj.getTarget(), %team);
   %t = %obj.getMountNodeObject(10);
   if (%t > 0 && %t.getTarget() != -1)
      setTargetSensorGroup(%t.getTarget(), %team);
   // Moving from its first tick: a vehicle placed motionless sometimes never
   // woke again, to moves, setVelocity or impulses alike.
   %obj.setVelocity("0 0 -0.5");
   $V00::veh = %obj;
   $V00::last[%block] = %obj;
   v00Log("SPAWN " @ %block SPC "id=" @ %obj SPC "class=" @ %obj.getClassName() SPC
          "team=" @ %team SPC "xf=" @ %obj.getTransform());
   return %obj;
}

// Seat a player the way a touch does: Armor::onCollision with the node forced,
// which is also what testVehicleForMount uses.
function v00Mount(%player, %veh, %node)
{
   if (%player.client.armor !$= "Light")
      %player.setArmor("Light");
   %player.getDataBlock().onCollision(%player, %veh, %node);
   v00Log("MOUNT player=" @ %player SPC "veh=" @ %veh SPC "node=" @ %node SPC
          "mount=" @ %player.getObjectMount() SPC "ctrl=" @ %player.client.getControlObject() SPC
          "pctrl=" @ %player.getControlObject());
}

function v00MountMe(%veh, %node)
{
   v00Mount(v00Client().player, %veh, %node);
}

// Put the human on the pad's station trigger, so the retail trigger runs.
function v00ToPad(%team, %dx, %dy)
{
   %pl = v00Client().player;
   %st = $V00::pad[%team].station;
   %p = getWords(%st.getTransform(), 0, 2);
   %p = VectorAdd(%p, %dx SPC %dy SPC "0.5");
   %pl.setVelocity("0 0 0");
   %pl.setTransform(%p SPC "0 0 1 0");
   v00Log("TOPAD player=" @ %pl SPC "xf=" @ %pl.getTransform());
}

function v00Put(%obj, %pos, %angleDeg)
{
   %obj.setVelocity("0 0 0");
   %obj.setTransform(%pos SPC "0 0 1" SPC mDegToRad(%angleDeg));
   v00Log("PUT " @ %obj SPC "xf=" @ %obj.getTransform());
}

function v00PutMe(%pos, %angleDeg)
{
   v00Put(v00Client().player, %pos, %angleDeg);
}

function v00Bot(%name, %team)
{
   %c = aiConnect(%name, %team, 0.9, false);
   $V00::bot[%name] = %c;
   v00Log("BOT " @ %name SPC "client=" @ %c SPC "team=" @ %team);
   return %c;
}

// Retail's own debug pilot task (aiDebug.cs testPilot / AITestPilot).
function v00Pilot(%bot, %veh)
{
   testPilot(%bot, %veh);
   v00Log("PILOTTASK bot=" @ %bot SPC "veh=" @ %veh SPC "task=" @ %bot.pilotTask);
}

function v00Destroy(%veh)
{
   %d = %veh.getDataBlock();
   v00Log("DESTROY " @ %veh SPC %d.getName() SPC "maxDamage=" @ %d.maxDamage SPC "xf=" @ %veh.getTransform());
   %veh.applyDamage(%d.maxDamage + 0.1);
}

function v00Observe(%target)
{
   %c = v00Client();
   serverCmdClientMakeObserver(%c);
   schedule(1500, 0, "v00Observe2", %c, %target);
}

function v00Observe2(%c, %target)
{
   serverCmdObserveClient(%c, %target);
   v00Log("OBSERVE client=" @ %c SPC "target=" @ %target SPC "mode=" @ %c.camera.mode SPC
          "ctrl=" @ %c.getControlObject());
}

function v00Mounts(%o)
{
   %s = "";
   %d = %o.getDataBlock();
   for (%i = 0; %i < 11; %i++)
   {
      %m = %o.getMountNodeObject(%i);
      if (%m > 0)
         %s = %s @ " n" @ %i @ "=" @ %m @ ":" @ %m.getClassName() @ ":" @ %m.getDataBlock().getName();
   }
   return %s;
}

function v00ProbeObj(%tag, %o)
{
   if (!isObject(%o))
   {
      v00Log(%tag SPC "id=" @ %o SPC "gone");
      return;
   }
   %d = %o.getDataBlock();
   v00Log(%tag SPC "id=" @ %o SPC "class=" @ %o.getClassName() SPC "db=" @ %d.getName() SPC
          "team=" @ %o.team SPC "xf=" @ %o.getTransform());
   v00Log(%tag @ "+ id=" @ %o SPC "vel=" @ %o.getVelocity() SPC "dmg=" @ %o.getDamageLevel() SPC
          "state=" @ %o.getDamageState() SPC "energy=" @ %o.getEnergyLevel() SPC
          "cloaked=" @ %o.isCloaked() SPC "ctlclient=" @ %o.getControllingClient() SPC
          "mount=" @ %o.getObjectMount() SPC "mounts:" @ v00Mounts(%o));
}

// One line per vehicle, turret, StationFXVehicle, player and client.
function v00Probe(%why)
{
   v00Log("PROBE-BEGIN " @ %why SPC "simtime=" @ getSimTime());
   %n = MissionCleanup.getCount();
   for (%i = 0; %i < %n; %i++)
   {
      %o = MissionCleanup.getObject(%i);
      %c = %o.getClassName();
      if (%c $= "HoverVehicle" || %c $= "FlyingVehicle" || %c $= "WheeledVehicle")
      {
         v00ProbeObj("VEH", %o);
         %t = %o.getMountNodeObject(10);
         if (%t > 0)
            v00ProbeObj("TUR", %t);
         if (%o.turret !$= "" && isObject(%o.turret))
         {
            v00ProbeObj("MPBTUR", %o.turret);
            v00Log("MPBTUR+ target=" @ %o.turret.getTargetObject() SPC "deployed=" @ %o.deployed SPC
                   "fullyDeployed=" @ %o.fullyDeployed SPC "station=" @ %o.station SPC "beacon=" @ %o.beacon);
         }
      }
      else if (%c $= "StationFXVehicle" || %c $= "BombProjectile" || %c $= "TargetProjectile" ||
               %c $= "BeaconObject" || %c $= "SeekerProjectile" || %c $= "Debris" || %c $= "Turret")
         v00Log("OBJ id=" @ %o SPC "class=" @ %c SPC "db=" @ %o.getDataBlock().getName() SPC
                "xf=" @ %o.getTransform());
   }
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %cl = ClientGroup.getObject(%i);
      %p = %cl.player;
      %co = %cl.getControlObject();
      v00Log("CLIENT id=" @ %cl SPC "ai=" @ %cl.isAIControlled() SPC "team=" @ %cl.team SPC
             "armor=" @ %cl.armor SPC "ctrl=" @ %co SPC (isObject(%co) ? %co.getClassName() : "-") SPC
             "player=" @ %p SPC "camera=" @ %cl.camera SPC "cammode=" @ %cl.camera.mode);
      if (isObject(%p))
      {
         %pc = %p.getControlObject();
         %m = %p.getObjectMount();
         %node = "";
         if (isObject(%m))
            for (%j = 0; %j < 11; %j++)
               if (%m.getMountNodeObject(%j) == %p)
                  %node = %j;
         v00Log("PLAYER id=" @ %p SPC "db=" @ %p.getDataBlock().getName() SPC "state=" @ %p.getState() SPC
                "pctrl=" @ %pc SPC (isObject(%pc) ? %pc.getClassName() : "-") SPC
                "mount=" @ %m SPC "node=" @ %node SPC "xf=" @ %p.getTransform());
      }
   }
   v00Log("PROBE-END " @ %why);
}

// Turn the game's clock off: the session runs well past a CTF time limit.
$Host::TimeLimit = 600;
v00Log("SERVER-HELPERS-LOADED");

// Out of any seat: retail's own dismount, as a death or a jump runs it.  A
// gunner's jump goes to the turret (playerDismount) and leaves him seated.
function v00Dismount()
{
   %p = v00Client().player;
   %p.getDataBlock().doDismount(%p, true);
   v00Log("DISMOUNT player=" @ %p SPC "mount=" @ %p.getObjectMount() SPC "ctrl=" @ %p.client.getControlObject());
}

v00Log("SERVER-HELPERS-LOADED 2");
