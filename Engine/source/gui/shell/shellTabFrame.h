// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTABFRAME_H_
#define _SHELLTABFRAME_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

class ShellTabFrame : public GuiControl
{
   typedef GuiControl Parent;

   ShellSkin mSkin;

   bool mIsVertical;
   bool mUseCloseButton;
   S32 mEdgeInset;

   bool mUseAltColor;

   GFXTexHandle part( const char *name );

public:
   ShellTabFrame();

   DECLARE_CONOBJECT( ShellTabFrame );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The panel a Tribes 2 tab bar stands on." );

   static void initPersistFields();

   bool onWake();
   void onRender( Point2I offset, const RectI &updateRect );

   void setAltColor( bool alt );
};

#endif // _SHELLTABFRAME_H_
