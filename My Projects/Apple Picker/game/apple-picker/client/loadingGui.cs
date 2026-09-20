//-----------------------------------------------------------------------------
// The loading screen.
//
// This is what covers the Canvas for the whole of a mission load.  It matters
// for more than looks: until the mission has finished ghosting in there is no
// complete TerrainBlock to draw, and anything that puts a 3D control up before
// then can be asked to render a terrain that is still arriving.  A plain 2D GUI
// cannot, so the rule this file exists to enforce is: something 2D owns the
// Canvas from the moment a load starts until GameConnection::initialControlSet
// says the mission is there.
//
// TGE's common/ shipped no loading screen, so this game brought its own
// LoadingGui built out of nothing but common's profiles.  This engine's core
// does ship one -- core/art/gui/loadingGui.gui, a singleton also called
// LoadingGui -- and initializeCore() has already exec'd it before any of our
// scripts run.  Defining a second object under the same name would leave two
// of them in the Sim dictionary with only one reachable by name, so the .gui
// and its two profiles are gone and this file is now just the two helpers that
// drive core's controls.
//
// Core's version does use art (art/gui/background and the Torque logo), which
// the self-contained original deliberately avoided.  Both are in this
// template's art directory, so it has nothing to go missing either.
//-----------------------------------------------------------------------------

// Put the loading screen up, if it is not already.  Called by startMission()
// before the server is even created, and again from phase 1 for the case where
// a load begins without going through startMission().

function loadingGuiOpen()
{
   if (Canvas.getContent() != LoadingGui.getId())
      Canvas.setContent(LoadingGui);
}

// The load runs inside the frame that started it, so the progress bar only
// moves if we push a frame out ourselves.
function loadingGuiProgress(%text, %progress)
{
   if (%text !$= "")
      LoadingProgressTxt.setValue(%text);
   LoadingProgress.setValue(%progress);
   Canvas.repaint();
}
