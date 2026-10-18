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

// HH: Modified for Tribes 2 compatibility. The whole of this file is the
// binary's WaterBlock (waterBlock.cc) and the fluid it draws with
// (fluidRender.cc, fluidQuadTree.cc, fluidSupport.cc); Torque 3D's
// shader-driven water is not used for it.

#include "platform/platform.h"
#include "environment/waterBlock.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/bitmap/gBitmap.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneRenderState.h"
#include "scene/sceneManager.h"
#include "scene/fogStructs.h"
#include "terrain/terrData.h"
#include "terrain/terrFile.h"
#include "T3D/objectTypes.h"

IMPLEMENT_CO_NETOBJECT_V1(WaterBlock);

ConsoleDocClass( WaterBlock,
   "@brief Tribes 2's block of liquid: water, lava or quicksand.\n\n"
   "The position is a corner and the top face is the surface.\n\n"
   "@ingroup Water"
);

ImplementEnumType( WaterBlockLiquidType,
   "The liquid a WaterBlock holds.\n\n"
   "@ingroup Water" )
   { WaterBlock::eWater,         "Water" },
   { WaterBlock::eOceanWater,    "OceanWater" },
   { WaterBlock::eRiverWater,    "RiverWater" },
   { WaterBlock::eStagnantWater, "StagnantWater" },
   { WaterBlock::eLava,          "Lava" },
   { WaterBlock::eHotLava,       "HotLava" },
   { WaterBlock::eCrustyLava,    "CrustyLava" },
   { WaterBlock::eQuicksand,     "Quicksand" },
EndImplementEnumType;

GFXTexHandle WaterBlock::smSubmergeTex[WaterBlock::NumSubmergeTextures];
S32 WaterBlock::smEyeSubmergedType = -1;

/// The fluid works in terrain space, whose origin is the terrain's corner
/// rather than the world's.
static const F32 sFluidOffset = 1024.0f;

/// The binary's squares are the terrain's: 8 m.
static const F32 sSquareSize = 8.0f;

static F32 readF32( BitStream *stream )
{
   const U32 word = stream->readInt( 32 );
   F32 value;
   dMemcpy( &value, &word, sizeof( value ) );
   return value;
}

static void writeF32( BitStream *stream, F32 value )
{
   U32 word;
   dMemcpy( &word, &value, sizeof( word ) );
   stream->writeInt( word, 32 );
}

//-----------------------------------------------------------------------------

WaterBlock::WaterBlock()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   // The binary's constructor (0x5b4220).
   mObjBox.minExtents.set( 0.0f, 0.0f, 0.0f );
   mObjBox.maxExtents.set( 1.0f, 1.0f, 1.0f );

   mT2LiquidType     = eOceanWater;
   mDensity          = 1.0f;
   mViscosity        = 15.0f;
   mWaveAmplitude    = 1.0f;
   mSurfaceName      = StringTable->EmptyString();
   mSurfaceOpacity   = 0.75f;
   mEnvMapName       = StringTable->EmptyString();
   mEnvMapIntensity  = 1.0f;
   for ( U32 i = 0; i < NumSubmergeTextures; i++ )
      mSubmergeName[i] = StringTable->EmptyString();
   mRemoveWetEdges   = true;
   mAudioEnvironment = NULL;

   mSurfaceZ      = 0.0f;
   mSquareX0      = 0;
   mSquareY0      = 0;
   mSquaresInX    = 0;
   mSquaresInY    = 0;
   mHighRes       = true;
   mBlocksInX     = 0;
   mBlocksInY     = 0;
   mWetBlocksBuilt = false;

   // Nothing here reflects, and a reflector would draw the scene again.
   mFullReflect = false;
}

WaterBlock::~WaterBlock()
{
}

//-----------------------------------------------------------------------------

void WaterBlock::initPersistFields()
{
   // The binary's fields (0x5b4f60) and nothing of WaterObject's, whose
   // waveMagnitude is an array and whose liquidType is a string.
   SceneObject::initPersistFields();

   addField( "liquidType", TYPEID< WaterBlockLiquidType >(), Offset( mT2LiquidType, WaterBlock ),
      "The liquid, which the scripts' onEnterLiquid handlers are told by number." );
   addField( "density", TypeF32, Offset( mDensity, WaterBlock ), "" );
   addField( "viscosity", TypeF32, Offset( mViscosity, WaterBlock ), "" );
   addField( "waveMagnitude", TypeF32, Offset( mWaveAmplitude, WaterBlock ), "" );
   addField( "surfaceTexture", TypeString, Offset( mSurfaceName, WaterBlock ), "" );
   addField( "surfaceOpacity", TypeF32, Offset( mSurfaceOpacity, WaterBlock ), "" );
   addField( "envMapTexture", TypeString, Offset( mEnvMapName, WaterBlock ), "" );
   addField( "envMapIntensity", TypeF32, Offset( mEnvMapIntensity, WaterBlock ), "" );
   addField( "submergeTexture", TypeString, Offset( mSubmergeName, WaterBlock ),
      NumSubmergeTextures, "" );
   addField( "removeWetEdges", TypeBool, Offset( mRemoveWetEdges, WaterBlock ), "" );
   addField( "audioEnvironment", TypeSimObjectName, Offset( mAudioEnvironment, WaterBlock ), "" );
}

