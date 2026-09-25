// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSCROLLCTRL_H_
#define _SHELLSCROLLCTRL_H_

#ifndef _GUISCROLLCTRL_H_
#include "gui/containers/guiScrollCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _SHELLSKIN_H_
#include "gui/shell/shellSkin.h"
#endif

class ShellScrollCtrl : public GuiScrollCtrl
{
   typedef GuiScrollCtrl Parent;

protected:
   enum
   {
      Resting = 0,
      Pressed = 1,
      Highlight = 2,
      Disabled = 3,

      StateCount = 4,
   };

   /// One direction's worth of art: the two arrows, the thumb's three pieces
   /// and the track's three, off three sheets that share a stem.
   struct Bar
   {
      GFXTexHandle buttons;
      GFXTexHandle thumb;
      GFXTexHandle track;

      RectI buttonCell[ 2 * StateCount ];
      RectI thumbCell[ 3 * StateCount ];
      RectI trackCell[ 3 * 2 ];

      bool ready;

      Bar() : ready( false ) {}

      const RectI &button( S32 piece, S32 state ) const
      {
         return buttonCell[ piece * StateCount + state ];
      }

      const RectI &thumbPiece( S32 piece, S32 state ) const
      {
         return thumbCell[ piece * StateCount + state ];
      }

      const RectI &trackPiece( S32 piece, bool enabled ) const
      {
         return trackCell[ piece * 2 + ( enabled ? 0 : 1 ) ];
      }
   };

   Bar mVert;
   Bar mHorz;

   GFXTexHandle mScaleTexture;
   RectI mScaleCell[ StateCount ];

   ShellSkin mFieldSkin;
   StringTableEntry mFieldBase;

   /// How far the arrow and thumb art hangs outside the track it runs in.
   S32 mOverhang;

   bool loadBar( Bar &bar, const char *stem, const char *which );

   S32 artState( Region region, bool enabled );

public:
   ShellScrollCtrl();

   DECLARE_CONOBJECT( ShellScrollCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell scroll container." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );

   void drawBorder( const Point2I &offset, bool isFirstResponder );
   void drawVScrollBar( const Point2I &offset );
   void drawHScrollBar( const Point2I &offset );
   void drawScrollCorner( const Point2I &offset );
};

#endif // _SHELLSCROLLCTRL_H_
