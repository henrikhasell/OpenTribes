#include "gui/hud/hudNavDisplay.h"

#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/shapeBase.h"
#include "T3D/staticShape.h"
#include "T3D/camera.h"
#include "T3D/missionMarker.h"
#include "T3D/gameBase/beaconObjectT2.h"
#include "gui/3d/guiTSControl.h"
#include "scene/sceneManager.h"
#include "collision/collision.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mMath.h"

IMPLEMENT_CONOBJECT(HudCompass);
IMPLEMENT_CONOBJECT(HudNavDisplay);

//-----------------------------------------------------------------------------
/// Which way the camera is pointing, in radians clockwise from north.
///
/// Off the control object's own transform rather than off anything the server
/// said: on a Tribes 2 connection the angles that come back are a round trip
/// old, and the view is drawn with the ones the move stream has accumulated.
/// Taking the matrix the renderer used is the same answer by construction.
static bool cameraYaw(F32 &yaw)
{
   GameConnection *conn = GameConnection::getConnectionToServer();
   if(!conn)
      return false;

   // This tree's getControlObject answers a GameBase, where the engine this
   // came from answered a ShapeBase.  Only a ShapeBase has a camera.
   ShapeBase *control = dynamic_cast<ShapeBase*>(conn->getControlObject());
   if(!control)
      return false;

   MatrixF mat;
   F32 pos = 0.f;
   control->getCameraTransform(&pos, &mat);

   // Column one is the way the camera faces; north is +y.
   Point3F forward;
   mat.getColumn(1, &forward);

   // mAtan2 here takes (y, x); north is +y, so the heading is atan2(x, y).
   yaw = mAtan2(forward.x, forward.y);
   return true;
}

//-----------------------------------------------------------------------------
HudCompass::HudCompass()
{
   mTextColor.set(255, 255, 255, 255);
   setExtent(128, 16);
}

void HudCompass::initPersistFields()
{
   Parent::initPersistFields();

   addField("textColor", TypeColorI, Offset(mTextColor, HudCompass));
}

/// The face of the dial, read off the table at 0x0077bea0: a float count, that
/// many x,y pairs, repeated until a count of zero.
struct CompassMark
{
   U32 count;
   struct { F32 x, y; } points[12];
};

static const CompassMark sCompassFace[] =
{
   {  4, { {-2,-28}, {-2,-34}, {2,-28}, {2,-34} } },                       // N
   {  4, { {28,2}, {28,-2}, {34,-2}, {34,2} } },                           // E
   {  2, { {31,-2}, {31,2} } },                                            // its bar
   {  5, { {-34,4}, {-28,2}, {-34,0}, {-28,-2}, {-34,-4} } },              // W
   { 12, { {2,30}, {1,29}, {-1,29}, {-2,30}, {-2,31}, {-1,32},
           {1,32}, {2,33}, {2,34}, {1,35}, {-1,35}, {-2,34} } },           // S
   {  2, { {25,-24}, {29,-28} } },                                         // NE
   {  2, { {25,24}, {29,28} } },                                           // SE
   {  2, { {-24,24}, {-28,28} } },                                         // SW
   {  2, { {-24,-24}, {-28,-28} } },                                       // NW
};

/// HudCompass::onRender at 0x00502870 rotates the marks about the middle of the
/// control by where north is from here.
void HudCompass::onRender(Point2I offset, const RectI &updateRect)
{
   F32 yaw;
   if(!cameraYaw(yaw))
      return;

   const F32 sine = mSin(-yaw);
   const F32 cosine = mCos(-yaw);

      // Retail adds a literal 40.5 and 38.5, the middle of the 81 by 77 control.
   const F32 centreX = F32(offset.x) + F32(getExtent().x) / 2.f;
   const F32 centreY = F32(offset.y) + F32(getExtent().y) / 2.f;

   ColorI colour = mTextColor;
   colour.alpha = 255;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   for(U32 m = 0; m < sizeof(sCompassFace) / sizeof(sCompassFace[0]); m++)
   {
      const CompassMark &mark = sCompassFace[m];
      Point2I last(0, 0);

      for(U32 i = 0; i < mark.count; i++)
      {
         const F32 x = mark.points[i].x;
         const F32 y = mark.points[i].y;

         const Point2I at(S32(centreX + cosine * x - sine * y),
                          S32(centreY + cosine * y + sine * x));

         if(i > 0)
            draw->drawLine(last, at, colour);
         last = at;
      }
   }
}