//-----------------------------------------------------------------------------

bool WaterBlock::onAdd()
{
   // The server snaps the block to the fluid's grid before it is ever
   // sent; a ghost arrives snapped.
   if ( isServerObject() )
      updateFluidRegion();

   if ( !Parent::onAdd() )
      return false;

   // A block that keeps its wet edges is a sea: anything below its surface,
   // anywhere, is in it.
   if ( !mRemoveWetEdges )
   {
      mObjBox.minExtents.set( -10000.0f, -10000.0f, 0.0f );
      mObjBox.maxExtents.set(  10000.0f,  10000.0f, 1.0f );
   }

   resetWorldBox();
   addToScene();

   if ( isClientObject() )
   {
      loadTextures();
      initStateBlocks();
   }

   return true;
}

void WaterBlock::onRemove()
{
   if ( isClientObject() )
   {
      for ( U32 i = 0; i < NumSubmergeTextures; i++ )
         smSubmergeTex[i] = NULL;
   }

   removeFromScene();

   Parent::onRemove();
}

void WaterBlock::inspectPostApply()
{
   Parent::inspectPostApply();

   updateFluidRegion();
   setMaskBits( UpdateMask );
}

void WaterBlock::setTransform( const MatrixF &mat )
{
   mObjToWorld = mat;
   updateFluidRegion();
}

void WaterBlock::setScale( const Point3F &scale )
{
   mObjScale = scale;
   updateFluidRegion();
}

//-----------------------------------------------------------------------------

U32 WaterBlock::packUpdate( NetConnection *, U32, BitStream *stream, bool )
{
   // The binary's layout (0x5b5100): every update carries the lot.
   stream->writeAffineTransform( mObjToWorld );
   mathWrite( *stream, mObjScale );

   stream->writeString( mSurfaceName );
   stream->writeString( mEnvMapName );
   for ( U32 i = 0; i < NumSubmergeTextures; i++ )
      stream->writeString( mSubmergeName[i] );

   stream->writeInt( (U32)mT2LiquidType, 32 );
   writeF32( stream, mDensity );
   writeF32( stream, mViscosity );
   writeF32( stream, mWaveAmplitude );
   writeF32( stream, mSurfaceOpacity );
   writeF32( stream, mEnvMapIntensity );
   stream->writeInt( mRemoveWetEdges ? 1 : 0, 8 );

   SimDataBlock *environment = dynamic_cast<SimDataBlock*>( mAudioEnvironment );
   if ( stream->writeFlag( environment != NULL ) )
      stream->writeInt( environment->getId() - DataBlockObjectIdFirst,
                        DataBlockObjectIdBitSize );

   return 0;
}

bool WaterBlock::unpackUpdate( BitStream *stream, bool )
{
   // The binary's reader (0x5b52b0).
   MatrixF mat;
   stream->readAffineTransform( &mat );
   Point3F scale;
   mathRead( *stream, &scale );

   char buf[256];
   stream->readString( buf );
   mSurfaceName = StringTable->insert( buf );
   stream->readString( buf );
   mEnvMapName = StringTable->insert( buf );
   for ( U32 i = 0; i < NumSubmergeTextures; i++ )
   {
      stream->readString( buf );
      mSubmergeName[i] = StringTable->insert( buf );
   }

   const S32 type = (S32)stream->readInt( 32 );
   mDensity         = readF32( stream );
   mViscosity       = readF32( stream );
   mWaveAmplitude   = readF32( stream );
   mSurfaceOpacity  = readF32( stream );
   mEnvMapIntensity = readF32( stream );
   mRemoveWetEdges  = stream->readInt( 8 ) != 0;

   mAudioEnvironment = NULL;
   if ( stream->readFlag() )
   {
      const SimObjectId id = stream->readInt( DataBlockObjectIdBitSize ) + DataBlockObjectIdFirst;
      Sim::findObject( id, mAudioEnvironment );
   }

   mT2LiquidType = (EWaterType)mClamp( type, (S32)eWater, (S32)eQuicksand );

   mObjToWorld = mat;
   mObjScale = scale;
   updateFluidRegion();

   return true;
}

//-----------------------------------------------------------------------------

