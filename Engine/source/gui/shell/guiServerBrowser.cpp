#include "platform/platform.h"
#include "gui/shell/guiServerBrowser.h"

#include "app/net/serverQuery.h"
#include "app/version.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( GuiServerBrowser );

ConsoleDocClass( GuiServerBrowser,
   "@brief Tribes 2's list of the servers a query found.\n\n"

   "The rows are the server list itself rather than a copy of it, so a row "
   "is whatever the query has learned about that server so far, and the "
   "columns script asked for decide what of it is shown.\n\n"

   "@ingroup GuiShell"
);

static const char *smIconSuffix[] =
{
   "_favorite", "_favorite_hi",
   "_notqueried", "_notqueried_hi",
   "_querying", "_querying_hi",
   "_timedout",
   "_dedicated", "_dedicated_hi",
   "_passworded", "_passworded_hi",
   "_tourney", "_tourney_hi",
   "_penguin",
};

GuiServerBrowser::GuiServerBrowser()
{
   mIconBase = StringTable->insert( "gui/shll_icon" );

   dMemset( &mSelectedAddress, 0, sizeof( mSelectedAddress ) );
   mHasSelection = false;
}

void GuiServerBrowser::initPersistFields()
{
   addField( "iconBase", TypeString, Offset( mIconBase, GuiServerBrowser ),
      "Stem for the icons a server's status and favourite columns show." );

   Parent::initPersistFields();
}

bool GuiServerBrowser::onWake()
{
   if ( !Parent::onWake() )
      return false;

   for ( U32 i = 0; i < IconCount; i++ )
   {
      char name[ 512 ];
      dSprintf( name, sizeof( name ), "%s%s", mIconBase, smIconSuffix[ i ] );

      mIcon[ i ].set( name, &GFXDefaultPersistentProfile,
                      avar( "%s() - icon", __FUNCTION__ ) );
   }

   sort();

   return true;
}

void GuiServerBrowser::onSleep()
{
   Parent::onSleep();

   for ( U32 i = 0; i < IconCount; i++ )
      mIcon[ i ] = NULL;
}

void GuiServerBrowser::onPreRender()
{
   Parent::onPreRender();

   if ( gServerBrowserDirty )
   {
      sort();
      gServerBrowserDirty = false;
   }
}

const ServerInfo *GuiServerBrowser::serverAt( S32 row ) const
{
   return row >= 0 && row < S32( gServerList.size() ) ? &gServerList[ row ] : NULL;
}

S32 GuiServerBrowser::rowOfSelection() const
{
   if ( !mHasSelection )
      return -1;

   for ( U32 i = 0; i < gServerList.size(); i++ )
      if ( Net::compareAddresses( &mSelectedAddress, &gServerList[ i ].address ) )
         return S32( i );

   return -1;
}

static S32 sSortKey = -1;
static bool sSortIncreasing = true;
static S32 sSecondaryKey = -1;
static bool sSecondaryIncreasing = true;

static void statusRank( const ServerInfo &server, S32 &count, S32 &score )
{
   count = 0;
   score = 0;

   if ( server.status.test( GuiServerBrowser::Passworded ) )
   {
      count++;
      score += 4;
   }

   if ( server.status.test( GuiServerBrowser::Dedicated ) )
   {
      count++;
      score += 2;
   }

   if ( server.status.test( GuiServerBrowser::Tournament ) )
   {
      count++;
      score += 1;
   }

   if ( server.status.test( GuiServerBrowser::Linux ) )
      count++;
}

static const char *rulesSetOf( const ServerInfo &server )
{
   if ( server.status.test( GuiServerBrowser::StockRules ) )
      return "base";

   if ( !server.gameType )
      return "";

   return dStricmp( server.gameType, "base" ) == 0 ? "variant" : server.gameType;
}

