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

#ifndef _WATERBLOCK_H_
#define _WATERBLOCK_H_

#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _SCENEDATA_H_
#include "materials/sceneData.h"
#endif
#ifndef _MATINSTANCE_H_
#include "materials/matInstance.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif
#ifndef _WATEROBJECT_H_
#include "waterObject.h"
#endif

class TerrainBlock;

//*****************************************************************************
// WaterBlock
//*****************************************************************************

// HH: Modified for Tribes 2 compatibility.
/// Tribes 2's WaterBlock: a box whose corner is its position and whose top
/// face is the surface, drawn as the fluid the binary draws -- two drifting
/// layers of one tiled texture, an additive environment map and the scene's
/// fog, over an 8 m grid clipped to where the terrain is below the surface.
/// Torque 3D's own water rendering is not used.
class WaterBlock : public WaterObject
{
   typedef WaterObject Parent;

public:

   /// The binary's liquid types, in its order: the scripts' onEnterLiquid
   /// handlers switch on the number.
   enum EWaterType
   {
      eWater            = 0,
      eOceanWater       = 1,
      eRiverWater       = 2,
      eStagnantWater    = 3,
      eLava             = 4,
      eHotLava          = 5,
      eCrustyLava       = 6,
      eQuicksand        = 7,
   };

   enum
   {
      NumSubmergeTextures = 2,
   };

   static bool isWater( S32 type ) { return type >= eWater && type <= eStagnantWater; }
   static bool isLava( S32 type ) { return type >= eLava && type <= eCrustyLava; }

private:

   enum MaskBits {
      UpdateMask =   Parent::NextFreeMask,
      NextFreeMask = Parent::NextFreeMask << 1
   };

   // The fields the binary registers, in its order.
   EWaterType        mT2LiquidType;
   F32               mWaveAmplitude;
   StringTableEntry  mSurfaceName;
   F32               mSurfaceOpacity;
   StringTableEntry  mEnvMapName;
   F32               mEnvMapIntensity;
   StringTableEntry  mSubmergeName[NumSubmergeTextures];
   bool              mRemoveWetEdges;
   SimObject        *mAudioEnvironment;

   // The fluid, in the binary's 8 m squares.
   F32   mSurfaceZ;
   S32   mSquareX0;
   S32   mSquareY0;
   S32   mSquaresInX;
   S32   mSquaresInY;
   bool  mHighRes;
   S32   mBlocksInX;
   S32   mBlocksInY;

   /// One entry per block, true where some corner of it is below the
   /// surface. Empty until a terrain has been found to test against.
   Vector<bool>   mWetBlocks;
   bool           mWetBlocksBuilt;
   SimObjectPtr<TerrainBlock> mTerrain;

   GFXTexHandle      mSurfaceTex;
   GFXTexHandle      mEnvMapTex;
   GFXTexHandle      mLocalSubmergeTex[NumSubmergeTextures];
   GFXStateBlockRef  mBlendSB;
   GFXStateBlockRef  mAddSB;
   GFXStateBlockRef  mFogSB;

   /// The submerge textures of the last block added, which the binary keeps
   /// in a static for the screen filter.
   static GFXTexHandle smSubmergeTex[NumSubmergeTextures];

   /// The liquid the camera was found in while the fluid drew, -1 for none.
   /// The screen filter reads it once a frame and clears it.
   static S32 smEyeSubmergedType;

   void updateFluidRegion();
   void setFluidInfo( F32 &x0, F32 &y0, F32 &sizeX, F32 &sizeY );
   void buildWetBlocks();
   bool findTerrain();
   bool isFluidAtXY( F32 x, F32 y ) const;
   F32  getWaveHeight( F32 x, F32 y, F32 seconds ) const;
   void loadTextures();
   GFXTexHandle loadTexture( StringTableEntry name ) const;
   void initStateBlocks();

   void renderFluid( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );

protected:

   //-------------------------------------------------------
   // Standard engine functions
   //-------------------------------------------------------
   bool onAdd();
   void onRemove();

   bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);

public:
   WaterBlock();
   virtual ~WaterBlock();

   DECLARE_CONOBJECT(WaterBlock);

   static void initPersistFields();
   virtual void inspectPostApply();
   virtual void setTransform( const MatrixF & mat );
   virtual void setScale( const Point3F &scale );

   // NetObject
   U32  packUpdate( NetConnection *conn, U32 mask, BitStream *stream, bool isInitial );
   bool unpackUpdate( BitStream *stream, bool isInitial );

   // SceneObject
   virtual void prepRenderImage( SceneRenderState *state );

   // WaterObject
   virtual F32 getWaterCoverage( const Box3F &worldBox ) const;
   virtual F32 getSurfaceHeight( const Point2F &pos ) const;
   virtual bool isUnderwater( const Point3F &pnt ) const;
   virtual const char* getLiquidType() const;

   // WaterBlock
   bool isPointSubmerged ( const Point3F &pos, bool worldSpace = true ) const{ return true; }

   // SceneObject.
   virtual F32 distanceTo( const Point3F& pos ) const;

   /// The binary's lava filter, the last thing GameRenderFilters draws: the
   /// two submerge textures swimming over the whole view while the camera
   /// is in lava.
   static void renderSubmergeFilter();
};

typedef WaterBlock::EWaterType WaterBlockLiquidType;
DefineEnumType( WaterBlockLiquidType );

#endif // _WATERBLOCK_H_
