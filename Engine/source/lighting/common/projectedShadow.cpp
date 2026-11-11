//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "lighting/common/projectedShadow.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxVertexBuffer.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/bitmap/gBitmap.h"
#include "T3D/shapeBase.h"
#include "T3D/player.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "ts/tsMesh.h"
#include "ts/tsRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderMeshMgr.h"
#include "materials/shaderData.h"
#include "materials/materialManager.h"
#include "lighting/shadowMap/shadowMatHook.h"
#include "console/console.h"
#include "console/engineAPI.h"


SimObjectPtr<RenderPassManager> ProjectedShadow::smRenderPass = NULL;

// HH: Added for Tribes 2 compatibility.
// Tribes 2's Shadow statics, at full detail until the scripts set a level.
F32 ProjectedShadow::smGlobalShadowDetail = 1.0f;
F32 ProjectedShadow::smShapeDetailScale = 1.0f;
S32 ProjectedShadow::smShapeDetailMin = 2;
F32 ProjectedShadow::smSmallestVisibleSize = 6.0f;
F32 ProjectedShadow::smLightDirSkew = 0.0f;
F32 ProjectedShadow::smLightLenSkew = 0.0f;
F32 ProjectedShadow::smGenericRadiusSkew = 0.4f;

// HH: Added for Tribes 2 compatibility.
// A shadow falls on terrain and interiors only (0x007b2554).
U32 ProjectedShadow::smShadowMask = TerrainObjectType | InteriorObjectType;
DepthSortList ProjectedShadow::smDepthSortList;
Box3F ProjectedShadow::smShadowBox;
SphereF ProjectedShadow::smShadowSphere;

// HH: Added for Tribes 2 compatibility.
// Tribes 2's tables (0x007b256c): from 100m out a shadow turns toward
// straight down and shortens.
const ProjectedShadow::DistanceDetail ProjectedShadow::smDistanceDetails[] =
{
   {    0.0f, 0.0f, 0.0f },
   {  100.0f, 0.0f, 0.0f },
   {  500.0f, 1.0f, 0.5f },
   { 1000.0f, 1.0f, 0.7f }
};

// (0x007b259c): the size of the outline, whether it is blurred and how often
// it is redrawn, by the shape's size on screen. Under 10 pixels the shadow is
// the generic blob.
const ProjectedShadow::PixelSizeDetail ProjectedShadow::smPixelSizeDetails[] =
{
   { 130.0f,  25, 64, true,  false },
   {  25.0f, 100, 64, true,  false },
   {  10.0f, 100, 32, false, false },
   {   0.0f,   0,  0, false, true  }
};

