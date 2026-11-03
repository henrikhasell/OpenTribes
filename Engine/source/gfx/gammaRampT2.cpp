// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gfx/gammaRampT2.h"

#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTarget.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxTextureProfile.h"
#include "gfx/gfxVertexBuffer.h"
#include "gfx/gfxStructs.h"
#include "gfx/gfxTransformSaver.h"
#include "materials/shaderData.h"
#include "console/simSet.h"
#include "math/mMathFn.h"

namespace
{
   /// What retail's getGammaCorrection (0x00564690) reads back off the
   /// display before it first sets a ramp, and so where the slider's
   /// middle sits. Our display has no ramp of its own, so this is that
   /// reading of an identity ramp; it comes out a little under one only
   /// because the reading puts entry i at i/256 where the ramp has i/255.
   F32 sgOriginalGamma = -1.0f;

   /// The slider's offset from its middle (0x007df474).
   F32 sgGammaCorrection = 0.0f;

   /// The exponent retail's setGammaCorrection (0x00564850) last built the
   /// ramp with, and whether it has built one at all.
   F32 sgRampGamma = 1.0f;
   bool sgRampSet = false;

   GFXTexHandle sgFrame;
   GFXVertexBufferHandle<GFXVertexPT> sgQuad;
   GFXStateBlockRef sgStateBlock;
   GFXShaderConstBufferRef sgConsts;
   GFXShaderConstHandle *sgGammaSC = NULL;
   GFXShader *sgShader = NULL;

   F32 readIdentityRamp()
   {
      F32 sum = 0.0f;
      U32 count = 0;
      for ( U32 i = 1; i < 256; i++ )
      {
         const U32 entry = i * 257;
         if ( entry == 0 || entry == 65535 )
            continue;

         const F64 b = F64( i ) / 256.0;
         const F64 a = F64( entry ) / 65535.0;
         sum += F32( mLog( a ) / mLog( b ) );
         count++;
      }
      return sum / F32( count );
   }

   bool init()
   {
      if ( sgShader )
         return true;

      ShaderData *data;
      if ( !Sim::findObject( "GammaRampT2Shader", data ) )
      {
         data = new ShaderData;
         data->setDataField( StringTable->insert( "OGLVertexShaderFile" ), NULL, "shaders/common/gl/gammaRampV.glsl" );
         data->setDataField( StringTable->insert( "OGLPixelShaderFile" ), NULL, "shaders/common/gl/gammaRampP.glsl" );
         data->setDataField( StringTable->insert( "samplerNames" ), "0", "$frame" );
         data->setDataField( StringTable->insert( "pixVersion" ), NULL, "2.0" );
         if ( !data->registerObject( "GammaRampT2Shader" ) )
         {
            delete data;
            return false;
         }
         Sim::getRootGroup()->addObject( data );
      }

      sgShader = data->getShader();
      if ( !sgShader )
         return false;

      sgConsts = sgShader->allocConstBuffer();
      sgGammaSC = sgShader->getShaderConstHandle( "$gamma" );

      GFXStateBlockDesc desc;
      desc.setZReadWrite( false, false );
      desc.setCullMode( GFXCullNone );
      desc.setBlend( false );
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
      sgStateBlock = GFX->createStateBlock( desc );

      sgQuad.set( GFX, 4, GFXBufferTypeStatic );
      GFXVertexPT *verts = sgQuad.lock();
      verts[0].point.set( -1.0f, -1.0f, 0.0f ); verts[0].texCoord.set( 0.0f, 0.0f );
      verts[1].point.set( -1.0f,  1.0f, 0.0f ); verts[1].texCoord.set( 0.0f, 1.0f );
      verts[2].point.set(  1.0f, -1.0f, 0.0f ); verts[2].texCoord.set( 1.0f, 0.0f );
      verts[3].point.set(  1.0f,  1.0f, 0.0f ); verts[3].texCoord.set( 1.0f, 1.0f );
      sgQuad.unlock();

      return true;
   }
}

void GammaRampT2::apply()
{
   if ( !sgRampSet || !init() )
      return;

   GFXTarget *target = GFX->getActiveRenderTarget();
   if ( !target )
      return;

   const Point2I &size = target->getSize();
   if ( sgFrame.isNull() || sgFrame.getWidth() != size.x || sgFrame.getHeight() != size.y )
      sgFrame.set( size.x, size.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, "GammaRampT2 frame" );
   target->resolveTo( sgFrame );

   GFXTransformSaver saver;
   GFX->setWorldMatrix( MatrixF::Identity );
   GFX->setViewMatrix( MatrixF::Identity );
   GFX->setProjectionMatrix( MatrixF::Identity );

   GFX->setClipRect( RectI( Point2I::Zero, size ) );
   GFX->setStateBlock( sgStateBlock );
   GFX->setShader( sgShader );
   sgConsts->setSafe( sgGammaSC, sgRampGamma );
   GFX->setShaderConstBuffer( sgConsts );
   GFX->setTexture( 0, sgFrame );
   GFX->setVertexBuffer( sgQuad );
   GFX->drawPrimitive( GFXTriangleStrip, 0, 2 );

   GFX->setTexture( 0, NULL );
}

DefineEngineFunction( videoSetGammaCorrection, void, ( F32 gamma ),,
   "@brief Set the display's gamma from Tribes 2's Gamma Correction slider, "
   "0 to 1 with the display's own at 0.5.\n\n"
   "@ingroup Rendering" )
{
   // 0x00402450: the slider's offset from its middle, negated, is added
   // to what the display started at -- only when it changes, so the
   // middle leaves the display alone until the slider is moved.
   const F32 g = mClampF( gamma, 0.0f, 1.0f );
   const F32 d = -( g - 0.5f );

   if ( d != sgGammaCorrection )
   {
      if ( sgOriginalGamma == -1.0f )
         sgOriginalGamma = readIdentityRamp();

      sgRampGamma = sgOriginalGamma + d;
      sgRampSet = true;
   }
   sgGammaCorrection = d;
}
