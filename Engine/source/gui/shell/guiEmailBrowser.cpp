#include "platform/platform.h"
#include "gui/shell/guiEmailBrowser.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( GuiEmailBrowser );

ConsoleDocClass( GuiEmailBrowser,
   "@brief Tribes 2's list of the messages in a mailbox.\n\n"

   "A row is a message's id, sender, subject, date and flags.  Flags of zero "
   "mean unread, which takes the unread typeface and colour and the unread "
   "icon in the status column.\n\n"

   "@ingroup GuiShell"
);

GuiEmailBrowser::GuiEmailBrowser()
{
   mIconBase = StringTable->insert( "gui/email" );
   mUnreadFontType = StringTable->insert( "Verdana Bold" );
   mUnreadFontSize = 12;
   mUnreadFontColor.set( 6, 245, 215, 255 );
}

void GuiEmailBrowser::initPersistFields()
{
   addField( "iconBase", TypeString, Offset( mIconBase, GuiEmailBrowser ),
      "Stem for the read and unread icons of the status column." );
   addField( "unreadFontType", TypeString, Offset( mUnreadFontType, GuiEmailBrowser ),
      "Typeface for an unread message." );
   addField( "unreadFontSize", TypeS32, Offset( mUnreadFontSize, GuiEmailBrowser ),
      "Size for an unread message." );
   addField( "unreadFontColor", TypeColorI, Offset( mUnreadFontColor, GuiEmailBrowser ),
      "Colour of an unread message that is neither chosen nor under the mouse." );

   Parent::initPersistFields();
}

