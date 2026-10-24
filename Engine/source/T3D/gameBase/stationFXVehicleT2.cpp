// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/stationFXVehicleT2.h"

#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "math/mathTypes.h"
#include "math/mathUtils.h"
#include "math/util/matrixSet.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneRenderState.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/shapeBase.h"
#include "ts/tsShapeInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(StationFXVehicleData);

ConsoleDocClass( StationFXVehicleData,
   "@brief What a Tribes 2 vehicle pad shows while it builds a "
   "vehicle.\n\n@internal" );

/// The defaults the constructor at 0x0069cf50 sets, which are not the ones
/// serverVehicleHud.cs gives VehicleInvFX.
StationFXVehicleData::StationFXVehicleData()
   : lifetime(6.0f),
     glowTopHeight(0.5f),
     glowBottomHeight(0.1f),
     glowTopRadius(8.0f),
     glowBottomRadius(7.5f),
     numGlowSegments(20),
     glowFadeTime(1.0f),
     armLightDelay(2.0f),
     armLightLifetime(5.0f),
     armLightFadeTime(2.0f),
     numArcSegments(10),
     sphereColor(0.1f, 0.1f, 0.5f, 1.0f),
     spherePhiSegments(13),
     sphereThetaSegments(5),
     sphereRadius(12.0f),
     sphereScale(1.0f, 1.0f, 0.85f)
{
   glowNodeName = StringTable->EmptyString();
   for(U32 i = 0; i < NumArmNodes; i++)
   {
      leftNodeName[i] = StringTable->EmptyString();
      rightNodeName[i] = StringTable->EmptyString();
   }
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = StringTable->EmptyString();
}

void StationFXVehicleData::initPersistFields()
{
   addField("glowTopHeight",       TypeF32,     Offset(glowTopHeight,       StationFXVehicleData));
   addField("glowBottomHeight",    TypeF32,     Offset(glowBottomHeight,    StationFXVehicleData));
   addField("glowTopRadius",       TypeF32,     Offset(glowTopRadius,       StationFXVehicleData));
   addField("glowBottomRadius",    TypeF32,     Offset(glowBottomRadius,    StationFXVehicleData));
   addField("numGlowSegments",     TypeS32,     Offset(numGlowSegments,     StationFXVehicleData));
   addField("glowFadeTime",        TypeF32,     Offset(glowFadeTime,        StationFXVehicleData));
   addField("armLightDelay",       TypeF32,     Offset(armLightDelay,       StationFXVehicleData));
   addField("armLightLifetime",    TypeF32,     Offset(armLightLifetime,    StationFXVehicleData));
   addField("armLightFadeTime",    TypeF32,     Offset(armLightFadeTime,    StationFXVehicleData));
   addField("sphereColor",         TypeColorF,  Offset(sphereColor,         StationFXVehicleData));
   addField("spherePhiSegments",   TypeS32,     Offset(spherePhiSegments,   StationFXVehicleData));
   addField("sphereThetaSegments", TypeS32,     Offset(sphereThetaSegments, StationFXVehicleData));
   addField("sphereRadius",        TypeF32,     Offset(sphereRadius,        StationFXVehicleData));
   addField("sphereScale",         TypePoint3F, Offset(sphereScale,         StationFXVehicleData));
   addField("lifetime",            TypeF32,     Offset(lifetime,            StationFXVehicleData));
   addField("numArcSegments",      TypeS32,     Offset(numArcSegments,      StationFXVehicleData));
   addField("glowNodeName",        TypeString,  Offset(glowNodeName,        StationFXVehicleData));
   addField("leftNodeName",        TypeString,  Offset(leftNodeName,        StationFXVehicleData), NumArmNodes);
   addField("rightNodeName",       TypeString,  Offset(rightNodeName,       StationFXVehicleData), NumArmNodes);
   addField("texture",             TypeString,  Offset(textureName,         StationFXVehicleData), NumTextures);

   Parent::initPersistFields();
}

/// 0x0069d2d0: the textures, on a client only.
bool StationFXVehicleData::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(server)
      return true;

   for(U32 i = 0; i < NumTextures; i++)
      if(textureName[i] && textureName[i][0])
         texture[i].set(String("textures/") + textureName[i],
                        &GFXDefaultStaticDiffuseProfile, "StationFXVehicleData");

   return true;
}

