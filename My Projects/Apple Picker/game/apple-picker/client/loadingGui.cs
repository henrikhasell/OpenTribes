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
// The screen is the game's own -- ui/LoadingGui.gui, built out of nothing but
// colours, so it has no content that can go missing.  Core ships one under the
// same name and initializeCore() has already exec'd it by the time any of this
// runs, so that one has to go first: two objects can hold one name in the Sim
// dictionary with only one of them answering to it.
//-----------------------------------------------------------------------------

new GuiControlProfile (LoadingGuiProfile)
{
   opaque = true;
   fillColor = "104 108 120";
};

new GuiControlProfile (LoadingGuiPanelProfile)
{
   opaque = true;
   fillColor = "200 200 200";
   border = true;
   borderColor = "0 0 0";
};

// Two profiles the .gui would rather have taken from core under their common/
// names, and cannot.
//
// The map name is authored eight pixels wide and grows to whatever it is given,
// which TGE's GuiTextProfile arranged with autoSizeWidth.  Core's does not set
// it, so without this the name draws clipped to those eight pixels.
new GuiControlProfile (LoadingGuiNameProfile : GuiMediumTextProfile)
{
   autoSizeWidth = true;
};

// Core's GuiProgressProfile is a brighter blue than the bar this screen was
// drawn around; these are common/'s colours.
new GuiControlProfile (LoadingGuiBarProfile)
{
   opaque = false;
   fillColor = "44 152 162 100";
   border = true;
   borderColor = "78 88 120";
};

// Core's, and its children with it, which is what frees LoadingProgress and
// LoadingProgressTxt for ours to take.  ui/LoadingGui.gui is exec'd over the
// name straight after this file -- see client/init.cs.
if (isObject(LoadingGui))
   LoadingGui.delete();


//-----------------------------------------------------------------------------

function LoadingGui::onAdd(%this)
{
   %this.qLineCount = 0;
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
