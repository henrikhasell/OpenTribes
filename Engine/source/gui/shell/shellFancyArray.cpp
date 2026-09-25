#include "platform/platform.h"
#include "gui/shell/shellFancyArray.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellFancyArray );

ConsoleDocClass( ShellFancyArray,
   "@brief The column machinery behind Tribes 2's browsers.\n\n"

   "Holds the columns, the header and the frame, and walks the rows calling "
   "onRenderRow for each cell.  What a cell contains is the deriving class's "
   "-- Tribes 2 never puts one of these in a .gui on its own.\n\n"

   "@ingroup GuiShell"
);

static const char *smFrameSuffix[ 9 ] =
{
   "_TL", "_TM", "_TR",
   "_ML", "_MM", "_MR",
   "_BL", "_BM", "_BR"
};

ShellFancyArray::ShellFancyArray()
{
   mRowCount = 0;
   mSelectedRow = -1;
   mMouseOverRow = -1;
   mMouseOverColumn = -1;

   mSortColumn = -1;
   mSecondarySortColumn = -1;
   mSortIncreasing = true;
   mSecondarySortIncreasing = true;

   mScrollOffset = 0;

   mHeaderBitmap = StringTable->EmptyString();
   mSortArrowBitmap = StringTable->EmptyString();
   mBarBase = StringTable->EmptyString();
   mFieldBase = StringTable->EmptyString();

   mRowHeight = 16;
   mGlowOffset = 0;

   mStartScrollRegion.set( 0, 0 );

   mHeaderFontType = StringTable->EmptyString();
   mHeaderFontSize = 0;

   mHeaderFontColor.set( 0, 0, 0 );
   mHeaderFontColorHL.set( 255, 255, 255 );
   mSeparatorColor.set( 192, 192, 192 );

   mDrawSeparators = false;
   mHeaderSort = false;
   mAllowReposition = false;
   mNoSelect = false;

   mHeaderHeight = 0;
   mFramed = false;
   mHeaderSliced = false;
}

void ShellFancyArray::initPersistFields()
{
   addField( "headerBitmap", TypeString, Offset( mHeaderBitmap, ShellFancyArray ),
      "Art laid across the row of column names." );
   addField( "sortArrowBitmap", TypeString, Offset( mSortArrowBitmap, ShellFancyArray ),
      "Art marking which column the rows are in the order of." );
   addField( "barBase", TypeString, Offset( mBarBase, ShellFancyArray ),
      "Stem for the bars behind the row under the mouse and the chosen one." );
   addField( "fieldBase", TypeString, Offset( mFieldBase, ShellFancyArray ),
      "Stem for the nine pieces of the frame around the rows." );
   addField( "rowHeight", TypeS32, Offset( mRowHeight, ShellFancyArray ),
      "How tall one row is." );
   addField( "glowOffset", TypeS32, Offset( mGlowOffset, ShellFancyArray ),
      "How far inside the control the frame is drawn." );
   addField( "startScrollRegion", TypePoint2I, Offset( mStartScrollRegion, ShellFancyArray ),
      "Where the rows begin, measured from inside the frame." );
   addField( "headerFontType", TypeString, Offset( mHeaderFontType, ShellFancyArray ),
      "Typeface for the column names.  Falls back to the profile's." );
   addField( "headerFontSize", TypeS32, Offset( mHeaderFontSize, ShellFancyArray ),
      "Size for the column names.  Zero takes the profile's." );
   addField( "headerFontColor", TypeColorI, Offset( mHeaderFontColor, ShellFancyArray ),
      "Colour of a column name." );
   addField( "headerFontColorHL", TypeColorI, Offset( mHeaderFontColorHL, ShellFancyArray ),
      "Colour of the column name the mouse is over." );
   addField( "separatorColor", TypeColorI, Offset( mSeparatorColor, ShellFancyArray ),
      "Colour of the line between two columns." );
   addField( "drawSeparators", TypeBool, Offset( mDrawSeparators, ShellFancyArray ),
      "Rule a line between the columns." );
   addField( "headerSort", TypeBool, Offset( mHeaderSort, ShellFancyArray ),
      "Let a click on a column name sort by it." );
   addField( "allowReposition", TypeBool, Offset( mAllowReposition, ShellFancyArray ),
      "Let a column be dragged to a new width." );
   addField( "noSelect", TypeBool, Offset( mNoSelect, ShellFancyArray ),
      "Draw the rows but never choose one." );

   Parent::initPersistFields();
}

