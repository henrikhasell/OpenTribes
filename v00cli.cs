// V00 client helpers: exec'd on the retail client, never in the repo.
// Recording goes through the same calls beginDemoRecord makes, with a chosen
// file name instead of the next free demoNNN.

function v00Rec(%name)
{
   if (isRecordingDemo())
      stopRecord();
   $DemoFile = "recordings/" @ %name @ ".rec";
   saveDemoSettings();
   startRecord($DemoFile);
   echo("V00C REC-START " @ $DemoFile SPC "recording=" @ isRecordingDemo() SPC "simtime=" @ getSimTime());
}

function v00Stop()
{
   %f = $DemoFile;
   if (isRecordingDemo())
      stopRecord();
   echo("V00C REC-STOP " @ %f SPC "recording=" @ isRecordingDemo() SPC "simtime=" @ getSimTime());
}

function v00Idle()
{
   $mvForwardAction = 0;
   $mvBackwardAction = 0;
   $mvLeftAction = 0;
   $mvRightAction = 0;
   $mvUpAction = 0;
   $mvYawLeftSpeed = 0;
   $mvYawRightSpeed = 0;
   $mvPitchUpSpeed = 0;
   $mvPitchDownSpeed = 0;
   if ($mvTriggerCount0 & 1) $mvTriggerCount0++;
   if ($mvTriggerCount3 & 1) $mvTriggerCount3++;
   echo("V00C IDLE simtime=" @ getSimTime());
}

// Held inputs: forward throttle, yaw right (negative is left), pitch up
// (negative is down), and the jet trigger.
function v00Drive(%fwd, %yaw, %pitch, %jet)
{
   $mvForwardAction = %fwd;
   $mvYawRightSpeed = %yaw > 0 ? %yaw : 0;
   $mvYawLeftSpeed = %yaw < 0 ? -%yaw : 0;
   $mvPitchUpSpeed = %pitch > 0 ? %pitch : 0;
   $mvPitchDownSpeed = %pitch < 0 ? -%pitch : 0;
   if (($mvTriggerCount3 & 1) != (%jet ? 1 : 0))
      $mvTriggerCount3++;
   echo("V00C DRIVE fwd=" @ %fwd SPC "yaw=" @ %yaw SPC "pitch=" @ %pitch SPC "jet=" @ %jet SPC "simtime=" @ getSimTime());
}

function v00Fire(%down)
{
   mouseFire(%down);
   echo("V00C FIRE " @ %down SPC "simtime=" @ getSimTime());
}

function v00Jump()
{
   jump(1);
   schedule(150, 0, jump, 0);
   echo("V00C JUMP simtime=" @ getSimTime());
}

function v00Weapon(%n)
{
   commandToServer('setVehicleWeapon', %n);
   echo("V00C WEAPON " @ %n SPC "simtime=" @ getSimTime());
}

function v00ProbeC(%why)
{
   // No ?: here: retail's console evaluated the ternary to the object id.
   %co = ServerConnection.getControlObject();
   %cls = "-";
   %db = "-";
   %xf = "-";
   if (isObject(%co))
   {
      %cls = %co.getClassName();
      %db = %co.getDataBlock().getName();
      %xf = %co.getTransform();
   }
   echo("V00C PROBE " @ %why SPC "ctrl=" @ %co SPC %cls SPC %db SPC "xf=" @ %xf SPC
        "firstPerson=" @ $firstPerson SPC "recording=" @ isRecordingDemo() SPC "simtime=" @ getSimTime());
}

echo("V00C CLIENT-HELPERS-LOADED");
