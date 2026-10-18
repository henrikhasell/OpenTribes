#include "gui/hud/hudItemCtrl.h"

#include "console/consoleTypes.h"
#include "core/strings/stringUnit.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "platform/platform.h"
#include "sim/netConnection.h"

IMPLEMENT_CONOBJECT(HudScoreCtrl);
IMPLEMENT_CONOBJECT(HudNetDisplay);

//-----------------------------------------------------------------------------
HudScoreCtrl::HudScoreCtrl()
{
   VECTOR_SET_ASSOCIATION(mSeparators);

   mVerticalInset.set(0, 0);
   mHorizontalInset.set(0, 0);
   mHorzSeparator = false;
   mHasArt = false;
}

void HudScoreCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("verticalInset",   TypePoint2I, Offset(mVerticalInset, HudScoreCtrl));
   addField("horizontalInset", TypePoint2I, Offset(mHorizontalInset, HudScoreCtrl));
}

bool HudScoreCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   mHasArt = mProfile->constructBitmapArray() >= 3;
   if(mHasArt)
   {
      for(U32 i = 0; i < 3; i++)
      {
         mArt[i] = mProfile->mBitmapArrayRects[i];
         mArt[i].inset(1, 1);
      }

      setMinExtent(Point2I(mArt[0].extent.x + mArt[2].extent.x, mArt[0].extent.y));
      resize(getPosition(), getExtent());
   }

   return true;
}

void HudScoreCtrl::setSeparators(const char *list)
{
   mSeparators.clear();

   const U32 count = list ? StringUnit::getUnitCount(list, " ") : 0;
   for(U32 i = 0; i < count; i++)
      mSeparators.push_back(dAtoi(StringUnit::getUnit(list, i, " ")));

   setUpdate();
}

void HudScoreCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const Point2I extent = getExtent();

   if(mHasArt && mProfile->mTextureObject)
   {
      draw->clearBitmapModulation();
      draw->drawBitmapSR(mProfile->mTextureObject, offset, mArt[0]);

      const S32 middle = extent.x - mArt[0].extent.x - mArt[2].extent.x;
      const Point2I at(offset.x + mArt[0].extent.x, offset.y);

      draw->drawBitmapStretchSR(mProfile->mTextureObject,
         RectI(at, Point2I(middle, mArt[1].extent.y)), mArt[1]);

      draw->drawBitmapSR(mProfile->mTextureObject,
         Point2I(at.x + middle, offset.y), mArt[2]);
   }
   else
      Parent::onRender(offset, updateRect);

   const ColorI &edge = mProfile->mBorderColor;
   const ColorI shadow(U8(edge.red   * 0.75f + 0.5f),
                       U8(edge.green * 0.75f + 0.5f),
                       U8(edge.blue  * 0.75f + 0.5f),
                       edge.alpha);

   for(U32 pass = 0; pass < 2; pass++)
   {
      const ColorI &colour = pass ? edge : shadow;
      const S32 step = pass ? 1 : 2;

      if(mHorzSeparator)
      {
         const S32 y = offset.y + extent.y / 2 + step;
         draw->drawLine(Point2I(offset.x + mHorizontalInset.x, y),
                        Point2I(offset.x + extent.x - mHorizontalInset.y, y),
                        colour);
      }

      for(U32 i = 0; i < mSeparators.size(); i++)
      {
         const S32 x = mSeparators[i];
         if(x <= 0 || x >= extent.x - 2)
            continue;

         draw->drawLine(Point2I(offset.x + x + step - 1, offset.y + mVerticalInset.x),
                        Point2I(offset.x + x + step - 1, offset.y + extent.y - mVerticalInset.y),
                        colour);
      }
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudNetDisplay::HudNetDisplay()
{
   VECTOR_SET_ASSOCIATION(mHistory);

   mInfoCallback = StringTable->insert("");
   mHistorySize = 64;
   mUpdatePeriod = 500;
   mRenderGraph = true;
   mLastSample = 0;
}

void HudNetDisplay::initPersistFields()
{
   Parent::initPersistFields();

   addField("infoCallback", TypeCaseString, Offset(mInfoCallback, HudNetDisplay));
   addField("historySize",  TypeS32,        Offset(mHistorySize, HudNetDisplay));
   addField("updatePeriod", TypeS32,        Offset(mUpdatePeriod, HudNetDisplay));
   addField("renderGraph",  TypeBool,       Offset(mRenderGraph, HudNetDisplay));
}

void HudNetDisplay::init()
{
   mHistory.clear();
   mLastSample = 0;
}

void HudNetDisplay::uninit()
{
   mHistory.clear();
}

void HudNetDisplay::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());
   draw->drawRectFill(bounds, faded(mFillColor));

   // One sample per period, of whatever the connection says the round trip is.
   const U32 now = Platform::getVirtualMilliseconds();
   if(mUpdatePeriod > 0 && now - mLastSample >= U32(mUpdatePeriod))
   {
      mLastSample = now;

      F32 ping = 0.f;
      if(NetConnection *conn = NetConnection::getConnectionToServer())
         ping = conn->getRoundTripTime();

      mHistory.push_back(ping);
      while(S32(mHistory.size()) > mHistorySize)
         mHistory.pop_front();
   }

   if(mRenderGraph && mHistory.size() > 1)
   {
      // Scaled to the worst sample in the window, so the shape of the trace
      // stays readable whatever the connection is doing.
      F32 worst = 1.f;
      for(S32 i = 0; i < S32(mHistory.size()); i++)
         worst = getMax(worst, mHistory[i]);

      const ColorI line = faded(mFrameColor);
      const S32 step = getMax(getExtent().x / mHistorySize, 1);

      for(S32 i = 1; i < S32(mHistory.size()); i++)
      {
         const S32 x0 = offset.x + (i - 1) * step;
         const S32 x1 = offset.x + i * step;
         const S32 y0 = offset.y + getExtent().y
                      - S32(mHistory[i - 1] / worst * F32(getExtent().y));
         const S32 y1 = offset.y + getExtent().y
                      - S32(mHistory[i] / worst * F32(getExtent().y));

         draw->drawLine(x0, y0, x1, y1, line);
      }
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudScoreCtrl, setSeparators, void, (const char *offsets), ,
   "@brief Put a column rule at each x offset in the list.\n\n@ingroup GuiHud")
{
   object->setSeparators(offsets);
}

DefineEngineMethod(HudScoreCtrl, enableHorzSeparator, void, (), ,
   "@brief Draw the rule between the two teams.\n\n@ingroup GuiHud")
{
   object->setHorzSeparator(true);
}

DefineEngineMethod(HudScoreCtrl, disableHorzSeparator, void, (), ,
   "@brief Stop drawing the rule between the two teams.\n\n@ingroup GuiHud")
{
   object->setHorzSeparator(false);
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudNetDisplay, init, void, (), ,
   "@brief Start the trace over.\n\n@ingroup GuiHud")
{
   object->init();
}

DefineEngineMethod(HudNetDisplay, uninit, void, (), ,
   "@brief Throw the trace away.\n\n@ingroup GuiHud")
{
   object->uninit();
}

DefineEngineMethod(HudNetDisplay, updateToggles, void, (), ,
   "@brief Re-read what to show.\n\n@ingroup GuiHud")
{
   object->setUpdate();
}

DefineEngineMethod(HudNetDisplay, getPrefs, const char *, (), ,
   "@brief What is being shown.  Nothing reads this yet.\n\n@ingroup GuiHud")
{
   return "";
}
