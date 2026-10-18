// HH: Added for Tribes 2 compatibility.
#ifndef _SHIELDIMPACTT2_H_
#define _SHIELDIMPACTT2_H_

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif

class ShapeBase;
class SceneRenderState;
class BaseMatInstance;

/// The flash a shield puts round a shape it turns a hit away from: retail's
/// ShieldImpactRenderImage, which every ShapeBase carries at this+0x284 and
/// constructs at 0x00693de0. It is a sphere stretched over the shape's
/// bounds and lit with special/shieldmap, fading out over the datablock's
/// shieldEffectLifetimeMS.
class ShieldImpactT2
{
public:
   ShieldImpactT2();

   /// $pref::useOldShieldEffect, the options dialog's "HI-RES SHIELD
   /// EFFECTS", at 0x009ecec3. Retail reads it each time it draws.
   static bool smUseOldShieldEffect;

   /// Load the map, which retail does in ShapeBase::onAdd (0x005e75db) for
   /// a ghost only.
   void load();

   /// What ShapeBase::playShieldEffect (0x005eb8e0) does with a client's
   /// object: a fresh start at the strength the server sent.
   void start(const Point3F &normal, F32 energy, F32 lifetime,
              const Point3F &scale);

   /// Retail draws and advances the effect while this holds, and its
   /// elapsed time starts past its lifetime so a new shape shows none.
   bool isActive() const { return mElapsed <= mLifetime; }

   void advance(F32 dt);

   /// One less the square of the share of its life gone, times the energy
   /// the server sent, which both effects fade by.
   F32 getAlpha() const;

   /// How far the hi-res effect has grown the shape, from 1 to 1.1 over its
   /// life.
   F32 getGrowth() const { return mGrowth; }

   void render(ShapeBase *owner, SceneRenderState *state);

   /// The material the hi-res effect draws a shape over with: blended while
   /// it fades, solid while it is still near full strength, and unlit over a
   /// self-illuminating material.
   static BaseMatInstance *getRedrawMaterial(bool blended, bool selfIllum);

   /// Free the instances while the materials and the material manager they
   /// came from are still there.
   static void freeRedrawMaterials();

private:
   GFXTexHandle      mTexture;
   GFXStateBlockRef  mStateBlock;

   F32      mLifetime;              ///< seconds (+0x08)
   F32      mElapsed;               ///< seconds (+0x0c)
   F32      mGrowth;                ///< +0x10, which only the hi-res
                                    ///< effect draws with
   F32      mEnergy;                ///< +0x14
   F32      mGrowFrom;              ///< +0x18
   F32      mGrowTo;                ///< +0x1c
   Point3F  mNormal;                ///< +0x20, kept but never drawn with
   F32      mDrift;                 ///< +0x2c, slides the map
   Point3F  mScale;                 ///< +0x30
   F32      mPhase;                 ///< +0x3c, ripples the map
};

#endif
