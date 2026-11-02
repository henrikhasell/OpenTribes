// HH: Added for Tribes 2 compatibility.
#ifndef _STATIONFXPERSONALT2_H_
#define _STATIONFXPERSONALT2_H_

#ifndef _DATABLOCKST2_H_
#include "T3D/gameBase/dataBlocksT2.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif

class TSShapeInstance;
class ObjectRenderInst;
class BaseMatInstance;

/// What an inventory station shows as it fits a player out: a band of light
/// trailing each of its two spinning arms round the player.
class StationFXPersonalData : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum { NumTextures = 2 };

   F32 delay;                       ///< sent, and read by nothing
   F32 fadeDelay;                   ///< seconds before the bands fade out
   F32 lifetime;                    ///< seconds the server keeps the effect
   F32 height;
   F32 leftRadius;
   F32 rightRadius;
   S32 numArcSegments;
   F32 numDegrees;                  ///< how far round each band trails
   F32 trailFadeTime;               ///< seconds a band takes to grow or shrink

   StringTableEntry leftNodeName;
   StringTableEntry rightNodeName;
   StringTableEntry textureName[NumTextures];  ///< only the first is drawn
   GFXTexHandle     texture[NumTextures];

   StationFXPersonalData();

   static void initPersistFields();
   bool preload(bool server, String &errorStr);
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(StationFXPersonalData);
};

class StationFXPersonal : public GameBase
{
   typedef GameBase Parent;

   StationFXPersonalData *mDataBlock;

   S32  mStationId;                 ///< stationObject, as the script set it
   SimObjectPtr<SceneObject> mStation;
   S32  mElapsedMS;                 ///< the server's clock, in whole ticks
   F32  mElapsed;                   ///< the client's clock, in seconds

   GFXStateBlockRef mStateBlock;

   void placeAtStation();

   void renderObject(ObjectRenderInst *ri, SceneRenderState *state,
                     BaseMatInstance *overrideMat);
   void renderArc(TSShapeInstance *shape, const MatrixF &station,
                  F32 radius, const char *nodeName);

public:
   StationFXPersonal();

   static void initPersistFields();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   bool unpackUpdate(BitStream *stream, bool isInitial);

   void processTick(const Move *move);
   void advanceTime(F32 dt);

   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(StationFXPersonal);
};

#endif