//-----------------------------------------------------------------------------
/// What blocks the view of a marker, and what the crosshair can land on.
static const U32 sSightMask = EnvironmentObjectType | TerrainObjectType |
                              InteriorObjectType | WaterObjectType |
                              StaticShapeObjectType | PlayerObjectType |
                              ItemObjectType | VehicleObjectType;

/// The names script knows the marker types by.
static const char *sMarkerTypeNames[] =
{
   "MissionWaypoint", "PotentialTask", "AssignedTask", "ClientWaypoint",
   "Target",
};

static S32 markerType(const char *name)
{
   for(U32 i = 0; i < sizeof(sMarkerTypeNames) / sizeof(sMarkerTypeNames[0]); i++)
      if(!dStricmp(name, sMarkerTypeNames[i]))
         return S32(i);
   return -1;
}

/// Where a point lands on the screen, or false if it is behind the eye.  A
/// point off the side of the view still lands somewhere, which is what a
/// triangle over a head half out of the frame needs.
static bool projectPoint(const GuiTSCtrl *ts, const Point3F &pt, Point3F *out)
{
   MatrixF worldProjection = ts->getLastProjectionMatrix();
   worldProjection.mul(ts->getLastWorldMatrix());

   Point4F p(pt.x, pt.y, pt.z, 1.0f);
   worldProjection.mul(p);
   if(p.w <= 0.0f)
      return false;

   const RectI &view = ts->getLastViewportRect();
   out->x = (p.x / p.w + 1.0f) * 0.5f * view.extent.x + view.point.x;
   out->y = (1.0f - p.y / p.w) * 0.5f * view.extent.y + view.point.y;
   out->z = p.z / p.w;
   return out->z >= 0.0f && out->z <= 1.0f;
}

/// `colour` out of 255, its alpha scaled.
static ColorI scaled(const ColorI &colour, F32 alpha)
{
   ColorF c(colour);
   c.alpha *= alpha;
   c.clamp();
   return c;
}

//-----------------------------------------------------------------------------
HudNavDisplay::HudNavDisplay()
{
   for(U32 i = 0; i < NumMarkerTypes; i++)
      mMarkerTypeVisible[i] = true;

   mSensorGroup = 0;

   for(U32 i = 0; i < NumMarkerImages; i++)
      mMarkerImageNames[i] = StringTable->EmptyString();

   // HudNavDisplay's constructor (0x00503350).
   mDefaultMarkerColor.set(0, 255, 0, 255);
   mFriendBeaconColor.set(255, 255, 0, 255);
   mEnemyBeaconColor.set(0, 255, 0, 255);
   mVehicleBeaconColor.set(0, 255, 0, 255);
   mDamageFrameColor.set(255, 255, 255, 255);
   mTargetLockedFrameColor.set(255, 0, 0, 255);

   mMissileMarker1      = StringTable->EmptyString();
   mMissileMarker2      = StringTable->EmptyString();
   mMissileFlash1       = StringTable->EmptyString();
   mMissileFlash2       = StringTable->EmptyString();
   mMissileTargetRing   = StringTable->EmptyString();
   mMissileMarkerHilite = StringTable->EmptyString();

   mImageOffset = 1;
   mInfoOffset.set(10, 10);
   mDamageRectSize.set(50, 12);

   mMarkerAngle = 45.f;
   mMarkerLen = 15.f;
   mMarkerHitRadius = 10.f;
   mMarkerScaleDistance = 1000.f;
   mMinMarkerScale = 0.5f;
   mMinProjectSize = 10;
   mMarkerTextEdgeOffset = 2;
   mAcquireScreenRadius = 15;
   mAcquireBoxCheckDistance = 50.f;
   mPlayerEyeZOffset = 2.f;

   mRenderEdgeMarkers = true;
   mRenderMarkerText = true;
   mRenderXYProjection = false;
   mProtectedStatics = false;
}

