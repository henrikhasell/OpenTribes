//-----------------------------------------------------------------------------
// PlayGui is the main TSControl through which the game is viewed.  The apple
// counter is a child control of it -- see client/ui/PlayGui.gui and
// client/applePicker.cs.
//-----------------------------------------------------------------------------

function PlayGui::onWake(%this)
{
   $enableDirectInput = "1";
   activateDirectInput();

   // Activate the game's action map.
   moveMap.push();

   // The on-screen controls exist in PlayGui.gui but stay hidden unless there
   // is a touchscreen to need them.  $platform::touchScreen is set from SDL at
   // startup, so a desktop with one gets them too.
   TouchLook.setVisible($platform::touchScreen);
}

//-----------------------------------------------------------------------------
// Touch controls.
//
// The engine reports these as callbacks rather than driving the movement
// variables itself, so the mapping lives here with the keyboard bindings it
// mirrors -- see client/default.bind.cs for moveforward(), yaw() and jump().
//-----------------------------------------------------------------------------

/// Thumbstick, -1..1 on each axis with forward positive.
function TouchStick::onStickMove(%this, %x, %y)
{
   // Straight to the same move variables the keyboard bindings set.  They are
   // analog already, so a half-pushed stick walks at half speed; the keyboard
   // only ever passes 0 or 1.
   $mvForwardAction  = (%y > 0) ? %y * $movementSpeed : 0;
   $mvBackwardAction = (%y < 0) ? -%y * $movementSpeed : 0;
   $mvRightAction    = (%x > 0) ? %x * $movementSpeed : 0;
   $mvLeftAction     = (%x < 0) ? -%x * $movementSpeed : 0;
}

function TouchStick::onStickUp(%this)
{
   $mvForwardAction = 0;
   $mvBackwardAction = 0;
   $mvRightAction = 0;
   $mvLeftAction = 0;
}

/// Look drag, in pixels.  yaw() and pitch() apply the FOV scaling that the
/// mouse bindings get, so touch and mouse turn at the same rate.
function TouchLook::onLook(%this, %dx, %dy)
{
   yaw(%dx);
   pitch(%dy);
}

/// A quick tap that did not drag.
function TouchLook::onTap(%this)
{
   jump(1);
   jump(0);
}


function PlayGui::onSleep(%this)
{
   // Pop the keymap.
   moveMap.pop();
}
