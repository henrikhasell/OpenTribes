// HH: Added for Tribes 2 compatibility.
#ifndef _HUDBOMBSIGHT_H_
#define _HUDBOMBSIGHT_H_

#ifndef _HUDCTRL_H_
#include "gui/hud/hudCtrl.h"
#endif

class BombSight;

/// @file
/// The sight a bomber drops by (hudBombSight.cc, 0xc4 bytes, built at
/// 0x00509f30), which clientCmdStartBomberSight puts on PlayGui.
///
/// The control draws nothing of its own.  Each frame it follows a bomb from
/// whatever the player sits in down to the first ground in its way, and moves
/// a BombSight there, fading it in over the height `minDisplayHeight` names.

class HudBombSight : public HudCtrl
{
   typedef HudCtrl Parent;

   BombSight *mSight;

   StringTableEntry mShapeName;
   F32 mMinDisplayHeight;

public:
   HudBombSight();

   DECLARE_CONOBJECT(HudBombSight);
   static void initPersistFields();

   bool onWake();
   void onSleep();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif // _HUDBOMBSIGHT_H_
