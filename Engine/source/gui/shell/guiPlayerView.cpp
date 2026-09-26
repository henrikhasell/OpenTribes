#include "platform/platform.h"
#include "gui/shell/guiPlayerView.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "renderInstance/renderMeshMgr.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderTranslucentMgr.h"

IMPLEMENT_CONOBJECT( GuiPlayerView );

ConsoleDocClass( GuiPlayerView,
   "@brief The warrior on Tribes 2's Warrior Setup tab.\n\n"

   "Shows the player's shape in the skin they chose, armed with whatever "
   "their armour carries, and turns it on the spot the way the shell "
   "does.\n\n"

   "@ingroup GuiShell"
);

GuiPlayerView::GuiPlayerView()
{
   mLightColor.set( 0.9f, 0.9f, 0.9f );
   mLightAmbient.set( 0.5f, 0.5f, 0.5f );
   mLightDirection.set( 0.f, 0.7f, -0.7f );

   mCameraRot.set( 0.f, 0.f, 3.9f );
   mOrbitDist = 3.5f;
}

bool GuiPlayerView::onWake()
{
   if ( !Parent::onWake() )
      return false;

   if ( !mRenderPass )
   {
      RenderPassManager *pass = new RenderPassManager();
      pass->registerObject();
      pass->assignName( "ShellPlayerRenderPass" );

      RenderMeshMgr *mesh = new RenderMeshMgr( RenderPassManager::RIT_Mesh, 0.5f, 0.5f );
      mesh->registerObject();
      pass->addManager( mesh );

      RenderTranslucentMgr *translucent = new RenderTranslucentMgr();
      translucent->registerObject();
      pass->addManager( translucent );

      mRenderPass = pass;
   }

   return true;
}

void GuiPlayerView::onSleep()
{
   Parent::onSleep();

   if ( mRenderPass )
   {
      mRenderPass->deleteObject();
      mRenderPass = NULL;
   }
}

RenderPassManager *GuiPlayerView::getRenderPass() const
{
   return mRenderPass;
}

bool GuiPlayerView::processCameraQuery( CameraQuery *query )
{
   if ( mModel )
      mOrbitDist = mClampF( mOrbitDist, mModel->getShape()->radius, 5.f );

   if ( !Parent::processCameraQuery( query ) )
      return false;

   query->nearPlane = 0.1f;
   query->farPlane = 2100.f;
   query->fov = 0.8975979f;

   return true;
}

void GuiPlayerView::setModel( const char *shape, const char *skin )
{
   if ( !shape || !shape[ 0 ] )
      return;

   char path[ 256 ];
   dSprintf( path, sizeof( path ), "shapes/%s.dts", shape );

   setObjectModel( path );
   setSkin( skin ? skin : "base" );

   const char *weapon = dStrstr( shape, "heavy" ) ? "shapes/weapon_mortar.dts"
                      : dStrstr( shape, "medium" ) ? "shapes/weapon_plasma.dts"
                                                   : "shapes/weapon_disc.dts";

   setMountedObject( weapon );
   setMountNode( String( "mount0" ) );
}

void GuiPlayerView::setSeq( S32 index )
{
   setObjectAnimation( index );
}

DefineEngineMethod( GuiPlayerView, setModel, void,
   ( const char *shape, const char *skin ), ( "" ),
   "@brief Show this shape wearing this skin.\n\n@ingroup GuiShell" )
{
   object->setModel( shape, skin );
}

DefineEngineMethod( GuiPlayerView, setSeq, void, ( S32 index ), ,
   "@brief Play the shape's nth sequence.\n\n@ingroup GuiShell" )
{
   object->setSeq( index );
}
