// HH: Added for Tribes 2 compatibility.
/// A Tribes 2 projectile is told to a client once, in the body that creates
/// it: where it left the muzzle and which way it is going. How fast and for
/// how long are its datablock's, and every position after the first is the
/// client's own arithmetic -- so a projectile that is only decoded is a
/// projectile that is never anywhere, and never drawn.
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/resourceManager.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "lighting/lightInfo.h"
#include "lighting/lightManager.h"
#include "lighting/lightQuery.h"
#include "math/mathUtils.h"
#include "math/mRandom.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTrack.h"
#include "T3D/debris.h"
#include "T3D/decal/decalData.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/decalsT2.h"
#include "T3D/gameBase/processList.h"
#include "T3D/fx/explosion.h"
#include "T3D/fx/particleEmitter.h"
#include "T3D/fx/splash.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/objectTypes.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/gravityT2.h"
#include "T3D/shapeBase.h"
#include "environment/waterBlock.h"
#include "sim/netConnection.h"
#include "ts/tsShapeInstance.h"

class ProjectileDataT2 : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   F32    velocity;        ///< metres a second along the way it was fired
   U32    lifetimeMS;      ///< how long retail's client keeps it
   F32    streakLength;    ///< the tracer a bullet is seen as, where it is
   F32    streakWidth;     ///< seen as one rather than drawn as a shape
   F32    crossSize;       ///< the quad a bolt is seen as end on
   F32    crossViewAng;    ///< how square to the view that has to be
   Point3F scale;          ///< a bolt's width, and its length in widths
   F32    elasticity;      ///< what a bounce keeps of its speed
   F32    friction;        ///< what a bounce takes off along the surface
   F32    gravityMod;      ///< a share of gravity, and zero for a bolt
   F32    drag;
   F32    density;
   U32    armingDelayMS;   ///< it bounces until this, and goes off after
   U32    explosionId;     ///< the ExplosionData it ends as
   U32    baseEmitterId;   ///< the ParticleEmitterData it trails in flight
   U32    bubbleEmitterId; ///< and the one it trails under water instead
   U32    soundId;         ///< the AudioProfile it flies to
   F32    blurLifetime;    ///< seconds a length of the trail lasts
   F32    blurWidth;
   ColorF blurColor;
   String texture[2];      ///< the bolt's and the cross's
   U32    numFlares;       ///< the petals a plasma bolt is seen as
   F32    flareSize[3];    ///< how far out they start and end
   ColorF flareColor;
   bool   faceViewer;      ///< its shape is turned to the camera
   bool   hasLight;        ///< it lights what it flies past
   F32    lightRadius;
   ColorF lightColor;
   bool   hasWaterLight; ///< and has another colour under water
   ColorF waterLightColor;

      /// The marks it leaves where it goes off.  A projectile's datablock
      /// carries six of them and retail lays the first (0x0062d9c8); they
      /// are the only DecalData its body names.
   Vector<U32> decalIds;
   DecalData  *decal;

   ProjectileDataT2();

      /// A bullet is seen as the streak it leaves, which is what retail draws
      /// of one, so the shape its datablock also names is never wanted.
   bool namesGameBaseShape() const { return streakLength <= 0.0f; }
   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(ProjectileDataT2);
};

IMPLEMENT_CONOBJECT(ProjectileDataT2);

ConsoleDocClass( ProjectileDataT2,
   "@brief What a Tribes 2 projectile's datablock says about its flight.\n\n"
   "@internal" );

ProjectileDataT2::ProjectileDataT2()
   : velocity(0.0f),
     lifetimeMS(0),
     streakLength(0.0f),
     streakWidth(0.0f),
     crossSize(0.0f),
     crossViewAng(0.0f),
     scale(1.0f, 1.0f, 1.0f),
     elasticity(0.0f),
     friction(0.0f),
     gravityMod(0.0f),
     drag(0.0f),
     density(1.0f),
     armingDelayMS(0),
     explosionId(0),
     baseEmitterId(0),
     bubbleEmitterId(0),
     soundId(0),
     blurLifetime(0.0f),
     blurWidth(0.0f),
     blurColor(1.0f, 1.0f, 1.0f, 1.0f),
     numFlares(0),
     flareColor(1.0f, 1.0f, 1.0f, 1.0f),
     faceViewer(false),
     hasLight(false),
     lightRadius(1.0f),
     lightColor(1.0f, 1.0f, 1.0f, 1.0f),
     hasWaterLight(false),
     waterLightColor(1.0f, 1.0f, 1.0f, 1.0f),
     decal(NULL)
{
   flareSize[0] = flareSize[1] = flareSize[2] = 0.0f;
}

/// The decals are the only DecalData among the datablocks a projectile's
/// body names; the rest are its explosion, its splash, its emitters and its
/// sounds.  Retail keeps the six it is given in the order they were sent and
/// marks the world with the first.
bool ProjectileDataT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   for(U32 i = 0; !decal && i < decalIds.size(); i++)
   {
      DecalData *data;
      if(Sim::findObject(decalIds[i], data))
         decal = data;
   }

   return true;
}

bool ProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbProjectileVelocity, &value))
      velocity = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileLifetime, &word))
      lifetimeMS = word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbTracerLength, &value))
      streakLength = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbTracerWidth, &value))
      streakWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbCrossSize, &value))
      crossSize = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbCrossViewAng, &value))
      crossViewAng = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbArmingDelay, &word))
      armingDelayMS = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbExplosion, &word))
      explosionId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileBaseEmitter, &word))
      baseEmitterId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileBubbleEmitter, &word))
      bubbleEmitterId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileSound, &word))
      soundId = word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBlurLifetime, &value))
      blurLifetime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBlurWidth, &value))
      blurWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBlurColorR, &value))
      blurColor.red = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBlurColorG, &value))
      blurColor.green = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBlurColorB, &value))
      blurColor.blue = value;
      // A scale is only on the wire when it is not the default one.
   if(T2Layout::lastDataBlockField(T2Layout::T2DbScaleX, &value))
      scale.x = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbScaleY, &value))
      scale.y = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbScaleZ, &value))
      scale.z = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElasticity, &value))
      elasticity = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFriction, &value))
      friction = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbGravityMod, &value))
      gravityMod = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbProjectileDrag, &value))
      drag = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbProjectileDensity, &value))
      density = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbNumFlares, &word))
      numFlares = S32(word) > 0 ? word : 0;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFlareColor, &word))
      flareColor.set(F32(word & 0xff) / 255.0f, F32((word >> 8) & 0xff) / 255.0f,
                     F32((word >> 16) & 0xff) / 255.0f);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFlareSize0, &value))
      flareSize[0] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFlareSize1, &value))
      flareSize[1] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFlareSize2, &value))
      flareSize[2] = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFaceViewer, &word))
      faceViewer = word != 0;
      // The light is only on the wire behind hasLight, and its radius goes
      // over it as a share of twenty (0x006317f6).
   hasLight = T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileLightRadius,
                                          &word);
   if(hasLight)
      lightRadius = T2Layout::t2Float(word, 8) * 20.0f;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileLightColorR, &word))
      lightColor.red = T2Layout::t2Float(word, 7);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileLightColorG, &word))
      lightColor.green = T2Layout::t2Float(word, 7);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileLightColorB, &word))
      lightColor.blue = T2Layout::t2Float(word, 7);
   hasWaterLight = T2Layout::lastDataBlockWord(
                      T2Layout::T2DbProjectileWaterLightColorR, &word);
   if(hasWaterLight)
      waterLightColor.red = T2Layout::t2Float(word, 7);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileWaterLightColorG, &word))
      waterLightColor.green = T2Layout::t2Float(word, 7);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileWaterLightColorB, &word))
      waterLightColor.blue = T2Layout::t2Float(word, 7);

   decalIds = T2Layout::lastDataBlockIds();

      // Every one of these layouts names its shape first and its two
      // textures after it, which for a bolt are what it is drawn with along
      // its flight and what it is drawn with end on.
   const Vector<String> &strings = T2Layout::lastStrings();
   if(strings.size() > 1)
      texture[0] = strings[1];
   if(strings.size() > 2)
      texture[1] = strings[2];

   return true;
}

#define IMPLEMENT_PROJECTILE_DATA_T2( className )                            \
   class className : public ProjectileDataT2                                 \
   {                                                                         \
      typedef ProjectileDataT2 Parent;                                       \
   public:                                                                   \
      DECLARE_CONOBJECT(className);                                          \
   };                                                                        \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 projectile's datablock."    \
                               "\n\n@internal" )

IMPLEMENT_PROJECTILE_DATA_T2( EnergyProjectileData );
IMPLEMENT_PROJECTILE_DATA_T2( GrenadeProjectileData );
IMPLEMENT_PROJECTILE_DATA_T2( LinearFlareProjectileData );
IMPLEMENT_PROJECTILE_DATA_T2( LinearProjectileData );
IMPLEMENT_PROJECTILE_DATA_T2( TracerProjectileData );

//----------------------------------------------------------------------------

/// A shape flies nose first along its own +Y, with the world's up for its
/// roll: the trail of a disc runs down its -Y, and a roll taken from any
/// perpendicular at all turns the disc over at random. Tribes 2 builds it
/// with one routine (0x00553d00), and straight up or down, where the world's
/// up gives no roll, that routine takes the world's +X for the right. A disc
/// fired straight up then faces along -Y.
static void headingMatrixT2(MatrixF *mat, const VectorF &along)
{
   VectorF forward = along;
   if(forward.isZero())
      return;
   forward.normalize();

      // Each part on its own, not the length: within about 0.6 degrees of
      // vertical, where a length test would still take a roll. A cross with
      // the world's up has no z.
   VectorF right = mCross(forward, Point3F(0.0f, 0.0f, 1.0f));
   if(mFabs(right.x) < 0.01f && mFabs(right.y) < 0.01f)
      right.set(1.0f, 0.0f, 0.0f);
   right.normalize();

   const VectorF up = mCross(right, forward);

   mat->setColumn(0, right);
   mat->setColumn(1, forward);
   mat->setColumn(2, up);
}

class ProjectileT2 : public GameBase, public ISceneLight
{
   typedef GameBase Parent;

   ProjectileDataT2 *mData;
   TSShapeInstance  *mShapeInstance;
   TSThread         *mAmbientThread;
   TSThread         *mFlightThread;  ///< Activate, and Maintain after it
   S32               mMaintainSeq;
   GFXTexHandle      mTexture[2];
   GFXStateBlockRef  mBoltSB;
   GFXStateBlockRef  mTrailSB;

   /// One length of the trail a bolt leaves, aged from each end so the quad
   /// fades along itself as well as away.
   struct Trail
   {
      Point3F from, to;
      F32     ageFrom, ageTo;
   };
   Vector<Trail> mTrail;
   Point3F mTrailAt;          ///< where the last length of it ended
   F32     mBoxReach;         ///< what the bolt and its trail need enclosing
   F32     mSinceTrail;       ///< seconds since the last length was laid

   /// One of the petals a plasma bolt is seen as: a thin cone out from the
   /// bolt along a way of its own, which grows out and then opens and fades.
   struct Flare
   {
      Point3F way;
      F32     from, to;       ///< how far out it reaches, first and last
      F32     tilt;           ///< how far its sides lean once it opens
      F32     peak, life, age;
   };
   Vector<Flare>    mFlares;
   GFXTexHandle     mFlareTexture;
   GFXStateBlockRef mFlareSB;

   Point3F mStart;            ///< where the body put it
   Point3F mPosition;         ///< where it has got to since
   VectorF mVelocity;         ///< metres a second, which is all the flight is
   bool    mNeedsSpeed;       ///< the body gave a direction and the datablock
                              ///< the speed, and it arrives after the body
   bool    mBounces;          ///< a grenade's family does; a disc goes off
   F32     mAge;
   F32     mCatchUp;
   bool    mFlying;
   bool    mExploded;         ///< it only goes off once
   bool    mMarks;            ///< what it hit takes a decal
   VectorF mImpactNormal;
   SFXSource *mSound;         ///< what it is heard as on the way
   SimObjectPtr<ParticleEmitter> mEmitter;  ///< the smoke it trails
   SimObjectPtr<ParticleEmitter> mBubbleEmitter;
   Point3F mEmitAt;           ///< where the smoke last got to
   F32     mEmitMS;           ///< flight not yet handed to the emitter

   SimObjectId mSourceObject;
   SimObjectId mVehicleObject;
   F32     mNoSourceHit;

   LightInfo *mLight;

   GFXStateBlockRef boltStateBlock();
   TSThread *startThread(const char *name);
   ParticleEmitter *createEmitter(U32 dataId);
   void step(F32 dt);
   void explode();
   void ageTrail(F32 dt);
   void renderBolt(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);
   void drawTrail(const Point3F &eye);
   void drawStreak(const Point3F &eye);
   void drawBolt(const Point3F &eye);
   void drawCross(const Point3F &eye);
   void spawnFlare(Flare &flare);
   void drawFlares(F32 clear);
   void drawGlow(const SceneRenderState *state, F32 clear);

protected:
   bool    mStopsOnDamageable;///< an energy bolt goes off on anything it can
                              ///< damage; a grenade bounces off it

public:
   ProjectileT2();
   ~ProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

   void submitLights(LightManager *lm, bool staticLighting);
   LightInfo *getLight() { return mLight; }

   DECLARE_CONOBJECT(ProjectileT2);
};

IMPLEMENT_CONOBJECT(ProjectileT2);

ConsoleDocClass( ProjectileT2,
   "@brief A Tribes 2 projectile, flown by the client that was told of it.\n\n"
   "@internal" );

ProjectileT2::ProjectileT2()
   : mData(NULL),
     mSound(NULL),
     mShapeInstance(NULL),
     mAmbientThread(NULL),
     mFlightThread(NULL),
     mMaintainSeq(-1),
     mStart(0.0f, 0.0f, 0.0f),
     mPosition(0.0f, 0.0f, 0.0f),
     mVelocity(0.0f, 0.0f, 0.0f),
     mNeedsSpeed(false),
     mBounces(false),
     mTrail(__FILE__, __LINE__),
     mTrailAt(0.0f, 0.0f, 0.0f),
     mBoxReach(0.0f),
     mSinceTrail(0.0f),
     mFlares(__FILE__, __LINE__),
     mAge(0.0f),
     mCatchUp(0.0f),
     mFlying(false),
     mExploded(false),
     mMarks(false),
     mImpactNormal(0.0f, 0.0f, 1.0f),
     mEmitAt(0.0f, 0.0f, 0.0f),
     mEmitMS(0.0f),
     mSourceObject(0),
     mVehicleObject(0),
     mNoSourceHit(0.0f),
     mLight(NULL)
{
   mStopsOnDamageable = false;
   mTypeMask |= ProjectileObjectType;
}

ProjectileT2::~ProjectileT2()
{
   delete mShapeInstance;
   SAFE_DELETE(mLight);
}

bool ProjectileT2::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(-0.5f, -0.5f, -0.5f);
   mObjBox.maxExtents.set( 0.5f,  0.5f,  0.5f);
   resetWorldBox();
   addToScene();

      // Nothing else moves it: the flight happens here.
   if(isClientObject())
      setProcessTick(true);

   return true;
}

void ProjectileT2::onRemove()
{
   SFX_DELETE(mSound);

      // What it has already trailed is left to clear on its own.
   if(bool(mEmitter))
      mEmitter->deleteWhenEmpty();
   mEmitter = NULL;
   if(bool(mBubbleEmitter))
      mBubbleEmitter->deleteWhenEmpty();
   mBubbleEmitter = NULL;

   removeFromScene();
   Parent::onRemove();
}

bool ProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<ProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   SFX_DELETE(mSound);
   if(isClientObject() && mData->soundId)
   {
      SFXTrack *track = NULL;
      if(Sim::findObject(mData->soundId, track))
         mSound = SFX->createSource(track, &getTransform());
   }

   if(bool(mEmitter))
      mEmitter->deleteWhenEmpty();
   if(bool(mBubbleEmitter))
      mBubbleEmitter->deleteWhenEmpty();

   mEmitter = createEmitter(mData->baseEmitterId);
   mBubbleEmitter = createEmitter(mData->bubbleEmitterId);

   if(mData->hasLight)
      mTypeMask |= LightObjectType;
   else
      mTypeMask &= ~LightObjectType;

   delete mShapeInstance;
   mShapeInstance = NULL;

   mAmbientThread = NULL;
   mFlightThread = NULL;
   mMaintainSeq = -1;

   mFlares.clear();
   mFlareTexture = NULL;
   if(isClientObject() && mData->numFlares && mData->texture[0].isNotEmpty())
   {
      mFlareTexture.set(String("textures/") + mData->texture[0],
                        &GFXDefaultStaticDiffuseProfile, "ProjectileT2");
      mFlares.setSize(mData->numFlares);
      for(U32 i = 0; i < mFlares.size(); i++)
         spawnFlare(mFlares[i]);
   }

   if(bool(mData->mShape))
   {
      mShapeInstance = new TSShapeInstance(mData->mShape, isClientObject());
      mObjBox = mData->mShape->bounds;
      resetWorldBox();
      mBoxReach = 0.0f;

         // The spin of a disc is in the shape, not in the transform: it
         // carries Ambient, Activate and Maintain, and a projectile that
         // plays none of them is a frozen frame of itself.
      mAmbientThread = startThread("ambient");
      mMaintainSeq = mData->mShape->findSequence("maintain");
      mFlightThread = startThread("activate");
      if(!mFlightThread && mMaintainSeq >= 0)
         mFlightThread = startThread("maintain");
   }
   else
   {
      for(U32 i = 0; i < 2; i++)
         if(mData->texture[i].isNotEmpty())
            mTexture[i].set(String("textures/") + mData->texture[i],
                            &GFXDefaultStaticDiffuseProfile, "ProjectileT2");

         // A bolt is drawn five metres long and trails a fifth of a second
         // behind itself, and none of that is inside a box the size of the
         // point it is at -- which would have the scene cull the lot.
      mBoxReach = getMax(mData->scale.x * mData->scale.y * 0.5f,
                         mData->blurLifetime * mData->velocity) + 1.0f;
      mObjBox.minExtents.set(-mBoxReach, -mBoxReach, -mBoxReach);
      mObjBox.maxExtents.set( mBoxReach,  mBoxReach,  mBoxReach);
      resetWorldBox();
   }

   return true;
}