// HH: Added for Tribes 2 compatibility.
// Tribes 2's Shadow::setGlobalShadowDetailLevel (0x00652350).
void ProjectedShadow::setGlobalShadowDetailLevel( F32 level )
{
   smGlobalShadowDetail = level;

   smShapeDetailScale = 0.5f + level * 0.5f;
   smShapeDetailMin = level > 0.49f ? 2 : 3;
   smSmallestVisibleSize = level > 0.7f ? 6.0f : ( level > 0.4f ? 7.0f :
                           ( level > 0.3f ? 8.0f : ( level > 0.2f ? 9.0f : 10.0f ) ) );
   smLightDirSkew = 0.85f * ( 1.0f - level );
   smLightLenSkew = ( 1.0f - level ) * 0.5f;
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2's setShadowDetailLevel (0x005f2220), which the options dialog's
// "Shadow Detail" slider and console_start.cs call.
DefineEngineFunction( setShadowDetailLevel, void, ( F32 level ),,
   "@brief Set the shadow detail level, from 0 to 1, and $pref::Shadows.\n\n"
   "@ingroup Lighting" )
{
   level = mClampF( level, 0.0f, 1.0f );

   if ( mFabs( ProjectedShadow::smGlobalShadowDetail - level ) < 0.001f )
      return;

   ProjectedShadow::setGlobalShadowDetailLevel( level );
   Con::setFloatVariable( "$pref::Shadows", level );
}


GFX_ImplementTextureProfile( BLProjectedShadowProfile,
                              GFXTextureProfile::DiffuseMap,
                              GFXTextureProfile::PreserveSize | 
                              GFXTextureProfile::RenderTarget |
                              GFXTextureProfile::Pooled,
                              GFXTextureProfile::None );

GFX_ImplementTextureProfile( BLProjectedShadowZProfile,
                              GFXTextureProfile::DiffuseMap,
                              GFXTextureProfile::PreserveSize | 
                              GFXTextureProfile::ZTarget |
                              GFXTextureProfile::Pooled,
                              GFXTextureProfile::None );


ProjectedShadow::ProjectedShadow( SceneObject *object )
{
   mParentObject = object;
   mShapeBase = dynamic_cast<ShapeBase*>( object );

   mRadius = 0.0f;
   mLightToWorld.identity();
   mInvShadowDistance = 1.0f;

   mUseGeneric = false;
   mNeedOutline = false;
   mOutlineDim = 0;
   mOutlineBlur = false;
   mOutlineTime = 0;

   mLastRenderTime = 0;
   mScore = 0.0f;

   mBlurModelViewSC = NULL;
   mBlurTexelSizeSC = NULL;
   mBlurSC = NULL;
}

ProjectedShadow::~ProjectedShadow()
{
   mShadowTexture = NULL;
   mGenericTexture = NULL;
   mRenderTarget = NULL;
}

/// How much of the fog lies between the eye and a point, as Tribes 2's
/// getHazeAndFog (0x00570b40) has it: the haze that thickens out to the
/// visible distance and the share of each fog volume the line crosses.
static F32 hazeAndFog( const FogData &fog, const Point3F &eye, const Point3F &at )
{
   const F32 dist = ( at - eye ).len();

   const F32 t = mClampF( ( dist - fog.densityOffset ) * fog.hazeScale, 0.0f, 1.0f ) - 1.0f;
   F32 fogged = 1.0f - t * t;

   const F32 dz = at.z - eye.z;
   for ( U32 i = 0; i < FogData::MaxBands; i++ )
   {
      if ( fog.bandFactor[i] == 0.0f )
         continue;

      const F32 lo = fog.bandMin[i];
      const F32 hi = fog.bandMax[i];
      F32 frac;
      if ( mFabs( dz ) > 1e-4f )
         frac = ( mClampF( at.z, lo, hi ) - mClampF( eye.z, lo, hi ) ) / dz;
      else
         frac = ( eye.z >= lo && eye.z <= hi ) ? 1.0f : 0.0f;
      fogged += dist * frac * fog.bandFactor[i];
   }

   return mClampF( fogged, 0.0f, 1.0f );
}

void ProjectedShadow::_findDistanceDetail( F32 dist, DistanceDetail *dd )
{
   const S32 count = sizeof( smDistanceDetails ) / sizeof( DistanceDetail );
   for ( S32 i = 0; i < count; i++ )
   {
      if ( dist > smDistanceDetails[i].dist )
         continue;

      if ( i == 0 )
         *dd = smDistanceDetails[0];
      else
      {
         const DistanceDetail &lo = smDistanceDetails[i - 1];
         const DistanceDetail &hi = smDistanceDetails[i];
         const F32 t = ( dist - lo.dist ) / ( hi.dist - lo.dist );
         dd->dist = dist;
         dd->directionSkew = lo.directionSkew + t * ( hi.directionSkew - lo.directionSkew );
         dd->lengthSkew = lo.lengthSkew + t * ( hi.lengthSkew - lo.lengthSkew );
      }
      return;
   }

   *dd = smDistanceDetails[count - 1];
}

const ProjectedShadow::PixelSizeDetail* ProjectedShadow::_findPixelSizeDetail( F32 pixelSize )
{
   const S32 count = sizeof( smPixelSizeDetails ) / sizeof( PixelSizeDetail );
   for ( S32 i = 0; i < count - 1; i++ )
      if ( pixelSize >= smPixelSizeDetails[i].size )
         return &smPixelSizeDetails[i];

   return &smPixelSizeDetails[count - 1];
}

bool ProjectedShadow::_castsShadow()
{
   if ( !mShapeBase || !mShapeBase->getShapeInstance() || !mParentObject->getSceneManager() )
      return false;

   if ( mShapeBase->isHidden() )
      return false;

   // HH: Added for Tribes 2 compatibility.
   // Only a player, a flying vehicle and a wheeled vehicle turn their
   // shadow on (0x005ed9cb tests it); an item, a hover vehicle or a static
   // shape casts none whatever its datablock's shadow levels.
   if ( !mShapeBase->generatesShadowT2() )
      return false;

   const ShapeBaseData *data = static_cast<ShapeBaseData*>( mShapeBase->getDataBlock() );
   if ( !data || smGlobalShadowDetail < data->noShadowLevel )
      return false;

   if ( mShapeBase->getDamageState() == ShapeBase::Destroyed && !data->renderWhenDestroyed )
      return false;

   // A shape with nothing opaque casts no shadow (0x005eda42).
   const TSShape *shape = mShapeBase->getShapeInstance()->getShape();
   if ( shape->subShapeFirstTranslucentObject.empty() || shape->subShapeFirstTranslucentObject[0] == 0 )
      return false;

   // Nor does one that is cloaked, riding something, faded out, or the
   // player's own when it is not drawn in first person (0x005dd215).
   if ( mShapeBase->getCloakLevel() != 0.0f || mShapeBase->isMounted() )
      return false;

   if ( mShapeBase->getFadeVal() < TSMesh::VISIBILITY_EPSILON )
      return false;

   Player *player = dynamic_cast<Player*>( mShapeBase );
   if ( player && player->isHiddenInFirstPerson() )
      return false;

   return true;
}

bool ProjectedShadow::_prepare( const SceneRenderState *state )
{
   if ( !_castsShadow() )
      return false;

   TSShapeInstance *shapeInst = mShapeBase->getShapeInstance();
   const TSShape *shape = shapeInst->getShape();
   const ShapeBaseData *data = static_cast<ShapeBaseData*>( mShapeBase->getDataBlock() );
   const Point3F &scale = mParentObject->getScale();

   // Below the class's generic level the shadow is the blob, moved halfway
   // down toward the shape's feet (0x005edb2a).
   const bool generic = smGlobalShadowDetail < data->genericShadowLevel;

   // The shadow is centred on the shape's centre, moved across with the
   // shadow node as it leaves the bounds (0x005edb5a).
   Point3F pos = shape->center;
   if ( generic )
      pos *= 0.5f;

   const S32 node = data->shadowNode;
   if ( node >= 0 && node < shapeInst->mNodeTransforms.size() )
   {
      Point3F offset = shapeInst->mNodeTransforms[node].getPosition() - shape->defaultTranslations[node];
      pos.x += offset.x;
      pos.y += offset.y;
   }

   pos.convolve( scale );
   mParentObject->getRenderTransform().mulP( pos );

   // Half the diagonal of the shape's bounds (0x00652b90).
   Point3F halfExtents = shape->bounds.getExtents() * 0.5f;
   halfExtents.convolve( scale );
   mRadius = halfExtents.len();

   const Point3F &camPos = state->getCameraPosition();
   const Point3F &objPos = mParentObject->getRenderPosition();
   const F32 dist = ( objPos - camPos ).len();
   F32 fog = hazeAndFog( state->getSceneManager()->getFogData(), camPos, objPos );

   // A shadow too small on screen is dropped, and one nearly so fades into a
   // pseudo fog (0x00652e70).
   const F32 maxScale = getMax( scale.x, getMax( scale.y, scale.z ) );
   const F32 scaledDist = getMax( dist / maxScale, 0.001f );
   const F32 width = state->getViewport().extent.x;
   const F32 pixelSize = ( shape->radius / scaledDist ) * state->getWorldToScreenScale().x *
                         ( width / 640.0f ) * TSShapeInstance::smDetailAdjust;
   const F32 smallest = getMax( smSmallestVisibleSize, shape->mSmallestVisibleSize );
   if ( pixelSize * smShapeDetailScale < smallest )
      return false;

   F32 pseudoFog = smallest / ( pixelSize * smShapeDetailScale );
   pseudoFog = pseudoFog > 0.5f ? 2.0f * ( pseudoFog - 0.5f ) : 0.0f;
   if ( fog < pseudoFog )
   {
      if ( pseudoFog >= 0.99f )
         return false;
      fog = pseudoFog;
   }

   DistanceDetail dd;
   _findDistanceDetail( dist, &dd );
   const PixelSizeDetail *psd = _findPixelSizeDetail( pixelSize );

   // The light is fixed, not the sun (0x005edc8a), and turns toward straight
   // down with distance and a lower detail level (0x00652ff0).
   VectorF lightDir( 0.57f, 0.57f, -0.57f );
   const F32 dirMult = ( 1.0f - dd.directionSkew ) * ( 1.0f - smLightDirSkew );
   if ( dirMult < 0.99f )
   {
      lightDir.z *= dirMult;
      lightDir.z -= 1.0f - dirMult;
   }
   lightDir.normalize();

   // Ten times the shape's radius, shortened the same way (0x005edadd).
   const F32 shadowLen = 10.0f * shape->radius *
                         ( 1.0f - dd.lengthSkew ) * ( 1.0f - smLightLenSkew );

   mUseGeneric = generic || psd->generic;

   F32 radius = mRadius;
   if ( mUseGeneric )
      radius *= smGenericRadiusSkew;

   _buildPartition( pos, lightDir, radius, shadowLen );
   if ( mPartition.empty() )
      return false;

   // A fading shape's shadow is drawn in the fade, with no falloff of its own
   // (0x006538db).
   const F32 fade = mShapeBase->getFadeVal();
   if ( fade < 1.0f )
   {
      mPartitionShades.setSize( mPartitionVerts.size() );
      const U8 shade = U8( mClampF( fade, 0.0f, 1.0f ) * 255.0f );
      for ( S32 i = 0; i < mPartitionShades.size(); i++ )
         mPartitionShades[i] = shade;
   }
   else
      _updatePartition( fog );

   if ( mUseGeneric )
   {
      mShadowTexture = NULL;
      return true;
   }

   // The outline is redrawn as often as its size on screen asks for, and when
   // that size calls for another texture (0x006530e0).
   const U32 now = Platform::getVirtualMilliseconds();
   const bool expired = now - mOutlineTime > psd->frameExpiration;
   const bool changed = mShadowTexture.isNull() || psd->bmpDim != mOutlineDim || psd->blur != mOutlineBlur;
   if ( expired || changed )
   {
      mNeedOutline = true;
      mOutlineDim = psd->bmpDim;
      mOutlineBlur = psd->blur;
      mScore = 1.0f + F32( now - mOutlineTime ) / 1000.0f;
   }

   return true;
}

void ProjectedShadow::_buildPartition( const Point3F &pos, const VectorF &lightDir, F32 radius, F32 shadowLen )
{
   // Light space: x and z across the light, y down it from the shape.
   Point3F x, z;
   if ( mFabs( lightDir.z ) > 0.001f )
   {
      z.set( 0.0f, lightDir.z, -lightDir.y );
      z.normalize();
      mCross( lightDir, z, &x );
   }
   else
   {
      mCross( lightDir, Point3F( 0, 0, 1 ), &x );
      x.normalize();
      mCross( x, lightDir, &z );
   }

   mLightToWorld.identity();
   mLightToWorld.setColumn( 0, x );
   mLightToWorld.setColumn( 1, lightDir );
   mLightToWorld.setColumn( 2, z );
   mLightToWorld.setColumn( 3, pos );

   MatrixF worldToLight( mLightToWorld );
   worldToLight.inverse();

   Point3F extent( 2.0f * radius, shadowLen, 2.0f * radius );
   smDepthSortList.clear();
   smDepthSortList.set( worldToLight, extent );
   smDepthSortList.setInterestNormal( lightDir );

   mInvShadowDistance = 1.0f / getMax( shadowLen, 1.0f );

   // The world box and sphere around the shadow's volume.
   const Point3F bx = x * radius;
   const Point3F by = lightDir * shadowLen;
   const Point3F bz = z * radius;
   const Point3F half( mFabs( bx.x ) + mFabs( by.x ) + mFabs( bz.x ),
                       mFabs( bx.y ) + mFabs( by.y ) + mFabs( bz.y ),
                       mFabs( bx.z ) + mFabs( by.z ) + mFabs( bz.z ) );
   const Point3F center = pos + by * 0.5f;
   smShadowSphere.center = center;
   smShadowSphere.radius = half.len();
   smShadowBox.minExtents = center - half;
   smShadowBox.maxExtents = center + half;

   mParentObject->getContainer()->findObjects( smShadowBox, smShadowMask, _collisionCallback, this );

   // Only the first surface down the light takes each part of the shadow.
   const Point3F square[4] =
   {
      Point3F( -radius, 0.0f, -radius ),
      Point3F( -radius, 0.0f,  radius ),
      Point3F(  radius, 0.0f,  radius ),
      Point3F(  radius, 0.0f, -radius )
   };

   mPartition.clear();
   mPartitionVerts.clear();
   smDepthSortList.depthPartition( square, 4, mPartition, mPartitionVerts );

   const F32 invRadius = 1.0f / radius;
   mPartitionTVerts.setSize( mPartitionVerts.size() );
   for ( S32 i = 0; i < mPartitionVerts.size(); i++ )
      mPartitionTVerts[i].set( 0.5f + 0.5f * mPartitionVerts[i].x * invRadius,
                               0.5f + 0.5f * mPartitionVerts[i].z * invRadius );
}

void ProjectedShadow::_updatePartition( F32 fog )
{
   // Full strength where the shadow leaves the shape, none at its far end,
   // less any fog (0x006536e0).
   const F32 clear = 1.0f - fog;

   mPartitionShades.setSize( mPartitionVerts.size() );
   for ( S32 i = 0; i < mPartitionVerts.size(); i++ )
   {
      const F32 val = 1.0f - mPartitionVerts[i].y * mInvShadowDistance;
      mPartitionShades[i] = U8( mClampF( val * clear, 0.0f, 1.0f ) * 255.0f );
   }
}

void ProjectedShadow::_collisionCallback( SceneObject *obj, void *key )
{
   if ( obj->getWorldBox().isOverlapped( smShadowBox ) )
      obj->buildPolyList( PLC_Collision, &smDepthSortList, smShadowBox, smShadowSphere );
}

bool ProjectedShadow::shouldRender( const SceneRenderState *state )
{
   return mNeedOutline;
}

void ProjectedShadow::update( const SceneRenderState *state )
{
   if ( !state->isDiffusePass() )
      return;

   mNeedOutline = false;
   mScore = 0.0f;

   if ( !_prepare( state ) )
   {
      // Let another shadow have the texture from the pool.
      mShadowTexture = NULL;
      return;
   }

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &ProjectedShadow::_renderPartition );
   ri->type = RenderPassManager::RIT_Shadow;
   state->getRenderPass()->addInst( ri );
}

