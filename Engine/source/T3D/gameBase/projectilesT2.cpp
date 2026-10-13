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
#include "renderInstance/renderPassManager.h"
#include "scene/sceneRenderState.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/processList.h"
#include "T3D/fx/explosion.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/objectTypes.h"
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
   U32    armingDelayMS;   ///< it bounces until this, and goes off after
   U32    explosionId;     ///< the ExplosionData it ends as
   F32    blurLifetime;    ///< seconds a length of the trail lasts
   F32    blurWidth;
   ColorF blurColor;
   String texture[2];      ///< the bolt's and the cross's

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
     armingDelayMS(0),
     explosionId(0),
     blurLifetime(0.0f),
     blurWidth(0.0f),
     blurColor(1.0f, 1.0f, 1.0f, 1.0f)
{
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

   Point3F mStart;            ///< where the body put it
   Point3F mPosition;         ///< where it has got to since
   VectorF mVelocity;         ///< metres a second, which is all the flight is
   bool    mNeedsSpeed;       ///< the body gave a direction and the datablock
                              ///< the speed, and it arrives after the body
   bool    mBounces;          ///< a grenade's family does; a disc goes off
   F32     mAge;              ///< seconds since that body
   bool    mFlying;
   bool    mExploded;         ///< it only goes off once
   VectorF mImpactNormal;

   TSThread *startThread(const char *name);
   void step(F32 dt);
   void explode();
   void headingMatrix(MatrixF *mat);
   void ageTrail(F32 dt);
   void renderBolt(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);
   void drawTrail(const Point3F &eye);
   void drawBolt(const Point3F &eye);
   void drawCross(const Point3F &eye);

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
     mAge(0.0f),
     mFlying(false),
     mExploded(false),
     mImpactNormal(0.0f, 0.0f, 1.0f)
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
   removeFromScene();
   Parent::onRemove();
}

bool ProjectileT2::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mData = dynamic_cast<ProjectileDataT2 *>(dptr);
   if(!mData || !Parent::onNewDataBlock(dptr, reload))
      return false;

   delete mShapeInstance;
   mShapeInstance = NULL;

   mAmbientThread = NULL;
   mFlightThread = NULL;
   mMaintainSeq = -1;

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

   if(!body.hasPosition)
      return;

   mStart = mPosition = body.position;
   if(mTrail.empty())
      mTrailAt = body.position;
   mAge = 0.0f;

      // A grenade's family is told its velocity; a disc's is told the way it
      // is going and takes the speed off its datablock. Either way the client
      // flies it from here, tests it against the world itself, and sets it
      // off where it lands -- which is what retail's own simulate does
      // (LinearProjectile at 0x0062e010, GrenadeProjectile at 0x00634910).
   if(body.hasVelocity)
   {
      mVelocity = body.velocity;
      mNeedsSpeed = false;
      mBounces = true;
      mFlying = true;
   }
   else if(body.hasDirection)
   {
         // The datablock arrives after the body it belongs to, so the speed
         // cannot be applied here.
      mVelocity = body.direction;
      mNeedsSpeed = true;
      mBounces = false;
      mFlying = true;
   }
   else
   {
         // A body that says where it is without saying which way it is going
         // is the one that says it has gone off. Retail draws nothing of the
         // projectile from here, and plays the explosion its datablock names.
      mFlying = false;
      if(body.hasImpactNormal)
         mImpactNormal = body.impactNormal;
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

   if(!mFlying)
      return;

   if(mNeedsSpeed)
   {
      mVelocity *= mData->velocity;
      mNeedsSpeed = false;
   }

   mAge += dt;

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

/// What a projectile collides with. Retail takes players and vehicles too and
/// turns the shooter's own collision off for the first few ticks; until the
/// body's source object is read, the world alone is the safe half -- a muzzle
/// that counted as a hit would go off in the shooter's face.
static const U32 sProjectileCollisionMask =
   TerrainObjectType | InteriorObjectType | StaticShapeObjectType;

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
      // The datablock's velocity already set the speed, so a projectile with
      // no gravity of its own holds its line.
   mVelocity.z -= 9.81f * mData->gravityMod * dt;

   const Point3F from = mPosition;
   const Point3F to = from + mVelocity * dt;

   RayInfo info;
   disableCollision();
   const bool hit = getContainer()->castRay(from, to,
                                            sProjectileCollisionMask, &info);
   enableCollision();

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

      MatrixF mat = getRenderTransform();
      mat.scale(mObjScale);
      GFX->setWorldMatrix(mat);

      mShapeInstance->setDetailFromPosAndScale(state, mat.getPosition(),
                                               mObjScale);
      mShapeInstance->animate();
      mShapeInstance->render(rdata);
      return;
   }

   if(mTexture[0].isNull() && mTrail.empty())
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

   GFX->setStateBlock(mBoltSB);
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

   GFX->setStateBlock(mBoltSB);
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
IMPLEMENT_PROJECTILE_T2( LinearFlareProjectile );
IMPLEMENT_PROJECTILE_T2( LinearProjectile );
IMPLEMENT_PROJECTILE_T2( TracerProjectile );