void HudNavDisplay::initPersistFields()
{
   Parent::initPersistFields();

   addField("defaultMarkerColor",     TypeColorI, Offset(mDefaultMarkerColor, HudNavDisplay));
   addField("friendBeaconColor",      TypeColorI, Offset(mFriendBeaconColor, HudNavDisplay));
   addField("enemyBeaconColor",       TypeColorI, Offset(mEnemyBeaconColor, HudNavDisplay));
   addField("vehicleBeaconColor",     TypeColorI, Offset(mVehicleBeaconColor, HudNavDisplay));
   addField("damageFrameColor",       TypeColorI, Offset(mDamageFrameColor, HudNavDisplay));
   addField("targetLockedFrameColor", TypeColorI, Offset(mTargetLockedFrameColor, HudNavDisplay));

   addField("markerImageNames", TypeString, Offset(mMarkerImageNames, HudNavDisplay), NumMarkerImages);

   addField("missileMarker1",      TypeString, Offset(mMissileMarker1, HudNavDisplay));
   addField("missileMarker2",      TypeString, Offset(mMissileMarker2, HudNavDisplay));
   addField("missileFlash1",       TypeString, Offset(mMissileFlash1, HudNavDisplay));
   addField("missileFlash2",       TypeString, Offset(mMissileFlash2, HudNavDisplay));
   addField("missileTargetRing",   TypeString, Offset(mMissileTargetRing, HudNavDisplay));
   addField("missileMarkerHilite", TypeString, Offset(mMissileMarkerHilite, HudNavDisplay));

   addField("imageOffset",    TypeS32,     Offset(mImageOffset, HudNavDisplay));
   addField("infoOffset",     TypePoint2I, Offset(mInfoOffset, HudNavDisplay));
   addField("damageRectSize", TypePoint2I, Offset(mDamageRectSize, HudNavDisplay));

   addField("markerAngle",             TypeF32, Offset(mMarkerAngle, HudNavDisplay));
   addField("markerLen",               TypeF32, Offset(mMarkerLen, HudNavDisplay));
   addField("markerHitRadius",         TypeF32, Offset(mMarkerHitRadius, HudNavDisplay));
   addField("markerScaleDistance",     TypeF32, Offset(mMarkerScaleDistance, HudNavDisplay));
   addField("minMarkerScale",          TypeF32, Offset(mMinMarkerScale, HudNavDisplay));
   addField("minProjectSize",          TypeS32, Offset(mMinProjectSize, HudNavDisplay));
   addField("markerTextEdgeOffset",    TypeS32, Offset(mMarkerTextEdgeOffset, HudNavDisplay));
   addField("acquireScreenRadius",     TypeS32, Offset(mAcquireScreenRadius, HudNavDisplay));
   addField("acquireBoxCheckDistance", TypeF32, Offset(mAcquireBoxCheckDistance, HudNavDisplay));
   addField("playerEyeZOffset",        TypeF32, Offset(mPlayerEyeZOffset, HudNavDisplay));

   addField("renderEdgeMarkers",  TypeBool, Offset(mRenderEdgeMarkers, HudNavDisplay));
   addField("renderMarkerText",   TypeBool, Offset(mRenderMarkerText, HudNavDisplay));
   addField("renderXYProjection", TypeBool, Offset(mRenderXYProjection, HudNavDisplay));
   addField("protectedStatics",   TypeBool, Offset(mProtectedStatics, HudNavDisplay));
}

void HudNavDisplay::loadMarkerImages()
{
   for(U32 i = 0; i < NumMarkerImages; i++)
   {
      if(mMarkerImageNames[i] && mMarkerImageNames[i][0])
         mMarkerImages[i].set(mMarkerImageNames[i], &GFXDefaultGUIProfile,
                              avar("%s() - mMarkerImages (line %d)", __FUNCTION__, __LINE__));
      else
         mMarkerImages[i] = NULL;
   }
}

bool HudNavDisplay::onWake()
{
   if(!Parent::onWake())
      return false;

   loadMarkerImages();
   return true;
}

void HudNavDisplay::onSleep()
{
   for(U32 i = 0; i < NumMarkerImages; i++)
      mMarkerImages[i] = NULL;

   Parent::onSleep();
}

void HudNavDisplay::inspectPostApply()
{
   Parent::inspectPostApply();

   if(isAwake())
      loadMarkerImages();
}

/// 0x00503e70: PlayGui lays this out at 640x480 from the corner, and it is
/// stretched over the whole view whatever the view's size.
void HudNavDisplay::onPreRender()
{
   Parent::onPreRender();

   GuiControl *parent = getParent();
   if(parent && getPosition().isZero() && getExtent() != parent->getExtent())
      resize(Point2I(0, 0), parent->getExtent());
}

bool HudNavDisplay::isMarkerTypeVisible(S32 type) const
{
   return type >= 0 && type < NumMarkerTypes && mMarkerTypeVisible[type];
}

void HudNavDisplay::setMarkerTypeVisible(S32 type, bool visible)
{
   // A shape's marker is not script's to hide.
   if(type >= 0 && type < Shape)
   {
      mMarkerTypeVisible[type] = visible;
      setUpdate();
   }
}

