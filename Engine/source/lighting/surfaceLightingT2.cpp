// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "lighting/surfaceLightingT2.h"

#include "lighting/lightInfo.h"
#include "lighting/lightManager.h"
#include "scene/sceneObject.h"
#include "scene/sceneContainer.h"
#include "interior/interior.h"
#include "interior/interiorInstance.h"
#include "interior/interiorLMManager.h"
#include "terrain/terrData.h"
#include "T3D/objectTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/bitmap/gBitmap.h"

namespace
{
   /// How far up and down an interior is looked for (0x0079300c), and how
   /// far down a terrain is (0x00793014).
   const F32 InteriorRayLength = 100.0f;
   const F32 TerrainRayLength = 10.0f;

   /// How far the colour moves towards a new one every 100ms (0x00793020).
   const F32 ColorStep = 0.2f;

   /// The ambient light an interior lights a shape by: its direction and
   /// how the interior's colour is split between its diffuse and ambient
   /// (0x0058f902, 0x00792f50, 0x00792f54).
   const VectorF AmbientLightDir( 0.57735f, 0.57735f, -0.57735f );
   const F32 DirectionalFactor = 0.3f;
   const F32 AmbientFactor = 0.7f;

   /// The lights handed out for one frame's render instances, reused the
   /// next.
   Vector<LightInfo*> sgLights;
   U32 sgLightsUsed = 0;
   U32 sgFrame = 1;
   bool sgHooked = false;

   bool onDeviceEvent( GFXDevice::GFXDeviceEventType evt )
   {
      if ( evt == GFXDevice::deStartOfFrame )
      {
         sgLightsUsed = 0;
         sgFrame++;
      }
      return true;
   }

   LightInfo *allocLight()
   {
      if ( sgLightsUsed == sgLights.size() )
         sgLights.push_back( LIGHTMGR->createLightInfo() );
      return sgLights[sgLightsUsed++];
   }

   F32 mean( const ColorF &color )
   {
      return ( color.red + color.green + color.blue ) / 3.0f;
   }

   /// A lightmap's colour at a point of an interior, as its texture
   /// coordinates fall on the texel grid.
   ColorF lightmapColor( GBitmap *lightmap, const Interior::TexGenPlanes &texGen, const Point3F &point )
   {
      if ( !lightmap )
         return ColorF::WHITE;

      const S32 w = lightmap->getWidth();
      const S32 h = lightmap->getHeight();
      const S32 x = mClamp( (S32)mFloor( texGen.planeX.distToPlane( point ) * w ), 0, w - 1 );
      const S32 y = mClamp( (S32)mFloor( texGen.planeY.distToPlane( point ) * h ), 0, h - 1 );

      ColorI color;
      if ( !lightmap->getColor( x, y, color ) )
         return ColorF::WHITE;
      return ColorF( color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, 1.0f );
   }

   GBitmap *getLightmap( Interior *detail, InteriorInstance *instance, U32 index )
   {
      GFXTexHandle &handle = gInteriorLMManager.getHandle( detail->getLMHandle(), instance->getLMHandle(), index );
      GFXTextureObject *texture = handle.getPointer();
      if ( !texture || !texture->mProfile->doStoreBitmap() )
         return NULL;
      return texture->getBitmap();
   }
}

SurfaceLightingT2::SurfaceLightingT2()
   : mQueryPos( Point3F::Zero ),
     mQueried( false ),
     mUseInfo( false ),
     mSource( SourceInterior ),
     mDefaultColor( 0.5f, 0.5f, 0.5f ),
     mAlarmColor( 0.5f, 0.5f, 0.5f ),
     mHasLastColor( false ),
     mLastColor( 0.5f, 0.5f, 0.5f ),
     mLastTime( 0 ),
     mFrame( 0 ),
     mLight( NULL )
{
}