void ProjectedShadow::_renderPartition( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   if ( overrideMat || mPartitionVerts.empty() )
      return;

   const GFXTexHandle &texture = mUseGeneric ? _getGenericTexture() : mShadowTexture;
   if ( texture.isNull() )
      return;

   GFXDEBUGEVENT_SCOPE( ProjectedShadow_RenderPartition, ColorI( 255, 0, 0 ) );

   if ( !mDrawSB )
   {
      // The shadow multiplies what is under it by one less its shade
      // (0x0065385d), pulled toward the eye against the surface (0x00653994).
      GFXStateBlockDesc desc;
      desc.setCullMode( GFXCullNone );
      desc.setZReadWrite( true, false );
      desc.zBias = -0.0002f;
      desc.setBlend( true, GFXBlendZero, GFXBlendInvSrcColor );
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      mDrawSB = GFX->createStateBlock( desc );
   }

   GFXVertexBufferHandle<GFXVertexPCT> verts( GFX, mPartitionVerts.size(), GFXBufferTypeVolatile );
   GFXVertexPCT *v = verts.lock();
   if ( !v )
      return;

   for ( S32 i = 0; i < mPartitionVerts.size(); i++ )
   {
      mLightToWorld.mulP( mPartitionVerts[i], &v[i].point );
      const U8 shade = mPartitionShades[i];
      v[i].color.set( shade, shade, shade, shade );
      v[i].texCoord = mPartitionTVerts[i];
   }
   verts.unlock();

   GFX->setStateBlock( mDrawSB );
   GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
   GFX->setTexture( 0, texture );
   GFX->setVertexBuffer( verts );

   for ( S32 i = 0; i < mPartition.size(); i++ )
      GFX->drawPrimitive( GFXTriangleFan, mPartition[i].vertexStart, mPartition[i].vertexCount - 2 );
}

