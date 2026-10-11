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
#include "terrain/terrRender.h"

#include "terrain/terrData.h"
#include "terrain/terrCell.h"
#include "terrain/terrMaterial.h"
#include "terrain/terrCellMaterial.h"
#include "materials/shaderData.h"

#include "platform/profiler.h"
#include "scene/sceneRenderState.h"
#include "math/util/frustum.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderTerrainMgr.h"

#include "lighting/lightInfo.h"
#include "lighting/lightManager.h"

#include "materials/matInstance.h"
#include "materials/materialManager.h"
#include "materials/matTextureTarget.h"
#include "shaderGen/conditionerFeature.h"

#include "gfx/gfxDrawUtil.h"

#include "gfx/gfxTransformSaver.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/bitmap/ddsFile.h"
#include "gfx/bitmap/ddsUtils.h"
#include "terrain/terrMaterial.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxCardProfile.h"
#include "core/stream/fileStream.h"


bool TerrainBlock::smDebugRender = false;


GFX_ImplementTextureProfile( TerrainLayerTexProfile,
                            GFXTextureProfile::DiffuseMap, 
                            GFXTextureProfile::PreserveSize | 
                            GFXTextureProfile::Dynamic,
                            GFXTextureProfile::None );


void TerrainBlock::_onFlushMaterials()
{
   if ( mCell )
      mCell->deleteMaterials();

   SAFE_DELETE( mBaseMaterial );
}

void TerrainBlock::_updateMaterials()
{   
   mBaseTextures.setSize( mFile->mMaterials.size() );

   mMaxDetailDistance = 0.0f;

   for ( U32 i=0; i < mFile->mMaterials.size(); i++ )
   {
      TerrainMaterial *mat = mFile->mMaterials[i];

      if( !mat->getDiffuseMap().isEmpty() )
         mBaseTextures[i].set( mat->getDiffuseMap(),  
            &GFXDefaultStaticDiffuseProfile, 
            "TerrainBlock::_updateMaterials() - DiffuseMap" );
      else
         mBaseTextures[ i ] = GFXTexHandle();

      // Find the maximum detail distance.
      if (  mat->getDetailMap().isNotEmpty() &&
            mat->getDetailDistance() > mMaxDetailDistance )
         mMaxDetailDistance = mat->getDetailDistance();
   }

   if ( mCell )
      mCell->deleteMaterials();
}

void TerrainBlock::_updateLayerTexture()
{
   const U32 layerSize = mFile->mSize;
   const Vector<U8> &layerMap = mFile->mLayerMap;
   const U32 pixelCount = layerMap.size();

   if (  mLayerTex.isNull() ||
         mLayerTex.getWidth() != layerSize ||
         mLayerTex.getHeight() != layerSize )
      mLayerTex.set( layerSize, layerSize, GFXFormatR8G8B8A8, &TerrainLayerTexProfile, "" );

   AssertFatal(   mLayerTex.getWidth() == layerSize &&
                  mLayerTex.getHeight() == layerSize,
      "TerrainBlock::_updateLayerTexture - The texture size doesn't match the requested size!" );

   // Update the layer texture.
   GFXLockedRect *lock = mLayerTex.lock();

   for ( U32 i=0; i < pixelCount; i++ )
   {  
      lock->bits[0] = layerMap[i];

      if ( i + 1 >= pixelCount )
         lock->bits[1] = lock->bits[0];
      else
         lock->bits[1] = layerMap[i+1];

      if ( i + layerSize >= pixelCount )
         lock->bits[2] = lock->bits[0];
      else
         lock->bits[2] = layerMap[i + layerSize];

      if ( i + layerSize + 1 >= pixelCount )
         lock->bits[3] = lock->bits[0];
      else
         lock->bits[3] = layerMap[i + layerSize + 1];

      lock->bits += 4;
   }

   mLayerTex.unlock();
   //mLayerTex->dumpToDisk( "png", "./layerTex.png" );

   // HH: Added for Tribes 2 compatibility.
   // The id map above lets two materials meet only where their indices are
   // neighbours, which is not how Tribes 2 draws a terrain: every material
   // has a weight everywhere. Those weights go up as they are, four to a
   // texture, and the shader samples its own channel instead of guessing one
   // from the ids around it.
   const U32 layers = mFile->mMaterials.size();
   mLayerAlphaTex.setSize( mFile->hasLayerAlpha() ? ( layers + 3 ) / 4 : 0 );

   for ( U32 plane = 0; plane < mLayerAlphaTex.size(); plane++ )
   {
      GFXTexHandle &tex = mLayerAlphaTex[ plane ];
      if (  tex.isNull() ||
            tex.getWidth() != layerSize ||
            tex.getHeight() != layerSize )
         tex.set( layerSize, layerSize, GFXFormatR8G8B8A8, &TerrainLayerTexProfile, "" );

      GFXLockedRect *alphaLock = tex.lock();

      for ( U32 i=0; i < pixelCount; i++ )
      {
         for ( U32 channel = 0; channel < 4; channel++ )
         {
            const U32 layer = plane * 4 + channel;
            alphaLock->bits[ channel ] = layer < layers
               ? mFile->getLayerAlpha( layer, i ) : 0;
         }

         alphaLock->bits += 4;
      }

      tex.unlock();
   }
}

