//-----------------------------------------------------------------------------
// Server initialization.
//-----------------------------------------------------------------------------

function initServer()
{
   echo("\n--------- Initializing MOD: apple-picker: Server ---------");

   $Server::MissionFileSpec = "apple-picker/data/missions/*.mis";

   // The core module provides the basic server functionality.
   initBaseServer();

   // Must come after initBaseServer(): it overrides core/scripts/server/game.cs.
   //
   // Only function definitions are exec'd here.  The datablock-bearing scripts
   // (camera.cs, player.cs, editor.cs) are exec'd from onServerCreated()
   // instead, because destroyServer() calls deleteDataBlocks() -- anything
   // registered at init time is wiped by the first createServer().
   exec("./game.cs");

   // Last, and it has to be: apple.cs overrides onServerCreated,
   // onMissionLoaded and GameConnection::onClientEnterGame through a package,
   // and activatePackage swaps entries with whatever the global namespace holds
   // at the moment it runs.  Exec'd before game.cs, the definitions game.cs then
   // makes would land on top of the swap and the overrides would never fire --
   // silently, with no error anywhere.  That is the whole game not happening.
   exec("./apple.cs");
}
