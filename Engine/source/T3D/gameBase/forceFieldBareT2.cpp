// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/forceFieldBareT2.h"

#include "collision/boxConvex.h"
#include "collision/earlyOutPolyList.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/objectTypes.h"
#include "T3D/player.h"

IMPLEMENT_CO_DATABLOCK_V1(ForceFieldBareData);

ConsoleDocClass( ForceFieldBareData,
   "@brief What a Tribes 2 force field looks like and who it lets "
   "through.\n\n@internal" );

/// The defaults the constructor at 0x00675020 sets, which are not the ones
/// forceField.cs gives every field it declares.
ForceFieldBareData::ForceFieldBareData()
   : fadeMS(1000),
     baseTranslucency(0.65f),
     powerOffTranslucency(0.35f),
     teamPermiable(false),
     otherPermiable(false),
     color(0.5f, 0.5f, 1.0f, 1.0f),
     powerOffColor(0.25f, 0.0f, 0.0f, 1.0f),
     framesPerSec(10),
     numFrames(5),
     scrollSpeed(0.5f),
     umapping(1.0f),
     vmapping(1.0f)
{
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = StringTable->EmptyString();
}

void ForceFieldBareData::initPersistFields()
{
   addField("fadeMS",               TypeS32,    Offset(fadeMS,               ForceFieldBareData));
   addField("baseTranslucency",     TypeF32,    Offset(baseTranslucency,     ForceFieldBareData));
   addField("powerOffTranslucency", TypeF32,    Offset(powerOffTranslucency, ForceFieldBareData));
   addField("teamPermiable",        TypeBool,   Offset(teamPermiable,        ForceFieldBareData));
   addField("otherPermiable",       TypeBool,   Offset(otherPermiable,       ForceFieldBareData));
   addField("color",                TypeColorF, Offset(color,                ForceFieldBareData));
   addField("powerOffColor",        TypeColorF, Offset(powerOffColor,        ForceFieldBareData));
   addField("texture",              TypeString, Offset(textureName,          ForceFieldBareData), NumTextures);
   addField("framesPerSec",         TypeS32,    Offset(framesPerSec,         ForceFieldBareData));
   addField("numFrames",            TypeS32,    Offset(numFrames,            ForceFieldBareData));
   addField("scrollSpeed",          TypeF32,    Offset(scrollSpeed,          ForceFieldBareData));
   addField("umapping",             TypeF32,    Offset(umapping,             ForceFieldBareData));
   addField("vmapping",             TypeF32,    Offset(vmapping,             ForceFieldBareData));

   Parent::initPersistFields();
}

/// The two checks at 0x006752e0, with their messages and their clamps.
bool ForceFieldBareData::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if(fadeMS < 0 || fadeMS > 10000)
   {
      Con::errorf("ForceFieldBareData(%s)::onAdd: fadeMS must be in the "
                  "range [0, 10000]", getName());
      fadeMS = fadeMS < 0 ? 0 : 10000;
   }

   if(baseTranslucency < 0.0f || baseTranslucency > 1.0f)
   {
      Con::errorf("ForceFieldBareData(%s)::onAdd: baseTranslucency must be in "
                  "the range [0, 1]", getName());
      baseTranslucency = baseTranslucency < 0.0f ? 0.0f : 1.0f;
   }

   return true;
}

bool ForceFieldBareData::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(server)
      return true;

   for(U32 i = 0; i < NumTextures; i++)
      if(textureName[i] && textureName[i][0])
         texture[i].set(String("textures/") + textureName[i],
                        &GFXDefaultStaticDiffuseProfile, "ForceFieldBareData");

   return true;
}

static ColorF colorOfWord(U32 word)
{
   return ColorF(F32(word & 0xff) / 255.0f,
                 F32((word >> 8) & 0xff) / 255.0f,
                 F32((word >> 16) & 0xff) / 255.0f,
                 F32((word >> 24) & 0xff) / 255.0f);
}

bool ForceFieldBareData::unpackData(BitStream *stream, U32 classId)
{
   if(!Parent::unpackData(stream, classId))
      return false;

   F32 value;
   U32 word;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldFadeMS, &word))
      fadeMS = S32(word);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFieldBaseTranslucency, &value))
      baseTranslucency = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFieldPowerOffTranslucency, &value))
      powerOffTranslucency = value;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldTeamPermiable, &word))
      teamPermiable = word != 0;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldOtherPermiable, &word))
      otherPermiable = word != 0;
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldColor, &word))
      color = colorOfWord(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldPowerOffColor, &word))
      powerOffColor = colorOfWord(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldFramesPerSec, &word))
      framesPerSec = S32(word);
   if(T2Layout::lastDataBlockWord(T2Layout::T2DbFieldNumFrames, &word))
      numFrames = S32(word);
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFieldScrollSpeed, &value))
      scrollSpeed = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFieldUMapping, &value))
      umapping = value;
   if(T2Layout::lastDataBlockField(T2Layout::T2DbFieldVMapping, &value))
      vmapping = value;

   const Vector<String> &strings = T2Layout::lastStrings();
   for(U32 i = 0; i < NumTextures; i++)
      textureName[i] = StringTable->insert(i < strings.size()
                                              ? strings[i].c_str() : "");

   return true;
}