const GFXTexHandle& ProjectedShadow::_getGenericTexture()
{
   // Tribes 2's generic shadow (0x00652620): 180 at the centre falling to
   // nothing at the edge.
   if ( mGenericTexture.isNull() )
   {
      const S32 dim = 32;
      const S32 center = dim >> 1;
      const F32 invRadiusSq = 1.0f / (F32)( center * center );

      GBitmap *bitmap = new GBitmap( dim, dim, false, GFXFormatR8G8B8A8 );
      for ( S32 i = 0; i < dim; i++ )
         for ( S32 j = 0; j < dim; j++ )
         {
            const F32 t = (F32)( ( i - center ) * ( i - center ) + ( j - center ) * ( j - center ) ) * invRadiusSq;
            const U8 val = t > 0.99f ? 0 : U8( 180.0f - 180.0f * t );
            bitmap->setColor( j, i, ColorI( val, val, val, val ) );
         }

      mGenericTexture.set( bitmap, &GFXDefaultStaticDiffuseProfile, true, "BLGenericShadow" );
   }

   return mGenericTexture;
}

void ProjectedShadow::render( F32 camDist, const TSRenderState &rdata )
{
   if ( !mNeedOutline || mUseGeneric )
      return;

   mNeedOutline = false;
   _renderToTexture( rdata );
}

