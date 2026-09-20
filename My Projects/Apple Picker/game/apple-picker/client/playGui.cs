//-----------------------------------------------------------------------------
// PlayGui is the main TSControl through which the game is viewed.  The apple
// counter is a child control of it -- see client/ui/PlayGui.gui and
// client/applePicker.cs.
//
// The touch controls this file used to carry are gone.  GuiTouchLookCtrl and
// GuiTouchStickCtrl, and the $platform::touchScreen that decided whether to
// show them, were additions to the engine this game was written against; this
// tree has none of the three, and a .gui naming a class that does not exist
// fails the whole control.  Porting them is an engine change, not a script one,
// so they are left out rather than stubbed.
//-----------------------------------------------------------------------------

function PlayGui::onWake(%this)
{
   $enableDirectInput = "1";
   activateDirectInput();

   // Activate the game's action map.
   moveMap.push();
}

function PlayGui::onSleep(%this)
{
   // Pop the keymap.
   moveMap.pop();
}
