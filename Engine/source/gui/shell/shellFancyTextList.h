// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLFANCYTEXTLIST_H_
#define _SHELLFANCYTEXTLIST_H_

#ifndef _SHELLFANCYARRAY_H_
#include "gui/shell/shellFancyArray.h"
#endif

class ShellFancyTextList : public ShellFancyArray
{
   typedef ShellFancyArray Parent;

   enum
   {
      Resting = 0,
      Highlight = 1,
      Selected = 2,

      StateCount = 3,
   };

   struct Row
   {
      S32 id;
      String text;
      S32 style;
   };

   struct Style
   {
      S32 id;

      Resource< GFont > font;

      bool given[ StateCount ];
      ColorI color[ StateCount ];
   };

   Vector< Row > mRows;
   Vector< Style > mStyles;

   bool mAllowColorChars;

   Style *styleFor( S32 row );

   static const char *field( const char *text, S32 index, S32 &length );

protected:
   void onRenderRow( const Point2I &offset, const Column &column,
                     S32 row, bool selected, bool mouseOver );

public:
   ShellFancyTextList();

   DECLARE_CONOBJECT( ShellFancyTextList );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 browser list with columns of text." );

   static void initPersistFields();

   void clearList();
   void setSortColumn( S32 key );

   void addRow( S32 id, const char *text, S32 index = -1 );
   void setRowById( S32 id, const char *text );

   void removeRow( S32 index );
   void removeRowById( S32 id );

   S32 getRowId( S32 index ) const;
   S32 getRowNumById( S32 id ) const;

   const char *getRowText( S32 index ) const;
   const char *getRowTextById( S32 id ) const;

   S32 findTextIndex( const char *text ) const;

   S32 getSelectedId() const;
   void setSelectedById( S32 id );
   void clearSelection();

   bool addStyle( S32 id, const char *face, S32 size, const char *color,
                  const char *colorHL, const char *colorSEL );

   S32 getRowStyle( S32 index ) const;
   void setRowStyle( S32 index, S32 style );

   void sort( S32 key, bool increasing );
};

#endif // _SHELLFANCYTEXTLIST_H_
