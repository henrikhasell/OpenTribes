//-----------------------------------------------------------------------------
// Client-side mission loading hooks.
//
// common/client/missionDownload.cs drives the three-phase handshake and calls
// each of these by name.  All they do is drive the loading screen -- see
// client/loadingGui.cs for why the Canvas must be showing one at all.
//-----------------------------------------------------------------------------

function onMissionDownloadPhase1(%missionName, %musicTrack)
{
   // startMission() has normally raised it already; this covers a load that
   // began some other way (the editor's cold-boot branch, or a reconnect).
   loadingGuiOpen();
   loadingGuiProgress("LOADING DATABLOCKS", 0);
}

function onPhase1Progress(%progress)
{
   loadingGuiProgress("", %progress);
}

function onPhase1Complete() { }

function onMissionDownloadPhase2()
{
   loadingGuiProgress("LOADING OBJECTS", 0);
}

function onPhase2Progress(%progress)
{
   loadingGuiProgress("", %progress);
}

function onPhase2Complete() { }

// Phase 3 is scene lighting.  Note that SceneLighting::lightScene fires its
// completion callback even on the failure path (sceneLighting.cc:215), so this
// always runs and onClientEnterGame always follows.
function onMissionDownloadPhase3()
{
   loadingGuiProgress("LIGHTING MISSION", 0);
}

function onPhase3Progress(%progress)
{
   loadingGuiProgress("", %progress);
}

function onPhase3Complete()
{
   loadingGuiProgress("", 1);
   $lightingMission = false;
}

// The end of the load, but *not* the point at which it is safe to show the
// mission: the client still has no control object here.  Waiting for
// GameConnection::initialControlSet is the whole point -- see loadingGui.cs.
function onMissionDownloadComplete() { }


//-----------------------------------------------------------------------------
// The server sends the MissionInfo block over these three messages during
// loadMission(), before phase 1.

addMessageCallback('MsgLoadInfo', handleLoadInfoMessage);
addMessageCallback('MsgLoadDescripition', handleLoadDescriptionMessage);
addMessageCallback('MsgLoadInfoDone', handleLoadInfoDoneMessage);

function handleLoadInfoMessage(%msgType, %msgString, %mapName)
{
   echo("*** Mission: " @ %mapName);

   loadingGuiOpen();

   for (%i = 0; %i < LoadingGui.qLineCount; %i++)
      LoadingGui.qLine[%i] = "";
   LoadingGui.qLineCount = 0;

   LOAD_MapName.setText(%mapName);
}

function handleLoadDescriptionMessage(%msgType, %msgString, %line)
{
   LoadingGui.qLine[LoadingGui.qLineCount] = %line;
   LoadingGui.qLineCount++;

   %text = "<spush><font:Arial:16>";
   for (%i = 0; %i < LoadingGui.qLineCount; %i++)
      %text = %text @ LoadingGui.qLine[%i] @ " ";
   %text = %text @ "<spop>";

   LOAD_MapDescription.setText(%text);
}

function handleLoadInfoDoneMessage(%msgType, %msgString) { }
