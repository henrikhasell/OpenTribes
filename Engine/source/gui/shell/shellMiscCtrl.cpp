#include "gui/shell/shellMiscCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiConsoleVariableCtrl);
IMPLEMENT_CONOBJECT(GuiDashBoardCtrl);

//-----------------------------------------------------------------------------
GuiConsoleVariableCtrl::GuiConsoleVariableCtrl()
{
   mExpression = StringTable->insert("");
}

void GuiConsoleVariableCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("expression", TypeCaseString, Offset(mExpression, GuiConsoleVariableCtrl));
}

void GuiConsoleVariableCtrl::onPreRender()
{
   if(mExpression[0])
   {
      // Evaluated rather than looked up: Tribes 2 puts whole expressions in
      // here, not just variable names.
      const char *value = Con::evaluatef("return %s;", mExpression);
      if(value)
         setText(value);
   }

   Parent::onPreRender();
}

//-----------------------------------------------------------------------------
GuiDashBoardCtrl::GuiDashBoardCtrl()
{
   mWrap = false;
   mInOnResize = false;
}

void GuiDashBoardCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("wrap", TypeBool, Offset(mWrap, GuiDashBoardCtrl));
}

// hud.cs places the dashboard from dashboardHud::onResize, by screen size and
// seat, so without the callback it stays where PlayGui.gui left it. The script
// moves the dashboard itself, which lands back here; it centres by halving the
// spare width without rounding, so on an odd width it never sees the position
// it asked for and would move it again without end. Its own move does not call
// it again.
bool GuiDashBoardCtrl::resize(const Point2I &newPosition, const Point2I &newExtent)
{
   const bool ret = Parent::resize(newPosition, newExtent);
   if(!mInOnResize && isMethod("onResize"))
   {
      char width[16], height[16];
      dSprintf(width, sizeof(width), "%d", getWidth());
      dSprintf(height, sizeof(height), "%d", getHeight());

      mInOnResize = true;
      Con::executef(this, "onResize", width, height);
      mInOnResize = false;
   }
   return ret;
}
