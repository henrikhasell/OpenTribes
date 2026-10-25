// HH: Added for Tribes 2 compatibility.
#ifndef _TARGETST2_H_
#define _TARGETST2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class ShapeBase;

namespace T2Targets
{
   /// The sensor group of a target, or 0 for none -- which is what
   /// GameBase::getSensorGroup (0x005e3490) answers for an object with no
   /// target.
   U32 sensorGroup(S32 target);

   /// Put the skin a target carries on the shape that wears the target, which
   /// is what makes a flag its team's and a player their team's. Does nothing
   /// for a shape with no target, or a target with no skin.
   void applySkin(ShapeBase *shape);
}

#endif
