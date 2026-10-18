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

// HH: Replaced for Tribes 2 compatibility. Tribes 2's precipitation is not
// Torque 3D's: the scripts name a .dml of sixteen flakes and the boxes drops
// spawn in, and the client moves every drop once a frame, as it draws them.

#ifndef _PRECIPITATION_H_
#define _PRECIPITATION_H_

#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class RenderInstType;
class SFXTrack;
class SFXSource;
class Sky;
class ObjectRenderInst;
class BaseMatInstance;

//--------------------------------------------------------------------------
class PrecipitationData : public GameBaseData
{
   typedef GameBaseData Parent;

  public:
   enum Type
   {
      Rain = 0,
      Snow = 1,
      Sand = 2,
   };

   SFXTrack*        soundProfile;
   S32              type;
   F32              maxSize;
   F32              sizeX;
   F32              sizeY;
   StringTableEntry materialList;

   F32 movingBoxPer;
   F32 divHeightVal;
   F32 sizeBigBox;
   F32 topBoxSpeed;
   F32 frontBoxSpeed;
   F32 topBoxDrawPer;
   F32 bottomDrawHeight;
   F32 skipIfPer;
   F32 bottomSpeedPer;
   F32 frontSpeedPer;
   F32 frontRadiusPer;

   PrecipitationData();
   DECLARE_CONOBJECT(PrecipitationData);
   static void initPersistFields();
   bool onAdd();
   bool preload( bool server, String &errorStr );

   void packData( BitStream *stream, U32 classId );
   bool unpackData( BitStream *stream, U32 classId );
};

//--------------------------------------------------------------------------
class Precipitation : public GameBase
{
   typedef GameBase Parent;

  public:
   enum
   {
      MaxDrops  = 2000,
      MaxColors = 3,
      TexCells  = 16,
   };

   /// Retail's mask bits. GameBase reads the second and third too, for its
   /// datablock and its target, which is why a new percentage resends both.
   enum
   {
      InitMask       = BIT(0),
      PercentageMask = BIT(1),
      StormMask      = BIT(2),
      StormShowMask  = BIT(3),
   };

   enum Box
   {
      NoBox     = 0,
      TopBox    = 1,
      FrontBox  = 2,
      BottomBox = 3,
   };

   struct Drop
   {
      F32     speed;       ///< metres a frame, negative is down
      Point3F pos;
      Point3F spawn;
      Point3F drift;       ///< sideways metres per metre fallen
      S32     color;
      S32     tex;
      bool    visible;
      F32     fadeHeight;
      S32     box;
      bool    respawn;
   };

  protected:
   PrecipitationData *mDataBlock;

   Drop     mDrops[MaxDrops];
   Point2F  mTexCoords[TexCells];

   // Fields a mission sets.
   F32     mPercentage;
   ColorF  mColors[MaxColors];
   S32     mNumColors;
   F32     mOffsetSpeed;
   F32     mMinVelocity;
   F32     mMaxVelocity;
   S32     mMaxNumDrops;
   S32     mMaxRadius;

   S32     mNumDrops;      ///< drops moved and drawn
   S32     mNumActive;     ///< of those, the ones that respawn

   // Storms.
   U32     mStormLastMs;
   F32     mStormTarget;
   F32     mStormTime;
   F32     mStormRate;
   S32     mStormDir;      ///< 1 rising, 2 falling, 0 idle
   F32     mStormTicks;
   bool    mStormShow;

   // A frame's working state.
   bool    mFirstFrame;
   bool    mNotRendered;
   F32     mTime;
   F32     mLastTime;
   Point3F mSpawnOffset;
   Point3F mLastCamPos;
   F32     mCamVelFracX;
   F32     mCamVelFracY;
   Point3F mBoxMin;
   Point3F mBoxMax;
   F32     mAvgSpeed;
   F32     mBoxTopDelta;
   Point3F mWind;
   Point3F mFrameOffset;

   GFXTexHandle      mTexture;
   SFXSource*        mAmbientSound;
   SimObjectPtr<SceneObject> mSky;

   GFXStateBlockRef        mStateBlock;
   GFXShaderRef            mShader;
   GFXShaderConstBufferRef mShaderConsts;
   GFXShaderConstHandle*   mModelViewSC;

   static bool smPrecipitationOn;
   static bool smPrePause;

   void validate();
   void initTexCoords();
   void loadMaterialList();
   void startSound();
   void stopSound();
   bool initRender();
   Sky *findSky();

   void updateBox( const Point3F &camPos, F32 *outVelZ, F32 *outTopChance );
   void updateStorm();
   void startStorm();
   void respawnDrop( Drop &drop, const Point3F &camPos, F32 topChance );
   void renderDrops( const Point3F &camPos, const Point3F &right, const Point3F &up );

   void renderObject( ObjectRenderInst *ri, SceneRenderState *state,
                      BaseMatInstance *overrideMat );

  public:
   Precipitation();
   ~Precipitation();

   DECLARE_CONOBJECT(Precipitation);
   static void initPersistFields();
   static void consoleInit();

   /// The default render pass draws this bin after the opaque ones and
   /// before every translucent one, which is where retail's BeginSort puts it.
   static const RenderInstType RIT_Precipitation;

   bool onAdd();
   void onRemove();
   bool onNewDataBlock( GameBaseData *dptr, bool reload );

   void processTick( const Move *move );
   void advanceTime( F32 dt );

   void prepRenderImage( SceneRenderState *state );

   U32  packUpdate( NetConnection *conn, U32 mask, BitStream *stream,
                    bool isInitial );
   bool unpackUpdate( BitStream *stream, bool isInitial );

   void setPercentage( F32 pct );
   void stormPrecipitation( F32 pct, F32 seconds );
   void stormShow( bool show );
};

#endif // _PRECIPITATION_H_