void WaterBlock::updateFluidRegion()
{
   // The binary's UpdateFluidRegion (0x5b4490): snap the corner and the
   // extent to the fluid's grid, drop any rotation, and put the surface on
   // top of the box.
   const Point3F pos = mObjToWorld.getPosition();

   F32 x0 = pos.x + sFluidOffset;
   F32 y0 = pos.y + sFluidOffset;
   F32 sizeX = mObjScale.x;
   F32 sizeY = mObjScale.y;

   mSurfaceZ = pos.z + mObjScale.z;
   setFluidInfo( x0, y0, sizeX, sizeY );

   mObjScale.x = sizeX;
   mObjScale.y = sizeY;

   MatrixF mat( true );
   mat.setPosition( Point3F( x0 - sFluidOffset, y0 - sFluidOffset, pos.z ) );
   SceneObject::setTransform( mat );
   resetWorldBox();

   if ( isServerObject() )
      setMaskBits( UpdateMask );

   mWetBlocksBuilt = false;
}

void WaterBlock::setFluidInfo( F32 &x0, F32 &y0, F32 &sizeX, F32 &sizeY )
{
   // The binary's fluid::SetInfo (0x5b63a0).
   mSurfaceOpacity  = mClampF( mSurfaceOpacity, 0.0f, 1.0f );
   mEnvMapIntensity = mClampF( mEnvMapIntensity, 0.0f, 1.0f );

   mSquareX0 = mClamp( (S32)( x0 * 0.125f + 0.5f ), 0, 2040 );
   mSquareY0 = mClamp( (S32)( y0 * 0.125f + 0.5f ), 0, 2040 );

   mSquaresInX = (S32)( sizeX * 0.125f + 0.5f );
   mSquaresInY = (S32)( sizeY * 0.125f + 0.5f );

   // A small block is cut into blocks of four squares, a large one into
   // blocks of eight; either way there are at most 32 blocks a side.
   mHighRes = mSquaresInX <= 128 && mSquaresInY <= 128;
   if ( mHighRes )
   {
      mSquaresInX = ( mSquaresInX + 3 ) & ~3;
      mSquaresInY = ( mSquaresInY + 3 ) & ~3;
      if ( mSquaresInX <= 0 ) mSquaresInX = 4;
      if ( mSquaresInY <= 0 ) mSquaresInY = 4;
      mBlocksInX = mSquaresInX >> 2;
      mBlocksInY = mSquaresInY >> 2;
   }
   else
   {
      mSquaresInX = getMin( ( mSquaresInX + 7 ) & ~7, 256 );
      mSquaresInY = getMin( ( mSquaresInY + 7 ) & ~7, 256 );
      if ( mSquaresInX <= 0 ) mSquaresInX = 8;
      if ( mSquaresInY <= 0 ) mSquaresInY = 8;
      mBlocksInX = mSquaresInX >> 3;
      mBlocksInY = mSquaresInY >> 3;
   }

   x0 = mSquareX0 * sSquareSize;
   y0 = mSquareY0 * sSquareSize;
   sizeX = mSquaresInX * sSquareSize;
   sizeY = mSquaresInY * sSquareSize;
}

//-----------------------------------------------------------------------------

static void findTerrainCallback( SceneObject *obj, void *key )
{
   SimObjectPtr<TerrainBlock> *terrain = static_cast<SimObjectPtr<TerrainBlock>*>( key );
   if ( terrain->isNull() )
      *terrain = dynamic_cast<TerrainBlock*>( obj );
}

bool WaterBlock::findTerrain()
{
   if ( mTerrain.isNull() && getContainer() )
      getContainer()->findObjects( getWorldBox(), TerrainObjectType,
                                   findTerrainCallback, &mTerrain );

   return !mTerrain.isNull();
}

