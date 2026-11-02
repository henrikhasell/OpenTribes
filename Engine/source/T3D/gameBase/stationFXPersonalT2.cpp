// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/stationFXPersonalT2.h"

#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "math/util/matrixSet.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneRenderState.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/shapeBase.h"
#include "ts/tsShapeInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(StationFXPersonalData);

ConsoleDocClass( StationFXPersonalData,
   "@brief What a Tribes 2 inventory station shows as it fits a player "
   "out.\n\n@internal" );

/// The defaults the constructor at 0x0069bc40 sets, which are not the ones
/// station.cs gives PersonalInvFX.
StationFXPersonalData::StationFXPersonalData()
   : delay(0.0f),
     fadeDelay(1.5f),
     lifetime(2.0f),
     height(2.5f),
     leftRadius(2.0f),
     rightRadius(2.0f),
     numArcSegments(10),
     numDegrees(180.0f),
     trailFadeTime(0.2f)
{
   leftNodeName = StringTable->EmptyString();
   rightNodeName = StringTable->EmptyString();
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = StringTable->EmptyString();
}

void StationFXPersonalData::initPersistFields()
{
   addField("delay",          TypeF32,    Offset(delay,          StationFXPersonalData));
   addField("fadeDelay",      TypeF32,    Offset(fadeDelay,      StationFXPersonalData));
   addField("lifetime",       TypeF32,    Offset(lifetime,       StationFXPersonalData));
   addField("height",         TypeF32,    Offset(height,         StationFXPersonalData));
   addField("numArcSegments", TypeS32,    Offset(numArcSegments, StationFXPersonalData));
   addField("numDegrees",     TypeF32,    Offset(numDegrees,     StationFXPersonalData));
   addField("trailFadeTime",  TypeF32,    Offset(trailFadeTime,  StationFXPersonalData));
   addField("leftRadius",     TypeF32,    Offset(leftRadius,     StationFXPersonalData));
   addField("rightRadius",    TypeF32,    Offset(rightRadius,    StationFXPersonalData));
   addField("leftNodeName",   TypeString, Offset(leftNodeName,   StationFXPersonalData));
   addField("rightNodeName",  TypeString, Offset(rightNodeName,  StationFXPersonalData));
   addField("texture",        TypeString, Offset(textureName,    StationFXPersonalData), NumTextures);

   Parent::initPersistFields();
}

/// 0x0069be40: the textures, on a client only.
bool StationFXPersonalData::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(server)
      return true;

   for(U32 i = 0; i < NumTextures; i++)
      if(textureName[i] && textureName[i][0])
         texture[i].set(String("textures/") + textureName[i],
                        &GFXDefaultStaticDiffuseProfile, "StationFXPersonalData");

   return true;
}

static StringTableEntry stringAt(const Vector<String> &strings, U32 i)
{
   return StringTable->insert(i < strings.size() ? strings[i].c_str() : "");
}

/// 0x0069c010.
bool StationFXPersonalData::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationDelay, &value))
      delay = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationFadeDelay, &value))
      fadeDelay = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationLifetime, &value))
      lifetime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationHeight, &value))
      height = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationNumDegrees, &value))
      numDegrees = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationTrailFadeTime, &value))
      trailFadeTime = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationLeftRadius, &value))
      leftRadius = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbStationRightRadius, &value))
      rightRadius = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbStationNumArcSegments, &word))
      numArcSegments = S32(word);

   // The left node, the right one, then the textures.
   const Vector<String> &strings = T2Layout::lastStrings();
   U32 next = 0;
   leftNodeName = stringAt(strings, next++);
   rightNodeName = stringAt(strings, next++);
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = stringAt(strings, next++);

   return true;
}

IMPLEMENT_CO_NETOBJECT_V1(StationFXPersonal);

ConsoleDocClass( StationFXPersonal,
   "@brief The effect a Tribes 2 inventory station plays as it fits a "
   "player out.\n\n@internal" );

/// 0x0069c120.
StationFXPersonal::StationFXPersonal()
   : mDataBlock(NULL),
     mStationId(-1),
     mElapsedMS(0),
     mElapsed(0.0f)
{
}

/// 0x0069c170.
void StationFXPersonal::initPersistFields()
{
   addField("stationObject", TypeS32, Offset(mStationId, StationFXPersonal));

   Parent::initPersistFields();
}

/// Retail sets the server's object on its station (0x0069c1a0) and leaves
/// the client's at the origin, drawing from the station's transform either
/// way. Putting the client's there too keeps it in scope and lets the scene
/// cull it where it is drawn.
void StationFXPersonal::placeAtStation()
{
   // The box retail gives it, and the station's own, which holds the arms.
   Box3F box(Point3F(-3.0f, -3.0f, -3.0f), Point3F(3.0f, 3.0f, 3.0f));
   if(mStation)
   {
      box.intersect(mStation->getObjBox());
      setTransform(mStation->getTransform());
   }

   mObjBox = box;
   resetWorldBox();
}

