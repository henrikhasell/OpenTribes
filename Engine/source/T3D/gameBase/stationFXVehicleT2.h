// HH: Added for Tribes 2 compatibility.
#ifndef _STATIONFXVEHICLET2_H_
#define _STATIONFXVEHICLET2_H_

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

/// What a vehicle pad shows while it builds a vehicle: a glowing ring round
/// the pad, bands of light between the station's arms and a blue dome over
/// it all, each fading in and out on its own clock.
class StationFXVehicleData : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum { NumArmNodes = 4, NumTextures = 2 };

   F32    lifetime;                 ///< seconds the server keeps the effect
   F32    glowTopHeight;
   F32    glowBottomHeight;
   F32    glowTopRadius;
   F32    glowBottomRadius;
   S32    numGlowSegments;
   F32    glowFadeTime;
   F32    armLightDelay;            ///< seconds before the bands and the dome
   F32    armLightLifetime;         ///< seconds from that delay to their end
   F32    armLightFadeTime;
   S32    numArcSegments;
   ColorF sphereColor;
   S32    spherePhiSegments;        ///< points round the dome
   S32    sphereThetaSegments;      ///< rings from its rim to its crown
   F32    sphereRadius;
   Point3F sphereScale;

   StringTableEntry glowNodeName;
   StringTableEntry leftNodeName[NumArmNodes];
   StringTableEntry rightNodeName[NumArmNodes];
   StringTableEntry textureName[NumTextures];  ///< the ring's, the bands'
   GFXTexHandle     texture[NumTextures];

   StationFXVehicleData();

   static void initPersistFields();
   bool preload(bool server, String &errorStr);
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(StationFXVehicleData);
};

class StationFXVehicle : public GameBase
{
   typedef GameBase Parent;

   StationFXVehicleData *mDataBlock;

   S32  mStationId;                 ///< stationObject, as the script set it
   SimObjectPtr<SceneObject> mStation;
   S32  mElapsedMS;                 ///< the server's clock, in whole ticks
   F32  mElapsed;                   ///< the client's clock, in seconds

   GFXStateBlockRef mBandStateBlock;
   GFXStateBlockRef mDomeStateBlock;

   void placeAtStation();

   void renderObject(ObjectRenderInst *ri, SceneRenderState *state,
                     BaseMatInstance *overrideMat);
   void renderArmLight(TSShapeInstance *shape, const MatrixF &station,
                       F32 topRadius, F32 bottomRadius, F32 sweep,
                       const char *topNode, const char *bottomNode);
   void renderGlow(TSShapeInstance *shape, const MatrixF &station);
   void renderSphere(TSShapeInstance *shape, const MatrixF &station);

public:
   StationFXVehicle();

   static void initPersistFields();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   bool unpackUpdate(BitStream *stream, bool isInitial);

   void processTick(const Move *move);
   void advanceTime(F32 dt);

   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(StationFXVehicle);
};

#endif
