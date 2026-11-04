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

#ifndef _PROJECTEDSHADOW_H_
#define _PROJECTEDSHADOW_H_

#ifndef _DEPTHSORTLIST_H_
#include "collision/depthSortList.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif
#ifndef _LIGHTINGSYSTEM_SHADOWBASE_H_
#include "lighting/common/shadowBase.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _GFXSHADER_H_
#include "gfx/gfxShader.h"
#endif

class ShapeBase;
class RenderPassManager;
class BaseMatInstance;
struct ObjectRenderInst;


GFX_DeclareTextureProfile( BLProjectedShadowProfile );
GFX_DeclareTextureProfile( BLProjectedShadowZProfile );

// HH: Modified for Tribes 2 compatibility.
/// A shape's shadow as Tribes 2 casts it (Shadow, 0x00652430): the shape's
/// outline seen from a fixed light, laid on the terrain and interior polygons
/// under it, which darken by it as they lie further down the light.
///
/// It is its own geometry, drawn in the shadow bin, and not a decal, so it
/// does not go with $pref::decalsOn.
class ProjectedShadow : public ShadowBase
{
public:

   /// How a shadow's direction and length change with its distance from the
   /// camera.
   struct DistanceDetail
   {
      F32 dist;
      F32 directionSkew;
      F32 lengthSkew;
   };

   /// How a shadow's outline is drawn at its size on screen.
   struct PixelSizeDetail
   {
      F32 size;
      U32 frameExpiration;
      U32 bmpDim;
      bool blur;
      bool generic;
   };

protected:

   SceneObject *mParentObject;
   ShapeBase *mShapeBase;

   F32 mRadius;
   MatrixF mLightToWorld;
   F32 mInvShadowDistance;

   Vector<DepthSortList::Poly> mPartition;
   Vector<Point3F> mPartitionVerts;
   Vector<Point2F> mPartitionTVerts;
   Vector<U8> mPartitionShades;

   bool mUseGeneric;
   bool mNeedOutline;
   U32 mOutlineDim;
   bool mOutlineBlur;
   U32 mOutlineTime;

   U32 mLastRenderTime;
   F32 mScore;

   GFXTexHandle mShadowTexture;
   GFXTexHandle mGenericTexture;
   GFXTextureTargetRef mRenderTarget;
   GFXTextureTargetRef mBlurTarget;

   GFXStateBlockRef mDrawSB;
   GFXStateBlockRef mBlurSB;
   GFXShaderRef mBlurShader;
   GFXShaderConstBufferRef mBlurConsts;
   GFXShaderConstHandle *mBlurModelViewSC;
   GFXShaderConstHandle *mBlurTexelSizeSC;
   GFXShaderConstHandle *mBlurSC;

   static SimObjectPtr<RenderPassManager> smRenderPass;
   static RenderPassManager* _getRenderPass();

   static DepthSortList smDepthSortList;
   static Box3F smShadowBox;
   static SphereF smShadowSphere;
   static U32 smShadowMask;

   static const DistanceDetail smDistanceDetails[];
   static const PixelSizeDetail smPixelSizeDetails[];

   static void _findDistanceDetail( F32 dist, DistanceDetail *dd );
   static const PixelSizeDetail* _findPixelSizeDetail( F32 pixelSize );
   static void _collisionCallback( SceneObject *obj, void *key );

   bool _castsShadow();
   bool _prepare( const SceneRenderState *state );
   void _buildPartition( const Point3F &pos, const VectorF &lightDir, F32 radius, F32 shadowLen );
   void _updatePartition( F32 fog );

   void _renderPartition( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );

   GFXTextureObject* _getDepthTarget( U32 width, U32 height );
   void _renderToTexture( const TSRenderState &rdata );
   bool _initBlur();
   const GFXTexHandle& _getGenericTexture();

   /// Returns a spotlight shadow material for use when
   /// rendering meshes into the projected shadow.
   static BaseMatInstance* _getShadowMaterial( BaseMatInstance *inMat );

public:

   /// Tribes 2's shadow detail level, 0 to 1, and what it sets; see
   /// setGlobalShadowDetailLevel.
   static F32 smGlobalShadowDetail;
   static F32 smShapeDetailScale;
   static S32 smShapeDetailMin;
   static F32 smSmallestVisibleSize;
   static F32 smLightDirSkew;
   static F32 smLightLenSkew;
   static F32 smGenericRadiusSkew;

   static void setGlobalShadowDetailLevel( F32 level );

   ProjectedShadow( SceneObject *object );
   virtual ~ProjectedShadow();

   bool shouldRender( const SceneRenderState *state );

   void update( const SceneRenderState *state );
   void render( F32 camDist, const TSRenderState &rdata );
   U32 getLastRenderTime() const { return mLastRenderTime; }
   const F32 getScore() const { return mScore; }

};

#endif // _PROJECTEDSHADOW_H_
