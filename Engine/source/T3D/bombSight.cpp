// HH: Added for Tribes 2 compatibility.
#include "T3D/bombSight.h"

#include "T3D/objectTypes.h"
#include "core/resourceManager.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStateBlock.h"
#include "gfx/gfxTransformSaver.h"
#include "materials/baseMatInstance.h"
#include "materials/baseMaterialDefinition.h"
#include "materials/materialFeatureTypes.h"
#include "renderInstance/renderMeshMgr.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderTranslucentMgr.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "sim/netConnection.h"
#include "ts/instancingMatHook.h"
#include "ts/tsMaterialList.h"
#include "ts/tsRenderState.h"
#include "ts/tsShapeInstance.h"

IMPLEMENT_CONOBJECT(BombSight);

/// A material instance for one of the sight's slots, set up the way retail's
/// renderObject (0x00677850) leaves the GL state for it.
static BaseMatInstance *asRetailDraws(BaseMatInstance *from, U32 flags, bool instanced)
{
   FeatureSet features(from->getRequestedFeatures());
   if(instanced)
      features.addFeature(MFT_UseInstancing);

      // It installs no lights and sets up no fog of its own, so a Tribes 2
      // client draws the texture as it is; Torque 3D would light it and mix
      // its fog colour into what is added.
   features.removeFeature(MFT_RTLighting);
   features.removeFeature(MFT_Fog);

   GFXStateBlockDesc desc;

      // Depth test off (glDisable at 0x006778b8), so walls and rises between
      // it and the eye never hide it.
   desc.setZReadWrite(false, false);

      // Tribes 2 adds an additive material scaled by its alpha
      // (glBlendFunc(GL_SRC_ALPHA, GL_ONE)), which is how the sight fades.
   if(flags & TSMaterialList::Additive)
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);

   BaseMatInstance *inst = from->getMaterial()->createMatInstance();
   inst->addStateBlockDesc(desc);
   if(!inst->init(features, from->getVertexFormat()))
      SAFE_DELETE(inst);
   return inst;
}

/// Answers InstancingMaterialHook::getInstancingMat with a material of ours.
class InstancedAsRetailDraws : public InstancingMaterialHook
{
public:
   InstancedAsRetailDraws(BaseMatInstance *inst) { mMatInst = inst; }
};

/// TSMesh::innerRender draws a mesh this small through an instancing copy of
/// its material, made fresh from the material and so drawn like any other;
/// the copy is made here instead.  A material re-init drops it, so this is
/// asked again each frame.
static void hookInstanced(BaseMatInstance *inst, U32 flags)
{
   if(!inst || inst->getHook<InstancingMaterialHook>())
      return;

   if(BaseMatInstance *instanced = asRetailDraws(inst, flags, true))
      inst->addHook(new InstancedAsRetailDraws(instanced));
}

//-----------------------------------------------------------------------------
/// Marks the sight's meshes for RenderBombSightMgr (TSRenderState's material
/// hint, which only forest wind otherwise reads).
static U8 sSightHint;

/// Retail submits the sight with sortType EndSort (0x0067782d), drawn after
/// every translucent in the scene.  This bin draws it after Torque 3D's
/// translucent bin, so nothing a translucent puts near the target covers it.
class RenderBombSightMgr : public RenderMeshMgr
{
   typedef RenderMeshMgr Parent;

public:
   RenderBombSightMgr() : Parent(RenderPassManager::RIT_Translucent, 1.f, 1.f) {}

   DECLARE_CONOBJECT(RenderBombSightMgr);

   void addElement(RenderInst *inst)
   {
      if(static_cast<MeshRenderInst*>(inst)->materialHint != &sSightHint)
         return;

         // Taken first, so the translucent bin, told of it next, skips it.
      inst->translucentSort = false;
      internalAddElement(inst);
   }
};

IMPLEMENT_CONOBJECT(RenderBombSightMgr);

static SimObjectPtr<RenderBombSightMgr> sSightBin;