static S32 compareOn( S32 key, const ServerInfo &a, const ServerInfo &b )
{
   switch ( key )
   {
      case GuiServerBrowser::ServerNameColumn:
         return a.name && b.name ? dStricmp( a.name, b.name ) : 0;

      case GuiServerBrowser::StatusColumn:
      {
         S32 countA, scoreA, countB, scoreB;

         statusRank( a, countA, scoreA );
         statusRank( b, countB, scoreB );

         return countA != countB ? countB - countA : scoreB - scoreA;
      }

      case GuiServerBrowser::FavoriteColumn:
         return S32( b.isFavorite ) - S32( a.isFavorite );

      case GuiServerBrowser::PingColumn:
         return S32( a.ping ) - S32( b.ping );

      case GuiServerBrowser::GameTypeColumn:
         return a.missionType && b.missionType
            ? dStricmp( a.missionType, b.missionType ) : 0;

      case GuiServerBrowser::MissionNameColumn:
         return a.missionName && b.missionName
            ? dStricmp( a.missionName, b.missionName ) : 0;

      case GuiServerBrowser::RulesSetColumn:
         return dStricmp( rulesSetOf( a ), rulesSetOf( b ) );

      case GuiServerBrowser::PlayerCountColumn:
         return S32( a.numPlayers ) - S32( b.numPlayers );

      case GuiServerBrowser::CpuSpeedColumn:
         return S32( a.cpuSpeed ) - S32( b.cpuSpeed );

      case GuiServerBrowser::AddressColumn:
      {
         for ( U32 i = 0; i < 4; i++ )
            if ( a.address.netNum[ i ] != b.address.netNum[ i ] )
               return S32( a.address.netNum[ i ] ) - S32( b.address.netNum[ i ] );

         return S32( a.address.port ) - S32( b.address.port );
      }

      case GuiServerBrowser::VersionColumn:
         return S32( a.version ) - S32( b.version );

      case GuiServerBrowser::VisibilityColumn:
         return S32( b.status.test( GuiServerBrowser::HighVisibility ) )
              - S32( a.status.test( GuiServerBrowser::HighVisibility ) );
   }

   return 0;
}

static S32 QSORT_CALLBACK compareServers( const void *lhs, const void *rhs )
{
   const ServerInfo &a = *reinterpret_cast< const ServerInfo* >( lhs );
   const ServerInfo &b = *reinterpret_cast< const ServerInfo* >( rhs );

   const bool respondedA = a.status.test( ServerInfo::Status_Responded );
   const bool respondedB = b.status.test( ServerInfo::Status_Responded );

   if ( respondedA != respondedB )
      return respondedA ? -1 : 1;

   if ( S32 order = compareOn( sSortKey, a, b ) )
      return sSortIncreasing ? order : -order;

   if ( S32 order = compareOn( sSecondaryKey, a, b ) )
      return sSecondaryIncreasing ? order : -order;

   return 0;
}

void GuiServerBrowser::sort()
{
   sSortKey = mSortColumn;
   sSortIncreasing = mSortIncreasing;
   sSecondaryKey = mSecondarySortColumn;
   sSecondaryIncreasing = mSecondarySortIncreasing;

   if ( gServerList.size() > 1 )
      dQsort( gServerList.address(), gServerList.size(), sizeof( ServerInfo ),
              compareServers );

   setNumRows( S32( gServerList.size() ) );

   mSelectedRow = rowOfSelection();

   setUpdate();
}

void GuiServerBrowser::setSortColumn( S32 key )
{
   Parent::setSortColumn( key );
   sort();
}

void GuiServerBrowser::clearList()
{
   Parent::clearList();

   mHasSelection = false;
}

void GuiServerBrowser::drawIcons( const Point2I &offset, const Column &column,
                                  GFXTexHandle *icons, S32 count )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   S32 span = 0;

   for ( S32 i = 0; i < count; i++ )
      if ( icons[ i ] )
         span += S32( icons[ i ]->getBitmapWidth() );

   S32 x = offset.x + ( column.width > span + 4 ? ( column.width - span ) / 2 : 2 );

   for ( S32 i = 0; i < count; i++ )
   {
      if ( !icons[ i ] )
         continue;

      const S32 height = S32( icons[ i ]->getBitmapHeight() );

      draw->drawBitmap( icons[ i ],
         Point2I( x, offset.y + ( mRowHeight - height ) / 2 ) );

      x += S32( icons[ i ]->getBitmapWidth() );
   }
}

