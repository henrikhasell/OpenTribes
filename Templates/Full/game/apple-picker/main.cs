//-----------------------------------------------------------------------------
// apple-picker -- a small complete game: walk over five apples on a flat plain
// and get a victory message on the fifth.
//
// Built from tutorial.base (the GettingStarted starting point -- its player, its
// mission, its art) with the boot plumbing taken from animation-test, which
// already goes straight into a mission instead of stopping at a main menu.
// Everything here is script; no engine change, no new art.  The apples are
// tutorial.base's own torque_logo.dts.
//-----------------------------------------------------------------------------

// Load up common script base.  This also pushes "common" onto the mod path.
loadDir("common");

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
// NOTE ON ORDERING: loadMods() in the root main.cs recurses, so mods are exec'd
// back-to-front.  With $userMods = "creator;apple-picker", our main.cs runs
// FIRST and creator/main.cs runs LAST -- so the Creator package is the OUTERMOST
// override.  So Creator::onStart calls Parent::onStart() (ours) before it execs
// creator/editor/editor.cs.  Consequence: toggleEditor() does not exist yet when
// initClient() runs -- anything there that wants the editor must schedule().
//
// Do NOT "fix" this by reversing the mod order.  That would move Creator's
// exec("./ui/InspectDlg.gui") ahead of our initCanvas(), so GuiDefaultProfile
// would not exist yet and ConsoleSetType(TypeGuiProfile) would NULL-deref.

// The mod directory is apple-picker, but a package name is a TorqueScript
// identifier -- no hyphens -- so the package is applePicker.
package applePicker {

// The root main.cs calls parseArgs() after the mods load and before onStart(),
// so anything set here is in place by the time initClient() runs.
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

   initServer();
   initClient();
}

function onExit()
{
   echo("Exporting prefs");
   export("$pref::*", "./prefs.cs", False);
   Parent::onExit();
}

}; // applePicker package

activatePackage(applePicker);
