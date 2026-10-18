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

   S32    fadeMS;                 ///< how long it takes to open or close
   F32    baseTranslucency;       ///< its alpha with the power on
   F32    powerOffTranslucency;   ///< and with it off
   bool   teamPermiable;          ///< its own team walks through it
   bool   otherPermiable;         ///< every other team does
   ColorF color;
   ColorF powerOffColor;
   S32    framesPerSec;
   S32    numFrames;
   F32    scrollSpeed;            ///< texture lengths a second along v
   F32    umapping;               ///< texture lengths a metre along u
   F32    vmapping;               ///< and along v

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
   /// The four states of ForceFieldBare, numbered as the exe numbers them
   /// on the wire: the field is solid in all but Open.
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
   S32  mFadeTime;          ///< milliseconds of fadeMS the field has opened
   S32  mLastFadeTime;      ///< the same at the tick before
   F32  mFade;              ///< one closed, nought open, between ticks
   F32  mAnimTime;          ///< seconds the texture has been animating
   S32  mT2Target;          ///< the target manager's id, or -1

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

   /// Whether a player's collision walks through the field.
   bool isPermeableTo(SceneObject *obj) const;

   void buildConvex(const Box3F &box, Convex *convex);
   bool buildPolyList(PolyListContext context, AbstractPolyList *polyList,
                      const Box3F &box, const SphereF &sphere);
   bool castRay(const Point3F &start, const Point3F &end, RayInfo *info);

   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(ForceFieldBare);
};

#endif