void GuiServerBrowser::onRenderRow( const Point2I &offset, const Column &column,
                                    S32 row, bool selected, bool mouseOver )
{
   Parent::onRenderRow( offset, column, row, selected, mouseOver );

   const ServerInfo *server = serverAt( row );

   if ( !server )
      return;

   char buffer[ 256 ];
   const char *text = NULL;

   switch ( column.key )
   {
      case ServerNameColumn:
         text = server->name ? server->name : "--";
         break;

      case StatusColumn:
      {
         GFXTexHandle icons[ 4 ];
         S32 count = 0;

         if ( server->status.getMask() == ServerInfo::Status_New )
            icons[ count++ ] = mIcon[ selected ? NotQueriedIconSEL : NotQueriedIcon ];
         else if ( server->status.test( ServerInfo::Status_Querying ) )
            icons[ count++ ] = mIcon[ selected ? QueryingIconSEL : QueryingIcon ];
         else if ( server->status.test( ServerInfo::Status_TimedOut ) )
            icons[ count++ ] = mIcon[ TimedOutIcon ];
         else
         {
            if ( server->status.test( Passworded ) )
               icons[ count++ ] = mIcon[ selected ? PasswordedIconSEL : PasswordedIcon ];
            if ( server->status.test( Dedicated ) )
               icons[ count++ ] = mIcon[ selected ? DedicatedIconSEL : DedicatedIcon ];
            if ( server->status.test( Tournament ) )
               icons[ count++ ] = mIcon[ selected ? TourneyIconSEL : TourneyIcon ];
            if ( server->status.test( Linux ) )
               icons[ count++ ] = mIcon[ PenguinIcon ];
         }

         drawIcons( offset, column, icons, count );
         return;
      }

      case FavoriteColumn:
      {
         if ( !server->isFavorite )
            return;

         GFXTexHandle icon = mIcon[ selected ? FavoriteIconSEL : FavoriteIcon ];

         drawIcons( offset, column, &icon, 1 );
         return;
      }

      case PingColumn:
         dSprintf( buffer, sizeof( buffer ), "%d", server->ping );
         text = buffer;
         break;

      case GameTypeColumn:
         text = server->missionType ? server->missionType : "--";
         break;

      case MissionNameColumn:
      {
         if ( !server->missionName )
         {
            text = "--";
            break;
         }

         dStrncpy( buffer, server->missionName, sizeof( buffer ) - 1 );
         buffer[ sizeof( buffer ) - 1 ] = 0;

         if ( char *ext = dStrstr( buffer, ".mis" ) )
            *ext = 0;

         text = buffer;
         break;
      }

      case RulesSetColumn:
         text = rulesSetOf( *server );

         if ( !text[ 0 ] )
            text = "--";
         break;

      case PlayerCountColumn:
         dSprintf( buffer, sizeof( buffer ), "%d/%d (%d)",
                   server->numPlayers, server->maxPlayers, server->numBots );
         text = buffer;
         break;

      case CpuSpeedColumn:
         if ( !server->cpuSpeed )
            text = "--";
         else
         {
            dSprintf( buffer, sizeof( buffer ), "%d MHz", server->cpuSpeed );
            text = buffer;
         }
         break;

      case AddressColumn:
      {
         char address[ 256 ];
         Net::addressToString( &server->address, address );

         dStrncpy( buffer, address + 3, sizeof( buffer ) - 1 );
         buffer[ sizeof( buffer ) - 1 ] = 0;

         text = buffer;
         break;
      }

      case VersionColumn:
         if ( !server->version )
            text = "--";
         else
         {
            dSprintf( buffer, sizeof( buffer ), "%d", server->version );
            text = buffer;
         }
         break;

      case VisibilityColumn:
         text = !server->status.test( ServerInfo::Status_Responded ) ? "--"
              : server->status.test( HighVisibility ) ? "DISTANCE" : "SPEED";
         break;

      default:
         Con::errorf( "GuiServerBrowser - invalid column key %d.", column.key );
         return;
   }

   if ( !text || !text[ 0 ] || !mProfile->mFont )
      return;

   GFont *font = mProfile->mFont;

   S32 set = StockColors;

   if ( server->version != getT2VersionNumber() )
      set = OtherBuildColors;
   else if ( server->status.test( ServerInfo::Status_Responded ) && !server->status.test( StockRules ) )
      set = ModColors;

   ColorI color;

   if ( set == StockColors )
      color = selected ? mProfile->mFontColorSEL
            : mouseOver ? mProfile->mFontColorHL
                        : mProfile->mFontColor;
   else
      color = mProfile->mFontColors[ set + ( selected ? 2 : mouseOver ? 1 : 0 ) ];

   const S32 width = S32( font->getStrWidth( (const UTF8 *)text ) );

   const S32 x = offset.x + getMax( 4, ( column.width - width ) / 2 );
   const S32 y = offset.y + ( mRowHeight - ( S32( font->getHeight() ) - 2 ) ) / 2;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   const RectI saved = GFX->getClipRect();

   RectI clip( offset.x, offset.y, column.width, mRowHeight );

   if ( clip.intersect( saved ) )
   {
      GFX->setClipRect( clip );

      draw->setBitmapModulation( color );
      draw->drawText( font, Point2I( x, y ), text );
   }

   GFX->setClipRect( saved );
}