TSThread *ProjectileT2::startThread(const char *name)
{
   const S32 seq = mData->mShape->findSequence(name);
   if(seq < 0)
      return NULL;

   TSThread *thread = mShapeInstance->addThread();
   mShapeInstance->setTimeScale(thread, 1.0f);
   mShapeInstance->setSequence(thread, seq, 0.0f);
   return thread;
}

ParticleEmitter *ProjectileT2::createEmitter(U32 dataId)
{
   ParticleEmitterData *data = NULL;
   if(!isClientObject() || !dataId || !Sim::findObject(dataId, data))
      return NULL;

   ParticleEmitter *emitter = new ParticleEmitter;
   emitter->onNewDataBlock(data, false);
   if(emitter->registerObject())
      return emitter;

   Con::warnf("ProjectileT2: %s could not register its emitter.",
              mData->getName());
   delete emitter;
   return NULL;
}

/// Water is any liquid but lava, which is the test retail makes of the
/// water blocks a point is in (0x00632cd0).
static bool isInWaterT2(SceneContainer *container, const Point3F &point)
{
   Vector<SceneObject *> found;
   const Point3F reach(0.01f, 0.01f, 0.01f);
   container->findObjectList(Box3F(point - reach, point + reach),
                             WaterObjectType, &found);

   for(U32 i = 0; i < found.size(); i++)
   {
      WaterObject *water = dynamic_cast<WaterObject *>(found[i]);
      if(water && water->isUnderwater(point) &&
         !WaterBlock::isLava(dAtoi(water->getLiquidType())))
         return true;
   }

   return false;
}

void ProjectileT2::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   if(body.hasSourceObject)
   {
      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(S32(body.sourceObject))
                            : NULL;
      mSourceObject = obj ? obj->getId() : 0;
      mNoSourceHit = 7.0f * TickSec;
   }

   if(body.hasVehicleObject)
   {
      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(S32(body.vehicleObject))
                            : NULL;
      mVehicleObject = obj ? obj->getId() : 0;
   }

   if(body.hasCurrTick)
      mAge = F32(body.currTick) * TickSec;

   if(body.hasPosition)
   {
      if(!mFlying && !body.hasCurrTick)
         mAge = 0.0f;

      mStart = mPosition = body.position;
      if(mTrail.empty())
         mTrailAt = body.position;
      if(!mFlying)
         mEmitAt = body.position;

      if(body.hasVelocity)
      {
         mVelocity = body.velocity;
         mNeedsSpeed = false;
         mBounces = true;
         mFlying = true;
      }
      else if(body.hasDirection)
      {
         mVelocity = body.direction;
         mNeedsSpeed = true;
         mBounces = false;
         mFlying = true;

         mCatchUp = body.hasCurrTick ? F32(body.currTick) * 0.032f : 0.0f;
      }
      else if(!body.hasExplodePoint)
      {
         mFlying = false;
         if(body.hasImpactNormal)
            mImpactNormal = body.impactNormal;
         explode();
      }
   }

   if(body.hasExplodePoint)
   {
      mPosition = body.explodePoint;
      if(body.hasImpactNormal)
         mImpactNormal = body.impactNormal;
      mFlying = false;
      explode();
   }
}

/// What retail's client does with the body that ends a projectile: play the
/// datablock's explosion where it ended (projectile.cc 0x00632530 and
/// Projectile::explode).
void ProjectileT2::explode()
{
   if(mExploded)
      return;

   mExploded = true;

      // The mark it leaves goes down where it went off (0x0062d9dc), facing
      // out of the surface it hit.
   if(mMarks && mData && mData->decal && gDecalManagerT2)
      gDecalManagerT2->addDecal(mPosition, mImpactNormal, mData->decal);

   ExplosionData *data = NULL;
   if(!mData || !mData->explosionId ||
      !Sim::findObject(mData->explosionId, data) || !data)
      return;

   Explosion *blast = new Explosion;
   blast->onNewDataBlock(data, false);

   MatrixF mat(true);
   mat.setPosition(mPosition);
   blast->setTransform(mat);
   blast->setInitialState(mPosition, mImpactNormal);

   if(!blast->registerObject())
   {
      Con::warnf("ProjectileT2: %s could not register its explosion.",
                 mData->getName());
      delete blast;
   }
}

void ProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(dt <= 0.0f)
      return;

      // The trail outlives the bolt: retail keeps the object a blurLifetime
      // past the explosion so what is left of it fades out rather than going
      // with it.
   ageTrail(dt);

   if(mSound)
   {
      if(mExploded || !mFlying)
         mSound->stop();
      else
      {
         if(!mSound->isPlaying())
            mSound->play();

         mSound->setVelocity(mVelocity);
         mSound->setTransform(getRenderTransform());
      }
   }

   if(!mFlying)
      return;

   if(mNeedsSpeed)
   {
      mVelocity *= mData->velocity;
      mNeedsSpeed = false;

      if(mCatchUp > 0.0f)
      {
         mPosition = mStart + mVelocity * mCatchUp;
         mTrailAt = mEmitAt = mPosition;
         mCatchUp = 0.0f;
      }
   }

   mAge += dt;

      // Each flare is replaced as soon as it has run out (0x0063df30).
   for(U32 i = 0; i < mFlares.size(); i++)
   {
      mFlares[i].age += dt;
      if(mFlares[i].age > mFlares[i].life)
         spawnFlare(mFlares[i]);
   }

   if(mData->lifetimeMS && mAge * 1000.0f > F32(mData->lifetimeMS))
   {
      mFlying = false;
      return;
   }

   step(dt);

      // The smoke runs from where the last of it was laid to where the
      // flight has got, backwards along it: Projectile::emitParticles
      // (0x00632530), which GrenadeProjectile calls a tick at a time
      // (0x006359a6). Here it is a frame at a time, so the milliseconds a
      // frame leaves over are kept for the next.
      // Under water the smoke stops and the bubbles, if there are any,
      // rise straight up instead (0x0063597e, 0x00636a48).
   if(bool(mEmitter) || bool(mBubbleEmitter))
   {
      mEmitMS += dt * 1000.0f;
      const U32 ms = U32(mEmitMS);
      if(ms)
      {
         const bool inWater = isInWaterT2(getContainer(), mPosition);
         ParticleEmitter *emitter = inWater ? mBubbleEmitter : mEmitter;

         VectorF axis = -mVelocity;
         if(inWater || axis.isZero())
            axis.set(0.0f, 0.0f, 1.0f);
         else
            axis.normalize();

         if(emitter)
            emitter->emitParticles(mEmitAt, mPosition, axis, mVelocity, ms);
         mEmitAt = mPosition;
         mEmitMS -= F32(ms);
      }
   }

      // One length of trail per tick, which is where retail lays one -- the
      // flight here is per frame, and a length a frame would be four of them
      // for every one of its.
   mSinceTrail += dt;
   if(mData->blurLifetime > 0.0f && mSinceTrail >= TickSec)
   {
      mTrail.increment();
      Trail &t = mTrail.last();
      t.from = mTrailAt;
      t.to = mPosition;
      t.ageFrom = t.ageTo = 0.0f;
      mTrailAt = mPosition;
      mSinceTrail = 0.0f;
   }

   MatrixF mat(true);
   headingMatrixT2(&mat, mVelocity);
   mat.setPosition(mPosition);
   setTransform(mat);

   if(!mShapeInstance)
      return;

      // Activate runs once and hands over to Maintain, which is the one that
      // keeps going for as long as the projectile is in the air.
   if(mFlightThread && mMaintainSeq >= 0 &&
      (S32)mShapeInstance->getSequence(mFlightThread) != mMaintainSeq &&
      mShapeInstance->getTime(mFlightThread) + dt >
         mShapeInstance->getDuration(mFlightThread))
      mShapeInstance->setSequence(mFlightThread, mMaintainSeq, 0.0f);

   mShapeInstance->advanceTime(dt);
}

/// Retail casts a projectile's ray against two masks it ors together: the
/// static half at 0x007abf60, which a force field is in next to static
/// objects, terrain and interiors, and the half at 0x007abf64 of everything
/// a projectile can damage -- players, vehicles, turrets, damageable items,
/// sensors, stations and generators, which are all shapes here.
static const U32 sProjectileStaticMask =
   StaticObjectType | TerrainObjectType | InteriorObjectType |
   ForceFieldObjectType;

static const U32 sProjectileDamageableMask =
   StaticShapeObjectType | PlayerObjectType | VehicleObjectType |
   ItemObjectType;

/// Terrain and interiors carry StaticShapeObjectType here as well as their
/// own, so the world has to be taken back out of the damageable half.
static const U32 sProjectileWorldMask =
   TerrainObjectType | InteriorObjectType;

static const U32 sProjectileCollisionMask =
   sProjectileStaticMask | sProjectileDamageableMask;

/// Age every length of the trail from both ends and drop the ones that have
/// run out, which is GrenadeBlur's advance at 0x00698380 and the pass after it
/// at 0x006983d0.
void ProjectileT2::ageTrail(F32 dt)
{
   if(mTrail.empty())
      return;

   for(U32 i = 0; i < mTrail.size(); i++)
   {
      mTrail[i].ageFrom += dt;
      mTrail[i].ageTo += dt;
   }

   while(!mTrail.empty() && mTrail.first().ageTo >= mData->blurLifetime)
      mTrail.pop_front();
}

/// One step of the flight of a grenade -- which a blaster bolt is underneath,
/// at no gravity and very nearly all of its speed kept. Both the bounce and
/// the rule that ends it are GrenadeProjectile's: the reflect, the tangent
/// rubbed off by friction and the elasticity at 0x00635001, and the test at
/// 0x00634e0f that bounces only while the projectile is younger than its
/// arming delay and goes off on the first thing it touches after that.
void ProjectileT2::step(F32 dt)
{
      // no gravity of its own holds its line.
   mVelocity.z += gWorldGravity * WorldGravityScale * mData->gravityMod * dt;

   const Point3F from = mPosition;
   const Point3F to = from + mVelocity * dt;

   SceneObject *source = NULL;
   SceneObject *vehicle = NULL;
   if(mNoSourceHit > 0.0f)
   {
      mNoSourceHit -= dt;
      if(mSourceObject)
         Sim::findObject(mSourceObject, source);
      if(mVehicleObject)
         Sim::findObject(mVehicleObject, vehicle);
      if(source)
         source->disableCollision();
      if(vehicle)
         vehicle->disableCollision();
   }

   RayInfo info;
   disableCollision();
   const bool hit = getContainer()->castRay(from, to,
                                            sProjectileCollisionMask, &info);
   enableCollision();

   if(source)
      source->enableCollision();
   if(vehicle)
      vehicle->enableCollision();

   if(!hit)
   {
      mPosition = to;
      return;
   }

   mPosition = info.point;
   mImpactNormal = info.normal;

   const U32 hitMask = info.object ? info.object->getTypeMask() : 0;

      // Only the ground and a building are marked (0x0062e09b, where the
      // mask is Tribes 2's Terrain and Interior); a shot that hits a player,
      // a vehicle or a deployable leaves nothing behind.
   mMarks = (hitMask & sProjectileWorldMask) != 0;

   const bool damageable = (hitMask & sProjectileDamageableMask) != 0 &&
                           (hitMask & sProjectileWorldMask) == 0;

      // A disc goes off on the first thing it touches; a grenade bounces
      // until its arming delay has run and goes off on the first thing after
      // that (0x00634e0f); an energy bolt also goes off on anything it can
      // damage, however young it is (0x0069588e).
   if(!mBounces || mAge * 1000.0f > F32(mData->armingDelayMS) ||
      (mStopsOnDamageable && damageable))
   {
      mFlying = false;
      explode();
      return;
   }

   const Point3F bounced =
      mVelocity - info.normal * (mDot(mVelocity, info.normal) * 2.0f);
   const Point3F tangent =
      bounced - info.normal * mDot(bounced, info.normal);

   mVelocity = (bounced - tangent * mData->friction) * mData->elasticity;

      // Off the surface, so the next step does not start inside it.
   mPosition = info.point + info.normal * 0.05f;
}

void ProjectileT2::prepRenderImage(SceneRenderState *state)
{
   if(mShapeInstance)
   {
      if(!mFlying)
         return;

      GFXTransformSaver saver;

      TSRenderState rdata;
      rdata.setSceneState(state);

      LightQuery query;
      query.init(getWorldSphere());
      rdata.setLightQuery(&query);

         // Turned to face the camera and scaled by the datablock, as
         // Projectile::renderObject does it (0x00632880, 0x00632ad1).
      MatrixF mat = getRenderTransform();
      const Point3F at = mat.getPosition();
      VectorF toEye = state->getCameraPosition() - at;
      if(mData->faceViewer && !toEye.isZero())
      {
         toEye.normalize();
         mat = MathUtils::createOrientFromDir(toEye);
         mat.setPosition(at);
      }
      mat.scale(mData->scale);
      GFX->setWorldMatrix(mat);

      mShapeInstance->setDetailFromPosAndScale(state, at, mData->scale);
      mShapeInstance->animate();
      mShapeInstance->render(rdata);
      if(mFlares.empty())
         return;
   }
   else if(mTexture[0].isNull() && mTrail.empty())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &ProjectileT2::renderBolt);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// Projectile::registerLights (0x006323d0): a point light where the
/// projectile is drawn, with the datablock's radius and colour, for as long
/// as it has not gone off (0x006348c4). Its colour under water is taken only
/// by a grenade's family, the one projectile that keeps track of whether it
/// is in water (0x00635c01).
void ProjectileT2::submitLights(LightManager *lm, bool staticLighting)
{
   if(staticLighting || !mData || !mData->hasLight || !mFlying)
      return;

   if(!mLight)
      mLight = LightManager::createLightInfo();

   mLight->setType(LightInfo::Point);
   mLight->setPosition(getRenderPosition());
   mLight->setRange(mData->lightRadius);
   const bool underwater = mData->hasWaterLight && mBounces &&
                           isInWaterT2(getContainer(), mPosition);
   mLight->setColor(underwater ? mData->waterLightColor : mData->lightColor);

   lm->registerGlobalLight(mLight, this);
}

/// How much of the fog lies between the eye and a point: the haze that
/// thickens from fogDistance out to visibleDistance and the share of each fog
/// volume the line crosses, which is the scene's getHazeAndFog (0x00570b40).
static F32 fogBetween(const FogData &fog, const Point3F &eye,
                      const Point3F &at)
{
   const F32 dist = (at - eye).len();

   const F32 t = mClampF((dist - fog.densityOffset) * fog.hazeScale,
                         0.0f, 1.0f) - 1.0f;
   F32 fogged = 1.0f - t * t;

   const F32 dz = at.z - eye.z;
   for(U32 i = 0; i < FogData::MaxBands; i++)
   {
      if(fog.bandFactor[i] == 0.0f)
         continue;

      const F32 lo = fog.bandMin[i];
      const F32 hi = fog.bandMax[i];
      F32 frac;
      if(mFabs(dz) > 1e-4f)
         frac = (mClampF(at.z, lo, hi) - mClampF(eye.z, lo, hi)) / dz;
      else
         frac = (eye.z >= lo && eye.z <= hi) ? 1.0f : 0.0f;
      fogged += dist * frac * fog.bandFactor[i];
   }

   return mClampF(fogged, 0.0f, 1.0f);
}

/// Everything retail's EnergyProjectile::renderObject (0x00696dd0) draws, in
/// the order it draws it: the trail first, and then the bolt and its cross
/// only while the bolt is still there.
void ProjectileT2::renderBolt(ObjectRenderInst *, SceneRenderState *state,
                              BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   const Point3F eye = state->getCameraPosition();

   drawTrail(eye);

   if(!mFlying)
      return;

      // What the fog leaves of them, by the distance and the climb to the
      // bolt (0x0063e3ba): they are added into the scene, so they fade out
      // into the fog rather than being blended towards its colour.
   const F32 clear = 1.0f - fogBetween(state->getSceneManager()->getFogData(),
                                       eye, getRenderPosition());
   drawFlares(clear);
   if(mShapeInstance)
      return;

   if(dynamic_cast<LinearFlareProjectileData *>(mData))
   {
      drawGlow(state, clear);
      return;
   }

   if(mData->streakLength > 0.0f)
      drawStreak(eye);
   else
      drawBolt(eye);

   drawCross(eye);
}

/// A length of trail is a flat quad, untextured, carrying the datablock's
/// blurColor at an alpha of one less the share of its life each end has used.
/// The oldest length has its far end pinned transparent, which is what keeps
/// the trail from ending in a hard edge.
void ProjectileT2::drawTrail(const Point3F &eye)
{
   if(mTrail.empty() || mData->blurLifetime <= 0.0f)
      return;

   if(mTrailSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      mTrailSB = GFX->createStateBlock(desc);
   }

   GFX->setStateBlock(mTrailSB);
   GFX->setupGenericShaders(GFXDevice::GSColor);

   const F32 half = mData->blurWidth * 0.5f;
   const ColorF &c = mData->blurColor;

   for(U32 i = 0; i < mTrail.size(); i++)
   {
      const Trail &t = mTrail[i];

      VectorF across = mCross(t.to - t.from, t.from - eye);
      if(across.isZero())
         continue;
      across.normalize();
      across *= half;

      const F32 a = i ? 1.0f - t.ageFrom / mData->blurLifetime : 0.0f;
      const F32 b = 1.0f - t.ageTo / mData->blurLifetime;

      PrimBuild::begin(GFXTriangleFan, 4);
      PrimBuild::color4f(c.red, c.green, c.blue, a);
      PrimBuild::vertex3fv(t.from + across);
      PrimBuild::vertex3fv(t.from - across);
      PrimBuild::color4f(c.red, c.green, c.blue, b);
      PrimBuild::vertex3fv(t.to - across);
      PrimBuild::vertex3fv(t.to + across);
      PrimBuild::end(false);
   }
}

