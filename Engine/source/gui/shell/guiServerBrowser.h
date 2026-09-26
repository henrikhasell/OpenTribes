// HH: Added for Tribes 2 compatibility.
#ifndef _GUISERVERBROWSER_H_
#define _GUISERVERBROWSER_H_

#ifndef _SHELLFANCYARRAY_H_
#include "gui/shell/shellFancyArray.h"
#endif
#ifndef _PLATFORMNET_H_
#include "platform/platformNet.h"
#endif

struct ServerInfo;

class GuiServerBrowser : public ShellFancyArray
{
   typedef ShellFancyArray Parent;

public:
   enum ColumnKey
   {
      ServerNameColumn = 0,
      StatusColumn,
      FavoriteColumn,
      PingColumn,
      GameTypeColumn,
      MissionNameColumn,
      RulesSetColumn,
      PlayerCountColumn,
      CpuSpeedColumn,
      AddressColumn,
      VersionColumn,
      VisibilityColumn,

      ColumnKeyCount,
   };

   enum InfoFlags
   {
      Dedicated = BIT( 0 ),
      Passworded = BIT( 1 ),
      Linux = BIT( 2 ),
      Tournament = BIT( 3 ),
      NoAliases = BIT( 4 ),
      TeamDamage = BIT( 5 ),
      StockRules = BIT( 6 ),
      HighVisibility = BIT( 7 ),
   };

protected:
   enum Icon
   {
      FavoriteIcon = 0, FavoriteIconSEL,
      NotQueriedIcon, NotQueriedIconSEL,
      QueryingIcon, QueryingIconSEL,
      TimedOutIcon,
      DedicatedIcon, DedicatedIconSEL,
      PasswordedIcon, PasswordedIconSEL,
      TourneyIcon, TourneyIconSEL,
      PenguinIcon,

      IconCount,
   };

   enum ColorSet
   {
      StockColors = 0,
      ModColors = 4,
      OtherBuildColors = 7,
   };

   StringTableEntry mIconBase;
   GFXTexHandle mIcon[ IconCount ];

   NetAddress mSelectedAddress;
   bool mHasSelection;

   String mSearchPattern;

   const ServerInfo *serverAt( S32 row ) const;

   S32 rowOfSelection() const;

   void drawIcons( const Point2I &offset, const Column &column,
                   GFXTexHandle *icons, S32 count );

   void onRenderRow( const Point2I &offset, const Column &column,
                     S32 row, bool selected, bool mouseOver );

   void onCellClicked( S32 row, S32 column );

public:
   GuiServerBrowser();

   DECLARE_CONOBJECT( GuiServerBrowser );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "Tribes 2's list of the servers a query found." );

   static void initPersistFields();

   bool onWake();
   void onSleep();
   void onPreRender();

   void clearList();
   void setSelectedRow( S32 row );
   void setSortColumn( S32 key );

   void sort();

   const char *getServerStatus() const;
   const char *getServerInfoString() const;
   const char *getServerContentString() const;

   void selectRowByAddress( const char *address );

   bool findServer( const char *pattern );
   bool findNextServer();
};

#endif // _GUISERVERBROWSER_H_
