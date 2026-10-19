// V03 review helpers: a bare Player far out of the client's scope, so its
// first ghost body reaches the client only when it is moved in.

function v03Far()
{
   %c = v00Client().player.getPosition();
   return VectorAdd(%c, "1800 1800 0");
}

function v03Dummy(%img, %ammo)
{
   %p = new Player() { dataBlock = LightMaleHumanArmor; };
   MissionCleanup.add(%p);
   %p.team = 2;
   %pos = v03Far();
   %p.setTransform(getWords(%pos, 0, 1) SPC (getTerrainHeight(getWords(%pos, 0, 1)) + 1) SPC "0 0 1 0");
   %p.setInventory(%ammo, 200);
   %p.mountImage(%img, 0);
   echo("V03B DUMMY " @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "ammo=" @ %p.getInventory(%ammo) SPC "xf=" @ %p.getTransform());
   return %p;
}

// Case A: an emptied slot whose count has moved, first seen by the client.
function v03A()
{
   $V03::A = v03Dummy(DiscImage, DiscAmmo);
   $V03::target = $V03::A;
   $V03::emptyOnFire = 1;
   $V03::A.setImageTrigger(0, true);
   schedule(400, 0, "v03AOff");
}

function v03AOff()
{
   $V03::A.setImageTrigger(0, false);
   $V03::emptyOnFire = 0;
   echo("V03B A-FIRED " @ $V03::A SPC "img0=" @ $V03::A.getMountedImage(0));
}

// Bring a dummy to 15 m in front of the client's player.
function v03Bring(%p)
{
   %c = v00Client().player.getPosition();
   %x = getWord(%c, 0) + 15;
   %y = getWord(%c, 1);
   %p.setTransform(%x SPC %y SPC (getTerrainHeight(%x SPC %y) + 1) SPC "0 0 1 0");
   echo("V03B BRING " @ %p SPC "xf=" @ %p.getTransform() SPC "img0=" @ %p.getMountedImage(0) SPC "state=" @ %p.getImageState(0));
}

function v03Remount(%p, %img)
{
   %p.mountImage(%img, 0);
   echo("V03B REMOUNT " @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "state=" @ %p.getImageState(0));
}

// Case B: a slot that is firing when the client first sees it.
function v03B()
{
   $V03::B = v03Dummy(ChaingunImage, ChaingunAmmo);
   $V03::B.setImageTrigger(0, true);
   echo("V03B B-TRIGGER " @ $V03::B SPC "state=" @ $V03::B.getImageState(0));
}

function v03State(%p)
{
   echo("V03B STATE " @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "state=" @ %p.getImageState(0) SPC "ammo=" @ %p.getInventory(ChaingunAmmo));
}

echo("V03B HELPERS-LOADED");

// A Player with no client drops its trigger every tick, so hold it again.
function v03Hold()
{
   if (!$V03::hold)
      return;
   $V03::B.setImageTrigger(0, true);
   schedule(16, 0, "v03Hold");
}
echo("V03B HOLD-LOADED");

// Case B again: the client's own player holds a chaingun trigger, so its first
// body after a re-ghost is sent mid-shot.
function v03Chain()
{
   %p = v00Client().player;
   %p.setInventory(Chaingun, 1);
   %p.setInventory(ChaingunAmmo, 150);
   %p.use(Chaingun);
   echo("V03B CHAIN " @ %p SPC "img0=" @ %p.getMountedImage(0) SPC "ammo=" @ %p.getInventory(ChaingunAmmo));
}

function v03Reghost()
{
   v03State(v00Client().player);
   v00Client().resetGhosting();
   schedule(500, 0, "v03Reghost2");
}

function v03Reghost2()
{
   v00Client().activateGhosting();
   echo("V03B REGHOST simtime=" @ getSimTime());
}
echo("V03B CHAIN-LOADED");
