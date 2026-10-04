// HH: Added for Tribes 2 compatibility.
#ifndef _HUDNAVDISPLAY_H_
#define _HUDNAVDISPLAY_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif

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
/// Thirty-five fields, because every kind of marker has its own colour, its
/// own icon and its own rule about when it stops being drawn.  Which kinds are
/// showing at all is script's: setMarkerTypeVisible is what the options screen
/// drives.
class HudNavDisplay : public HudCtrl
{
   typedef HudCtrl Parent;

   enum { MaxMarkerTypes = 32 };

   bool mMarkerTypeVisible[MaxMarkerTypes];

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

   Point2I mImageOffset;
   Point2I mInfoOffset;
   Point2I mDamageRectSize;

   F32 mMarkerAngle;
   F32 mMarkerLen;
   F32 mMarkerHitRadius;
   F32 mMarkerScaleDistance;
   F32 mMinMarkerScale;
   F32 mMinProjectSize;
   F32 mMarkerTextEdgeOffset;
   F32 mAcquireScreenRadius;
   F32 mAcquireBoxCheckDistance;
   F32 mPlayerEyeZOffset;
   F32 mLOSMarkerUpdate;

   bool mRenderEdgeMarkers;
   bool mRenderMarkerText;
   bool mRenderXYProjection;
   bool mProtectedStatics;

public:
   HudNavDisplay();

   DECLARE_CONOBJECT(HudNavDisplay);
   static void initPersistFields();

   bool isMarkerTypeVisible(S32 type) const;
   void setMarkerTypeVisible(S32 type, bool visible);

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDNAVDISPLAY_H_