//-----------------------------------------------------------------------------
/// The shapes worth a marker (0x00503f00): those the server gave a target
/// and that this client senses, or that have an image drawn whether sensed
/// or not.  A player needs to be in sight as well; anything else only matters
/// near the crosshair, where it can be acquired.
void HudNavDisplay::collectShapes(GameConnection *conn, ShapeBase *control,
                                  const Point3F &camPos, const Point3F &fwd,
                                  F32 fov, F32 visDist, F32 acquireAngle)
{
   const F32 halfFov = fov * 0.5f;

   for(SimSet::iterator i = conn->begin(); i != conn->end(); i++)
   {
      ShapeBase *shape = dynamic_cast<ShapeBase*>(*i);
      if(!shape || shape == control)
         continue;

      ShapeBaseData *data = static_cast<ShapeBaseData*>(shape->getDataBlock());
      const S32 target = shape->getT2Target();
      if(!data || target < 0)
         continue;

      const U32 mask = T2Targets::renderMask(target);
      bool always = (mask & BIT(8)) != 0;
      for(U32 k = 0; !always && k < ShapeBaseData::NumHudRenderImages; k++)
         always = (mask & BIT(k)) && data->hudRenderAlways[k];

      if(!T2Targets::isSensed(target) && !always)
         continue;

      const Point3F centre = shape->getWorldBox().getCenter();
      Point3F dir = centre - camPos;
      const F32 len = dir.len();
      if(len > 0.0f)
         dir /= len;
      const F32 angle = mFabs(mAcos(mClampF(mDot(dir, fwd), -1.0f, 1.0f)));

      const bool isPlayer = (shape->getTypeMask() & PlayerObjectType) != 0;
      if(!isPlayer && !always && angle >= acquireAngle)
         continue;
      if(shape->getTypeMask() & CorpseObjectType)
         continue;
      // A player in a vehicle is marked by the vehicle.
      if(isPlayer && shape->getObjectMount())
         continue;

      if(!always && ((centre - camPos).lenSquared() > visDist * visDist ||
                     angle > halfFov))
         continue;

      bool inSight = false;
      if(!always || isPlayer)
      {
         Point3F to = centre;
         if(isPlayer)
         {
            MatrixF eye;
            shape->getRenderEyeTransform(&eye);
            to = eye.getPosition();
         }

         SceneObject *mount = shape->getObjectMount();
         control->disableCollision();
         shape->disableCollision();
         if(mount)
            mount->disableCollision();

         RayInfo ri;
         inSight = !gClientContainer.castRay(camPos, to, sSightMask, &ri);

         if(mount)
            mount->enableCollision();
         shape->enableCollision();
         control->enableCollision();

         if(!inSight && !always)
            continue;
      }

      Entry e;
      e.type = Shape;
      e.flags = (inSight ? InSight : 0) | (always ? Always : 0) |
                (isPlayer ? IsPlayer : 0) |
                (inSight && !data->isInvincible ? Acquirable : 0);
      e.pos = centre;
      e.obj = shape;
      e.health = mClampF(1.0f - shape->getDamageValue(), 0.0f, 1.0f);
      e.dist = 0.0f;
      mEntries.push_back(e);
   }
}

