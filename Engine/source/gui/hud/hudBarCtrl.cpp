#include "gui/hud/hudBarCtrl.h"

#include "T3D/camera.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/player.h"
#include "T3D/shapeBase.h"
#include "T3D/vehicles/vehicle.h"
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

/// The double constant at 0x0077b330 retail compares a damage bar against.
static const F32 sDamageWarning = 0.25f;

//-----------------------------------------------------------------------------
HudBarBaseCtrl::HudBarBaseCtrl()
{
   mValue = 0.f;
   mPulseRate = 500.f;
   mPulseThreshold = 0.3f;
   mVerticalFill = false;
   mDisplayMounted = false;
   mWarning = false;
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

//-----------------------------------------------------------------------------
ShapeBase *HudBarBaseCtrl::controlObject()
{
   GameConnection *conn = GameConnection::getConnectionToServer();
   if(!conn)
      return NULL;

   return dynamic_cast<ShapeBase*>(conn->getControlObject());
}

ShapeBase *HudBarBaseCtrl::watchedObject(Camera *camera)
{
      // Retail casts the orbit object to Player and to Vehicle and takes
      // whichever answers (0x00501271, 0x00501293).
   GameBase *orbit = camera->getOrbitObject();
   if(Player *player = dynamic_cast<Player*>(orbit))
      return player;
   return dynamic_cast<Vehicle*>(orbit);
}

ShapeBase *HudBarBaseCtrl::mountedObject(ShapeBase *object)
{
      // Retail walks to the far end of the mount chain (0x00501379).
   ShapeBase *mount = dynamic_cast<ShapeBase*>(object->getObjectMount());
   while(mount)
   {
      ShapeBase *next = dynamic_cast<ShapeBase*>(mount->getObjectMount());
      if(!next)
         break;
      mount = next;
   }
   return mount;
}

F32 HudBarBaseCtrl::barValue()
{
   return mValue;
}

F32 HudBarBaseCtrl::pulse() const
{
   if(mPulseRate <= 0.f || !mWarning)
      return 1.f;

   const F32 age = F32(Platform::getVirtualMilliseconds());
   const F32 phase = mFmod(age, mPulseRate) / mPulseRate;

      // Retail negates the alpha byte instead of scaling it (0x0050104c).
   return 1.f - mFabs(phase * 2.f - 1.f);
}

void HudBarBaseCtrl::onRender(Point2I offset, const RectI &updateRect)
{
      // Retail draws nothing at all without an object (0x00500efc).
   if(!controlObject())
      return;

   const F32 value = barValue();

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const RectI bounds(offset, getExtent());

   // How much of the bar is filled.  A vertical one fills from the bottom,
   // because that is the direction a gauge reads.
   const S32 across = S32(F32(getExtent().x) * value);
   const S32 up     = S32(F32(getExtent().y) * value);

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
            const S32 h = S32(F32(source.extent.y) * value);
            source.point.y += source.extent.y - h;
            source.extent.y = h;
         }
         else
            source.extent.x = S32(F32(source.extent.x) * value);

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
/// HudEnergy::barValue at 0x005011f0.
F32 HudEnergy::barValue()
{
   ShapeBase *object = controlObject();
   if(!object)
      return 0.f;

   if(Camera *camera = dynamic_cast<Camera*>(object))
      object = watchedObject(camera);
   else if(mDisplayMounted)
      object = mountedObject(object);

   return object ? mClampF(object->getEnergyValue(), 0.f, 1.f) : 0.f;
}

//-----------------------------------------------------------------------------
/// HudDamage::barValue at 0x00501680, which retail flashes off the health
/// (0x00501944).
F32 HudDamage::barValue()
{
   mWarning = false;

   ShapeBase *object = controlObject();
   if(!object)
      return 0.f;

   if(Camera *camera = dynamic_cast<Camera*>(object))
      object = watchedObject(camera);
   else if(mDisplayMounted)
      object = mountedObject(object);

   if(!object)
      return 0.f;

   const F32 health = mClampF(1.f - object->getDamageValue(), 0.f, 1.f);
   mWarning = health <= sDamageWarning;
   return health;
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

/// HudHeat::barValue at 0x00501a20, which ignores displayMounted (0x00501b4b).
F32 HudHeat::barValue()
{
   mWarning = false;

   ShapeBase *object = controlObject();
   if(!object)
      return 0.f;

   if(Camera *camera = dynamic_cast<Camera*>(object))
   {
      GameBase *orbit = camera->getOrbitObject();
      if(Player *player = dynamic_cast<Player*>(orbit))
         return mClampF(player->getHeat(), 0.f, 1.f);
      return dynamic_cast<Vehicle*>(orbit) ? 1.f : 0.f;
   }

   const F32 heat = object->getHeat();
   mWarning = heat > mHeatWarning;
   return mClampF(heat, 0.f, 1.f);
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
   "@brief How full the bar would draw this instant.\n\n@ingroup GuiHud")
{
   return object->barValue();
}
