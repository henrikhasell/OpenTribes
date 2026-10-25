// HH: Added for Tribes 2 compatibility.
/// A Tribes 2 projectile is told to a client once, in the body that creates
/// it: where it left the muzzle and which way it is going. How fast and for
/// how long are its datablock's, and every position after the first is the
/// client's own arithmetic -- so a projectile that is only decoded is a
/// projectile that is never anywhere, and never drawn.
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "lighting/lightQuery.h"
#include "math/mathUtils.h"
#include "math/mRandom.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneRenderState.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTrack.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/processList.h"
#include "T3D/fx/explosion.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/objectTypes.h"
#include "T3D/gameBase/gravityT2.h"
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
   U32    soundId;         ///< the AudioProfile it flies to
   F32    blurLifetime;    ///< seconds a length of the trail lasts
   F32    blurWidth;
   ColorF blurColor;
   String texture[2];      ///< the bolt's and the cross's
   U32    numFlares;       ///< the petals a plasma bolt is seen as
   F32    flareSize[3];    ///< how far out they start and end
   ColorF flareColor;
   bool   faceViewer;      ///< its shape is turned to the camera

   ProjectileDataT2();

      /// A bullet is seen as the streak it leaves, which is what retail draws
      /// of one, so the shape its datablock also names is never wanted.
   bool namesGameBaseShape() const { return streakLength <= 0.0f; }
   bool unpackData(BitStream *stream, U32 classId);

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
     soundId(0),
     blurLifetime(0.0f),
     blurWidth(0.0f),
     blurColor(1.0f, 1.0f, 1.0f, 1.0f),
     numFlares(0),
     flareColor(1.0f, 1.0f, 1.0f, 1.0f),
     faceViewer(false)
{
   flareSize[0] = flareSize[1] = flareSize[2] = 0.0f;
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

class ProjectileT2 : public GameBase
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
   VectorF mImpactNormal;
   SFXSource *mSound;         ///< what it is heard as on the way

   SimObjectId mSourceObject;
   SimObjectId mVehicleObject;
   F32     mNoSourceHit;

   GFXStateBlockRef boltStateBlock();
   TSThread *startThread(const char *name);
   void step(F32 dt);
   void explode();
   void headingMatrix(MatrixF *mat);
   void ageTrail(F32 dt);
   void renderBolt(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);
   void drawTrail(const Point3F &eye);
   void drawStreak(const Point3F &eye);
   void drawBolt(const Point3F &eye);
   void drawCross(const Point3F &eye);
   void spawnFlare(Flare &flare);
   void drawFlares();

public:
   ProjectileT2();
   ~ProjectileT2();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr, bool reload);
   void onT2BodyRead(const T2Layout::Body &body);
   void advanceTime(F32 dt);
   void prepRenderImage(SceneRenderState *state);

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
     mImpactNormal(0.0f, 0.0f, 1.0f),
     mSourceObject(0),
     mVehicleObject(0),
     mNoSourceHit(0.0f)
{
   mTypeMask |= ProjectileObjectType;
}

ProjectileT2::~ProjectileT2()
{
   delete mShapeInstance;
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
         mTrailAt = mPosition;
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
   headingMatrix(&mat);
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

/// A force field is in retail's static half of this, at 0x007abf60, next to
/// static objects, terrain and interiors.
static const U32 sProjectileCollisionMask =
   StaticObjectType | TerrainObjectType | InteriorObjectType |
   StaticShapeObjectType | PlayerObjectType | VehicleObjectType |
   ItemObjectType | ForceFieldObjectType;

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

      // A disc goes off on the first thing it touches; a grenade bounces
      // until its arming delay has run and goes off on the first thing after
      // that (0x00634e0f).
   if(!mBounces || mAge * 1000.0f > F32(mData->armingDelayMS))
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

/// A shape flies nose first along its own +Y, with the world's up for its
/// roll: the trail of a disc runs down its -Y, and a roll taken from any
/// perpendicular at all turns the disc over at random.
void ProjectileT2::headingMatrix(MatrixF *mat)
{
   VectorF forward = mVelocity;
   if(forward.isZero())
      return;
   forward.normalize();

   VectorF right = mCross(forward, Point3F(0.0f, 0.0f, 1.0f));
   if(right.len() < 0.001f)
      right = mCross(forward, Point3F(1.0f, 0.0f, 0.0f));
   right.normalize();

   const VectorF up = mCross(right, forward);

   mat->setColumn(0, right);
   mat->setColumn(1, forward);
   mat->setColumn(2, up);
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

   drawFlares();
   if(mShapeInstance)
      return;

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
/// as its root (0x0063ee80, 0x0063f4e0).
void ProjectileT2::drawFlares()
{
   if(mFlares.empty() || mFlareTexture.isNull())
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
   const ColorF &c = mData->flareColor;
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

#define IMPLEMENT_PROJECTILE_T2( className )                                 \
   class className : public ProjectileT2                                     \
   {                                                                         \
      typedef ProjectileT2 Parent;                                           \
   public:                                                                   \
      DECLARE_CONOBJECT(className);                                          \
   };                                                                        \
   IMPLEMENT_CO_NETOBJECT_V1(className);                                     \
   ConsoleDocClass( className, "@brief A Tribes 2 projectile.\n\n@internal" )

IMPLEMENT_PROJECTILE_T2( EnergyProjectile );
IMPLEMENT_PROJECTILE_T2( GrenadeProjectile );
IMPLEMENT_PROJECTILE_T2( LinearFlareProjectile );
IMPLEMENT_PROJECTILE_T2( LinearProjectile );
IMPLEMENT_PROJECTILE_T2( TracerProjectile );
