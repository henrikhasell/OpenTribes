// Does a turret of full skill fire at a still target, and when?
function t5Fire()
{
   t5Deploy(TurretBaseLarge, PlasmaBarrelLarge, $T5::base, 1);
   schedule(500, 0, t5Watch, 300);
   schedule(1000, 0, t5Enemy, $T5::near, 2);
   schedule(12000, 0, t5Skill, 0.99);
   schedule(18000, 0, t5Skill, 1);
   schedule(25000, 0, t5Log, "DONE");
}
