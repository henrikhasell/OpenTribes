//-----------------------------------------------------------------------------
// Client initialization.
//-----------------------------------------------------------------------------

function initClient()
{
   echo("\n--------- Initializing MOD: apple-picker: Client ---------");
   $Server::Dedicated = false;

   // The common module provides basic client functionality.
   initBaseClient();

   // initCanvas() starts the graphics system AND is the only thing that execs
   // common/ui/defaultProfiles.cs, which defines GuiDefaultProfile.  Every
   // GuiControl falls back to that profile when it wakes, so this must run
   // before any .gui file is exec'd anywhere.
   //
   // It is also what execs common/client/messageBox.cs (via
   // common/client/canvas.cs), which is where the victory box's MessageBoxOK()
   // comes from -- see client/applePicker.cs.  Every mod gets that for free.
   initCanvas("apple-picker");

   // PlayGui is what the game runs in, and also what you land back on when you
   // F11 out of the editor -- toggleEditor()'s else-branch does
   // Editor.close("PlayGui").
   //
   // applePicker.cs first: it defines the two profiles PlayGui.gui names for the
   // apple counter, and a .gui naming a profile that does not exist yet falls
   // back to GuiDefaultProfile without saying so.
   exec("./applePicker.cs");
   exec("./ui/PlayGui.gui");
   exec("./playGui.cs");

   // The loading screen, which owns the Canvas for the whole of every mission
   // load.  Same ordering rule: the .cs defines the two profiles the .gui asks
   // for, so it goes first.
   exec("./loadingGui.cs");
   exec("./ui/LoadingGui.gui");

   // Client scripts.
   exec("./missionDownload.cs");
   exec("./serverConnection.cs");

   // Defines moveMap, which both PlayGui::onWake and EditorGui::onWake push.
   // Without it neither takes any keyboard or mouse input at all.
   exec("./default.bind.cs");

   // Copy saved script prefs into C++ code.
   setShadowDetailLevel($pref::shadows);
   setDefaultFov($pref::Player::defaultFov);
   setZoomSpeed($pref::Player::zoomSpeed);

   Canvas.setCursor("DefaultCursor");

   initTelnetConsole();

   //--------------------------------------------------------------------------
   // Boot straight into the game.  startMission() puts LoadingGui on the Canvas
   // before it creates the server, so the load is covered by a 2D control from
   // its first frame; PlayGui only appears once the mission has finished
   // arriving.  The Mission Editor is NOT opened -- press F11 for that, which is
   // toggleEditor()'s warm branch since a mission is running.

   // tutorial.base's flat.mis, copied into this mod rather than read from there:
   // a mod is only on the resource path if it is in $userMods, and putting it
   // there would exec its main.cs too -- its packages, its own initClient, its
   // main menu.  See data/missions/orchard.mis for what the copy changed.
   $Game::DefaultMission = "apple-picker/data/missions/orchard.mis";

   // Only read if the editor is ever entered with no mission running, in which
   // case creator/editor/editor.cs:100 loads this instead of its own
   // "~/data/newMission.mis" template.
   $Editor::newMissionOverride = $Game::DefaultMission;

   // Consumed by our patches to the copied creator scripts.
   $Editor::defaultCloseGui = "PlayGui";
   $Editor::quitExitsGame = true;

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