BaseMatInstance* ProjectedShadow::_getShadowMaterial( BaseMatInstance *inMat )
{
   // See if we have an existing material hook.
   ShadowMaterialHook *hook = static_cast<ShadowMaterialHook*>( inMat->getHook( ShadowMaterialHook::Type ) );
   if ( !hook )
   {
      // Create a hook and initialize it using the incoming material.
      hook = new ShadowMaterialHook;
      hook->init( inMat ); 
      inMat->addHook( hook );
   }

   return hook->getShadowMat( ShadowType_Spot );
}

bool ProjectedShadow::_initBlur()
{
   if ( mBlurShader )
      return true;

   ShaderData *shaderData;
   if ( !Sim::findObject( "BL_ShadowBlurShader", shaderData ) )
   {
      shaderData = new ShaderData;
      shaderData->setField( "OGLVertexShaderFile", "shaders/common/fixedFunction/gl/modColorTextureV.glsl" );
      shaderData->setField( "OGLPixelShaderFile", "shaders/common/lighting/basic/gl/shadowBlurP.glsl" );
      shaderData->setDataField( StringTable->insert( "samplerNames" ), "0", "diffuseMap" );
      shaderData->setField( "pixVersion", "2.0" );
      if ( !shaderData->registerObject( "BL_ShadowBlurShader" ) )
      {
         delete shaderData;
         return false;
      }
      Sim::getRootGroup()->addObject( shaderData );
   }

   mBlurShader = shaderData->getShader();
   if ( !mBlurShader )
      return false;

   mBlurConsts = mBlurShader->allocConstBuffer();
   mBlurModelViewSC = mBlurShader->getShaderConstHandle( "$modelview" );
   mBlurTexelSizeSC = mBlurShader->getShaderConstHandle( "$texelSize" );
   mBlurSC = mBlurShader->getShaderConstHandle( "$blur" );

   GFXStateBlockDesc desc;
   desc.setCullMode( GFXCullNone );
   desc.setZReadWrite( false, false );
   desc.samplersDefined = true;
   desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   mBlurSB = GFX->createStateBlock( desc );

   return true;
}