void GuiServerBrowser::setSelectedRow( S32 row )
{
   Parent::setSelectedRow( row );

   const ServerInfo *server = serverAt( mSelectedRow );

   if ( !server )
   {
      mHasSelection = false;
      return;
   }

   mSelectedAddress = server->address;
   mHasSelection = true;

   char address[ 256 ];
   Net::addressToString( &mSelectedAddress, address );

   Con::executef( this, "onSelect", address );
}

void GuiServerBrowser::onCellClicked( S32 row, S32 column )
{
   if ( column < 0 || getColumnKey( column ) != FavoriteColumn )
      return;

   ServerInfo *server = const_cast< ServerInfo * >( serverAt( row ) );

   if ( !server )
      return;

   server->isFavorite = !server->isFavorite;
   setUpdate();

   char address[ 256 ];
   Net::addressToString( &server->address, address );

   if ( server->isFavorite )
      Con::executef( this, "addFavorite", server->name ? server->name : "", address );
   else
      Con::executef( this, "removeFavorite", address );
}

void GuiServerBrowser::selectRowByAddress( const char *address )
{
   NetAddress wanted;

   if ( !Net::stringToAddress( address, &wanted ) )
      return;

   for ( U32 i = 0; i < gServerList.size(); i++ )
      if ( Net::compareAddresses( &wanted, &gServerList[ i ].address ) )
      {
         setSelectedRow( S32( i ) );
         scrollVisible( S32( i ) );
         return;
      }
}

const char *GuiServerBrowser::getServerStatus() const
{
   const ServerInfo *server = serverAt( rowOfSelection() );

   if ( !server )
      return "invalid";

   if ( server->status.test( ServerInfo::Status_TimedOut ) )
      return "timedOut";
   if ( server->status.test( ServerInfo::Status_Updating ) )
      return "updating";
   if ( server->status.test( ServerInfo::Status_Responded ) )
      return "responded";
   if ( server->status.test( ServerInfo::Status_Querying ) )
      return "querying";

   return "new";
}

