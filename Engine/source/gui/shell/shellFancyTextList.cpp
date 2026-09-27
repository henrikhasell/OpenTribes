#include "platform/platform.h"
#include "gui/shell/shellFancyTextList.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( ShellFancyTextList );

ConsoleDocClass( ShellFancyTextList,
   "@brief A Tribes 2 browser list with columns of text.\n\n"

   "A row is one string with a tab between each column's worth of it, an id "
   "script knows it by, and a style.  A style is a typeface and up to three "
   "colours, which is how a list marks a row out without a column saying "
   "so.\n\n"

   "@ingroup GuiShell"
);

ShellFancyTextList::ShellFancyTextList()
{
   mAllowColorChars = false;
}

void ShellFancyTextList::initPersistFields()
{
   addField( "allowColorChars", TypeBool, Offset( mAllowColorChars, ShellFancyTextList ),
      "Let a row's text carry colour escapes of its own." );

   Parent::initPersistFields();
}

ShellFancyTextList::Style *ShellFancyTextList::styleFor( S32 row )
{
   if ( row < 0 || row >= mRows.size() )
      return NULL;

   for ( U32 i = 0; i < mStyles.size(); i++ )
      if ( mStyles[ i ].id == mRows[ row ].style )
         return &mStyles[ i ];

   return NULL;
}

const char *ShellFancyTextList::field( const char *text, S32 index, S32 &length )
{
   length = 0;

   if ( !text )
      return NULL;

   for ( S32 i = 0; i < index; i++ )
   {
      const char *tab = dStrchr( text, '\t' );

      if ( !tab )
         return NULL;

      text = tab + 1;
   }

   const char *tab = dStrchr( text, '\t' );

   length = tab ? S32( tab - text ) : dStrlen( text );

   return text;
}

void ShellFancyTextList::onRenderRow( const Point2I &offset, const Column &column,
                                      S32 row, bool selected, bool mouseOver )
{
   Parent::onRenderRow( offset, column, row, selected, mouseOver );

   if ( row < 0 || row >= mRows.size() )
      return;

   Style *style = styleFor( row );

   GFont *font = style && style->font ? (GFont *)style->font : mProfile->mFont;

   if ( !font )
      return;

   S32 which = 0;

   for ( U32 i = 0; i < mColumns.size(); i++ )
      if ( mColumns[ i ].key == column.key )
      {
         which = S32( i );
         break;
      }

   S32 length = 0;
   const char *text = field( mRows[ row ].text.c_str(), which, length );

   if ( !text || !length )
      return;

   const S32 state = selected ? Selected : mouseOver ? Highlight : Resting;

   ColorI color;

   if ( style && style->given[ state ] )
      color = style->color[ state ];
   else
      color = state == Selected ? mProfile->mFontColorSEL
            : state == Highlight ? mProfile->mFontColorHL
                                 : mProfile->mFontColor;

   const S32 width = S32( font->getStrNWidth( (const UTF8 *)text, length ) );

   S32 x = offset.x + 2;

   if ( column.flags & AlignCenter )
      x = offset.x + ( column.width - width ) / 2;
   else if ( column.flags & AlignRight )
      x = offset.x + column.width - width - 2;

   const S32 y = offset.y + ( mRowHeight - S32( font->getHeight() ) ) / 2;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   const RectI saved = GFX->getClipRect();

   RectI clip( offset.x, offset.y, column.width, mRowHeight );

   if ( clip.intersect( saved ) )
   {
      GFX->setClipRect( clip );

      draw->setBitmapModulation( color );
      draw->drawTextN( font, Point2I( x, y ), text, length,
                       mAllowColorChars ? mProfile->mFontColors : NULL );
   }

   GFX->setClipRect( saved );
}

void ShellFancyTextList::clearList()
{
   mRows.clear();
   Parent::clearList();
}

void ShellFancyTextList::setSortColumn( S32 key )
{
   Parent::setSortColumn( key );
   sort( key, mSortIncreasing );
}

void ShellFancyTextList::addRow( S32 id, const char *text, S32 index )
{
   Row row;

   row.id = id;
   row.text = text ? text : "";
   row.style = -1;

   if ( index < 0 || index >= mRows.size() )
      mRows.push_back( row );
   else
      mRows.insert( index, row );

   setNumRows( mRows.size() );
}

void ShellFancyTextList::setRowById( S32 id, const char *text )
{
   const S32 index = getRowNumById( id );

   if ( index >= 0 )
   {
      mRows[ index ].text = text ? text : "";
      setUpdate();
   }
}

