#include "platform/platform.h"
#include "gui/shell/shellFancyArray.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxTrack.h"

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

   mScrollOffset.set( 0, 0 );

   mHeaderBitmap = StringTable->insert( "gui/server_tabs" );
   mSortArrowBitmap = StringTable->insert( "gui/shll_sortarrow" );
   mBarBase = StringTable->insert( "gui/shll_bar" );

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
   mMinColumnWidth = 0;
   mFitColumns = false;
   mFitPending = false;
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

   const char *face = mHeaderFontType && mHeaderFontType[ 0 ]
      ? mHeaderFontType : mProfile->mFontType;

   const S32 size = mHeaderFontSize ? mHeaderFontSize : mProfile->mFontSize;

   if ( face && face[ 0 ] && size > 0 )
      mHeaderFont = GFont::create( face, size, Con::getVariable( "$GUI::fontCacheDirectory" ) );

   mHeaderHeight = mHeaderSliced ? mHeaderCell[ 0 ].extent.y
                 : mHeader ? S32( mHeader->getBitmapHeight() ) : 0;

   mMinColumnWidth = mHeaderSliced
      ? mHeaderCell[ 0 ].extent.x + mHeaderCell[ 6 ].extent.x - mGlowOffset * 2 : 0;

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
   const S32 top = mRowHeight * mStartScrollRegion.y + mHeaderHeight - mGlowOffset;

   return RectI( 0, top, getWidth(), getHeight() - top );
}

bool ShellFancyArray::columnRegion( RectI &region ) const
{
   region.point.set( mGlowOffset, 0 );
   region.extent.set( getWidth() - mGlowOffset, getHeight() );

   if ( mStartScrollRegion.x >= S32( mColumns.size() ) )
      return false;

   region.point.x += pinnedWidth();
   region.extent.x = getWidth() - region.point.x;

   return region.extent.x > 0;
}

Point2I ShellFancyArray::contentSize() const
{
   Point2I size( 0, 0 );

   for ( S32 i = mStartScrollRegion.x; i < S32( mColumns.size() ); i++ )
      size.x += mColumns[ i ].width;

   if ( mRowCount > mStartScrollRegion.y )
      size.y = ( mRowCount - mStartScrollRegion.y ) * mRowHeight;

   return size;
}

Point2I ShellFancyArray::barOrigin() const
{
   RectI columns;
   columnRegion( columns );

   return Point2I( columns.point.x, rowRegion().point.y );
}

S32 ShellFancyArray::pinnedWidth() const
{
   const S32 pinned = getMin( S32( mColumns.size() ), mStartScrollRegion.x );

   S32 width = 0;

   for ( S32 i = 0; i < pinned; i++ )
      width += mColumns[ i ].width;

   return width;
}