void WaterBlock::buildWetBlocks()
{
   // The binary's fluid::BuildDryMap (0x5b6120). A corner of a square is wet
   // where the surface, half a wave up, is above the terrain there.
   const S32 n = mHighRes ? 4 : 8;
   const S32 w = mSquaresInX + 1;
   const S32 h = mSquaresInY + 1;

   mWetBlocks.setSize( mBlocksInX * mBlocksInY );

   TerrainFile *file = mTerrain.isNull() ? NULL : mTerrain->getFile();
   if ( !file )
   {
      for ( S32 i = 0; i < mWetBlocks.size(); i++ )
         mWetBlocks[i] = true;
      return;
   }

   const U16 level = (U16)(S32)( ( mSurfaceZ + mWaveAmplitude * 0.5f ) * 32.0f );

   Vector<U8> wet;
   wet.setSize( w * h );
   for ( S32 y = 0; y < h; y++ )
      for ( S32 x = 0; x < w; x++ )
         wet[ y * w + x ] = level > file->getHeight( ( mSquareX0 + x ) & 255,
                                                     ( mSquareY0 + y ) & 255 );

   // Removing the wet edges takes away every pool that reaches the edge of
   // the block, which leaves the ones the terrain closes in.
   if ( mRemoveWetEdges )
   {
      Vector<S32> stack;
      for ( S32 i = 0; i < 2 * ( w + h ); i++ )
      {
         S32 sx, sy;
         if ( i < w )               { sx = i;            sy = 0; }
         else if ( i < 2 * w )      { sx = i - w;        sy = h - 1; }
         else if ( i < 2 * w + h )  { sx = 0;            sy = i - 2 * w; }
         else                       { sx = w - 1;        sy = i - 2 * w - h; }

         stack.push_back( sy * w + sx );
         while ( stack.size() )
         {
            const S32 at = stack.last();
            stack.pop_back();
            if ( !wet[at] )
               continue;

            wet[at] = 0;
            const S32 x = at % w;
            const S32 y = at / w;
            if ( x > 0 )     stack.push_back( at - 1 );
            if ( x < w - 1 ) stack.push_back( at + 1 );
            if ( y > 0 )     stack.push_back( at - w );
            if ( y < h - 1 ) stack.push_back( at + w );
         }
      }
   }

   // A block is drawn whole if any corner in it is wet.
   for ( S32 by = 0; by < mBlocksInY; by++ )
      for ( S32 bx = 0; bx < mBlocksInX; bx++ )
      {
         bool any = false;
         for ( S32 j = 0; j <= n && !any; j++ )
            for ( S32 i = 0; i <= n && !any; i++ )
               any = wet[ ( by * n + j ) * w + bx * n + i ] != 0;
         mWetBlocks[ by * mBlocksInX + bx ] = any;
      }

   mWetBlocksBuilt = true;
}

bool WaterBlock::isFluidAtXY( F32 x, F32 y ) const
{
   // The binary's fluid::IsFluidAtXY (0x5b6890), in terrain space: the
   // fluid repeats every 2048 m, as the terrain does.
   const S32 shift = mHighRes ? 5 : 6;
   const S32 bx = ( ( (S32)x - mSquareX0 * 8 ) & 0x7ff ) >> shift;
   const S32 by = ( ( (S32)y - mSquareY0 * 8 ) & 0x7ff ) >> shift;

   if ( bx >= mBlocksInX || by >= mBlocksInY )
      return false;

   if ( mWetBlocks.size() != mBlocksInX * mBlocksInY )
      return true;

   return mWetBlocks[ by * mBlocksInX + bx ];
}

F32 WaterBlock::getWaveHeight( F32 x, F32 y, F32 seconds ) const
{
   // The wave the binary puts on every vertex (0x5b8f60), a quarter of the
   // magnitude a side.
   return mSurfaceZ + ( mSin( 0.05f * y + seconds ) + mSin( 0.05f * x + seconds ) )
                      * ( 0.25f * mWaveAmplitude );
}

//-----------------------------------------------------------------------------

GFXTexHandle WaterBlock::loadTexture( StringTableEntry name ) const
{
   if ( !name || !name[0] )
      return GFXTexHandle();

   Vector<Torque::Path> found;
   Torque::Path path( String( "textures/" ) + name );
   if ( GBitmap::sFindFiles( path, &found ) && found.size() )
      return GFXTexHandle( found.first().getFullPath(), &GFXDefaultStaticDiffuseProfile,
                           "WaterBlock" );

   Con::warnf( "WaterBlock: no texture for '%s'.", name );
   return GFXTexHandle();
}

void WaterBlock::loadTextures()
{
   // The binary's onAdd (0x5b45f0): the newest block's submerge textures are
   // the ones the screen filter draws.
   mSurfaceTex = loadTexture( mSurfaceName );
   mEnvMapTex  = loadTexture( mEnvMapName );

   for ( U32 i = 0; i < NumSubmergeTextures; i++ )
   {
      if ( !mSubmergeName[i] || !mSubmergeName[i][0] )
         continue;

      mLocalSubmergeTex[i] = loadTexture( mSubmergeName[i] );
      smSubmergeTex[i] = mLocalSubmergeTex[i];
   }
}

void WaterBlock::initStateBlocks()
{
   // Every pass tests depth and none writes it, from either side.
   GFXStateBlockDesc d;
   d.setCullMode( GFXCullNone );
   d.setZReadWrite( true, false );
   d.samplersDefined = true;
   d.samplers[0] = GFXSamplerStateDesc::getWrapLinear();

   d.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   mBlendSB = GFX->createStateBlock( d );

   d.setBlend( true, GFXBlendSrcAlpha, GFXBlendOne );
   mAddSB = GFX->createStateBlock( d );

   d.samplersDefined = false;
   d.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   mFogSB = GFX->createStateBlock( d );
}

//-----------------------------------------------------------------------------