void ShellFancyTextList::removeRow( S32 index )
{
   if ( index < 0 || index >= mRows.size() )
      return;

   mRows.erase( index );
   setNumRows( mRows.size() );
}

void ShellFancyTextList::removeRowById( S32 id )
{
   removeRow( getRowNumById( id ) );
}

S32 ShellFancyTextList::getRowId( S32 index ) const
{
   return index >= 0 && index < mRows.size() ? mRows[ index ].id : -1;
}

S32 ShellFancyTextList::getRowNumById( S32 id ) const
{
   for ( U32 i = 0; i < mRows.size(); i++ )
      if ( mRows[ i ].id == id )
         return S32( i );

   return -1;
}

const char *ShellFancyTextList::getRowText( S32 index ) const
{
   return index >= 0 && index < mRows.size() ? mRows[ index ].text.c_str() : "";
}

const char *ShellFancyTextList::getRowTextById( S32 id ) const
{
   return getRowText( getRowNumById( id ) );
}

S32 ShellFancyTextList::findTextIndex( const char *text ) const
{
   if ( !text )
      return -1;

   for ( U32 i = 0; i < mRows.size(); i++ )
      if ( dStrcmp( mRows[ i ].text.c_str(), text ) == 0 )
         return S32( i );

   return -1;
}

S32 ShellFancyTextList::getSelectedId() const
{
   return getRowId( getSelectedRow() );
}

void ShellFancyTextList::setSelectedById( S32 id )
{
   setSelectedRow( getRowNumById( id ) );
}

void ShellFancyTextList::clearSelection()
{
   setSelectedRow( -1 );
}

bool ShellFancyTextList::addStyle( S32 id, const char *face, S32 size,
                                   const char *color, const char *colorHL,
                                   const char *colorSEL )
{
   for ( U32 i = 0; i < mStyles.size(); i++ )
      if ( mStyles[ i ].id == id )
         return false;

   mStyles.increment();

   Style &style = mStyles.last();
   style.id = id;

   if ( face && face[ 0 ] && size > 0 )
      style.font = GFont::create( face, size,
         Con::getVariable( "$GUI::fontCacheDirectory" ) );

   const char *given[ StateCount ] = { color, colorHL, colorSEL };

   for ( U32 i = 0; i < StateCount; i++ )
   {
      S32 r = 0, g = 0, b = 0, a = 255;

      const S32 read = given[ i ]
         ? dSscanf( given[ i ], "%d %d %d %d", &r, &g, &b, &a ) : -1;

      style.given[ i ] = read >= 3;
      style.color[ i ] = ColorI( r, g, b, read == 4 ? a : 255 );
   }

   return true;
}

S32 ShellFancyTextList::getRowStyle( S32 index ) const
{
   return index >= 0 && index < mRows.size() ? mRows[ index ].style : -1;
}

void ShellFancyTextList::setRowStyle( S32 index, S32 style )
{
   if ( index >= 0 && index < mRows.size() )
   {
      mRows[ index ].style = style;
      setUpdate();
   }
}

void ShellFancyTextList::sort( S32 key, bool increasing )
{
   S32 which = -1;
   U32 flags = 0;

   for ( U32 i = 0; i < mColumns.size(); i++ )
      if ( mColumns[ i ].key == key )
      {
         which = S32( i );
         flags = mColumns[ i ].flags;
         break;
      }

   if ( which < 0 || mRows.size() < 2 )
      return;

   const S32 chosen = getSelectedId();

   for ( U32 i = 1; i < mRows.size(); i++ )
   {
      Row row = mRows[ i ];

      S32 aLen = 0;
      const char *a = field( row.text.c_str(), which, aLen );

      S32 j = S32( i );

      while ( j > 0 )
      {
         S32 bLen = 0;
         const char *b = field( mRows[ j - 1 ].text.c_str(), which, bLen );

         S32 order;

         if ( flags & Numeric )
         {
            const F32 av = a ? dAtof( a ) : 0.0f;
            const F32 bv = b ? dAtof( b ) : 0.0f;

            order = av < bv ? -1 : av > bv ? 1 : 0;
         }
         else
         {
            order = dStrnicmp( a ? a : "", b ? b : "",
                               getMin( aLen, bLen ) );

            if ( !order )
               order = aLen - bLen;
         }

         if ( !increasing )
            order = -order;

         if ( order >= 0 )
            break;

         mRows[ j ] = mRows[ j - 1 ];
         j--;
      }

      mRows[ j ] = row;
   }

   if ( chosen != -1 )
      setSelectedById( chosen );

   setUpdate();
}

