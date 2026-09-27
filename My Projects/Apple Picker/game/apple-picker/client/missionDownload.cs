//-----------------------------------------------------------------------------
// Client-side mission loading hooks.
//
// core/scripts/client/missionDownload.cs drives the three-phase handshake and
// calls each of these by name -- the same three phases, under the same names,
// as TGE's common/.  All they do is drive the loading screen; see
// client/loadingGui.cs for why the Canvas must be showing one at all.
//-----------------------------------------------------------------------------

function onMissionDownloadPhase1(%missionName, %musicTrack)
{
   // startMission() has normally raised it already; this covers a load that
   // began some other way.
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

// Phase 3 was scene lighting -- the lightmap bake TGE ran at the end of every
// load.  This engine has no SceneLighting, so core's clientCmdMissionStartPhase3
// runs the phase through in one step and this is over as soon as it starts.
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
// The server sends the level name and description over these three messages
// during loadMission(), before phase 1.  In TGE they came from the mission's
// MissionInfo ScriptObject; here core/scripts/server/levelInfo.cs sends them
// from the LevelInfo object in the .mis, so the names on the far end changed
// but the messages did not.
//
// The description arrives a line at a time and is reassembled here, which is
// why the lines are kept on the control rather than appended to it.

addMessageCallback('MsgLoadInfo', handleLoadInfoMessage);
addMessageCallback('MsgLoadDescripition', handleLoadDescriptionMessage);
addMessageCallback('MsgLoadInfoDone', handleLoadInfoDoneMessage);

function handleLoadInfoMessage(%msgType, %msgString, %mapName)
{
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

   LOAD_MapDescription.setText(%text @ "<spop>");
}

function handleLoadInfoDoneMessage(%msgType, %msgString) { }