void WaterBlock::prepRenderImage( SceneRenderState *state )
{
   if ( !state->isDiffusePass() )
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &WaterBlock::renderFluid );
   ri->type = RenderPassManager::RIT_Water;
   ri->defaultKey = 1;
   state->getRenderPass()->addInst( ri );
}

/// What the binary keeps per fluid vertex (68 bytes there), less the colours
/// that are always white.
struct FluidVertex
{
   Point3F  pos;
   Point2F  base1;
   Point2F  base2;
   F32      alpha1;
   F32      alpha2;
   Point2F  env;
   F32      fog;
};

static F32 fogBand( const Point3F &eye, const Point3F &pos, F32 bandMin, F32 bandMax,
                    F32 factor, F32 dist )
{
   const F32 dz = pos.z - eye.z;
   const F32 inBand = ( eye.z >= bandMin && eye.z <= bandMax ) ? 1.0f : 0.0f;
   const F32 frac = mFabs( dz ) > 1e-4f
      ? ( mClampF( pos.z, bandMin, bandMax ) - mClampF( eye.z, bandMin, bandMax ) ) / dz
      : inBand;
   return dist * frac * factor;
}

/// The fog the scene's shaders give a point (torque.glsl), as an alpha: the
/// binary fogs the fluid with the scene's own getHazeAndFog (0x5b41c0).
static F32 fogAlpha( const FogData &fog, const Point3F &eye, const Point3F &pos )
{
   const F32 dist = ( pos - eye ).len();

   const F32 falloff = mIsZero( fog.atmosphereHeight ) ? 0.0f : 1.0f / fog.atmosphereHeight;
   F32 amount = mExp( -fog.density * ( dist - fog.densityOffset ) * ( 1.0f - pos.z * falloff ) );

   for ( U32 i = 0; i < FogData::MaxBands; i++ )
      amount -= fogBand( eye, pos, fog.bandMin[i], fog.bandMax[i], fog.bandFactor[i], dist );

   const F32 t = mClampF( ( dist - fog.densityOffset ) * fog.hazeScale, 0.0f, 1.0f ) - 1.0f;
   amount -= 1.0f - t * t;

   return 1.0f - mClampF( amount, 0.0f, 1.0f );
}

/// What a pass needs that is the same for every vertex.
struct FluidPassInfo
{
   GFXTexHandle      surfaceTex;
   GFXTexHandle      envMapTex;
   GFXStateBlockRef  blendSB;
   GFXStateBlockRef  addSB;
   GFXStateBlockRef  fogSB;
   F32               driftX;
   F32               driftY;
   F32               sin60;
   F32               cos60;
   F32               envMapIntensity;
   ColorI            fogColor;
};

/// The binary's four passes over one batch: the base texture twice, the
/// environment map added, and the fog laid over all of it.
static void drawFluidPasses( const FluidPassInfo &info, const Vector<FluidVertex> &verts,
                             const Vector<U16> &indices )
{
   if ( indices.empty() )
      return;

   GFXPrimitiveBufferHandle pb;
   pb.set( GFX, indices.size(), 0, GFXBufferTypeVolatile );
   U16 *ip;
   pb.lock( &ip );
   dMemcpy( ip, indices.address(), indices.size() * sizeof( U16 ) );
   pb.unlock();

   const U32 numVerts = verts.size();
   const U32 numPrims = indices.size() / 3;

   for ( U32 pass = 0; pass < 4; pass++ )
   {
      GFXTexHandle tex;
      if ( pass < 2 )
         tex = info.surfaceTex;
      else if ( pass == 2 )
         tex = info.envMapTex;

      if ( pass < 3 && tex.isNull() )
         continue;

      GFXVertexBufferHandle<GFXVertexPCT> vb( GFX, numVerts, GFXBufferTypeVolatile );
      GFXVertexPCT *out = vb.lock();
      if ( !out )
         return;

      for ( U32 i = 0; i < numVerts; i++ )
      {
         const FluidVertex &v = verts[i];
         out[i].point = v.pos;
         switch ( pass )
         {
         case 0:
            out[i].texCoord = v.base1;
            out[i].color = ColorI( 255, 255, 255, (U8)( v.alpha1 * 255.0f + 0.5f ) );
            break;

         case 1:
            out[i].texCoord = v.base2;
            out[i].color = ColorI( 255, 255, 255, (U8)( v.alpha2 * 255.0f + 0.5f ) );
            break;

         case 2:
            out[i].texCoord = v.env;
            out[i].color = ColorI( 255, 255, 255, (U8)( info.envMapIntensity * 255.0f + 0.5f ) );
            break;

         default:
            out[i].texCoord.set( 0.0f, 0.0f );
            out[i].color = ColorI( info.fogColor.red, info.fogColor.green, info.fogColor.blue,
                                   (U8)( v.fog * 255.0f + 0.5f ) );
            break;
         }
      }
      vb.unlock();

      if ( pass < 3 )
      {
         GFX->setStateBlock( pass == 2 ? info.addSB : info.blendSB );
         GFX->setTexture( 0, tex );
         GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
      }
      else
      {
         GFX->setStateBlock( info.fogSB );
         GFX->setTexture( 0, NULL );
         GFX->setupGenericShaders( GFXDevice::GSColor );
      }

      GFX->setVertexBuffer( vb );
      GFX->setPrimitiveBuffer( pb );
      GFX->drawIndexedPrimitive( GFXTriangleList, 0, 0, numVerts, 0, numPrims );
   }
}

