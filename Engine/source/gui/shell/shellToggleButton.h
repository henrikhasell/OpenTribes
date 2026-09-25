// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTOGGLEBUTTON_H_
#define _SHELLTOGGLEBUTTON_H_

#ifndef _SHELLRADIOBUTTON_H_
#include "gui/shell/shellRadioButton.h"
#endif

class ShellToggleButton : public ShellRadioButton
{
   typedef ShellRadioButton Parent;

public:
   ShellToggleButton();

   DECLARE_CONOBJECT( ShellToggleButton );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell button that latches on its own." );
};

#endif // _SHELLTOGGLEBUTTON_H_
