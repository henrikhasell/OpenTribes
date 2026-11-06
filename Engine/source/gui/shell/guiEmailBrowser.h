// HH: Added for Tribes 2 compatibility.
#ifndef _GUIEMAILBROWSER_H_
#define _GUIEMAILBROWSER_H_

#ifndef _SHELLFANCYARRAY_H_
#include "gui/shell/shellFancyArray.h"
#endif

class GuiEmailBrowser : public ShellFancyArray
{
   typedef ShellFancyArray Parent;

public:
   enum ColumnKey
   {
      StatusColumn = 0,
      FromColumn,
      SubjectColumn,
      ReceivedColumn,
   };

   struct Row
   {
      S32 id;
      String from;
      String subject;
      String received;
      S32 flags;
   };

protected:
   Vector< Row > mRows;

   StringTableEntry mIconBase;
   StringTableEntry mUnreadFontType;
   S32 mUnreadFontSize;
   ColorI mUnreadFontColor;

   GFXTexHandle mReadIcon;
   GFXTexHandle mNotReadIcon;

   Resource< GFont > mUnreadFont;

   void onRenderRow( const Point2I &offset, const Column &column,
                     S32 row, bool selected, bool mouseOver );

public:
   GuiEmailBrowser();

   DECLARE_CONOBJECT( GuiEmailBrowser );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "Tribes 2's list of the messages in a mailbox." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void clearList();
   void setSelectedRow( S32 row );
   void setSortColumn( S32 key );

   void addRow( S32 id, const char *from, const char *subject,
                const char *received, S32 flags );
   void setRow( S32 id, const char *from, const char *subject,
                const char *received, S32 flags );
   void setRowFlags( S32 id, S32 flags );

   void removeRowByIndex( S32 index );
   void removeRowById( S32 id );

   S32 findById( S32 id ) const;
   void selectRowById( S32 id );
   S32 getSelectedId() const;

   void sort();
};

#endif // _GUIEMAILBROWSER_H_