bool StationFXPersonal::onAdd()
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

void StationFXPersonal::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

/// 0x0069c2d0.
bool StationFXPersonal::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mDataBlock = dynamic_cast<StationFXPersonalData *>(dptr);
   if(!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   scriptOnNewDataBlock();
   return true;
}

/// 0x0069c970: the station comes as a ghost index on the first body only,
/// and an effect whose station the client has not got draws nothing.
bool StationFXPersonal::unpackUpdate(BitStream *stream, bool isInitial)
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

/// 0x0069c330: the server's copy goes once its lifetime is up, which takes
/// the client's with it.
void StationFXPersonal::processTick(const Move *move)
{
   Parent::processTick(move);

   mElapsedMS += TickMs;
   if(isServerObject() && mDataBlock &&
      F32(mElapsedMS) >= mDataBlock->lifetime * 1000.0f)
      deleteObject();
}

/// 0x0069c390: what the client draws by, from when its ghost arrived.
void StationFXPersonal::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);
   mElapsed += dt;
}

void StationFXPersonal::prepRenderImage(SceneRenderState *state)
{
   if(!mDataBlock || !mStation || !state->isDiffusePass())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &StationFXPersonal::renderObject);
   // The bin the port's other immediate-mode effects draw in.
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

/// 0x0069c480: a band behind each arm, left then right, added into the
/// scene without writing depth.
void StationFXPersonal::renderObject(ObjectRenderInst *, SceneRenderState *state,
                                     BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   ShapeBase *station = dynamic_cast<ShapeBase *>(mStation.getObject());
   TSShapeInstance *shape = station ? station->getShapeInstance() : NULL;
   if(!shape || mDataBlock->texture[0].isNull())
      return;

   if(mStateBlock.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setZReadWrite(true, false);
      desc.setCullMode(GFXCullNone);
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mStateBlock = GFX->createStateBlock(desc);
   }

   shape->animate();

   // A bin leaves the scene's camera in its matrix set and the device's
   // view at identity, which the generic shader would draw through.
   GFXTransformSaver saver;
   MatrixSet &matrices = state->getRenderPass()->getMatrixSet();
   GFX->setViewMatrix(matrices.getWorldToCamera());
   GFX->setProjectionMatrix(matrices.getCameraToScreen());

   const MatrixF &stationMat = station->getRenderTransform();
   renderArc(shape, stationMat, mDataBlock->leftRadius, mDataBlock->leftNodeName);
   renderArc(shape, stationMat, mDataBlock->rightRadius, mDataBlock->rightNodeName);
}

/// 0x0069c540: a band of texture[0] height high, centred on the arm's node
/// and curving from it round a circle of the given radius, which is the
/// circle the arm spins on, so the band trails the arm. It grows to
/// numDegrees over trailFadeTime and shrinks back over the same time before
/// lifetime, and fades out from fadeDelay to lifetime.
void StationFXPersonal::renderArc(TSShapeInstance *shape, const MatrixF &station,
                                  F32 radius, const char *nodeName)
{
   const StationFXPersonalData *data = mDataBlock;
   if(data->numArcSegments < 2 || !nodeName || !nodeName[0])
      return;

   const S32 node = shape->getShape()->findNode(nodeName);
   if(node < 0 || node >= shape->mNodeTransforms.size())
      return;

   F32 alpha = 1.0f;
   if(mElapsed > data->fadeDelay)
      alpha = (data->lifetime - mElapsed) / (data->lifetime - data->fadeDelay);
   if(alpha <= 0.0f)
      return;

   F32 degrees = data->numDegrees;
   if(mElapsed < data->trailFadeTime)
      degrees *= mElapsed / data->trailFadeTime;
   const F32 left = data->lifetime - mElapsed;
   if(left < data->trailFadeTime)
      degrees *= left / data->trailFadeTime;

   MatrixF world;
   world.mul(station, shape->mNodeTransforms[node]);
   GFX->setWorldMatrix(world);

   GFX->setStateBlock(mStateBlock);
   GFX->setTexture(0, data->texture[0]);
   GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

   const F32 angle = mDegToRad(degrees);
   const F32 half = data->height * 0.5f;
   const U32 count = U32(data->numArcSegments);

   PrimBuild::begin(GFXTriangleStrip, count * 2);
   PrimBuild::color4f(1.0f, 1.0f, 1.0f, alpha);
   for(U32 i = 0; i < count; i++)
   {
      const F32 u = F32(i) / F32(count - 1);
      const F32 a = angle * u;
      const F32 x = radius * mSin(a);
      const F32 y = radius * mCos(a) - radius;
      PrimBuild::texCoord2f(u, 0.0f);
      PrimBuild::vertex3f(x, y, half);
      PrimBuild::texCoord2f(u, 1.0f);
      PrimBuild::vertex3f(x, y, -half);
   }
   PrimBuild::end(false);
}
