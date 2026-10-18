// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/shieldImpactT2.h"

#include "T3D/shapeBase.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "math/util/matrixSet.h"

namespace
{
   /// What sphereGen.cc's builder (0x006a0fe0) is asked for when the game
   /// starts: eight faces split twice, which is 128 triangles.
   enum { SphereFaces = 8, SphereSplits = 2 };

   struct SphereVert
   {
      Point3F pos;
      Point2F uv;
   };

   /// The one sphere every shield draws, kept as retail keeps it at
   /// 0x009e874c: three corners a triangle, none of them shared, so a corner
   /// on the seam can take its own side's coordinate.
   class ShieldSphere
   {
   public:
      Vector<Point3F> pos;
      Vector<Point2F> uv;

      const ShieldSphere &build();

   private:
      Vector<SphereVert> mVerts;

      void subdivide(const SphereVert *tri, U32 depth);
      static void split(const SphereVert *tri, SphereVert *out);
      static F32 unwrap(F32 u, F32 a, F32 b);
   };

   ShieldSphere sSphere;

   /// Four smaller triangles, wound as the one they came from (0x006a1620).
   void ShieldSphere::split(const SphereVert *tri, SphereVert *out)
   {
      SphereVert mid[3];
      for(U32 i = 0; i < 3; i++)
      {
         const SphereVert &a = tri[i];
         const SphereVert &b = tri[(i + 1) % 3];
         mid[i].pos.interpolate(a.pos, b.pos, 0.5f);
         mid[i].pos.normalize();
         mid[i].uv = a.uv * 0.5f + b.uv * 0.5f;
      }

      const SphereVert &a = mid[0];
      const SphereVert &b = mid[1];
      const SphereVert &c = mid[2];

      out[0] = tri[0]; out[1]  = a; out[2]  = c;
      out[3] = tri[1]; out[4]  = b; out[5]  = a;
      out[6] = a;      out[7]  = b; out[8]  = c;
      out[9] = tri[2]; out[10] = c; out[11] = b;
   }

   void ShieldSphere::subdivide(const SphereVert *tri, U32 depth)
   {
      if(depth == SphereSplits)
      {
         for(U32 i = 0; i < 3; i++)
            mVerts.push_back(tri[i]);
         return;
      }

      SphereVert out[12];
      split(tri, out);
      for(U32 i = 0; i < 4; i++)
         subdivide(&out[i * 3], depth + 1);
   }

   /// A corner whose neighbours sit across the seam from it is carried on
   /// round past 1 rather than back to 0 (0x006a1300).
   F32 ShieldSphere::unwrap(F32 u, F32 a, F32 b)
   {
      if((mFabs(u - a) > 0.5f || mFabs(u - b) > 0.5f) && u < 0.5f)
         return u + 1.0f;
      return u;
   }

   const ShieldSphere &ShieldSphere::build()
   {
      if(!pos.empty())
         return *this;

      // A pole and a ring round the equator. The ring is worked in double,
      // as the x87 worked it, so the corner on the seam lands on its right
      // side of it.
      const U32 half = SphereFaces / 2;
      SphereVert ring[half + 1];
      ring[0].pos.set(0.0f, 0.0f, 1.0f);
      for(U32 i = 1; i <= half; i++)
      {
         const F64 angle = F64(i) / F64(half) * M_2PI;
         ring[i].pos.set(F32(mCos(angle)), F32(mSin(angle)), 0.0f);
      }
      for(U32 i = 0; i <= half; i++)
         ring[i].uv.set(0.0f, 0.0f);

      mVerts.clear();
      for(U32 k = 0; k < half; k++)
      {
         const U32 next = (k + 2) % (half + 1) + (k == half - 1 ? 1 : 0);

         SphereVert tri[3];
         ring[0].pos.z = 1.0f;
         tri[0] = ring[0];
         tri[1] = ring[next];
         tri[2] = ring[k + 1];
         subdivide(tri, 0);

         ring[0].pos.z = -1.0f;
         tri[0] = ring[0];
         tri[1] = ring[k + 1];
         tri[2] = ring[next];
         subdivide(tri, 0);
      }

      // The map wraps once round the waist and once from pole to pole
      // (0x006a1380).
      for(U32 i = 0; i < mVerts.size(); i++)
      {
         F32 yaw, pitch;
         MathUtils::getAnglesFromVector(mVerts[i].pos, yaw, pitch);
         mVerts[i].uv.set(F32(F64(yaw) * (1.0 / M_2PI)),
                          (mVerts[i].pos.z + 1.0f) * 0.5f);
      }

      for(U32 i = 0; i + 2 < mVerts.size(); i += 3)
      {
         const F32 u0 = mVerts[i].uv.x;
         const F32 u1 = mVerts[i + 1].uv.x;
         const F32 u2 = mVerts[i + 2].uv.x;
         mVerts[i].uv.x     = unwrap(u0, u1, u2);
         mVerts[i + 1].uv.x = unwrap(u1, u0, u2);
         mVerts[i + 2].uv.x = unwrap(u2, u0, u1);
      }

      pos.setSize(mVerts.size());
      uv.setSize(mVerts.size());
      for(U32 i = 0; i < mVerts.size(); i++)
      {
         pos[i] = mVerts[i].pos;
         uv[i] = mVerts[i].uv;
      }
      mVerts.clear();
      return *this;
   }
}

