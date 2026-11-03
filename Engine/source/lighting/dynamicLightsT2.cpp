// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "lighting/dynamicLightsT2.h"

#include "lighting/lightInfo.h"
#include "lighting/lightManager.h"
#include "lighting/lightQuery.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxDevice.h"

namespace
{
   /// Copies of fading lights with their colour dimmed, handed out for one
   /// frame's render instances and reused the next.
   Vector<LightInfo*> sgFaded;
   U32 sgFadedUsed = 0;
   bool sgHooked = false;

   bool onDeviceEvent( GFXDevice::GFXDeviceEventType evt )
   {
      if ( evt == GFXDevice::deStartOfFrame )
         sgFadedUsed = 0;
      return true;
   }

   LightInfo *fadedCopy( LightInfo *light, F32 factor )
   {
      if ( !sgHooked )
      {
         GFXDevice::getDeviceEventSignal().notify( &onDeviceEvent );
         sgHooked = true;
      }

      if ( sgFadedUsed == sgFaded.size() )
         sgFaded.push_back( LIGHTMGR->createLightInfo() );

      LightInfo *copy = sgFaded[sgFadedUsed++];
      copy->set( light );
      copy->setColor( light->getColor() * factor );
      return copy;
   }
}

U32 DynamicLightsT2::gather( const SceneRenderState *state,
                             const SphereF &bounds,
                             F32 clipPix,
                             F32 fadePix,
                             LightInfo **outLights,
                             U32 maxLights )
{
   if ( !LIGHTMGR )
      return 0;

   LightQuery query( 8 );
   query.init( bounds );

   LightInfo *lights[8] = { NULL };
   query.getLights( lights, 8 );

   const Point3F &camPos = state->getCameraPosition();
   const F32 worldToScreen = state->getWorldToScreenScale().x;

   U32 count = 0;
   for ( U32 i = 0; i < 8 && lights[i] && count < maxLights; i++ )
   {
      LightInfo *light = lights[i];
      if ( light->getType() != LightInfo::Point && light->getType() != LightInfo::Spot )
         continue;

      // dglProjectRadius (0x004475f0): the light's radius in pixels at its
      // distance from the camera.
      const F32 dist = ( light->getPosition() - camPos ).len();
      const F32 pixels = dist > 0.0f ? light->getRange().x / dist * worldToScreen : F32_MAX;
      if ( pixels <= clipPix )
         continue;

      if ( pixels < fadePix )
      {
         const F32 factor = 1.0f - ( fadePix - pixels ) / ( fadePix - clipPix );
         light = fadedCopy( light, factor );
      }

      outLights[count++] = light;
   }

   return count;
}