/// What all three of the textured quads are drawn with, which is the state
/// retail sets before each of them: no cull, no depth write, and the blend
/// that adds them into the scene.
GFXStateBlockRef ProjectileT2::boltStateBlock()
{
   if(mBoltSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      mBoltSB = GFX->createStateBlock(desc);
   }

   return mBoltSB;
}

/// A bullet is the streak it leaves: a quad from where it is back along its
/// flight by the whole of tracerLength, tracerWidth to either side of it, flat
/// into the view. Retail builds it at 0x006406c0 -- the width is not halved
/// there, and the colour of the quad is white whatever tracerColor says.
void ProjectileT2::drawStreak(const Point3F &eye)
{
   if(mTexture[0].isNull() || mVelocity.isZero())
      return;

   const Point3F at = getRenderTransform().getPosition();

   VectorF across = mCross(at - eye, mVelocity);
   if(across.isZero())
      return;
   across.normalize();
   across *= mData->streakWidth;

   VectorF along = mVelocity;
   along.normalize();

   const Point3F tail = at - along * mData->streakLength;

   GFX->setStateBlock(boltStateBlock());
   GFX->setTexture(0, mTexture[0]);

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(0.0f, 0.0f);
   PrimBuild::vertex3fv(tail + across);
   PrimBuild::texCoord2f(0.0f, 1.0f);
   PrimBuild::vertex3fv(tail - across);
   PrimBuild::texCoord2f(1.0f, 1.0f);
   PrimBuild::vertex3fv(at - across);
   PrimBuild::texCoord2f(1.0f, 0.0f);
   PrimBuild::vertex3fv(at + across);
   PrimBuild::end();
}

/// The bolt itself: a quad along the flight, as wide as the datablock's scale
/// in x and as long as that again in its y, drawn flat into the view. Retail
/// builds it at 0x00696ed0 from the velocity and the way to the camera, and
/// takes the half length as scale.x * scale.y * 0.5.
void ProjectileT2::drawBolt(const Point3F &eye)
{
   if(mTexture[0].isNull() || mVelocity.isZero())
      return;

   const Point3F at = getRenderTransform().getPosition();

   VectorF across = mCross(at - eye, mVelocity);
   if(across.isZero())
      return;
   across.normalize();
   across *= mData->scale.x;

   VectorF along = mVelocity;
   along.normalize();
   along *= mData->scale.x * mData->scale.y * 0.5f;

   const Point3F tail = at - along;
   const Point3F nose = at + along;

   GFX->setStateBlock(boltStateBlock());
   GFX->setTexture(0, mTexture[0]);

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(0.0f, 0.0f);
   PrimBuild::vertex3fv(tail + across);
   PrimBuild::texCoord2f(0.0f, 1.0f);
   PrimBuild::vertex3fv(tail - across);
   PrimBuild::texCoord2f(1.0f, 1.0f);
   PrimBuild::vertex3fv(nose - across);
   PrimBuild::texCoord2f(1.0f, 0.0f);
   PrimBuild::vertex3fv(nose + across);
   PrimBuild::end();
}

/// A bolt coming at the camera is a quad the width of its flight instead, and
/// retail draws that one only once the flight is within crossViewAng of the
/// way to the eye (0x006973b0).
void ProjectileT2::drawCross(const Point3F &eye)
{
   if(mTexture[1].isNull() || mData->crossSize <= 0.0f || mVelocity.isZero())
      return;

   const Point3F at = getRenderTransform().getPosition();

   VectorF toEye = at - eye;
   if(toEye.isZero())
      return;
   toEye.normalize();

   VectorF along = mVelocity;
   along.normalize();

   if(mFabs(mDot(along, toEye)) < mData->crossViewAng)
      return;

      // Square to the flight, which is what the matrix retail loads for it
      // puts the quad in.
   VectorF right = mCross(along, Point3F(0.0f, 0.0f, 1.0f));
   if(right.len() < 0.001f)
      right = mCross(along, Point3F(1.0f, 0.0f, 0.0f));
   right.normalize();
   const VectorF up = mCross(right, along);

   const F32 half = mData->crossSize * 0.5f;
   const Point3F r = right * half;
   const Point3F u = up * half;

   GFX->setStateBlock(boltStateBlock());
   GFX->setTexture(0, mTexture[1]);

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(0.0f, 0.0f);
   PrimBuild::vertex3fv(at - r + u);
   PrimBuild::texCoord2f(0.0f, 1.0f);
   PrimBuild::vertex3fv(at - r - u);
   PrimBuild::texCoord2f(1.0f, 1.0f);
   PrimBuild::vertex3fv(at + r - u);
   PrimBuild::texCoord2f(1.0f, 0.0f);
   PrimBuild::vertex3fv(at + r + u);
   PrimBuild::end();
}

/// A fresh flare, from the ranges retail's advance draws one from
/// (0x0063dfe3): it starts out reaching size[1] and a little more, ends
/// reaching size[2] and up to a metre more, peaks inside the first 0.4 of a
/// second and is gone inside the first one.
void ProjectileT2::spawnFlare(Flare &flare)
{
   flare.way.set(1.0f - 2.0f * gRandGen.randF(),
                 1.0f - 2.0f * gRandGen.randF(),
                 1.0f - 2.0f * gRandGen.randF());
   flare.way.normalizeSafe();
   flare.from = mData->flareSize[1] + gRandGen.randF() * 0.25f;
   flare.to = mData->flareSize[2] + gRandGen.randF();
   flare.tilt = mDegToRad(65.0f + gRandGen.randF() * 20.0f);
   flare.peak = 0.15f + gRandGen.randF() * 0.225f;
   flare.life = 0.4f + gRandGen.randF() * 0.595f;
   flare.age = 0.0f;
}

/// What retail draws of a LinearFlareProjectile besides its shape
/// (0x0063e2b0): each flare a ring of eight quads from size[0] out along its
/// way, flareModTexture's root at the bolt and its tip at the far end, added
/// into the scene and culled like any other face. A flare grows out nearly
/// closed, at 87 degrees, until its peak, then holds its reach and opens out
/// to its tilt while it fades, and its tip is half and a quarter as bright
/// as its root (0x0063ee80, 0x0063f4e0). All of it is dimmed by what the
/// fog leaves clear.
void ProjectileT2::drawFlares(F32 clear)
{
   if(mFlares.empty() || mFlareTexture.isNull() || clear <= 0.0f)
      return;

   if(mFlareSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendOne, GFXBlendOne);
      desc.setCullMode(GFXCullCCW);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      mFlareSB = GFX->createStateBlock(desc);
   }

   GFX->setStateBlock(mFlareSB);
   GFX->setTexture(0, mFlareTexture);

   static const F32 sClosed = mDegToRad(87.0f);
   static const F32 sU[6] = { 0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f };
   static const F32 sV[6] = { 0.9f, 0.9f, 0.1f, 0.1f, 0.1f, 0.9f };
   static const U32 sShade[6] = { 0, 0, 1, 2, 1, 0 };
   static const U32 sFan[12] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5 };

   const MatrixF &mat = getRenderTransform();
   const ColorF c = mData->flareColor * clear;
   const F32 root = mData->flareSize[0];

   PrimBuild::begin(GFXTriangleList, mFlares.size() * 4 * 12);
   for(U32 i = 0; i < mFlares.size(); i++)
   {
      const Flare &f = mFlares[i];

      F32 angle, reach, glow;
      if(f.age < f.peak)
      {
         glow = f.age / f.peak;
         angle = sClosed;
         reach = f.from + (f.to - f.from) * glow;
      }
      else
      {
         const F32 t = (f.age - f.peak) / (f.life - f.peak);
         glow = 1.0f - t;
         angle = sClosed + (f.tilt - sClosed) * t;
         reach = f.to;
      }

      const ColorF shade[3] = { c * glow, c * (glow * 0.5f),
                                c * (glow * 0.25f) };

      VectorF u = mCross(f.way, mFabs(f.way.x) < 0.9f ? Point3F(1, 0, 0)
                                                      : Point3F(0, 1, 0));
      u.normalize();
      VectorF v = mCross(f.way, u);
      v.normalize();
      u *= mCos(angle);
      v *= mCos(angle);

      const VectorF sink = f.way * (1.0f - mSin(angle));
      const VectorF side[8] = { -v - u, -u, v - u, v, v + u, u, u - v, -v };

      Point3F in[8], out[8];
      for(U32 k = 0; k < 8; k++)
      {
         in[k] = (f.way + side[k]) * root;
         out[k] = (f.way + side[k]) * reach - sink;
         mat.mulP(in[k]);
         mat.mulP(out[k]);
      }

      for(U32 k = 0; k < 8; k += 2)
      {
         const U32 j = (k + 2) & 7;
         const Point3F *at[6] = { &in[k + 1], &in[k], &out[k], &out[k + 1],
                                  &out[j], &in[j] };
         for(U32 n = 0; n < 12; n++)
         {
            const U32 m = sFan[n];
            PrimBuild::color(shade[sShade[m]]);
            PrimBuild::texCoord2f(sU[m], sV[m]);
            PrimBuild::vertex3fv(*at[m]);
         }
      }
   }
   PrimBuild::end();
}

/// What retail draws of a LinearFlareProjectile that has no shape, after its
/// flares (0x0063e50a): two squares of flareBaseTexture flat to the view and
/// added into the scene, one 1.2 of scale.x across in the root of the flare
/// colour and one half that across in white, both dimmed by the fog.
void ProjectileT2::drawGlow(const SceneRenderState *state, F32 clear)
{
   if(mTexture[1].isNull() || clear <= 0.0f)
      return;

   const MatrixF &cam = state->getCameraTransform();
   VectorF right, up;
   cam.getColumn(0, &right);
   cam.getColumn(2, &up);

   const Point3F at = getRenderPosition();
   const ColorF &c = mData->flareColor;
   const ColorF shade[2] = {
      ColorF(mSqrt(c.red) * clear, mSqrt(c.green) * clear,
             mSqrt(c.blue) * clear),
      ColorF(clear, clear, clear) };
   const F32 half[2] = { mData->scale.x * 0.6f, mData->scale.x * 0.3f };

   GFX->setStateBlock(boltStateBlock());
   GFX->setTexture(0, mTexture[1]);

   for(U32 i = 0; i < 2; i++)
   {
      const Point3F r = right * half[i];
      const Point3F u = up * half[i];

      PrimBuild::color(shade[i]);
      PrimBuild::begin(GFXTriangleFan, 4);
      PrimBuild::texCoord2f(0.0f, 0.0f);
      PrimBuild::vertex3fv(at - r - u);
      PrimBuild::texCoord2f(0.0f, 1.0f);
      PrimBuild::vertex3fv(at - r + u);
      PrimBuild::texCoord2f(1.0f, 1.0f);
      PrimBuild::vertex3fv(at + r + u);
      PrimBuild::texCoord2f(1.0f, 0.0f);
      PrimBuild::vertex3fv(at + r - u);
      PrimBuild::end();
   }
}

#define IMPLEMENT_PROJECTILE_T2( className, stopsOnDamageable )              \
   class className : public ProjectileT2                                     \
   {                                                                         \
      typedef ProjectileT2 Parent;                                           \
   public:                                                                   \
      className() { mStopsOnDamageable = stopsOnDamageable; }                \
      DECLARE_CONOBJECT(className);                                          \
   };                                                                        \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 projectile.\n\n@internal" )

IMPLEMENT_PROJECTILE_T2( EnergyProjectile, true );
IMPLEMENT_PROJECTILE_T2( GrenadeProjectile, false );
IMPLEMENT_PROJECTILE_T2( LinearFlareProjectile, false );
IMPLEMENT_PROJECTILE_T2( LinearProjectile, false );
IMPLEMENT_PROJECTILE_T2( TracerProjectile, false );

//----------------------------------------------------------------------------

/// What both of Tribes 2's beams are drawn with. A sniper shot names twelve
/// textures and a targeting beam four, each in its own order.
class BeamProjectileDataT2 : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum { MaxTextures = 12 };

   F32    range;           ///< how far a source aims one
   ColorF color;
   F32    fadeTime;        ///< a sniper shot's, over which it goes
   F32    startWidth;
   F32    endWidth;        ///< what a sniper shot has spread to by then
   F32    pulseWidth;
   F32    flareAngle;      ///< degrees off the beam its start flares from
   F32    minFlareSize;
   F32    maxFlareSize;    ///< pixels
   F32    pulseSpeed;
   F32    pulseLength;
   U32    explosionId;
   U32    splashId;
   String textureName[MaxTextures];
   GFXTexHandle texture[MaxTextures];

   BeamProjectileDataT2();
   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(BeamProjectileDataT2);
};

IMPLEMENT_CONOBJECT(BeamProjectileDataT2);

ConsoleDocClass( BeamProjectileDataT2,
   "@brief What a Tribes 2 beam's datablock says about how it is drawn.\n\n"
   "@internal" );

BeamProjectileDataT2::BeamProjectileDataT2()
   : range(1000.0f),
     color(1.0f, 1.0f, 1.0f, 1.0f),
     fadeTime(1.0f),
     startWidth(0.25f),
     endWidth(0.5f),
     pulseWidth(0.5f),
     flareAngle(3.0f),
     minFlareSize(0.0f),
     maxFlareSize(400.0f),
     pulseSpeed(6.0f),
     pulseLength(0.15f),
     explosionId(0),
     splashId(0)
{
}

bool BeamProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamRange, &value))
      range = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbBeamColor, &word))
      color.set(F32(word & 0xff) / 255.0f, F32((word >> 8) & 0xff) / 255.0f,
                F32((word >> 16) & 0xff) / 255.0f,
                F32((word >> 24) & 0xff) / 255.0f);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamFadeTime, &value))
      fadeTime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamStartWidth, &value))
      startWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamEndWidth, &value))
      endWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamPulseWidth, &value))
      pulseWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamFlareAngle, &value))
      flareAngle = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamMinFlareSize, &value))
      minFlareSize = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamMaxFlareSize, &value))
      maxFlareSize = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamPulseSpeed, &value))
      pulseSpeed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamPulseLength, &value))
      pulseLength = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbExplosion, &word))
      explosionId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileSplash, &word))
      splashId = word;

      // The shape ProjectileData names comes first; the textures are every
      // string after it.
   const Vector<String> &strings = T2Layout::lastStrings();
   for(U32 i = 0; i < MaxTextures && i + 1 < strings.size(); i++)
      textureName[i] = strings[i + 1];

   return true;
}

bool BeamProjectileDataT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(!server)
      for(U32 i = 0; i < MaxTextures; i++)
         if(textureName[i].isNotEmpty())
            texture[i].set(String("textures/") + textureName[i],
                           &GFXDefaultStaticDiffuseProfile, "BeamProjectileT2");

   return true;
}

#define IMPLEMENT_BEAM_DATA_T2( className )                                  \
   class className : public BeamProjectileDataT2                             \
   {                                                                         \
      typedef BeamProjectileDataT2 Parent;                                   \
   public:                                                                   \
      DECLARE_CONOBJECT(className);                                          \
   };                                                                        \
   IMPLEMENT_CO_DATABLOCK_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 beam's datablock."         \
                               "\n\n@internal" )

IMPLEMENT_BEAM_DATA_T2( SniperProjectileData );
IMPLEMENT_BEAM_DATA_T2( TargetProjectileData );

//----------------------------------------------------------------------------

/// A beam is told to a client as its two ends, or as a source and its muzzle
/// slot instead of the start. One that follows its source has its end worked
/// out by the client too, every frame, from where that source is aiming.
class BeamProjectileT2 : public GameBase
{
   typedef GameBase Parent;

protected:
   BeamProjectileDataT2 *mData;
   bool    mFades;            ///< a sniper shot goes over its fadeTime; a
                              ///< targeting beam lasts until it is deleted
   F32     mEaseTime;         ///< how long a moved end takes to get there
   Point3F mStart;
   Point3F mEnd;
   Point3F mEndFrom;
   Point3F mEndTo;
   F32     mEase;             ///< seconds into that, or < 0 once it is there
   F32     mElapsed;
   F32     mEnergy;
   bool    mHit;
   bool    mWet;
   bool    mFollow;
   SimObjectId mSourceObject;
   U32     mSourceSlot;
   GFXStateBlockRef mBeamSB;
   GFXStateBlockRef mFlareSB;
   GFXStateBlockRef mGlowSB;

   ShapeBase *source() const;
   void renderBeam(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);
   void drawQuad(const VectorF &across, F32 width, F32 u0, F32 du);
   F32 flareShare(SceneRenderState *state, const VectorF &along);
   void drawSprite(SceneRenderState *state, const Point3F &at, F32 width,
                   F32 spin);

public:
   BeamProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(BeamProjectileT2);
};

IMPLEMENT_CONOBJECT(BeamProjectileT2);

ConsoleDocClass( BeamProjectileT2,
   "@brief A Tribes 2 beam, drawn by the client that was told of it.\n\n"
   "@internal" );

BeamProjectileT2::BeamProjectileT2()
   : mData(NULL),
     mFades(false),
     mEaseTime(0.15f),
     mStart(0.0f, 0.0f, 0.0f),
     mEnd(0.0f, 0.0f, 0.0f),
     mEndFrom(0.0f, 0.0f, 0.0f),
     mEndTo(0.0f, 0.0f, 0.0f),
     mEase(-1.0f),
     mElapsed(0.0f),
     mEnergy(1.0f),
     mHit(false),
     mWet(false),
     mFollow(false),
     mSourceObject(0),
     mSourceSlot(0)
{
   mTypeMask |= ProjectileObjectType;
}

