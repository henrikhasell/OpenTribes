// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/guiCommanderMap.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/primBuilder.h"
#include "gui/commander/commanderIconData.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiTypes.h"
#include "scene/sceneManager.h"
#include "T3D/gameBase/clientTargetT2.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/targetManagerT2.h"
#include "T3D/gameBase/targetsT2.h"
#include "T3D/gameFunctions.h"
#include "T3D/missionArea.h"
#include "T3D/player.h"
#include "T3D/shapeBase.h"
#include "core/frameAllocator.h"
#include "terrain/terrData.h"

IMPLEMENT_CONOBJECT(GuiCommanderMap);

ConsoleDocClass( GuiCommanderMap,
   "@brief The commander map: the mission from above, with an icon on "
   "everything the team knows of.\n\n@internal" );

GuiCommanderMap *GuiCommanderMap::smInstance = NULL;

ClientTarget *CommanderRef::getLocal() const
{
   ClientTarget *target = NULL;
   return local && Sim::findObject(local, target) ? target : NULL;
}

namespace
{
   enum Cursor { DefaultCursor, ArrowCursor, HandCursor, MoveCursor, ZoomCursor,
                 AddCursor, RemoveCursor, NumCursors };

   const char *sCursorFields[NumCursors] =
   {
      "defaultCursor", "arrowCursor", "handCursor", "moveCursor", "zoomCursor",
      "addCursor", "removeCursor",
   };

   /// Retail looks down with a 90 degree view across (0x659569).
   const F32 sFov = M_HALFPI_F;

   /// The ghost each target is worn by, gathered once a frame.
   ShapeBase *sGhosts[TargetManagerT2::MaxTargets];
   U32 sGhostFrame = U32(-1);

   void gatherGhosts()
   {
      if (sGhostFrame == GuiTSCtrl::getFrameCount())
         return;
      sGhostFrame = GuiTSCtrl::getFrameCount();
      dMemset(sGhosts, 0, sizeof(sGhosts));

      GameConnection *conn = GameConnection::getConnectionToServer();
      if (!conn)
         return;

      for (SimSet::iterator i = conn->begin(); i != conn->end(); i++)
      {
         ShapeBase *shape = dynamic_cast<ShapeBase *>(*i);
         if (!shape)
            continue;

         const S32 target = shape->getT2Target();
         if (target < 0 || target >= TargetManagerT2::MaxTargets)
            continue;

         // A player's corpse keeps its target; the living one wins.
         if (sGhosts[target] && (shape->getTypeMask() & CorpseObjectType))
            continue;
         sGhosts[target] = shape;
      }
   }
}

ShapeBase *GuiCommanderMap::ghostOf(S32 target)
{
   if (target < 0 || target >= TargetManagerT2::MaxTargets)
      return NULL;
   gatherGhosts();
   return sGhosts[target];
}

bool GuiCommanderMap::isKnown(S32 target)
{
   if (target < TargetManagerT2::TeamTargets || !T2Targets::isAllocated(target))
      return false;
   return T2Targets::sensorGroup(target) == T2Targets::clientSensorGroup() ||
          T2Targets::isSensed(target);
}

//----------------------------------------------------------------------------

GuiCommanderMap::GuiCommanderMap()
{
   // Retail's own names (0x7b3834 on), which the shipped gui relies on: it
   // spells one field "waypintIconName".
   static const char *sCursors[NumCursors] =
   {
      "DefaultCursor", "CMDCursorArrow", "CMDCursorHandOpen", "CMDCursorHandClosed",
      "CMDCursorZoom", "CMDCursorSelectAdd", "CMDCursorSelectRemove",
   };
   for (U32 i = 0; i < NumCursors; i++)
      mCursorNames[i] = StringTable->insert(sCursors[i]);

   // The defaults of retail's constructor (0x658c93 on).
   mMouseSelectRectColor.set(255, 255, 0, 255);
   mCameraOffset = 10.0f;
   mCameraZoomTime = 300;
   mCameraMoveTime = 500;
   mDefaultIconName = StringTable->insert("CMDDefaultIcon");
   mWaypointIconName = StringTable->insert("CMDWaypointIcon");
   mAssignedTaskIconName = StringTable->insert("CMDAssignedTaskIcon");
   mPotentialTaskIconName = StringTable->insert("CMDPotentialTaskIcon");
   mSensorSphereFrameAlpha = 75;
   mSensorSphereFillAlpha = 30;
   mSensorSphereDetailLevel = 64;
   mRenderMissionArea = true;
   mMissionAreaFillColor.set(60, 60, 60, 80);
   mMissionAreaFrameColor.set(128, 0, 0, 255);
   mRenderText = true;
   mTextOffset = 2;
   mHilightedObjectColor.set(244, 0, 0, 128);
   mSelectedObjectColor.set(255, 0, 255, 128);
   mPlayerMarkerAngle = 30.0f;
   mPlayerMarkerOffset = 8.0f;
   mPlayerMarkerLen = 8.0f;
   mPlayerMarkerColor.set(0, 255, 0, 200);
   mEnableEdgeMarkers = true;
   mEdgeMarkerSize.set(32, 32);
   mEdgeMarkerTextureName = StringTable->insert("commander/gui/cmd_offscreen_arrow");
   mRenderSensors = false;
   mMinIconSize = 8;
   mMaxIconSize = 40;
   mIconProjLen = 20.0f;
   mMinDistanceScale = 0.02f;
   mMaxDistanceScale = 0.05f;
   mScreenFrameSize = 0.05f;

   mMouseMode = SelectMode;
   mCamera.set(0, 0, 500);
   mCameraMin.set(-1024, -1024, 0);
   mCameraMax.set(1024, 1024, 1000);
   mHaveBounds = false;
   mResetPending = false;
   mTopZ = 0.0f;
   for (U32 i = 0; i < NumDirections; i++)
      mMoving[i] = false;
   mLastUpdate = 0;
   mZooming = false;
   mZoomStart = 0;
   mDragging = false;
   mDragSelect = false;
   mMouseInside = false;
}

