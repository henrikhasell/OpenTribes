// HH: Added for Tribes 2 compatibility.
#ifndef _DECALST2_H_
#define _DECALST2_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif

class DecalData;
class ObjectRenderInst;
class BaseMatInstance;

/// The marks Tribes 2 leaves on the world: a bullet hole, a footprint.
///
/// Each is one quad on the plane of what it marks, the size its datablock
/// gives, held for $pref::Decal::decalTimeout and faded out over the last
/// quarter of that.  Retail's manager (0x00593dd0) clips nothing and lights
/// nothing: a decal is the texture laid flat, 8 mm off the surface so it does
/// not fight with it.
class DecalManagerT2 : public SceneObject
{
   typedef SceneObject Parent;

   struct Decal
   {
      DecalData *data;
      Point3F    point[4];
      U32        born;
      F32        fade;
   };

   Vector<Decal>    mDecals;
   GFXStateBlockRef mStateBlock;

   void render(ObjectRenderInst *ri, SceneRenderState *state,
               BaseMatInstance *overrideMat);

public:
   DecalManagerT2();

   DECLARE_CONOBJECT(DecalManagerT2);

   /// Lay a decal flat on the surface it marks, which is all a projectile's
   /// mark needs (0x00593dd0).
   void addDecal(const Point3F &pos, Point3F normal, DecalData *data);

   /// The same, turned so its y axis lies along \a rot -- a footprint points
   /// the way the player is facing (0x00594420).  Nothing is laid where the
   /// surface is within about eleven degrees of \a rot.
   void addDecal(const Point3F &pos, const Point3F &rot, Point3F normal,
                 DecalData *data);

   /// Forget every decal of a datablock that is about to go.
   void dataDeleted(DecalData *data);

   void prepRenderImage(SceneRenderState *state) override;

   static void consoleInit();

   static S32  smMaxNumDecals;
   static S32  smDecalTimeout;
};

extern DecalManagerT2 *gDecalManagerT2;

#endif // _DECALST2_H_
