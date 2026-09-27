//-----------------------------------------------------------------------------
// The window and the button the game's dialogs are drawn from.
//
// Both carry the art this game has on the engine it came from rather than the
// Torque 3D set core/art/gui/profiles.cs points at.  The two window sheets
// slice the same way -- images/window.png and ui/darkWindow.png are both three
// states of four title-bar buttons over the frame strips, at the same height --
// so this is a change of profile and not of control.
//
// Restating a profile rather than declaring one: `singleton` hands back the
// object core already made and assigns over it, which keeps every .gui that
// names these pointed at the same object.  A plain `new` would be a
// re-declaration and would fail.  client/init.cs execs this before anything
// wakes a control on either, so the art named here is the art that gets read.
//
// The editors take the same two profiles, so their windows and buttons follow.
//-----------------------------------------------------------------------------

singleton GuiControlProfile (GuiWindowProfile)
{
   opaque = true;
   border = 2;
   fillColor = "192 192 192";
   fillColorHL = "64 150 150";
   fillColorNA = "150 150 150";
   fontColor = "255 255 255";
   fontColorHL = "0 0 0";
   bitmap = "./ui/darkWindow";
   textOffset = "6 6";
   hasBitmapArray = true;
   justify = "left";
};

// No bitmap at all: the button this game is drawn around is a filled rectangle
// with a border, where core's is skinned from images/button.
singleton GuiControlProfile (GuiButtonProfile)
{
   opaque = true;
   border = true;
   bitmap = "";
   hasBitmapArray = false;
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   justify = "center";
   canKeyFocus = false;
};