bool BeamProjectileT2::onAdd()
{
   if(!Parent::onAdd())
      return false;

      // A beam can be a kilometre long and go anywhere; retail gives it a
      // box of 1e7 either way (0x006425e7).
   setGlobalBounds();
   resetWorldBox();
   addToScene();

   if(isClientObject() && mData && mFades)
   {
         // A weaker shot starts that much of the way through its fade
         // (0x006421c0), and goes off where it hit unless that was water,
         // where it splashes instead (0x006424f2).
      mElapsed = (1.0f - mEnergy) * mData->fadeTime;

      ExplosionData *data = NULL;
      if(mHit && !mWet && mData->explosionId &&
         Sim::findObject(mData->explosionId, data) && data)
      {
         VectorF along = mEnd - mStart;
         along.normalizeSafe();

         Explosion *blast = new Explosion;
         blast->onNewDataBlock(data, false);

         MatrixF mat(true);
         mat.setPosition(mEnd);
         blast->setTransform(mat);
         blast->setInitialState(mEnd, along);
         if(!blast->registerObject())
            delete blast;
      }

      SplashData *splashData = NULL;
      if(mHit && mWet && mData->splashId &&
         Sim::findObject(mData->splashId, splashData) && splashData)
      {
         MatrixF mat = getTransform();
         mat.setPosition(mEnd);

         Splash *splash = new Splash;
         splash->onNewDataBlock(splashData, false);
         splash->setTransform(mat);
         splash->setInitialState(mEnd, Point3F(0.0f, 0.0f, 1.0f));
         if(!splash->registerObject())
            delete splash;
      }
   }

   return true;
}

void BeamProjectileT2::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

bool BeamProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<BeamProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   return true;
}

ShapeBase *BeamProjectileT2::source() const
{
   ShapeBase *obj = NULL;
   if(mSourceObject)
      Sim::findObject(mSourceObject, obj);
   return obj;
}

void BeamProjectileT2::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   mSourceObject = 0;
   if(body.hasSourceObject)
   {
      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(S32(body.sourceObject))
                            : NULL;
      mSourceObject = obj ? obj->getId() : 0;
      mSourceSlot = getMin(body.sourceSlot,
                           U32(ShapeBase::MaxMountedImages - 1));
   }

   mFollow = body.beamFollow && mSourceObject;
   mHit = body.beamHit;

   if(body.hasPosition)
      mStart = body.position;

   if(body.beamInitial)
   {
      mEnd = mEndFrom = mEndTo = body.explodePoint;
      mEase = -1.0f;
      mWet = body.beamWet;
      if(body.hasBeamEnergy)
         mEnergy = body.beamEnergy;
   }
   else if(body.hasExplodePoint && !mFollow)
   {
         // A moved end is eased to rather than jumped to (0x00643740,
         // 0x00649800).
      mEndFrom = mEnd;
      mEndTo = body.explodePoint;
      mEase = 0.0f;
   }
}

void BeamProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(!mData || dt <= 0.0f)
      return;

   mElapsed += dt;

      // A targeting beam starts at its source's muzzle wherever that has got
      // to (0x006497b0); a sniper shot only while it follows.
   ShapeBase *src = source();
   if(src && (mFollow || !mFades))
      src->getRenderMuzzlePoint(mSourceSlot, &mStart);

   if(mFollow && src)
   {
         // Its end is as far as the source can aim it, or the first thing
         // in the way (0x006493d0).
      VectorF aim;
      src->getRenderMuzzleVector(mSourceSlot, &aim);
      mEnd = mStart + aim * mData->range;

      RayInfo info;
      src->disableCollision();
      mHit = getContainer()->castRay(mStart, mEnd,
                                     sProjectileCollisionMask | WaterObjectType,
                                     &info);
      src->enableCollision();
      if(mHit)
         mEnd = info.point;
   }
   else if(mEase >= 0.0f)
   {
      mEase += dt;
      const F32 t = mClampF(mEase / mEaseTime, 0.0f, 1.0f);
      mEnd = mEndFrom + (mEndTo - mEndFrom) * t;
      if(t >= 1.0f)
         mEase = -1.0f;
   }
}

void BeamProjectileT2::prepRenderImage(SceneRenderState *state)
{
   if(!mData || (mFades && mElapsed >= mData->fadeTime))
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &BeamProjectileT2::renderBeam);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// One quad from the start to the end, width wide, flat into the view; its
/// texture runs along it from u0 for du (0x00677b80).
void BeamProjectileT2::drawQuad(const VectorF &across, F32 width, F32 u0,
                                F32 du)
{
   const Point3F side = across * (width * 0.5f);

   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(u0, 0.0f);
   PrimBuild::vertex3fv(mStart + side);
   PrimBuild::texCoord2f(u0, 1.0f);
   PrimBuild::vertex3fv(mStart - side);
   PrimBuild::texCoord2f(u0 + du, 1.0f);
   PrimBuild::vertex3fv(mEnd - side);
   PrimBuild::texCoord2f(u0 + du, 0.0f);
   PrimBuild::vertex3fv(mEnd + side);
   PrimBuild::end();
}

/// How much of its flare a beam's start shows: none unless the beam comes
/// at the camera and the camera faces it, and all of it only straight down
/// the beam, flareAngle being where it runs out (0x00642940).
F32 BeamProjectileT2::flareShare(SceneRenderState *state, const VectorF &along)
{
   const Point3F eye = state->getCameraPosition();
   VectorF toStart = mStart - eye;
   toStart.normalizeSafe();
   const F32 dot = mDot(toStart, along);

   VectorF facing;
   state->getCameraTransform().getColumn(1, &facing);
   if(dot >= -0.5f || mDot(facing, along) >= -0.5f)
      return 0.0f;

   ShapeBase *src = source();
   RayInfo info;
   if(src)
      src->disableCollision();
   const bool hidden = getContainer()->castRay(eye, mStart,
                          sProjectileCollisionMask | WaterObjectType, &info);
   if(src)
      src->enableCollision();
   if(hidden || mData->maxFlareSize <= 0.0f)
      return 0.0f;

   const F32 edge = -mCos(mDegToRad(mData->flareAngle));
   if(dot >= edge)
      return mData->minFlareSize / mData->maxFlareSize;

   return (dot - edge) / (-1.0f - edge);
}

/// A square facing the camera, width wide and turned spin radians in the
/// view, as retail draws its flares.
void BeamProjectileT2::drawSprite(SceneRenderState *state, const Point3F &at,
                                  F32 width, F32 spin)
{
   const MatrixF &cam = state->getCameraTransform();
   VectorF right, up;
   cam.getColumn(0, &right);
   cam.getColumn(2, &up);

   const F32 half = width * 0.5f;
   const F32 c = mCos(spin) * half;
   const F32 s = mSin(spin) * half;
   const VectorF a = right * c + up * s;
   const VectorF b = up * c - right * s;

   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(0.0f, 0.0f);
   PrimBuild::vertex3fv(at - a - b);
   PrimBuild::texCoord2f(0.0f, 1.0f);
   PrimBuild::vertex3fv(at - a + b);
   PrimBuild::texCoord2f(1.0f, 1.0f);
   PrimBuild::vertex3fv(at + a + b);
   PrimBuild::texCoord2f(1.0f, 0.0f);
   PrimBuild::vertex3fv(at + a - b);
   PrimBuild::end();
}

/// SniperProjectile::renderObject (0x00642820) and TargetProjectile's
/// (0x00647de0): the flare at the start, the glow at a targeting beam's
/// end, then the beam in two passes.
void BeamProjectileT2::renderBeam(ObjectRenderInst *, SceneRenderState *state,
                                  BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   const Point3F eye = state->getCameraPosition();
   VectorF along = mEnd - mStart;
   const F32 length = along.len();
   if(length < 0.001f)
      return;
   along /= length;

   VectorF across = mCross(along, mStart - eye);
   if(across.isZero())
      return;
   across.normalize();

   if(mBeamSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mBeamSB = GFX->createStateBlock(desc);

      desc.setZReadWrite(false, false);
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      mFlareSB = GFX->createStateBlock(desc);

      desc.setBlend(true, GFXBlendOne, GFXBlendOne);
      mGlowSB = GFX->createStateBlock(desc);
   }

   const F32 t = mFades ? mClampF(mElapsed / mData->fadeTime, 0.0f, 1.0f)
                        : 0.0f;
   ColorF color = mData->color;
   color.alpha = 1.0f - t;

      // The flare is pixels across wherever the start is, which is how far
      // away a world-sized one has to be scaled.
   const F32 share = flareShare(state, along);
   const GFXTexHandle &flare = mData->texture[mFades ? 0 : 1];
   if(share > 0.0f && flare.isValid())
   {
      VectorF facing;
      state->getCameraTransform().getColumn(1, &facing);
      const F32 depth = getMax(mDot(mStart - eye, facing), 0.01f);
      const F32 width = mData->maxFlareSize * share * depth /
                        state->getWorldToScreenScale().y;

      GFX->setStateBlock(mFlareSB);
      GFX->setTexture(0, flare);
      PrimBuild::color(color);
      drawSprite(state, mStart, width, mElapsed);
      drawSprite(state, mStart, width * 0.75f, -mElapsed);
   }

      // A targeting beam's end glows green over whatever it is on, unless
      // something else is in the way (0x00648090, 0x00648cb0).
   if(!mFades && mData->texture[3].isValid())
   {
      const Point3F glow = mEnd - along * 0.2f;
      ShapeBase *src = source();
      RayInfo info;
      if(src)
         src->disableCollision();
      const bool hidden = getContainer()->castRay(eye, glow,
                          sProjectileCollisionMask | WaterObjectType, &info);
      if(src)
         src->enableCollision();

      if(!hidden)
      {
         GFX->setStateBlock(mGlowSB);
         GFX->setTexture(0, mData->texture[3]);
         PrimBuild::color4f(0.0f, 1.0f, 0.0f, 1.0f);
         drawSprite(state, mEnd, 0.6f, 0.0f);
      }
   }

   GFX->setStateBlock(mBeamSB);

   if(mFades)
   {
         // A sniper shot widens as it fades, its core white and its colour
         // over it a texture that tears as it goes (0x00642f60).
      const F32 width = mData->startWidth +
                        (mData->endWidth - mData->startWidth) * t;
      const U32 rip = getMin(U32(t * 10.0f + 1.0f), U32(BeamProjectileDataT2::MaxTextures - 1));

      GFX->setTexture(0, mData->texture[11]);
      PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f - t);
      drawQuad(across, width, 0.0f, 1.0f);

      GFX->setTexture(0, mData->texture[rip]);
      PrimBuild::color(color);
      drawQuad(across, width * 1.25f, -mData->pulseSpeed * mElapsed * 0.5f,
               length * mData->pulseLength);
   }
   else
   {
         // A targeting beam is a pulse running out along it under a steady
         // core (0x00648810).
      PrimBuild::color(color);
      GFX->setTexture(0, mData->texture[2]);
      drawQuad(across, mData->pulseWidth, -mData->pulseSpeed * mElapsed,
               length * mData->pulseLength);

      GFX->setTexture(0, mData->texture[0]);
      drawQuad(across, mData->startWidth, 0.0f, 1.0f);
   }
}

#define IMPLEMENT_BEAM_T2( className, fades, easeTime )                      \
   class className : public BeamProjectileT2                                 \
   {                                                                         \
      typedef BeamProjectileT2 Parent;                                       \
   public:                                                                   \
      className() { mFades = fades; mEaseTime = easeTime; }                  \
      DECLARE_CONOBJECT(className);                                          \
   };                                                                        \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 beam.\n\n@internal" )

// A sniper shot's moved end takes ten ticks to get there, a targeting
// beam's 0.15 seconds.
IMPLEMENT_BEAM_T2( SniperProjectile, true,  0.32f );
IMPLEMENT_BEAM_T2( TargetProjectile, false, 0.15f );

//----------------------------------------------------------------------------

/// What a repair beam's datablock says about how it is drawn. It shares only
/// its range with the sniper's beam, so it is its own class rather than more
/// fields on that one: a width, the segments it is built from, how fast the
/// texture runs along it, how often that texture repeats, how often it leaves
/// a fading copy of itself behind and how long one of those lives, and the
/// angle off the muzzle it will still be drawn at.
class RepairProjectileDataT2 : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   F32    range;
   F32    width;
   U32    segments;
   F32    speed;
   F32    texRepeat;
   F32    blurFreq;
   F32    blurLifetime;
   F32    cutoffAngle;      ///< degrees
   U32    soundId;
   String textureName[3];
   GFXTexHandle texture[3];

   RepairProjectileDataT2();
   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(RepairProjectileDataT2);
};

IMPLEMENT_CONOBJECT(RepairProjectileDataT2);

ConsoleDocClass( RepairProjectileDataT2,
   "@brief What a Tribes 2 repair beam's datablock says about how it is "
   "drawn.\n\n@internal" );

RepairProjectileDataT2::RepairProjectileDataT2()
   : range(10.0f),
     width(0.15f),
     segments(20),
     speed(1.0f),
     texRepeat(0.2f),
     blurFreq(10.0f),
     blurLifetime(1.0f),
     cutoffAngle(25.0f),
     soundId(0)
{
}

bool RepairProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBeamRange, &value))
      range = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairWidth, &value))
      width = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbRepairSegments, &word))
      segments = word ? word : 1;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairSpeed, &value))
      speed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairTexRepeat, &value))
      texRepeat = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairBlurFreq, &value))
      blurFreq = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairBlurLifetime, &value))
      blurLifetime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbRepairCutoffAngle, &value))
      cutoffAngle = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbProjectileSound, &word))
      soundId = word;

      // The shape name comes first, as it does for every projectile; the
      // beam's textures are the strings after it -- two for a repair beam,
      // three for an ELF.
   const Vector<String> &strings = T2Layout::lastStrings();
   for(U32 i = 0; i < 3 && i + 1 < strings.size(); i++)
      textureName[i] = strings[i + 1];

   return true;
}

bool RepairProjectileDataT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(!server)
      for(U32 i = 0; i < 3; i++)
         if(textureName[i].isNotEmpty())
            texture[i].set(String("textures/") + textureName[i],
                           &GFXDefaultStaticDiffuseProfile,
                           "RepairProjectileT2");

   return true;
}

/// Tribes 2 does not draw a beam as a straight line. It fits a quadratic to
/// three points -- the muzzle, a control point one beam-length along the gun's
/// barrel, and the far end -- and walks that. The weighting is its own:
/// P(t) = (p2 - p1)t^2 + (p1 - p0)t + p0, which passes through p0 and p2 and
/// leaves p0 along p1, so the beam exits down the barrel and bends to whatever
/// it has hold of.
struct T2BeamCurve
{
   Point3F quad;
   Point3F lin;
   Point3F con;

   void set(const Point3F &p0, const Point3F &p1, const Point3F &p2)
   {
      quad = p2 - p1;
      lin  = p1 - p0;
      con  = p0;
   }

   Point3F at(F32 t) const { return quad * (t * t) + lin * t + con; }
};

/// A strip of quads along a curve, each turned flat into the view, with the
/// texture running along it. The texture repeats over the straight distance
/// between the curve's ends times the datablock's repeat -- a multiplier, not
/// a divisor -- and slides along by the scroll it is given.
///
/// Tribes 2 has two of these: one for a beam, which leaves the colour to its
/// caller (0x0044da90), and one for a fading copy of a beam, which carries a
/// colour and draws the first pair of corners at no alpha at all so that the
/// copy comes out of the muzzle rather than starting there (0x0044de00).
static void drawT2BeamRibbon(const T2BeamCurve &curve, U32 segments,
                             F32 halfWidth, F32 scroll, F32 repeats,
                             const Point3F &eye, const ColorF *color = NULL)
{
   if(segments < 2)
      return;

   const F32 span = (curve.at(1.0f) - curve.at(0.0f)).len() * repeats;
   const F32 step = 1.0f / F32(segments);

   PrimBuild::begin(GFXTriangleStrip, segments * 2);
   for(U32 i = 0; i < segments; i++)
   {
      const F32 t = F32(i) / F32(segments - 1);
      const Point3F p = curve.at(t);

         // The last point has nothing after it to look towards, so it takes
         // the direction it arrived from instead.
      VectorF along = i + 1 == segments
                         ? p - curve.at(F32(segments - 1) / F32(segments))
                         : curve.at(t + step) - p;
      if(along.isZero())
         continue;
      along.normalize();

         // Tribes 2 drops a point only when the step along the curve is
         // degenerate, never when the corner offset is (0x0044dfb2). Dropping
         // one pair out of a strip welds the strip across the gap, which puts
         // a triangle the width of the beam across the view.
      VectorF across = mCross(along, p - eye);
      if(across.isZero())
         across.set(0.0f, 0.0f, 0.0f);
      else
      {
         across.normalize();
         across *= halfWidth;
      }

      if(color)
         PrimBuild::color4f(color->red, color->green, color->blue,
                            i == 0 ? 0.0f : color->alpha);

      const F32 u = span * t + scroll;
      PrimBuild::texCoord2f(u, 0.0f); PrimBuild::vertex3fv(p + across);
      PrimBuild::texCoord2f(u, 1.0f); PrimBuild::vertex3fv(p - across);
   }
   PrimBuild::end();
}

/// A beam runs from its source's muzzle to whatever it has hold of, and stays
/// there while the trigger is down: the server sends it once with the two
/// objects and the client keeps both ends on them.
class RepairProjectileT2 : public GameBase
{
   typedef GameBase Parent;

protected:
      /// What a repair beam leaves behind it: a copy of the curve it was
      /// drawn along, which fades and widens where it stands until it has
      /// outlived the datablock's blurLifetime. Tribes 2 keeps room for five
      /// (0x00645c40 walks this+0x360 five times in steps of 0x58).
   enum { NumBlurs = 5 };

   struct Blur
   {
      F32 age;
      F32 life;
      F32 alpha;
      T2BeamCurve curve;
   };

   RepairProjectileDataT2 *mData;
   SimObjectId mSourceObject;
   U32         mSourceSlot;
   SimObjectId mTargetObject;
   F32         mElapsed;
   F32         mBlurTime;
   Point3F     mStart;
   VectorF     mAim;
   Point3F     mHit;
   Point3F     mEnd;
   bool        mEndPlaced;
   T2BeamCurve mCurve;
   bool        mHaveEnds;
   Blur        mBlur[NumBlurs];
   GFXStateBlockRef mBeamSB;
   GFXStateBlockRef mFlareSB;

