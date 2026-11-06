// HH: Added for Tribes 2 compatibility.
#ifndef _GUICOMMANDERMAP_H_
#define _GUICOMMANDERMAP_H_

#ifndef _GUITSCONTROL_H_
#include "gui/3d/guiTSControl.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class ClientTarget;
class CommanderIconData;
class GameBase;
class GuiCursor;
class ShapeBase;

/// What the commander map and its list both know an order's subject by: a
/// server target, or a target of this client's own.
struct CommanderRef
{
   S32 target;
   SimObjectId local;

   CommanderRef(S32 t = -1, SimObjectId l = 0) : target(t), local(l) {}
   bool isValid() const { return target >= 0 || local; }
   bool operator==(const CommanderRef &o) const { return target == o.target && local == o.local; }
   bool operator!=(const CommanderRef &o) const { return !(*this == o); }

   ClientTarget *getLocal() const;
};

/// The commander map: the mission seen from straight above, with an icon on
/// everything the team knows of, for picking things out and giving orders.
class GuiCommanderMap : public GuiTSCtrl
{
   typedef GuiTSCtrl Parent;

public:
   enum MouseMode { SelectMode, MoveMode, ZoomMode };

   GuiCommanderMap();

   static void initPersistFields();

   bool onAdd();
   void onRemove();
   bool onWake();
   void onSleep();

   bool processCameraQuery(CameraQuery *query);
   void renderWorld(const RectI &updateRect);
   void renderGui(Point2I offset, const RectI &updateRect);

   void onMouseDown(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseMove(const GuiEvent &event);
   void onMouseLeave(const GuiEvent &event);
   void onRightMouseDown(const GuiEvent &event);
   bool onMouseWheelUp(const GuiEvent &event);
   bool onMouseWheelDown(const GuiEvent &event);
   void getCursor(GuiCursor *&cursor, bool &showCursor, const GuiEvent &lastGuiEvent);

   /// The one awake on this client, which its list keeps step with.
   static GuiCommanderMap *get() { return smInstance; }

   /// @name Selection, which the list shares
   /// @{
   bool isSelected(const CommanderRef &ref) const;
   void select(const CommanderRef &ref, bool on);
   void clearSelection();
   const Vector<CommanderRef> &getSelection() const { return mSelection; }
   /// The order's subject at a point on the screen, through script.
   void issueCommand(const CommanderRef &ref, const Point2I &globalPoint);
   /// @}

   void setMouseMode(MouseMode mode) { mMouseMode = mode; }
   MouseMode getMouseMode() const { return mMouseMode; }
   void cameraMove(U32 dir, bool on);
   void resetCamera();
   void followLastSelected();
   void selectControlObject();
   void setTypeVisible(U32 type, bool visible);

   /// The ghost wearing a target, found once a frame.
   static ShapeBase *ghostOf(S32 target);
   /// Whether this client's team is told about a target at all.
   static bool isKnown(S32 target);

   DECLARE_CONOBJECT(GuiCommanderMap);

   enum Direction { Left, Right, Up, Down, In, Out, NumDirections };

private:
   struct Icon
   {
      CommanderRef ref;
      Point3F pos;
      CommanderIconData *data;
      ColorI teamColor;
      RectI rect;
      ShapeBase *shape;
      bool isPlayer;
   };

   static GuiCommanderMap *smInstance;

   /// @name Fields
   /// @{
   StringTableEntry mCursorNames[7];
   ColorI mMouseSelectRectColor;
   F32  mCameraOffset;
   S32  mCameraZoomTime;
   S32  mCameraMoveTime;
   StringTableEntry mDefaultIconName;
   StringTableEntry mWaypointIconName;
   StringTableEntry mAssignedTaskIconName;
   StringTableEntry mPotentialTaskIconName;
   S32  mSensorSphereFrameAlpha;
   S32  mSensorSphereFillAlpha;
   S32  mSensorSphereDetailLevel;
   bool mRenderMissionArea;
   ColorI mMissionAreaFillColor;
   ColorI mMissionAreaFrameColor;
   bool mRenderText;
   S32  mTextOffset;
   ColorI mHilightedObjectColor;
   ColorI mSelectedObjectColor;
   F32  mPlayerMarkerAngle;
   F32  mPlayerMarkerOffset;
   F32  mPlayerMarkerLen;
   ColorI mPlayerMarkerColor;
   bool mEnableEdgeMarkers;
   Point2I mEdgeMarkerSize;
   StringTableEntry mEdgeMarkerTextureName;
   bool mRenderSensors;
   S32  mMinIconSize;
   S32  mMaxIconSize;
   F32  mIconProjLen;
   F32  mMinDistanceScale;
   F32  mMaxDistanceScale;
   F32  mScreenFrameSize;
   /// @}

   MouseMode mMouseMode;
   Vector<CommanderRef> mSelection;
   CommanderRef mLastSelected;
   CommanderRef mHilight;
   CommanderRef mFollow;
   Vector<U32> mHiddenTypes;

   /// The camera: where it is over the map, and the box it is kept in.
   Point3F mCamera;
   Point3F mCameraMin;
   Point3F mCameraMax;
   bool    mHaveBounds;
   RectI   mBoundsArea;
   bool    mResetPending;
   /// The tallest thing in the mission area, plus cameraOffset.
   F32     mTopZ;
   bool    mMoving[NumDirections];
   U32     mLastUpdate;
   /// A click's zoom, eased over cameraZoomTime.
   bool    mZooming;
   Point3F mZoomFrom;
   Point3F mZoomTo;
   U32     mZoomStart;

   /// The mouse.
   bool    mDragging;
   bool    mDragSelect;
   Point2I mMouseDown;
   Point2I mMousePos;
   Point3F mDragGrab;
   bool    mMouseInside;

   Vector<Icon> mIcons;
   GFXTexHandle mEdgeMarker;
   GFXStateBlockRef mBlendBlock;

   GuiCursor *cursor(U32 which) const;
   void computeBounds();
   void clampCamera();
   void updateCamera();
   F32  viewWidthAt(F32 z) const;
   F32  groundHeight() const { return mCameraMin.z - mCameraOffset; }
   bool screenToGround(const Point2I &local, Point3F *out);
   bool worldToScreen(const Point3F &pos, Point2F *out) const;

   void collectIcons();
   const Icon *iconAt(const Point2I &local) const;
   S32  iconSize(const Point3F &pos) const;
   F32  textFade() const;
   CommanderIconData *iconFor(const char *name) const;

   void renderMissionArea(const RectI &bounds);
   void renderSensors(const RectI &bounds);
   void renderIcon(const Icon &icon);
   void renderPlayerMarker(const Icon &icon);
   void renderEdgeMarker(const Icon &icon, const RectI &bounds);
   void fillPolygon(const Point2F *points, U32 count, const ColorI &color);

   void selectOnly(const CommanderRef &ref);
   void zoomTo(const Point3F &target);
   void onSelectCallback(const CommanderRef &ref, bool on);
};

#endif