void GuiCommanderMap::initPersistFields()
{
   for (U32 i = 0; i < NumCursors; i++)
      addField(sCursorFields[i], TypeString,
               Offset(mCursorNames, GuiCommanderMap) + i * sizeof(StringTableEntry),
               "A GuiCursor for one of the map's mouse modes.");

   addField("mouseSelectRectColor", TypeColorI, Offset(mMouseSelectRectColor, GuiCommanderMap), "");
   addField("cameraOffset", TypeF32, Offset(mCameraOffset, GuiCommanderMap),
            "How near the ground and the tallest thing the camera may come.");
   addField("cameraZoomTime", TypeS32, Offset(mCameraZoomTime, GuiCommanderMap), "");
   addField("cameraMoveTime", TypeS32, Offset(mCameraMoveTime, GuiCommanderMap), "");
   addField("defaultIconName", TypeString, Offset(mDefaultIconName, GuiCommanderMap), "");
   addField("waypointIconName", TypeString, Offset(mWaypointIconName, GuiCommanderMap), "");
   addField("assignedTaskIconName", TypeString, Offset(mAssignedTaskIconName, GuiCommanderMap), "");
   addField("potentialTaskIconName", TypeString, Offset(mPotentialTaskIconName, GuiCommanderMap), "");
   addField("sensorSphereFrameAlpha", TypeS32, Offset(mSensorSphereFrameAlpha, GuiCommanderMap), "");
   addField("sensorSphereFillAlpha", TypeS32, Offset(mSensorSphereFillAlpha, GuiCommanderMap), "");
   addField("sensorSphereDetailLevel", TypeS32, Offset(mSensorSphereDetailLevel, GuiCommanderMap), "");
   addField("renderMissionArea", TypeBool, Offset(mRenderMissionArea, GuiCommanderMap), "");
   addField("missionAreaFillColor", TypeColorI, Offset(mMissionAreaFillColor, GuiCommanderMap), "");
   addField("missionAreaFrameColor", TypeColorI, Offset(mMissionAreaFrameColor, GuiCommanderMap), "");
   addField("renderText", TypeBool, Offset(mRenderText, GuiCommanderMap), "");
   addField("textOffset", TypeS32, Offset(mTextOffset, GuiCommanderMap), "");
   addField("hilightedObjectColor", TypeColorI, Offset(mHilightedObjectColor, GuiCommanderMap), "");
   addField("selectedObjectColor", TypeColorI, Offset(mSelectedObjectColor, GuiCommanderMap), "");
   addField("playerMarkerAngle", TypeF32, Offset(mPlayerMarkerAngle, GuiCommanderMap), "");
   addField("playerMarkerOffset", TypeF32, Offset(mPlayerMarkerOffset, GuiCommanderMap), "");
   addField("playerMarkerLen", TypeF32, Offset(mPlayerMarkerLen, GuiCommanderMap), "");
   addField("playerMarkerColor", TypeColorI, Offset(mPlayerMarkerColor, GuiCommanderMap), "");
   addField("enableEdgeMarkers", TypeBool, Offset(mEnableEdgeMarkers, GuiCommanderMap), "");
   addField("edgeMarkerSize", TypePoint2I, Offset(mEdgeMarkerSize, GuiCommanderMap), "");
   addField("edgeMarkerTextureName", TypeString, Offset(mEdgeMarkerTextureName, GuiCommanderMap), "");
   addField("renderSensors", TypeBool, Offset(mRenderSensors, GuiCommanderMap), "");
   addField("minIconSize", TypeS32, Offset(mMinIconSize, GuiCommanderMap), "");
   addField("maxIconSize", TypeS32, Offset(mMaxIconSize, GuiCommanderMap), "");
   addField("iconProjLen", TypeF32, Offset(mIconProjLen, GuiCommanderMap),
            "The metres an icon stands for, before minIconSize and maxIconSize.");
   addField("minDistanceScale", TypeF32, Offset(mMinDistanceScale, GuiCommanderMap), "");
   addField("maxDistanceScale", TypeF32, Offset(mMaxDistanceScale, GuiCommanderMap),
            "How much of the view iconProjLen must cover before every label shows.");
   addField("screenFrameSize", TypeF32, Offset(mScreenFrameSize, GuiCommanderMap),
            "The share of the view left around the mission area when furthest out.");

   Parent::initPersistFields();
}

bool GuiCommanderMap::onAdd()
{
   ClientTarget::initTypes();

   if (!Parent::onAdd())
      return false;

   if (smInstance && smInstance != this)
      Con::errorf("GuiCommanderMap:: already instantiated");
   smInstance = this;
   return true;
}

void GuiCommanderMap::onRemove()
{
   if (smInstance == this)
      smInstance = NULL;
   Parent::onRemove();
}

bool GuiCommanderMap::onWake()
{
   if (!Parent::onWake())
      return false;

   smInstance = this;

   if (mEdgeMarkerTextureName && mEdgeMarkerTextureName[0])
      mEdgeMarker.set(mEdgeMarkerTextureName, &GFXDefaultGUIProfile, avar("%s() - edge", __FUNCTION__));

   if (!mBlendBlock)
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      desc.setZReadWrite(false);
      desc.zWriteEnable = false;
      desc.setCullMode(GFXCullNone);
      mBlendBlock = GFX->createStateBlock(desc);
   }

   // A new mission, or one whose area had not arrived when the map was last
   // reset, starts from above the whole area again -- once the scripts have
   // woken too, since retail opens there even though they ask it to follow
   // the player the first time.
   const RectI was = mBoundsArea;
   computeBounds();
   mResetPending = mBoundsArea != was;
   clampCamera();

   for (U32 i = 0; i < NumDirections; i++)
      mMoving[i] = false;
   mLastUpdate = Platform::getRealMilliseconds();
   return true;
}

void GuiCommanderMap::onSleep()
{
   mEdgeMarker = NULL;
   mDragging = false;
   mDragSelect = false;
   Parent::onSleep();
}

GuiCursor *GuiCommanderMap::cursor(U32 which) const
{
   GuiCursor *cur = NULL;
   if (mCursorNames[which] && mCursorNames[which][0])
      Sim::findObject(mCursorNames[which], cur);
   return cur;
}

//----------------------------------------------------------------------------
// The camera