   ShapeBase *resolve(SimObjectId id) const;
   void ageBlur(F32 dt);
   bool flareVisible(const Point3F &eye);
   void drawFlare(const Point3F &eye) const;
   void drawBlur(const Point3F &eye) const;
   virtual void renderBeam(ObjectRenderInst *ri, SceneRenderState *state,
                           BaseMatInstance *overrideMat);

public:
   RepairProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   virtual void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(RepairProjectileT2);
};

IMPLEMENT_CONOBJECT(RepairProjectileT2);

ConsoleDocClass( RepairProjectileT2,
   "@brief A Tribes 2 repair beam.\n\n@internal" );

RepairProjectileT2::RepairProjectileT2()
   : mData(NULL),
     mSourceObject(0),
     mSourceSlot(0),
     mTargetObject(0),
     mElapsed(0.0f),
     mBlurTime(0.0f),
     mStart(0.0f, 0.0f, 0.0f),
     mAim(0.0f, 1.0f, 0.0f),
     mHit(0.0f, 0.0f, 0.0f),
     mEnd(0.0f, 0.0f, 0.0f),
     mEndPlaced(false),
     mHaveEnds(false)
{
   mTypeMask |= ProjectileObjectType;

   for(U32 i = 0; i < NumBlurs; i++)
   {
      mBlur[i].age   = 0.0f;
      mBlur[i].life  = 0.0f;
      mBlur[i].alpha = 0.0f;
   }
}

bool RepairProjectileT2::onAdd()
{
   if(!Parent::onAdd())
      return false;

      // Both ends move with the objects they are on, so the beam is given
      // the same global box the other beams have rather than one that would
      // have to be chased every tick.
   setGlobalBounds();
   resetWorldBox();
   addToScene();
   return true;
}

void RepairProjectileT2::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

bool RepairProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<RepairProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   return true;
}

ShapeBase *RepairProjectileT2::resolve(SimObjectId id) const
{
   ShapeBase *obj = NULL;
   if(id)
      Sim::findObject(id, obj);
   return obj;
}

void RepairProjectileT2::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   NetConnection *conn = NetConnection::getConnectionToServer();

   if(body.hasSourceObject)
   {
      NetObject *obj = conn ? conn->resolveGhost(S32(body.sourceObject))
                            : NULL;
      mSourceObject = obj ? obj->getId() : 0;
      mSourceSlot = getMin(body.sourceSlot,
                           U32(ShapeBase::MaxMountedImages - 1));
   }

   if(body.hasBeamTarget)
   {
      NetObject *obj = conn ? conn->resolveGhost(S32(body.beamTarget))
                            : NULL;
      mTargetObject = obj ? obj->getId() : 0;
   }
}

/// RepairProjectile::advanceTime (0x006457d0). Both of a beam's ends are
/// objects, so it is placed from scratch every frame rather than flown.
void RepairProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(!mData || dt <= 0.0f)
      return;

      // Nothing is drawn at all without both ends: Tribes 2 leaves from here
      // before it has a muzzle to run from (0x006457ee).
   ShapeBase *src = resolve(mSourceObject);
   ShapeBase *dst = resolve(mTargetObject);
   mHaveEnds = src != NULL && dst != NULL;
   if(!mHaveEnds)
      return;

   mElapsed += dt;
   mBlurTime += dt;

   src->getRenderMuzzlePoint(mSourceSlot, &mStart);

      // Not the muzzle node's own forward. Tribes 2's getRenderMuzzleVector
      // bends it onto whatever is straight ahead of the firer's eye, and it
      // does so for a ghost as well as for the player this machine drives --
      // the connection it asks is the one the ghost came down, which every
      // client has. weapon_elf.dts's Muzzlepoint node faces up, so without
      // the bend the beam leaves the gun into the sky.
   MatrixF muzzle;
   src->getRenderMuzzleTransform(mSourceSlot, &muzzle);
   if(!src->getCorrectedAim(muzzle, &mAim))
      muzzle.getColumn(1, &mAim);

      // The far end is where the aim meets the thing being repaired, not that
      // thing's middle: the beam lands on the part of the shape being aimed
      // at. The cast is made with the target's own type mask (0x00645905), so
      // only something of its kind can be hit, and an answer that is not the
      // target is thrown away and the last point held.
   RayInfo info;
   if(getContainer()->castRay(mStart, mStart + mAim * mData->range,
                              dst->getTypeMask(), &info)
      && info.object == dst)
   {
      mHit = info.point;
      if(!mEndPlaced)
      {
            // The first one is taken as it stands, or the beam would reel in
            // from wherever the end happened to be.
         mEndPlaced = true;
         mEnd = mHit;
      }
   }

      // After that the drawn end chases the aimed-at point instead of jumping
      // to it, which is what slides the beam over a shape as the gun turns.
   mEnd += (mHit - mEnd) * dt * 2.0f;

   mCurve.set(mStart, mStart + mAim * (mEnd - mStart).len(), mEnd);

   ageBlur(dt);
}

/// RepairProjectile::updateBlur (0x00645c40), which only a ghost runs: age
/// every copy of the beam, and once a blurFreq of a second has gone by leave
/// another behind in the first slot that has expired.
void RepairProjectileT2::ageBlur(F32 dt)
{
   if(!isGhost())
      return;

   for(U32 i = 0; i < NumBlurs; i++)
      mBlur[i].age += dt;

   const F32 interval = 1.0f / mData->blurFreq;
   if(mBlurTime <= interval)
      return;
   mBlurTime -= interval;

   for(U32 i = 0; i < NumBlurs; i++)
      if(mBlur[i].age >= mBlur[i].life)
      {
         mBlur[i].age   = 0.0f;
         mBlur[i].life  = mData->blurLifetime;
         mBlur[i].alpha = 0.5f;
         mBlur[i].curve.set(mStart,
                            mStart + mAim * (mStart - mEnd).len(), mEnd);
         return;
      }
}

void RepairProjectileT2::prepRenderImage(SceneRenderState *state)
{
   if(!mData || !mHaveEnds || mData->texture[0].isNull())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &RepairProjectileT2::renderBeam);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// Whether the flare where the beam lands can be seen from the camera
/// (0x006465a0): not if the beam is coming back at the camera, and not if
/// anything but the two ends stands between the two.
bool RepairProjectileT2::flareVisible(const Point3F &eye)
{
   ShapeBase *src = resolve(mSourceObject);
   ShapeBase *dst = resolve(mTargetObject);
   if(!src || !dst)
      return false;

   VectorF along = mEnd - mStart;
   VectorF toEnd = mEnd - eye;
   if(along.isZero() || toEnd.isZero())
      return false;
   along.normalize();
   toEnd.normalize();

      // Looking into the beam rather than along it, within about forty
      // degrees of head on, and there is no flare (0x00646952).
   if(mDot(along, toEnd) < -0.75f)
      return false;

      // The firer is stepped over as well as the target unless some client is
      // watching it from outside its own head, which is the one case where
      // its body is between the camera and the beam (0x00646602).
   const GameConnection *conn = src->getControllingClient();
   const bool skipSource = conn == NULL || conn->isFirstPerson();

   dst->disableCollision();
   if(skipSource)
      src->disableCollision();

   RayInfo info;
   const bool blocked = getContainer()->castRay(eye + toEnd * 0.5f, mEnd,
                                                0xFFFFFFFF, &info);

   if(skipSource)
      src->enableCollision();
   dst->enableCollision();

   return !blocked;
}

/// RepairProjectile::renderFlare (0x00646a20): three billboards of the
/// beam's second texture where it lands, each its own size and turning at its
/// own rate, drawn through anything in front of them.
void RepairProjectileT2::drawFlare(const Point3F &eye) const
{
   static const F32 sSize[3] = { 2.00f, 1.70f, 1.82f };
   static const F32 sSpin[3] = { 0.15f, -0.10f, 0.05f };

   VectorF toEye = eye - mEnd;
   if(toEye.isZero())
      return;
   toEye.normalize();

   VectorF right = mCross(toEye, VectorF(0.0f, 0.0f, 1.0f));
   if(right.isZero())
      right = mCross(toEye, VectorF(0.0f, 1.0f, 0.0f));
   right.normalize();
   const VectorF up = mCross(right, toEye);

   GFX->setStateBlock(mFlareSB);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
   GFX->setTexture(0, mData->texture[1]);

      // A third of white, and the alpha the beam was drawn with left where it
      // was: the flare is put down with glColor3f (0x00646a96).
   PrimBuild::color4f(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f);
   for(U32 i = 0; i < 3; i++)
   {
         // Six tenths is what the renderer asks for, halved because the
         // billboard is drawn a size across rather than a size out.
      const F32 half = sSize[i] * 0.6f * 0.5f;
      const F32 a = mElapsed * sSpin[i] * F32(M_2PI);
      const VectorF x = (right * mCos(a) + up * mSin(a)) * half;
      const VectorF y = (up * mCos(a) - right * mSin(a)) * half;

      PrimBuild::begin(GFXTriangleStrip, 4);
      PrimBuild::texCoord2f(0.0f, 0.0f); PrimBuild::vertex3fv(mEnd - x + y);
      PrimBuild::texCoord2f(0.0f, 1.0f); PrimBuild::vertex3fv(mEnd - x - y);
      PrimBuild::texCoord2f(1.0f, 0.0f); PrimBuild::vertex3fv(mEnd + x + y);
      PrimBuild::texCoord2f(1.0f, 1.0f); PrimBuild::vertex3fv(mEnd + x - y);
      PrimBuild::end();
   }
}

/// Every copy of the beam that has not yet outlived its lifetime, drawn in
/// the red of the beam's own texture, widening from three tenths of a metre
/// to eight and fading out over the first half of its life (0x006464b7,
/// 0x006464a3).
void RepairProjectileT2::drawBlur(const Point3F &eye) const
{
   for(U32 i = 0; i < NumBlurs; i++)
   {
      if(mBlur[i].age >= mBlur[i].life)
         continue;

      const F32 t = mBlur[i].age / mBlur[i].life;

         // Past half its life the alpha Tribes 2 works out goes negative, and
         // glColor4f clamps it to nothing: the second half of a copy's life is
         // spent invisible. Our own colour is packed into a byte, where a
         // negative alpha comes back out near opaque, so the copy has to be
         // dropped here instead.
      const F32 alpha = (mBlur[i].alpha - t) * mBlur[i].alpha;
      if(alpha <= 0.0f)
         continue;

      const F32 width = t * 0.5f + 0.3f;
      const ColorF color(1.0f, 0.0f, 0.0f, alpha);

      drawT2BeamRibbon(mBlur[i].curve, mData->segments, width * 0.5f,
                       -mElapsed * mData->speed, mData->texRepeat, eye,
                       &color);
   }
}

/// RepairProjectile::renderObject (0x00645fc0): the flare where the beam
/// lands, the beam itself, and the copies it has left behind it.
void RepairProjectileT2::renderBeam(ObjectRenderInst *, SceneRenderState *state,
                                    BaseMatInstance *overrideMat)
{
   if(overrideMat || !mHaveEnds)
      return;

   VectorF along = mEnd - mStart;
   if(along.isZero())
      return;
   along.normalize();

      // The beam is not drawn at all once the gun has swung too far off what
      // it has hold of. Tribes 2 measures that in ninetieths rather than
      // taking an arc cosine, and compares the answer against the cutoffAngle
      // its datablock carries (0x00646114).
   if(90.0f * (1.0f - mDot(along, mAim)) > mData->cutoffAngle)
      return;

   if(mBeamSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mBeamSB = GFX->createStateBlock(desc);

         // The flare is drawn through whatever is in front of it
         // (0x00646a41).
      desc.zEnable = false;
      mFlareSB = GFX->createStateBlock(desc);
   }

   const Point3F eye = state->getCameraPosition();

   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

   if(mData->texture[1].isValid() && flareVisible(eye))
      drawFlare(eye);

   GFX->setStateBlock(mBeamSB);
   GFX->setTexture(0, mData->texture[0]);

      // Twenty segments, a tenth of a metre either side and three quarters
      // alpha: Tribes 2 fixes all three in the renderer rather than reading
      // the numSegments and beamWidth its datablock carries.
   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 0.75f);
   drawT2BeamRibbon(mCurve, 20, 0.1f, -mElapsed * mData->speed,
                    mData->texRepeat, eye);

   drawBlur(eye);
}

/// What an ELF beam's datablock says about how it is drawn: the beam's own
/// four words, two for the lightning that runs along it, and three textures.
class ELFProjectileDataT2 : public RepairProjectileDataT2
{
   typedef RepairProjectileDataT2 Parent;

public:
   F32 lightningWidth;
   F32 lightningDist;

   /// The cosine of the half-angle the gun will hold a target within.
   /// Tribes 2 calls the angle beamHitWidth, keeps it in degrees, clamps it
   /// to ninety and takes the cosine once in preload (0x0064aa14). It is not
   /// on the wire and no script sets it, so every beam uses the thirty
   /// degrees the constructor puts there.
   F32 hitAngleCos;

   ELFProjectileDataT2();
   bool unpackData(BitStream *stream, U32 classId);

   DECLARE_CONOBJECT(ELFProjectileDataT2);
};

IMPLEMENT_CONOBJECT(ELFProjectileDataT2);

ConsoleDocClass( ELFProjectileDataT2,
   "@brief What a Tribes 2 ELF beam's datablock says about how it is drawn."
   "\n\n@internal" );

ELFProjectileDataT2::ELFProjectileDataT2()
   : lightningWidth(0.1f),
     lightningDist(0.15f),
     hitAngleCos(mCos(mDegToRad(30.0f)))
{
   width = 0.1f;
   speed = 9.0f;
   texRepeat = 0.25f;
   range = 38.0f;
}

bool ELFProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElfBeamWidth, &value))
      width = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElfBeamSpeed, &value))
      speed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElfBeamRepeat, &value))
      texRepeat = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElfLightningWidth, &value))
      lightningWidth = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElfLightningDist, &value))
      lightningDist = value;

   return true;
}

/// An ELF beam is the same curve with three ropes of lightning threaded along
/// it and a flash where it lands.
///
/// The lightning is not simulated. Each rope is sixteen points; every point
/// but the two ends carries one random direction of length lightningDist,
/// drawn once and then redrawn thirty times a second, and the rope is just
/// the curve sampled at those sixteen places with the offsets added. Each
/// span of the rope wears one whole copy of the lightning texture, laid down
/// alternately forwards and backwards, which is what makes it read as a chain
/// of separate arcs rather than one stretched streak.
class ELFProjectileT2 : public RepairProjectileT2
{
   typedef RepairProjectileT2 Parent;

   enum {
      NumBolts  = 3,
      NumPoints = 16
   };

   /// Tribes 2 rolls the lightning again every tick.
   static const F32 sRollPeriod;

   ELFProjectileDataT2 *mElf;
   Point3F mOffset[NumBolts][NumPoints];
   F32     mSinceRoll;
   bool    mRoll;
   bool    mBlocked;
   GFXStateBlockRef mBoltSB;

   void clipToTarget();
   void rollLightning();
   void drawLightning(U32 bolt, const Point3F &eye) const;
   void drawFlash(const Point3F &eye) const;

public:
   ELFProjectileT2();

   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void advanceTime(F32 dt);
   void renderBeam(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);

   DECLARE_CONOBJECT(ELFProjectileT2);
};

IMPLEMENT_CONOBJECT(ELFProjectileT2);

ConsoleDocClass( ELFProjectileT2,
   "@brief A Tribes 2 ELF beam.\n\n@internal" );

const F32 ELFProjectileT2::sRollPeriod = 1.0f / 30.0f;

ELFProjectileT2::ELFProjectileT2()
   : mElf(NULL),
     mSinceRoll(0.0f),
     mRoll(true),
     mBlocked(false)
{
   for(U32 b = 0; b < NumBolts; b++)
      for(U32 i = 0; i < NumPoints; i++)
         mOffset[b][i].set(0.0f, 0.0f, 0.0f);
}

bool ELFProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   if(!Parent::onNewDataBlock(dptr, reload))
      return false;

   mElf = dynamic_cast<ELFProjectileDataT2 *>(dptr);
   return mElf != NULL;
}

void ELFProjectileT2::rollLightning()
{
   for(U32 b = 0; b < NumBolts; b++)
      for(U32 i = 0; i < NumPoints; i++)
      {
            // The ends are pinned to the beam; everything between gets one
            // direction, at the distance the datablock allows.
         if(i == 0 || i + 1 == NumPoints)
         {
            mOffset[b][i].set(0.0f, 0.0f, 0.0f);
            continue;
         }

         VectorF v(gRandGen.randF(-1.0f, 1.0f),
                   gRandGen.randF(-1.0f, 1.0f),
                   gRandGen.randF(-1.0f, 1.0f));
         const F32 len = v.len();
         if(len > 0.01f)
            v /= len;
         mOffset[b][i] = v * mElf->lightningDist;
      }
}

/// Walk the curve in sixteen steps looking for the first thing in the way. If
/// that is what the beam has hold of, the far end moves onto it; if it is
/// anything else the link is broken and nothing is drawn until it clears.
/// Tribes 2 does this in its render, once a frame, at 0x0064c650.
void ELFProjectileT2::clipToTarget()
{
   ShapeBase *src = resolve(mSourceObject);
   if(src == NULL)
      return;

   const SceneObject *target = resolve(mTargetObject);

      // A target that has walked out of range, or out of the cone the gun
      // holds one in, breaks the link as surely as one that has stepped
      // behind a wall. Tribes 2 makes both tests while it works out what the
      // gun is holding -- the range against beamRange (0x0064bee7) and the
      // angle as a dot product against the cosine it prepared (0x0064bfc6) --
      // and it makes them only on the server. The server leaves the beam
      // object alone afterwards, so a client that does not make the same
      // tests keeps drawing to a target it can no longer reach.
   VectorF toTarget = mEnd - mStart;
   const F32 reach = toTarget.len();
   if(reach > mData->range)
   {
      mBlocked = true;
      return;
   }

   if(reach > 0.0f)
   {
      toTarget /= reach;
      if(mDot(toTarget, mAim) < mElf->hitAngleCos)
      {
         mBlocked = true;
         return;
      }
   }

   src->disableCollision();

   Point3F from = mCurve.at(0.0f);
   RayInfo info;
   bool hit = false;
   for(U32 i = 0; i < 16 && !hit; i++)
   {
      const Point3F to = mCurve.at(F32(i + 1) / 16.0f);
      hit = getContainer()->castRay(from, to, sProjectileCollisionMask, &info);
      from = to;
   }

   src->enableCollision();

   mBlocked = hit && info.object != target;
   if(hit && !mBlocked)
   {
      mEnd = info.point;
      mCurve.set(mStart, mStart + mAim * (mEnd - mStart).len(), mEnd);
   }
}

void ELFProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(!mElf || dt <= 0.0f)
      return;

   if(mHaveEnds)
      clipToTarget();

   mSinceRoll += dt;
   while(mSinceRoll >= sRollPeriod)
   {
      mSinceRoll -= sRollPeriod;
      mRoll = true;
   }
}

void ELFProjectileT2::drawLightning(U32 bolt, const Point3F &eye) const
{
   Point3F pt[NumPoints];
   for(U32 i = 0; i < NumPoints; i++)
      pt[i] = mCurve.at(F32(i) / F32(NumPoints - 1)) + mOffset[bolt][i];

   PrimBuild::begin(GFXTriangleStrip, NumPoints * 2);
   for(U32 i = 0; i < NumPoints; i++)
   {
      VectorF along = i + 1 == NumPoints ? pt[i] - pt[i - 1]
                                         : pt[i + 1] - pt[i];
      if(along.isZero())
         continue;
      along.normalize();

      VectorF across = mCross(along, pt[i] - eye);
      if(across.isZero())
         continue;
      across.normalize();
      across *= mElf->lightningWidth * 0.5f;

      const F32 u = F32(i & 1);
      PrimBuild::texCoord2f(u, 1.0f); PrimBuild::vertex3fv(pt[i] - across);
      PrimBuild::texCoord2f(u, 0.0f); PrimBuild::vertex3fv(pt[i] + across);
   }
   PrimBuild::end();
}

/// Where the beam lands: three sprites of the same texture over one another,
/// each a different size and each turning at its own rate, two one way and
/// one the other.
void ELFProjectileT2::drawFlash(const Point3F &eye) const
{
   static const F32 sSize[3] = { 2.00f, 1.70f, 1.82f };
   static const F32 sSpin[3] = { 0.30f, -0.20f, 0.10f };

   VectorF toEye = eye - mEnd;
   if(toEye.isZero())
      return;
   toEye.normalize();

   VectorF right = mCross(toEye, VectorF(0.0f, 0.0f, 1.0f));
   if(right.isZero())
      right = mCross(toEye, VectorF(0.0f, 1.0f, 0.0f));
   right.normalize();
   const VectorF up = mCross(right, toEye);

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f / 3.0f);
   for(U32 i = 0; i < 3; i++)
   {
      const F32 half = sSize[i] * 0.5f * 0.5f;
      const F32 a = mElapsed * sSpin[i] * F32(M_2PI);
      const VectorF x = (right * mCos(a) + up * mSin(a)) * half;
      const VectorF y = (up * mCos(a) - right * mSin(a)) * half;

      PrimBuild::begin(GFXTriangleStrip, 4);
      PrimBuild::texCoord2f(0.0f, 0.0f); PrimBuild::vertex3fv(mEnd - x + y);
      PrimBuild::texCoord2f(0.0f, 1.0f); PrimBuild::vertex3fv(mEnd - x - y);
      PrimBuild::texCoord2f(1.0f, 0.0f); PrimBuild::vertex3fv(mEnd + x + y);
      PrimBuild::texCoord2f(1.0f, 1.0f); PrimBuild::vertex3fv(mEnd + x - y);
      PrimBuild::end();
   }
}

void ELFProjectileT2::renderBeam(ObjectRenderInst *, SceneRenderState *state,
                                 BaseMatInstance *overrideMat)
{
   if(overrideMat || !mHaveEnds || !mElf || mBlocked)
      return;

   if((mEnd - mStart).isZero())
      return;

   if(mBoltSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mBoltSB = GFX->createStateBlock(desc);
   }

   const Point3F eye = state->getCameraPosition();

   GFX->setStateBlock(mBoltSB);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

   if(mElf->texture[0].isValid())
   {
      GFX->setTexture(0, mElf->texture[0]);
      PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
      drawT2BeamRibbon(mCurve, 16, mElf->width, -mElapsed * mElf->speed,
                       mElf->texRepeat, eye);
   }

   if(mElf->texture[1].isValid())
   {
      if(mRoll)
      {
         const_cast<ELFProjectileT2 *>(this)->rollLightning();
         const_cast<ELFProjectileT2 *>(this)->mRoll = false;
      }

      GFX->setTexture(0, mElf->texture[1]);
      PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
      for(U32 b = 0; b < NumBolts; b++)
         drawLightning(b, eye);
   }

   if(mElf->texture[2].isValid())
   {
      GFX->setTexture(0, mElf->texture[2]);
      drawFlash(eye);
   }
}

/// What a shock lance's zap looks like. Seven words of its own, then four at
/// a time for each of the two bolts, and four textures. Tribes 2 writes the
/// array fields a bolt at a time rather than a field at a time, which the
/// field list does not say and only the values do: startWidth, endWidth,
/// boltSpeed and texWrap for bolt nought, then the same four for bolt one.
class ShockLanceProjectileDataT2 : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum { NumBolts = 2 };

   F32    zapDuration;
   F32    boltLength;
   U32    parts;
   F32    lightningFreq;
   F32    lightningDensity;
   F32    lightningAmp;
   F32    lightningWidth;
   F32    startWidth[NumBolts];
   F32    endWidth[NumBolts];
   F32    boltSpeed[NumBolts];
   F32    texWrap[NumBolts];
   String textureName[4];
   GFXTexHandle texture[4];

   ShockLanceProjectileDataT2();
   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(ShockLanceProjectileDataT2);
};

IMPLEMENT_CONOBJECT(ShockLanceProjectileDataT2);

ConsoleDocClass( ShockLanceProjectileDataT2,
   "@brief What a Tribes 2 shock lance's zap says about how it is drawn."
   "\n\n@internal" );

ShockLanceProjectileDataT2::ShockLanceProjectileDataT2()
   : zapDuration(1.0f),
     boltLength(14.0f),
     parts(25),
     lightningFreq(25.0f),
     lightningDensity(3.0f),
     lightningAmp(0.25f),
     lightningWidth(0.05f)
{
   for(U32 i = 0; i < NumBolts; i++)
   {
      startWidth[i] = 0.3f;
      endWidth[i] = 0.6f;
      boltSpeed[i] = i ? -0.5f : 2.0f;
      texWrap[i] = 1.5f;
   }
}

bool ShockLanceProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbZapDuration, &value))
      zapDuration = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltLength, &value))
      boltLength = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbBoltParts, &word))
      parts = word ? getMin(word, U32(64)) : 1;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbShockLightningFreq, &value))
      lightningFreq = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbShockLightningDensity, &value))
      lightningDensity = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbShockLightningAmp, &value))
      lightningAmp = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbShockLightningWidth, &value))
      lightningWidth = value;

   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltStartWidth0, &value))
      startWidth[0] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltEndWidth0, &value))
      endWidth[0] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltSpeed0, &value))
      boltSpeed[0] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltTexWrap0, &value))
      texWrap[0] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltStartWidth1, &value))
      startWidth[1] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltEndWidth1, &value))
      endWidth[1] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltSpeed1, &value))
      boltSpeed[1] = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbBoltTexWrap1, &value))
      texWrap[1] = value;

      // The shape name comes first, as it does for every projectile.
   const Vector<String> &strings = T2Layout::lastStrings();
   for(U32 i = 0; i < 4 && i + 1 < strings.size(); i++)
      textureName[i] = strings[i + 1];

   return true;
}

bool ShockLanceProjectileDataT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(!server)
      for(U32 i = 0; i < 4; i++)
         if(textureName[i].isNotEmpty())
            texture[i].set(String("textures/") + textureName[i],
                           &GFXDefaultStaticDiffuseProfile,
                           "ShockLanceProjectileT2");

   return true;
}

/// A shock lance's zap. Unlike the repair and ELF beams this one carries both
/// of its ends on the wire, so there is nothing to cast for: it is drawn
/// where the server put it and goes away after zapDuration. A dry fire is the
/// exception -- see placeDryFire.
class ShockLanceProjectileT2 : public GameBase
{
   typedef GameBase Parent;

   enum { NumRopes = 2, MaxRopePoints = 50 };

   ShockLanceProjectileDataT2 *mData;
   SimObjectId mSourceObject;
   U32     mSourceSlot;
   Point3F mStart;
   Point3F mEnd;
   F32     mElapsed;
   F32     mSinceRoll;
   bool    mHaveEnds;
   bool    mHit;
      /// In the zap's own frame, not the world: a rope is rolled once every
      /// lightningFreq and has to ride the muzzle until the next one.
   Point3F mRope[NumRopes][MaxRopePoints];
   U32     mRopePoints;
   GFXStateBlockRef mBoltSB;

   ShapeBase *resolve(SimObjectId id) const;
   void placeDryFire();
   void rollLightning();
   void drawBolt(U32 bolt, F32 widthScale, const Point3F &eye) const;
   void drawFilaments(const Point3F &eye) const;
   void renderZap(ObjectRenderInst *ri, SceneRenderState *state,
                  BaseMatInstance *overrideMat);

public:
   ShockLanceProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

   DECLARE_CONOBJECT(ShockLanceProjectileT2);
};

IMPLEMENT_CONOBJECT(ShockLanceProjectileT2);

ConsoleDocClass( ShockLanceProjectileT2,
   "@brief A Tribes 2 shock lance's zap.\n\n@internal" );

ShockLanceProjectileT2::ShockLanceProjectileT2()
   : mData(NULL),
     mSourceObject(0),
     mSourceSlot(0),
     mStart(0.0f, 0.0f, 0.0f),
     mEnd(0.0f, 0.0f, 0.0f),
     mElapsed(0.0f),
     mSinceRoll(0.0f),
     mHaveEnds(false),
     mHit(false),
     mRopePoints(0)
{
   mTypeMask |= ProjectileObjectType;
}

bool ShockLanceProjectileT2::onAdd()
{
   if(!Parent::onAdd())
      return false;

   setGlobalBounds();
   resetWorldBox();
   addToScene();
   return true;
}

void ShockLanceProjectileT2::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

bool ShockLanceProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<ShockLanceProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   rollLightning();
   return true;
}

void ShockLanceProjectileT2::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   if(body.hasZap)
   {
      mStart = body.zapStart;
      mEnd = body.zapEnd;
      mHaveEnds = true;
      mHit = body.zapHit;
   }

   if(body.hasSourceObject)
   {
      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(S32(body.sourceObject)) : NULL;
      mSourceObject = obj ? obj->getId() : 0;
      mSourceSlot = getMin(body.sourceSlot,
                           U32(ShapeBase::MaxMountedImages - 1));
   }
}

ShapeBase *ShockLanceProjectileT2::resolve(SimObjectId id) const
{
   ShapeBase *obj = NULL;
   if(id)
      Sim::findObject(id, obj);
   return obj;
}

/// A lance that reached nothing throws its two wire ends away and is drawn as a
/// stub off the muzzle instead, remade every frame so it tracks the aim
/// (0x0064f960, called from renderObject before anything is drawn). The 0.2 is
/// retail's literal, not its boltLength.
void ShockLanceProjectileT2::placeDryFire()
{
   ShapeBase *src = resolve(mSourceObject);
   if(!src)
      return;

   src->getRenderMuzzlePoint(mSourceSlot, &mStart);

   VectorF aim;
   MatrixF muzzle;
   src->getRenderMuzzleTransform(mSourceSlot, &muzzle);
   if(!src->getCorrectedAim(muzzle, &aim))
      muzzle.getColumn(1, &aim);

   mEnd = mStart + aim * 0.2f;
}

/// Both ropes are rolled afresh lightningFreq times a second (0x00650c20).
/// lightningDensity is points per metre, not a count, so a short zap gets a
/// short rope -- and a dry fire is given twenty a metre at an amplitude of
/// 0.1 whatever the datablock says (0x0064f8f3).
void ShockLanceProjectileT2::rollLightning()
{
   F32 density = mData->lightningDensity;
   F32 amp = mData->lightningAmp;
   if(!mHit)
   {
      density = 20.0f;
      amp = 0.1f;
   }

   const F32 len = (mEnd - mStart).len();
   const S32 points = S32(density * len);
   mRopePoints = U32(getMax(getMin(points, S32(MaxRopePoints)), 0));
   if(!mRopePoints)
      return;

   const F32 step = len / F32(mRopePoints);

      // The offset is a whole direction, so a point wanders along the zap as
      // well as across it; the ends are not pinned.
   for(U32 r = 0; r < NumRopes; r++)
      for(U32 i = 0; i < mRopePoints; i++)
      {
         VectorF off(gRandGen.randF(-1.0f, 1.0f),
                     gRandGen.randF(-1.0f, 1.0f),
                     gRandGen.randF(-1.0f, 1.0f));
         if(off.lenSquared() > 0.0001f)
            off.normalize();
         else
            off.set(0.0f, 0.0f, 0.0f);

         mRope[r][i].set(off.x * amp, F32(i) * step + off.y * amp,
                         off.z * amp);
      }
}

void ShockLanceProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(!mData || dt <= 0.0f)
      return;

   mElapsed += dt;

   const F32 period = mData->lightningFreq > 0.0f
                         ? 1.0f / mData->lightningFreq : 1.0f;
   if((mSinceRoll += dt) >= period)
   {
      mSinceRoll -= period;
      if(!mHit)
         placeDryFire();
      rollLightning();
   }
}

void ShockLanceProjectileT2::prepRenderImage(SceneRenderState *state)
{
   if(!mData || !mHaveEnds || mElapsed > mData->zapDuration)
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &ShockLanceProjectileT2::renderZap);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// One bolt: a strip of quads that widens from the muzzle to the far end, with
/// its texture running along it and scrolling at its own speed. The two bolts
/// scroll opposite ways, which is what gives the zap its churn.
void ShockLanceProjectileT2::drawBolt(U32 bolt, F32 widthScale,
                                      const Point3F &eye) const
{
   const U32 parts = mData->parts;
   const F32 repeats = mData->texWrap[bolt] > 0.0f ? mData->texWrap[bolt] : 1.0f;
   const F32 scroll = mElapsed * mData->boltSpeed[bolt];

   PrimBuild::begin(GFXTriangleList, parts * 6);
   for(U32 i = 0; i < parts; i++)
   {
      const F32 t0 = F32(i) / F32(parts);
      const F32 t1 = F32(i + 1) / F32(parts);
      const Point3F a = mStart + (mEnd - mStart) * t0;
      const Point3F b = mStart + (mEnd - mStart) * t1;

      VectorF seg = b - a;
      VectorF across = mCross(seg, a - eye);
      if(across.isZero())
         continue;
      across.normalize();

      const F32 w0 = mLerp(mData->startWidth[bolt],
                           mData->endWidth[bolt], t0) * widthScale;
      const F32 w1 = mLerp(mData->startWidth[bolt],
                           mData->endWidth[bolt], t1) * widthScale;
      const VectorF a0 = across * w0 * 0.5f;
      const VectorF a1 = across * w1 * 0.5f;
      const F32 u0 = scroll + t0 * repeats;
      const F32 u1 = scroll + t1 * repeats;

      PrimBuild::texCoord2f(u0, 0.0f); PrimBuild::vertex3fv(a + a0);
      PrimBuild::texCoord2f(u0, 1.0f); PrimBuild::vertex3fv(a - a0);
      PrimBuild::texCoord2f(u1, 1.0f); PrimBuild::vertex3fv(b - a1);

      PrimBuild::texCoord2f(u0, 0.0f); PrimBuild::vertex3fv(a + a0);
      PrimBuild::texCoord2f(u1, 1.0f); PrimBuild::vertex3fv(b - a1);
      PrimBuild::texCoord2f(u1, 0.0f); PrimBuild::vertex3fv(b + a1);
   }
   PrimBuild::end();
}

/// The two ropes, each a strip of width lightningWidth through the points
/// rolled for it. Every span wears one whole copy of the texture, laid
/// alternately forwards and backwards (0x006512cd) -- that is what makes the
/// painted bolt read as a chain of arcs instead of one stretched streak.
void ShockLanceProjectileT2::drawFilaments(const Point3F &eye) const
{
   if(mRopePoints < 2)
      return;

   VectorF along = mEnd - mStart;
   if(along.isZero())
      return;
   along.normalize();

   VectorF right = mCross(along, VectorF(0.0f, 0.0f, 1.0f));
   if(right.isZero())
      right = mCross(along, VectorF(0.0f, 1.0f, 0.0f));
   right.normalize();
   const VectorF up = mCross(right, along);

   const F32 half = mData->lightningWidth * 0.5f;

   for(U32 r = 0; r < NumRopes; r++)
   {
      PrimBuild::begin(GFXTriangleList, (mRopePoints - 1) * 6);
      for(U32 i = 0; i + 1 < mRopePoints; i++)
      {
         Point3F pt[2];
         for(U32 k = 0; k < 2; k++)
         {
            const Point3F &p = mRope[r][i + k];
            pt[k] = mStart + right * p.x + along * p.y + up * p.z;
         }

         VectorF across = mCross(pt[1] - pt[0], pt[0] - eye);
         if(across.isZero())
            continue;
         across.normalize();
         across *= half;

         const F32 u0 = F32(i & 1);
         const F32 u1 = F32((i + 1) & 1);

         PrimBuild::texCoord2f(u0, 0.0f); PrimBuild::vertex3fv(pt[0] + across);
         PrimBuild::texCoord2f(u0, 1.0f); PrimBuild::vertex3fv(pt[0] - across);
         PrimBuild::texCoord2f(u1, 1.0f); PrimBuild::vertex3fv(pt[1] - across);

         PrimBuild::texCoord2f(u0, 0.0f); PrimBuild::vertex3fv(pt[0] + across);
         PrimBuild::texCoord2f(u1, 1.0f); PrimBuild::vertex3fv(pt[1] - across);
         PrimBuild::texCoord2f(u1, 0.0f); PrimBuild::vertex3fv(pt[1] + across);
      }
      PrimBuild::end();
   }
}

