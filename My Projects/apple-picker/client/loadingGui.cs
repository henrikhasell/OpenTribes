//-----------------------------------------------------------------------------
// The loading screen.
//
// This is what covers the Canvas for the whole of a mission load.  It matters
// for more than looks: until the mission has finished ghosting in there is no
// complete TerrainBlock to draw, and anything that puts a 3D control up before
// then -- PlayGui's TSControl, or the editor's EditTSCtrl -- can be asked to
// render a TerrainBlock that is still arriving.  TerrainRender::renderBlock
// segfaults on one of those.  A plain 2D GUI cannot, so the rule this file
// exists to enforce is: something 2D owns the Canvas from the moment a load
// starts until GameConnection::initialControlSet says the mission is there.
//
// Deliberately built out of nothing but common/'s profiles and no bitmaps at
// all, so it has no content to go missing.
//-----------------------------------------------------------------------------

// The backdrop.  Opaque is the point -- the Canvas has no content of its own
// before the first setContent, so anything translucent here would leave
// whatever the GL buffers happened to contain showing through.
new GuiControlProfile (LoadingGuiProfile)
{
   opaque = true;
   fillColor = "104 108 120";
};

// The panel in the middle of it.  Light, because every text profile in
// common/ui/defaultProfiles.cs draws black.
new GuiControlProfile (LoadingGuiPanelProfile)
{
   opaque = true;
   fillColor = "200 200 200";
   border = true;
   borderColor = "0 0 0";
};


//-----------------------------------------------------------------------------

function LoadingGui::onAdd(%this)
{
   %this.qLineCount = 0;
}

function LoadingGui::onWake(%this)
{
   // The overlay is a dialog, so it sits above the content control rather than
   // being replaced by it.  PlayGui::onSleep pops it on the way out of the
   // game, but a load can also start with PlayGui already up (a second
   // startMission), and then nothing has popped it yet.
   Canvas.popDialog(AnimStageGui);
}

function LoadingGui::onSleep(%this)
{
   for (%i = 0; %i < %this.qLineCount; %i++)
      %this.qLine[%i] = "";
   %this.qLineCount = 0;

   LOAD_MapName.setText("");
   LOAD_MapDescription.setText("");
   LoadingProgress.setValue(0);
   LoadingProgressTxt.setValue("WAITING FOR SERVER");
}


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