void WaterBlock::renderFluid( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   if ( overrideMat )
      return;

   GFXDEBUGEVENT_SCOPE( WaterBlock_RenderFluid, ColorF::WHITE );

   if ( !mWetBlocksBuilt )
   {
      findTerrain();
      buildWetBlocks();
   }

   // The binary's fluid::Render (0x5b9520) and the vertices its quad tree
   // builds (0x5b8f60), at the tree's finest: an 8 m grid.
   const F32 seconds = Platform::getVirtualMilliseconds() * 0.001f;
   const Point3F camPos = state->getCameraPosition();
   const Point3F eye( camPos.x + sFluidOffset, camPos.y + sFluidOffset, camPos.z );

   const F32 eyeWave = getWaveHeight( eye.x, eye.y, seconds );
   if ( eye.z < eyeWave && isFluidAtXY( eye.x, eye.y ) )
      smEyeSubmergedType = mT2LiquidType;

   SceneManager *scene = state->getSceneManager();
   const FogData &fog = scene->getFogData();
   const F32 visibleDistance = scene->getVisibleDistance();

   // Both base layers are turned, the second twice and drifting.
   FluidPassInfo info;
   info.surfaceTex = mSurfaceTex;
   info.envMapTex = mEnvMapTex;
   info.blendSB = mBlendSB;
   info.addSB = mAddSB;
   info.fogSB = mFogSB;
   info.driftX = 0.02f * seconds;
   info.driftY = 0.03f * mCos( mFmod( seconds * M_PI_F / 4.0f, M_2PI_F ) );
   mSinCos( mDegToRad( 60.0f ), info.sin60, info.cos60 );
   info.envMapIntensity = mEnvMapIntensity;
   info.fogColor = ColorI( fog.color );

   F32 sin30, cos30;
   mSinCos( mDegToRad( 30.0f ), sin30, cos30 );

   // Near the eye the surface is pushed a quarter metre clear of it, fully
   // inside the first distance and fading out by the second.
   const F32 nearFull = mHighRes ? 8.0f : 16.0f;
   const F32 nearFade = mHighRes ? 16.0f : 32.0f;
   const bool eyeAbove = eye.z > eyeWave;
   const F32 clearZ = eyeAbove ? eye.z - 0.25f : eye.z + 0.25f;

   const S32 n = mHighRes ? 4 : 8;
   const S32 side = n + 1;
   const F32 blockSize = n * sSquareSize;
   const F32 originX = mSquareX0 * sSquareSize;
   const F32 originY = mSquareY0 * sSquareSize;

   // The wet blocks in sight, of this copy of the fluid and the copies
   // around it.
   static Vector<Point2F> blocks;
   blocks.clear();

   const S32 tileX = (S32)mFloor( ( eye.x - originX ) / 2048.0f );
   const S32 tileY = (S32)mFloor( ( eye.y - originY ) / 2048.0f );

   for ( S32 ty = tileY - 1; ty <= tileY + 1; ty++ )
      for ( S32 tx = tileX - 1; tx <= tileX + 1; tx++ )
         for ( S32 by = 0; by < mBlocksInY; by++ )
            for ( S32 bx = 0; bx < mBlocksInX; bx++ )
            {
               if ( mWetBlocks.size() && !mWetBlocks[ by * mBlocksInX + bx ] )
                  continue;

               const F32 x0 = originX + tx * 2048.0f + bx * blockSize;
               const F32 y0 = originY + ty * 2048.0f + by * blockSize;

               const F32 nearX = mClampF( eye.x, x0, x0 + blockSize ) - eye.x;
               const F32 nearY = mClampF( eye.y, y0, y0 + blockSize ) - eye.y;
               if ( nearX * nearX + nearY * nearY <= visibleDistance * visibleDistance )
                  blocks.push_back( Point2F( x0, y0 ) );
            }

   static Vector<FluidVertex> verts;
   static Vector<U16> indices;
   verts.clear();
   indices.clear();

   for ( S32 b = 0; b < blocks.size(); b++ )
   {
      // Draw what is gathered before the indices would overflow.
      if ( verts.size() + side * side > 65535 )
      {
         drawFluidPasses( info, verts, indices );
         verts.clear();
         indices.clear();
      }

      const U16 first = (U16)verts.size();
      for ( S32 j = 0; j < side; j++ )
         for ( S32 i = 0; i < side; i++ )
         {
            const F32 x = blocks[b].x + i * sSquareSize;
            const F32 y = blocks[b].y + j * sSquareSize;
            const F32 dx = x - eye.x;
            const F32 dy = y - eye.y;
            const F32 flatDz = mSurfaceZ - eye.z;
            const F32 dist = mSqrt( dx * dx + dy * dy + flatDz * flatDz );

            F32 z = getWaveHeight( x, y, seconds );
            if ( dist < nearFade && ( eyeAbove ? z > clearZ : z < clearZ ) )
            {
               if ( dist < nearFull )
                  z = clearZ;
               else
               {
                  const F32 t = ( nearFade - dist ) / nearFull;
                  z = ( 1.0f - t ) * z + t * clearZ;
               }
            }

            FluidVertex v;
            v.pos.set( x - sFluidOffset, y - sFluidOffset, z );

            // Texgen at a 48 m tile, turned through the texture matrix.
            const F32 s = x / 48.0f;
            const F32 t = y / 48.0f;
            v.base1.set( s * cos30 - t * sin30, s * sin30 + t * cos30 );
            const F32 s2 = s + info.driftX;
            const F32 t2 = t + info.driftY;
            v.base2.set( s2 * info.cos60 - t2 * info.sin60, s2 * info.sin60 + t2 * info.cos60 );

            // A sphere-ish map of the eye's ray, wobbled.
            F32 dz = mFabs( z - eye.z );
            if ( dz < 0.001f )
               dz = 0.001f;
            Point2F env( 0.0f, 0.0f );
            if ( dist >= 0.001f )
            {
               const F32 k = ( dist - dz ) / ( dist * dist );
               env.set( dx * k, dy * k );
            }
            env.x = env.x * 0.5f + 0.5f;
            env.y = env.y * 0.5f + 0.5f;

            const F32 wobbleX = mCos( ( seconds / 2.0f + x / 150.0f ) * 6.0f );
            const F32 wobbleY = mSin( ( seconds / 2.0f + y / 150.0f ) * 6.28f );
            env.x += wobbleX * 0.01f;
            env.y += wobbleY * 0.01f;
            v.env = env;

            // The two base layers share the opacity between them so that,
            // stacked, they come to exactly the surface's opacity.
            const F32 q = ( wobbleX + wobbleY ) * 0.15f + 0.5f;
            const F32 opacity = mSurfaceOpacity;
            v.alpha2 = q * opacity;
            v.alpha1 = ( 1.0f - q ) * opacity / ( 1.0f - q * opacity );

            v.fog = fogAlpha( fog, camPos, Point3F( v.pos.x, v.pos.y, mSurfaceZ ) );

            verts.push_back( v );
         }

      for ( S32 j = 0; j < n; j++ )
         for ( S32 i = 0; i < n; i++ )
         {
            const U16 a = first + j * side + i;
            const U16 b1 = a + 1;
            const U16 c = a + side + 1;
            const U16 d = a + side;
            indices.push_back( a ); indices.push_back( b1 ); indices.push_back( c );
            indices.push_back( a ); indices.push_back( c ); indices.push_back( d );
         }
   }

   drawFluidPasses( info, verts, indices );
}