IMPLEMENT_CO_NETOBJECT_V1(ForceFieldBare);

ConsoleDocClass( ForceFieldBare,
   "@brief A Tribes 2 force field.\n\n@internal" );

/// A tick is 32 milliseconds of fade, which is what both tick functions at
/// 0x006769a0 and 0x00676a50 add or take away.
static const S32 sFadeTickMS = 32;

ForceFieldBare::ForceFieldBare()
   : mDataBlock(NULL),
     mState(Closed),
     mFadeTime(0),
     mLastFadeTime(0),
     mFade(1.0f),
     mAnimTime(0.0f),
     mT2Target(-1),
     mConvexList(new Convex)
{
   mNetFlags.set(Ghostable);
   mTypeMask |= ForceFieldObjectType;
}

ForceFieldBare::~ForceFieldBare()
{
   delete mConvexList;
   mConvexList = NULL;
}

/// The unit box at 0x00675850: the field's scale is its size, and its
/// position is a corner.
bool ForceFieldBare::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(0.0f, 0.0f, 0.0f);
   mObjBox.maxExtents.set(1.0f, 1.0f, 1.0f);
   resetWorldBox();
   addToScene();

   scriptOnAdd();
   return true;
}

void ForceFieldBare::onRemove()
{
   scriptOnRemove();
   mConvexList->nukeList();
   removeFromScene();
   Parent::onRemove();
}

bool ForceFieldBare::onNewDataBlock(GameBaseData *dptr, bool reload)
{
   mDataBlock = dynamic_cast<ForceFieldBareData *>(dptr);
   if(!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   scriptOnNewDataBlock();
   return true;
}

/// ForceFieldBare::unpackUpdate (0x00676d30) reads the state after GameBase
/// has named the datablock, and a field that arrives open or closed has the
/// fade time that state means rather than one off the wire.
bool ForceFieldBare::unpackUpdate(BitStream *stream, bool isInitial)
{
   if(!Parent::unpackUpdate(stream, isInitial))
      return false;

   const T2Layout::Body &body = T2Layout::lastBody();
   if(body.hasTarget)
      mT2Target = body.target;

   if(body.hasFieldState)
   {
      mState = body.fieldState;
      if(mState == Closed)
         mFadeTime = 0;
      else if(mState == Open)
         mFadeTime = mDataBlock ? mDataBlock->fadeMS : 0;
      else
         mFadeTime = S32(body.fieldFade);
   }

   return true;
}

void ForceFieldBare::tickFade()
{
   switch(mState)
   {
      case Open:
      case Closed:
         return;

      case Opening:
         mFadeTime += sFadeTickMS;
         if(mFadeTime >= (mDataBlock ? mDataBlock->fadeMS : 0))
         {
            mState = Open;
            mFadeTime = mDataBlock ? mDataBlock->fadeMS : 0;
         }
         return;

      case Closing:
         mFadeTime -= sFadeTickMS;
         if(mFadeTime <= 0)
         {
            mState = Closed;
            mFadeTime = 0;
         }
         return;

      default:
         Con::errorf("Bad client state on forcefield!  Setting to closed");
         mState = Closed;
         mFadeTime = 0;
         if(isServerObject())
            setMaskBits(InitialUpdateMask);
         return;
   }
}

void ForceFieldBare::processTick(const Move *move)
{
   Parent::processTick(move);

   mLastFadeTime = mFadeTime;
   tickFade();
}

/// 0x00676b70 as it is: the fade between the last tick and this one, run
/// from the last tick at a delta of nought.
void ForceFieldBare::interpolateTick(F32 delta)
{
   Parent::interpolateTick(delta);

   const S32 fadeMS = mDataBlock ? mDataBlock->fadeMS : 0;
   if(fadeMS <= 0)
   {
      mFade = mState == Open ? 0.0f : 1.0f;
      return;
   }

   const F32 time = F32(mLastFadeTime) + F32(mFadeTime - mLastFadeTime) * delta;
   mFade = getMax(1.0f - time / F32(fadeMS), 0.0f);
}

void ForceFieldBare::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);
   mAnimTime += dt;
}

/// 0x00675950. An open from part way through a close carries on from where
/// the close had got to.
void ForceFieldBare::open()
{
   if(mState == Opening || mState == Open)
      return;

   mState = Opening;
   setMaskBits(StateMask);
}

