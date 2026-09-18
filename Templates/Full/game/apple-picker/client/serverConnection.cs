//-----------------------------------------------------------------------------
// Client-side connection handling.
//-----------------------------------------------------------------------------

addMessageCallback('MsgConnectionError', handleConnectionErrorMessage);

function handleConnectionErrorMessage(%msgType, %msgString, %msgError)
{
   $ServerConnectionErrorMessage = %msgError;
}

function onServerMessage(%mesg) { }


//-----------------------------------------------------------------------------
// Start the game.  This is toggleEditor()'s cold-boot branch
// (creator/editor/editor.cs:92) with the editor left out: stand up a local
// server on the mission, then connect to it.  initialControlSet() below puts
// PlayGui on the Canvas once the mission has arrived and there is a camera to
// look through.
//
// Nothing here needs deferring -- createServer() comes from common/, which
// initServer() has already exec'd by the time initClient() runs.

function startMission(%mission)
{
   if (%mission $= "")
      %mission = $Game::DefaultMission;

   // Raise the loading screen *first*, before there is a server or a
   // connection, so no frame of the load is ever drawn by a 3D control looking
   // at a half-arrived mission.  It comes down in initialControlSet() below,
   // which is the first moment the mission is known to be complete.
   loadingGuiOpen();
   loadingGuiProgress("WAITING FOR SERVER", 0);
   Canvas.repaint();

   createServer("SinglePlayer", expandFilename(%mission));

   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);
   %conn.setConnectArgs($pref::Player::Name);
   %conn.setJoinPassword($Client::Password);
   %conn.connectLocal();
}


//-----------------------------------------------------------------------------

function GameConnection::initialControlSet(%this)
{
   echo("*** Initial Control Object");

   // This is the end of the loading screen: the mission has ghosted in and
   // there is a control object to look through, so a 3D control can safely
   // take the Canvas now.  Whichever branch below runs replaces LoadingGui.
   //
   // The editor gets first refusal: if it was opened from a cold boot it set
   // EditorGui.loadingMission before connecting, and checkActiveLoadDone()
   // then puts EditorGui up instead of PlayGui.  On a normal boot no editor
   // exists yet and PlayGui wins.
   if (!Editor::checkActiveLoadDone())
      if (Canvas.getContent() != PlayGui.getId())
         Canvas.setContent(PlayGui);
}

function GameConnection::setLagIcon(%this, %state) { }
function GameConnection::onConnectionAccepted(%this) { }

function GameConnection::onConnectionTimedOut(%this)
{
   disconnectedCleanup();
   MessageBoxOK("TIMED OUT", "The server connection has timed out.");
}

function GameConnection::onConnectionDropped(%this, %msg)
{
   disconnectedCleanup();
   MessageBoxOK("DISCONNECT", "The server dropped the connection: " @ %msg);
}

function GameConnection::onConnectionError(%this, %msg)
{
   disconnectedCleanup();
   MessageBoxOK("DISCONNECT", $ServerConnectionErrorMessage SPC "(" @ %msg @ ")");
}

function GameConnection::onConnectRequestRejected(%this, %msg)
{
   disconnectedCleanup();
   MessageBoxOK("REJECTED", %msg);
}

function GameConnection::onConnectRequestTimedOut(%this)
{
   disconnectedCleanup();
   MessageBoxOK("TIMED OUT", "Connection to the server timed out.");
}


//-----------------------------------------------------------------------------

function disconnect()
{
   if (isObject(ServerConnection))
      ServerConnection.delete();
   disconnectedCleanup();
   destroyServer();
}

function disconnectedCleanup()
{
   // Deliberately does NOT touch the Canvas.  toggleEditor() calls disconnect()
   // on a cold boot, before any GUI exists and with nothing to fall back to --
   // the stock kits' Canvas.setContent(MainMenuGui) here would error every run.
   alxStopAll();
   clearTextureHolds();
   purgeResources();
}
