// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/beaconObjectT2.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "T3D/gameBase/t2Layout.h"

IMPLEMENT_CO_NETOBJECT_V1(BeaconObject);

ConsoleDocClass( BeaconObject,
   "@brief A Tribes 2 deployed beacon: a shape its team sees marked on the "
   "hud.\n\n@internal" );

/// The names script gives the types (0x007c3340).
static const char *sBeaconTypeNames[BeaconObject::NumBeaconTypes] =
{
   "enemy",
   "friend",
   "vehicle",
};

/// 0x006a38e0: a beacon is in scope everywhere, so a team-mate is marked to
/// it from across the map.
BeaconObject::BeaconObject()
   : mBeaconType(Enemy)
{
   mNetFlags.set(Ghostable | ScopeAlways);
}

void BeaconObject::setBeaconType(U32 type)
{
   mBeaconType = type;
}

/// 0x006a3ae0: after StaticShape's body, a flag and then the type.
void BeaconObject::onT2BodyRead(const T2Layout::Body &body)
{
   Parent::onT2BodyRead(body);

   if(body.hasBeaconType)
      setBeaconType(body.beaconType);
}

DefineEngineMethod(BeaconObject, getBeaconType, const char *, (), ,
   "@brief The beacon's type: enemy, friend or vehicle.\n\n@internal")
{
   return sBeaconTypeNames[object->getBeaconType()];
}

/// 0x006a3b90: only the server's beacon takes a type from script.
DefineEngineMethod(BeaconObject, setBeaconType, void, (const char *type), ,
   "@brief Make the beacon an enemy, friend or vehicle one.\n\n@internal")
{
   for(U32 i = 0; i < BeaconObject::NumBeaconTypes; i++)
      if(!dStricmp(type, sBeaconTypeNames[i]))
      {
         if(object->isServerObject())
            object->setBeaconType(i);
         return;
      }

   Con::errorf("BeaconObject::cGetBeaconType: invalid beacon type [%s]", type);
}
