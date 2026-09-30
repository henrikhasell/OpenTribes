// HH: Added for Tribes 2 compatibility.
#ifndef _GUIPLAYERVIEW_H_
#define _GUIPLAYERVIEW_H_

#ifndef _GUIOBJECTVIEW_H_
#include "T3D/guiObjectView.h"
#endif

class RenderPassManager;

class GuiPlayerView : public GuiObjectView
{
   typedef GuiObjectView Parent;

   SimObjectPtr< RenderPassManager > mRenderPass;

protected:
   RenderPassManager *getRenderPass() const;

public:
   GuiPlayerView();

   DECLARE_CONOBJECT( GuiPlayerView );
   DECLARE_CATEGORY( "Gui Shell" );
   DECLARE_DESCRIPTION( "The warrior on Tribes 2's Warrior Setup tab." );

   bool onWake();
   void onSleep();

   bool processCameraQuery( CameraQuery *query );

   void setModel( const char *shape, const char *skin );

   void setSeq( S32 index );
};

#endif // _GUIPLAYERVIEW_H_
