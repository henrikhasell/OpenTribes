// HH: Added for Tribes 2 compatibility.
#ifndef _BOMBSIGHT_H_
#define _BOMBSIGHT_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif

class TSShapeInstance;

/// @file
/// The marker a bomber's sight lays on the ground (bombSight.cc, 0x25c bytes,
/// built at 0x00677580).
///
/// It is not a HUD drawing at all: it is a shape in the client's world, put
/// where a bomb let go now would land, so the ground it sits on and anything in
/// front of it shows through the way the world would.  HudBombSight owns one for
/// as long as it is awake and moves it every frame.

class BombSight : public SceneObject
{
   typedef SceneObject Parent;

   StringTableEntry mShapeName;
   Resource<TSShape> mShape;
   TSShapeInstance *mShapeInstance;

   bool mShown;
   F32 mAlpha;

public:
   BombSight();
   ~BombSight();

   DECLARE_CONOBJECT(BombSight);

   /// The shape under shapes/ to draw, which has to be named before the
   /// object is registered (0x00677950).
   void setShapeName(const char *name);

   /// Whether to draw it this frame, and how faint (0x00677960).
   void setShown(bool shown, F32 alpha);

   bool onAdd();
   void onRemove();

   void prepRenderImage(SceneRenderState *state);
};

#endif // _BOMBSIGHT_H_
