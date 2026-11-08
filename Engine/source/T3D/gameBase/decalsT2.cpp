// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/decalsT2.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/module.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "T3D/decal/decalData.h"
#include "T3D/decal/decalManager.h"
#include "T3D/objectTypes.h"

DecalManagerT2 *gDecalManagerT2 = NULL;

S32  DecalManagerT2::smMaxNumDecals = 256;
S32  DecalManagerT2::smDecalTimeout = 5000;

IMPLEMENT_CONOBJECT(DecalManagerT2);

ConsoleDocClass( DecalManagerT2,
   "@brief The marks Tribes 2 leaves on the world.\n\n@internal" );

MODULE_BEGIN( DecalManagerT2 )

   MODULE_INIT_AFTER( Scene )
   MODULE_SHUTDOWN_BEFORE( Scene )

   MODULE_INIT
   {
      gDecalManagerT2 = new DecalManagerT2;
      gClientSceneGraph->addObjectToScene(gDecalManagerT2);
   }

   MODULE_SHUTDOWN
   {
      gClientSceneGraph->removeObjectFromScene(gDecalManagerT2);
      SAFE_DELETE(gDecalManagerT2);
   }

MODULE_END;

DecalManagerT2::DecalManagerT2()
{
   // Everywhere at once, so no zone can cull the marks out of a frame.
   setGlobalBounds();
   mTypeMask |= EnvironmentObjectType;

   VECTOR_SET_ASSOCIATION(mDecals);
}

/// $pref::decalsOn is the decal manager Torque 3D came with, which carries
/// Tribes 2's name for it already; these two are Tribes 2's own (0x00593d80).
void DecalManagerT2::consoleInit()
{
   Con::addVariable("$pref::Decal::maxNumDecals", TypeS32, &smMaxNumDecals,
      "How many marks the world holds before the oldest goes.\n\n@ingroup Decals");
   Con::addVariable("$pref::Decal::decalTimeout", TypeS32, &smDecalTimeout,
      "How long a mark lasts, in milliseconds.\n\n@ingroup Decals");
}

//----------------------------------------------------------------------------

void DecalManagerT2::addDecal(const Point3F &pos, Point3F normal,
                              DecalData *data)
{
   if(smMaxNumDecals <= 0 || !data)
      return;

   if(mDecals.size() >= smMaxNumDecals)
      mDecals.pop_front();

   // Any pair of axes in the plane will do where nothing says which way up
   // the mark goes; retail takes them from the world's up, or from +y where
   // the surface is too near flat for that to give anything (0x00593e79).
   Point3F vecX;
   if(mFabs(normal.z) > 0.9f)
      vecX = mCross(normal, Point3F(0.0f, 1.0f, 0.0f));
   else
      vecX = mCross(normal, Point3F(0.0f, 0.0f, 1.0f));

   Point3F vecY = mCross(vecX, normal);

   normal.normalizeSafe();
   vecX.normalizeSafe();
   vecY.normalizeSafe();

   Decal decal;
   decal.data = data;
   decal.born = Platform::getVirtualMilliseconds();
   decal.fade = 1.0f;

   const Point3F at = pos + normal * 0.008f;
   const Point3F x = vecX * data->t2SizeX;
   const Point3F y = vecY * data->t2SizeY;

   decal.point[0] = at + x + y;
   decal.point[1] = at + x - y;
   decal.point[2] = at - x - y;
   decal.point[3] = at - x + y;

   mDecals.push_back(decal);
}

void DecalManagerT2::addDecal(const Point3F &pos, const Point3F &rot,
                              Point3F normal, DecalData *data)
{
   if(smMaxNumDecals <= 0 || !data)
      return;

   // Nothing to lay where the surface faces the way the mark would point
   // (0x0059445a).
   if(mDot(rot, normal) >= 0.98f)
      return;

   if(mDecals.size() >= smMaxNumDecals)
      mDecals.pop_front();

   Point3F vecX = mCross(rot, normal);
   Point3F vecY = mCross(normal, vecX);

   normal.normalizeSafe();
   vecX.normalizeSafe();
   vecY.normalizeSafe();

   Decal decal;
   decal.data = data;
   decal.born = Platform::getVirtualMilliseconds();
   decal.fade = 1.0f;

   const Point3F at = pos + normal * 0.008f;
   const Point3F x = vecX * data->t2SizeX;
   const Point3F y = vecY * data->t2SizeY;

   decal.point[0] = at + x + y;
   decal.point[1] = at + x - y;
   decal.point[2] = at - x - y;
   decal.point[3] = at - x + y;

   mDecals.push_back(decal);
}

void DecalManagerT2::dataDeleted(DecalData *data)
{
   for(S32 i = mDecals.size() - 1; i >= 0; i--)
      if(mDecals[i].data == data)
         mDecals.erase(i);
}

//----------------------------------------------------------------------------

void DecalManagerT2::prepRenderImage(SceneRenderState *state)
{
   if(!DecalManager::smDecalsOn || mDecals.empty() || !state->isDiffusePass())
      return;

   // The last quarter of a mark's life is spent fading out (0x00594820).
   const U32 now = Platform::getVirtualMilliseconds();
   const U32 full = U32(smDecalTimeout) * 3 / 4;
   for(S32 i = mDecals.size() - 1; i >= 0; i--)
   {
      const U32 age = now - mDecals[i].born;
      if(age > U32(smDecalTimeout))
         mDecals.erase(i);
      else if(age > full)
         mDecals[i].fade = 1.0f - F32(age - full) / F32(smDecalTimeout / 4);
      else
         mDecals[i].fade = 1.0f;
   }

   if(mDecals.empty())
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &DecalManagerT2::render);
   ri->type = RenderPassManager::RIT_ObjectTranslucent;
   state->getRenderPass()->addInst(ri);
}

void DecalManagerT2::render(ObjectRenderInst *, SceneRenderState *,
                            BaseMatInstance *overrideMat)
{
   if(overrideMat)
      return;

   if(mStateBlock.isNull())
   {
      // What retail draws them with (0x00594820): blended over the surface,
      // never written to the depth buffer, and nothing under a tenth of an
      // alpha drawn at all.
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      desc.setAlphaTest(true, GFXCmpGreater, 25);
      desc.setZReadWrite(true, false);
      desc.setCullMode(GFXCullNone);
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      mStateBlock = GFX->createStateBlock(desc);
   }

   GFX->setStateBlock(mStateBlock);

   const DecalData *bound = NULL;
   for(U32 i = 0; i < mDecals.size(); i++)
   {
      const Decal &decal = mDecals[i];
      if(decal.data->t2Texture.isNull())
         continue;

      if(decal.data != bound)
      {
         GFX->setTexture(0, decal.data->t2Texture);
         GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
         bound = decal.data;
      }

      PrimBuild::color4f(1.0f, 1.0f, 1.0f, decal.fade);
      PrimBuild::begin(GFXTriangleFan, 4);
      PrimBuild::texCoord2f(0.0f, 0.0f);
      PrimBuild::vertex3fv(decal.point[0]);
      PrimBuild::texCoord2f(0.0f, 1.0f);
      PrimBuild::vertex3fv(decal.point[1]);
      PrimBuild::texCoord2f(1.0f, 1.0f);
      PrimBuild::vertex3fv(decal.point[2]);
      PrimBuild::texCoord2f(1.0f, 0.0f);
      PrimBuild::vertex3fv(decal.point[3]);
      PrimBuild::end();
   }
}