/// The box the camera stays in (0x659230 and 0x659490): over the mission
/// area, no lower than the lowest ground there and no higher than fits the
/// whole area in the view, both reckoned from cameraOffset above the
/// ground and the tallest interior.
void GuiCommanderMap::computeBounds()
{
   RectI area = MissionArea::smMissionArea;
   if (area.extent.x <= 0 || area.extent.y <= 0)
      area.set(-1024, -1024, 2048, 2048);

   F32 low = F32_MAX, high = -F32_MAX;

   Vector<SceneObject *> terrains;
   gClientContainer.findObjectList(TerrainObjectType, &terrains);
   for (S32 i = 0; i < terrains.size(); i++)
   {
      TerrainBlock *terrain = dynamic_cast<TerrainBlock *>(terrains[i]);
      if (!terrain)
         continue;

      const F32 step = getMax(terrain->getSquareSize(), 1.0f);
      for (F32 y = area.point.y; y <= area.point.y + area.extent.y; y += step)
         for (F32 x = area.point.x; x <= area.point.x + area.extent.x; x += step)
         {
            F32 h;
            if (!terrain->getHeight(Point2F(x, y) - Point2F(terrain->getPosition().x, terrain->getPosition().y), &h))
               continue;
            h += terrain->getPosition().z;
            low = getMin(low, h);
            high = getMax(high, h);
         }
   }

   if (low > high)
      low = high = 0.0f;

   Vector<SceneObject *> tall;
   const Box3F box(Point3F(area.point.x, area.point.y, -F32_MAX),
                   Point3F(area.point.x + area.extent.x, area.point.y + area.extent.y, F32_MAX));
   gClientContainer.findObjectList(box, InteriorObjectType | StaticShapeObjectType, &tall);
   for (S32 i = 0; i < tall.size(); i++)
      if (!tall[i]->isGlobalBounds())
         high = getMax(high, tall[i]->getWorldBox().maxExtents.z);

   const F32 w = getWidth(), h = getHeight();
   const F32 screenDiag = mSqrt(w * w + h * h) * 0.5f * (1.0f - mScreenFrameSize);
   const F32 areaDiag = mSqrt(F32(area.extent.x * area.extent.x + area.extent.y * area.extent.y)) * 0.5f;
   const F32 fit = screenDiag > 0 ? (w / (2.0f * mTan(sFov * 0.5f))) * areaDiag / screenDiag : 500.0f;

   mBoundsArea = area;
   mTopZ = high + mCameraOffset;
   mCameraMin.set(area.point.x, area.point.y, low + mCameraOffset);
   mCameraMax.set(area.point.x + area.extent.x, area.point.y + area.extent.y,
                  high + mCameraOffset + fit);
   mHaveBounds = true;
}

void GuiCommanderMap::clampCamera()
{
   mCamera.x = mClampF(mCamera.x, mCameraMin.x, mCameraMax.x);
   mCamera.y = mClampF(mCamera.y, mCameraMin.y, mCameraMax.y);
   mCamera.z = mClampF(mCamera.z, mCameraMin.z, mCameraMax.z);

   // Never into the hill under the camera.
   RayInfo ray;
   if (gClientContainer.castRay(Point3F(mCamera.x, mCamera.y, mCameraMax.z),
                                Point3F(mCamera.x, mCamera.y, mCameraMin.z - mCameraOffset),
                                TerrainObjectType, &ray))
      mCamera.z = getMax(mCamera.z, ray.point.z + mCameraOffset);
}

void GuiCommanderMap::resetCamera()
{
   computeBounds();
   mCamera.set((mCameraMin.x + mCameraMax.x) * 0.5f,
               (mCameraMin.y + mCameraMax.y) * 0.5f, mCameraMax.z);
   mFollow = CommanderRef();
   mZooming = false;
}

/// Held keys move a view's width each cameraMoveTime and halve the height
/// over the ground each cameraZoomTime (0x6598f0); a followed object keeps
/// the view centred on it.
void GuiCommanderMap::updateCamera()
{
   if (mResetPending)
   {
      mResetPending = false;
      resetCamera();
   }

   const U32 now = Platform::getRealMilliseconds();
   const F32 dt = F32(getMin(now - mLastUpdate, U32(250)));
   mLastUpdate = now;

   const F32 height = mCamera.z - mCameraMin.z + mCameraOffset;
   const F32 pan = 2.0f * mTan(sFov * 0.5f) * height * dt / F32(getMax(mCameraMoveTime, 100));
   const F32 zoom = 0.5f * height * dt / F32(getMax(mCameraZoomTime, 100));

   if (mMoving[Left] || mMoving[Right] || mMoving[Up] || mMoving[Down])
      mFollow = CommanderRef();

   if (mMoving[Left])  mCamera.x -= pan;
   if (mMoving[Right]) mCamera.x += pan;
   if (mMoving[Up])    mCamera.y += pan;
   if (mMoving[Down])  mCamera.y -= pan;
   if (mMoving[In])    mCamera.z -= zoom;
   if (mMoving[Out])   mCamera.z += zoom;

   if (mZooming)
   {
      const F32 t = mClampF(F32(now - mZoomStart) / F32(getMax(mCameraZoomTime, 1)), 0.0f, 1.0f);
      mCamera.interpolate(mZoomFrom, mZoomTo, t);
      if (t >= 1.0f)
         mZooming = false;
   }

   if (mFollow.isValid())
   {
      Point3F pos;
      bool have = false;
      if (ClientTarget *local = mFollow.getLocal())
      {
         pos = local->getPosition();
         have = true;
      }
      else if (ShapeBase *shape = ghostOf(mFollow.target))
      {
         pos = shape->getRenderPosition();
         have = true;
      }

      if (have)
      {
         mCamera.x = pos.x;
         mCamera.y = pos.y;
         if (mZooming)
         {
            mZoomFrom.x = mZoomTo.x = pos.x;
            mZoomFrom.y = mZoomTo.y = pos.y;
         }
      }
   }

   clampCamera();
}

void GuiCommanderMap::zoomTo(const Point3F &target)
{
   mZoomFrom = mCamera;
   mZoomTo = target;
   mZoomTo.x = mClampF(mZoomTo.x, mCameraMin.x, mCameraMax.x);
   mZoomTo.y = mClampF(mZoomTo.y, mCameraMin.y, mCameraMax.y);
   mZoomTo.z = mClampF(mZoomTo.z, mCameraMin.z, mCameraMax.z);
   mZoomStart = Platform::getRealMilliseconds();
   mZooming = true;
}

void GuiCommanderMap::cameraMove(U32 dir, bool on)
{
   if (dir < NumDirections)
      mMoving[dir] = on;
}

void GuiCommanderMap::followLastSelected()
{
   if (mLastSelected.isValid() && isSelected(mLastSelected))
   {
      mFollow = mLastSelected;
      return;
   }

   // Nothing to follow puts the whole mission area back in view.
   mFollow = CommanderRef();
   mCamera.x = (mCameraMin.x + mCameraMax.x) * 0.5f;
   mCamera.y = (mCameraMin.y + mCameraMax.y) * 0.5f;
}

