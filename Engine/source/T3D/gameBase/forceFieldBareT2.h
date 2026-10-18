// HH: Added for Tribes 2 compatibility.
#ifndef _FORCEFIELDBARET2_H_
#define _FORCEFIELDBARET2_H_

#ifndef _DATABLOCKST2_H_
#include "T3D/gameBase/dataBlocksT2.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif

class Convex;
class ObjectRenderInst;
class BaseMatInstance;

class ForceFieldBareData : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum { NumTextures = 5 };

   S32    fadeMS;
   F32    baseTranslucency;
   F32    powerOffTranslucency;
   bool   teamPermiable;
   bool   otherPermiable;
   ColorF color;
   ColorF powerOffColor;
   S32    framesPerSec;
   S32    numFrames;
   F32    scrollSpeed;
   F32    umapping;
   F32    vmapping;

   StringTableEntry textureName[NumTextures];
   GFXTexHandle     texture[NumTextures];

   ForceFieldBareData();

   static void initPersistFields();
   bool onAdd();
   bool preload(bool server, String &errorStr);
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(ForceFieldBareData);
};

class ForceFieldBare : public GameBase
{
   typedef GameBase Parent;

public:
   enum State
   {
      Open    = 0,
      Opening = 1,
      Closing = 2,
      Closed  = 3,
   };

   enum MaskBits
   {
      StateMask = Parent::NextFreeMask,
      NextFreeMask = Parent::NextFreeMask << 1
   };

private:
   ForceFieldBareData *mDataBlock;

   U32  mState;
   S32  mFadeTime;
   S32  mLastFadeTime;
   F32  mFade;
   F32  mAnimTime;
   S32  mT2Target;

   Convex *mConvexList;
   GFXStateBlockRef mStateBlock;

   void tickFade();
   void renderObject(ObjectRenderInst *ri, SceneRenderState *state,
                     BaseMatInstance *overrideMat);

public:
   ForceFieldBare();
   ~ForceFieldBare();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   bool unpackUpdate(BitStream *stream, bool isInitial);

   void processTick(const Move *move);
   void interpolateTick(F32 delta);
   void advanceTime(F32 dt);

   void open();
   void close();

   U32 getState() const { return mState; }
   U32 getSensorGroup() const;

   bool isPermeableTo(SceneObject *obj) const;

   void buildConvex(const Box3F &box, Convex *convex);
   bool buildPolyList(PolyListContext context, AbstractPolyList *polyList,
                      const Box3F &box, const SphereF &sphere);
   bool castRay(const Point3F &start, const Point3F &end, RayInfo *info);

   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(ForceFieldBare);
};

#endif