void ProjectedShadow::_renderToTexture( const TSRenderState &rdata )
{
   PROFILE_SCOPE( ProjectedShadow_RenderToTexture );

   GFXDEBUGEVENT_SCOPE( ProjectedShadow_RenderToTexture, ColorI( 255, 0, 0 ) );

   RenderPassManager *renderPass = _getRenderPass();
   if ( !renderPass || !_initBlur() )
      return;

   const U32 dim = mOutlineDim;

   // Tribes 2 draws the shape from the light into a bitmap a bit to a pixel,
   // and widens each bit to a byte, through a 3x3 blur at the larger sizes
   // (0x0065319d, 0x00443cd0). The outline here is drawn on the card and the
   // blur turns whatever it covers into those bits.
   GFXTexHandle outline( dim, dim, GFXFormatR8G8B8A8, &BLProjectedShadowProfile, "BLShadowOutline" );
   if ( mShadowTexture.isNull() || mShadowTexture.getWidth() != dim )
      mShadowTexture.set( dim, dim, GFXFormatR8G8B8A8, &BLProjectedShadowProfile, "BLShadow" );

   GFXTransformSaver saver;
   
   // NOTE: GFXTransformSaver does not save/restore the frustum
   // so we must save it here before we modify it.
   F32 l, r, b, t, n, f;
   bool ortho;
   GFX->getFrustum( &l, &r, &b, &t, &n, &f, &ortho );
   const RectI viewport = GFX->getViewport();

   // Look down the light at the shape, with light space's x and z across the
   // texture as the shadow's texture coordinates have them.
   const F32 back = 3.0f * mRadius;
   MatrixF lightCam( mLightToWorld );
   Point3F lightDir;
   mLightToWorld.getColumn( 1, &lightDir );
   lightCam.setPosition( mLightToWorld.getPosition() - lightDir * back );
   lightCam.inverse();

   GFX->setOrtho( -mRadius, mRadius, -mRadius, mRadius, 0.0f, 2.0f * back, true );
   GFX->setWorldMatrix( lightCam );

   GFX->pushActiveRenderTarget();

   if ( !mRenderTarget )
      mRenderTarget = GFX->allocRenderToTextureTarget();

   mRenderTarget->attachTexture( GFXTextureTarget::DepthStencil, _getDepthTarget( dim, dim ) );
   mRenderTarget->attachTexture( GFXTextureTarget::Color0, outline );
   GFX->setActiveRenderTarget( mRenderTarget );

   GFX->clear( GFXClearZBuffer | GFXClearStencil | GFXClearTarget, ColorI( 0, 0, 0, 0 ), 1.0f, 0 );

   const SceneRenderState *diffuseState = rdata.getSceneState();
   SceneManager *sceneManager = diffuseState->getSceneManager();

   SceneRenderState baseState
   (
      sceneManager,
      SPT_Shadow,
      SceneCameraState::fromGFXWithViewport( diffuseState->getViewport() ),
      renderPass
   );

   baseState.getMaterialDelegate().bind( &ProjectedShadow::_getShadowMaterial );
   baseState.setDiffuseCameraTransform( diffuseState->getCameraTransform() );
   baseState.setWorldToScreenScale( diffuseState->getWorldToScreenScale() );
   baseState.getCullingState().disableZoneCulling( true );

   // Tribes 2 draws a shadow's shape at a detail scaled by the shadow detail
   // level, and never finer than its minimum (0x00652928, 0x00652906).
   const F32 saveDetailAdjust = TSShapeInstance::smDetailAdjust;
   const S32 saveSkipDetails = TSShapeInstance::smNumSkipRenderDetails;
   TSShapeInstance::smDetailAdjust *= smShapeDetailScale;
   TSShapeInstance::smNumSkipRenderDetails = smShapeDetailMin;

   mParentObject->prepRenderImage( &baseState );
   renderPass->renderPass( &baseState );

   TSShapeInstance::smDetailAdjust = saveDetailAdjust;
   TSShapeInstance::smNumSkipRenderDetails = saveSkipDetails;

   mRenderTarget->resolve();

   // Then into the shadow's own texture, through the blur.
   if ( !mBlurTarget )
      mBlurTarget = GFX->allocRenderToTextureTarget();

   mBlurTarget->attachTexture( GFXTextureTarget::Color0, mShadowTexture );
   GFX->setActiveRenderTarget( mBlurTarget );

   GFXVertexBufferHandle<GFXVertexPCT> quad( GFX, 4, GFXBufferTypeVolatile );
   GFXVertexPCT *v = quad.lock();
   if ( v )
   {
      v[0].point.set( -1.0f, -1.0f, 0.0f ); v[0].texCoord.set( 0.0f, 0.0f );
      v[1].point.set(  1.0f, -1.0f, 0.0f ); v[1].texCoord.set( 1.0f, 0.0f );
      v[2].point.set( -1.0f,  1.0f, 0.0f ); v[2].texCoord.set( 0.0f, 1.0f );
      v[3].point.set(  1.0f,  1.0f, 0.0f ); v[3].texCoord.set( 1.0f, 1.0f );
      for ( U32 i = 0; i < 4; i++ )
         v[i].color.set( 255, 255, 255, 255 );
      quad.unlock();

      GFX->setStateBlock( mBlurSB );
      GFX->setShader( mBlurShader );
      GFX->setShaderConstBuffer( mBlurConsts );
      mBlurConsts->setSafe( mBlurModelViewSC, MatrixF( true ) );
      mBlurConsts->setSafe( mBlurTexelSizeSC, Point2F( 1.0f / dim, 1.0f / dim ) );
      mBlurConsts->setSafe( mBlurSC, mOutlineBlur ? 1.0f : 0.0f );
      GFX->setTexture( 0, outline );
      GFX->setVertexBuffer( quad );
      GFX->drawPrimitive( GFXTriangleStrip, 0, 2 );
      GFX->setTexture( 0, NULL );
   }

   mBlurTarget->resolve();
   GFX->popActiveRenderTarget();
   GFX->setViewport( viewport );

   // Restore frustum
   if ( !ortho )
      GFX->setFrustum( l, r, b, t, n, f );
   else
      GFX->setOrtho( l, r, b, t, n, f );

   mOutlineTime = mLastRenderTime = Platform::getVirtualMilliseconds();
}

