// Turret stage 5 test helpers, exec'd on our server and on retail's.
function t5Log(%s) { echo("T5 " @ getSimTime() SPC %s); }

package T5Pkg {
function TurretData::selectTarget(%this, %turret)
{
   %before = %turret.getTargetObject();
   Parent::selectTarget(%this, %turret);
   if (%turret == $T5::turret)
      t5Log("SELECT " @ %turret SPC %before @ " -> " @ %turret.getTargetObject());
}
function TurretData::replaceCallback(%this, %turret, %engineer)
{
   t5Log("REPLACE " @ %turret SPC %engineer SPC %turret.getMountedImage(0).getName());
   Parent::replaceCallback(%this, %turret, %engineer);
   t5Log("REPLACED " @ %turret.getMountedImage(0).getName() SPC %turret.initialBarrel);
}
};
activatePackage(T5Pkg);

function t5Turrets() { t5TurretsIn(nameToID("MissionGroup")); }

function t5TurretsIn(%g)
{
   for (%i = 0; %i < %g.getCount(); %i++)
   {
      %o = %g.getObject(%i);
      if (%o.getClassName() $= "SimGroup")
         t5TurretsIn(%o);
      else if (%o.getClassName() $= "Turret")
         t5Log("TURRET " @ %o SPC %o.getName() SPC %o.getDataBlock().getName() SPC %o.team SPC %o.isPowered() SPC %o.getMountedImage(0).getName() SPC %o.initialBarrel SPC %o.getDamageState() SPC %o.getTarget() SPC getTargetSensorGroup(%o.getTarget()) SPC %o.getTargetObject() SPC %o.getImageState(0));
   }
}

function t5Deploy(%db, %barrel, %pos, %team)
{
   %t = new Turret() { dataBlock = %db; initialBarrel = %barrel; };
   MissionCleanup.add(%t);
   %t.setTransform(%pos SPC "0 0 1 0");
   %t.team = %team;
   if (%t.getTarget() != -1)
      setTargetSensorGroup(%t.getTarget(), %team);
   %t.setSelfPowered();
   $T5::turret = %t;
   t5Log("DEPLOYED " @ %t SPC %db SPC %t.getMountedImage(0).getName() SPC %t.initialBarrel SPC %t.getTarget() SPC getTargetSensorGroup(%t.getTarget()) SPC %t.isPowered());
   return %t;
}

// A player of %team held at %pos, seen by every sensor group.
function t5Enemy(%pos, %team)
{
   %p = new Player() { dataBlock = LightMaleHumanArmor; };
   MissionCleanup.add(%p);
   %p.team = %team;
   %tg = allocTarget("", "", "", "", %team, LightMaleHumanArmor.getId(), 1.0);
   %p.setTarget(%tg);
   setTargetAlwaysVisMask(%tg, 0xffffffff);
   $T5::enemy = %p;
   t5Hold(%p, %pos);
   t5Log("ENEMY " @ %p SPC %tg SPC getTargetSensorGroup(%tg) SPC %p.getWorldBoxCenter());
   return %p;
}

function t5Hold(%pl, %pos)
{
   cancel($T5::hold);
   if (!isObject(%pl))
      return;
   %pl.setTransform(%pos SPC "0 0 1 0");
   %pl.setVelocity("0 0 0");
   %pl.setDamageLevel(0);
   $T5::hold = schedule(100, 0, t5Hold, %pl, %pos);
}

function t5Watch(%n)
{
   cancel($T5::watch);
   %t = $T5::turret;
   if (!isObject(%t) || %n <= 0)
      return;
   %v = %t.getMuzzleVector(0);
   %h = mRadToDeg(mAtan(getWord(%v, 0), getWord(%v, 1)));
   %e = mRadToDeg(mAsin(getWord(%v, 2)));
   t5Log("W " @ %t.getTargetObject() SPC %t.getImageState(0) SPC %h SPC %e SPC %t.getEnergyLevel() SPC %t.getRechargeRate());
   $T5::watch = schedule(96, 0, t5Watch, %n - 1);
}