LightInfo* SurfaceLightingT2::getLight( SceneObject *obj )
{
   if ( !sgHooked )
   {
      GFXDevice::getDeviceEventSignal().notify( &onDeviceEvent );
      sgHooked = true;
   }

   // Once a frame, however many times the shape and its images are drawn.
   if ( mFrame == sgFrame )
      return mLight;
   mFrame = sgFrame;
   mLight = NULL;

   if ( !LIGHTMGR )
      return NULL;

   Point3F pos;
   obj->getRenderWorldBox().getCenter( &pos );
   if ( !mQueried || pos != mQueryPos )
   {
      _query( pos );
      mQueryPos = pos;
      mQueried = true;
   }

   ColorF color;
   const bool hasColor = _getColor( &color );

   if ( hasColor && mSource == SourceInterior )
   {
      // Put in the sun's place as the basic lighting shader takes a
      // directional light: far off against its direction.
      LightInfo *light = allocLight();
      light->setType( LightInfo::Ambient );
      light->setDirection( AmbientLightDir );
      light->setPosition( pos - AmbientLightDir * 10000.0f );
      light->setRange( 2000000.0f );
      light->setBrightness( 1.0f );
      light->setCastShadows( false );

      ColorF diffuse = color * DirectionalFactor;
      ColorF ambient = color * AmbientFactor;
      diffuse.alpha = ambient.alpha = 1.0f;
      light->setColor( diffuse );
      light->setAmbient( ambient );

      mLight = light;
      return mLight;
   }

   LightInfo *sun = LIGHTMGR->getSpecialLight( LightManager::slSunLightType, false );
   if ( !sun )
      return NULL;

   // Whatever the surface, the sun is cut by its own ambient: in full by a
   // terrain's shadow, by a third of it under no surface at all.
   const F32 attenuation = hasColor ? mClampF( mean( color ), 0.0f, 1.0f ) : 1.0f;
   const F32 factor = mClampF( attenuation - mean( sun->getAmbient() ), 0.0f, 1.0f );

   LightInfo *light = allocLight();
   light->set( sun );
   light->setColor( sun->getColor() * factor );

   mLight = light;
   return mLight;
}

void SurfaceLightingT2::_query( const Point3F &pos )
{
   mUseInfo = false;
   mInterior = NULL;

   RayInfo ray;

   // An interior counts only with one above as well as below.
   if ( gClientContainer.castRay( pos, pos + Point3F( 0, 0, InteriorRayLength ), InteriorObjectType, &ray ) )
   {
      if ( !gClientContainer.castRay( pos, pos - Point3F( 0, 0, InteriorRayLength ), InteriorObjectType, &ray ) )
         return;

      InteriorInstance *instance = dynamic_cast<InteriorInstance*>( ray.object );
      if ( instance && ray.face != U32( -1 ) )
         _sampleInterior( instance, ray );

      // Nothing found under it keeps the last colour.
      mSource = SourceInterior;
      mUseInfo = true;
      return;
   }

   if ( gClientContainer.castRay( pos, pos - Point3F( 0, 0, TerrainRayLength ), TerrainObjectType, &ray ) )
   {
      TerrainBlock *terrain = dynamic_cast<TerrainBlock*>( ray.object );
      if ( terrain && _sampleTerrain( terrain, ray ) )
      {
         mSource = SourceTerrain;
         mUseInfo = true;
      }
   }
}

