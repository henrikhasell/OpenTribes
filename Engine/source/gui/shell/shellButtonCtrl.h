// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLBUTTONCTRL_H_
#define _SHELLBUTTONCTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif
#ifndef _SHELLSTATEBITMAP_H_
#include "gui/shell/shellStateBitmap.h"
#endif

class ShellBitmapButton : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

protected:
   ShellStateBitmap mArt;

   bool mSimpleStyle;

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

   S32 mMinTabWidth;

   S32 mTabThickness;

   S32 tabExtent( S32 *remainder = NULL ) const;

   RectI tabRect( S32 along, S32 size ) const;

   S32 hitTab( const Point2I &point ) const;

   bool horizontal() const { return getWidth() >= getHeight(); }

   S32 findSet( S32 id ) const;

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

   S32 getTabCount() const { return mTabs.size(); }

   void addTab( S32 id, const char *text, S32 set = 0 );
   void removeTabByIndex( S32 index );

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
