// V03 review client helper: list the player ghosts.
function v03Players()
{
   %n = 0;
   for (%i = 0; %i < ServerConnection.getCount(); %i++)
   {
      %o = ServerConnection.getObject(%i);
      if (%o.getClassName() $= "Player")
      {
         echo("V03B P " @ %o SPC %o.getPosition() SPC "img0=" @ %o.getMountedImage(0) SPC "st=" @ %o.getImageState(0));
         %n++;
      }
   }
   echo("V03B players=" @ %n SPC "ghosts=" @ ServerConnection.getCount());
}
echo("V03B CLIENT-HELPERS-LOADED");