bool TerrainBlock::_initBaseShader()
{
   ShaderData *shaderData = NULL;
   if ( !Sim::findObject( "TerrainBlendShader", shaderData ) || !shaderData )
      return false;

   mBaseShader = shaderData->getShader();

   mBaseShaderConsts = mBaseShader->allocConstBuffer();
   mBaseTexScaleConst = mBaseShader->getShaderConstHandle( "$texScale" );
   mBaseTexIdConst = mBaseShader->getShaderConstHandle( "$texId" );
   mBaseLayerSizeConst = mBaseShader->getShaderConstHandle( "$layerSize" );

   mBaseTarget = GFX->allocRenderToTextureTarget();

   GFXStateBlockDesc desc;
   desc.samplersDefined = true;
   desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();   
   desc.samplers[1] = GFXSamplerStateDesc::getWrapLinear();   
   desc.zDefined = true;
   desc.zWriteEnable = false;
   desc.zEnable = false;
   desc.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   desc.cullDefined = true;
   desc.cullMode = GFXCullNone;
   mBaseShaderSB = GFX->createStateBlock( desc );

   return true;
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2 composites its terrain on the CPU: `terrData.cc` hands eight alpha
// maps to `Blender` (0x005b3e80, five mip levels) and that sums the source
// textures weighted by them. Torque bakes the same sum on the GPU, a quad per
// material blended by src alpha, but takes the weight from the layer index
// map -- one material per sample -- so two materials only ever meet where
// their indices are neighbours. With the alpha maps the file came with, the
// sum is the one Tribes 2 draws, and doing it here needs neither the
// script-defined blend shader nor a render target.
bool TerrainBlock::_updateBaseTextureBlended()
{
   PROFILE_SCOPE( TerrainBlock_UpdateBaseTextureBlended );

   const U32 layers = mFile->mMaterials.size();
   if ( !layers )
      return false;

   const U32 maxTextureSize = GFX->getCardProfiler()->queryProfile( "maxTextureSize", 1024 );
   const U32 size = getMin( maxTextureSize, getNextPow2( mBaseTexSize ) );
   const U32 terrSize = mFile->mSize;
   const F32 blockSize = getWorldBlockSize();

   // A material's texture covers diffuseSize metres, so it repeats this often
   // over the block and lands on this few texels each time. Reduce each
   // source to that, by the box filter the mip levels Tribes 2's blender
   // keeps would have given it -- sampling the full texture at this stride
   // leaves the terrain stippled with whatever texels the stride happens to
   // land on.
   Vector< Vector<ColorI> > sources;
   Vector<U32> sourceDim;
   sources.setSize( layers );
   sourceDim.setSize( layers );
   U32 haveSources = 0;

   for ( U32 i=0; i < layers; i++ )
   {
      sourceDim[i] = 0;

      Resource<GBitmap> bitmap =
         GBitmap::load( Torque::Path( mFile->mMaterials[i]->getDiffuseMap() ) );
      if ( !bitmap )
         continue;

      F32 repeats = 1.0f;
      const F32 diffuseSize = mFile->mMaterials[i]->getDiffuseSize();
      if ( !mIsZero( diffuseSize ) )
         repeats = blockSize / diffuseSize;

      // How many texels of the base texture one repeat of this material
      // lands on. Under eight there is no tile left to see -- what reaches
      // the screen is whichever texels the stride fell on -- so take the
      // bottom of the mip chain, the material's average colour, and let the
      // detail map carry the near field as it does in Tribes 2.
      const U32 perRepeat = U32( F32( size ) / getMax( 1.0f, repeats ) );
      const U32 dim = perRepeat < 8 ? 1
         : getMin( getNextPow2( perRepeat ), bitmap->getWidth() );

      const U32 srcW = bitmap->getWidth();
      const U32 srcH = bitmap->getHeight();
      const U32 stepX = getMax( U32( 1 ), srcW / dim );
      const U32 stepY = getMax( U32( 1 ), srcH / dim );

      sources[i].setSize( dim * dim );
      for ( U32 y=0; y < dim; y++ )
      {
         for ( U32 x=0; x < dim; x++ )
         {
            U32 red = 0, green = 0, blue = 0, taken = 0;
            for ( U32 sy = y * stepY; sy < ( y + 1 ) * stepY && sy < srcH; sy++ )
            {
               for ( U32 sx = x * stepX; sx < ( x + 1 ) * stepX && sx < srcW; sx++ )
               {
                  ColorI texel;
                  bitmap->getColor( sx, sy, texel );
                  red += texel.red;
                  green += texel.green;
                  blue += texel.blue;
                  taken++;
               }
            }

            if ( !taken )
               taken = 1;
            sources[i][ x + y * dim ].set( red / taken, green / taken, blue / taken, 255 );
         }
      }

      sourceDim[i] = dim;
      haveSources++;
   }

   if ( !haveSources )
   {
      Con::errorf( "TerrainBlock: none of the %d terrain textures loaded; "
                   "the base texture is left unblended.", layers );
      return false;
   }

   GBitmap bitmap( size, size, false, GFXFormatR8G8B8A8 );

   for ( U32 y=0; y < size; y++ )
   {
      const U32 sampleY = ( y * terrSize ) / size;

      for ( U32 x=0; x < size; x++ )
      {
         const U32 sampleX = ( x * terrSize ) / size;
         const U32 sample = sampleX + sampleY * terrSize;

         U32 red = 0, green = 0, blue = 0, weight = 0;

         for ( U32 i=0; i < layers; i++ )
         {
            const U32 alpha = mFile->getLayerAlpha( i, sample );
            if ( !alpha || !sourceDim[i] )
               continue;

            const U32 dim = sourceDim[i];
            const ColorI &texel = sources[i][ ( x % dim ) + ( y % dim ) * dim ];

            red += texel.red * alpha;
            green += texel.green * alpha;
            blue += texel.blue * alpha;
            weight += alpha;
         }

         ColorI out( 0, 0, 0, 255 );
         if ( weight )
            out.set( red / weight, green / weight, blue / weight, 255 );
         bitmap.setColor( x, y, out );
      }
   }

   bitmap.extrudeMipLevels();
   mBaseTex.set( &bitmap, &GFXDefaultStaticDiffuseProfile, false,
                 "TerrainBlock::mBaseTex" );

   if ( Con::getBoolVariable( "$pref::Terrain::dumpBaseTexture", false ) )
   {
      FileStream fs;
      const String path = _getBaseTexCacheFileName() + ".png";
      if ( fs.open( path, Torque::FS::File::Write ) )
      {
         bitmap.writeBitmap( "png", fs );
         fs.close();
         Con::printf( "TerrainBlock: wrote %s", path.c_str() );
      }
   }

   return true;
}

void TerrainBlock::_updateBaseTexture( bool writeToCache )
{
   // HH: Modified for Tribes 2 compatibility.
   if ( mFile->hasLayerAlpha() && _updateBaseTextureBlended() )
      return;

   if ( !mBaseShader && !_initBaseShader() )
      return;

   // This can sometimes occur outside a begin/end scene.
   const bool sceneBegun = GFX->canCurrentlyRender();
   if ( !sceneBegun )
      GFX->beginScene();

   GFXDEBUGEVENT_SCOPE( TerrainBlock_UpdateBaseTexture, ColorI::GREEN );

   PROFILE_SCOPE( TerrainBlock_UpdateBaseTexture );

   GFXTransformSaver saver;

   const U32 maxTextureSize = GFX->getCardProfiler()->queryProfile( "maxTextureSize", 1024 );

   U32 baseTexSize = getNextPow2( mBaseTexSize );
   baseTexSize = getMin( maxTextureSize, baseTexSize );
   Point2I destSize( baseTexSize, baseTexSize );

   // Setup geometry
   GFXVertexBufferHandle<GFXVertexPT> vb;
   {
      F32 copyOffsetX = 2.0f * GFX->getFillConventionOffset() / (F32)destSize.x;
      F32 copyOffsetY = 2.0f * GFX->getFillConventionOffset() / (F32)destSize.y;

      const bool needsYFlip = GFX->getAdapterType() == OpenGL;

      GFXVertexPT points[4];
      points[0].point      = Point3F( -1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0 );
      points[0].texCoord   = Point2F(  0.0, needsYFlip ? 0.0f : 1.0f );
      points[1].point      = Point3F( -1.0 - copyOffsetX,  1.0 + copyOffsetY, 0.0 );
      points[1].texCoord   = Point2F(  0.0, needsYFlip ? 1.0f : 0.0f );
      points[2].point      = Point3F(  1.0 - copyOffsetX,  1.0 + copyOffsetY, 0.0 );
      points[2].texCoord   = Point2F(  1.0, needsYFlip ? 1.0f : 0.0f );
      points[3].point      = Point3F(  1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0 );
      points[3].texCoord   = Point2F(  1.0, needsYFlip ? 0.0f : 1.0f );

      vb.set( GFX, 4, GFXBufferTypeVolatile );
      dMemcpy( vb.lock(), points, sizeof(GFXVertexPT) * 4 );
      vb.unlock();
   }

   GFXTexHandle blendTex;

   // If the base texture is already a valid render target then 
   // use it to render to else we create one.
   if (  mBaseTex.isValid() && 
         mBaseTex->isRenderTarget() &&
         mBaseTex->getFormat() == GFXFormatR8G8B8A8 &&
         mBaseTex->getWidth() == destSize.x &&
         mBaseTex->getHeight() == destSize.y )
      blendTex = mBaseTex;
   else
      blendTex.set( destSize.x, destSize.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, "" );

   GFX->pushActiveRenderTarget();   

   // Set our shader stuff
   GFX->setShader( mBaseShader );
   GFX->setShaderConstBuffer( mBaseShaderConsts );
   GFX->setStateBlock( mBaseShaderSB );
   GFX->setVertexBuffer( vb );

   mBaseTarget->attachTexture( GFXTextureTarget::Color0, blendTex );
   GFX->setActiveRenderTarget( mBaseTarget );

   GFX->setTexture( 0, mLayerTex );
   mBaseShaderConsts->setSafe( mBaseLayerSizeConst, (F32)mLayerTex->getWidth() );      

   for ( U32 i=0; i < mBaseTextures.size(); i++ )
   {
      GFXTextureObject *tex = mBaseTextures[i];
      if ( !tex )
         continue;

      GFX->setTexture( 1, tex );

      F32 baseSize = mFile->mMaterials[i]->getDiffuseSize();
      F32 scale = 1.0f;
      if ( !mIsZero( baseSize ) )
         scale = getWorldBlockSize() / baseSize;
      
      // A mistake early in development means that texture
      // coords are not flipped correctly.  To compensate
      // we flip the y scale here.
      mBaseShaderConsts->setSafe( mBaseTexScaleConst, Point2F( scale, -scale ) );
      mBaseShaderConsts->setSafe( mBaseTexIdConst, (F32)i );

      GFX->drawPrimitive( GFXTriangleFan, 0, 2 );
   }

   mBaseTarget->resolve();
   
   GFX->setShader( NULL );
   //GFX->setStateBlock( NULL ); // WHY NOT?
   GFX->setShaderConstBuffer( NULL );
   GFX->setVertexBuffer( NULL );

   GFX->popActiveRenderTarget();

   // End it if we begun it... Yeehaw!
   if ( !sceneBegun )
      GFX->endScene();

   /// Do we cache this sucker?
   if ( writeToCache )
   {
      String cachePath = _getBaseTexCacheFileName();

      FileStream fs;
      if ( fs.open( _getBaseTexCacheFileName(), Torque::FS::File::Write ) )
      {
         // Read back the render target, dxt compress it, and write it to disk.
         GBitmap blendBmp( destSize.x, destSize.y, false, GFXFormatR8G8B8A8 );
         blendTex.copyToBmp( &blendBmp );

         /*
         // Test code for dumping uncompressed bitmap to disk.
         {
         FileStream fs;
         if ( fs.open( "./basetex.png", Torque::FS::File::Write ) )
         {
         blendBmp.writeBitmap( "png", fs );
         fs.close();
         }         
         }
         */

         blendBmp.extrudeMipLevels();

         DDSFile *blendDDS = DDSFile::createDDSFileFromGBitmap( &blendBmp );
         DDSUtil::squishDDS( blendDDS, GFXFormatDXT1 );

         // Write result to file stream
         blendDDS->write( fs );
         
         delete blendDDS;
      }
      fs.close();
   }
   else
   {
      // We didn't cache the result, so set the base texture
      // to the render target we updated.  This should be good
      // for realtime painting cases.
      mBaseTex = blendTex;
   }
}

void TerrainBlock::_renderBlock( SceneRenderState *state )
{
   PROFILE_SCOPE( TerrainBlock_RenderBlock );

   // Prevent rendering shadows if feature is disabled
   if ( !mCastShadows && state->isShadowPass() ) 
      return;
	  
   MatrixF worldViewXfm = state->getWorldViewMatrix();
   worldViewXfm.mul( getRenderTransform() );

   MatrixF worldViewProjXfm = state->getProjectionMatrix();
   worldViewProjXfm.mul( worldViewXfm );

   const MatrixF &objectXfm = getRenderWorldTransform();

   Point3F objCamPos = state->getDiffuseCameraPosition();
   objectXfm.mulP( objCamPos );

   // Get the shadow material.
   if ( !mDefaultMatInst )
      mDefaultMatInst = TerrainCellMaterial::getShadowMat();

   // Make sure we have a base material.
   if ( !mBaseMaterial )
   {
      mBaseMaterial = new TerrainCellMaterial();
      mBaseMaterial->init( this, 0, false, false, true );
   }

   // Did the detail layers change?
   if ( mDetailsDirty )
   {   
      _updateMaterials();
      mDetailsDirty = false;
   }

   // If the layer texture has been cleared or is 
   // dirty then update it.
   if ( mLayerTex.isNull() || mLayerTexDirty )
      _updateLayerTexture();

   // If the layer texture is dirty or we lost the base
   // texture then regenerate it.
   if ( mLayerTexDirty || mBaseTex.isNull() )
   {
      _updateBaseTexture( false );
      mLayerTexDirty = false;
   }   

   static Vector<TerrCell*> renderCells;
   renderCells.clear();

   mCell->cullCells( state,
                     objCamPos,
                     &renderCells );

   RenderPassManager *renderPass = state->getRenderPass();

   MatrixF *riObjectToWorldXfm = renderPass->allocUniqueXform( getRenderTransform() );

   const bool isColorDrawPass = state->isDiffusePass() || state->isReflectPass();

   // This is here for shadows mostly... it allows the
   // proper shadow material to be generated.
   BaseMatInstance *defaultMatInst = state->getOverrideMaterial( mDefaultMatInst );

   // Only pass and use the light manager if this is not a shadow pass.
   LightManager *lm = NULL;
   if ( isColorDrawPass )
      lm = LIGHTMGR;

   for ( U32 i=0; i < renderCells.size(); i++ )
   {
      TerrCell *cell = renderCells[i];

      // Ok this cell is fit to render.
      TerrainRenderInst *inst = renderPass->allocInst<TerrainRenderInst>();

      // Setup lights for this cell.
      if ( lm )
      {
         SphereF bounds = cell->getSphereBounds();
         getRenderTransform().mulP( bounds.center );

         LightQuery query;
         query.init( bounds );
		   query.getLights( inst->lights, 8 );
      }

      GFXVertexBufferHandleBase vertBuff;
      GFXPrimitiveBufferHandle  primBuff;

      cell->getRenderPrimitive( &inst->prim, &vertBuff, &primBuff );

      inst->mat = defaultMatInst;
      inst->vertBuff = vertBuff.getPointer();

      if ( primBuff.isValid() )
      {
         // Use the cell's custom primitive buffer
         inst->primBuff = primBuff.getPointer();
      }
      else
      {
         // Use the standard primitive buffer for this cell
         inst->primBuff = mPrimBuffer.getPointer();
      }

      inst->objectToWorldXfm = riObjectToWorldXfm;

      // If we're not drawing to the shadow map then we need
      // to include the normal rendering materials. 
      if ( isColorDrawPass )
      {         
         const SphereF &bounds = cell->getSphereBounds();         

         F32 sqDist = ( bounds.center - objCamPos ).lenSquared();         

         F32 radiusSq = mSquared( ( mMaxDetailDistance + bounds.radius ) * smDetailScale );

         // If this cell is near enough to get detail textures then
         // use the full detail mapping material.  Else we use the
         // simple base only material.
         if ( !state->isReflectPass() && sqDist < radiusSq )
            inst->cellMat = cell->getMaterial();
         else if ( state->isReflectPass() )
            inst->cellMat = mBaseMaterial->getReflectMat();
         else
            inst->cellMat = mBaseMaterial;
      }

      inst->defaultKey = (U32)cell->getMaterials();

      // Submit it for rendering.
      renderPass->addInst( inst );
   }

   // Trigger the debug rendering.
   if (  state->isDiffusePass() && 
         !renderCells.empty() && 
         smDebugRender )
   {      
      // Store the render cells for later.
      mDebugCells = renderCells;

      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &TerrainBlock::_renderDebug );
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri );
   }
}

void TerrainBlock::_renderDebug( ObjectRenderInst *ri, 
                                 SceneRenderState *state, 
                                 BaseMatInstance *overrideMat )
{
   GFXTransformSaver saver;
   GFX->multWorld( getRenderTransform() );

   for ( U32 i=0; i < mDebugCells.size(); i++ )
      mDebugCells[i]->renderBounds();

   mDebugCells.clear();
}