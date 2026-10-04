#include "gui/hud/hudCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "platform/platform.h"

IMPLEMENT_CONOBJECT(HudCtrl);
IMPLEMENT_CONOBJECT(HudBitmapCtrl);
IMPLEMENT_CONOBJECT(HudPulsingBitmap);
IMPLEMENT_CONOBJECT(HudCrosshair);
IMPLEMENT_CONOBJECT(HudClock);
IMPLEMENT_CONOBJECT(HudFancyCtrl);
IMPLEMENT_CONOBJECT(HudHorzCtrl);
IMPLEMENT_CONOBJECT(HudHelpTag);

//-----------------------------------------------------------------------------
HudCtrl::HudCtrl()
{
   mFillColor.set(0.f, 0.f, 0.f, 0.f);
   mFrameColor.set(0.f, 1.f, 0.f, 1.f);
   mOpacity = 1.f;

   mAutoCenter = false;
   mAutoResize = false;
   mFlipHorizontal = false;
   mFlipVertical = false;

   mBitmapName = StringTable->insert("");
}

void HudCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("fillColor",      TypeColorF,   Offset(mFillColor, HudCtrl));
   addField("frameColor",     TypeColorF,   Offset(mFrameColor, HudCtrl));
   addField("opacity",        TypeF32,      Offset(mOpacity, HudCtrl));
   addField("autoCenter",     TypeBool,     Offset(mAutoCenter, HudCtrl));
   addField("autoResize",     TypeBool,     Offset(mAutoResize, HudCtrl));
   addField("flipHorizontal", TypeBool,     Offset(mFlipHorizontal, HudCtrl));
   addField("flipVertical",   TypeBool,     Offset(mFlipVertical, HudCtrl));
   addField("bitmap",         TypeFilename, Offset(mBitmapName, HudCtrl));
}

bool HudCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   if(mBitmapName[0])
      setBitmap(mBitmapName);

   return true;
}

void HudCtrl::setBitmap(const char *name)
{
   mBitmapName = StringTable->insert(name ? name : "", true);

   if(!mBitmapName[0])
   {
      mTexture = NULL;
      return;
   }

   mTexture.set(mBitmapName, &GFXDefaultGUIProfile,
                avar("%s() - mTexture (line %d)", __FUNCTION__, __LINE__));

   // autoResize means the control is the size of what it shows, which is how
   // the HUD's icons get their extent -- the layout gives them none.
   if(mAutoResize && mTexture)
      resize(getPosition(), Point2I(mTexture->getBitmapWidth(), mTexture->getBitmapHeight()));

   setUpdate();
}

GFXBitmapFlip HudCtrl::flipFlags() const
{
   if(mFlipHorizontal && mFlipVertical)
      return GFXBitmapFlip_XY;
   if(mFlipHorizontal)
      return GFXBitmapFlip_X;
   if(mFlipVertical)
      return GFXBitmapFlip_Y;
   return GFXBitmapFlip_None;
}

RectI HudCtrl::artRect(const Point2I &offset, const Point2I &size) const
{
   if(!mAutoCenter)
      return RectI(offset, size);

   return RectI(offset.x + (getExtent().x - size.x) / 2,
                offset.y + (getExtent().y - size.y) / 2,
                size.x, size.y);
}

ColorI HudCtrl::faded(const ColorF &colour) const
{
   ColorF c = colour;
   c.alpha *= mClampF(mOpacity, 0.f, 1.f);
   return ColorI(c);
}

ColorI HudCtrl::faded(const ColorI &colour) const
{
   ColorI c = colour;
   c.alpha = U8(F32(c.alpha) * mClampF(mOpacity, 0.f, 1.f));
   return c;
}

void HudCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());

   const ColorI fill = faded(mFillColor);
   if(fill.alpha)
      draw->drawRectFill(bounds, fill);

   if(mTexture)
   {
      draw->setBitmapModulation(ColorI(255, 255, 255,
                                       U8(mClampF(mOpacity, 0.f, 1.f) * 255.f)));
      draw->drawBitmapStretch(mTexture,
                              artRect(offset, Point2I(mTexture->getBitmapWidth(),
                                                      mTexture->getBitmapHeight())),
                              flipFlags());
      draw->clearBitmapModulation();
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudPulsingBitmap::HudPulsingBitmap()
{
   mColor.set(255, 255, 255, 255);
   mJamColor.set(255, 255, 0, 255);
   mPingColor.set(255, 0, 0, 255);

   mJam = false;
   mPing = false;
   mPulse = 0.f;
   mPulseRate = 1000.f;
   mPulseStart = 0;
}

void HudPulsingBitmap::initPersistFields()
{
   Parent::initPersistFields();

   addField("color",     TypeColorI, Offset(mColor, HudPulsingBitmap));
   addField("jamColor",  TypeColorI, Offset(mJamColor, HudPulsingBitmap));
   addField("pingColor", TypeColorI, Offset(mPingColor, HudPulsingBitmap));
   addField("jam",       TypeBool,   Offset(mJam, HudPulsingBitmap));
   addField("ping",      TypeBool,   Offset(mPing, HudPulsingBitmap));
   addField("pulse",     TypeF32,    Offset(mPulse, HudPulsingBitmap));
   addField("pulseRate", TypeF32,    Offset(mPulseRate, HudPulsingBitmap));
}

void HudPulsingBitmap::update()
{
   mPulseStart = Platform::getVirtualMilliseconds();
   setUpdate();
}

void HudPulsingBitmap::onRender(Point2I offset, const RectI &updateRect)
{
   if(!mTexture)
   {
      renderChildControls(offset, updateRect);
      return;
   }

   // Jammed beats pinging beats resting: what the sensor is doing matters more
   // than what it is showing.
   ColorI colour = mJam ? mJamColor : (mPing ? mPingColor : mColor);

   // The pulse is a fade in and out over one rate, running from whenever
   // update() last restarted it.
   F32 alpha = 1.f;
   if(mPulseRate > 0.f && mPulseStart)
   {
      const F32 age = F32(Platform::getVirtualMilliseconds() - mPulseStart);
      const F32 phase = mFmod(age, mPulseRate) / mPulseRate;
      alpha = 1.f - mFabs(phase * 2.f - 1.f);
   }

   colour.alpha = U8(F32(colour.alpha) * alpha * mClampF(mOpacity, 0.f, 1.f));

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->setBitmapModulation(colour);
   draw->drawBitmapStretch(mTexture,
                           artRect(offset, Point2I(mTexture->getBitmapWidth(),
                                                   mTexture->getBitmapHeight())),
                           flipFlags());
   draw->clearBitmapModulation();

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudCrosshair::HudCrosshair()
{
   // A reticle is centred on where it is aimed, never on the top-left corner
   // of a box.
   mAutoCenter = true;
}

//-----------------------------------------------------------------------------
HudClock::HudClock()
{
   mSubRegion.set(0, 0, 0, 0);
   mTime = -1;
   mSetAt = 0;
}

void HudClock::initPersistFields()
{
   Parent::initPersistFields();

   addField("subRegion", TypeRectI, Offset(mSubRegion, HudClock));
}

void HudClock::setTime(S32 seconds)
{
   mTime = seconds;
   mSetAt = Platform::getVirtualMilliseconds();
   setUpdate();
}

void HudClock::onRender(Point2I offset, const RectI &updateRect)
{
   Parent::onRender(offset, updateRect);

   if(mTime < 0 || !mProfile || !mProfile->mFont)
      return;

   // Script sets the clock once and lets it run, so what is on it now is what
   // it was set to less however long ago that was.
   const S32 elapsed = S32((Platform::getVirtualMilliseconds() - mSetAt) / 1000);
   const S32 left = getMax(mTime - elapsed, 0);

   char buf[32];
   dSprintf(buf, sizeof(buf), "%d:%02d", left / 60, left % 60);

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->setBitmapModulation(faded(mFrameColor));
   renderJustifiedText(offset, getExtent(), buf);
   draw->clearBitmapModulation();
}

//-----------------------------------------------------------------------------
HudFancyCtrl::HudFancyCtrl()
{
   setExtent(100, 100);
}

void HudHorzCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------
HudHelpTag::HudHelpTag()
{
   mFlip = false;
   mText = StringTable->insert("");
   setExtent(16, 16);
}

void HudHelpTag::initPersistFields()
{
   Parent::initPersistFields();

   addField("flip", TypeBool,       Offset(mFlip, HudHelpTag));
   addField("text", TypeCaseString, Offset(mText, HudHelpTag));
}

void HudHelpTag::onRender(Point2I offset, const RectI &updateRect)
{
   // Silent unless it has been given something to say.  Seventy-two of these
   // sit in PlayGui and all but one are quiet at any moment.
   if(!mText[0])
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());

   draw->drawRectFill(bounds, faded(mFillColor));
   draw->drawRect(bounds, faded(mFrameColor));

   if(mProfile && mProfile->mFont)
   {
      draw->setBitmapModulation(mProfile->mFontColor);
      renderJustifiedText(offset + Point2I(2, 1), getExtent() - Point2I(4, 2), mText);
      draw->clearBitmapModulation();
   }
}

//-----------------------------------------------------------------------------
DefineEngineMethod(HudCtrl, setBitmap, void, (const char *bitmap), ,
   "@brief Show this bitmap.\n\n@ingroup GuiHud")
{
   object->setBitmap(bitmap);
}

DefineEngineMethod(HudPulsingBitmap, update, void, (), ,
   "@brief Restart the pulse.\n\n@ingroup GuiHud")
{
   object->update();
}

DefineEngineMethod(HudClock, setTime, void, (S32 seconds), ,
   "@brief Start the clock at this many seconds; negative stops it.\n\n"
   "@ingroup GuiHud")
{
   object->setTime(seconds);
}