void SurfaceLightingT2::_sampleInterior( InteriorInstance *instance, const RayInfo &ray )
{
   Interior *detail = instance->getDetailLevel( 0 );
   if ( !detail || ray.face >= detail->getSurfaceCount() )
      return;

   const Interior::Surface &surface = detail->getSurface( ray.face );

   Point3F point = ray.point;
   instance->getWorldTransform().mulP( point );
   point.convolveInverse( instance->getScale() );

   // Retail takes the lightmap's colour at each corner of the triangle of
   // the surface's strip the point is in, and blends them across it.
   for ( U32 i = 2; i < surface.windingCount; i++ )
   {
      U32 indices[3];
      Point3F corners[3];
      for ( U32 j = 0; j < 3; j++ )
      {
         indices[j] = surface.windingStart + i - 2 + j;
         corners[j] = detail->getPoint( detail->getWinding( indices[j] ) );
      }

      const VectorF v0 = corners[1] - corners[0];
      const VectorF v1 = corners[2] - corners[0];
      const VectorF v2 = point - corners[0];
      const F32 d00 = mDot( v0, v0 );
      const F32 d01 = mDot( v0, v1 );
      const F32 d11 = mDot( v1, v1 );
      const F32 d20 = mDot( v2, v0 );
      const F32 d21 = mDot( v2, v1 );
      const F32 denom = d00 * d11 - d01 * d01;
      if ( mIsZero( denom ) )
         continue;

      F32 weights[3];
      weights[1] = ( d11 * d20 - d01 * d21 ) / denom;
      weights[2] = ( d00 * d21 - d01 * d20 ) / denom;
      weights[0] = 1.0f - weights[1] - weights[2];

      const F32 eps = -0.001f;
      if ( weights[0] < eps || weights[1] < eps || weights[2] < eps )
         continue;

      const Interior::TexGenPlanes &texGen = detail->getLMTexGenEQ( ray.face );

      for ( U32 pass = 0; pass < 2; pass++ )
      {
         U32 index;
         ColorF *dest;
         if ( pass == 0 )
         {
            index = detail->getNormalLMapIndex( ray.face );
            dest = &mDefaultColor;
         }
         else if ( detail->hasAlarmState() )
         {
            mInterior = instance;
            index = detail->getAlarmLMapIndex( ray.face );
            dest = &mAlarmColor;
         }
         else
         {
            mAlarmColor = mDefaultColor;
            break;
         }

         GBitmap *lightmap = getLightmap( detail, instance, index );

         ColorF color( 0.0f, 0.0f, 0.0f, 1.0f );
         for ( U32 j = 0; j < 3; j++ )
            color += lightmapColor( lightmap, texGen, corners[j] ) * weights[j];

         color.clamp();
         color.alpha = 1.0f;
         *dest = color;
      }
      return;
   }
}

bool SurfaceLightingT2::_sampleTerrain( TerrainBlock *terrain, const RayInfo &ray )
{
   const GBitmap *lightmap = terrain->getLightMap();
   if ( !lightmap || !lightmap->getWidth() )
      return false;

   Point3F point = ray.point;
   terrain->getWorldTransform().mulP( point );

   // The texel under the point, the terrain repeating as it does.
   const S32 size = lightmap->getWidth();
   const F32 texelSize = terrain->getWorldBlockSize() / F32( size );
   S32 x = (S32)mFloor( point.x / texelSize ) % size;
   S32 y = (S32)mFloor( point.y / texelSize ) % size;
   if ( x < 0 ) x += size;
   if ( y < 0 ) y += size;

   ColorI texel;
   if ( !lightmap->getColor( x, y, texel ) )
      return false;

   // The lexel's own colour: the sun's ambient, and its colour by how
   // squarely the ground faces it where it reaches (0x0057e7b4), as the
   // terrain's lighting baked it.
   ColorF color( texel.red / 255.0f, texel.green / 255.0f, texel.blue / 255.0f, 1.0f );
   mDefaultColor = color;
   return true;
}

bool SurfaceLightingT2::_getColor( ColorF *out )
{
   if ( !mUseInfo )
   {
      mHasLastColor = false;
      return false;
   }

   InteriorInstance *interior = static_cast<InteriorInstance*>( mInterior.getObject() );
   ColorF color = interior && interior->inAlarmState() ? mAlarmColor : mDefaultColor;

   const U32 time = Platform::getVirtualMilliseconds();

   if ( mHasLastColor )
   {
      const F32 step = ( F32( time - mLastTime ) / 100.0f ) * ColorStep;

      F32 *dest = color;
      const F32 *last = mLastColor;
      for ( U32 i = 0; i < 3; i++ )
      {
         if ( dest[i] > last[i] )
            dest[i] = mClampF( last[i] + step, 0.0f, dest[i] );
         else if ( dest[i] < last[i] )
            dest[i] = mClampF( last[i] - step, dest[i], 1.0f );
      }
   }

   mHasLastColor = true;
   mLastColor = color;
   mLastTime = time;

   *out = color;
   return true;
}
