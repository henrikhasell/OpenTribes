#include "gui/hud/hudBombSight.h"

#include "T3D/bombSight.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/objectTypes.h"
#include "T3D/tsStatic.h"
#include "console/consoleTypes.h"
#include "scene/sceneContainer.h"

IMPLEMENT_CONOBJECT(HudBombSight);

/// One step of the bomb's fall, in seconds, and what gravity takes off its
/// climb over it (0x0077d8b8, 0x0077d8b4).
static const F32 sStep = 0.2f;
static const F32 sFall = -1.962f;

/// The bomb is followed this long and no further (0x0077d8bc).
static const F32 sHorizon = 10.f;

/// Terrain, interiors, water and static shapes (0x201c, pushed at 0x0050a0b0).
static const U32 sGroundMask = TerrainObjectType | InteriorObjectType |
                               WaterObjectType | StaticShapeObjectType;

/// Tribes 2 types a TSStatic StaticTSObjectType (its constructor, 0x0068f3c0),
/// which that mask leaves out; Torque 3D types one a static shape.
static bool notTSStatic(RayInfo *info)
{
   return dynamic_cast<TSStatic*>(info->object) == NULL;
}

//-----------------------------------------------------------------------------
HudBombSight::HudBombSight()
{
   mSight = NULL;
   mShapeName = StringTable->insert("");
   mMinDisplayHeight = 10.f;
}

void HudBombSight::initPersistFields()
{
   Parent::initPersistFields();

   addField("Shape",            TypeString, Offset(mShapeName, HudBombSight));
   addField("minDisplayHeight", TypeF32,    Offset(mMinDisplayHeight, HudBombSight));
}

bool HudBombSight::onWake()
{
   if(!Parent::onWake())
      return false;

   mSight = new BombSight;
   mSight->setShapeName(mShapeName && mShapeName[0] ? mShapeName : "reticle_bomber.dts");
   if(!mSight->registerObject())
   {
      delete mSight;
      mSight = NULL;
   }

   return true;
}

void HudBombSight::onSleep()
{
   if(mSight)
      mSight->deleteObject();
   mSight = NULL;

   Parent::onSleep();
}

void HudBombSight::onRender(Point2I, const RectI &)
{
      // Each of these leaves the sight as the last frame left it (0x00509f9e,
      // 0x00509fb8, 0x00509ff2).
   GameConnection *conn = GameConnection::getConnectionToServer();
   if(!conn)
      return;

   GameBase *control = conn->getControlObject();
   if(!control)
      return;

   SceneObject *vehicle = control->getObjectMount();
   if(!vehicle)
      return;

   Point3F velocity = vehicle->getVelocity();
   Point3F origin;
   vehicle->getTransform().getColumn(3, &origin);

   Point3F from = origin;
   Point3F impact(0.f, 0.f, 0.f);
   bool hit = false;

   for(F32 elapsed = 0.f; ; )
   {
      velocity.z += sFall;

         // Retail sums these on the x87 stack and rounds once, at the store.
      const Point3F to(F32(F64(velocity.x) * sStep + from.x),
                       F32(F64(velocity.y) * sStep + from.y),
                       F32(F64(velocity.z) * sStep + from.z));

      RayInfo info;
      if(gClientContainer.castRay(from, to, sGroundMask, &info, notTSStatic))
      {
         impact = info.point;
         hit = true;
         break;
      }

      elapsed += sStep;
      from = to;
      if(!(elapsed <= sHorizon))
         break;
   }

      // Nothing below minDisplayHeight, then fading in until twice that.
   F32 fade = 1.f;
   if(mMinDisplayHeight > 0.f)
      fade = F32((F64(origin.z) - impact.z) / mMinDisplayHeight - 1.0);

   if(!mSight)
      return;

   if(hit && fade > 0.f)
   {
      MatrixF at(true);
      at.setColumn(3, impact);
      mSight->setTransform(at);
      mSight->setShown(true, fade > 1.f ? 1.f : fade);
   }
   else
      mSight->setShown(false, 0.f);
}
