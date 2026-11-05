function clientCmdRelightMission()
{
   if(!lightScene("relightMissionComplete", "forceAlways"))
      error("Mission relight did not start.");
   else
      echo("Relighting mission...");
}

function relightMissionComplete()
{
   echo("Mission relit.");
}

// A retail client's Sun already has these from its ghost, and has no azimuth
// field. TODO: To match the retail client, the azimuth field should be removed from OpenTribes.
function clientCmdSetLighting(%color, %ambient)
{
   %sun = -1;
   %count = ServerConnection.getCount();
   for(%i = 0; %i < %count; %i++)
   {
      %obj = ServerConnection.getObject(%i);
      if(isObject(%obj) && %obj.getClassName() $= "Sun")
         %sun = %obj;
   }
   if(%sun == -1 || %sun.azimuth $= "")
      return;

   %sun.color = %color;
   %sun.ambient = %ambient;
   %sun.apply();
}