const char *GuiServerBrowser::getServerInfoString() const
{
   const ServerInfo *server = serverAt( rowOfSelection() );

   if ( !server )
      return "";

   char address[ 256 ];
   Net::addressToString( &server->address, address );

   String flags = server->status.test( HighVisibility )
      ? "High Visibility" : "High Frame Rate";

   if ( server->status.test( Dedicated ) )
      flags += "; Dedicated";
   if ( server->status.test( Passworded ) )
      flags += "; Passworded";
   if ( server->status.test( Linux ) )
      flags += "; Linux";
   if ( server->status.test( Tournament ) )
      flags += "; Tournament Mode";
   if ( server->status.test( NoAliases ) )
      flags += "; No Aliases";

   flags += server->status.test( TeamDamage ) ? "; Team Damage On"
                                              : "; Team Damage Off";

   String out;

   out += server->name ? server->name : "";
   out += "\n";
   out += address + 3;   // the IP: prefix is not worth showing
   out += "\n";
   out += rulesSetOf( *server );
   out += "\n";
   out += flags;
   out += "\n";
   out += server->missionType ? server->missionType : "";
   out += "\n";
   out += server->missionName ? server->missionName : "";
   out += "\n";
   out += server->infoString ? server->infoString : "";

   return Con::getReturnBuffer( out.c_str() );
}

const char *GuiServerBrowser::getServerContentString() const
{
   const ServerInfo *server = serverAt( rowOfSelection() );

   if ( !server || !server->statusString )
      return "";

   return Con::getReturnBuffer( server->statusString );
}

bool GuiServerBrowser::findServer( const char *pattern )
{
   mSearchPattern = pattern;

   return findNextServer();
}

bool GuiServerBrowser::findNextServer()
{
   if ( mSearchPattern.isEmpty() || !gServerList.size() )
      return false;

   const bool ignoreCase = Con::getBoolVariable( "$pref::ServerBrowser::ignoreCase", true );

   const S32 count = S32( gServerList.size() );
   const S32 from = mSelectedRow >= 0 ? mSelectedRow : -1;

   for ( S32 i = 1; i <= count; i++ )
   {
      const S32 row = ( from + i ) % count;

      const char *name = gServerList[ row ].name;

      if ( !name )
         continue;

      const char *hit = ignoreCase ? dStristr( name, mSearchPattern.c_str() )
                                   : dStrstr( name, mSearchPattern.c_str() );

      if ( hit )
      {
         setSelectedRow( row );
         scrollVisible( row );
         return true;
      }
   }

   return false;
}

DefineEngineMethod( GuiServerBrowser, sort, void, ( ), ,
   "@brief Put the rows back in the order the chosen column asks for.\n\n"
   "@ingroup GuiShell" )
{
   object->sort();
}

DefineEngineMethod( GuiServerBrowser, getServerStatus, const char *, ( ), ,
   "@brief How far the query into the chosen server has got: invalid, new, "
   "querying, updating, responded or timedOut.\n\n@ingroup GuiShell" )
{
   return object->getServerStatus();
}

DefineEngineMethod( GuiServerBrowser, getServerInfoString, const char *, ( ), ,
   "@brief What the chosen server said about itself, a line per heading.\n\n"
   "@ingroup GuiShell" )
{
   return object->getServerInfoString();
}

DefineEngineMethod( GuiServerBrowser, getServerContentString, const char *, ( ), ,
   "@brief The chosen server's teams and players.\n\n@ingroup GuiShell" )
{
   return object->getServerContentString();
}

DefineEngineMethod( GuiServerBrowser, selectRowByAddress, void,
   ( const char *address, bool ), ( false ),
   "@brief Choose the server at this address, if it is listed.\n\n"
   "@ingroup GuiShell" )
{
   object->selectRowByAddress( address );
}

DefineEngineMethod( GuiServerBrowser, findServer, bool, ( const char *pattern ), ,
   "@brief Choose the next server whose name has this in it.\n\n"
   "@ingroup GuiShell" )
{
   return object->findServer( pattern );
}

DefineEngineMethod( GuiServerBrowser, findNextServer, bool, ( ), ,
   "@brief Choose the server after that one.\n\n@ingroup GuiShell" )
{
   return object->findNextServer();
}