bool ShellFancyArray::onWake()
{
   if ( !Parent::onWake() )
      return false;

   char name[ 512 ];

   if ( mFieldBase && mFieldBase[ 0 ] )
   {
      mFramed = true;

      for ( U32 i = 0; i < FrameCount; i++ )
      {
         dSprintf( name, sizeof( name ), "%s%s", mFieldBase, smFrameSuffix[ i ] );
         mFrame[ i ].set( name, &GFXDefaultPersistentProfile,
                          avar( "%s() - frame", __FUNCTION__ ) );

         if ( !mFrame[ i ] )
            mFramed = false;
      }
   }

   if ( mBarBase && mBarBase[ 0 ] )
   {
      dSprintf( name, sizeof( name ), "%s_act", mBarBase );
      mBarSelected.set( name, &GFXDefaultPersistentProfile,
                        avar( "%s() - selected", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_rol", mBarBase );
      mBarRollover.set( name, &GFXDefaultPersistentProfile,
                        avar( "%s() - rollover", __FUNCTION__ ) );
   }

   if ( mHeaderBitmap && mHeaderBitmap[ 0 ] )
   {
      mHeader.set( mHeaderBitmap, &GFXDefaultPersistentProfile,
                   avar( "%s() - header", __FUNCTION__ ) );

      mHeaderSliced = mHeader &&
         shellSliceGrid( mHeader->getBitmap(), mHeaderCell, 3, 3 ) == 3;
   }

   if ( mSortArrowBitmap && mSortArrowBitmap[ 0 ] )
      mSortArrow.set( mSortArrowBitmap, &GFXDefaultPersistentProfile,
                      avar( "%s() - arrow", __FUNCTION__ ) );

   // A header font of its own, or the profile's where the .gui left it out --
   // headerFontSize of zero is how the shipped content asks for that.
   const char *face = mHeaderFontType && mHeaderFontType[ 0 ]
      ? mHeaderFontType : mProfile->mFontType;

   const S32 size = mHeaderFontSize ? mHeaderFontSize : mProfile->mFontSize;

   if ( face && face[ 0 ] && size > 0 )
      mHeaderFont = GFont::create( face, size, Con::getVariable( "$GUI::fontCacheDirectory" ) );

   mHeaderHeight = mHeaderSliced ? mHeaderCell[ 0 ].extent.y
                 : mHeader ? S32( mHeader->getBitmapHeight() ) : 0;

   return true;
}

void ShellFancyArray::onSleep()
{
   Parent::onSleep();

   for ( U32 i = 0; i < FrameCount; i++ )
      mFrame[ i ] = NULL;

   mBarSelected = NULL;
   mBarRollover = NULL;
   mHeader = NULL;
   mSortArrow = NULL;
   mHeaderSliced = false;
   mHeaderFont = NULL;

   mFramed = false;
}

RectI ShellFancyArray::rowRegion() const
{
   const S32 left = mFramed ? S32( mFrame[ TopLeft ]->getBitmapWidth() ) : 0;
   const S32 right = mFramed ? S32( mFrame[ TopRight ]->getBitmapWidth() ) : 0;
   const S32 foot = mFramed ? S32( mFrame[ BotLeft ]->getBitmapHeight() ) : 0;

   const S32 top = mHeaderHeight + mGlowOffset + mStartScrollRegion.y;

   return RectI( mGlowOffset + left + mStartScrollRegion.x, top,
                 getWidth() - mGlowOffset * 2 - left - right - mStartScrollRegion.x,
                 getHeight() - top - foot - mGlowOffset );
}

S32 ShellFancyArray::columnAt( S32 x ) const
{
   S32 running = rowRegion().point.x;

   for ( U32 i = 0; i < mColumns.size(); i++ )
   {
      if ( x >= running && x < running + mColumns[ i ].width )
         return i;

      running += mColumns[ i ].width;
   }

   return -1;
}

S32 ShellFancyArray::rowAt( const Point2I &local ) const
{
   const RectI rows = rowRegion();

   if ( mRowHeight < 1 || !rows.pointInRect( local ) )
      return -1;

   const S32 row = ( local.y - rows.point.y + mScrollOffset ) / mRowHeight;

   return row >= 0 && row < mRowCount ? row : -1;
}

void ShellFancyArray::drawFrame( const Point2I &offset )
{
   if ( !mFramed )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const S32 left = S32( mFrame[ TopLeft ]->getBitmapWidth() );
   const S32 right = S32( mFrame[ TopRight ]->getBitmapWidth() );
   const S32 span = getWidth() - mGlowOffset * 2 - left - right;

   const S32 head = S32( mFrame[ TopLeft ]->getBitmapHeight() );
   const S32 foot = S32( mFrame[ BotLeft ]->getBitmapHeight() );

   const S32 top = offset.y + mHeaderHeight + mGlowOffset;
   const S32 body = getHeight() - mHeaderHeight - mGlowOffset * 2 - head - foot;

   const S32 x = offset.x + mGlowOffset;

   for ( U32 row = 0; row < 3; row++ )
   {
      const S32 h = row == 0 ? head : row == 1 ? body : foot;
      const S32 y = top + ( row == 0 ? 0 : row == 1 ? head : head + body );

      if ( h <= 0 )
         continue;

      draw->drawBitmapStretch( mFrame[ row * 3 ], RectI( x, y, left, h ) );

      if ( span > 0 )
         draw->drawBitmapStretch( mFrame[ row * 3 + 1 ], RectI( x + left, y, span, h ) );

      draw->drawBitmapStretch( mFrame[ row * 3 + 2 ],
         RectI( x + left + span, y, right, h ) );
   }
}

void ShellFancyArray::drawHeader( const Point2I &offset )
{
   if ( !mHeaderHeight )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   GFont *font = mHeaderFont ? (GFont *)mHeaderFont : mProfile->mFont;

   if ( !font )
      return;

   S32 x = offset.x + rowRegion().point.x;

   const S32 y = offset.y + ( mHeaderHeight - S32( font->getHeight() ) ) / 2;

   for ( U32 i = 0; i < mColumns.size(); i++ )
   {
      const Column &column = mColumns[ i ];

      // Each column wears its own tab: a cap at either end and the middle
      // stretched between them, out of a sheet three pieces by three states.
      if ( mHeaderSliced )
      {
         const S32 state = column.key == mSortColumn ? 1
                           : S32( i ) == mMouseOverColumn ? 2 : 0;

         const S32 cap = mHeaderCell[ 0 ].extent.x;
         const S32 span = column.width - cap * 2;

         draw->clearBitmapModulation();
         draw->drawBitmapSR( mHeader, Point2I( x, offset.y ), mHeaderCell[ state ] );

         if ( span > 0 )
            draw->drawBitmapStretchSR( mHeader,
               RectI( x + cap, offset.y, span, mHeaderHeight ),
               mHeaderCell[ 3 + state ] );

         draw->drawBitmapSR( mHeader, Point2I( x + cap + span, offset.y ),
                             mHeaderCell[ 6 + state ] );
      }

      if ( column.flags & HeaderIcon )
      {
         if ( column.icon )
            draw->drawBitmap( column.icon,
               Point2I( x + ( column.width - S32( column.icon->getBitmapWidth() ) ) / 2,
                        offset.y + ( mHeaderHeight - S32( column.icon->getBitmapHeight() ) ) / 2 ) );
      }
      else if ( column.name && column.name[ 0 ] )
      {
         const S32 width = S32( font->getStrWidth( column.name ) );

         S32 at = x + 2;

         if ( column.flags & AlignCenter )
            at = x + ( column.width - width ) / 2;
         else if ( column.flags & AlignRight )
            at = x + column.width - width - 2;

         draw->setBitmapModulation( S32( i ) == mSortColumn
            ? mHeaderFontColorHL : mHeaderFontColor );

         draw->drawText( font, Point2I( at, y ), column.name, mProfile->mFontColors );
      }

      // The arrow marks the column the rows are in the order of, and which
      // way round that order runs.
      if ( mSortArrow && column.key == mSortColumn )
      {
         const S32 arrowW = S32( mSortArrow->getBitmapWidth() );
         const S32 arrowH = S32( mSortArrow->getBitmapHeight() );

         draw->clearBitmapModulation();
         draw->drawBitmap( mSortArrow,
            Point2I( x + column.width - arrowW - 2,
                     offset.y + ( mHeaderHeight - arrowH ) / 2 ),
            mSortIncreasing ? GFXBitmapFlip_None : GFXBitmapFlip_Y );
      }

      x += column.width;
   }
}

void ShellFancyArray::onRenderRow( const Point2I &offset, const Column &column,
                                   S32 row, bool selected, bool mouseOver )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   GFXTexHandle bar = selected ? mBarSelected : mouseOver ? mBarRollover : GFXTexHandle();

   if ( bar )
      draw->drawBitmapStretch( bar,
         RectI( offset.x, offset.y, column.width, mRowHeight ) );

   if ( mDrawSeparators )
      draw->drawLine( Point2I( offset.x + column.width, offset.y ),
                      Point2I( offset.x + column.width, offset.y + mRowHeight ),
                      mSeparatorColor );
}

void ShellFancyArray::onRender( Point2I offset, const RectI &updateRect )
{
   drawFrame( offset );
   drawHeader( offset );

   const RectI rows = rowRegion();

   RectI clip( offset + rows.point, rows.extent );

   if ( mRowHeight > 0 && clip.intersect( updateRect ) )
   {
      GFX->setClipRect( clip );

      const S32 first = mScrollOffset / mRowHeight;
      const S32 last = getMin( mRowCount,
         first + ( rows.extent.y + mRowHeight - 1 ) / mRowHeight + 1 );

      for ( S32 row = first; row < last; row++ )
      {
         Point2I at( offset.x + rows.point.x,
                     offset.y + rows.point.y + row * mRowHeight - mScrollOffset );

         const bool selected = row == mSelectedRow;
         const bool mouseOver = row == mMouseOverRow;

         for ( U32 i = 0; i < mColumns.size(); i++ )
         {
            onRenderRow( at, mColumns[ i ], row, selected, mouseOver );
            at.x += mColumns[ i ].width;
         }
      }
   }

   renderChildControls( offset, updateRect );
}

void ShellFancyArray::onMouseDown( const GuiEvent &event )
{
   const Point2I local = globalToLocalCoord( event.mousePoint );

   // A click on the header is a click on the order, not on a row.
   if ( mHeaderSort && local.y < mHeaderHeight )
   {
      const S32 column = columnAt( local.x );

      if ( column >= 0 )
      {
         if ( mColumns[ column ].key == mSortColumn )
            setSortIncreasing( !mSortIncreasing );
         else
            setSortColumn( mColumns[ column ].key );
      }

      return;
   }

   if ( mNoSelect )
      return;

   const S32 row = rowAt( local );

   if ( row >= 0 )
   {
      setSelectedRow( row );

      if ( event.mouseClickCount > 1 )
         execAltConsoleCallback();
      else
         execConsoleCallback();
   }
}

void ShellFancyArray::onMouseMove( const GuiEvent &event )
{
   const Point2I local = globalToLocalCoord( event.mousePoint );

   const S32 row = rowAt( local );
   const S32 column = local.y < mHeaderHeight ? columnAt( local.x ) : -1;

   if ( row != mMouseOverRow || column != mMouseOverColumn )
   {
      mMouseOverRow = row;
      mMouseOverColumn = column;
      setUpdate();
   }
}

void ShellFancyArray::onMouseLeave( const GuiEvent & )
{
   if ( mMouseOverRow != -1 || mMouseOverColumn != -1 )
   {
      mMouseOverRow = -1;
      mMouseOverColumn = -1;
      setUpdate();
   }
}

void ShellFancyArray::addColumn( S32 key, const char *name, S32 width,
                                 S32 minWidth, S32 maxWidth, const char *flags )
{
   mColumns.increment();

   Column &column = mColumns.last();

   column.key = key;
   column.name = StringTable->insert( name ? name : "" );
   column.width = width;
   column.minWidth = minWidth;
   column.maxWidth = maxWidth;
   column.flags = 0;

   if ( flags )
   {
      if ( dStrstr( flags, "center" ) )
         column.flags |= AlignCenter;
      if ( dStrstr( flags, "right" ) )
         column.flags |= AlignRight;
      if ( dStrstr( flags, "numeric" ) )
         column.flags |= Numeric;
      if ( dStrstr( flags, "headericon" ) )
         column.flags |= HeaderIcon;
   }

   // A header-icon column names art rather than text.
   if ( column.flags & HeaderIcon )
      column.icon.set( column.name, &GFXDefaultPersistentProfile,
                       avar( "%s() - icon", __FUNCTION__ ) );

   setUpdate();
}

void ShellFancyArray::clearColumns()
{
   mColumns.clear();
   setUpdate();
}

S32 ShellFancyArray::getColumnKey( S32 index ) const
{
   return index >= 0 && index < mColumns.size() ? mColumns[ index ].key : -1;
}

S32 ShellFancyArray::getColumnWidth( S32 index ) const
{
   return index >= 0 && index < mColumns.size() ? mColumns[ index ].width : 0;
}

void ShellFancyArray::setNumRows( S32 rows )
{
   mRowCount = getMax( rows, 0 );

   if ( mSelectedRow >= mRowCount )
      mSelectedRow = -1;

   setUpdate();
}

void ShellFancyArray::addRow()
{
   setNumRows( mRowCount + 1 );
}

void ShellFancyArray::clearList()
{
   mRowCount = 0;
   mSelectedRow = -1;
   mMouseOverRow = -1;
   mScrollOffset = 0;

   setUpdate();
}

void ShellFancyArray::setSelectedRow( S32 row )
{
   mSelectedRow = row >= 0 && row < mRowCount ? row : -1;
   setUpdate();
}

void ShellFancyArray::setSortColumn( S32 key )
{
   mSortColumn = key;
   setUpdate();
}

void ShellFancyArray::setSortIncreasing( bool increasing )
{
   mSortIncreasing = increasing;
   setUpdate();
}

void ShellFancyArray::setSecondarySortColumn( S32 key )
{
   mSecondarySortColumn = key;
   setUpdate();
}

void ShellFancyArray::setSecondarySortIncreasing( bool increasing )
{
   mSecondarySortIncreasing = increasing;
   setUpdate();
}

void ShellFancyArray::setScrollOffset( S32 offset )
{
   const S32 room = getMax( 0, contentHeight() - rowRegion().extent.y );

   offset = mClamp( offset, 0, room );

   if ( offset != mScrollOffset )
   {
      mScrollOffset = offset;
      setUpdate();
   }
}

void ShellFancyArray::scrollVisible( S32 row )
{
   if ( row < 0 || row >= mRowCount || mRowHeight < 1 )
      return;

   const S32 top = row * mRowHeight;
   const S32 height = rowRegion().extent.y;

   if ( top < mScrollOffset )
      setScrollOffset( top );
   else if ( top + mRowHeight > mScrollOffset + height )
      setScrollOffset( top + mRowHeight - height );
}

void ShellFancyArray::forceUpdate()
{
   setUpdate();
}

DefineEngineMethod( ShellFancyArray, addColumn, void,
   ( S32 key, const char *name, S32 width, S32 minWidth, S32 maxWidth,
     const char *flags ), ( "" ),
   "@brief Add a column.  flags may name center, right, numeric or headericon.\n\n"
   "@ingroup GuiShell" )
{
   object->addColumn( key, name, width, minWidth, maxWidth, flags );
}

DefineEngineMethod( ShellFancyArray, clearColumns, void, ( ), ,
   "@brief Throw away every column.\n\n@ingroup GuiShell" )
{
   object->clearColumns();
}

DefineEngineMethod( ShellFancyArray, getNumColumns, S32, ( ), ,
   "@brief How many columns there are.\n\n@ingroup GuiShell" )
{
   return object->getNumColumns();
}

DefineEngineMethod( ShellFancyArray, getColumnKey, S32, ( S32 index ), ,
   "@brief The key the column at this index was added under.\n\n@ingroup GuiShell" )
{
   return object->getColumnKey( index );
}

DefineEngineMethod( ShellFancyArray, getColumnWidth, S32, ( S32 index ), ,
   "@brief How wide the column at this index is.\n\n@ingroup GuiShell" )
{
   return object->getColumnWidth( index );
}

DefineEngineMethod( ShellFancyArray, addRow, void, ( ), ,
   "@brief Make room for one more row.\n\n@ingroup GuiShell" )
{
   object->addRow();
}

DefineEngineMethod( ShellFancyArray, setNumRows, void, ( S32 rows ), ,
   "@brief Make room for this many rows.\n\n@ingroup GuiShell" )
{
   object->setNumRows( rows );
}

DefineEngineMethod( ShellFancyArray, rowCount, S32, ( ), ,
   "@brief How many rows there are.\n\n@ingroup GuiShell" )
{
   return object->getRowCount();
}

DefineEngineMethod( ShellFancyArray, clearList, void, ( ), ,
   "@brief Throw away every row.\n\n@ingroup GuiShell" )
{
   object->clearList();
}

DefineEngineMethod( ShellFancyArray, getSelectedRow, S32, ( ), ,
   "@brief Which row is chosen, or -1.\n\n@ingroup GuiShell" )
{
   return object->getSelectedRow();
}

DefineEngineMethod( ShellFancyArray, setSelectedRow, void, ( S32 row ), ,
   "@brief Choose a row.\n\n@ingroup GuiShell" )
{
   object->setSelectedRow( row );
}

DefineEngineMethod( ShellFancyArray, getSortColumnKey, S32, ( ), ,
   "@brief The key of the column the rows are in the order of.\n\n@ingroup GuiShell" )
{
   return object->getSortColumnKey();
}

DefineEngineMethod( ShellFancyArray, setSortColumn, void, ( S32 key ), ,
   "@brief Put the rows in the order of this column.\n\n@ingroup GuiShell" )
{
   object->setSortColumn( key );
}

DefineEngineMethod( ShellFancyArray, getSortIncreasing, bool, ( ), ,
   "@brief Whether that order runs upwards.\n\n@ingroup GuiShell" )
{
   return object->getSortIncreasing();
}

DefineEngineMethod( ShellFancyArray, setSortIncreasing, void, ( bool increasing ), ,
   "@brief Run the order upwards or down.\n\n@ingroup GuiShell" )
{
   object->setSortIncreasing( increasing );
}

DefineEngineMethod( ShellFancyArray, getSecondarySortColumnKey, S32, ( ), ,
   "@brief The key of the column that breaks ties.\n\n@ingroup GuiShell" )
{
   return object->getSecondarySortColumnKey();
}

DefineEngineMethod( ShellFancyArray, setSecondarySortColumn, void, ( S32 key ), ,
   "@brief Break ties with this column.\n\n@ingroup GuiShell" )
{
   object->setSecondarySortColumn( key );
}

DefineEngineMethod( ShellFancyArray, getSecondarySortIncreasing, bool, ( ), ,
   "@brief Whether the tie-break runs upwards.\n\n@ingroup GuiShell" )
{
   return object->getSecondarySortIncreasing();
}

DefineEngineMethod( ShellFancyArray, setSecondarySortIncreasing, void, ( bool increasing ), ,
   "@brief Run the tie-break upwards or down.\n\n@ingroup GuiShell" )
{
   object->setSecondarySortIncreasing( increasing );
}

DefineEngineMethod( ShellFancyArray, scrollVisible, void, ( S32 row ), ,
   "@brief Scroll until this row can be seen.\n\n@ingroup GuiShell" )
{
   object->scrollVisible( row );
}

DefineEngineMethod( ShellFancyArray, forceUpdate, void, ( ), ,
   "@brief Redraw on the next pass.\n\n@ingroup GuiShell" )
{
   object->forceUpdate();
}