S32 ShellFancyArray::columnAt( S32 x ) const
{
   RectI columns;
   const bool scrolls = columnRegion( columns );

   S32 running = mGlowOffset;

   for ( S32 i = 0; i < S32( mColumns.size() ); i++ )
   {
      if ( scrolls && i == mStartScrollRegion.x )
         running -= mScrollOffset.x;

      const S32 from = i < mStartScrollRegion.x
         ? running : getMax( running, columns.point.x );

      if ( x >= from && x < running + mColumns[ i ].width )
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

   const S32 row = ( local.y - rows.point.y + mScrollOffset.y ) / mRowHeight;

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
   const S32 span = getWidth() - mGlowOffset - left - right;

   const S32 foot = S32( mFrame[ BotLeft ]->getBitmapHeight() );

   const S32 room = getHeight() - mHeaderHeight + mGlowOffset;
   const S32 head = getMin( room - foot, S32( mFrame[ TopLeft ]->getBitmapHeight() ) );
   const S32 body = room - head - foot;

   const S32 top = offset.y + mHeaderHeight - mGlowOffset;

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

void ShellFancyArray::drawHeader( const Point2I &offset, S32 first, S32 last, S32 x )
{
   if ( !mHeaderHeight )
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   GFont *font = mHeaderFont ? (GFont *)mHeaderFont : mProfile->mFont;

   if ( !font )
      return;

   const S32 inside = mHeaderHeight - mGlowOffset * 2;

   const S32 top = offset.y + mGlowOffset;
   const S32 y = top + ( inside - S32( font->getHeight() ) ) / 2;

   for ( S32 i = first; i < last; i++ )
   {
      const Column &column = mColumns[ i ];

      if ( mHeaderSliced )
      {
         const S32 state = S32( i ) == mMouseOverColumn ? 2 : 0;

         const S32 cap = mHeaderCell[ state ].extent.x;
         const S32 end = mHeaderCell[ 6 + state ].extent.x;

         const S32 span = column.width + mGlowOffset * 2 - 1 - cap - end;

         const S32 left = x - mGlowOffset;

         draw->clearBitmapModulation();
         draw->drawBitmapSR( mHeader, Point2I( left, offset.y ), mHeaderCell[ state ] );

         if ( span > 0 )
            draw->drawBitmapStretchSR( mHeader,
               RectI( left + cap, offset.y, span, mHeaderHeight ),
               mHeaderCell[ 3 + state ] );

         draw->drawBitmapSR( mHeader, Point2I( left + cap + span, offset.y ),
                             mHeaderCell[ 6 + state ] );
      }

      if ( column.flags & HeaderIcon )
      {
         if ( column.icon )
         {
            const S32 room = column.width - S32( column.icon->getBitmapWidth() );

            draw->drawBitmap( column.icon,
               Point2I( x + ( room > 8 ? room / 2 : 4 ),
                        top + ( inside - S32( column.icon->getBitmapHeight() ) ) / 2 ) );
         }
      }
      else if ( column.name && column.name[ 0 ] )
      {
         const S32 width = S32( font->getStrWidth( column.name ) );
         const S32 room = column.width - width;

         S32 at = x + ( room > 8 ? room / 2 : 4 );

         if ( column.flags & AlignLeft )
            at = x + 2;
         else if ( column.flags & AlignRight )
            at = x + column.width - width - 2;

         draw->setBitmapModulation( S32( i ) == mMouseOverColumn
            ? mHeaderFontColorHL : mHeaderFontColor );

         draw->drawText( font, Point2I( at, y ), column.name, mProfile->mFontColors );
      }

      if ( mSortArrow && column.key == mSortColumn )
      {
         const S32 arrowW = S32( mSortArrow->getBitmapWidth() );
         const S32 arrowH = S32( mSortArrow->getBitmapHeight() );

         draw->clearBitmapModulation();
         draw->drawBitmap( mSortArrow,
            Point2I( x + column.width - arrowW - 6,
                     top + ( inside - arrowH ) / 2 ),
            mSortIncreasing ? GFXBitmapFlip_Y : GFXBitmapFlip_None );
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
      draw->drawLine( Point2I( offset.x, offset.y + mRowHeight ),
                      Point2I( offset.x + column.width, offset.y + mRowHeight ),
                      mSeparatorColor );
}

void ShellFancyArray::drawColumns( const Point2I &offset, S32 first, S32 last,
                                   S32 x, const RectI &clip )
{
   if ( first >= last )
      return;

   GFX->setClipRect( clip );

   drawHeader( offset, first, last, x );

   const RectI rows = rowRegion();

   RectI rowClip( offset + rows.point, rows.extent );

   if ( mRowHeight < 1 || !rowClip.intersect( clip ) )
      return;

   GFX->setClipRect( rowClip );

   const S32 from = mScrollOffset.y / mRowHeight;
   const S32 to = getMin( mRowCount,
      from + ( rows.extent.y + mRowHeight - 1 ) / mRowHeight + 1 );

   for ( S32 row = from; row < to; row++ )
   {
      Point2I at( x, offset.y + rows.point.y + row * mRowHeight - mScrollOffset.y );

      const bool selected = row == mSelectedRow;
      const bool mouseOver = row == mMouseOverRow;

      for ( S32 i = first; i < last; i++ )
      {
         onRenderRow( at, mColumns[ i ], row, selected, mouseOver );
         at.x += mColumns[ i ].width;
      }
   }
}

void ShellFancyArray::onRender( Point2I offset, const RectI &updateRect )
{
   if ( mFitPending )
   {
      mFitPending = false;
      fitColumns();
   }

   drawFrame( offset );

   const S32 pinned = getMin( S32( mColumns.size() ), mStartScrollRegion.x );

   drawColumns( offset, 0, pinned, offset.x + mGlowOffset, updateRect );

   RectI region;

   if ( columnRegion( region ) )
   {
      RectI clip( offset + region.point, region.extent );

      if ( clip.intersect( updateRect ) )
         drawColumns( offset, mStartScrollRegion.x, S32( mColumns.size() ),
                      offset.x + region.point.x - mScrollOffset.x, clip );
   }

   GFX->setClipRect( updateRect );

   renderChildControls( offset, updateRect );
}

void ShellFancyArray::onMouseDown( const GuiEvent &event )
{
   const Point2I local = globalToLocalCoord( event.mousePoint );

   // HH: Modified for Tribes 2 compatibility.
   if ( mActive && local.y < mHeaderHeight && columnAt( local.x ) >= 0 )
   {
      if ( SFXTrack *down = mProfile->getSoundButtonDown() )
         SFX->playOnce( down );
   }

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
      onCellClicked( row, columnAt( local.x ) );

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
      // HH: Modified for Tribes 2 compatibility.
      if ( mActive && column >= 0 && column != mMouseOverColumn )
      {
         if ( SFXTrack *over = mProfile->getSoundButtonOver() )
            SFX->playOnce( over );
      }

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
   column.name = StringTable->insert( name ? name : "", true );
   column.width = getMin( getMax( width, getMax( mMinColumnWidth, minWidth ) ), maxWidth );
   column.minWidth = minWidth;
   column.maxWidth = maxWidth;
   column.flags = 0;

   if ( flags )
   {
      if ( dStrstr( flags, "center" ) )
         column.flags |= AlignCenter;
      if ( dStrstr( flags, "right" ) )
         column.flags |= AlignRight;
      if ( dStrstr( flags, "left" ) )
         column.flags |= AlignLeft;
      if ( dStrstr( flags, "numeric" ) )
         column.flags |= Numeric;
      if ( dStrstr( flags, "headericon" ) )
         column.flags |= HeaderIcon;
   }

   if ( column.flags & HeaderIcon )
   {
      char icon[ 256 ];
      dSprintf( icon, sizeof( icon ), "gui/%s", column.name );

      column.icon.set( icon, &GFXDefaultPersistentProfile,
                       avar( "%s() - icon", __FUNCTION__ ) );
   }

   if ( mFitColumns )
   {
      mStartScrollRegion.x = mColumns.size();
      mFitPending = true;
   }

   setUpdate();
}

void ShellFancyArray::fitColumns()
{
   if ( mColumns.empty() )
      return;

   S32 slack = getWidth() - mGlowOffset;

   for ( U32 i = 0; i < mColumns.size(); i++ )
      slack -= mColumns[ i ].width;

   if ( slack == 0 )
      return;

   if ( slack > 0 )
   {
      Column &last = mColumns.last();

      last.width += slack;
      last.maxWidth = getMax( last.maxWidth, last.width );

      return;
   }

   for ( S32 i = S32( mColumns.size() ) - 1; i >= 0; i-- )
   {
      Column &column = mColumns[ i ];

      column.width += slack;

      if ( column.width >= column.minWidth )
         break;

      slack = column.width - column.minWidth;
      column.width = column.minWidth;
   }
}

void ShellFancyArray::fitScrollColumns()
{
   RectI region;

   if ( !columnRegion( region ) )
      return;

   const S32 first = mStartScrollRegion.x;
   const S32 last = S32( mColumns.size() );

   S32 total = 0;

   for ( S32 i = first; i < last; i++ )
      total += mColumns[ i ].width;

   if ( total >= region.extent.x )
      return;

   const S32 share = ( region.extent.x - total ) / ( last - first );

   S32 running = 0;

   for ( S32 i = first; i < last - 1; i++ )
   {
      Column &column = mColumns[ i ];

      column.width += share;
      column.maxWidth = getMax( column.maxWidth, column.width );

      running += column.width;
   }

   Column &tail = mColumns.last();

   tail.width = region.extent.x - running;
   tail.maxWidth = getMax( tail.maxWidth, tail.width );
   tail.minWidth = getMin( tail.minWidth, tail.width );

   setUpdate();
}

bool ShellFancyArray::resize( const Point2I &position, const Point2I &extent )
{
   const bool changed = Parent::resize( position, extent );

   if ( mFitColumns )
      mFitPending = true;

   return changed;
}

void ShellFancyArray::clearColumns()
{
   mColumns.clear();
   mFitPending = mFitColumns;
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
   mScrollOffset.set( 0, 0 );

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

void ShellFancyArray::setScrollOffset( const Point2I &offset )
{
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

   const S32 top = ( row - mStartScrollRegion.y ) * mRowHeight;
   const S32 height = rowRegion().extent.y;

   S32 offset = mScrollOffset.y;

   if ( top < offset )
      offset = top;
   else if ( top + mRowHeight > offset + height )
      offset = top + mRowHeight - height;

   setScrollOffset( Point2I( mScrollOffset.x,
      mClamp( offset, 0, getMax( 0, contentSize().y - height ) ) ) );
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