F32 GuiCommanderMap::viewWidthAt(F32 z) const
{
   return getMax(2.0f * mTan(sFov * 0.5f) * (mCamera.z - z), 0.01f);
}

bool GuiCommanderMap::processCameraQuery(CameraQuery *query)
{
   updateCamera();

   // Looking straight down, north up the screen.
   MatrixF mat(true);
   mat.setColumn(0, Point3F(1, 0, 0));
   mat.setColumn(1, Point3F(0, 0, -1));
   mat.setColumn(2, Point3F(0, 1, 0));
   mat.setPosition(mCamera);

   query->object = NULL;
   query->ortho = false;
   query->nearPlane = 0.1f;
   query->farPlane = mCamera.z - mCameraMin.z + mCameraOffset + 2000.0f;
   query->fov = fovFromHorizontal(sFov);
   query->cameraMatrix = mat;
   return true;
}

/// The ground the mission is fought over, without fog and as far down as it
/// goes: retail's map shows the whole mission area at any height, and draws
/// the terrain, the interiors and the water and nothing else (0x0065cf1f
/// renders with a type mask of 0x1c). Everything that moves is an icon.
void GuiCommanderMap::renderWorld(const RectI &updateRect)
{
   SceneManager *scene = gClientSceneGraph;
   const FogData fog = scene->getFogData();
   const F32 visible = scene->getVisibleDistance();

   scene->setFogData(FogData());
   scene->setVisibleDistance(mLastCameraQuery.farPlane);

   FrameAllocator::setWaterMark(0);
   scene->renderScene(SPT_Diffuse,
      TerrainObjectType | InteriorObjectType | WaterObjectType);
   GFX->updateStates();
   FrameAllocator::setWaterMark(0);

   scene->setFogData(fog);
   scene->setVisibleDistance(visible);
}

/// Where a point lands on the screen, on or off it: the camera only ever
/// looks straight down, so this is a scale about the middle of the view.
bool GuiCommanderMap::worldToScreen(const Point3F &pos, Point2F *out) const
{
   if (pos.z >= mCamera.z)
      return false;

   const F32 scale = F32(getWidth()) / viewWidthAt(pos.z);
   const Point2I mid = const_cast<GuiCommanderMap *>(this)->localToGlobalCoord(getExtent() / 2);
   out->set(mid.x + (pos.x - mCamera.x) * scale, mid.y - (pos.y - mCamera.y) * scale);
   return true;
}

bool GuiCommanderMap::screenToGround(const Point2I &local, Point3F *out)
{
   const Point2I global = localToGlobalCoord(local);
   Point3F nearPt, farPt;
   if (!unproject(Point3F(global.x, global.y, 0.0f), &nearPt) ||
       !unproject(Point3F(global.x, global.y, 1.0f), &farPt))
      return false;

   RayInfo ray;
   if (gClientContainer.castRay(nearPt, farPt, TerrainObjectType | InteriorObjectType |
                                StaticShapeObjectType | WaterObjectType, &ray))
   {
      *out = ray.point;
      return true;
   }

   // Off the terrain: the plane of the lowest ground.
   const F32 ground = groundHeight();
   const Point3F dir = farPt - nearPt;
   if (mFabs(dir.z) < 0.0001f)
      return false;
   const F32 t = (ground - nearPt.z) / dir.z;
   *out = nearPt + dir * t;
   return true;
}

//----------------------------------------------------------------------------
// What is drawn

CommanderIconData *GuiCommanderMap::iconFor(const char *name) const
{
   CommanderIconData *icon = NULL;
   if (name && name[0])
      Sim::findObject(name, icon);
   return icon;
}

void GuiCommanderMap::setTypeVisible(U32 type, bool visible)
{
   for (S32 i = 0; i < mHiddenTypes.size(); i++)
      if (mHiddenTypes[i] == type)
      {
         if (visible)
            mHiddenTypes.erase(i);
         return;
      }

   if (!visible)
      mHiddenTypes.push_back(type);
}

/// Everything the team knows of that its datablock files on the map, then
/// this client's own waypoints and tasks.
void GuiCommanderMap::collectIcons()
{
   mIcons.clear();

   GameConnection *conn = GameConnection::getConnectionToServer();
   GameBase *control = conn ? conn->getControlObject() : NULL;
   const U32 mine = T2Targets::clientSensorGroup();

   for (S32 target = TargetManagerT2::TeamTargets; target < TargetManagerT2::MaxTargets; target++)
   {
      ShapeBase *shape = ghostOf(target);
      if (!shape || !isKnown(target))
         continue;

      if (shape->getTypeMask() & CorpseObjectType)
         continue;
      // A player in a vehicle is shown by the vehicle.
      const bool isPlayer = (shape->getTypeMask() & PlayerObjectType) != 0;
      if (isPlayer && shape->getObjectMount())
         continue;

      ShapeBaseData *data = dynamic_cast<ShapeBaseData *>(shape->getDataBlock());
      if (!data)
         continue;

      CommanderIconData *icon = dynamic_cast<CommanderIconData *>(data->cmdIcon);
      if (!icon)
      {
         if (!data->cmdCategory || !data->cmdCategory[0])
            continue;
         icon = iconFor(mDefaultIconName);
      }

      bool hidden = false;
      for (S32 i = 0; i < mHiddenTypes.size(); i++)
         hidden |= mHiddenTypes[i] == T2Targets::typeTag(target);
      if (hidden)
         continue;

      Icon entry;
      entry.ref = CommanderRef(target);
      entry.pos = shape->getRenderWorldBox().getCenter();
      entry.data = icon;
      entry.teamColor = T2Targets::groupColor(mine, T2Targets::sensorGroup(target));
      entry.shape = shape;
      entry.isPlayer = isPlayer && shape == control;
      mIcons.push_back(entry);
   }

   const Vector<ClientTarget *> &own = ClientTarget::typed();
   for (S32 i = 0; i < own.size(); i++)
   {
      ClientTarget *local = own[i];
      bool hidden = false;
      for (S32 j = 0; j < mHiddenTypes.size(); j++)
         hidden |= mHiddenTypes[j] == local->getType();
      if (hidden)
         continue;

      const char *name = NULL;
      if (local->getType() == ClientTarget::typeId(ClientTarget::WayPoint))
         name = mWaypointIconName;
      else if (local->getType() == ClientTarget::typeId(ClientTarget::AssignedTask))
         name = mAssignedTaskIconName;
      else if (local->getType() == ClientTarget::typeId(ClientTarget::PotentialTask))
         name = mPotentialTaskIconName;

      Icon entry;
      entry.ref = CommanderRef(-1, local->getId());
      entry.pos = local->getPosition();
      entry.data = iconFor(name);
      entry.teamColor = T2Targets::groupColor(mine, mine);
      entry.shape = NULL;
      entry.isPlayer = false;
      if (entry.data)
         mIcons.push_back(entry);
   }

   // Where each lands on the screen, nearest the camera last so it is on top.
   for (S32 i = 0; i < mIcons.size(); i++)
   {
      Icon &icon = mIcons[i];
      Point2F at;
      const S32 size = iconSize(icon.pos);
      if (worldToScreen(icon.pos, &at))
      {
         const Point2I local = globalToLocalCoord(Point2I(S32(at.x), S32(at.y)));
         icon.rect.set(local.x - size / 2, local.y - size / 2, size, size);
      }
      else
         icon.rect.set(-100000, -100000, 0, 0);
   }
}