/// 0x005046a0.
void HudNavDisplay::collect(GameConnection *conn, ShapeBase *control,
                            const Point3F &camPos, const Point3F &fwd, F32 fov,
                            F32 visDist)
{
   mEntries.clear();

   mAcquireScreenRadius = getMax(getMin(mAcquireScreenRadius, getExtent().x), 1);
   const F32 acquireAngle = fov * F32(mAcquireScreenRadius) / F32(getExtent().x);

   collectShapes(conn, control, camPos, fwd, fov, visDist, acquireAngle);

   // What the crosshair is on.
   control->disableCollision();
   RayInfo ri;
   SceneObject *hit = gClientContainer.castRay(camPos, camPos + fwd * visDist,
                                               sSightMask, &ri) ? ri.object : NULL;
   control->enableCollision();

   // The one the crosshair is on wins outright; short of that, the last one
   // near enough to it does, not the nearest.
   S32 acquired = -1;
   for(U32 i = 0; i < mEntries.size(); i++)
   {
      const Entry &e = mEntries[i];
      if(!(e.flags & Acquirable))
         continue;

      const Point3F d = e.pos - camPos;
      const F32 len = d.len();
      if(len > visDist)
         continue;

      if(e.obj == hit)
      {
         acquired = i;
         break;
      }

      F32 angle = mFabs(mAcos(mClampF(mDot(d / len, fwd), -1.0f, 1.0f)));
      if(mDot(d, fwd) < mAcquireBoxCheckDistance)
      {
         // Close up, being on the box counts as being on the shape.
         Point3F start, end;
         e.obj->getWorldTransform().mulP(camPos, &start);
         e.obj->getWorldTransform().mulP(camPos + fwd * mAcquireBoxCheckDistance, &end);
         start.convolveInverse(e.obj->getScale());
         end.convolveInverse(e.obj->getScale());

         RayInfo boxRi;
         if(e.obj->castRay(start, end, &boxRi))
            angle = 0.0f;
      }

      if(angle < acquireAngle)
         acquired = i;
   }
   if(acquired != -1)
      mEntries[acquired].flags |= Acquired;

   for(SimSet::iterator i = Sim::getWayPointSet()->begin();
       i != Sim::getWayPointSet()->end(); i++)
   {
      WayPoint *point = dynamic_cast<WayPoint*>(*i);
      if(!point || point->mWayPointHidden)
         continue;

      Entry e;
      e.type = MissionWaypoint;
      e.flags = Always;
      e.pos = point->getPosition();
      e.obj = point;
      e.health = 0.0f;
      e.dist = 0.0f;
      mEntries.push_back(e);
   }

   // The beacons of this client's own side (0x00504f1b), sensed or not.
   for(SimSet::iterator i = conn->begin(); i != conn->end(); i++)
   {
      BeaconObject *beacon = dynamic_cast<BeaconObject*>(*i);
      if(!beacon)
         continue;

      const S32 target = beacon->getT2Target();
      if(target == -1 || T2Targets::sensorGroup(target) != mSensorGroup)
         continue;

      Entry e;
      e.type = Target;
      e.flags = Always;
      e.pos = beacon->getRenderPosition();
      e.obj = beacon;
      e.health = 0.0f;
      e.dist = 0.0f;
      mEntries.push_back(e);
   }
}

//-----------------------------------------------------------------------------
/// Where the way to something off the screen crosses its edge (0x00507490 to
/// 0x0050878e).  The way to it is turned about the view's up axis into the
/// plane of the view's left or right edge, so it meets that edge as high as
/// the thing is; if that is off the top or bottom, it is turned about the
/// view's right axis into the plane of the top or bottom edge instead.
bool HudNavDisplay::edgePoint(const GuiTSCtrl *ts, const MatrixF &cam,
                              const Point3F &dir, const Point2I &offset,
                              Point2I *at) const
{
   const Point2I &ext = getExtent();
   const Point3F camPos = cam.getPosition();
   const Point3F right = cam.getRightVector();
   const Point3F fwd = cam.getForwardVector();
   const Point3F up = cam.getUpVector();

   const Frustum &frustum = ts->getLastFrustum();
   const F32 nearDist = frustum.getNearDist();
   const F32 halfWidth = (frustum.getNearRight() - frustum.getNearLeft()) * 0.5f;
   const F32 halfHeight = (frustum.getNearTop() - frustum.getNearBottom()) * 0.5f;

   const F32 dr = mDot(dir, right);
   const F32 df = mDot(dir, fwd);
   const F32 du = mDot(dir, up);

   Point3F s;
   bool projected = false;

   const F32 across = mSqrt(dr * dr + df * df);
   if(across > 0.0f)
   {
      Point3F edge = fwd * nearDist + right * (dr > 0.0f ? halfWidth : -halfWidth);
      edge.normalize();
      projected = projectPoint(ts, camPos + edge * across + up * du, &s);
   }

   if(!projected || s.y < offset.y || s.y >= offset.y + ext.y)
   {
      const F32 along = mSqrt(du * du + df * df);
      projected = false;
      if(along > 0.0f)
      {
         Point3F edge = fwd * nearDist + up * (du > 0.0f ? halfHeight : -halfHeight);
         edge.normalize();
         projected = projectPoint(ts, camPos + edge * along + right * dr, &s);
      }
   }

   if(!projected)
   {
      s.x = F32(dr > 0.0f ? offset.x + ext.x : offset.x);
      s.y = F32(du > 0.0f ? offset.y : offset.y + ext.y);
   }

   // Out from the middle of the control to its border.
   const F32 centreX = offset.x + (ext.x + 1) * 0.5f;
   const F32 centreY = offset.y + (ext.y + 1) * 0.5f;
   Point2F way(s.x - centreX, s.y - centreY);
   const F32 len = way.len();
   if(len == 0.0f)
      return false;
   way /= len;

   F32 best = 1e10f;
   S32 side = -1;
   if(way.x < 0.0f && (centreX - offset.x) / -way.x < best)
      best = (centreX - offset.x) / -way.x, side = 0;
   if(way.x > 0.0f && (offset.x + ext.x - centreX) / way.x < best)
      best = (offset.x + ext.x - centreX) / way.x, side = 1;
   if(way.y < 0.0f && (centreY - offset.y) / -way.y < best)
      best = (centreY - offset.y) / -way.y, side = 2;
   if(way.y > 0.0f && (offset.y + ext.y - centreY) / way.y < best)
      best = (offset.y + ext.y - centreY) / way.y, side = 3;
   if(side < 0)
      return false;

   F32 x = centreX + way.x * best;
   F32 y = centreY + way.y * best;
   switch(side)
   {
      case 0: x = F32(offset.x);         break;
      case 1: x = F32(offset.x + ext.x); break;
      case 2: y = F32(offset.y);         break;
      case 3: y = F32(offset.y + ext.y); break;
   }

   at->set(S32(x), S32(y));
   return true;
}

