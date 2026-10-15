// HH: imported from TGE
// (not a copy: the same class ported to the GFX layer, storms dropped)

#ifndef _SKY_H_
#define _SKY_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif

#define MAX_NUM_LAYERS 3
#define MAX_BAN_POINTS 20

/// A horizontal slab of fog.  The first three floats are set as one Point3F by
/// the fogVolumeN fields, so their order and adjacency are part of the file
/// format and must not change.
struct SkyFogVolume
{
   F32    visibleDistance;
   F32    minHeight;
   F32    maxHeight;
   F32    percentage;
   ColorF color;
};

enum SkyFogConstants
{
   MaxFogVolumes = 3
};

//---------------------------------------------------------------------------
/// One scrolling cloud layer: a 5x5 grid of points bowed into a shallow dome,
/// its texture coordinates sliding with the wind and its alpha falling off
/// towards the rim.
class Cloud
{
private:
   Point3F mPoints[25];
   Point2F mSpeed;
   F32 mCenterHeight, mInnerHeight, mEdgeHeight;
   F32 mAlpha[25];
   static F32 mRadius;
   F32 mLastTime, mOffset;
   Point2F mBaseOffset, mTexCoords[25], mTextureScale;
   GFXTexHandle mCloudHandle;

public:
   Cloud();

   void setPoints();
   void setHeights( F32 cHeight, F32 iHeight, F32 eHeight );
   void setTexture( GFXTexHandle texture );
   void setSpeed( Point2F speed );
   void updateCoord();
   void calcAlpha();
   void render( U32 currentTime, PlaneF *planes, GFXStateBlock *sb );

   static void setRadius( F32 rad ) { mRadius = rad; }

   void setRenderPoints( Point3F *renderPoints, Point2F *renderTexPoints,
                         F32 *renderAlpha, S32 index );
   void clipToPlane( Point3F *points, Point2F *texPoints, F32 *alphaPoints,
                     U32 &rNumPoints, const PlaneF &rPlane );
};

//--------------------------------------------------------------------------
/// The sky box, cloud layers and fog of the original engine, driven by a
/// .dml material list.  Publishes its fog to the SceneManager, so a mission
/// carrying a Sky needs no LevelInfo to see anything.
class Sky : public SceneObject
{
   typedef SceneObject Parent;

private:
   GFXTexHandle mSkyHandle[6];
   F32 mCloudHeight[MAX_NUM_LAYERS];
   F32 mCloudSpeed[MAX_NUM_LAYERS];
   Cloud mCloudLayer[MAX_NUM_LAYERS];
   F32 mRadius;
   Point3F mPoints[10];
   Point2F mTexCoord[4];
   StringTableEntry mMaterialListName;

   Point3F mSkyBoxPt;
   Point3F mTopCenterPt;
   Point3F mSpherePt;
   ColorI mRealFogColor;
   ColorI mRealSkyColor;

   ColorF mFogColor;
   bool mSkyTexturesOn;
   bool mRenderBoxBottom;
   ColorF mSolidFillColor;
   F32 mFogDistance;
   F32 mVisibleDistance;
   U32 mNumFogVolumes;
   SkyFogVolume mFogVolumes[MaxFogVolumes];
   bool mFogStorm[MaxFogVolumes];
   F32 mFogLine;
   bool mNoRenderBans;

   S32 mNumCloudLayers;
   Point3F mWindVelocity;

   F32 mLastVisDisMod;

   GFXStateBlockRef mSkyStateBlock;
   GFXStateBlockRef mBanStateBlock;
   GFXStateBlockRef mCloudStateBlock;

   static bool smCloudsOn;
   static bool smSkyOn;
   static S32  smNumCloudsOn;
   static F32  smVisibleDistanceMod;

   void calcPoints();

protected:
   bool onAdd();
   void onRemove();

   void render( const Point3F &camPos );
   void calcAlphas_Heights( F32 zCamPos, F32 *banHeights, F32 *alphaBan, F32 depthInFog );
   void renderSkyBox( F32 lowerBanHeight, F32 alphaBanUpper );
   void calcBans( F32 *banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F *cornerPoints );
   void renderBans( F32 *alphaBan, F32 *banHeights, Point3F banPoints[][MAX_BAN_POINTS],
                    Point3F *cornerPoints );
   void inspectPostApply();
   void setVisibility();
   void initSkyData();
   bool loadDml();
   void initStateBlocks();
   void setRenderPoints( Point3F *renderPoints, S32 index );
   void calcTexCoords( Point2F *texCoords, Point3F *renderPoints, S32 index );

public:
   bool mEffectPrecip;
   Point2F mWindDir;

   enum NetMaskBits
   {
      InitMask       = BIT(0),
      VisibilityMask = BIT(1),
      WindMask       = BIT(2)
   };

   enum Constants
   {
      EnvMapMaterialOffset = 6,
      CloudMaterialOffset  = 7
   };

   Sky();

   F32 getVisibleDistance() const { return mVisibleDistance; }

   void setWindVelocity( const Point3F & );
   Point3F getWindVelocity();

   DECLARE_CONOBJECT(Sky);
   static void initPersistFields();
   static void consoleInit();


   void prepRenderImage( SceneRenderState *state );
   void _renderObject( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *mi );

   void updateVisibility();
};

#endif