/// iconProjLen metres at an icon's depth, kept between minIconSize and
/// maxIconSize (0x65aab9).
S32 GuiCommanderMap::iconSize(const Point3F &pos) const
{
   const F32 pixels = F32(getWidth()) / viewWidthAt(pos.z) * mIconProjLen;
   return S32(mClampF(pixels, F32(mMinIconSize), F32(mMaxIconSize)));
}

/// How much of the view iconProjLen covers, from minDistanceScale to
/// maxDistanceScale (0x65ab89): every label shows once it is all the way.
F32 GuiCommanderMap::textFade() const
{
   const F32 share = mIconProjLen / viewWidthAt(groundHeight());
   if (share <= mMinDistanceScale)
      return 0.0f;
   if (share >= mMaxDistanceScale)
      return 1.0f;
   return (share - mMinDistanceScale) / (mMaxDistanceScale - mMinDistanceScale);
}

/// Of the icons under a point, the one whose centre is nearest it
/// (0x65a810), so a base's icons each answer to their own middle however
/// the players stood at them overlap.
const GuiCommanderMap::Icon *GuiCommanderMap::iconAt(const Point2I &local) const
{
   const Icon *nearest = NULL;
   S32 nearestDist = 0;
   for (S32 i = 0; i < mIcons.size(); i++)
   {
      const RectI &rect = mIcons[i].rect;
      if (!rect.pointInRect(local))
         continue;

      const Point2I off = rect.point + rect.extent / 2 - local;
      const S32 dist = off.x * off.x + off.y * off.y;
      if (!nearest || dist < nearestDist)
      {
         nearest = &mIcons[i];
         nearestDist = dist;
      }
   }
   return nearest;
}

void GuiCommanderMap::fillPolygon(const Point2F *points, U32 count, const ColorI &color)
{
   GFX->setStateBlock(mBlendBlock);
   GFX->setupGenericShaders(GFXDevice::GSColor);
   PrimBuild::begin(GFXTriangleFan, count);
   PrimBuild::color(color);
   for (U32 i = 0; i < count; i++)
      PrimBuild::vertex2f(points[i].x, points[i].y);
   // Untextured: PrimBuild's own shader would modulate by whatever texture
   // was bound last.
   PrimBuild::end(false);
}

void GuiCommanderMap::renderMissionArea(const RectI &bounds)
{
   RectI area = MissionArea::smMissionArea;
   if (area.extent.x <= 0 || area.extent.y <= 0)
      return;

   // Framed at the height of the tallest thing in it, as retail does.
   const F32 z = mTopZ;
   Point2F lo, hi;
   if (!worldToScreen(Point3F(area.point.x, area.point.y + area.extent.y, z), &lo) ||
       !worldToScreen(Point3F(area.point.x + area.extent.x, area.point.y, z), &hi))
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const F32 l = bounds.point.x, t = bounds.point.y;
   const F32 r = bounds.point.x + bounds.extent.x, b = bounds.point.y + bounds.extent.y;
   const F32 x0 = mClampF(lo.x, l, r), x1 = mClampF(hi.x, l, r);
   const F32 y0 = mClampF(lo.y, t, b), y1 = mClampF(hi.y, t, b);

   // Everything outside the area is dimmed.
   draw->drawRectFill(RectF(l, t, r - l, y0 - t), mMissionAreaFillColor);
   draw->drawRectFill(RectF(l, y1, r - l, b - y1), mMissionAreaFillColor);
   draw->drawRectFill(RectF(l, y0, x0 - l, y1 - y0), mMissionAreaFillColor);
   draw->drawRectFill(RectF(x1, y0, r - x1, y1 - y0), mMissionAreaFillColor);

   draw->drawRect(Point2F(lo.x, lo.y), Point2F(hi.x, hi.y), mMissionAreaFrameColor);
}

/// Each of the team's sensors, ringed at its reach in its datablock's
/// colour.
void GuiCommanderMap::renderSensors(const RectI &bounds)
{
   const U32 mine = T2Targets::clientSensorGroup();
   const U32 segments = mClamp(mSensorSphereDetailLevel, 8, 256);

   for (S32 i = 0; i < mIcons.size(); i++)
   {
      const Icon &icon = mIcons[i];
      if (!icon.shape || T2Targets::sensorGroup(icon.ref.target) != mine)
         continue;

      ShapeBaseData *data = static_cast<ShapeBaseData *>(icon.shape->getDataBlock());
      if (!data || data->sensorRadius <= 0)
         continue;

      Point2F centre;
      if (!worldToScreen(icon.pos, &centre))
         continue;

      const F32 radius = F32(getWidth()) / viewWidthAt(icon.pos.z) * data->sensorRadius;

      Vector<Point2F> ring;
      ring.push_back(centre);
      for (U32 s = 0; s <= segments; s++)
      {
         const F32 a = M_2PI_F * s / segments;
         ring.push_back(Point2F(centre.x + mCos(a) * radius, centre.y + mSin(a) * radius));
      }

      ColorI fill = data->sensorColor;
      fill.alpha = mSensorSphereFillAlpha;
      fillPolygon(ring.address(), ring.size(), fill);

      ColorI frame = data->sensorColor;
      frame.alpha = mSensorSphereFrameAlpha;
      GFXDrawUtil *draw = GFX->getDrawUtil();
      for (U32 s = 1; s < ring.size() - 1; s++)
         draw->drawLine(ring[s], ring[s + 1], frame);
   }
}