//-----------------------------------------------------------------------------
/// One marker (0x00505740): for a shape, the images its render mask asks for,
/// stacked over its head, and then the one centred on it with its name below
/// and its distance above.  Nothing is scaled; a far marker only fades.
void HudNavDisplay::drawMarker(const GuiTSCtrl *ts, const Entry &e, Point2I at,
                               bool onEdge, const Point2I &offset)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const Point2I &ext = getExtent();

   const F32 dist = mClampF(e.dist, 0.0f, 9999.0f);
   const F32 fade = mMarkerScaleDistance * mMinMarkerScale > dist
                  ? 1.0f - dist / mMarkerScaleDistance : mMinMarkerScale;
   const F32 alpha = mOpacity * fade;

   ColorI imageColor = T2Targets::groupColor(mSensorGroup, mSensorGroup);
   const ColorI textColor = mProfile->mFontColor;

   char distText[40];
   dSprintf(distText, sizeof(distText), "%4.1f", dist);

   char nameText[256];
   const char *name = NULL;
   const char *distance = NULL;
   GFXTextureObject *centre = NULL;

   switch(e.type)
   {
      case MissionWaypoint:
      {
         WayPoint *point = static_cast<WayPoint*>(e.obj);
         centre = mMarkerImages[0];
         const S32 team = point->mTeam.mTeamId;
         if(team >= 0 && team < 32)
            imageColor = T2Targets::groupColor(mSensorGroup, team);
         distance = distText;
         if(point->mName && point->mName[0])
            name = point->mName;
         break;
      }

      // A beacon is coloured by its type, with its distance under it
      // (0x00506131).
      case Target:
      {
         BeaconObject *beacon = static_cast<BeaconObject*>(e.obj);
         centre = mMarkerImages[Target];
         if(beacon->getT2Target() < 32)
            switch(beacon->getBeaconType())
            {
               case BeaconObject::Enemy:  imageColor = mEnemyBeaconColor;   break;
               case BeaconObject::Friend: imageColor = mFriendBeaconColor;  break;
               default:                   imageColor = mVehicleBeaconColor; break;
            }
         name = distText;
         break;
      }

      case Shape:
      {
         ShapeBase *shape = static_cast<ShapeBase*>(e.obj);
         ShapeBaseData *data = static_cast<ShapeBaseData*>(shape->getDataBlock());
         const S32 target = shape->getT2Target();
         const U32 mask = T2Targets::renderMask(target);
         if(!data || !(mask & 0xff))
            return;

         const U32 group = T2Targets::sensorGroup(target);
         const bool enemy = group != mSensorGroup;
         const bool sensed = T2Targets::isSensed(target);

         bool haveCentre = false, haveEye = false;
         Point2I eye(0, 0);
         S32 left = 0, right = 0;

         for(U32 i = 0; i < ShapeBaseData::NumHudRenderImages; i++)
         {
            if(!(mask & BIT(i)) || !data->hudImageFriendly[i])
               continue;

            // Drawn when forced, or when sensed and in sight.
            if(!data->hudRenderAlways[i] && !(e.flags & Always) &&
               !(sensed && (e.flags & InSight)))
               continue;

            GFXTextureObject *image = enemy && data->hudImageEnemy[i]
                                    ? data->hudImageEnemy[i] : data->hudImageFriendly[i];
            const ColorI colour = data->hudRenderModulated[i]
                                ? T2Targets::groupColor(mSensorGroup, group)
                                : ColorI(255, 255, 255, 255);

            if(data->hudRenderCenter[i])
            {
               if(haveCentre)
                  continue;
               haveCentre = true;

               centre = image;
               imageColor = colour;
               if(data->hudRenderDistance[i])
                  distance = distText;
               if(data->hudRenderName[i] &&
                  T2Targets::getTargetName(target, nameText, sizeof(nameText)))
                  name = nameText;
               continue;
            }

            // Over the head, the first centred on it and the rest beside it,
            // each a row higher, as retail stacks them.
            if(!haveEye)
            {
               MatrixF eyeMat;
               shape->getRenderEyeTransform(&eyeMat);
               Point3F p = eyeMat.getPosition();
               p.z += mPlayerEyeZOffset;

               Point3F s;
               if(projectPoint(ts, p, &s))
                  eye.set(S32(s.x), S32(s.y));
               haveEye = true;
            }

            draw->setBitmapModulation(scaled(colour, alpha));

            const S32 width = image->getBitmapWidth();
            if(left == 0)
            {
               const S32 half = (width + 1) >> 1;
               eye.x -= half;
               left = right = half + mImageOffset;
            }
            else
            {
               const S32 step = mImageOffset + width;
               if(left <= right)
               {
                  eye.x -= left + width;
                  left += step;
               }
               else
               {
                  eye.x += right;
                  right += step;
               }
            }
            eye.y -= image->getBitmapHeight();
            draw->drawBitmap(image, eye);
         }
         break;
      }

      default:
         return;
   }

   if(!centre)
      return;

   const S32 width = centre->getBitmapWidth();
   const S32 height = centre->getBitmapHeight();
   Point2I p(at.x - (width >> 1), at.y - (height >> 1));
   p.x = getMax(getMin(p.x, offset.x + ext.x - width), offset.x);
   p.y = getMax(getMin(p.y, offset.y + ext.y - height), offset.y);

   draw->setBitmapModulation(scaled(imageColor, alpha));
   draw->drawBitmap(centre, p);

   if(!mRenderMarkerText)
      return;

   GFont *font = mProfile->mFont;
   if(!font)
      return;
   const S32 fontHeight = font->getHeight();

   // Kept off the sides of the screen by markerTextEdgeOffset.
   auto textX = [&](const char *text) -> S32
   {
      const S32 w = font->getStrWidth(text);
      S32 x = p.x + (width >> 1) - (w >> 1);
      if(x < offset.x + mMarkerTextEdgeOffset)
         x = offset.x + mMarkerTextEdgeOffset;
      else if(x + w >= offset.x + ext.x - mMarkerTextEdgeOffset)
         x = offset.x + ext.x - w - mMarkerTextEdgeOffset;
      return x;
   };

   // Under the image, or over it if that would leave the screen.  Retail
   // tests the extent alone, not the position plus the extent.
   bool nameAbove = false;
   if(name)
   {
      S32 y = p.y + height;
      if(U32(y + fontHeight) >= U32(ext.y))
      {
         y = p.y - fontHeight;
         nameAbove = true;
      }

      draw->setBitmapModulation(scaled(textColor, alpha));
      draw->drawText(font, Point2I(textX(name), y), name, NULL, 9);
   }

   if(distance && !nameAbove && !onEdge && p.y != 0)
   {
      draw->setBitmapModulation(scaled(textColor, alpha));
      draw->drawText(font, Point2I(textX(distance), p.y - fontHeight), distance, NULL, 9);
   }
}