bool GuiEmailBrowser::onWake()
{
   if ( !Parent::onWake() )
      return false;

   if ( mProfile->mFont )
      mRowHeight = S32( mProfile->mFont->getHeight() ) + 3;

   if ( mIconBase && mIconBase[ 0 ] )
   {
      char name[ 512 ];

      dSprintf( name, sizeof( name ), "%s_read", mIconBase );
      mReadIcon.set( name, &GFXDefaultPersistentProfile,
                     avar( "%s() - read", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_notread", mIconBase );
      mNotReadIcon.set( name, &GFXDefaultPersistentProfile,
                        avar( "%s() - not read", __FUNCTION__ ) );
   }

   if ( mUnreadFontType && mUnreadFontType[ 0 ] && mUnreadFontSize > 0 )
      mUnreadFont = GFont::create( mUnreadFontType, mUnreadFontSize,
                                   Con::getVariable( "$GUI::fontCacheDirectory" ) );

   if ( !mUnreadFont )
      mUnreadFont = mProfile->mFont;
   else
      mRowHeight = getMax( mRowHeight, S32( mUnreadFont->getHeight() ) + 3 );

   return true;
}

void GuiEmailBrowser::onSleep()
{
   Parent::onSleep();

   mReadIcon = NULL;
   mNotReadIcon = NULL;
   mUnreadFont = NULL;
}

void GuiEmailBrowser::onRenderRow( const Point2I &offset, const Column &column,
                                   S32 row, bool selected, bool mouseOver )
{
   Parent::onRenderRow( offset, column, row, selected, mouseOver );

   if ( row < 0 || row >= mRows.size() )
      return;

   const Row &entry = mRows[ row ];
   const bool read = entry.flags != 0;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const char *text = NULL;

   switch ( column.key )
   {
      case StatusColumn:
      {
         GFXTexHandle icon = read ? mReadIcon : mNotReadIcon;

         if ( icon )
         {
            const S32 w = S32( icon->getBitmapWidth() );
            const S32 h = S32( icon->getBitmapHeight() );

            draw->drawBitmap( icon,
               Point2I( offset.x + getMax( 2, ( column.width - w ) / 2 ),
                        offset.y + getMax( 0, ( mRowHeight - h ) / 2 ) ) );
            return;
         }

         text = read ? "" : "!";
         break;
      }

      case FromColumn:
         text = entry.from.c_str();
         break;

      case SubjectColumn:
         text = entry.subject.c_str();
         break;

      case ReceivedColumn:
         text = entry.received.c_str();
         break;

      default:
         Con::errorf( "GuiEmailBrowser - invalid column key %d.", column.key );
         return;
   }

   GFont *font = read ? (GFont *)mProfile->mFont : (GFont *)mUnreadFont;

   if ( !text || !text[ 0 ] || !font )
      return;

   const ColorI &color = selected ? mProfile->mFontColorSEL
                       : mouseOver ? mProfile->mFontColorHL
                       : read ? mProfile->mFontColor
                              : mUnreadFontColor;

   const Point2I at( offset.x + 4,
      offset.y + ( mRowHeight - ( S32( font->getHeight() ) - 2 ) ) / 2 );

   const RectI saved = GFX->getClipRect();

   RectI clip( offset.x, offset.y, column.width, mRowHeight );

   if ( clip.intersect( saved ) )
   {
      GFX->setClipRect( clip );

      draw->setBitmapModulation( color );
      draw->drawText( font, at, text );
   }

   GFX->setClipRect( saved );
}

void GuiEmailBrowser::clearList()
{
   mRows.clear();

   Parent::clearList();
}

void GuiEmailBrowser::setSelectedRow( S32 row )
{
   Parent::setSelectedRow( row );

   if ( mSelectedRow < 0 )
      return;

   Con::executef( this, "onSelect", Con::getIntArg( mRows[ mSelectedRow ].id ) );
}

void GuiEmailBrowser::setSortColumn( S32 key )
{
   Parent::setSortColumn( key );
   sort();
}

void GuiEmailBrowser::addRow( S32 id, const char *from, const char *subject,
                              const char *received, S32 flags )
{
   mRows.increment();

   Row &row = mRows.last();

   row.id = id;
   row.from = from;
   row.subject = subject;
   row.received = received;
   row.flags = flags;

   setNumRows( mRows.size() );
}

void GuiEmailBrowser::setRow( S32 id, const char *from, const char *subject,
                              const char *received, S32 flags )
{
   const S32 index = findById( id );

   if ( index == -1 )
   {
      addRow( id, from, subject, received, flags );
      return;
   }

   Row &row = mRows[ index ];

   row.from = from;
   row.subject = subject;
   row.received = received;
   row.flags = flags;

   setUpdate();
}

void GuiEmailBrowser::setRowFlags( S32 id, S32 flags )
{
   const S32 index = findById( id );

   if ( index == -1 )
   {
      Con::errorf( "Email entry id #%d not found!", id );
      return;
   }

   if ( mRows[ index ].flags != flags )
   {
      mRows[ index ].flags = flags;
      setUpdate();
   }
}

void GuiEmailBrowser::removeRowByIndex( S32 index )
{
   if ( index < 0 || index >= mRows.size() )
      return;

   mRows.erase( index );

   const S32 selected = mSelectedRow;

   // Parent::setNumRows forgets a selection past the end, so choose first.
   mRowCount = mRows.size();

   if ( selected == index )
      setSelectedRow( selected < mRowCount ? selected : selected - 1 );
   else if ( selected > index )
      setSelectedRow( selected - 1 );

   setNumRows( mRows.size() );
}

void GuiEmailBrowser::removeRowById( S32 id )
{
   removeRowByIndex( findById( id ) );
}

S32 GuiEmailBrowser::findById( S32 id ) const
{
   for ( S32 i = 0; i < mRows.size(); i++ )
      if ( mRows[ i ].id == id )
         return i;

   return -1;
}

void GuiEmailBrowser::selectRowById( S32 id )
{
   setSelectedRow( findById( id ) );
}

S32 GuiEmailBrowser::getSelectedId() const
{
   return mSelectedRow >= 0 && mSelectedRow < mRows.size()
      ? mRows[ mSelectedRow ].id : -1;
}

static S32 sSortKey = -1;
static bool sSortIncreasing = true;

static S32 QSORT_CALLBACK compareRows( const void *lhs, const void *rhs )
{
   const GuiEmailBrowser::Row &a = *reinterpret_cast< const GuiEmailBrowser::Row * >( lhs );
   const GuiEmailBrowser::Row &b = *reinterpret_cast< const GuiEmailBrowser::Row * >( rhs );

   S32 order = 0;

   switch ( sSortKey )
   {
      case GuiEmailBrowser::FromColumn:
         order = dStricmp( a.from.c_str(), b.from.c_str() );
         break;

      case GuiEmailBrowser::SubjectColumn:
         order = dStricmp( a.subject.c_str(), b.subject.c_str() );
         break;
   }

   if ( !order )
      order = dStricmp( a.received.c_str(), b.received.c_str() );

   return sSortIncreasing ? order : -order;
}

void GuiEmailBrowser::sort()
{
   const S32 selected = getSelectedId();

   sSortKey = mSortColumn;
   sSortIncreasing = mSortIncreasing;

   if ( mRows.size() > 1 )
      dQsort( mRows.address(), mRows.size(), sizeof( Row ), compareRows );

   setNumRows( mRows.size() );

   if ( selected != -1 )
      selectRowById( selected );

   setUpdate();
}

DefineEngineMethod( GuiEmailBrowser, clear, void, ( ), ,
   "@brief Take out every message.\n\n@ingroup GuiShell" )
{
   object->clearList();
}

DefineEngineMethod( GuiEmailBrowser, addRow, void,
   ( S32 id, const char *from, const char *subject, const char *received, S32 flags ), ,
   "@brief Add a message.  Flags of zero mean it is unread.\n\n@ingroup GuiShell" )
{
   object->addRow( id, from, subject, received, flags );
}

DefineEngineMethod( GuiEmailBrowser, removeRowById, void, ( S32 id ), ,
   "@brief Take out the message with this id.\n\n@ingroup GuiShell" )
{
   object->removeRowById( id );
}

DefineEngineMethod( GuiEmailBrowser, removeRowByIndex, void, ( S32 index ), ,
   "@brief Take out the message at this index.\n\n@ingroup GuiShell" )
{
   object->removeRowByIndex( index );
}

DefineEngineMethod( GuiEmailBrowser, setRow, void,
   ( S32 id, const char *from, const char *subject, const char *received, S32 flags ), ,
   "@brief Replace the message with this id, or add it.\n\n@ingroup GuiShell" )
{
   object->setRow( id, from, subject, received, flags );
}

DefineEngineMethod( GuiEmailBrowser, setRowFlags, void, ( S32 id, S32 flags ), ,
   "@brief Change the flags of the message with this id.\n\n@ingroup GuiShell" )
{
   object->setRowFlags( id, flags );
}

DefineEngineMethod( GuiEmailBrowser, findById, S32, ( S32 id ), ,
   "@brief The index of the message with this id, or -1.\n\n@ingroup GuiShell" )
{
   return object->findById( id );
}

DefineEngineMethod( GuiEmailBrowser, selectRowById, void, ( S32 id ), ,
   "@brief Choose the message with this id.\n\n@ingroup GuiShell" )
{
   object->selectRowById( id );
}

DefineEngineMethod( GuiEmailBrowser, getSelectedId, S32, ( ), ,
   "@brief The id of the chosen message, or -1.\n\n@ingroup GuiShell" )
{
   return object->getSelectedId();
}

DefineEngineMethod( GuiEmailBrowser, sort, void, ( ), ,
   "@brief Put the messages in the order the chosen column asks for.\n\n"
   "@ingroup GuiShell" )
{
   object->sort();
}