//-----------------------------------------------------------------------------

void WaterBlock::renderSubmergeFilter()
{
   // The tail of the binary's GameRenderFilters (0x5bc257) and the filter it
   // calls (0x69f510): a 50 by 50 grid over the view, each layer of it with
   // its texture coordinates swimming on a sine.
   const S32 type = smEyeSubmergedType;
   smEyeSubmergedType = -1;

   if ( !isLava( type ) || !GFX )
      return;

   static GFXStateBlockRef sb;
   if ( sb.isNull() )
   {
      GFXStateBlockDesc d;
      d.setCullMode( GFXCullNone );
      d.setZReadWrite( false, false );
      d.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
      d.samplersDefined = true;
      d.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      sb = GFX->createStateBlock( d );
   }

   const RectI &viewport = GFX->getViewport();
   if ( viewport.extent.y <= 0 )
      return;

   // The binary divides the extent as integers, so the width is one for any
   // screen wider than it is tall and narrower than twice that.
   const S32 aspect = viewport.extent.x / viewport.extent.y;
   const S32 cols = (S32)( aspect * 50.0f );
   const S32 rows = 50;
   if ( cols < 2 )
      return;

   const F64 time = Sim::getCurrentTime() * 0.001;
   const F32 drift = (F32)( time * 0.025 );
   const F32 amplitude = 0.02f;
   const F32 frequency = 2.0f;
   F32 phase = (F32)( time * 2.0 );

   GFXTransformSaver saver;
   GFX->setProjectionMatrix( MatrixF::Identity );
   GFX->setViewMatrix( MatrixF::Identity );
   GFX->setWorldMatrix( MatrixF::Identity );
   GFX->setStateBlock( sb );

   const U32 numVerts = cols * ( rows + 1 );
   GFXPrimitiveBufferHandle pb;
   pb.set( GFX, ( cols - 1 ) * rows * 6, 0, GFXBufferTypeVolatile );
   U16 *ip;
   pb.lock( &ip );
   for ( S32 j = 0; j < rows; j++ )
      for ( S32 i = 0; i < cols - 1; i++ )
      {
         const U16 a = j * cols + i;
         *ip++ = a;  *ip++ = a + 1;  *ip++ = a + cols + 1;
         *ip++ = a;  *ip++ = a + cols + 1;  *ip++ = a + cols;
      }
   pb.unlock();

   for ( U32 layer = 0; layer < NumSubmergeTextures; layer++ )
   {
      if ( layer )
         phase += 10.0f;

      if ( smSubmergeTex[layer].isNull() )
         continue;

      GFXVertexBufferHandle<GFXVertexPCT> vb( GFX, numVerts, GFXBufferTypeVolatile );
      GFXVertexPCT *v = vb.lock();
      if ( !v )
         return;

      for ( S32 j = 0; j <= rows; j++ )
         for ( S32 i = 0; i < cols; i++, v++ )
         {
            const F32 fx = (F32)i / (F32)( cols - 1 ) * (F32)aspect;
            const F32 fy = (F32)j / (F32)( rows - 1 );
            v->point.set( i * 2.0f / ( cols - 1 ) - 1.0f, j * 2.0f / ( rows - 1 ) - 1.0f, 0.0f );
            v->texCoord.set( fx + mSin( fx * M_2PI_F * frequency + phase ) * amplitude + drift,
                             fy + mSin( fy * M_2PI_F * frequency + phase ) * amplitude + drift );
            // The binary draws both layers at half alpha, modulated
            // (glColor4f at 0x69f5d7, GL_MODULATE at 0x69f618), so the
            // view and the first layer show through the second.
            v->color = ColorI( 255, 255, 255, 128 );
         }
      vb.unlock();

      GFX->setTexture( 0, smSubmergeTex[layer] );
      GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
      GFX->setVertexBuffer( vb );
      GFX->setPrimitiveBuffer( pb );
      GFX->drawIndexedPrimitive( GFXTriangleList, 0, 0, numVerts, 0, ( cols - 1 ) * rows * 2 );
   }
}