/// 0x00675980.
void ForceFieldBare::close()
{
   if(mState == Closing || mState == Closed)
      return;

   mState = Closing;
   setMaskBits(StateMask);
}

U32 ForceFieldBare::getSensorGroup() const
{
   return T2Targets::sensorGroup(mT2Target);
}

static U32 sensorGroupOf(SceneObject *obj)
{
   if(Player *player = dynamic_cast<Player *>(obj))
      return T2Targets::sensorGroup(player->getT2Target());
   if(ForceFieldBare *field = dynamic_cast<ForceFieldBare *>(obj))
      return field->getSensorGroup();
   return 0;
}

/// 0x00677050, which Player asks of every field its collision meets. A
/// vehicle never gets through; anything else does when the field is open, or
/// when the datablock lets its team through.
bool ForceFieldBare::isPermeableTo(SceneObject *obj) const
{
   if(obj->getTypeMask() & VehicleObjectType)
      return false;

   if(mState == Open)
      return true;

   if(!mDataBlock)
      return false;

   const bool sameTeam = sensorGroupOf(obj) == getSensorGroup();
   if(mDataBlock->otherPermiable && !sameTeam)
      return true;
   if(mDataBlock->teamPermiable && sameTeam)
      return true;

   return false;
}

/// 0x00676ee0: one box convex for the whole field, whatever its state, since
/// the state and the team are the player's to weigh.
void ForceFieldBare::buildConvex(const Box3F &box, Convex *convex)
{
   mConvexList->collectGarbage();

   if(!box.isOverlapped(getWorldBox()))
      return;

   CollisionWorkingList &wl = convex->getWorkingList();
   for(CollisionWorkingList *itr = wl.wLink.mNext; itr != &wl;
       itr = itr->wLink.mNext)
      if(itr->mConvex->getType() == BoxConvexType &&
         itr->mConvex->getObject() == this)
         return;

   BoxConvex *cp = new BoxConvex;
   mConvexList->registerObject(cp);
   convex->addToWorkingList(cp);
   cp->init(this);
   cp->mCenter.set(0.5f, 0.5f, 0.5f);
   cp->mSize.set(0.5f, 0.5f, 0.5f);
}

/// 0x00676850 adds the box whatever the state. Torque 3D also asks for
/// polygons to put decals and navigation on, which Tribes 2 never did of a
/// field, so only collision gets them.
bool ForceFieldBare::buildPolyList(PolyListContext context,
                                   AbstractPolyList *polyList,
                                   const Box3F &, const SphereF &)
{
   if(context != PLC_Collision)
      return false;

   polyList->setTransform(&mObjToWorld, mObjScale);
   polyList->setObject(this);
   polyList->addBox(mObjBox);
   return true;
}

/// 0x00676900: an open field stops nothing; any other stops a ray whoever
/// cast it.
bool ForceFieldBare::castRay(const Point3F &start, const Point3F &end,
                             RayInfo *info)
{
   if(mState == Open)
      return false;

   F32 t;
   Point3F normal;
   if(!mObjBox.collideLine(start, end, &t, &normal))
      return false;

   mObjToWorld.mulV(normal);
   normal.normalizeSafe();

   info->t = t;
   info->normal = normal;
   info->object = this;
   return true;
}

void ForceFieldBare::prepRenderImage(SceneRenderState *state)
{
   if(!mDataBlock || !state->isDiffusePass())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &ForceFieldBare::renderObject);
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst(ri);
}

