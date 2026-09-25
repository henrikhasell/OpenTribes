// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLFIELDCTRL_H_
#define _SHELLFIELDCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

class ShellFieldCtrl : public GuiControl
{
   typedef GuiControl Parent;

   enum Part
   {
      TopLeft, TopMiddle, TopRight,
      MidLeft, MidMiddle, MidRight,
      BotLeft, BotMiddle, BotRight,
      PartCount
   };

   static const char *smSuffix[ PartCount ];

   ShellSkin mSkin;

   bool parts( GFXTexHandle *out );

public:
   DECLARE_CONOBJECT( ShellFieldCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The frame a Tribes 2 shell panel is drawn in." );

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLFIELDCTRL_H_