//-----------------------------------------------------------------------------

bool WaterBlock::castRay( const Point3F &start, const Point3F &end, RayInfo *info )
{
   // In object space the box is the unit cube and the surface its top.
   const Point3F norm( 0, 0, 1 );
   PlaneF plane( Point3F( 0.0f, 0.0f, 1.0f ), norm );

   F32 hit = plane.intersect( start, end );
   if ( hit < 0.0f || hit > 1.0f )
      return false;

   info->t = hit;
   info->object = this;
   info->point = start + ( ( end - start ) * hit );
   info->normal = norm;
   info->material = NULL;

   return info->point.x >= mObjBox.minExtents.x && info->point.x <= mObjBox.maxExtents.x &&
          info->point.y >= mObjBox.minExtents.y && info->point.y <= mObjBox.maxExtents.y;
}

F32 WaterBlock::getWaterCoverage( const Box3F &testBox ) const
{
   // The top of the box is the surface.
   const Box3F &wbox = getWorldBox();

   F32 coverage = 0.0f;
   if ( wbox.isOverlapped( testBox ) )
   {
      if ( wbox.maxExtents.z < testBox.maxExtents.z )
         coverage = ( wbox.maxExtents.z - testBox.minExtents.z ) /
                    ( testBox.maxExtents.z - testBox.minExtents.z );
      else
         coverage = 1.0f;
   }

   return coverage;
}

F32 WaterBlock::getSurfaceHeight( const Point2F &pos ) const
{
   if ( !mWorldBox.isContained( pos ) )
      return -1.0f;

   return mWorldBox.maxExtents.z;
}

bool WaterBlock::isUnderwater( const Point3F &pnt ) const
{
   return mWorldBox.isContained( pnt );
}

const char* WaterBlock::getLiquidType() const
{
   // The scripts compare the type with numbers, which is what the binary
   // hands them.
   static const char *sNumbers[] = { "0", "1", "2", "3", "4", "5", "6", "7" };
   return sNumbers[ mClamp( (S32)mT2LiquidType, (S32)eWater, (S32)eQuicksand ) ];
}

F32 WaterBlock::distanceTo( const Point3F& point ) const
{
   return getWorldBox().getDistanceToPoint( point );
}
