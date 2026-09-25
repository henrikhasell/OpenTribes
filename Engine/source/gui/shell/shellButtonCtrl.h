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

   /// A group of tabs drawn from their own art in their own colours.
   ///
   /// The bar is not one strip of identical tabs: GameGui.gui puts WARRIOR
   /// SETUP in a set of its own so that it reads as a different kind of thing
   /// from JOIN and HOST beside it, and script asks getTabSet which set the
   /// open tab is in to tint the panel below to match.
   struct Set
   {
      S32 id;
      ShellStateBitmap art;
      ColorI fontColor;
      ColorI fontColorHL;
      ColorI fontColorSEL;
   };

   Vector< Tab > mTabs;
   Vector< Set > mSets;

   S32 mSelected;
   S32 mMouseOver;

   S32 mTabSpacing;
   S32 mMaxTabWidth;
   S32 mGlowOffset;
   bool mStretchToFit;

   /// The narrowest a tab is allowed to be: its two caps plus room for one
   /// letter.  Worked out at wake, because it depends on the art and the font.
   S32 mMinTabWidth;

   /// How thick a tab is across the bar -- the art's own height, less the
   /// margin the art leaves clear for the glow.  Worked out at wake.
   S32 mTabThickness;

   /// How wide one tab is, given how many there are and how much room, and
   /// how many of them get a spare pixel.
   S32 tabExtent( S32 *remainder = NULL ) const;

   /// Where a tab sits, in the bar's own coordinates.  `along` is how far the
   /// tab starts down the row, `size` how much of the row it takes.
   RectI tabRect( S32 along, S32 size ) const;

   /// Which tab `point` is over, or -1.
   S32 hitTab( const Point2I &point ) const;

   bool horizontal() const { return getWidth() >= getHeight(); }

   /// Where the set with this id sits, or -1.
   S32 findSet( S32 id ) const;

   /// The set a tab draws from.  A tab naming a set that was never registered
   /// falls back to the first one, which is what Tribes 2 does.
   const Set *setFor( const Tab &tab ) const;

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

   bool addSet( S32 id, const char *bitmap, const ColorI &fontColor,
                const ColorI &fontColorHL, const ColorI &fontColorSEL );
   void removeSet( S32 index );
   void clearTabSets();

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