void ShockLanceProjectileT2::renderZap(ObjectRenderInst *, SceneRenderState *state,
                                       BaseMatInstance *overrideMat)
{
   if(overrideMat || !mHaveEnds || !mData)
      return;

   if(!mHit)
      placeDryFire();

   if((mEnd - mStart).isZero())
      return;

   if(mBoltSB.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mBoltSB = GFX->createStateBlock(desc);
   }

   const Point3F eye = state->getCameraPosition();

   GFX->setStateBlock(mBoltSB);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

      // The whole zap fades out over its life (0x0064fe44, 0x00650478).
   const F32 fade = mData->zapDuration > 0.0f
                       ? 1.0f - mElapsed / mData->zapDuration : 1.0f;
   PrimBuild::color4f(1.0f, 1.0f, 1.0f, getMax(fade, 0.0f));

      // Bolts and ropes alike are drawn with the fourth texture and only the
      // fourth -- special/ELFBeam, not one of the three shockLightning
      // bitmaps, which are loaded and never bound (0x0064e596 puts the
      // handles at +0x194, and both render paths read +0x1a0). The jag is in
      // the rope, not the artwork.
   GFX->setTexture(0, mData->texture[3]);

      // A dry fire draws no bolt at all (0x0064fd90), only the ropes, and by
      // here those run over the stub placeDryFire put at the muzzle.
   if(mHit)
      for(U32 i = 0; i < ShockLanceProjectileDataT2::NumBolts; i++)
         drawBolt(i, 1.0f, eye);

   drawFilaments(eye);
}

#define IMPLEMENT_BEAM_PAIR_T2( objectName, dataName, dataBase, objectBase ) \
   class dataName : public dataBase                                          \
   {                                                                         \
      typedef dataBase Parent;                                               \
   public:                                                                   \
      DECLARE_CONOBJECT(dataName);                                           \
   };                                                                        \
   IMPLEMENT_CO_DATABLOCK_V1(dataName);                                      \
   ConsoleDocClass( dataName, "@brief A Tribes 2 beam's datablock."          \
                              "\n\n@internal" );                             \
   class objectName : public objectBase                                      \
   {                                                                         \
      typedef objectBase Parent;                                             \
   public:                                                                   \
      DECLARE_CONOBJECT(objectName);                                         \
   };                                                                        \
   IMPLEMENT_CO_NETOBJECT_V1(objectName);                                    \
   ConsoleDocClass( objectName, "@brief A Tribes 2 beam.\n\n@internal" )

IMPLEMENT_BEAM_PAIR_T2( RepairProjectile, RepairProjectileData,
                        RepairProjectileDataT2, RepairProjectileT2 );
IMPLEMENT_BEAM_PAIR_T2( ELFProjectile, ELFProjectileData,
                        ELFProjectileDataT2, ELFProjectileT2 );
IMPLEMENT_BEAM_PAIR_T2( ShockLanceProjectile, ShockLanceProjectileData,
                        ShockLanceProjectileDataT2, ShockLanceProjectileT2 );

//----------------------------------------------------------------------------

/// What a seeking missile's datablock says about its flight, on top of the run
/// every projectile shares. turningSpeed and terrainAvoidanceSpeed are degrees
/// a second; proximityRadius has no addField, so the ten metres
/// SeekerProjectileData's constructor leaves is the only value the wire ever
/// carries (0x00636fdd).
class SeekerProjectileDataT2 : public ProjectileDataT2
{
   typedef ProjectileDataT2 Parent;

public:
   F32  muzzleVelocity;    ///< how fast it leaves, before its motor lights
   F32  turningSpeed;      ///< degrees a second it can turn its nose
   F32  proximityRadius;   ///< how near the target it goes off once past it
   F32  avoidanceSpeed;    ///< degrees a second it can pull up by
   F32  scanAhead;         ///< how far ahead it looks for ground
   F32  heightFail;        ///< how far below itself that look reaches
   F32  avoidanceRadius;   ///< inside this it only pulls up for ground that
                           ///< stands between it and its target
   F32  flareDistance;     ///< how far off a flare can take it
   F32  flareAngle;
   bool useFlechette;      ///< it drops a casing when its motor lights
   F32  maxVelocity;       ///< what the motor takes it to
   F32  acceleration;      ///< metres a second a second
   U32  flechetteDelayMS;  ///< how long the motor takes to light
   U32  exhaustTimeMS;     ///< how long the launcher blows back for
   StringTableEntry exhaustNodeName;  ///< the launcher's node it comes out of
   StringTableEntry casingShapeName;  ///< the missile with its casing still on
   Resource<TSShape> mCasingShape;
   U32  puffEmitterId;     ///< the puff the casing comes off in
   U32  exhaustEmitterId;
   U32  casingDebrisId;

   SeekerProjectileDataT2();

   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   DECLARE_CONOBJECT(SeekerProjectileDataT2);
};

IMPLEMENT_CONOBJECT(SeekerProjectileDataT2);

ConsoleDocClass( SeekerProjectileDataT2,
   "@brief What a Tribes 2 seeking missile's datablock says about its "
   "flight.\n\n@internal" );

SeekerProjectileDataT2::SeekerProjectileDataT2()
   : muzzleVelocity(100.0f),
     turningSpeed(180.0f),
     proximityRadius(10.0f),
     avoidanceSpeed(0.0f),
     scanAhead(0.0f),
     heightFail(0.0f),
     avoidanceRadius(0.0f),
     flareDistance(0.0f),
     flareAngle(0.0f),
     useFlechette(false),
     maxVelocity(65.0f),
     acceleration(10.0f),
     flechetteDelayMS(500),
     exhaustTimeMS(0),
     exhaustNodeName(StringTable->EmptyString()),
     casingShapeName(StringTable->EmptyString()),
     puffEmitterId(0),
     exhaustEmitterId(0),
     casingDebrisId(0)
{
}

// The casing's shape is loaded beside the projectile's, and the missile is
// drawn with it until the casing comes off (0x006379de, 0x0063cf9d).
bool SeekerProjectileDataT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(casingShapeName && casingShapeName[0])
      mCasingShape = ResourceManager::get().load(
         T2Layout::resourcePath(casingShapeName, "shapes/"));

   return true;
}

bool SeekerProjectileDataT2::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerMuzzleVelocity, &value))
      muzzleVelocity = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerTurningSpeed, &value))
      turningSpeed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerProximityRadius, &value))
      proximityRadius = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerAvoidanceSpeed, &value))
      avoidanceSpeed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerScanAhead, &value))
      scanAhead = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerHeightFail, &value))
      heightFail = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerAvoidanceRadius, &value))
      avoidanceRadius = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerFlareDistance, &value))
      flareDistance = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerFlareAngle, &value))
      flareAngle = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerUseFlechette, &word))
      useFlechette = word != 0;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerMaxVelocity, &value))
      maxVelocity = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbSeekerAcceleration, &value))
      acceleration = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerFlechetteDelay, &word))
      flechetteDelayMS = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerExhaustTime, &word))
      exhaustTimeMS = word;
   if(const char *name = T2Layout::lastDataBlockString(T2Layout::T2DbSeekerExhaustNode))
      exhaustNodeName = StringTable->insert(name);
   if(const char *name = T2Layout::lastDataBlockString(T2Layout::T2DbSeekerCasingShape))
      casingShapeName = StringTable->insert(name);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerPuffEmitter, &word))
      puffEmitterId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerExhaustEmitter, &word))
      exhaustEmitterId = word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbSeekerCasingDebris, &word))
      casingDebrisId = word;

   return true;
}

//----------------------------------------------------------------------------

/// The seconds a tick is worth to a missile. The engine's general physics
/// constant is a thirty-second (0x0079c140), but projSeeker.cc carries its own
/// 0.032 (0x007adf30) and uses it for every step and every push of the motor
/// -- so a missile is 2.4% quicker than the rest of the engine would make it.
/// Measured against the retail server: a shoulder missile holds 93.2 metres a
/// second where a thirty-second tick holds 91.2.
static const F32 sSeekerTickSec = 0.032f;

/// Turn `way` towards `want` by at most `limit` radians, and answer whether
/// `want` lies ahead of it at all -- which is what tells a missile it has gone
/// past what it was chasing (0x006399cc). Retail builds the same turn out of
/// an axis and an angle it hands to AngAxisF; here it is the rotation in the
/// plane the two ways span, which needs no axis and so cannot take its sign
/// the wrong way round.
static bool turnTowards(VectorF *way, const VectorF &want, F32 limit)
{
   const F32 speed = way->len();
   if(speed < 1e-4f || want.isZero())
      return true;

   VectorF from = *way / speed;
   VectorF to = want;
   to.normalize();

   const F32 dot = mClampF(mDot(to, from), -1.0f, 1.0f);

      // Within a hundredth of a degree it is already pointing there, which is
      // the cosine retail compares against (0x00639a0c).
   if(dot >= 0.99985f)
      return true;

   const F32 turn = getMin(mAcos(dot), limit);
   if(turn <= 0.0f)
      return dot >= 0.0f;

   VectorF across = to - from * dot;
   if(across.isZero())
      return dot >= 0.0f;
   across.normalize();

   *way = (from * mCos(turn) + across * mSin(turn)) * speed;
   return dot >= 0.0f;
}

/// A Tribes 2 seeking missile. The server flies it and sends where it has got
/// to and what it is chasing; between those bodies the client flies the same
/// simulation, which is what gives the flight its curve rather than a line
/// from one body to the next (SeekerProjectile::processTick 0x0063a430, the
/// ghost half from 0x0063ae37).
class SeekerProjectileT2 : public GameBase, public ISceneLight
{
   typedef GameBase Parent;

   /// What it is chasing, which the body names on any update: a ghost, a
   /// point in the world, or nothing at all (0x00638eb0).
   enum Chasing
   {
      ChasingNothing,
      ChasingPoint,
      ChasingObject
   };

   SeekerProjectileDataT2 *mData;
   TSShapeInstance *mShapeInstance;
   TSShapeInstance *mCasingInstance;   ///< drawn in its place until the
                                       ///< casing comes off

   Point3F mPosition;
   VectorF mVelocity;
   VectorF mInherit;          ///< the share of its source's velocity it left
                              ///< with, which the motor works on top of
   VectorF mLaunchDir;        ///< the way it is drawn along until its motor
                              ///< lights, which is the way it was fired

   U32         mChasing;
   SimObjectId mTargetId;
   Point3F     mTargetPoint;  ///< where the target was, which is what a lost
                              ///< lock leaves behind (0x00638ed7)

   bool mCoasting;            ///< its motor has not lit yet
   bool mPuffed;              ///< and the puff it lights in has been laid
   F32  mAge;                 ///< seconds since this ghost arrived
   F32  mThrustTime;          ///< flight the motor has not yet been given
   bool mFlying;
   bool mExploded;
   VectorF mImpactNormal;

   SFXSource *mSound;
   SimObjectPtr<ParticleEmitter> mEmitter;
   SimObjectPtr<ParticleEmitter> mBubbleEmitter;
   SimObjectPtr<ParticleEmitter> mPuffEmitter;
   SimObjectPtr<ParticleEmitter> mExhaustEmitter;
   Point3F mEmitAt;
   F32     mEmitMS;
   F32     mExhaustMS;        ///< the launcher's blow-back a frame has left
                              ///< over, kept the way the smoke's is

   SimObjectId mSourceObject;
   U32         mSourceSlot;   ///< the image slot it was fired from, which is
                              ///< where the blow-back comes out
   F32         mNoSourceHit;

   LightInfo *mLight;

   ParticleEmitter *createEmitter(U32 dataId);
   bool motorLit() const;
   bool casingDropped() const;

   /// Throw the casing's four pieces off at `at`.
   void dropCasing(const Point3F &at);

   /// Blow the launcher's exhaust out of its muzzle for `dt`.
   void blowExhaust(F32 dt);

   bool targetPosition(Point3F *out);
   void seek(F32 dt);
   void avoidTerrain(const Point3F &target, F32 dt);
   void step(F32 dt);
   void explode(const Point3F &at, const VectorF &normal);

public:
   SeekerProjectileT2();
   ~SeekerProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

   void submitLights(LightManager *lm, bool staticLighting);
   LightInfo *getLight() { return mLight; }

   DECLARE_CONOBJECT(SeekerProjectileT2);
};

IMPLEMENT_CONOBJECT(SeekerProjectileT2);

ConsoleDocClass( SeekerProjectileT2,
   "@brief A Tribes 2 seeking missile.\n\n@internal" );

SeekerProjectileT2::SeekerProjectileT2()
   : mData(NULL),
     mShapeInstance(NULL),
     mCasingInstance(NULL),
     mPosition(0.0f, 0.0f, 0.0f),
     mVelocity(0.0f, 0.0f, 0.0f),
     mInherit(0.0f, 0.0f, 0.0f),
     mLaunchDir(0.0f, 1.0f, 0.0f),
     mChasing(ChasingNothing),
     mTargetId(0),
     mTargetPoint(0.0f, 0.0f, 0.0f),
     mCoasting(false),
     mPuffed(false),
     mAge(0.0f),
     mThrustTime(0.0f),
     mFlying(false),
     mExploded(false),
     mImpactNormal(0.0f, 0.0f, 1.0f),
     mSound(NULL),
     mEmitAt(0.0f, 0.0f, 0.0f),
     mEmitMS(0.0f),
     mExhaustMS(0.0f),
     mSourceObject(0),
     mSourceSlot(0),
     mNoSourceHit(0.0f),
     mLight(NULL)
{
   mTypeMask |= ProjectileObjectType;
}

SeekerProjectileT2::~SeekerProjectileT2()
{
   delete mShapeInstance;
   delete mCasingInstance;
   SAFE_DELETE(mLight);
}

bool SeekerProjectileT2::onAdd()
{
   if(!Parent::onAdd())
      return false;

      // A quarter of a metre either way, which is the box retail gives one
      // (0x00638c09).
   mObjBox.minExtents.set(-0.25f, -0.25f, -0.25f);
   mObjBox.maxExtents.set( 0.25f,  0.25f,  0.25f);
   resetWorldBox();
   addToScene();

   return true;
}

void SeekerProjectileT2::onRemove()
{
   SFX_DELETE(mSound);

   if(bool(mEmitter))
      mEmitter->deleteWhenEmpty();
   mEmitter = NULL;
   if(bool(mBubbleEmitter))
      mBubbleEmitter->deleteWhenEmpty();
   mBubbleEmitter = NULL;
   if(bool(mPuffEmitter))
      mPuffEmitter->deleteWhenEmpty();
   mPuffEmitter = NULL;
   if(bool(mExhaustEmitter))
      mExhaustEmitter->deleteWhenEmpty();
   mExhaustEmitter = NULL;

   removeFromScene();
   Parent::onRemove();
}

ParticleEmitter *SeekerProjectileT2::createEmitter(U32 dataId)
{
   ParticleEmitterData *data = NULL;
   if(!isClientObject() || !dataId || !Sim::findObject(dataId, data))
      return NULL;

   ParticleEmitter *emitter = new ParticleEmitter;
   emitter->onNewDataBlock(data, false);
   if(emitter->registerObject())
      return emitter;

   delete emitter;
   return NULL;
}

bool SeekerProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<SeekerProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   SFX_DELETE(mSound);
   if(isClientObject() && mData->soundId)
   {
      SFXTrack *track = NULL;
      if(Sim::findObject(mData->soundId, track))
         mSound = SFX->createSource(track, &getTransform());
   }

   if(bool(mEmitter))
      mEmitter->deleteWhenEmpty();
   if(bool(mBubbleEmitter))
      mBubbleEmitter->deleteWhenEmpty();
   if(bool(mPuffEmitter))
      mPuffEmitter->deleteWhenEmpty();
   if(bool(mExhaustEmitter))
      mExhaustEmitter->deleteWhenEmpty();

   mEmitter = createEmitter(mData->baseEmitterId);
   mBubbleEmitter = createEmitter(mData->bubbleEmitterId);
   mPuffEmitter = createEmitter(mData->puffEmitterId);
   mExhaustEmitter = createEmitter(mData->exhaustEmitterId);

   if(mData->hasLight)
      mTypeMask |= LightObjectType;
   else
      mTypeMask &= ~LightObjectType;

   delete mShapeInstance;
   mShapeInstance = NULL;
   if(bool(mData->mShape))
      mShapeInstance = new TSShapeInstance(mData->mShape, isClientObject());

   delete mCasingInstance;
   mCasingInstance = NULL;
   if(bool(mData->mCasingShape))
      mCasingInstance = new TSShapeInstance(mData->mCasingShape,
                                            isClientObject());

   return true;
}

/// The motor lights a flechetteDelayMs after the launch. A missile the
/// client picks up already under way says so in its first body, and skips
/// the wait.
bool SeekerProjectileT2::motorLit() const
{
   return !mCoasting || mAge * 1000.0f >= F32(mData->flechetteDelayMS);
}

/// The casing comes off **halfway** through that delay, counted in whole
/// ticks (0x0063c8c6), and that is also where the missile stops being drawn
/// along the way it was fired and starts being drawn along its flight
/// (0x0063b945).  Half a second of coasting is therefore two things, not
/// one: a quarter of a second of cased missile pointing where the launcher
/// did, then a quarter of an uncased one already following its own nose
/// before the motor pushes it.
bool SeekerProjectileT2::casingDropped() const
{
   return !mCoasting ||
          mAge >= F32(S32(mData->flechetteDelayMS) / 64) * sSeekerTickSec;
}

