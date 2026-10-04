#include "platform/platform.h"
#include "materials/renderBootstrap.h"

#include "console/simBase.h"
#include "materials/shaderData.h"

namespace
{

/// Declare a ShaderData the engine looks up by name, if nothing else has.
///
/// The four file names are what the script declaration carried; the engine
/// picks the OGL pair. A name that is already declared is left alone, so a
/// GameData that does ship the scripts keeps its own.
void shader( const char *name,
             const char *dxVert, const char *dxPix,
             const char *glVert, const char *glPix,
             F32 pixVersion,
             const char *const *samplerNames = NULL,
             const char *defines = NULL )
{
   if( Sim::findObject( name ) )
      return;

   ShaderData *sd = new ShaderData;
   sd->setDataField( StringTable->insert( "DXVertexShaderFile" ), NULL, dxVert );
   sd->setDataField( StringTable->insert( "DXPixelShaderFile" ),  NULL, dxPix );
   sd->setDataField( StringTable->insert( "OGLVertexShaderFile" ), NULL, glVert );
   sd->setDataField( StringTable->insert( "OGLPixelShaderFile" ),  NULL, glPix );
   sd->setDataField( StringTable->insert( "pixVersion" ), NULL, avar( "%g", pixVersion ) );
   if( defines )
      sd->setDataField( StringTable->insert( "defines" ), NULL, defines );

   // In texture-unit order, NULL terminated. The GL path needs these: a
   // sampler the ShaderData does not name gets no texture unit, so every
   // sampler in the program reads unit zero and the shader silently samples
   // the wrong texture rather than failing.
   for( U32 unit = 0; samplerNames && samplerNames[ unit ]; unit++ )
      sd->setDataField( StringTable->insert( "samplerNames" ),
                        avar( "%u", unit ), samplerNames[ unit ] );

   if( sd->registerObject( name ) )
      Sim::getRootGroup()->addObject( sd );
   else
      delete sd;
}

} // namespace

/// _updateBaseTexture binds the layer map to unit 0 and each base texture to
/// unit 1; these are the names blendP.glsl gives those two samplers.
static const char *const sTerrainBlendSamplers[] = { "layerTex", "textureMap", NULL };

void RenderBootstrap::declareMissing()
{
   // core/scripts/client/terrainBlock.cs. Without it TerrainBlock::_initBaseShader
   // returns false, _updateBaseTexture returns at its first line, and the base
   // texture a terrain cell samples is never built -- the ground draws black.
   // Both .glsl files are staged under shaders/common/terrain/gl.
   shader( "TerrainBlendShader",
           "shaders/common/terrain/blendV.hlsl",
           "shaders/common/terrain/blendP.hlsl",
           "shaders/common/terrain/gl/blendV.glsl",
           "shaders/common/terrain/gl/blendP.glsl",
           2.0f,
           // _updateBaseTexture binds the layer map to unit 0 and each base
           // texture to unit 1. Without these the GL shader gave both samplers
           // no unit, so the blend pass sampled the layer map where it meant to
           // sample the ground -- which is a terrain that draws, in the right
           // shape, at a few per cent of its brightness.
           sTerrainBlendSamplers );
}
