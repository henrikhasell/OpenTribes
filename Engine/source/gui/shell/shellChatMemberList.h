// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLCHATMEMBERLIST_H_
#define _SHELLCHATMEMBERLIST_H_

#ifndef _GUITEXTLISTCTRL_H_
#include "gui/controls/guiTextListCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellChatMemberList : public GuiTextListCtrl
{
   typedef GuiTextListCtrl Parent;

   enum
   {
      Normal = 0,
      Highlight = 1,
      Selected = 2,

      StateCount = 3,
   };

   enum
   {
      RunPlain = 0,
      RunNick = 1,
      RunTribe = 2,

      RunCount = 3,
   };

   /// A look a row can be put in: two badges and a colour per state.
   struct Style
   {
      S32 id;

      GFXTexHandle left;
      GFXTexHandle right;

      /// Which of the three colours script actually gave, by state.
      bool given[ StateCount ];
      ColorI color[ StateCount ];
   };

   Vector< Style > mStyles;

   /// Which style each row is in, by row.  Rows script never styled are -1.
   Vector< S32 > mRowStyle;

   GFXTexHandle mRollover;
   GFXTexHandle mSelectedBar;

   const Style *styleFor( S32 row ) const;

   /// The colour a run of the given kind takes in the given state.
   ColorI runColor( S32 run, S32 state, const Style *style ) const;

public:
   ShellChatMemberList();

   DECLARE_CONOBJECT( ShellChatMemberList );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The list of who is in a Tribes 2 chat channel." );

   bool onWake();
   void onSleep();

   void onRenderCell( Point2I offset, Point2I cell, bool selected, bool mouseOver );

   //-- what script drives -----------------------------------------------------
   bool addStyle( S32 id, const char *left, const char *right,
                  const char *color, const char *colorHL, const char *colorSEL );

   S32 getRowStyle( S32 row ) const;
   void setRowStyle( S32 row, S32 style );
};

#endif // _SHELLCHATMEMBERLIST_H_
