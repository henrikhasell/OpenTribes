//-----------------------------------------------------------------------------
// apple-picker -- a small complete game: walk over five apples on a flat plain
// and get a victory message on the fifth.
//
// Originally written against a Torque Game Engine fork; ported to this
// Torque 3D 1.2 tree.  Run it with
//
//    ./OpenTribes -game apple-picker
//
// from Templates/Full/game.  -game replaces the directory list outright
// (core/parseArgs.cs), so the stock "scripts" mod does not also boot and try to
// stand up its own client, server and main menu on top of this one.
//-----------------------------------------------------------------------------

// Load up the core script base.  This also pushes "core" onto the mod path.
// (TGE called this directory "common".)
loadDir("core");

// Default console values, then preferences (which override them).
exec("./client/defaults.cs");
exec("./server/defaults.cs");

// prefs.cs is written by onExit() and is .gitignore'd, so it is absent on a
// fresh checkout.  Guard the exec to keep the first boot free of console noise.
if (isFile("./prefs.cs"))
   exec("./prefs.cs");


//-----------------------------------------------------------------------------
// Package overrides to initialize the game.
//
// NOTE ON ORDERING: loadDirs() in the root main.cs recurses, so directories are
// exec'd back-to-front and the FIRST entry in $userDirs is exec'd LAST.  With
// "apple-picker" alone in the list, core/main.cs is exec'd by the loadDir()
// above and CorePackage is therefore activated before applePicker -- so
// applePicker is the outermost override and applePicker::onStart runs first,
// reaching CorePackage::onStart through Parent::onStart().
//
// That ordering is what gets the Canvas: CorePackage::onStart calls
// initializeCore(), which is this engine's equivalent of TGE's initCanvas() --
// it creates the Canvas, execs core/art/gui/profiles.cs (so GuiDefaultProfile
// exists before any .gui of ours is exec'd), and brings up messageBox.cs,
// cursor.cs and the core materials.  Everything in our onStart() below
// therefore runs with a live Canvas already in place.

// The mod directory is apple-picker, but a package name is a TorqueScript
// identifier -- no hyphens -- so the package is applePicker.
package applePicker {

// The root main.cs calls parseArgs() after the directories load and before
// onStart(), so anything set here is in place by the time initClient() runs.
function parseArgs()
{
   Parent::parseArgs();

   for (%i = 1; %i < $Game::argc; %i++)
   {
      if ($Game::argv[%i] $= "-telnet")
      {
         $argUsed[%i]++;
         if ($Game::argc - %i > 1)
         {
            $pref::Telnet::Port = $Game::argv[%i + 1];
            $argUsed[%i + 1]++;
            %i++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -telnet <port>");
      }
   }
}

function displayHelp()
{
   Parent::displayHelp();
   error("  -telnet <port>         Open the developer console on a TCP port\n");
}

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: apple-picker ---------");

   exec("./client/init.cs");
   exec("./server/init.cs");
   exec("./data/init.cs");

   // Not in TGE: the physics and sound subsystems are started from script in
   // this engine, and initBaseServer()'s audio.cs needs the latter up.
   physicsInit();
   sfxStartup();

   initServer();
   initClient();
}

function onExit()
{
   // Not in TGE, where the engine tore the server down itself.  Without this
   // the SceneGraph is deleted before the objects in it and the shutdown
   // crashes.
   disconnect();
   physicsDestroy();

   echo("Exporting prefs");
   export("$pref::*", "./prefs.cs", False);
   Parent::onExit();
}

}; // applePicker package

activatePackage(applePicker);