static F32 fogBetween(const FogData &fog, const Point3F &eye,
                      const Point3F &at)
{
   const F32 dist = (at - eye).len();

   F32 t = mClampF((dist - fog.densityOffset) * fog.hazeScale, 0.0f, 1.0f) - 1.0f;
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

/// ForceFieldBare::renderObject (0x00676050): no lighting, no culling, no
/// depth write, added into the scene with its alpha, its colour and alpha
/// the power off ones faded towards the power on ones, and both fogged by
/// the distance to its corner.
void ForceFieldBare::renderObject(ObjectRenderInst *, SceneRenderState *state,
                                  BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   const ForceFieldBareData *data = mDataBlock;
   const F32 on = mFade;
   const F32 off = 1.0f - on;

   ColorF c;
   c.red   = data->powerOffColor.red   * off + data->color.red   * on;
   c.green = data->powerOffColor.green * off + data->color.green * on;
   c.blue  = data->powerOffColor.blue  * off + data->color.blue  * on;
   c.alpha = data->powerOffTranslucency * off + data->baseTranslucency * on;

   const FogData &fogData = state->getSceneManager()->getFogData();
   const F32 fog = fogBetween(fogData, state->getCameraPosition(),
                              getRenderPosition());
   c.red   = c.red   * (1.0f - fog) + fogData.color.red   * fog;
   c.green = c.green * (1.0f - fog) + fogData.color.green * fog;
   c.blue  = c.blue  * (1.0f - fog) + fogData.color.blue  * fog;
   c.alpha = c.alpha * (1.0f - fog);

   if(c.alpha <= 0.0f)
      return;

   GFXTexHandle *tex = NULL;
   if(data->numFrames > 0)
   {
      const U32 frame = U32(U64(F64(data->framesPerSec) * mAnimTime)) %
                        U32(data->numFrames);
      if(frame < ForceFieldBareData::NumTextures &&
         data->texture[frame].isValid())
         tex = const_cast<GFXTexHandle *>(&data->texture[frame]);
   }

   if(mStateBlock.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setCullMode(GFXCullNone);
      desc.zWriteEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mStateBlock = GFX->createStateBlock(desc);
   }

   GFXTransformSaver saver;

   MatrixF mat = getRenderTransform();
   mat.scale(getScale());
   GFX->multWorld(mat);

   GFX->setStateBlock(mStateBlock);
   if(tex)
   {
      GFX->setTexture(0, *tex);
      GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
   }
   else
      GFX->setupGenericShaders(GFXDevice::GSColor);

   const Point3F &scale = getScale();
   const F32 scroll = mAnimTime * data->scrollSpeed;

   struct Face { U32 fixed; F32 at; U32 u, v; };
   static const Face faces[6] =
   {
      { 2, 0.0f, 0, 1 },
      { 2, 1.0f, 0, 1 },
      { 0, 1.0f, 1, 2 },
      { 0, 0.0f, 1, 2 },
      { 1, 1.0f, 0, 2 },
      { 1, 0.0f, 0, 2 },
   };
   static const F32 corners[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

   for(U32 f = 0; f < 6; f++)
   {
      const Face &face = faces[f];
      const F32 uLen = scale[face.u] * data->umapping;
      const F32 vLen = scale[face.v] * data->vmapping;

      PrimBuild::begin(GFXTriangleFan, 4);
      PrimBuild::color4f(c.red, c.green, c.blue, c.alpha);
      for(U32 i = 0; i < 4; i++)
      {
         Point3F p;
         p[face.fixed] = face.at;
         p[face.u] = corners[i][0];
         p[face.v] = corners[i][1];

         PrimBuild::texCoord2f(corners[i][0] * uLen,
                               scroll + corners[i][1] * vLen);
         PrimBuild::vertex3fv(p);
      }
      PrimBuild::end(false);
   }
}

DefineEngineMethod( ForceFieldBare, open, void, (),,
   "Let everything through, after the datablock's fadeMS.\n\n@internal" )
{
   if(object->isServerObject())
      object->open();
}

DefineEngineMethod( ForceFieldBare, close, void, (),,
   "Stop everything the datablock does not let through, after its "
   "fadeMS.\n\n@internal" )
{
   if(object->isServerObject())
      object->close();
}

/// ShapeBase::isInForceField (0x005f32b0): the first field not open whose
/// box meets the object's world box, or nought.
DefineEngineMethod( ShapeBase, isInForceField, S32, (),,
   "The force field the object is inside, or 0.\n\n@internal" )
{
   SceneContainer *container = object->getContainer();
   if(!container)
      return 0;

   const Box3F &wBox = object->getWorldBox();

   Vector<SceneObject *> found;
   container->findObjectList(wBox, ForceFieldObjectType, &found);

   for(U32 i = 0; i < found.size(); i++)
   {
      ForceFieldBare *field = dynamic_cast<ForceFieldBare *>(found[i]);
      if(!field || field->getState() == ForceFieldBare::Open)
         continue;

      EarlyOutPolyList polyList;
      polyList.mNormal.set(0.0f, 0.0f, 0.0f);
      polyList.mPlaneList.clear();
      polyList.mPlaneList.setSize(6);
      polyList.mPlaneList[0].set(wBox.minExtents, VectorF(-1.0f, 0.0f, 0.0f));
      polyList.mPlaneList[1].set(wBox.maxExtents, VectorF( 0.0f, 1.0f, 0.0f));
      polyList.mPlaneList[2].set(wBox.maxExtents, VectorF( 1.0f, 0.0f, 0.0f));
      polyList.mPlaneList[3].set(wBox.minExtents, VectorF( 0.0f,-1.0f, 0.0f));
      polyList.mPlaneList[4].set(wBox.minExtents, VectorF( 0.0f, 0.0f,-1.0f));
      polyList.mPlaneList[5].set(wBox.maxExtents, VectorF( 0.0f, 0.0f, 1.0f));

      field->buildPolyList(PLC_Collision, &polyList, wBox,
                           SphereF(Point3F(0.0f, 0.0f, 0.0f), 0.0f));
      if(!polyList.isEmpty())
         return field->getId();
   }

   return 0;
}
