#include "gui/hud/hudBarCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "platform/platform.h"

IMPLEMENT_CONOBJECT(HudBarBaseCtrl);
IMPLEMENT_CONOBJECT(HudEnergy);
IMPLEMENT_CONOBJECT(HudDamage);
IMPLEMENT_CONOBJECT(HudHeat);
IMPLEMENT_CONOBJECT(HudZoom);

//-----------------------------------------------------------------------------
HudBarBaseCtrl::HudBarBaseCtrl()
{
   mValue = 0.f;
   mPulseRate = 0.f;
   mPulseThreshold = 0.f;
   mVerticalFill = false;
   mDisplayMounted = false;
   mSubRegion.set(0, 0, 0, 0);
}

void HudBarBaseCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("value",          TypeF32,   Offset(mValue, HudBarBaseCtrl));
   addField("pulseRate",      TypeF32,   Offset(mPulseRate, HudBarBaseCtrl));
   addField("pulseThreshold", TypeF32,   Offset(mPulseThreshold, HudBarBaseCtrl));
   addField("verticalFill",   TypeBool,  Offset(mVerticalFill, HudBarBaseCtrl));
   addField("displayMounted", TypeBool,  Offset(mDisplayMounted, HudBarBaseCtrl));
   addField("subRegion",      TypeRectI, Offset(mSubRegion, HudBarBaseCtrl));
}

void HudBarBaseCtrl::setValue(F32 value)
{
   mValue = mClampF(value, 0.f, 1.f);
   setUpdate();
}

void HudBarBaseCtrl::setScriptValue(const char *value)
{
   setValue(dAtof(value));
}

const char *HudBarBaseCtrl::getScriptValue()
{
   char *buf = Con::getReturnBuffer(32);
   dSprintf(buf, 32, "%g", mValue);
   return buf;
}

F32 HudBarBaseCtrl::pulse() const
{
   if(mPulseRate <= 0.f || mValue > mPulseThreshold)
      return 1.f;

   const F32 age = F32(Platform::getVirtualMilliseconds());
   const F32 phase = mFmod(age, mPulseRate) / mPulseRate;

   // A triangle rather than a sine: it is one multiply, and at the rates a HUD
   // pulses at nothing can tell the difference.
   return 1.f - mFabs(phase * 2.f - 1.f);
}

void HudBarBaseCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());

   // How much of the bar is filled.  A vertical one fills from the bottom,
   // because that is the direction a gauge reads.
   const S32 across = S32(F32(getExtent().x) * mValue);
   const S32 up     = S32(F32(getExtent().y) * mValue);

   const RectI filled = mVerticalFill
      ? RectI(bounds.point.x, bounds.point.y + getExtent().y - up, getExtent().x, up)
      : RectI(bounds.point, Point2I(across, getExtent().y));

   ColorF colour = mFillColor;
   colour.alpha *= pulse();

   if(filled.extent.x > 0 && filled.extent.y > 0)
   {
      if(mTexture)
      {
         // With art, the fill is how much of the art is shown rather than a
         // rectangle over it: subRegion says which part of the bitmap is the
         // bar, and the fill is a fraction of that.
         RectI source = mSubRegion.extent.x > 0 ? mSubRegion
                      : RectI(0, 0, mTexture->getBitmapWidth(), mTexture->getBitmapHeight());

         if(mVerticalFill)
         {
            const S32 h = S32(F32(source.extent.y) * mValue);
            source.point.y += source.extent.y - h;
            source.extent.y = h;
         }
         else
            source.extent.x = S32(F32(source.extent.x) * mValue);

         if(source.extent.x > 0 && source.extent.y > 0)
         {
            draw->setBitmapModulation(faded(colour));
            draw->drawBitmapStretchSR(mTexture, filled, source, flipFlags());
            draw->clearBitmapModulation();
         }
      }
      else
         draw->drawRectFill(filled, faded(colour));
   }

   if(mFrameColor.alpha > 0.f)
      draw->drawRect(bounds, faded(mFrameColor));

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudHeat::HudHeat()
{
   mHeatWarning = 1.f;
}

void HudHeat::initPersistFields()
{
   Parent::initPersistFields();

   addField("heatWarning", TypeF32, Offset(mHeatWarning, HudHeat));
}

//-----------------------------------------------------------------------------
HudZoom::HudZoom()
{
   mAutoCenter = true;
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudBarBaseCtrl, setValue, void, (F32 value), ,
   "@brief How full the bar is, 0 to 1.\n\n@ingroup GuiHud")
{
   object->setValue(value);
}

DefineEngineMethod(HudBarBaseCtrl, getValue, F32, (), ,
   "@brief How full the bar is.\n\n@ingroup GuiHud")
{
   return object->getValue();
}
