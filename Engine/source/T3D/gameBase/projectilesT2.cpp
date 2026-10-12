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
   F32    boltSize;        ///< metres across, for a bolt that is neither
   F32    elasticity;      ///< what a bounce keeps of its speed
   F32    friction;        ///< what a bounce takes off along the surface
   F32    gravityMod;      ///< a share of gravity, and zero for a bolt
   String texture;         ///< what to draw it with, where there is no shape

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
     boltSize(0.0f),
     elasticity(0.0f),
     friction(0.0f),
     gravityMod(0.0f),
     texture(String::EmptyString)
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
      boltSize = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbElasticity, &value))
      elasticity = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFriction, &value))
      friction = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbGravityMod, &value))
      gravityMod = value;

      // Every one of these layouts names its shape first and its textures
      // after it, so a projectile with no shape is drawn with the second
      // string the body carried.
   const Vector<String> &strings = T2Layout::lastStrings();
   if(strings.size() > 1)
      texture = strings[1];

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
   GFXTexHandle      mTexture;
   GFXStateBlockRef  mBoltSB;

   Point3F mStart;            ///< where the body put it
   Point3F mPosition;         ///< where it has got to since
   VectorF mHeading;          ///< a unit vector, or a whole velocity where
   bool    mHeadingIsVelocity;///< the body carried one
   F32     mAge;              ///< seconds since that body
   bool    mFlying;

   TSThread *startThread(const char *name);
   void step(F32 dt);
   void headingMatrix(MatrixF *mat);
   void renderBolt(ObjectRenderInst *ri, SceneRenderState *state,
                   BaseMatInstance *overrideMat);

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
     mHeading(0.0f, 0.0f, 0.0f),
     mHeadingIsVelocity(false),
     mAge(0.0f),
     mFlying(false)
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

         // The spin of a disc is in the shape, not in the transform: it
         // carries Ambient, Activate and Maintain, and a projectile that
         // plays none of them is a frozen frame of itself.
      mAmbientThread = startThread("ambient");
      mMaintainSeq = mData->mShape->findSequence("maintain");
      mFlightThread = startThread("activate");
      if(!mFlightThread && mMaintainSeq >= 0)
         mFlightThread = startThread("maintain");
   }
   else if(mData->texture.isNotEmpty())
      mTexture.set(String("textures/") + mData->texture,
                   &GFXDefaultStaticDiffuseProfile, "ProjectileT2");

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
   mAge = 0.0f;

   if(body.hasVelocity)
   {
      mHeading = body.velocity;
      mHeadingIsVelocity = true;
      mFlying = true;
   }
   else if(body.hasDirection)
   {
      mHeading = body.direction;
      mHeadingIsVelocity = false;
      mFlying = true;
   }
   else
         // A body that says where it is without saying which way it is going
         // is the one that says it has gone off, and retail draws nothing of
         // it either.
      mFlying = false;
}

void ProjectileT2::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if(!mFlying || dt <= 0.0f)
      return;

   mAge += dt;

   if(mData->lifetimeMS && mAge * 1000.0f > F32(mData->lifetimeMS))
   {
      mFlying = false;
      return;
   }

      // A projectile the body gave a whole velocity to is one the client is
      // meant to fly itself, bounces and all; one given a direction holds the
      // line its datablock's velocity puts it on, and the server says where
      // it ended.
   if(mHeadingIsVelocity)
      step(dt);
   else
      mPosition = mStart + mHeading * (mData->velocity * mAge);

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

/// What a projectile collides with, which is Projectile's own two masks.
static const U32 sProjectileCollisionMask =
   TerrainObjectType | InteriorObjectType | StaticShapeObjectType |
   PlayerObjectType | VehicleObjectType;

/// One step of the flight of a grenade -- which a blaster bolt is underneath,
/// at no gravity and very nearly all of its speed kept. The bounce is
/// Projectile::simulate's, because that is the same bounce Tribes 2 hands its
/// client the elasticity and friction for.
void ProjectileT2::step(F32 dt)
{
   mHeading.z -= 9.81f * mData->gravityMod * dt;

   const Point3F from = mPosition;
   const Point3F to = from + mHeading * dt;

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

   const Point3F bounced =
      mHeading - info.normal * (mDot(mHeading, info.normal) * 2.0f);
   const Point3F tangent =
      bounced - info.normal * mDot(bounced, info.normal);

   mHeading = (bounced - tangent * mData->friction) * mData->elasticity;

      // Off the surface, so the next step does not start inside it.
   mPosition = info.point + info.normal * 0.05f;
}

/// A shape flies nose first along its own +Y, with the world's up for its
/// roll: the trail of a disc runs down its -Y, and a roll taken from any
/// perpendicular at all turns the disc over at random.
void ProjectileT2::headingMatrix(MatrixF *mat)
{
   VectorF forward = mHeading;
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
   if(!mFlying)
      return;

   if(mShapeInstance)
   {
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

   if(mTexture.isNull())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &ProjectileT2::renderBolt);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

void ProjectileT2::renderBolt(ObjectRenderInst *, SceneRenderState *state,
                              BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

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
   GFX->setTexture(0, mTexture);

   const Point3F at = getRenderTransform().getPosition();
   const Point3F eye = state->getCameraPosition();

   Point3F corner[4];
   if(mData->streakLength > 0.0f)
   {
         // A bullet is a streak drawn back along the flight it has had.
      VectorF across = mCross(mHeading, at - eye);
      if(across.isZero())
         return;
      across.normalize();
      across *= mData->streakWidth * 0.5f;

      VectorF along = mHeading;
      along.normalize();
      const Point3F tail = at - along * mData->streakLength;

      corner[0] = tail + across;
      corner[1] = at + across;
      corner[2] = at - across;
      corner[3] = tail - across;
   }
   else
   {
         // Anything else is a square held square to the camera.
      const F32 half = (mData->boltSize > 0.0f ? mData->boltSize : 0.5f) * 0.5f;
      VectorF right, up;
      state->getCameraTransform().getColumn(0, &right);
      state->getCameraTransform().getColumn(2, &up);
      right *= half;
      up *= half;

      corner[0] = at - right + up;
      corner[1] = at + right + up;
      corner[2] = at + right - up;
      corner[3] = at - right - up;
   }

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, 1.0f);
   PrimBuild::begin(GFXTriangleFan, 4);
   PrimBuild::texCoord2f(0.0f, 0.0f);
   PrimBuild::vertex3fv(corner[0]);
   PrimBuild::texCoord2f(1.0f, 0.0f);
   PrimBuild::vertex3fv(corner[1]);
   PrimBuild::texCoord2f(1.0f, 1.0f);
   PrimBuild::vertex3fv(corner[2]);
   PrimBuild::texCoord2f(0.0f, 1.0f);
   PrimBuild::vertex3fv(corner[3]);
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
