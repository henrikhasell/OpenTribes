#include "platform/platform.h"
#include "gui/shell/shellToggleButton.h"

#include "console/console.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT( ShellToggleButton );

ConsoleDocClass( ShellToggleButton,
   "@brief A Tribes 2 shell button that latches on its own.\n\n"

   "The same art and the same bar as ShellRadioButton -- Tribes 2 builds the "
   "two from one piece of code -- but it answers for itself rather than for a "
   "group, so pressing one does not release its neighbours.\n\n"

   "@ingroup GuiShell"
);

ShellToggleButton::ShellToggleButton()
{
   mButtonType = ButtonTypeCheck;
}