/// The four pieces the casing breaks into, in the missile's own frame: one
/// to each corner of its nose, five parts forward to one or two out, which
/// is what sends them ahead of it in a widening cross (0x0063ca5e).  Each
/// piece spins a full turn and a quarter a second about two axes, which the
/// debris datablock's own spin speeds would not give it.
struct CasingPiece
{
   F32 x, z;       ///< the sign each random part goes on
   F32 spinX, spinZ;
};

static const CasingPiece sCasingPieces[] =
{
   { -1.0f, -1.0f,  450.0f, -450.0f },
   {  1.0f, -1.0f,  450.0f,  450.0f },
   {  1.0f,  1.0f, -450.0f,  450.0f },
   { -1.0f,  1.0f, -450.0f, -450.0f },
};

// HH: Added for Tribes 2 compatibility.
// Four of the casing datablock, thrown from the point the motor lit at,
// each along the missile's own axes and at eight metres a second
// (0x0063c9d6).  They are the missile's own debris, not the server's, so
// nothing about them is sent.
void SeekerProjectileT2::dropCasing(const Point3F &at)
{
   if(!mData->useFlechette || !mData->casingDebrisId)
      return;

   DebrisData *data = NULL;
   if(!Sim::findObject(mData->casingDebrisId, data))
      return;

   MatrixF mat = getTransform();
   mat.setPosition(at);

   for(U32 i = 0; i < sizeof(sCasingPieces) / sizeof(sCasingPieces[0]); i++)
   {
      const CasingPiece &piece = sCasingPieces[i];

         // One and two, so no piece goes straight up the nose.
      const F32 a = gRandGen.randF() + 1.0f;
      const F32 b = gRandGen.randF() + 1.0f;

      VectorF velocity(piece.x * b, 5.0f, piece.z * a);
      velocity.normalize();
      velocity *= 8.0f;
      mat.mulV(velocity);

      Debris *casing = new Debris;
      casing->onNewDataBlock(data, false);
      casing->setTransform(mat);
      if(!casing->registerObject())
      {
         delete casing;
         continue;
      }

      casing->init(at, velocity);
      casing->setRotAngles(Point3F(piece.spinX, 0.0f, piece.spinZ));
   }
}

// HH: Added for Tribes 2 compatibility.
// The launcher's own exhaust, out of the node its datablock names on the
// image the missile was fired from, for as long as exhaustTimeMs says
// (0x0063b447).  It comes out of the launcher rather than off the missile,
// so it needs the shooter rather than this: a missile whose source has gone
// blows nothing.
void SeekerProjectileT2::blowExhaust(F32 dt)
{
   if(!bool(mExhaustEmitter) || !mData->exhaustNodeName ||
      !mData->exhaustNodeName[0])
      return;

      // Retail counts whole ticks, so 300ms of exhaust is nine of them.
   if(mAge > F32(S32(mData->exhaustTimeMS) / S32(TickMs)) * TickSec)
      return;

   ShapeBase *source = NULL;
   if(!mSourceObject || !Sim::findObject(mSourceObject, source))
      return;

   mExhaustMS += dt * 1000.0f;
   const U32 ms = U32(mExhaustMS);
   if(!ms)
      return;
   mExhaustMS -= F32(ms);

   MatrixF node(true);
   source->getImageTransform(mSourceSlot, mData->exhaustNodeName, &node);

   const Point3F at = node.getPosition();
   Point3F along;
   node.getColumn(1, &along);

      // Against the shooter's own velocity, so the cloud it leaves stands
      // still rather than being dragged along with them.
   Point3F against = source->getVelocity();
   against.neg();

   mExhaustEmitter->emitParticles(at, at, along, against, ms);
}

bool SeekerProjectileT2::targetPosition(Point3F *out)
{
   if(mChasing == ChasingNothing)
      return false;

   if(mChasing == ChasingObject)
   {
      SceneObject *obj = NULL;
      if(mTargetId && Sim::findObject(mTargetId, obj))
      {
            // The middle of its world box, which is what retail aims at
            // rather than its origin (0x0063c38a).
         const Box3F &box = obj->getWorldBox();
         mTargetPoint = (box.minExtents + box.maxExtents) * 0.5f;
      }
      else
      {
            // The lock is gone; what is left is where it last was.
         mChasing = ChasingPoint;
      }
   }

   *out = mTargetPoint;
   return true;
}

/// Turn the flight towards what it is chasing, and once it is already pointing
/// there, away from ground that stands in the way (0x006397a0).
void SeekerProjectileT2::seek(F32 dt)
{
   Point3F target;
   if(!targetPosition(&target))
      return;

      // Retail multiplies turningSpeed by the target's heat and compares the
      // product, in degrees, against an angle in radians (0x00639b29): the
      // only heat that holds the turn back at all is none.
   if(mChasing == ChasingObject)
   {
      ShapeBase *shape = NULL;
      if(Sim::findObject(mTargetId, shape) && shape->getHeat() <= 0.0f)
         return;
   }

   const VectorF toTarget = target - mPosition;
   const F32 turn = mDegToRad(mData->turningSpeed) * dt;
   const bool ahead = turnTowards(&mVelocity, toTarget, turn);

   if(!ahead)
   {
         // It has gone past: inside the proximity radius that is a hit, and
         // outside it the lock is dropped (0x0063a700, 0x0063b3b7).
      if(toTarget.len() < mData->proximityRadius)
      {
         VectorF normal = -mVelocity;
         if(!normal.isZero())
            normal.normalize();
         mFlying = false;
         explode(mPosition, normal);
         return;
      }

      mChasing = ChasingNothing;
      return;
   }

      // Only once it is lined up: a missile still turning onto its target
      // never looks at the ground (0x00639ce2).
   VectorF way = mVelocity;
   if(way.isZero() || toTarget.isZero())
      return;
   way.normalize();
   VectorF to = toTarget;
   to.normalize();
   if(mDot(to, way) >= 0.99985f)
      avoidTerrain(target, dt);
}

void SeekerProjectileT2::avoidTerrain(const Point3F &target, F32 dt)
{
   if(mData->scanAhead <= 0.0f)
      return;

      // Near the target it only pulls up for ground that stands between the
      // two: the mask is the target's own next to the terrain's, so a ray
      // that stops on the target means the way is clear (0x00639e11).
   if((mPosition - target).len() < mData->avoidanceRadius)
   {
      U32 mask = TerrainObjectType;
      SceneObject *obj = NULL;
      if(mChasing == ChasingObject && mTargetId &&
         Sim::findObject(mTargetId, obj))
         mask |= obj->getTypeMask();

      RayInfo los;
      disableCollision();
      const bool blocked = getContainer()->castRay(mPosition, target, mask,
                                                   &los) &&
                           los.object &&
                           (los.object->getTypeMask() & TerrainObjectType);
      enableCollision();

      if(!blocked)
         return;
   }

      // A look along the flight, scanAhead out and heightFail down: ground
      // inside that wedge is ground it would fly into (0x00639eeb).
   VectorF ahead = mVelocity;
   if(ahead.isZero())
      return;
   ahead.normalize();
   ahead *= mData->scanAhead;

   const Point3F probe(mPosition.x + ahead.x, mPosition.y + ahead.y,
                       mPosition.z - mData->heightFail + ahead.z);

   RayInfo info;
   disableCollision();
   const bool ground = getContainer()->castRay(mPosition, probe,
                                               TerrainObjectType, &info);
   enableCollision();

   if(ground)
      turnTowards(&mVelocity, Point3F(0.0f, 0.0f, 1.0f),
                  mDegToRad(mData->avoidanceSpeed) * dt);
}

void SeekerProjectileT2::step(F32 dt)
{
      // The motor accelerates the way the missile is already thrusting, which
      // is its flight less what it inherited from whoever fired it -- so a
      // missile fired from a moving vehicle keeps that speed and adds to it
      // along its own nose (0x0063afaf).
      //
      // A tick's worth at a time, not a frame's: retail stops the moment the
      // speed has passed maxVelocity, so what it settles at is the first
      // whole tick of thrust past it -- 93.2 metres a second for a shoulder
      // missile, not the 90 its datablock asks for. A frame's worth at a time
      // settles on the 90 instead, which is the wrong answer by 3%.
   if(motorLit())
   {
      mThrustTime += dt;
      while(mThrustTime >= sSeekerTickSec)
      {
         mThrustTime -= sSeekerTickSec;
         if(mVelocity.len() >= mData->maxVelocity)
            continue;

         VectorF thrust = mVelocity - mInherit;
         if(thrust.isZero())
            continue;
         thrust.normalize();
         mVelocity += thrust * mData->acceleration * sSeekerTickSec;
      }
   }

   seek(dt);
   if(!mFlying)
      return;

   const Point3F from = mPosition;
   const Point3F to = from + mVelocity * dt;

      // A ghost only stops on the static world: what a missile does to a
      // player is the server's to decide, and its body says so (0x0063b57c
      // passes no damageable mask).
   SceneObject *source = NULL;
   if(mNoSourceHit > 0.0f)
   {
      mNoSourceHit -= dt;
      if(mSourceObject && Sim::findObject(mSourceObject, source))
         source->disableCollision();
   }

   RayInfo info;
   disableCollision();
   const bool hit = getContainer()->castRay(from, to, sProjectileStaticMask,
                                            &info);
   enableCollision();

   if(source)
      source->enableCollision();

   if(!hit)
   {
      mPosition = to;
      return;
   }

      // While the casing is still on it is a casing coming out of the
      // launcher, and what it touches does not set it off -- the same half
      // delay the casing comes off at, not the whole one (0x0063b5f2).
   mPosition = info.point;
   mFlying = false;
   if(casingDropped())
      explode(info.point, info.normal);
}

/// The explosion goes a centimetre back along the way it came, facing out of
/// whatever stopped it (0x00639414).
void SeekerProjectileT2::explode(const Point3F &at, const VectorF &normal)
{
   if(mExploded)
      return;

   mExploded = true;
   mImpactNormal = normal;
   mPosition = at;

   ExplosionData *data = NULL;
   if(!mData || !mData->explosionId ||
      !Sim::findObject(mData->explosionId, data) || !data)
      return;

   const Point3F blastAt = at + normal * 0.01f;

   Explosion *blast = new Explosion;
   blast->onNewDataBlock(data, false);

   MatrixF mat(true);
   mat.setPosition(blastAt);
   blast->setTransform(mat);
   blast->setInitialState(blastAt, normal);

   if(!blast->registerObject())
   {
      Con::warnf("SeekerProjectileT2: %s could not register its explosion.",
                 mData->getName());
      delete blast;
   }
}

void SeekerProjectileT2::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   if(body.hasSourceObject)
   {
      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(S32(body.sourceObject))
                            : NULL;
      mSourceObject = obj ? obj->getId() : 0;
      mSourceSlot = getMin(body.sourceSlot,
                           U32(ShapeBase::MaxMountedImages - 1));
      mNoSourceHit = 7.0f * TickSec;
   }

      // The inherited velocity and whether the motor has lit come only on the
      // first body, and the first body is also the launch.
   if(body.hasInheritedVel)
      mInherit = body.inheritedVel;
   if(body.hasPosition && body.hasVelocity && !mFlying)
   {
      mCoasting = body.seekCoasting;
      mAge = 0.0f;
      mLaunchDir = body.velocity - mInherit;
      if(mLaunchDir.isZero())
         mLaunchDir = body.velocity;
      if(mLaunchDir.isZero())
         mLaunchDir.set(0.0f, 1.0f, 0.0f);
      mLaunchDir.normalize();
   }

   if(body.hasSeekInfo)
   {
      if(body.hasSeekObject)
      {
         NetConnection *conn = NetConnection::getConnectionToServer();
         NetObject *obj = conn ? conn->resolveGhost(S32(body.seekObject))
                               : NULL;
         SceneObject *scene = dynamic_cast<SceneObject *>(obj);
         if(scene)
         {
            mTargetId = scene->getId();
            mChasing = ChasingObject;
            const Box3F &box = scene->getWorldBox();
            mTargetPoint = (box.minExtents + box.maxExtents) * 0.5f;
         }
         else
            mChasing = ChasingNothing;
      }
      else if(body.hasSeekPoint)
      {
         mTargetPoint = body.seekPoint;
         mChasing = ChasingPoint;
      }
      else
         mChasing = ChasingNothing;
   }

      // A body with both ends is the detonation; one with a position and a
      // velocity is the flight, and the server's is the one that counts.
   if(body.hasExplodePoint)
   {
      mFlying = false;
      explode(body.explodePoint,
              body.hasImpactNormal ? body.impactNormal : mImpactNormal);
      return;
   }

   if(body.hasPosition && body.hasVelocity)
   {
      if(!mFlying)
         mEmitAt = body.position;

      mPosition = body.position;
      mVelocity = body.velocity;
      mFlying = true;

      MatrixF mat(true);
      mat.setPosition(mPosition);
      setTransform(mat);
   }
}

void SeekerProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(dt <= 0.0f)
      return;

   if(mSound)
   {
      if(!mFlying || mExploded)
         mSound->stop();
      else
      {
         if(!mSound->isPlaying())
            mSound->play();

         mSound->setVelocity(mVelocity);
         mSound->setTransform(getRenderTransform());
      }
   }

   if(!mFlying)
      return;

   const bool hadCasing = !casingDropped();
   mAge += dt;

      // A missile that lost its last body keeps flying on its own; retail's
      // server ends it at its lifetime and sends the body that says so, and
      // without that body there is nothing to draw past it (0x00638974).
   if(mData->lifetimeMS && mAge * 1000.0f > F32(mData->lifetimeMS))
   {
      mFlying = false;
      return;
   }

   step(dt);

      // The casing comes off in a puff where the motor lights, and breaks
      // into four as it goes (0x0063c93d).  The puff is laid still: its
      // particles carry no velocity of the missile's, so the cloud stays
      // where the casing left rather than chasing the missile.
   if(hadCasing && casingDropped() && !mPuffed)
   {
      mPuffed = true;
      if(bool(mPuffEmitter))
         mPuffEmitter->emitParticles(mPosition, mPosition,
                                     Point3F(0.0f, 0.0f, 1.0f),
                                     Point3F(0.0f, 0.0f, 0.0f), 1000);
      dropCasing(mPosition);
   }

   blowExhaust(dt);

      // The smoke runs from where the last of it was laid to where the flight
      // has got, backwards along it, and under water the bubbles rise
      // instead -- Projectile::emitParticles (0x00632530), a frame at a time
      // here with the milliseconds a frame leaves over kept for the next.
   if(bool(mEmitter) || bool(mBubbleEmitter))
   {
      mEmitMS += dt * 1000.0f;
      const U32 ms = U32(mEmitMS);
      if(ms)
      {
         const bool inWater = isInWaterT2(getContainer(), mPosition);
         ParticleEmitter *emitter = inWater ? mBubbleEmitter : mEmitter;

         VectorF axis = -mVelocity;
         if(inWater || axis.isZero())
            axis.set(0.0f, 0.0f, 1.0f);
         else
            axis.normalize();

         if(emitter)
            emitter->emitParticles(mEmitAt, mPosition, axis, mVelocity, ms);
         mEmitAt = mPosition;
         mEmitMS -= F32(ms);
      }
   }

      // Until the casing is off, the missile is still pointing the way it was
      // fired; after it, along its flight (0x0063b98e).
   MatrixF mat(true);
   headingMatrixT2(&mat, casingDropped() ? mVelocity : mLaunchDir);
   mat.setPosition(mPosition);
   setTransform(mat);

   if(mShapeInstance)
      mShapeInstance->advanceTime(dt);
   if(mCasingInstance)
      mCasingInstance->advanceTime(dt);
}

void SeekerProjectileT2::prepRenderImage(SceneRenderState *state)
{
   // A missile that drops a casing is the casing until it does, and the
   // missile under it afterwards (0x0063cf97).
   TSShapeInstance *instance = mShapeInstance;
   if(mData->useFlechette && !casingDropped() && mCasingInstance)
      instance = mCasingInstance;

   if(!instance || !mFlying)
      return;

   GFXTransformSaver saver;

   TSRenderState rdata;
   rdata.setSceneState(state);

   LightQuery query;
   query.init(getWorldSphere());
   rdata.setLightQuery(&query);

   MatrixF mat = getRenderTransform();
   mat.scale(mData->scale);
   GFX->setWorldMatrix(mat);

   instance->setDetailFromPosAndScale(state, mat.getPosition(), mData->scale);
   instance->animate();
   instance->render(rdata);
}

void SeekerProjectileT2::submitLights(LightManager *lm, bool staticLighting)
{
   if(staticLighting || !mData || !mData->hasLight || !mFlying)
      return;

   if(!mLight)
      mLight = LightManager::createLightInfo();

   mLight->setType(LightInfo::Point);
   mLight->setPosition(getRenderPosition());
   mLight->setRange(mData->lightRadius);
   mLight->setColor(mData->lightColor);

   lm->registerGlobalLight(mLight, this);
}

#define IMPLEMENT_SEEKER_T2( objectName, dataName )                          \
   class dataName : public SeekerProjectileDataT2                            \
   {                                                                         \
      typedef SeekerProjectileDataT2 Parent;                                 \
   public:                                                                   \
      DECLARE_CONOBJECT(dataName);                                           \
   };                                                                        \
   IMPLEMENT_CO_DATABLOCK_V1(dataName);                                      \
   ConsoleDocClass( dataName, "@brief A Tribes 2 seeking missile's "          \
                              "datablock.\n\n@internal" );                   \
   class objectName : public SeekerProjectileT2                              \
   {                                                                         \
      typedef SeekerProjectileT2 Parent;                                     \
   public:                                                                   \
      DECLARE_CONOBJECT(objectName);                                         \
   };                                                                        \
   IMPLEMENT_CO_NETOBJECT_V1(objectName);                                    \
   ConsoleDocClass( objectName, "@brief A Tribes 2 seeking missile."          \
                                "\n\n@internal" )

IMPLEMENT_SEEKER_T2( SeekerProjectile, SeekerProjectileData );
