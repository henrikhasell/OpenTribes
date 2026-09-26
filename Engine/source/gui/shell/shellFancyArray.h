// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLFANCYARRAY_H_
#define _SHELLFANCYARRAY_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

class ShellFancyArray : public GuiControl
{
   typedef GuiControl Parent;

public:
   enum ColumnFlags
   {
      AlignCenter = BIT( 0 ),
      AlignRight = BIT( 1 ),
      Numeric = BIT( 2 ),
      HeaderIcon = BIT( 3 ),
      AlignLeft = BIT( 4 ),
   };

   struct Column
   {
      S32 key;
      StringTableEntry name;

      S32 width;
      S32 minWidth;
      S32 maxWidth;

      U32 flags;

      GFXTexHandle icon;
   };

protected:
   enum
   {
      TopLeft = 0, TopMiddle, TopRight,
      MidLeft, MidMiddle, MidRight,
      BotLeft, BotMiddle, BotRight,

      FrameCount,
   };

   Vector< Column > mColumns;

   S32 mRowCount;
   S32 mSelectedRow;
   S32 mMouseOverRow;
   S32 mMouseOverColumn;

   S32 mSortColumn;
   S32 mSecondarySortColumn;
   bool mSortIncreasing;
   bool mSecondarySortIncreasing;

   S32 mScrollOffset;

   StringTableEntry mHeaderBitmap;
   StringTableEntry mSortArrowBitmap;
   StringTableEntry mBarBase;
   StringTableEntry mFieldBase;

   S32 mRowHeight;
   S32 mGlowOffset;

   Point2I mStartScrollRegion;

   StringTableEntry mHeaderFontType;
   S32 mHeaderFontSize;

   ColorI mHeaderFontColor;
   ColorI mHeaderFontColorHL;
   ColorI mSeparatorColor;

   bool mDrawSeparators;
   bool mHeaderSort;
   bool mAllowReposition;
   bool mNoSelect;

   GFXTexHandle mFrame[ FrameCount ];
   GFXTexHandle mBarSelected;
   GFXTexHandle mBarRollover;
   GFXTexHandle mHeader;
   GFXTexHandle mSortArrow;

   RectI mHeaderCell[ 3 * 3 ];
   bool mHeaderSliced;

   Resource< GFont > mHeaderFont;

   S32 mHeaderHeight;

   S32 mMinColumnWidth;

   bool mFitColumns;

   bool mFitPending;

   bool mFramed;

   RectI rowRegion() const;

   void fitColumns();

   S32 columnAt( S32 x ) const;
   S32 rowAt( const Point2I &local ) const;

   void drawFrame( const Point2I &offset );
   void drawHeader( const Point2I &offset );

   virtual void onRenderRow( const Point2I &offset, const Column &column,
                             S32 row, bool selected, bool mouseOver );

   virtual void onCellClicked( S32 row, S32 column ) { }

public:
   ShellFancyArray();

   DECLARE_CONOBJECT( ShellFancyArray );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The column machinery behind Tribes 2's browsers." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   bool resize( const Point2I &position, const Point2I &extent );

   void onRender( Point2I offset, const RectI &updateRect );

   void setFitColumns( bool fit ) { mFitColumns = fit; }

   void onMouseDown( const GuiEvent &event );
   void onMouseMove( const GuiEvent &event );
   void onMouseLeave( const GuiEvent &event );

   void addColumn( S32 key, const char *name, S32 width, S32 minWidth,
                   S32 maxWidth, const char *flags );
   void clearColumns();

   S32 getNumColumns() const { return mColumns.size(); }
   S32 getColumnKey( S32 index ) const;
   S32 getColumnWidth( S32 index ) const;

   S32 getRowCount() const { return mRowCount; }
   void setNumRows( S32 rows );
   void addRow();
   virtual void clearList();

   S32 getSelectedRow() const { return mSelectedRow; }
   virtual void setSelectedRow( S32 row );

   S32 getSortColumnKey() const { return mSortColumn; }
   bool getSortIncreasing() const { return mSortIncreasing; }
   S32 getSecondarySortColumnKey() const { return mSecondarySortColumn; }
   bool getSecondarySortIncreasing() const { return mSecondarySortIncreasing; }

   virtual void setSortColumn( S32 key );
   void setSortIncreasing( bool increasing );
   void setSecondarySortColumn( S32 key );
   void setSecondarySortIncreasing( bool increasing );

   void scrollVisible( S32 row );
   virtual void forceUpdate();

   S32 contentHeight() const { return mRowCount * mRowHeight; }

   S32 getRowHeight() const { return mRowHeight; }
   S32 getHeaderHeight() const { return mHeaderHeight; }
   S32 getGlowOffset() const { return mGlowOffset; }

   S32 headerSpan() const
      { return mRowHeight * mStartScrollRegion.y + mHeaderHeight - mGlowOffset * 2; }

   S32 pinnedWidth() const;

   void setScrollOffset( S32 offset );
   S32 getScrollOffset() const { return mScrollOffset; }
};

#endif // _SHELLFANCYARRAY_H_
