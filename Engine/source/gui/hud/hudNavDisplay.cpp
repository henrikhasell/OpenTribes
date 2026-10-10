#include "gui/hud/hudNavDisplay.h"

#include "T3D/gameBase/gameConnection.h"
#include "T3D/shapeBase.h"
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

void HudCompass::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   // No panel behind it: HudCompass::onRender at 0x00502870 draws the
   // headings and the mark and nothing else, and the .gui's fillColor of
   // "0.25 0.25 0.25 0.25" was a grey box over the dial underneath.
   F32 yaw;
   if(!cameraYaw(yaw))
      return;

   // The strip shows a quarter turn either side of where the player is
   // looking, so a heading slides across as they turn and the mark in the
   // middle is what they are facing.
   const F32 span = M_PI_F;
   const F32 pixelsPerRadian = F32(getExtent().x) / span;

   static const char *sPoints[] = { "N", "E", "S", "W" };

   if(mProfile->mFont)
   {
      draw->setBitmapModulation(faded(mTextColor));

      for(U32 i = 0; i < 4; i++)
      {
         // Where this point of the compass sits relative to the heading,
         // wrapped into -pi..pi so it comes round rather than running off.
         F32 delta = F32(i) * M_PI_F / 2.f - yaw;
         while(delta >  M_PI_F) delta -= M_2PI_F;
         while(delta < -M_PI_F) delta += M_2PI_F;

         const S32 x = offset.x + getExtent().x / 2 + S32(delta * pixelsPerRadian);
         const S32 w = mProfile->mFont->getStrWidth((const UTF8*)sPoints[i]);

         if(x - w / 2 < offset.x || x + w / 2 > offset.x + getExtent().x)
            continue;

         draw->drawText(mProfile->mFont, Point2I(x - w / 2, offset.y), sPoints[i]);
      }

      draw->clearBitmapModulation();
   }

   // The mark the headings slide under.
   const S32 mid = offset.x + getExtent().x / 2;
   draw->drawLine(mid, offset.y, mid, offset.y + getExtent().y, faded(mFrameColor));

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudNavDisplay::HudNavDisplay()
{
   for(U32 i = 0; i < MaxMarkerTypes; i++)
      mMarkerTypeVisible[i] = true;

   mDefaultMarkerColor.set(255, 255, 255, 255);
   mFriendBeaconColor.set(255, 255, 0, 255);
   mEnemyBeaconColor.set(0, 255, 0, 255);
   mVehicleBeaconColor.set(255, 0, 0, 255);
   mDamageFrameColor.set(255, 0, 0, 255);
   mTargetLockedFrameColor.set(255, 0, 0, 255);

   mMissileMarker1 = StringTable->insert("");
   mMissileMarker2 = StringTable->insert("");
   mMissileFlash1  = StringTable->insert("");
   mMissileFlash2  = StringTable->insert("");

   mImageOffset.set(0, 0);
   mInfoOffset.set(0, 0);
   mDamageRectSize.set(0, 0);

   mMarkerAngle = 0.f;
   mMarkerLen = 0.f;
   mMarkerHitRadius = 0.f;
   mMarkerScaleDistance = 0.f;
   mMinMarkerScale = 0.f;
   mMinProjectSize = 0.f;
   mMarkerTextEdgeOffset = 0.f;
   mAcquireScreenRadius = 0.f;
   mAcquireBoxCheckDistance = 0.f;
   mPlayerEyeZOffset = 0.f;
   mLOSMarkerUpdate = 0.f;

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

   addField("missileMarker1", TypeFilename, Offset(mMissileMarker1, HudNavDisplay));
   addField("missileMarker2", TypeFilename, Offset(mMissileMarker2, HudNavDisplay));
   addField("missileFlash1",  TypeFilename, Offset(mMissileFlash1, HudNavDisplay));
   addField("missileFlash2",  TypeFilename, Offset(mMissileFlash2, HudNavDisplay));

   addField("imageOffset",    TypePoint2I, Offset(mImageOffset, HudNavDisplay));
   addField("infoOffset",     TypePoint2I, Offset(mInfoOffset, HudNavDisplay));
   addField("damageRectSize", TypePoint2I, Offset(mDamageRectSize, HudNavDisplay));

   addField("markerAngle",             TypeF32, Offset(mMarkerAngle, HudNavDisplay));
   addField("markerLen",               TypeF32, Offset(mMarkerLen, HudNavDisplay));
   addField("markerHitRadius",         TypeF32, Offset(mMarkerHitRadius, HudNavDisplay));
   addField("markerScaleDistance",     TypeF32, Offset(mMarkerScaleDistance, HudNavDisplay));
   addField("minMarkerScale",          TypeF32, Offset(mMinMarkerScale, HudNavDisplay));
   addField("minProjectSize",          TypeF32, Offset(mMinProjectSize, HudNavDisplay));
   addField("markerTextEdgeOffset",    TypeF32, Offset(mMarkerTextEdgeOffset, HudNavDisplay));
   addField("acquireScreenRadius",     TypeF32, Offset(mAcquireScreenRadius, HudNavDisplay));
   addField("acquireBoxCheckDistance", TypeF32, Offset(mAcquireBoxCheckDistance, HudNavDisplay));
   addField("playerEyeZOffset",        TypeF32, Offset(mPlayerEyeZOffset, HudNavDisplay));
   addField("LOSMarkerUpdate",         TypeF32, Offset(mLOSMarkerUpdate, HudNavDisplay));

   addField("renderEdgeMarkers",  TypeBool, Offset(mRenderEdgeMarkers, HudNavDisplay));
   addField("renderMarkerText",   TypeBool, Offset(mRenderMarkerText, HudNavDisplay));
   addField("renderXYProjection", TypeBool, Offset(mRenderXYProjection, HudNavDisplay));
   addField("protectedStatics",   TypeBool, Offset(mProtectedStatics, HudNavDisplay));
}

