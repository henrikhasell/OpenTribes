// V03 server helpers: exec'd on the retail dedicated server after v00srv.cs,
// never in the repo.  They make a slot empty in the same update that carries
// a new fire count, which is what crashed our client's readT2Images.

package V03Empty {
function ShapeBaseImageData::onFire(%data, %obj, %slot)
{
   %r = Parent::onFire(%data, %obj, %slot);
   if ($V03::emptyOnFire && %obj == $V03::target)
   {
      // Scheduled, not inline: unmounting inside the server's own state
      // transition is not what is under test.  It still lands before the
      // next packet, so the update has no datablock and a changed count.
      %obj.schedule(0, "unmountImage", %slot);
      echo("V03 EMPTIED-ON-FIRE obj=" @ %obj SPC "slot=" @ %slot SPC "img=" @ %data.getName() SPC "simtime=" @ getSimTime());
   }
   return %r;
}
};
activatePackage(V03Empty);

function v03Arm()
{
   %p = v00Client().player;
   %p.setInventory(Disc, 1);
   %p.setInventory(DiscAmmo, 15);
   %p.use(Disc);
   echo("V03 ARM player=" @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "ammo=" @ %p.getInventory(DiscAmmo));
}

function v03Fire()
{
   %p = v00Client().player;
   $V03::target = %p;
   $V03::emptyOnFire = 1;
   %p.setImageTrigger(0, true);
   echo("V03 FIRE player=" @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "simtime=" @ getSimTime());
   schedule(400, 0, "v03FireOff", %p);
}

function v03FireOff(%p)
{
   %p.setImageTrigger(0, false);
   $V03::emptyOnFire = 0;
   echo("V03 FIREOFF player=" @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "ammo=" @ %p.getInventory(DiscAmmo) SPC "simtime=" @ getSimTime());
}

// The plan's check: a bomber, the client at the gunner's seat, then the
// bomber destroyed, which runs BomberFlyer::deleteAllMounted.
function v03Bomber()
{
   $V03::bomber = v00Spawn(BomberFlyer, 1, v00At(200, -185, 6), 0);
   $V03::turret = $V03::bomber.getMountNodeObject(10);
   echo("V03 BOMBER veh=" @ $V03::bomber SPC "turret=" @ $V03::turret);
}

function v03Count(%why)
{
   %n = 0;
   for (%i = 0; %i < MissionCleanup.getCount(); %i++)
      if (MissionCleanup.getObject(%i).getClassName() $= "LinearFlareProjectile")
         %n++;
   %t = $V03::turret;
   %s = "";
   if (isObject(%t))
      for (%j = 0; %j < 8; %j++)
         %s = %s SPC "s" @ %j @ "=" @ %t.getMountedImage(%j);
   echo("V03 COUNT " @ %why SPC "flares=" @ %n SPC "bomber=" @ isObject($V03::bomber) SPC "turret=" @ isObject(%t) @ %s SPC "simtime=" @ getSimTime());
}

echo("V03 SERVER-HELPERS-LOADED");
