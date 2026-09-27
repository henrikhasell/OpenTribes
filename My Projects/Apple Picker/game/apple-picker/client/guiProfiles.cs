//-----------------------------------------------------------------------------
// The window, button, entry field, slider and check box the game's dialogs are
// drawn from.
//
// All three carry the look this game has on the engine it came from rather than
// the Torque 3D set core/art/gui/profiles.cs points at.  Only the window is a
// bitmap, and the two sheets slice the same way -- images/window.png and
// ui/darkWindow.png are both three states of four title-bar buttons over the
// frame strips, at the same height -- so this is a change of profile and not of
// control.  The button and the field are drawn from colours and a border style.
//
// Restating a profile rather than declaring one: `singleton` hands back the
// object core already made and assigns over it, which keeps every .gui that
// names these pointed at the same object.  A plain `new` would be a
// re-declaration and would fail.  client/init.cs execs this before anything
// wakes a control on any of them, so what is named here is what gets read.
//
// The editors take the same five profiles, so their chrome follows.
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
// with a rule round it, where core's is skinned from images/button.
//
// The fills and the border colour are written out rather than left to whatever
// GuiDefaultProfile has, because the two defaults do not agree: the one this
// game is drawn against fills 192 192 192 behind a black rule, core's fills
// 242 241 240 behind a grey one, and a button that only says "border = true"
// comes out as core's near-white plate.
singleton GuiControlProfile (GuiButtonProfile)
{
   opaque = true;
   border = true;
   bitmap = "";
   hasBitmapArray = false;
   fillColor = "192 192 192";
   fillColorHL = "220 220 220";
   fillColorNA = "220 220 220";
   borderColor = "0 0 0";
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   justify = "center";
   canKeyFocus = false;
};

// An entry field is drawn rather than skinned too.  Border style 3 is the
// sunken rule -- dark down the top and left, light along the bottom and right
// (guiDefaultControlRender.cpp:119) -- and it takes its four colours from the
// profile, so those are written out here for the same reason the button's are.
singleton GuiControlProfile (GuiTextEditProfile)
{
   opaque = true;
   bitmap = "";
   hasBitmapArray = false;
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   borderColorNA = "64 64 64";
   bevelColorHL = "255 255 255";
   bevelColorLL = "0 0 0";
   fillColor = "255 255 255";

   // Not the 128 128 128 the field is drawn with elsewhere.  That engine spends
   // fillColorHL on the bar behind selected text and fills the control itself
   // with fillColor whichever control has the keyboard; this one fills the whole
   // control with fillColorHL as soon as it is the first responder
   // (guiTextEditCtrl.cpp:1261) and takes the selection bar from fontColorSEL
   // instead.  Carried across unchanged it would turn the focused field grey.
   fillColorHL = "255 255 255";

   fontColor = "0 0 0";
   fontColorHL = "255 255 255";
   fontColorNA = "128 128 128";
   textOffset = "0 2";
   autoSizeWidth = false;
   autoSizeHeight = true;
   tab = true;
   canKeyFocus = true;
};

// The slider has no art over there at all -- its profile is declared and left
// empty, and the control draws a rule, its tick marks and a raised thumb.  This
// engine does the same whenever the profile names no bitmap
// (guiSliderCtrl.cpp:432), so clearing core's images/slider is the whole of it.
// The two colours are the thumb's, through renderRaisedBox.
singleton GuiControlProfile (GuiSliderProfile)
{
   bitmap = "";
   hasBitmapArray = false;
   fillColor = "192 192 192";
   borderColor = "0 0 0";
};

// The check box.  Four cells where core's sheet has six -- unchecked, checked,
// and the two again under the mouse, with no pair for an inactive box -- which
// GuiCheckBoxCtrl calls the old style and handles by drawing index 2 for one
// (guiCheckBoxCtrl.cpp:120).  A checked box over there is a filled square
// rather than core's tick.
singleton GuiControlProfile (GuiCheckBoxProfile)
{
   opaque = false;
   fillColor = "232 232 232";
   border = false;
   borderColor = "0 0 0";
   fontSize = 14;
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fixedExtent = true;
   justify = "left";
   bitmap = "./ui/torqueCheck";
   hasBitmapArray = true;
};
