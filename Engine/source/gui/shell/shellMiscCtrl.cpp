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
}

void GuiDashBoardCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("wrap", TypeBool, Offset(mWrap, GuiDashBoardCtrl));
}