ShieldImpactT2::ShieldImpactT2()
{
   mLifetime = 0.3f;
   mElapsed  = mLifetime + 1.0f;
   mGrowth   = 1.0f;
   mEnergy   = 1.0f;
   mGrowFrom = 1.0f;
   mGrowTo   = 1.1f;
   mNormal.set(0.0f, 0.0f, 1.0f);
   mDrift    = 0.0f;
   mScale.set(1.0f, 1.0f, 1.0f);
   mPhase    = 0.0f;
}

void ShieldImpactT2::load()
{
   if(mTexture.isNull())
      mTexture.set("textures/special/shieldmap",
                   &GFXDefaultStaticDiffuseProfile, "ShieldImpactT2");
}

void ShieldImpactT2::start(const Point3F &normal, F32 energy, F32 lifetime,
                           const Point3F &scale)
{
   mNormal   = normal;
   mLifetime = lifetime;
   mScale    = scale;
   mEnergy   = energy;
   mElapsed  = 0.0f;
   mGrowth   = 1.0f;
}

/// 0x006945a0, then 0x00694880 for the ripple. The drift and the phase are
/// never put back, so each hit picks the map up where the last left it.
void ShieldImpactT2::advance(F32 dt)
{
   if(dt == 0.0f)
      return;

   mElapsed += dt;
   mGrowth = mGrowFrom + (mGrowTo - mGrowFrom) * (mElapsed / mLifetime);
   mDrift += dt * 0.2f;
   mPhase += dt * 6.0f;
}

/// The effect retail draws while $pref::useOldShieldEffect is clear
/// (0x00694600): the sphere, scaled by the datablock's shieldEffectScale to
/// eight tenths of the shape's bounding sphere and stood on its centre, its
/// map rippling and doubled by the texture matrix, added into the scene at
/// one less the square of the share of its life gone, times the energy the
/// server sent.
void ShieldImpactT2::render(ShapeBase *owner, SceneRenderState *state)
{
   if(!owner || !isActive() || mTexture.isNull())
      return;

   const ShieldSphere &sphere = sSphere.build();

   if(mStateBlock.isNull())
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);
      desc.setZReadWrite(true, false);
      desc.setCullMode(GFXCullCCW);
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
      mStateBlock = GFX->createStateBlock(desc);
   }

   const SphereF &bounds = owner->getWorldSphere();
   const F32 radius = bounds.radius * 0.8f;

   MatrixF mat = owner->getRenderTransform();
   mat.setPosition(bounds.center);
   mat.scale(Point3F(mScale.x * radius, mScale.y * radius,
                     mScale.z * radius));

   const F32 t = mElapsed / mLifetime;
   F32 alpha = (1.0f - t * t) * mEnergy;
   if(alpha > 1.0f)
      alpha = 1.0f;

   // A bin leaves the scene's camera in its matrix set and the device's
   // view at identity, which the generic shader would draw through.
   GFXTransformSaver saver;
   MatrixSet &matrices = state->getRenderPass()->getMatrixSet();
   GFX->setViewMatrix(matrices.getWorldToCamera());
   GFX->setProjectionMatrix(matrices.getCameraToScreen());
   GFX->setWorldMatrix(mat);
   GFX->setStateBlock(mStateBlock);
   GFX->setTexture(0, mTexture);

   PrimBuild::color4f(1.0f, 1.0f, 1.0f, alpha);
   PrimBuild::begin(GFXTriangleList, sphere.pos.size());
   for(U32 i = 0; i < sphere.pos.size(); i++)
   {
      const Point2F &uv = sphere.uv[i];
      const F32 u = uv.x + mDrift +
                    0.02f * mSin(uv.x * M_2PI_F * 10.0f + mPhase);
      const F32 v = uv.y + mDrift +
                    0.02f * mSin(uv.y * M_2PI_F * 10.0f + mPhase);
      PrimBuild::texCoord2f(u * 2.0f, v * 2.0f);
      PrimBuild::vertex3fv(sphere.pos[i]);
   }
   PrimBuild::end();
}
