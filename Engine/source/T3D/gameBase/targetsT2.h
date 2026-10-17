// HH: Added for Tribes 2 compatibility.
#ifndef _TARGETST2_H_
#define _TARGETST2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class GameBase;

/// The client's half of Tribes 2's target manager: what the server has told
/// it about each target slot, and which ghost is wearing it.
namespace TargetsT2
{
   enum
   {
      TargetBits = 9,
      MaxTargets = 1 << TargetBits,

      /// The slots below this are the sensor groups' own and are never worn.
      FirstObjectTarget = 32,
   };

   /// Say that \a object now wears \a target, or wears none for -1.
   void setObjectTarget(GameBase *object, S32 previous, S32 target);
}

#endif
