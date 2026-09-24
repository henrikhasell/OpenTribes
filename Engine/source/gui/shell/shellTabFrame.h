// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLTABFRAME_H_
#define _SHELLTABFRAME_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

/// The panel a row of tabs stands on.
///
/// Two rails with chamfered outer corners and a line stretched between them --
/// no more than that.  The rails are a gradient that is solid where the tabs
/// meet them and fades away down the panel, which is what makes the open tab
/// look joined to what it opened.
///
/// It draws in one of two palettes.  Script switches between them with
/// setAltColor, and what it passes is which set the open tab belongs to:
///
///   GM_TabFrame.setAltColor( %this.getTabSet( %id ) != 0 );
///
/// so the panel is tinted to match the tab above it, and the art comes in
/// pairs -- grad and grada, gradedge and gradedgea -- rather than being
/// modulated.
class ShellTabFrame : public GuiControl
{
   typedef GuiControl Parent;

   ShellSkin mSkin;

   bool mIsVertical;
   bool mUseCloseButton;
   S32 mEdgeInset;

   bool mUseAltColor;

   /// The art for the palette in use.  Tribes 2 spells these without a
   /// separator, so the stem runs straight into them.
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
