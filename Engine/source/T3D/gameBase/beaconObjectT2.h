// HH: Added for Tribes 2 compatibility.
#ifndef _BEACONOBJECTT2_H_
#define _BEACONOBJECTT2_H_

#ifndef _STATICSHAPE_H_
#include "T3D/staticShape.h"
#endif

/// A beacon a player has put down: a shape that stands where it was placed,
/// and that the hud marks for the team it belongs to.
class BeaconObject : public StaticShape
{
   typedef StaticShape Parent;

public:
   /// What the beacon is for, in the order the wire and script name them.
   enum BeaconType
   {
      Enemy,      ///< a target beacon
      Friend,     ///< a marker beacon
      Vehicle,    ///< a bomb target
      NumBeaconTypes,
   };

private:
   U32 mBeaconType;

public:
   BeaconObject();

   U32 getBeaconType() const { return mBeaconType; }
   void setBeaconType(U32 type);

   void onT2BodyRead(const T2Layout::Body &body) override;

   DECLARE_CONOBJECT(BeaconObject);
};

#endif