DefineEngineMethod( ShellFancyTextList, addRow, void,
   ( S32 id, const char *text, S32 index ), ( -1 ),
   "@brief Add a row, tabs between the columns.\n\n@ingroup GuiShell" )
{
   object->addRow( id, text, index );
}

DefineEngineMethod( ShellFancyTextList, setRowById, void, ( S32 id, const char *text ), ,
   "@brief Replace the text of the row with this id.\n\n@ingroup GuiShell" )
{
   object->setRowById( id, text );
}

DefineEngineMethod( ShellFancyTextList, removeRow, void, ( S32 index ), ,
   "@brief Take out the row at this index.\n\n@ingroup GuiShell" )
{
   object->removeRow( index );
}

DefineEngineMethod( ShellFancyTextList, removeRowById, void, ( S32 id ), ,
   "@brief Take out the row with this id.\n\n@ingroup GuiShell" )
{
   object->removeRowById( id );
}

DefineEngineMethod( ShellFancyTextList, clear, void, ( ), ,
   "@brief Take out every row.\n\n@ingroup GuiShell" )
{
   object->clearList();
}

DefineEngineMethod( ShellFancyTextList, getRowId, S32, ( S32 index ), ,
   "@brief The id of the row at this index.\n\n@ingroup GuiShell" )
{
   return object->getRowId( index );
}

DefineEngineMethod( ShellFancyTextList, getRowNumById, S32, ( S32 id ), ,
   "@brief Where the row with this id sits.\n\n@ingroup GuiShell" )
{
   return object->getRowNumById( id );
}

DefineEngineMethod( ShellFancyTextList, getRowText, const char *, ( S32 index ), ,
   "@brief The text of the row at this index.\n\n@ingroup GuiShell" )
{
   return object->getRowText( index );
}

DefineEngineMethod( ShellFancyTextList, getRowTextById, const char *, ( S32 id ), ,
   "@brief The text of the row with this id.\n\n@ingroup GuiShell" )
{
   return object->getRowTextById( id );
}

DefineEngineMethod( ShellFancyTextList, findTextIndex, S32, ( const char *text ), ,
   "@brief Where the row with exactly this text sits.\n\n@ingroup GuiShell" )
{
   return object->findTextIndex( text );
}

DefineEngineMethod( ShellFancyTextList, getSelectedId, S32, ( ), ,
   "@brief The id of the chosen row, or -1.\n\n@ingroup GuiShell" )
{
   return object->getSelectedId();
}

DefineEngineMethod( ShellFancyTextList, setSelectedById, void, ( S32 id ), ,
   "@brief Choose the row with this id.\n\n@ingroup GuiShell" )
{
   object->setSelectedById( id );
}

DefineEngineMethod( ShellFancyTextList, clearSelection, void, ( ), ,
   "@brief Leave no row chosen.\n\n@ingroup GuiShell" )
{
   object->clearSelection();
}

DefineEngineMethod( ShellFancyTextList, addStyle, bool,
   ( S32 id, const char *fontType, S32 fontSize, const char *color,
     const char *colorHL, const char *colorSEL ), ,
   "@brief Register a look rows can be put in.\n\n@ingroup GuiShell" )
{
   return object->addStyle( id, fontType, fontSize, color, colorHL, colorSEL );
}

DefineEngineMethod( ShellFancyTextList, getRowStyle, S32, ( S32 index ), ,
   "@brief The style the row at this index is in.\n\n@ingroup GuiShell" )
{
   return object->getRowStyle( index );
}

DefineEngineMethod( ShellFancyTextList, getRowStyleById, S32, ( S32 id ), ,
   "@brief The style the row with this id is in.\n\n@ingroup GuiShell" )
{
   return object->getRowStyle( object->getRowNumById( id ) );
}

DefineEngineMethod( ShellFancyTextList, setRowStyle, void, ( S32 index, S32 style ), ,
   "@brief Put the row at this index in a style.\n\n@ingroup GuiShell" )
{
   object->setRowStyle( index, style );
}

DefineEngineMethod( ShellFancyTextList, setRowStyleById, void, ( S32 id, S32 style ), ,
   "@brief Put the row with this id in a style.\n\n@ingroup GuiShell" )
{
   object->setRowStyle( object->getRowNumById( id ), style );
}

DefineEngineMethod( ShellFancyTextList, sort, void, ( S32 key, bool increasing ),
   ( -1, true ),
   "@brief Put the rows in the order of a column.\n\n@ingroup GuiShell" )
{
   object->sort( key == -1 ? object->getSortColumnKey() : key, increasing );
}