static ColorF colorOfWord(U32 word)
{
   return ColorF(F32(word & 0xff) / 255.0f,
                 F32((word >> 8) & 0xff) / 255.0f,
                 F32((word >> 16) & 0xff) / 255.0f,
                 F32((word >> 24) & 0xff) / 255.0f);
}

static StringTableEntry stringAt(const Vector<String> &strings, U32 i)
{
   return StringTable->insert(i < strings.size() ? strings[i].c_str() : "");
}

bool StationFXVehicleData::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationGlowTopHeight, &value))
      glowTopHeight = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationGlowBottomHeight, &value))
      glowBottomHeight = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationGlowTopRadius, &value))
      glowTopRadius = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationGlowBottomRadius, &value))
      glowBottomRadius = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationNumGlowSegments, &word))
      numGlowSegments = S32(word);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationGlowFadeTime, &value))
      glowFadeTime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationArmLightDelay, &value))
      armLightDelay = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationArmLightLifetime, &value))
      armLightLifetime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationArmLightFadeTime, &value))
      armLightFadeTime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationLifetime, &value))
      lifetime = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationNumArcSegments, &word))
      numArcSegments = S32(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationSphereColor, &word))
      sphereColor = colorOfWord(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationSpherePhiSegments, &word))
      spherePhiSegments = S32(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationSphereThetaSegments, &word))
      sphereThetaSegments = S32(word);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationSphereRadius, &value))
      sphereRadius = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationSphereScaleX, &value))
      sphereScale.x = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationSphereScaleY, &value))
      sphereScale.y = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationSphereScaleZ, &value))
      sphereScale.z = value;

   // The glow node, then each left node beside its right one, then the
   // textures (0x0069d6e0).
   const Vector<String> &strings = T2Layout::lastStrings();
   U32 next = 0;
   glowNodeName = stringAt(strings, next++);
   for(U32 i = 0; i < NumArmNodes; i++)
   {
      leftNodeName[i] = stringAt(strings, next++);
      rightNodeName[i] = stringAt(strings, next++);
   }
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = stringAt(strings, next++);

   return true;
}

IMPLEMENT_CO_NETOBJECT_V1(StationFXVehicle);

ConsoleDocClass( StationFXVehicle,
   "@brief The effect a Tribes 2 vehicle pad plays while it builds a "
   "vehicle.\n\n@internal" );

namespace
{
   /// What retail draws the dome in (0x0069dfbf), whatever sphereColor
   /// says.
   const ColorF sDomeColor(0.1f, 0.1f, 0.5f, 1.0f);

   /// A node's transform in the station's space as its shape last
   /// animated it, or none where the shape has no such node (0x0069dc60).
   MatrixF nodeTransform(TSShapeInstance *shape, const char *name)
   {
      if(name && name[0])
      {
         const S32 node = shape->getShape()->findNode(name);
         if(node >= 0 && node < shape->mNodeTransforms.size())
            return shape->mNodeTransforms[node];
      }

      return MatrixF(true);
   }

   /// Half the distance between two nodes, which is the radius of the arc
   /// that joins them round the pad (0x0069dd30); one where either is
   /// missing.
   F32 armRadius(TSShapeInstance *shape, const char *left, const char *right)
   {
      if(!left || !left[0] || !right || !right[0])
         return 1.0f;

      const TSShape *ts = shape->getShape();
      const S32 a = ts->findNode(left);
      const S32 b = ts->findNode(right);
      const S32 count = shape->mNodeTransforms.size();
      if(a < 0 || b < 0 || a >= count || b >= count)
         return 1.0f;

      const Point3F d = shape->mNodeTransforms[a].getPosition() -
                        shape->mNodeTransforms[b].getPosition();
      if(d.x * d.x < 0.0001f && d.y * d.y < 0.0001f && d.z * d.z < 0.0001f)
         return 0.0f;

      return d.len() * 0.5f;
   }
}

/// 0x0069d740.
StationFXVehicle::StationFXVehicle()
   : mDataBlock(NULL),
     mStationId(-1),
     mElapsedMS(0),
     mElapsed(0.0f)
{
}

/// 0x0069d790.
void StationFXVehicle::initPersistFields()
{
   addField("stationObject", TypeS32, Offset(mStationId, StationFXVehicle));

   Parent::initPersistFields();
}