/// The name and health of what the crosshair is on (0x00505380), beside the
/// middle of the screen.  Only a player's, or something on this client's own
/// side.
void HudNavDisplay::drawTargetInfo(const Entry &e, const Point2I &offset)
{
   ShapeBase *shape = static_cast<ShapeBase*>(e.obj);
   const S32 target = shape->getT2Target();
   const U32 group = T2Targets::sensorGroup(target);
   if(group != mSensorGroup && !(e.flags & IsPlayer))
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const Point2I &ext = getExtent();
   const S32 cx = offset.x + (ext.x >> 1);
   const S32 cy = offset.y + (ext.y >> 1);
   const Point2I at(cx + mInfoOffset.x, cy + mInfoOffset.y);

   GFont *font = mProfile->mFont;
   const S32 fontHeight = font ? font->getHeight() : 0;

   char name[256];
   if(font && T2Targets::getTargetName(target, name, sizeof(name)))
   {
      draw->setBitmapModulation(scaled(mProfile->mFontColor, mOpacity));
      draw->drawText(font, at, name, mProfile->mFontColors, 9);
   }
   draw->clearBitmapModulation();

   ShapeBaseData *data = static_cast<ShapeBaseData*>(shape->getDataBlock());
   if(mProtectedStatics && (shape->getTypeMask() & StaticObjectType) &&
      dynamic_cast<StaticShapeData*>(data) &&
      static_cast<StaticShapeData*>(data)->noIndividualDamage)
      return;

   ColorI fill = T2Targets::groupColor(mSensorGroup, group);
   fill.alpha = U8(S32(fill.alpha * mOpacity));
   if(!fill.alpha)
      return;

   ColorI frame = mDamageFrameColor;
   frame.alpha = U8(S32(frame.alpha * mOpacity));

   // A one pixel frame round damageRectSize, and inside it a bar as long as
   // the health left, never one pixel: a sliver shows as two.
   const S32 x = at.x;
   const S32 y = at.y + fontHeight + 2;
   const S32 w = mDamageRectSize.x;
   const S32 h = mDamageRectSize.y;

   draw->drawRectFill(Point2I(x, y), Point2I(x + w, y + 1), frame);
   draw->drawRectFill(Point2I(x, y + h - 1), Point2I(x + w, y + h), frame);
   draw->drawRectFill(Point2I(x, y + 1), Point2I(x + 1, y + h - 1), frame);
   draw->drawRectFill(Point2I(x + w - 1, y + 1), Point2I(x + w, y + h - 1), frame);

   S32 bar = S32((w - 1) * e.health);
   if(bar == 1)
      bar = 2;
   if(bar > 0)
      draw->drawRectFill(Point2I(x + 1, y + 1), Point2I(x + bar, y + h - 1), fill);
}

