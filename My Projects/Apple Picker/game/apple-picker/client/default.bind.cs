//-----------------------------------------------------------------------------
// Key bindings.
//
// tutorial.base's set, trimmed: no mouseTrigger (nothing here shoots) and no
// cycleDebugRenderMode.  moveMap is pushed by PlayGui::onWake, so this file is
// what makes it take input at all.
//-----------------------------------------------------------------------------

if (isObject(moveMap))
   moveMap.delete();
new ActionMap(moveMap);


//-----------------------------------------------------------------------------
// Leaving the game.
//
// There is somewhere to disconnect to now, so leaving the orchard is no longer
// leaving the game: disconnect() ends up in disconnectedCleanup(), which opens
// the menu.  Quit is on the menu.

function escapeFromGame()
{
   MessageBoxYesNo("Leave", "Leave the orchard?", "disconnect();", "");
}

moveMap.bindCmd(keyboard, "escape", "", "escapeFromGame();");


//-----------------------------------------------------------------------------
// Movement.

$movementSpeed = 1; // m/s

function setSpeed(%speed)
{
   if (%speed)
      $movementSpeed = %speed;
}

function moveleft(%val)     { $mvLeftAction     = %val * $movementSpeed; }
function moveright(%val)    { $mvRightAction    = %val * $movementSpeed; }
function moveforward(%val)  { $mvForwardAction  = %val * $movementSpeed; }
function movebackward(%val) { $mvBackwardAction = %val * $movementSpeed; }
function moveup(%val)       { $mvUpAction       = %val * $movementSpeed; }
function movedown(%val)     { $mvDownAction     = %val * $movementSpeed; }

function turnLeft(%val)  { $mvYawRightSpeed  = %val ? $Pref::Input::KeyboardTurnSpeed : 0; }
function turnRight(%val) { $mvYawLeftSpeed   = %val ? $Pref::Input::KeyboardTurnSpeed : 0; }
function panUp(%val)     { $mvPitchDownSpeed = %val ? $Pref::Input::KeyboardTurnSpeed : 0; }
function panDown(%val)   { $mvPitchUpSpeed   = %val ? $Pref::Input::KeyboardTurnSpeed : 0; }

function getMouseAdjustAmount(%val)
{
   // Based on a default camera fov of 90 degrees.
   return (%val * ($cameraFov / 90) * 0.01);
}

function yaw(%val)   { $mvYaw   += getMouseAdjustAmount(%val); }
function pitch(%val) { $mvPitch += getMouseAdjustAmount(%val); }
function jump(%val)  { $mvTriggerCount2++; }

moveMap.bind(keyboard, a, moveleft);
moveMap.bind(keyboard, d, moveright);
moveMap.bind(keyboard, w, moveforward);
moveMap.bind(keyboard, s, movebackward);
moveMap.bind(keyboard, space, jump);

moveMap.bind(keyboard, left,  turnLeft);
moveMap.bind(keyboard, right, turnRight);
moveMap.bind(keyboard, up,    panUp);
moveMap.bind(keyboard, down,  panDown);

moveMap.bind(mouse, xaxis, yaw);
moveMap.bind(mouse, yaxis, pitch);


//-----------------------------------------------------------------------------
// Camera and view.

function toggleFreeLook(%val)
{
   $mvFreeLook = %val ? true : false;
}

$firstPerson = true;
function toggleFirstPerson(%val)
{
   if (%val)
   {
      $firstPerson = !$firstPerson;
      ServerConnection.setFirstPerson($firstPerson);
   }
}

function toggleCamera(%val)
{
   if (%val)
      commandToServer('ToggleCamera');
}

moveMap.bind(keyboard, z, toggleFreeLook);
moveMap.bind(keyboard, tab, toggleFirstPerson);
moveMap.bind(keyboard, "alt c", toggleCamera);


//-----------------------------------------------------------------------------
// Helpers used by the Mission Editor.  Both are serverCmds in server/editor.cs.

function dropCameraAtPlayer(%val)
{
   if (%val)
      commandToServer('DropCameraAtPlayer');
}

function dropPlayerAtCamera(%val)
{
   if (%val)
      commandToServer('DropPlayerAtCamera');
}

moveMap.bind(keyboard, "F8", dropCameraAtPlayer);
moveMap.bind(keyboard, "F7", dropPlayerAtCamera);


//-----------------------------------------------------------------------------
// The global map, available everywhere.
//
// Both of these are already bound by initializeCore(), which runs before this
// file, and both are rebound here so that this file remains the one place the
// game's keys are written down.  TGE's toggleFullScreen() console function is
// gone; the Canvas grew a method for it instead.

GlobalActionMap.bind(keyboard, "tilde", toggleConsole);
GlobalActionMap.bindCmd(keyboard, "alt enter", "Canvas.attemptFullscreenToggle();", "");
