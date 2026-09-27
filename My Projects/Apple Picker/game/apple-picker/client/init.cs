//-----------------------------------------------------------------------------
// Client initialization.
//-----------------------------------------------------------------------------

function initClient()
{
   echo("\n--------- Initializing MOD: apple-picker: Client ---------");
   $Server::Dedicated = false;

   // The core module provides basic client functionality.  It also brings in
   // the three-phase mission download whose onMissionDownloadPhaseN callbacks
   // client/missionDownload.cs fills in.
   initBaseClient();

   // TGE's initCanvas() is gone.  Its two jobs -- starting the graphics system
   // and exec'ing the default GUI profiles -- are done by initializeCore(),
   // which CorePackage::onStart has already run by the time we get here (see
   // the ordering note in main.cs).  So the Canvas, GuiDefaultProfile and
   // MessageBoxOK all exist already and there is nothing to do for them.

   // The window and button art, restated over core's before anything wakes a
   // control on them.
   exec("./guiProfiles.cs");

   // PlayGui is what the game runs in.
   //
   // applePicker.cs first: it defines the two profiles PlayGui.gui names for the
   // apple counter, and a .gui naming a profile that does not exist yet falls
   // back to GuiDefaultProfile without saying so.
   exec("./applePicker.cs");
   exec("./ui/PlayGui.gui");
   exec("./playGui.cs");

   // The loading screen.  loadingGui.cs first: it declares the profiles the
   // .gui names, and takes core's LoadingGui out of the way so this one can
   // have the name.
   exec("./loadingGui.cs");
   exec("./ui/LoadingGui.gui");

   // Client scripts.
   exec("./missionDownload.cs");
   exec("./serverConnection.cs");

   // Defines moveMap, which PlayGui::onWake pushes.  Without it PlayGui takes
   // no keyboard or mouse input at all.
   exec("./default.bind.cs");

   // Copy saved script prefs into C++ code.  TGE's setShadowDetailLevel() is
   // gone -- shadow quality in this engine is per-light-manager, and the Basic
   // Lighting pref for it is $pref::Shadows::textureScalar.
   setDefaultFov($pref::Player::defaultFov);
   setZoomSpeed($pref::Player::zoomSpeed);

   Canvas.setCursor("DefaultCursor");

   initTelnetConsole();

   //--------------------------------------------------------------------------
   // Boot straight into the game.  startMission() puts LoadingGui on the Canvas
   // before it creates the server, so the load is covered by a 2D control from
   // its first frame; PlayGui only appears once the mission has finished
   // arriving.
   $Game::DefaultMission = "apple-picker/data/missions/orchard.mis";

   // -worldEditor and -guiEditor set their own Canvas content once the root
   // main.cs returns from onStart(), and the tools package initializes after
   // this function does.  Starting the mission here would race both.
   if (!$startWorldEditor && !$startGUIEditor)
      startMission();
}


//-----------------------------------------------------------------------------
// Developer telnet console.  See the notes in client/defaults.cs.

function initTelnetConsole()
{
   if ($pref::Telnet::Port <= 0)
      return;

   telnetSetParameters($pref::Telnet::Port,
                       $pref::Telnet::ConsolePass,
                       $pref::Telnet::ListenPass);

   // probe.py waits for this line in console.log before it tries to connect.
   echo("Telnet console listening on port " @ $pref::Telnet::Port);
}
