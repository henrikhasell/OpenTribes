// The stage 5 timeline, the same on both servers.
$T5::base = "165 -122 101.3";
$T5::near = "135 -122 125";
$T5::far = "-200 600 200";

function t5Run()
{
   t5Turrets();
   t5DeployRot();
   %d = new Turret() { dataBlock = TurretDeployedOutdoor; };
   MissionCleanup.add(%d);
   %d.setTransform("300 300 150 0 0 1 0");
   t5Log("OUTDOOR " @ %d SPC %d.getMountedImage(0).getName() SPC %d.initialBarrel);
   t5Deploy(TurretBaseLarge, PlasmaBarrelLarge, $T5::base, 1);
   schedule(1000, 0, t5Watch, 440);
   schedule(1500, 0, t5Enemy, $T5::near, 2);
   schedule(1800, 0, t5Probe);
   // Open and aimed: swap to a seeking barrel, which closes first.
   schedule(6000, 0, t5Swap, AABarrelPackImage, AABarrelPack, "167 -122 102");
   // Closed: swap back at once.
   schedule(12000, 0, t5Swap, PlasmaBarrelPackImage, PlasmaBarrelPack, "167 -120 102");
   schedule(18000, 0, t5Away);
   schedule(23000, 0, t5Skill, 0.3);
   schedule(23500, 0, t5Back);
   schedule(30000, 0, t5Away);
   schedule(36000, 0, t5Methods);
   schedule(38000, 0, t5Turrets);
   schedule(39000, 0, t5Log, "DONE");
}

function t5Away() { t5Log("AWAY"); t5Hold($T5::enemy, $T5::far); }
function t5Back() { t5Log("BACK"); t5Hold($T5::enemy, $T5::near); }
function t5Skill(%s) { t5Log("SKILL " @ %s); $T5::turret.setSkill(%s); }