void GuiCommanderMap::renderIcon(const Icon &icon)
{
   if (!icon.data)
      return;

   const RectI rect(localToGlobalCoord(icon.rect.point), icon.rect.extent);
   const U32 time = Platform::getVirtualMilliseconds();
   const bool selected = isSelected(icon.ref);
   const bool hilit = mHilight == icon.ref;

   const CommanderIconData::Image &base = icon.data->getImage(CommanderIconData::Base);
   const CommanderIconData::Image &sel = icon.data->getImage(CommanderIconData::Select);
   const CommanderIconData::Image &hil = icon.data->getImage(CommanderIconData::Hilight);

   const CommanderIconData::Image *over = NULL;
   ColorI overColor;
   if (selected && sel.valid())
   {
      over = &sel;
      overColor = mSelectedObjectColor;
   }
   else if (hilit && hil.valid())
   {
      over = &hil;
      overColor = mHilightedObjectColor;
   }

   GFXDrawUtil *draw = GFX->getDrawUtil();
   if (base.valid() && (!over || over->overlay))
   {
      if (base.modulate)
         draw->setBitmapModulation(icon.teamColor);
      else
         draw->clearBitmapModulation();
      draw->drawBitmapStretch(base.frame(time), rect, GFXBitmapFlip_None, GFXTextureFilterLinear);
   }

   if (over)
   {
      if (over->modulate)
         draw->setBitmapModulation(overColor);
      else
         draw->clearBitmapModulation();
      draw->drawBitmapStretch(over->frame(time), rect, GFXBitmapFlip_None, GFXTextureFilterLinear);
   }

   draw->clearBitmapModulation();
}

/// The control object's heading, as a wedge off its icon.
void GuiCommanderMap::renderPlayerMarker(const Icon &icon)
{
   if (!icon.shape)
      return;

   VectorF fwd;
   icon.shape->getRenderTransform().getColumn(1, &fwd);
   Point2F dir(fwd.x, -fwd.y);
   if (dir.lenSquared() < 0.0001f)
      return;
   dir.normalize();

   const Point2I c = localToGlobalCoord(icon.rect.point + icon.rect.extent / 2);
   const Point2F centre(c.x, c.y);
   const F32 reach = icon.rect.extent.x * 0.5f + mPlayerMarkerOffset;
   const Point2F side(-dir.y, dir.x);
   const F32 half = mPlayerMarkerLen * mTan(mDegToRad(mPlayerMarkerAngle));

   const Point2F base = centre + dir * reach;
   Point2F tri[3] =
   {
      base + dir * mPlayerMarkerLen,
      base + side * half,
      base - side * half,
   };
   fillPolygon(tri, 3, mPlayerMarkerColor);
}

/// A selected object out of view gets an arrow at the edge pointing to it.
void GuiCommanderMap::renderEdgeMarker(const Icon &icon, const RectI &bounds)
{
   const Point2I mid = icon.rect.point + icon.rect.extent / 2;
   if (getBounds().extent.x <= 0 || RectI(Point2I(0, 0), getExtent()).pointInRect(mid))
      return;

   const Point2F centre(bounds.point.x + bounds.extent.x * 0.5f, bounds.point.y + bounds.extent.y * 0.5f);
   Point2F target;
   if (!worldToScreen(icon.pos, &target))
   {
      // Behind the camera cannot happen looking down; anything else is
      // simply off to one side.
      target = Point2F(localToGlobalCoord(mid).x, localToGlobalCoord(mid).y);
   }

   Point2F dir = target - centre;
   if (dir.lenSquared() < 1.0f)
      return;
   dir.normalize();

   const F32 hw = bounds.extent.x * 0.5f - mEdgeMarkerSize.x * 0.5f;
   const F32 hh = bounds.extent.y * 0.5f - mEdgeMarkerSize.y * 0.5f;
   const F32 sx = mFabs(dir.x) > 0.0001f ? hw / mFabs(dir.x) : F32_MAX;
   const F32 sy = mFabs(dir.y) > 0.0001f ? hh / mFabs(dir.y) : F32_MAX;
   const Point2F at = centre + dir * getMin(sx, sy);

   // The arrow art points up.
   const Point2F side(-dir.y, dir.x);
   const F32 w = mEdgeMarkerSize.x * 0.5f, h = mEdgeMarkerSize.y * 0.5f;
   const Point2F up = dir * h, across = side * w;

   if (mEdgeMarker)
   {
      GFXStateBlockDesc desc;
      desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      desc.setZReadWrite(false);
      desc.zWriteEnable = false;
      desc.setCullMode(GFXCullNone);
      desc.samplersDefined = true;
      desc.samplers[0] = GFXSamplerStateDesc::getClampLinear();
      GFX->setStateBlockByDesc(desc);
      GFX->setTexture(0, mEdgeMarker);
      GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

      PrimBuild::begin(GFXTriangleFan, 4);
      PrimBuild::color(mSelectedObjectColor);
      PrimBuild::texCoord2f(0, 0); Point2F p = at + up - across; PrimBuild::vertex2f(p.x, p.y);
      PrimBuild::texCoord2f(1, 0); p = at + up + across;          PrimBuild::vertex2f(p.x, p.y);
      PrimBuild::texCoord2f(1, 1); p = at - up + across;          PrimBuild::vertex2f(p.x, p.y);
      PrimBuild::texCoord2f(0, 1); p = at - up - across;          PrimBuild::vertex2f(p.x, p.y);
      PrimBuild::end();
      GFX->setTexture(0, NULL);
   }
   else
   {
      Point2F tri[3] = { at + up, at - up + across, at - up - across };
      fillPolygon(tri, 3, mSelectedObjectColor);
   }
}