/// Retail sets the server's object on its station (0x0069d7c0) and leaves
/// the client's at the origin, drawing from the station's transform either
/// way. Putting the client's there too keeps it in scope and lets the scene
/// cull it where it is drawn.
void StationFXVehicle::placeAtStation()
{
   // The box retail gives it, and the station's own, which holds the arms.
   Box3F box(Point3F(-12.0f, -12.0f, 0.0f), Point3F(12.0f, 12.0f, 12.0f));
   if(mStation)
   {
      box.intersect(mStation->getObjBox());
      setTransform(mStation->getTransform());
   }

   // The dome and the ring, which a datablock can make larger than either.
   if(mDataBlock)
   {
      const F32 dome = mDataBlock->sphereRadius *
                       getMax(mDataBlock->sphereScale.x, mDataBlock->sphereScale.y);
      const F32 reach = getMax(dome, getMax(mDataBlock->glowTopRadius,
                                            mDataBlock->glowBottomRadius));
      const F32 height = getMax(mDataBlock->sphereRadius * mDataBlock->sphereScale.z,
                                mDataBlock->glowTopHeight);
      box.intersect(Box3F(Point3F(-reach, -reach, 0.0f),
                          Point3F(reach, reach, height)));
   }

   mObjBox = box;
   resetWorldBox();
}

bool StationFXVehicle::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if(isServerObject() && mStationId != -1)
   {
      SceneObject *station = NULL;
      if(Sim::findObject(mStationId, station))
         mStation = station;
   }

   placeAtStation();
   addToScene();
   return true;
}

void StationFXVehicle::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

/// 0x0069d8f0.
bool StationFXVehicle::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mDataBlock = dynamic_cast<StationFXVehicleData *>(dptr);
   if(!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   scriptOnNewDataBlock();
   return true;
}

/// 0x0069eea0: the station comes as a ghost index on the first body only,
/// and an effect whose station the client has not got draws nothing.
bool StationFXVehicle::unpackUpdate(BitStream *stream, bool isInitial)
{
   if(!Parent::unpackUpdate(stream, isInitial))
      return false;

   const T2Layout::Body &body = T2Layout::lastBody();
   if(body.hasStationObject)
   {
      mStationId = S32(body.stationObject);

      NetConnection *conn = NetConnection::getConnectionToServer();
      NetObject *obj = conn ? conn->resolveGhost(mStationId) : NULL;
      mStation = dynamic_cast<SceneObject *>(obj);

      if(isProperlyAdded())
         placeAtStation();
   }

   return true;
}

/// 0x0069d950: the server's copy goes once its lifetime is up, which takes
/// the client's with it.
void StationFXVehicle::processTick(const Move *move)
{
   Parent::processTick(move);

   mElapsedMS += TickMs;
   if(isServerObject() && mDataBlock &&
      F32(mElapsedMS) >= mDataBlock->lifetime * 1000.0f)
      deleteObject();
}

/// 0x0069d9b0: what the client draws by, from when its ghost arrived.
void StationFXVehicle::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);
   mElapsed += dt;
}

void StationFXVehicle::prepRenderImage(SceneRenderState *state)
{
   if(!mDataBlock || !mStation || !state->isDiffusePass())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &StationFXVehicle::renderObject);
   // The bin the port's other immediate-mode effects draw in.
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// 0x0069daa0: the four bands between the arms, the ring, then the dome,
/// all added into the scene without writing depth and all placed by the
/// station's nodes as its activate thread moves them.
void StationFXVehicle::renderObject(ObjectRenderInst *, SceneRenderState *state,
                                    BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   ShapeBase *station = dynamic_cast<ShapeBase *>(mStation.getObject());
   TSShapeInstance *shape = station ? station->getShapeInstance() : NULL;
   if(!shape)
      return;

   if(mBandStateBlock.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setZReadWrite(true, false);
      desc.setCullMode(GFXCullNone);
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mBandStateBlock = GFX->createStateBlock(desc);

      // Retail culls the dome alone (0x0069e0b9), so it shows only from
      // outside.
      desc.setCullMode(GFXCullCCW);
      desc.samplersDefined = false;
      mDomeStateBlock = GFX->createStateBlock(desc);
   }

   shape->animate();

   // A bin leaves the scene's camera in its matrix set and the device's
   // view at identity, which the generic shader would draw through.
   GFXTransformSaver saver;
   MatrixSet &matrices = state->getRenderPass()->getMatrixSet();
   GFX->setViewMatrix(matrices.getWorldToCamera());
   GFX->setProjectionMatrix(matrices.getCameraToScreen());

   const StationFXVehicleData *data = mDataBlock;
   const MatrixF &stationMat = station->getRenderTransform();

   F32 radius[StationFXVehicleData::NumArmNodes];
   for(U32 i = 0; i < StationFXVehicleData::NumArmNodes; i++)
      radius[i] = armRadius(shape, data->leftNodeName[i], data->rightNodeName[i]);

   // Each band runs half way round from its own arm's upper node, the left
   // one round one side and the right one round the other, so each pair of
   // them closes a ring between two heights.
   renderArmLight(shape, stationMat, radius[1], radius[0], 180.0f,
                  data->leftNodeName[1], data->leftNodeName[0]);
   renderArmLight(shape, stationMat, radius[1], radius[0], 180.0f,
                  data->rightNodeName[1], data->rightNodeName[0]);
   renderArmLight(shape, stationMat, radius[3], radius[2], 180.0f,
                  data->leftNodeName[3], data->leftNodeName[2]);
   renderArmLight(shape, stationMat, radius[3], radius[2], 180.0f,
                  data->rightNodeName[3], data->rightNodeName[2]);

   renderGlow(shape, stationMat);
   renderSphere(shape, stationMat);
}