/// Put the bin in the diffuse pass, drawing just after the translucent bin and
/// told of a translucent mesh just before it.
static void installSightBin()
{
   if(!sSightBin.isNull() || !gClientSceneGraph)
      return;

   RenderPassManager *pass = gClientSceneGraph->getDefaultRenderPass();
   F32 renderOrder = 1.45f, addOrder = 1.35f;
   for(U32 i = 0; i < pass->getManagerCount(); i++)
      if(RenderTranslucentMgr *translucent = dynamic_cast<RenderTranslucentMgr*>(pass->getManager(i)))
      {
         renderOrder = translucent->getRenderOrder() + 0.05f;
         addOrder = translucent->getProcessAddOrder() - 0.05f;
      }

   RenderBombSightMgr *bin = new RenderBombSightMgr;
   bin->setRenderOrder(renderOrder);
   bin->setProcessAddOrder(addOrder);
   if(!bin->registerObject())
   {
      delete bin;
      return;
   }

   pass->addManager(bin);
   sSightBin = bin;
}

//-----------------------------------------------------------------------------
BombSight::BombSight()
{
      // Retail marks it a projectile (0x006775a7).  Torque 3D draws only what
      // DEFAULT_RENDER_TYPEMASK names, so it is a dynamic shape too; it has no
      // collision, so no query that finds it by that can stop at it.
   mTypeMask |= ProjectileObjectType | DynamicShapeObjectType;

      // Made on the client for the client alone.
   mNetFlags.set(IsGhost);

   mShapeName = NULL;
   mShapeInstance = NULL;
   mShown = false;
   mAlpha = 1.f;
}

BombSight::~BombSight()
{
   SAFE_DELETE(mShapeInstance);
}

void BombSight::setShapeName(const char *name)
{
   mShapeName = StringTable->insert(name ? name : "");
}

void BombSight::setShown(bool shown, F32 alpha)
{
   mShown = shown;
   mAlpha = alpha;
}

bool BombSight::onAdd()
{
   if(!Parent::onAdd())
      return false;

      // Always under shapes/, whatever the name already says (0x00677678).
   char path[256];
   dSprintf(path, sizeof(path), "shapes/%s", mShapeName ? mShapeName : "");

   mShape = ResourceManager::get().load(path);
   if(!bool(mShape))
   {
         // Retail goes on with no shape and faults on its first use of it.
      Con::errorf("BombSight: couldn't load shape \"%s\"", path);
      return false;
   }

   if(!mShape->preloadMaterialList(mShape.getPath()) &&
      NetConnection::filesWereDownloaded())
      return false;

   mShapeInstance = new TSShapeInstance(mShape, true);

      // An ifl slot only points at one of its frames' instances, so it is let
      // go while the instances are swapped and pointed again after.
   mShapeInstance->cloneMaterialList();
   mShapeInstance->releaseIflMaterials();

   TSMaterialList *materials = mShapeInstance->getMaterialList();
   for(U32 i = 0; i < materials->size(); i++)
   {
      BaseMatInstance *shared = materials->getMaterialInst(i);
      BaseMatInstance *own = shared ? asRetailDraws(shared, materials->getFlags(i), false) : NULL;
      if(!own)
         continue;

      materials->setMaterialInst(own, i);
      delete shared;
   }

   mShapeInstance->bindIflMaterials();

   installSightBin();

   mObjBox = mShape->bounds;
   resetWorldBox();

   addToScene();
   return true;
}

void BombSight::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

void BombSight::prepRenderImage(SceneRenderState *state)
{
   if(!mShown || !mShapeInstance || !state->isDiffusePass())
      return;

   TSMaterialList *materials = mShapeInstance->getMaterialList();
   for(U32 i = 0; i < materials->size(); i++)
      hookInstanced(materials->getMaterialInst(i), materials->getFlags(i));

   GFXTransformSaver saver;

   TSRenderState rdata;
   rdata.setSceneState(state);
   rdata.setMaterialHint(&sSightHint);

   MatrixF mat = getRenderTransform();
   mat.scale(mObjScale);
   GFX->setWorldMatrix(mat);

      // Retail draws the top detail at every range (0x006778f5) and never
      // asks the distance for one, which would drop a shape this small long
      // before a bomber is high enough to want it.
   mShapeInstance->setCurrentDetail(0);
   mShapeInstance->animate();

      // Faded as a whole, and blended only below 0.99 (0x006778c4).
   mShapeInstance->setAlphaAlways(mAlpha);
   mShapeInstance->render(rdata);
}