bool HudNavDisplay::isMarkerTypeVisible(S32 type) const
{
   return (type >= 0 && type < MaxMarkerTypes) ? mMarkerTypeVisible[type] : false;
}

void HudNavDisplay::setMarkerTypeVisible(S32 type, bool visible)
{
   if(type >= 0 && type < MaxMarkerTypes)
   {
      mMarkerTypeVisible[type] = visible;
      setUpdate();
   }
}

void HudNavDisplay::onRender(Point2I offset, const RectI &updateRect)
{
   // What to mark is the target manager's list, and on a Tribes 2 connection
   // that list is built from TargetInfoEvent and TargetToEvent, which are read
   // off the wire but not yet kept -- see game/net/t2Events.cc.  So this draws
   // no markers yet.
   //
   // It also draws no fill, and that is a fix rather than an omission.  This
   // control is `navHud`, and PlayGui gives it position 0 0 and extent
   // 640x480 -- the whole screen at the resolution that layout was drawn for.
   // Filling it paints a wash over everything, and because `horizSizing` holds
   // the width rather than stretching it, at 1920x1142 that wash becomes a
   // hard-edged rectangle across the upper-left: the sky seam t2w5 filed, with
   // nothing in the log because nothing had failed.
   //
   // The fillColor it carries is not a decision about this control. Every Hud
   // control in PlayGui carries the same "0.25 0.25 0.25 0.25", 107 of them,
   // the way a .gui carries every field of every class it writes. A full-screen
   // marker overlay is not a panel, and retail draws no wash: measured at the
   // matched oracle camera, where the strongest edge anywhere in its sky is
   // 3.8 against our 18.2 and columns 637..641 are smooth.
   //
   // Tools/swarm/t2navseam.cs builds this one control over a local scene and
   // reproduces it with no server at all -- alpha 0.127 against the 0.25 x 0.5
   // the layout asks for, grey 65 against 0.25 x 255.
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudNavDisplay, isMarkerTypeVisible, bool, (S32 type), ,
   "@brief Whether markers of this type are drawn.\n\n@ingroup GuiHud")
{
   return object->isMarkerTypeVisible(type);
}

DefineEngineMethod(HudNavDisplay, setMarkerTypeVisible, void, (S32 type, bool visible), ,
   "@brief Draw markers of this type, or stop.\n\n@ingroup GuiHud")
{
   object->setMarkerTypeVisible(type, visible);
}
