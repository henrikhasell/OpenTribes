//-----------------------------------------------------------------------------
// The main menu: what the game opens on, and what it comes back to.
//
// Laid out after the Starter FPS kit's mainMenuGui -- a column of buttons that
// hand off to a screen each, rather than screens of its own.  Two of the three
// are already here: ChooseLevelDlg is the level list with the host-multiplayer
// box on it, and OptionsDlg is the video, audio and control settings, both
// exec'd by initializeCore() before any of this runs.  Only the server browser
// had to be brought in, as client/ui/JoinServerDlg.gui.
//
// Built out of colours, like the loading screen, so it has no art that can go
// missing.
//-----------------------------------------------------------------------------

new GuiControlProfile (MainMenuProfile)
{
   opaque = true;
   fillColor = "104 108 120";
};

new GuiControlProfile (MainMenuPanelProfile)
{
   opaque = true;
   fillColor = "200 200 200";
   border = true;
   borderColor = "0 0 0";
};

new GuiControlProfile (MainMenuTitleProfile)
{
   fontType = "Arial Bold";
   fontSize = 28;
   fontColor = "32 40 32";
   justify = "center";
};


//-----------------------------------------------------------------------------

// Put the menu up.  Called at the end of initClient(), and again by
// disconnectedCleanup() whenever a game ends or a connection is lost -- which
// is the whole reason the game needs one.
function mainMenuOpen()
{
   if (Canvas.getContent() != MainMenuGui.getId())
      Canvas.setContent(MainMenuGui);
}