// An engineer of the turret's team carrying %item, whose pack image is
// %image, asks for its barrel.
function t5Swap(%image, %item, %pos)
{
   %e = new Player() { dataBlock = LightMaleHumanArmor; };
   MissionCleanup.add(%e);
   %e.team = $T5::turret.team;
   %e.setTransform(%pos SPC "0 0 1 0");
   %e.setInventory(%item, 1);
   %e.mountImage(%image, $BackpackSlot);
   $T5::engineer = %e;
   t5Log("SWAP " @ %e SPC %e.getMountedImage($BackpackSlot).getName() SPC $T5::turret.getMountedImage(0).getName());
   t5Log("SWAPPED " @ $T5::turret.initiateBarrelSwap(%e) SPC $T5::turret.getMountedImage(0).getName());
}

function t5Methods()
{
   %t = $T5::turret;
   t5Log("M valid " @ %t.isValidTarget($T5::enemy) SPC %t.isValidTarget(%t) SPC %t.isValidTarget(0));
   t5Log("M set " @ %t.setTargetObject($T5::enemy) SPC %t.getTargetObject());
   %t.clearTarget();
   t5Log("M clear " @ %t.getTargetObject());
   t5Log("M set0 " @ %t.setTargetObject(0) SPC %t.getTargetObject());
   %t.setCapacitorLevel(12.7);
   t5Log("M cap " @ %t.getCapacitorLevel());
   %t.setCapacitorLevel(-3.7);
   t5Log("M capneg " @ %t.getCapacitorLevel());
   %t.setCapacitorLevel(0);
   %t.setCapacitorRechargeRate(0);
   %t.setSkill(0);
   %t.setSkill(1);
   %t.setAutoFire(false);
   %t.setAutoFire(true);
}

// Where setDeployRotation puts a turret, for a few surface normals.
function t5DeployRot()
{
   %t = new Turret() { dataBlock = TurretDeployedOutdoor; };
   MissionCleanup.add(%t);
   %n[0] = "0 0 1";
   %n[1] = "0 0 -1";
   %n[2] = "1 0 0";
   %n[3] = "0 1 0";
   %n[4] = "-1 0 0";
   %n[5] = "0 -1 0";
   %n[6] = "0.6 0 0.8";
   %n[7] = "0 -0.6 0.8";
   %n[8] = "0.48 0.64 0.6";
   %n[9] = "0.8 0.6 0";
   %n[10] = "0 0 2";
   for (%i = 0; %i < 11; %i++)
   {
      %t.setDeployRotation("10 20 30", %n[%i]);
      t5Log("DROT " @ %n[%i] @ " : " @ %t.getTransform() @ " : " @ %t.getForwardVector() @ " : " @ %t.getUpVector());
   }
   %t.delete();
}

function t5Probe()
{
   %t = $T5::turret;
   %e = $T5::enemy;
   %m = %t.getMuzzlePoint(0);
   %c = %e.getWorldBoxCenter();
   t5Log("P muzzle " @ %m @ " centre " @ %c @ " pos " @ %t.getPosition() @ " valid " @ %t.isValidTarget(%e));
   t5Log("P ray terrain " @ containerRayCast(%m, %c, $TypeMasks::TerrainObjectType, %t));
   t5Log("P ray interior " @ containerRayCast(%m, %c, $TypeMasks::InteriorObjectType, %t));
   t5Log("P ray vehicle " @ containerRayCast(%m, %c, $TypeMasks::VehicleObjectType, %t));
   t5Log("P down " @ containerRayCast(getWords(%c, 0, 1) SPC 500, getWords(%c, 0, 1) SPC -100, $TypeMasks::TerrainObjectType, 0));
   t5Log("P downt " @ containerRayCast(getWords(%m, 0, 1) SPC 500, getWords(%m, 0, 1) SPC -100, $TypeMasks::TerrainObjectType, 0));
   t5Log("P groups " @ getTargetSensorGroup(%t.getTarget()) SPC getTargetSensorGroup(%e.getTarget()) SPC %t.isPowered() SPC %t.getMountedImage(0).attackRadius);
   InitContainerRadiusSearch(%m, %t.getMountedImage(0).attackRadius, $TypeMasks::PlayerObjectType | $TypeMasks::VehicleObjectType);
   while ((%o = ContainerSearchNext()) != 0)
      t5Log("P found " @ %o SPC %o.getClassName() SPC %t.isValidTarget(%o));
}