/// 0x0069e480: a band of texture[1] stood round the glow node, from the
/// bottom node's height up to the top node's, starting at the top node and
/// sweeping half way round. It grows round as it fades in after
/// armLightDelay and draws back as it fades out at the end of
/// armLightLifetime.
void StationFXVehicle::renderArmLight(TSShapeInstance *shape,
                                      const MatrixF &station,
                                      F32 topRadius, F32 bottomRadius,
                                      F32 sweep, const char *topNode,
                                      const char *bottomNode)
{
   const StationFXVehicleData *data = mDataBlock;
   if(mElapsed < data->armLightDelay || data->numArcSegments < 2 ||
      data->texture[1].isNull())
      return;

   const F32 fade = data->armLightFadeTime;
   const F32 t = mElapsed - data->armLightDelay;
   F32 alpha = 1.0f;
   if(t < fade)
   {
      alpha = t / fade;
      sweep *= alpha;
   }
   const F32 left = data->armLightLifetime - t;
   if(left < fade)
   {
      alpha = left / fade;
      sweep *= alpha;
   }
   // Past its lifetime retail still draws it, at an alpha GL clamps to
   // nothing.
   if(alpha <= 0.0f)
      return;

   const Point3F top = nodeTransform(shape, topNode).getPosition();
   const Point3F bottom = nodeTransform(shape, bottomNode).getPosition();
   MatrixF glow = nodeTransform(shape, data->glowNodeName);
   const Point3F centre = glow.getPosition();

   // Facing the top node across the pad, level, at the bottom node's height.
   Point3F dir(top.x - centre.x, top.y - centre.y, 0.0f);
   dir.normalizeSafe();
   glow.setPosition(Point3F(centre.x, centre.y, bottom.z));

   MatrixF local;
   local.mul(glow, MathUtils::createOrientFromDir(dir));
   MatrixF world;
   world.mul(station, local);
   GFX->setWorldMatrix(world);

   GFX->setStateBlock(mBandStateBlock);
   GFX->setTexture(0, data->texture[1]);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

   const F32 height = top.z - bottom.z;
   const F32 angle = mDegToRad(sweep);
   const U32 count = U32(data->numArcSegments);

   PrimBuild::begin(GFXTriangleStrip, count * 2);
   PrimBuild::color4f(1.0f, 1.0f, 1.0f, alpha);
   for(U32 i = 0; i < count; i++)
   {
      F32 u = F32(i) / F32(count - 1);
      const F32 a = angle * u;
      // The texture runs white to black along the band, so its far end
      // stops short of wrapping back to white.
      if(u == 1.0f)
         u = 0.97f;

      const F32 s = mSin(a);
      const F32 c = mCos(a);
      PrimBuild::texCoord2f(u, 0.025f);
      PrimBuild::vertex3f(topRadius * s, topRadius * c, height);
      PrimBuild::texCoord2f(u, 1.0f);
      PrimBuild::vertex3f(bottomRadius * s, bottomRadius * c, 0.0f);
   }
   PrimBuild::end(false);
}

