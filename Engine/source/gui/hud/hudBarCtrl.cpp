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
IMPLEMENT_CONOBJECT(HudCapacitor);
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

ColorI HudBarBaseCtrl::barColor()
{
   return mFillColor;
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

/// HudBarBaseCtrl::onRender at 0x00500ed0 draws the fill and returns: no frame,
/// no art and no children, whatever the .gui sets.
void HudBarBaseCtrl::onRender(Point2I offset, const RectI &updateRect)
{
      // Retail draws nothing at all without an object (0x00500efc).
   if(!controlObject())
      return;

   const F32 value = barValue();

      // What fills is the sub region, which Tribes 2 leaves the clip rect to cut
      // back to the control.
   S32 width = S32(F32(mSubRegion.extent.x) * value);
   if(width == 1)
      width = 2;      // 0x00500f7d: the first sliver of a gauge is visible

   RectI filled(offset + mSubRegion.point, Point2I(width, mSubRegion.extent.y));
   if(filled.extent.x <= 0 || filled.extent.y <= 0)
      return;
   if(!filled.intersect(RectI(offset, getExtent())))
      return;

   ColorI colour = barColor();
   colour.alpha = U8(255.f * mClampF(mOpacity, 0.f, 1.f) * pulse());

   GFX->getDrawUtil()->drawRectFill(filled, colour);
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

/// Retail picks the colour off the health at 0x00501580.
ColorI HudDamage::barColor()
{
   const F32 health = barValue();
   if(health >= 0.6f)
      return Parent::barColor();

   return health > sDamageWarning ? ColorI(255, 178, 0) : ColorI(255, 0, 0);
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
/// HudCapacitor::barValue at 0x00501470.  Unlike its siblings it ignores
/// displayMounted and a camera's orbit object, and never warns: it reads the
/// turret at node 10 of whatever the control object rides, which is where
/// the scripts mount the bomber's and the tank's (vehicle.cs:398, 443).
F32 HudCapacitor::barValue()
{
   ShapeBase *object = controlObject();
   if(!object)
      return 0.f;

   SceneObject *mount = object->getObjectMount();
   if(!mount)
      return 0.f;

   ShapeBase *turret = dynamic_cast<ShapeBase*>(mount->getMountNodeObject(10));
   if(!turret)
      return 0.f;

      // Capped at one before it is floored at zero (0x00501503, 0x00501522),
      // so a NaN reads empty.
   F32 value = turret->getCapacitorValue();
   if(value > 1.f)
      value = 1.f;
   return value > 0.f ? value : 0.f;
}

//-----------------------------------------------------------------------------
HudZoom::HudZoom()
{
}

/// HudZoom::onRender at 0x0050c2d0.  With no art it draws four corners, not
/// HudCtrl's fill and frame, in the profile's border colour, its alpha
/// included, whatever the control's opacity says.  The corners are built at
/// 0x0050c3f0 from the same vertices as these, each leg a quarter of its side
/// and rounded towards zero.
void HudZoom::onRender(Point2I offset, const RectI &updateRect)
{
   if(mTexture || !mProfile)
   {
      Parent::onRender(offset, updateRect);
      return;
   }

   const Point2I extent = getExtent();
   const S32 legX = extent.x / 4;
   const S32 legY = extent.y / 4;

   const S32 left   = offset.x + 1;
   const S32 top    = offset.y + 1;
   const S32 right  = offset.x - 1 + extent.x;
   const S32 bottom = offset.y - 1 + extent.y;

   const Point2I corners[4][3] =
   {
      { Point2I(left, top + legY),     Point2I(left, top),     Point2I(left + legX, top) },
      { Point2I(right - legX, top),    Point2I(right, top),    Point2I(right, top + legY) },
      { Point2I(right, bottom - legY), Point2I(right, bottom), Point2I(right - legX, bottom) },
      { Point2I(left + legX, bottom),  Point2I(left, bottom),  Point2I(left, bottom - legY) },
   };

   GFXDrawUtil *draw = GFX->getDrawUtil();
   const ColorI colour = mProfile->mBorderColor;

   for(U32 c = 0; c < 4; c++)
   {
      draw->drawLine(corners[c][0], corners[c][1], colour);
      draw->drawLine(corners[c][1], corners[c][2], colour);
   }
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