/// 0x00506870.  Nothing of HudCtrl's own is drawn: this control covers the
/// whole view, and a fill would wash over all of it.
void HudNavDisplay::onRender(Point2I offset, const RectI &updateRect)
{
   GameConnection *conn = GameConnection::getConnectionToServer();
   if(!conn)
      return;

   // This tree's getControlObject answers a GameBase, where the engine this
   // came from answered a ShapeBase.  Retail marks nothing for a camera.
   ShapeBase *control = dynamic_cast<ShapeBase*>(conn->getControlObject());
   if(!control || dynamic_cast<Camera*>(control))
      return;

   GuiTSCtrl *ts = dynamic_cast<GuiTSCtrl*>(getParent());
   if(!ts)
      return;

   // The camera the view was drawn with.
   const CameraQuery &query = ts->getLastCameraQuery();
   const MatrixF &cam = query.cameraMatrix;
   const Point3F camPos = cam.getPosition();
   const Point3F fwd = cam.getForwardVector();

   mSensorGroup = T2Targets::clientSensorGroup();

   collect(conn, control, camPos, fwd, query.fov,
           gClientSceneGraph->getVisibleDistance());

   const Point2I &ext = getExtent();
   const F32 farLimit = query.farPlane * 0.75f;

   for(U32 i = 0; i < mEntries.size(); i++)
   {
      Entry &e = mEntries[i];
      if(!mMarkerTypeVisible[e.type])
         continue;

      Point3F d = e.pos - camPos;
      const F32 dist = d.len();
      if(dist == 0.0f)
         continue;

      // A point past the far plane is brought in along the same line.
      Point3F p = e.pos;
      if(dist > farLimit)
         p = camPos + d * (farLimit / dist);

      Point3F s;
      bool onScreen = false;
      Point2I sp(0, 0);
      if(projectPoint(ts, p, &s))
      {
         sp.set(S32(s.x), S32(s.y));
         onScreen = sp.x >= offset.x && sp.x < offset.x + ext.x &&
                    sp.y >= offset.y && sp.y < offset.y + ext.y;
      }

      e.dist = dist;
      if(onScreen)
      {
         if((e.flags & Acquired) && conn->isFirstPerson())
            drawTargetInfo(e, offset);
         drawMarker(ts, e, sp, false, offset);
         continue;
      }

      if(!mRenderEdgeMarkers || dist <= mMarkerHitRadius)
         continue;

      Point2I edge;
      if(edgePoint(ts, cam, d / dist, offset, &edge))
         drawMarker(ts, e, edge, true, offset);
   }

   GFX->getDrawUtil()->clearBitmapModulation();
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudNavDisplay, isMarkerTypeVisible, bool, (const char *type), ,
   "@brief Whether markers of this type are drawn: MissionWaypoint, "
   "PotentialTask, AssignedTask, ClientWaypoint or Target.\n\n@ingroup GuiHud")
{
   return object->isMarkerTypeVisible(markerType(type));
}

DefineEngineMethod(HudNavDisplay, setMarkerTypeVisible, void, (const char *type, bool visible), ,
   "@brief Draw markers of this type, or stop.\n\n@ingroup GuiHud")
{
   object->setMarkerTypeVisible(markerType(type), visible);
}
