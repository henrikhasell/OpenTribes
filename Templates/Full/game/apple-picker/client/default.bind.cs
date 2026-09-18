//-----------------------------------------------------------------------------
// Key bindings.
//
// tutorial.base's set, trimmed: no mouseTrigger (nothing here shoots) and no
// cycleDebugRenderMode.  moveMap is pushed by PlayGui::onWake and also by
// EditorGui::onWake, so this file is what makes both take input at all.
//-----------------------------------------------------------------------------

if (isObject(moveMap))
   moveMap.delete();
new ActionMap(moveMap);


//-----------------------------------------------------------------------------
// Leaving the game.
//
// This mod boots straight into the mission and has no main menu, so there is
// nothing to disconnect *to*: stock's bare disconnect() would leave the Canvas
// showing a dead PlayGui with no mission behind it.  Leaving the orchard is
// leaving the game.

function escapeFromGame()
{
   MessageBoxYesNo("Quit", "Leave the orchard?", "disconnect(); quit();", "");
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
// The global map, available everywhere -- editor included.
//
// toggleFullScreen is a console function; do NOT wrap it in a script function
// of the same name, which would only shadow it (GuiCanvas has no such method in
// this SDK).

GlobalActionMap.bind(keyboard, "tilde", toggleConsole);
GlobalActionMap.bindCmd(keyboard, "alt enter", "", "toggleFullScreen();");
