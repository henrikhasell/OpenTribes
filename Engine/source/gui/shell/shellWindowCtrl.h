// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLWINDOWCTRL_H_
#define _SHELLWINDOWCTRL_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellWindowCtrl : public GuiTextCtrl
{
   typedef GuiTextCtrl Parent;

   enum
   {
      TitleCap = 0,
      TitleMiddle = 1,
   };

   enum
   {
      Idle = 0,
      SizingWidth = 1,
      SizingHeight = 2,
      Moving = 4,
      Closing = 8,
   };

   GFXTexHandle mTitleTexture;
   GFXTexHandle mCloseTexture;
   GFXTexHandle mCornerTexture;
   GFXTexHandle mFillTexture;
   GFXTexHandle mGradeTexture;

   RectI mTitleCell[ 2 ];
   RectI mCloseCell[ 3 ];

   SimObjectPtr< GuiCursor > mArrowCursor;
   SimObjectPtr< GuiCursor > mLeftRightCursor;
   SimObjectPtr< GuiCursor > mUpDownCursor;
   SimObjectPtr< GuiCursor > mDiagonalCursor;

   StringTableEntry mFrameBase;

   StringTableEntry mCloseCommand;

   bool mCanMove;
   bool mCanClose;

   S32 mBorderWidth;

   bool mSliced;

   S32 mTitleHeight;

   S32 mSizingMargin;

   S32 mMode;
   bool mCloseOver;

   Point2I mMouseDownPosition;
   RectI mOrigBounds;

   RectI mCloseRect;
   S32 mCloseInset;

   bool loadCursors();

   S32 sizingAt( const Point2I &local ) const;

   void selectWindow();

public:
   ShellWindowCtrl();

   DECLARE_CONOBJECT( ShellWindowCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell window." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   bool resize( const Point2I &position, const Point2I &extent );

   void onMouseDown( const GuiEvent &event );
   void onMouseDragged( const GuiEvent &event );
   void onMouseUp( const GuiEvent &event );
   void onMouseMove( const GuiEvent &event );
   void onMouseEnter( const GuiEvent &event );
   void onMouseLeave( const GuiEvent &event );

   void getCursor( GuiCursor *&cursor, bool &showCursor, const GuiEvent &event );

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLWINDOWCTRL_H_