RenderPassManager* ProjectedShadow::_getRenderPass()
{
   if ( smRenderPass.isNull() )
   {
      SimObject* renderPass = NULL;

      if ( !Sim::findObject( "BL_ProjectedShadowRPM", renderPass ) )
      {
         RenderPassManager *pass = new RenderPassManager;
         if ( pass->registerObject( "BL_ProjectedShadowRPM" ) )
         {
            RenderMeshMgr *bin = new RenderMeshMgr;
            if ( bin->registerObject() )
            {
               pass->addManager( bin );
               Sim::getRootGroup()->addObject( bin );
            }
            else
               delete bin;

            Sim::getRootGroup()->addObject( pass );
            smRenderPass = pass;

            Con::warnf( "ProjectedShadow: no BL_ProjectedShadowRPM was set up "
                        "in script; using a default one." );
         }
         else
         {
            delete pass;
            Con::errorf( "ProjectedShadow::init() - 'BL_ProjectedShadowRPM' not initialized" );
         }
      }
      else
         smRenderPass = dynamic_cast<RenderPassManager*>(renderPass);
   }

   return smRenderPass;
}

GFXTextureObject* ProjectedShadow::_getDepthTarget( U32 width, U32 height )
{
   // Get a depth texture target from the pooled profile
   // which is returned as a temporary.
   GFXTexHandle depthTex( width, height, GFXFormatD24S8, &BLProjectedShadowZProfile, 
      "ProjectedShadow::_getDepthTarget()" );

   return depthTex;
}