void GuiCommanderMap::renderGui(Point2I offset, const RectI &updateRect)
{
   const RectI bounds(offset, getExtent());
   GFXDrawUtil *draw = GFX->getDrawUtil();

   collectIcons();

   if (mRenderMissionArea)
      renderMissionArea(bounds);

   if (mRenderSensors)
      renderSensors(bounds);

   for (S32 i = 0; i < mIcons.size(); i++)
      renderIcon(mIcons[i]);

   for (S32 i = 0; i < mIcons.size(); i++)
      if (mIcons[i].isPlayer)
         renderPlayerMarker(mIcons[i]);

   // Labels under the icons: every one once near enough, otherwise only
   // what is selected or under the mouse.
   GFont *font = mProfile ? mProfile->mFont : NULL;
   if (mRenderText && font)
   {
      const bool all = textFade() >= 1.0f;
      for (S32 i = 0; i < mIcons.size(); i++)
      {
         const Icon &icon = mIcons[i];
         if (!all && !isSelected(icon.ref) && mHilight != icon.ref)
            continue;

         char name[256];
         name[0] = 0;
         if (ClientTarget *local = icon.ref.getLocal())
            dStrncpy(name, local->getText(), sizeof(name));
         else
            T2Targets::getTargetName(icon.ref.target, name, sizeof(name));
         name[sizeof(name) - 1] = 0;
         if (!name[0])
            continue;

         const Point2I at = localToGlobalCoord(icon.rect.point);
         const S32 width = font->getStrWidth(name);
         draw->setBitmapModulation(ColorI(255, 255, 255, 255));
         draw->drawText(font, Point2I(at.x + (icon.rect.extent.x - width) / 2,
                                      at.y + icon.rect.extent.y + mTextOffset), name);
      }
      draw->clearBitmapModulation();
   }

   if (mEnableEdgeMarkers)
      for (S32 i = 0; i < mIcons.size(); i++)
         if (isSelected(mIcons[i].ref))
            renderEdgeMarker(mIcons[i], bounds);

   if (mDragSelect)
   {
      const Point2I a = localToGlobalCoord(mMouseDown), b = localToGlobalCoord(mMousePos);
      draw->drawRect(Point2I(getMin(a.x, b.x), getMin(a.y, b.y)),
                     Point2I(getMax(a.x, b.x), getMax(a.y, b.y)), mMouseSelectRectColor);
   }
}

//----------------------------------------------------------------------------
// Selection

bool GuiCommanderMap::isSelected(const CommanderRef &ref) const
{
   for (S32 i = 0; i < mSelection.size(); i++)
      if (mSelection[i] == ref)
         return true;
   return false;
}

void GuiCommanderMap::onSelectCallback(const CommanderRef &ref, bool on)
{
   S32 target = ref.target;
   U32 name = 0, type = 0;
   if (ClientTarget *local = ref.getLocal())
   {
      target = local->getTargetId();
      type = local->getType();
   }
   else
   {
      name = T2Targets::nameTag(ref.target);
      type = T2Targets::typeTag(ref.target);
   }

   Con::executef(this, "onSelect", Con::getIntArg(target), Con::getIntArg(name),
                 Con::getIntArg(type), Con::getIntArg(on));
}

void GuiCommanderMap::select(const CommanderRef &ref, bool on)
{
   if (!ref.isValid() || isSelected(ref) == on)
      return;

   if (on)
   {
      mSelection.push_back(ref);
      mLastSelected = ref;
   }
   else
      for (S32 i = 0; i < mSelection.size(); i++)
         if (mSelection[i] == ref)
         {
            mSelection.erase(i);
            break;
         }

   onSelectCallback(ref, on);
}

void GuiCommanderMap::clearSelection()
{
   while (mSelection.size())
      select(mSelection.last(), false);
}

void GuiCommanderMap::selectOnly(const CommanderRef &ref)
{
   for (S32 i = mSelection.size() - 1; i >= 0; i--)
      if (mSelection[i] != ref)
         select(mSelection[i], false);
   select(ref, true);
}

void GuiCommanderMap::selectControlObject()
{
   GameConnection *conn = GameConnection::getConnectionToServer();
   GameBase *control = conn ? conn->getControlObject() : NULL;
   if (!control || control->getT2Target() < 0)
      return;

   // A player in a vehicle is selected as the vehicle it is in.
   ShapeBase *shape = dynamic_cast<ShapeBase *>(control);
   GameBase *mount = shape ? dynamic_cast<GameBase *>(shape->getObjectMount()) : NULL;
   if (mount && mount->getT2Target() >= 0)
      control = mount;

   selectOnly(CommanderRef(control->getT2Target()));
}

void GuiCommanderMap::issueCommand(const CommanderRef &ref, const Point2I &globalPoint)
{
   ClientTarget *target = ref.getLocal();
   S32 group = -1;
   U32 name = 0;

   if (!target)
   {
      target = new ClientTarget;
      target->setServerTarget(ref.target);
      if (ref.target >= 0)
      {
         target->setType(T2Targets::typeTag(ref.target));
         group = T2Targets::sensorGroup(ref.target);
         name = T2Targets::nameTag(ref.target);
         if (ShapeBase *shape = ghostOf(ref.target))
            target->setPosition(shape->getRenderPosition());
      }
      else
      {
         target->setType(ClientTarget::typeId(ClientTarget::Location));
         Point3F ground;
         if (screenToGround(globalToLocalCoord(globalPoint), &ground))
            target->setPosition(ground);
      }

      if (!target->registerObject())
      {
         delete target;
         return;
      }
   }

   Con::executef(this, "issueCommand", Con::getIntArg(target->getId()),
                 Con::getIntArg(target->getType()), Con::getIntArg(name),
                 Con::getIntArg(group),
                 avar("%d %d", globalPoint.x, globalPoint.y));
}

//----------------------------------------------------------------------------
// The mouse

void GuiCommanderMap::onMouseDown(const GuiEvent &event)
{
   const Point2I local = globalToLocalCoord(event.mousePoint);
   mMouseDown = mMousePos = local;
   mDragSelect = false;

   if (mMouseMode == ZoomMode)
   {
      Point3F ground;
      if (screenToGround(local, &ground))
      {
         mFollow = CommanderRef();
         const F32 height = mCamera.z - mCameraMin.z;
         zoomTo(Point3F(ground.x, ground.y, mCameraMin.z + height * 0.5f));
      }
      return;
   }

   mDragging = true;
   mouseLock();

   if (mMouseMode == MoveMode)
   {
      mFollow = CommanderRef();
      screenToGround(local, &mDragGrab);
   }
}

void GuiCommanderMap::onMouseDragged(const GuiEvent &event)
{
   const Point2I local = globalToLocalCoord(event.mousePoint);
   mMousePos = local;
   if (!mDragging)
      return;

   if (mMouseMode == MoveMode)
   {
      // Keep the ground grabbed under the cursor.
      const F32 metres = viewWidthAt(mDragGrab.z) / F32(getMax(getWidth(), 1));
      const Point2I moved = local - mMouseDown;
      mCamera.x -= moved.x * metres;
      mCamera.y += moved.y * metres;
      mMouseDown = local;
      clampCamera();
   }
   else if (mMouseMode == SelectMode)
   {
      const Point2I moved = local - mMouseDown;
      if (mAbs(moved.x) > 3 || mAbs(moved.y) > 3)
         mDragSelect = true;
   }
}

