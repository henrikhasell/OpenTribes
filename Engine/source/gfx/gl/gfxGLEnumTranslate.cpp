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
#include "gfx/gl/gfxGLEnumTranslate.h"
 
GLenum GFXGLPrimType[GFXPT_COUNT];
GLenum GFXGLBlend[GFXBlend_COUNT];
GLenum GFXGLBlendOp[GFXBlendOp_COUNT];
GLenum GFXGLSamplerState[GFXSAMP_COUNT];
GLenum GFXGLTextureFilter[GFXTextureFilter_COUNT];
GLenum GFXGLTextureAddress[GFXAddress_COUNT];
GLenum GFXGLCmpFunc[GFXCmp_COUNT];
GLenum GFXGLStencilOp[GFXStencilOp_COUNT];
GLenum GFXGLTextureInternalFormat[GFXFormat_COUNT];
GLenum GFXGLTextureFormat[GFXFormat_COUNT];
GLenum GFXGLTextureType[GFXFormat_COUNT];
const GLint* GFXGLTextureSwizzle[GFXFormat_COUNT];
GLenum GFXGLBufferType[GFXBufferType_COUNT];
GLenum GFXGLCullMode[GFXCull_COUNT];
GLenum GFXGLFillMode[GFXFill_COUNT];

void GFXGLEnumTranslate::init()
{
   // Buffer types
   GFXGLBufferType[GFXBufferTypeStatic] = GL_STATIC_DRAW;
   GFXGLBufferType[GFXBufferTypeDynamic] = GL_DYNAMIC_DRAW;
   GFXGLBufferType[GFXBufferTypeVolatile] = GL_STREAM_DRAW;

   // Primitives
   GFXGLPrimType[GFXPointList] = GL_POINTS;
   GFXGLPrimType[GFXLineList] = GL_LINES;
   GFXGLPrimType[GFXLineStrip] = GL_LINE_STRIP;
   GFXGLPrimType[GFXTriangleList] = GL_TRIANGLES;
   GFXGLPrimType[GFXTriangleStrip] = GL_TRIANGLE_STRIP;
   GFXGLPrimType[GFXTriangleFan] = GL_TRIANGLE_FAN;

   // Blend
   GFXGLBlend[GFXBlendZero] = GL_ZERO;
   GFXGLBlend[GFXBlendOne] = GL_ONE;
   GFXGLBlend[GFXBlendSrcColor] = GL_SRC_COLOR;
   GFXGLBlend[GFXBlendInvSrcColor] = GL_ONE_MINUS_SRC_COLOR;
   GFXGLBlend[GFXBlendSrcAlpha] = GL_SRC_ALPHA;
   GFXGLBlend[GFXBlendInvSrcAlpha] = GL_ONE_MINUS_SRC_ALPHA;
   GFXGLBlend[GFXBlendDestAlpha] = GL_DST_ALPHA;
   GFXGLBlend[GFXBlendInvDestAlpha] = GL_ONE_MINUS_DST_ALPHA;
   GFXGLBlend[GFXBlendDestColor] = GL_DST_COLOR;
   GFXGLBlend[GFXBlendInvDestColor] = GL_ONE_MINUS_DST_COLOR;
   GFXGLBlend[GFXBlendSrcAlphaSat] = GL_SRC_ALPHA_SATURATE;
   
   // Blend op
   GFXGLBlendOp[GFXBlendOpAdd] = GL_FUNC_ADD;
   GFXGLBlendOp[GFXBlendOpSubtract] = GL_FUNC_SUBTRACT;
   GFXGLBlendOp[GFXBlendOpRevSubtract] = GL_FUNC_REVERSE_SUBTRACT;
   GFXGLBlendOp[GFXBlendOpMin] = GL_MIN;
   GFXGLBlendOp[GFXBlendOpMax] = GL_MAX;

   // Sampler
   GFXGLSamplerState[GFXSAMPMagFilter] = GL_TEXTURE_MAG_FILTER;
   GFXGLSamplerState[GFXSAMPMinFilter] = GL_TEXTURE_MIN_FILTER;
   GFXGLSamplerState[GFXSAMPAddressU] = GL_TEXTURE_WRAP_S;
   GFXGLSamplerState[GFXSAMPAddressV] = GL_TEXTURE_WRAP_T;
   GFXGLSamplerState[GFXSAMPAddressW] = GL_TEXTURE_WRAP_R;
   GFXGLSamplerState[GFXSAMPMipMapLODBias] = GL_TEXTURE_LOD_BIAS;
   
   // Comparison
   GFXGLCmpFunc[GFXCmpNever] = GL_NEVER;
   GFXGLCmpFunc[GFXCmpLess] = GL_LESS;
   GFXGLCmpFunc[GFXCmpEqual] = GL_EQUAL;
   GFXGLCmpFunc[GFXCmpLessEqual] = GL_LEQUAL;
   GFXGLCmpFunc[GFXCmpGreater] = GL_GREATER;
   GFXGLCmpFunc[GFXCmpNotEqual] = GL_NOTEQUAL;
   GFXGLCmpFunc[GFXCmpGreaterEqual] = GL_GEQUAL;
   GFXGLCmpFunc[GFXCmpAlways] = GL_ALWAYS;

   GFXGLTextureFilter[GFXTextureFilterNone] = GL_NEAREST;
   GFXGLTextureFilter[GFXTextureFilterPoint] = GL_NEAREST;
   GFXGLTextureFilter[GFXTextureFilterLinear] = GL_LINEAR;

   GFXGLTextureFilter[GFXTextureFilterAnisotropic] = GL_LINEAR;
   GFXGLTextureFilter[GFXTextureFilterPyramidalQuad] = GL_LINEAR; 
   GFXGLTextureFilter[GFXTextureFilterGaussianQuad] = GL_LINEAR;

   GFXGLTextureAddress[GFXAddressWrap] = GL_REPEAT;
   GFXGLTextureAddress[GFXAddressMirror] = GL_REPEAT;
   GFXGLTextureAddress[GFXAddressClamp] = GL_CLAMP_TO_EDGE;
   GFXGLTextureAddress[GFXAddressBorder] = GL_REPEAT;
   GFXGLTextureAddress[GFXAddressMirrorOnce] = GL_REPEAT;
   
   // Stencil ops
   GFXGLStencilOp[GFXStencilOpKeep] = GL_KEEP;
   GFXGLStencilOp[GFXStencilOpZero] = GL_ZERO;
   GFXGLStencilOp[GFXStencilOpReplace] = GL_REPLACE;
   GFXGLStencilOp[GFXStencilOpIncrSat] = GL_INCR;
   GFXGLStencilOp[GFXStencilOpDecrSat] = GL_DECR;
   GFXGLStencilOp[GFXStencilOpInvert] = GL_INVERT;
   
   GFXGLStencilOp[GFXStencilOpIncr] = GL_INCR_WRAP;
   GFXGLStencilOp[GFXStencilOpDecr] = GL_DECR_WRAP;
   
   
   // Texture formats
   GFXGLTextureInternalFormat[GFXFormatA8] = GL_R8;
   GFXGLTextureInternalFormat[GFXFormatL8] = GL_R8;
   GFXGLTextureInternalFormat[GFXFormatR5G6B5] = GL_RGB5_A1;  // OpenGL has no R5G6B5 format.
   GFXGLTextureInternalFormat[GFXFormatR5G5B5A1] = GL_RGB5_A1;
   GFXGLTextureInternalFormat[GFXFormatR5G5B5X1] = GL_RGB5_A1;
   GFXGLTextureInternalFormat[GFXFormatL16] = GL_R16;
   GFXGLTextureInternalFormat[GFXFormatR16F] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatD16] = GL_DEPTH_COMPONENT;
   GFXGLTextureInternalFormat[GFXFormatR8G8B8] = GL_RGB8;
   GFXGLTextureInternalFormat[GFXFormatR8G8B8A8] = GL_RGBA8;
   GFXGLTextureInternalFormat[GFXFormatR8G8B8X8] = GL_RGBA8;
   GFXGLTextureInternalFormat[GFXFormatR32F] = GL_ZERO; 
   GFXGLTextureInternalFormat[GFXFormatR16G16] = GL_RGBA16;
   GFXGLTextureInternalFormat[GFXFormatR16G16F] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatR10G10B10A2] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatD32] = GL_DEPTH_COMPONENT32;
   GFXGLTextureInternalFormat[GFXFormatD24X8] = GL_DEPTH_COMPONENT24;
   GFXGLTextureInternalFormat[GFXFormatD24S8] = GL_DEPTH24_STENCIL8;
   GFXGLTextureInternalFormat[GFXFormatR16G16B16A16] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatR16G16B16A16F] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatR32G32B32A32F] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatDXT1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
   GFXGLTextureInternalFormat[GFXFormatDXT2] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatDXT3] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   GFXGLTextureInternalFormat[GFXFormatDXT4] = GL_ZERO;
   GFXGLTextureInternalFormat[GFXFormatDXT5] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
   
   GFXGLTextureFormat[GFXFormatA8] = GL_RED;
   GFXGLTextureFormat[GFXFormatL8] = GL_RED;
   GFXGLTextureFormat[GFXFormatR5G6B5] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR5G5B5A1] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR5G5B5X1] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatL16] = GL_RED;
   GFXGLTextureFormat[GFXFormatR16F] = GL_ZERO;
   GFXGLTextureFormat[GFXFormatD16] = GL_DEPTH_COMPONENT;
   GFXGLTextureFormat[GFXFormatR8G8B8] = GL_RGB;
   GFXGLTextureFormat[GFXFormatR8G8B8A8] = GL_BGRA;
   GFXGLTextureFormat[GFXFormatR8G8B8X8] = GL_BGRA;
   GFXGLTextureFormat[GFXFormatR32F] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR16G16] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR16G16F] = GL_ZERO;
   GFXGLTextureFormat[GFXFormatR10G10B10A2] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatD32] = GL_DEPTH_COMPONENT;
   GFXGLTextureFormat[GFXFormatD24X8] = GL_DEPTH_COMPONENT;
   GFXGLTextureFormat[GFXFormatD24S8] = GL_DEPTH_STENCIL;
   GFXGLTextureFormat[GFXFormatR16G16B16A16] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR16G16B16A16F] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatR32G32B32A32F] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatDXT1] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatDXT2] = GL_ZERO;
   GFXGLTextureFormat[GFXFormatDXT3] = GL_RGBA;
   GFXGLTextureFormat[GFXFormatDXT4] = GL_ZERO;
   GFXGLTextureFormat[GFXFormatDXT5] = GL_RGBA;
   
   GFXGLTextureType[GFXFormatA8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatL8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR5G6B5] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR5G5B5A1] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR5G5B5X1] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatL16] = GL_UNSIGNED_SHORT;
   GFXGLTextureType[GFXFormatR16F] = GL_ZERO;
   GFXGLTextureType[GFXFormatD16] = GL_UNSIGNED_SHORT;
   GFXGLTextureType[GFXFormatR8G8B8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR8G8B8A8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR8G8B8X8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatR32F] = GL_FLOAT;
   GFXGLTextureType[GFXFormatR16G16] = GL_UNSIGNED_SHORT;
   GFXGLTextureType[GFXFormatR16G16F] = GL_FLOAT;
   GFXGLTextureType[GFXFormatR10G10B10A2] = GL_UNSIGNED_SHORT;
   GFXGLTextureType[GFXFormatD32] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatD24X8] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatD24S8] = GL_UNSIGNED_INT_24_8;
   GFXGLTextureType[GFXFormatR16G16B16A16] = GL_UNSIGNED_SHORT;
   GFXGLTextureType[GFXFormatR16G16B16A16F] = GL_FLOAT;
   GFXGLTextureType[GFXFormatR32G32B32A32F] = GL_FLOAT;
   GFXGLTextureType[GFXFormatDXT1] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatDXT2] = GL_ZERO;
   GFXGLTextureType[GFXFormatDXT3] = GL_UNSIGNED_BYTE;
   GFXGLTextureType[GFXFormatDXT4] = GL_ZERO;
   GFXGLTextureType[GFXFormatDXT5] = GL_UNSIGNED_BYTE;

   // Cull
   GFXGLCullMode[GFXCullNone] = GL_BACK;
   GFXGLCullMode[GFXCullCW] = GL_BACK;
   GFXGLCullMode[GFXCullCCW] = GL_FRONT;
   
   // Fill
   GFXGLFillMode[GFXFillPoint] = GL_POINT;
   GFXGLFillMode[GFXFillWireframe] = GL_LINE;
   GFXGLFillMode[GFXFillSolid] = GL_FILL;

   // A core profile has no GL_ALPHA or GL_LUMINANCE, so both are stored in red
   // and swizzled back into the channels the old formats delivered.
   //
   // Alpha reads as white rather than master's GL_NONE: the generic textured
   // shader multiplies colour by the sample, and rgb of zero takes text to
   // black. One reproduces what GL_MODULATE did for an alpha texture.
   static const GLint sSwizzleA8[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
   static const GLint sSwizzleL[]  = { GL_RED, GL_RED, GL_RED, GL_ONE };

   GFXGLTextureSwizzle[GFXFormatA8]  = sSwizzleA8;
   GFXGLTextureSwizzle[GFXFormatL8]  = sSwizzleL;
   GFXGLTextureSwizzle[GFXFormatL16] = sSwizzleL;
}
