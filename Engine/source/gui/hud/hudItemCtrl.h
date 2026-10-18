// HH: Added for Tribes 2 compatibility.
#ifndef _HUDITEMCTRL_H_
#define _HUDITEMCTRL_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

/// @file
/// The objective panel across the top of a Tribes 2 screen and the connection
/// graph in its corner.  The three rows of icons are in hudWeaponInvBase.h.

//-----------------------------------------------------------------------------
/// The objective panel: the two team names, their scores and where the flags
/// are.
///
/// Every field on it is a tagged string id rather than the text itself, which
/// is how a server that has already sent the string sends the line that uses
/// it in a few bits.  A container, because the layout nests the labels inside.
class HudScoreCtrl : public HudCtrl
{
   typedef HudCtrl Parent;

   Point2I mVerticalInset;
   Point2I mHorizontalInset;

   Vector<S32> mSeparators;
   bool mHorzSeparator;

   RectI mArt[3];
   bool mHasArt;

public:
   HudScoreCtrl();

   DECLARE_CONOBJECT(HudScoreCtrl);
   static void initPersistFields();

   bool onWake();

   void setSeparators(const char *list);
   void setHorzSeparator(bool on) { mHorzSeparator = on; setUpdate(); }

   void onRender(Point2I offset, const RectI &updateRect);
};

//-----------------------------------------------------------------------------
/// The connection graph -- ping, packet loss and the rest -- that Tribes 2
/// draws over the corner of the screen when it is asked to.
class HudNetDisplay : public HudCtrl
{
   typedef HudCtrl Parent;

   StringTableEntry mInfoCallback;
   S32 mHistorySize;
   S32 mUpdatePeriod;
   bool mRenderGraph;

   Vector<F32> mHistory;
   U32 mLastSample;

public:
   HudNetDisplay();

   DECLARE_CONOBJECT(HudNetDisplay);
   static void initPersistFields();

   void init();
   void uninit();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDITEMCTRL_H_