void GuiCommanderMap::onMouseUp(const GuiEvent &event)
{
   const Point2I local = globalToLocalCoord(event.mousePoint);
   mMousePos = local;
   if (!mDragging)
      return;

   mDragging = false;
   mouseUnlock();

   if (mMouseMode != SelectMode)
      return;

   const bool add = (event.modifier & SI_SHIFT) != 0;
   const bool remove = (event.modifier & SI_CTRL) != 0;

   if (mDragSelect)
   {
      mDragSelect = false;
      const RectI box(Point2I(getMin(mMouseDown.x, local.x), getMin(mMouseDown.y, local.y)),
                      Point2I(mAbs(local.x - mMouseDown.x) + 1, mAbs(local.y - mMouseDown.y) + 1));

      if (!add && !remove)
         clearSelection();
      for (S32 i = 0; i < mIcons.size(); i++)
      {
         const Point2I mid = mIcons[i].rect.point + mIcons[i].rect.extent / 2;
         if (box.pointInRect(mid))
            select(mIcons[i].ref, !remove);
      }
      return;
   }

   const Icon *icon = iconAt(local);
   if (!icon)
   {
      if (!add && !remove)
         clearSelection();
      return;
   }

   const CommanderRef ref = icon->ref;
   if (remove)
      select(ref, false);
   else if (add)
      select(ref, !isSelected(ref));
   else
      selectOnly(ref);
}

void GuiCommanderMap::onMouseMove(const GuiEvent &event)
{
   mMousePos = globalToLocalCoord(event.mousePoint);
   mMouseInside = true;
   const Icon *icon = mMouseMode == SelectMode ? iconAt(mMousePos) : NULL;
   mHilight = icon ? icon->ref : CommanderRef();
}

void GuiCommanderMap::onMouseLeave(const GuiEvent &event)
{
   mMouseInside = false;
   mHilight = CommanderRef();
   Parent::onMouseLeave(event);
}

void GuiCommanderMap::onRightMouseDown(const GuiEvent &event)
{
   const Point2I local = globalToLocalCoord(event.mousePoint);

   if (mMouseMode == ZoomMode)
   {
      mFollow = CommanderRef();
      const F32 height = mCamera.z - mCameraMin.z;
      zoomTo(Point3F(mCamera.x, mCamera.y, mCameraMin.z + getMax(height, 1.0f) * 2.0f));
      return;
   }

   const Icon *icon = iconAt(local);
   issueCommand(icon ? icon->ref : CommanderRef(), event.mousePoint);
}

bool GuiCommanderMap::onMouseWheelUp(const GuiEvent &event)
{
   const F32 height = mCamera.z - mCameraMin.z;
   zoomTo(Point3F(mCamera.x, mCamera.y, mCameraMin.z + height * 0.75f));
   return true;
}

bool GuiCommanderMap::onMouseWheelDown(const GuiEvent &event)
{
   const F32 height = mCamera.z - mCameraMin.z;
   zoomTo(Point3F(mCamera.x, mCamera.y, mCameraMin.z + getMax(height, 1.0f) / 0.75f));
   return true;
}

void GuiCommanderMap::getCursor(GuiCursor *&cur, bool &showCursor, const GuiEvent &lastGuiEvent)
{
   U32 which = ArrowCursor;
   switch (mMouseMode)
   {
      case MoveMode:
         which = mDragging ? MoveCursor : HandCursor;
         break;
      case ZoomMode:
         which = ZoomCursor;
         break;
      default:
         if (lastGuiEvent.modifier & SI_SHIFT)
            which = AddCursor;
         else if (lastGuiEvent.modifier & SI_CTRL)
            which = RemoveCursor;
         break;
   }

   GuiCursor *found = cursor(which);
   if (found)
      cur = found;
   showCursor = true;
}

//----------------------------------------------------------------------------

static const char *sDirectionNames[GuiCommanderMap::NumDirections] =
{
   "left", "right", "up", "down", "in", "out",
};

DefineEngineMethod( GuiCommanderMap, cameraMove, void, (const char *dir, bool on), ,
   "commanderMap.cameraMove(<left,right,up,down,in,out>, <bool>);" )
{
   for (U32 i = 0; i < GuiCommanderMap::NumDirections; i++)
      if (!dStricmp(dir, sDirectionNames[i]))
      {
         object->cameraMove(i, on);
         return;
      }
}

DefineEngineMethod( GuiCommanderMap, setTargetTypeVisible, void, (S32 type, bool visible), ,
   "commanderMap.setTargetTypeVisible(type, bool)" )
{
   object->setTypeVisible(U32(type), visible);
}

DefineEngineMethod( GuiCommanderMap, followLastSelected, void, (), ,
   "commanderMap.followLastSelected();" )
{
   object->followLastSelected();
}

DefineEngineMethod( GuiCommanderMap, resetCamera, void, (), ,
   "commanderMap.resetCamera()" )
{
   object->resetCamera();
}

DefineEngineMethod( GuiCommanderMap, getMouseMode, const char *, (), ,
   "commanderMap.getMouseMode()" )
{
   switch (object->getMouseMode())
   {
      case GuiCommanderMap::MoveMode: return "move";
      case GuiCommanderMap::ZoomMode: return "zoom";
      default:                        return "select";
   }
}

DefineEngineMethod( GuiCommanderMap, setMouseMode, void, (const char *mode), ,
   "commanderMap.setMouseMode(mode)" )
{
   if (!dStricmp(mode, "select"))
      object->setMouseMode(GuiCommanderMap::SelectMode);
   else if (!dStricmp(mode, "move"))
      object->setMouseMode(GuiCommanderMap::MoveMode);
   else if (!dStricmp(mode, "zoom"))
      object->setMouseMode(GuiCommanderMap::ZoomMode);
   else
      Con::errorf("GuiCommanderMap::cSetMouseMode: invalid mode '%s'", mode);
}

DefineEngineMethod( GuiCommanderMap, selectControlObject, void, (), ,
   "commanderMap.selectControlObject()" )
{
   object->selectControlObject();
}

DefineEngineMethod( GuiCommanderMap, selectClientTarget, void, (const char *target, bool select), ,
   "commanderMap.selectClientTarget(clientTarget, select)" )
{
   ClientTarget *local = NULL;
   if (!Sim::findObject(target, local))
   {
      Con::errorf("GuiCommanderMap::cSelectClientTarget: not a clientTarget %s", target);
      return;
   }
   object->select(CommanderRef(-1, local->getId()), select);
}