/// 0x0069ea10: a ring of texture[0] round the glow node, from
/// glowBottomRadius at glowBottomHeight to glowTopRadius at glowTopHeight.
/// It fades in over glowFadeTime from the start and out over the same time
/// before lifetime, the fade out winning where the two meet.
void StationFXVehicle::renderGlow(TSShapeInstance *shape, const MatrixF &station)
{
   const StationFXVehicleData *data = mDataBlock;
   if(data->numGlowSegments < 2 || data->texture[0].isNull())
      return;

   const F32 fade = data->glowFadeTime;
   F32 alpha = 1.0f;
   if(mElapsed < fade)
      alpha = mElapsed / fade;
   const F32 left = data->lifetime - mElapsed;
   if(left < fade)
      alpha = left / fade;
   if(alpha <= 0.0f)
      return;

   MatrixF world;
   world.mul(station, nodeTransform(shape, data->glowNodeName));
   GFX->setWorldMatrix(world);

   GFX->setStateBlock(mBandStateBlock);
   GFX->setTexture(0, data->texture[0]);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

   const U32 count = U32(data->numGlowSegments);

   PrimBuild::begin(GFXTriangleStrip, count * 2);
   PrimBuild::color4f(1.0f, 1.0f, 1.0f, alpha);
   for(U32 i = 0; i < count; i++)
   {
      const F32 u = F32(i) / F32(count - 1);
      const F32 a = u * M_2PI_F;
      const F32 s = mSin(a);
      const F32 c = mCos(a);
      PrimBuild::texCoord2f(u, 0.05f);
      PrimBuild::vertex3f(data->glowTopRadius * s, data->glowTopRadius * c,
                          data->glowTopHeight);
      PrimBuild::texCoord2f(u, 1.0f);
      PrimBuild::vertex3f(data->glowBottomRadius * s,
                          data->glowBottomRadius * c, data->glowBottomHeight);
   }
   PrimBuild::end(false);
}

/// 0x0069df10: the upper half of a sphere of sphereRadius on the glow node,
/// stretched by sphereScale, untextured. It fades in over armLightFadeTime
/// from armLightDelay and out over the same time before the arm lights'
/// lifetime ends, on the bands' clock.
void StationFXVehicle::renderSphere(TSShapeInstance *shape, const MatrixF &station)
{
   const StationFXVehicleData *data = mDataBlock;
   if(data->spherePhiSegments < 2 || data->sphereThetaSegments < 2)
      return;

   const F32 fade = data->armLightFadeTime;
   const F32 t = mElapsed - data->armLightDelay;
   F32 alpha = 1.0f;
   if(t < fade)
      alpha = getMax(t / fade, 0.0f);
   const F32 left = data->armLightLifetime - t;
   if(left < fade)
      alpha = getMax(left / fade, 0.0f);
   if(alpha <= 0.0f)
      return;

   MatrixF world;
   world.mul(station, nodeTransform(shape, data->glowNodeName));
   world.scale(data->sphereScale);
   GFX->setWorldMatrix(world);

   GFX->setStateBlock(mDomeStateBlock);
   GFX->setupGenericShaders(GFXDevice::GSColor);

   const F32 r = data->sphereRadius;
   const U32 rings = U32(data->sphereThetaSegments);
   const U32 points = U32(data->spherePhiSegments);

   // A strip for each band of latitude, from the rim up to the crown.
   for(U32 i = 1; i < rings; i++)
   {
      const F32 upper = F32(i) / F32(rings - 1) * M_HALFPI_F;
      const F32 lower = F32(i - 1) / F32(rings - 1) * M_HALFPI_F;
      const F32 upperZ = r * mSin(upper);
      const F32 lowerZ = r * mSin(lower);
      const F32 upperR = r * mCos(upper);
      const F32 lowerR = r * mCos(lower);

      PrimBuild::begin(GFXTriangleStrip, points * 2);
      PrimBuild::color4f(sDomeColor.red, sDomeColor.green, sDomeColor.blue,
                         alpha);
      for(U32 j = 0; j < points; j++)
      {
         const F32 a = F32(j) / F32(points - 1) * M_2PI_F;
         const F32 s = mSin(a);
         const F32 c = mCos(a);
         PrimBuild::vertex3f(upperR * s, upperR * c, upperZ);
         PrimBuild::vertex3f(lowerR * s, lowerR * c, lowerZ);
      }
      PrimBuild::end(false);
   }
}
