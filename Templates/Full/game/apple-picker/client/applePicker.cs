//-----------------------------------------------------------------------------
// Client half of the game: the "Apples: n / 5" counter in PlayGui, and the
// victory box.  The rules live in server/apple.cs, which drives both of these
// with commandToClient.
//
// State lives under $ApplePickerHud::, kept deliberately separate from the
// server's $ApplePicker::.  In single player both halves share one console, and
// a HUD that read the globals the server also writes would only look correct by
// accident -- the point of the counter is that it is what the client was *told*.
//-----------------------------------------------------------------------------

// Profiles first: both must exist before client/ui/PlayGui.gui is exec'd.  A
// .gui naming a profile that does not exist yet falls back to
// GuiDefaultProfile without saying so (and asserts on the font in a debug
// build), so this file is exec'd ahead of it in client/init.cs.

// A plate behind the counter.  Every text profile in
// common/ui/defaultProfiles.cs draws black, which is invisible against a bright
// sky, so the HUD brings its own colours.
if (!isObject(ApplePickerHudProfile))
   new GuiControlProfile (ApplePickerHudProfile)
   {
      opaque = true;
      fillColor = "32 40 32";
      border = true;
      borderColor = "220 220 220";
   };

// Deliberately NOT ": GuiTextProfile" -- that one sets autoSizeWidth and
// autoSizeHeight, and a label that resizes itself to its text drifts out of a
// fixed plate as the digits change.  GuiDefaultProfile, which this falls back
// to, has both off.
if (!isObject(ApplePickerTextProfile))
   new GuiControlProfile (ApplePickerTextProfile)
   {
      fontType = "Arial Bold";
      fontSize = 24;
      fontColor = "240 240 160";
      justify = "center";
   };


//-----------------------------------------------------------------------------
// Server hand-off.  Sent once when this client enters the game, and again after
// every pickup.

function clientCmdAppleCount(%picked, %total)
{
   $ApplePickerHud::picked = %picked;
   $ApplePickerHud::total  = %total;

   // A fresh mission: drop any victory left over from the last one.
   if (%picked == 0)
      $ApplePickerHud::victory = false;

   AppleCountText.setText("Apples: " @ %picked @ " / " @ %total);
}

function clientCmdAppleVictory(%total)
{
   $ApplePickerHud::victory = true;

   // MessageBoxOK comes from common/client/messageBox.cs, which initCanvas()
   // execs for free (common/client/canvas.cs) -- every mod gets it, no exec of
   // ours and no art of ours.
   //
   // MessageBoxOKDlg carries no noCursor, so common/client/cursor.cs's
   // CanvasCursor package turns the cursor on as the dialog goes up and off
   // again when it comes down -- which is also what releases the mouse from
   // camera look and hands it back afterwards.  Nothing to do here for that.
   MessageBoxOK("Victory!",
                "You picked up all " @ %total @ " apples.",
                "");
}
