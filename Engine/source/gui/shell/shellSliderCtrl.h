// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSLIDERCTRL_H_
#define _SHELLSLIDERCTRL_H_

#ifndef _GUISLIDERCTRL_H_
#include "gui/controls/guiSliderCtrl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ShellSliderCtrl : public GuiSliderCtrl
{
   typedef GuiSliderCtrl Parent;

   enum
   {
      Resting = 0,
      Pressed = 1,
      Highlight = 2,

      ThumbStates = 4,
      TrackStates = 2,
   };

   enum
   {
      LeftCap = 0,
      Middle = 1,
      RightCap = 2,
   };

   GFXTexHandle mTrackTexture;
   GFXTexHandle mThumbTexture;

   RectI mTrackCell[ 3 * TrackStates ];
   RectI mThumbCell[ 3 * ThumbStates ];

   bool mSliced;

   bool mUsePlusMinus;

   /// How much room each end label takes, and both of them together.  Zero
   /// unless the slider is labelled.
   S32 mLabelWidth;

   /// How far the thumb art hangs over the track it runs in.
   S32 mInset;

   const RectI &trackPiece( S32 piece, bool enabled ) const
   {
      return mTrackCell[ piece * TrackStates + ( enabled ? 0 : 1 ) ];
   }

   const RectI &thumbPiece( S32 piece, S32 state ) const
   {
      return mThumbCell[ piece * ThumbStates + state ];
   }

   S32 thumbState() const;

public:
   ShellSliderCtrl();

   DECLARE_CONOBJECT( ShellSliderCtrl );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "A Tribes 2 shell slider." );

   static void initPersistFields();

   bool onWake();
   void onSleep();

   void onRender( Point2I offset, const RectI &updateRect );
};

#endif // _SHELLSLIDERCTRL_H_
