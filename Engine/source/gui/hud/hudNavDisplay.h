// HH: Added for Tribes 2 compatibility.
#ifndef _HUDNAVDISPLAY_H_
#define _HUDNAVDISPLAY_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif

class GameConnection;
class GuiTSCtrl;
class SceneObject;
class ShapeBase;

/// @file
/// Knowing where things are.
///
/// The compass is a strip of headings that slides under a mark as the player
/// turns.  The nav display is the layer over the whole view that puts a marker
/// on everything worth seeing -- team-mates, enemies, vehicles, objectives,
/// incoming missiles -- and an arrow at the edge of the screen for the ones
/// behind you.
///
/// Both need the camera's heading, and on a Tribes 2 connection that is not
/// the server's: the server echoes back the angles the client sent it a round
/// trip ago, so what the view is actually using is what the move stream has
/// accumulated.  Reading it off the control object is the same answer the
/// renderer got.

//-----------------------------------------------------------------------------
class HudCompass : public HudCtrl
{
   typedef HudCtrl Parent;

   ColorI mTextColor;

public:
   HudCompass();

   DECLARE_CONOBJECT(HudCompass);
   static void initPersistFields();

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The markers over the world.
///
/// Every shape whose target the server has given a render mask gets the hud
/// images its datablock names for each bit of it: the triangle over a
/// player's head, the flag icon over a flag and its carrier.  Mission
/// waypoints get a marker of their own.  Whatever is off the screen is drawn
/// where the way to it crosses the edge, and the shape under the crosshair
/// gets its name and a health bar beside the centre of the screen.
///
/// Which kinds are showing at all is script's: setMarkerTypeVisible is what
/// the options screen drives.
class HudNavDisplay : public HudCtrl
{
   typedef HudCtrl Parent;

public:
   enum MarkerType
   {
      MissionWaypoint,
      PotentialTask,
      AssignedTask,
      ClientWaypoint,
      Target,
      Shape,
      NumMarkerTypes,
   };

   enum { NumMarkerImages = 6 };

private:
   enum EntryFlags
   {
      Acquired   = BIT(0),   ///< under the crosshair: gets the name and bar
      Always     = BIT(1),   ///< drawn sensed or not
      InSight    = BIT(2),   ///< nothing stands between the eye and it
      IsPlayer   = BIT(3),
      Acquirable = BIT(4),
   };

   struct Entry
   {
      MarkerType  type;
      U32         flags;
      Point3F     pos;
      SceneObject *obj;
      F32         health;
      F32         dist;
   };

   Vector<Entry> mEntries;

   bool mMarkerTypeVisible[NumMarkerTypes];

   /// The sensor group the server put this client in, as of this frame.
   U32 mSensorGroup;

   StringTableEntry mMarkerImageNames[NumMarkerImages];
   GFXTexHandle     mMarkerImages[NumMarkerImages];

   ColorI mDefaultMarkerColor;
   ColorI mFriendBeaconColor;
   ColorI mEnemyBeaconColor;
   ColorI mVehicleBeaconColor;
   ColorI mDamageFrameColor;
   ColorI mTargetLockedFrameColor;

   StringTableEntry mMissileMarker1;
   StringTableEntry mMissileMarker2;
   StringTableEntry mMissileFlash1;
   StringTableEntry mMissileFlash2;
   StringTableEntry mMissileTargetRing;
   StringTableEntry mMissileMarkerHilite;

   S32     mImageOffset;
   Point2I mInfoOffset;
   Point2I mDamageRectSize;

   F32 mMarkerAngle;
   F32 mMarkerLen;
   F32 mMarkerHitRadius;
   F32 mMarkerScaleDistance;
   F32 mMinMarkerScale;
   S32 mMinProjectSize;
   S32 mMarkerTextEdgeOffset;
   S32 mAcquireScreenRadius;
   F32 mAcquireBoxCheckDistance;
   F32 mPlayerEyeZOffset;

   bool mRenderEdgeMarkers;
   bool mRenderMarkerText;
   bool mRenderXYProjection;
   bool mProtectedStatics;

   void loadMarkerImages();

   void collect(GameConnection *conn, ShapeBase *control,
                const Point3F &camPos, const Point3F &fwd, F32 fov,
                F32 visDist);
   void collectShapes(GameConnection *conn, ShapeBase *control,
                      const Point3F &camPos, const Point3F &fwd, F32 fov,
                      F32 visDist, F32 acquireAngle);

   bool edgePoint(const GuiTSCtrl *ts, const MatrixF &cam,
                  const Point3F &dir, const Point2I &offset,
                  Point2I *at) const;

   void drawMarker(const GuiTSCtrl *ts, const Entry &e, Point2I at,
                   bool onEdge, const Point2I &offset);
   void drawTargetInfo(const Entry &e, const Point2I &offset);

public:
   HudNavDisplay();

   DECLARE_CONOBJECT(HudNavDisplay);
   static void initPersistFields();

   bool onWake();
   void onSleep();
   void onPreRender();
   void inspectPostApply();

   bool isMarkerTypeVisible(S32 type) const;
   void setMarkerTypeVisible(S32 type, bool visible);

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDNAVDISPLAY_H_
