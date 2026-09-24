// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLBUTTONCTRL_H_
#define _SHELLBUTTONCTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _SHELLSTATEBITMAP_H_
#include "gui/shell/shellStateBitmap.h"
#endif

/// The shell's ordinary button.
///
/// Draws one state of its profile's art across itself and centres the label
/// over it.  GuiButtonBaseCtrl already owns what pressing one means, so this
/// only decides which state to draw.
class ShellBitmapButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

protected:
   ShellStateBitmap mArt;

   bool mSimpleStyle;

   /// Which band of the art the control's current state calls for.
   virtual S32 artState();

public:
   ShellBitmapButton();

   DECLARE_CONOBJECT( ShellBitmapButton );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell button." );

   static void initPersistFields();

   bool onWake();
   void onRender( Point2I offset, const RectI &updateRect );
};

/// The row of tabs along the launch bar.
///
/// The tabs are the control's own, not its children: script calls addTab with
/// an id and a label and the control draws them, which is how the launch bar
/// grows a tab per screen the player opens and drops it again when they close
/// it.  A tab can also be inactive -- drawn, labelled, and not selectable --
/// which is how the demo shows the screens it does not have.
class ShellTabGroupCtrl : public GuiControl
{
   typedef GuiControl Parent;

   struct Tab
   {
      S32 id;                  ///< Script's, and not necessarily unique.
      StringTableEntry text;
      bool active;
      S32 set;
   };

   Vector< Tab > mTabs;
   S32 mSelected;
   S32 mMouseOver;

   ShellStateBitmap mArt;

   S32 mTabSpacing;
   S32 mMaxTabWidth;
   S32 mGlowOffset;
   bool mStretchToFit;

   /// How wide one tab is, given how many there are and how much room.
   S32 tabExtent() const;

   /// Which tab `point` is over, or -1.
   S32 hitTab( const Point2I &point ) const;

   bool horizontal() const { return getWidth() >= getHeight(); }

public:
   ShellTabGroupCtrl();

   DECLARE_CONOBJECT( ShellTabGroupCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The row of tabs across Tribes 2's launch bar." );

   static void initPersistFields();

   bool onWake();
   void onRender( Point2I offset, const RectI &updateRect );

   void onMouseDown( const GuiEvent &event );
   void onMouseMove( const GuiEvent &event );
   void onMouseLeave( const GuiEvent &event );

   //-- what script drives -----------------------------------------------------
   S32 getTabCount() const { return mTabs.size(); }

   void addTab( S32 id, const char *text, S32 set = 0 );
   void removeTabByIndex( S32 index );

   /// By the id script gave the tab, which addLaunchTab warns is not unique --
   /// the first match wins, which is what Tribes 2 does with it.
   S32 indexOfId( S32 id ) const;
   S32 getTabId( S32 index ) const;

   void clear();

   void setTabText( S32 index, const char *text );
   const char *getTabText( S32 index ) const;

   void setTabActive( S32 index, bool active );
   bool isTabActive( S32 index ) const;

   S32 getTabSet( S32 index ) const;

   S32 getSelectedTab() const { return mSelected; }
   void setSelectedByIndex( S32 index );
   S32 getSelectedId() const;
   const char *getSelectedText() const;
};

#endif // _SHELLBUTTONCTRL_H_
